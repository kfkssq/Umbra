#include "Skills/UmbraSkillProjectile.h"
#include "Skills/UmbraSweepDefinition.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/UmbraAttackable.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Umbra.h"

AUmbraSkillProjectile::AUmbraSkillProjectile()
{
    bReplicates = true;
    SetReplicateMovement(true);
    PrimaryActorTick.bCanEverTick = true;
    SetNetUpdateFrequency(60.f);
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Flight = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Flight"));
    Flight->SetupAttachment(RootComponent);
    Flight->bAutoActivate = false;
    Flight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FlightAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FlightAudio"));
    FlightAudio->SetupAttachment(RootComponent);
    FlightAudio->bAutoActivate = false;
    FlightAudio->bStopWhenOwnerDestroyed = true;
    FlightNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlightNiagara"));
    FlightNiagara->SetupAttachment(RootComponent);
    FlightNiagara->bAutoActivate = false;
    FlightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlightMesh"));
    FlightMesh->SetupAttachment(RootComponent);
    FlightMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FlightMesh->SetCastShadow(false);
}

void AUmbraSkillProjectile::Initialize(const UUmbraSweepDefinition* InDefinition, FVector InDirection, AActor* InTarget, int32 InLevel)
{
    Definition = InDefinition;
    Direction = InDirection.GetSafeNormal();
    HomingTarget = InTarget;
    AbilityLevel = InLevel;
    Speed = Definition->ProjectileInitialSpeed;
}

void AUmbraSkillProjectile::BeginPlay()
{
    Super::BeginPlay();
    SetActorTickEnabled(HasAuthority());
    if (Definition && GetNetMode() != NM_DedicatedServer && !bImpacted)
    {
        if (Definition->ProjectileMesh)
        {
            FlightMesh->SetStaticMesh(Definition->ProjectileMesh);
            FlightMesh->SetRelativeScale3D(Definition->ProjectileMeshScale);
            if (Definition->ProjectileMaterial) FlightMesh->SetMaterial(0, Definition->ProjectileMaterial);
        }
        if (Definition->ProjectileNiagara)
        {
            FlightNiagara->SetAsset(Definition->ProjectileNiagara);
            FlightNiagara->Activate(true);
        }
        else
        {
            Flight->SetTemplate(Definition->ProjectileEffect);
            Flight->ActivateSystem();
        }
        if (Definition->ProjectileSound)
        {
            FlightAudio->SetSound(Definition->ProjectileSound);
            FlightAudio->Play();
        }
    }
    if (HasAuthority()) SetLifeSpan(Definition ? Definition->ProjectileLifetime + .1f : .1f);
    if (bImpacted) OnRep_Impact();
}

void AUmbraSkillProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FlightAudio->Stop();
    FlightNiagara->DeactivateImmediate();
    Super::EndPlay(EndPlayReason);
}

void AUmbraSkillProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Definition || bImpacted) return;
    // Bounded substeps retain steering/acceleration behavior under a slow server frame.
    float Remaining = FMath::Min(DeltaSeconds, Definition->ProjectileLifetime - Age);
    while (Remaining > KINDA_SMALL_NUMBER && !bImpacted)
    {
        const float Step = FMath::Min(Remaining, 1.f / 120.f);
        if (AActor* Target = HomingTarget.Get(); IsValid(Target) && Target->Implements<UUmbraAttackable>()
            && IUmbraAttackable::Execute_CanBeAttacked(Target))
            Direction = FMath::VInterpNormalRotationTo(Direction,
                (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal(), Step, Definition->ProjectileTurnRate);
        const float AcceleratingTime = FMath::Max(0.f, Age + Step - Definition->ProjectileAccelerationDelay)
            - FMath::Max(0.f, Age - Definition->ProjectileAccelerationDelay);
        const float NextSpeed = FMath::Min(Definition->ProjectileMaxSpeed, Speed + Definition->ProjectileAcceleration * AcceleratingTime);
        const FVector Next = GetActorLocation() + Direction * ((Speed + NextSpeed) * .5f * Step);
        FCollisionQueryParams Query(SCENE_QUERY_STAT(UmbraSkillProjectile), false, GetOwner());
        Query.AddIgnoredActor(this);
        FCollisionObjectQueryParams Objects;
        Objects.AddObjectTypesToQuery(ECC_WorldStatic);
        Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
        Objects.AddObjectTypesToQuery(ECC_Pawn);
        FHitResult Hit;
        const FCollisionShape Shape = Definition->ProjectileHalfWidth > 0.f
            ? FCollisionShape::MakeBox(FVector(Definition->ProjectileRadius, Definition->ProjectileHalfWidth, Definition->ProjectileRadius))
            : FCollisionShape::MakeSphere(Definition->ProjectileRadius);
        if (GetWorld()->SweepSingleByObjectType(Hit, GetActorLocation(), Next, Direction.Rotation().Quaternion(), Objects,
            Shape, Query)) Impact(Hit);
        else SetActorLocationAndRotation(Next, Direction.Rotation());
        Speed = NextSpeed;
        Age += Step;
        Remaining -= Step;
    }
    if (!bImpacted && Age >= Definition->ProjectileLifetime - KINDA_SMALL_NUMBER) Destroy();
}

void AUmbraSkillProjectile::Impact(const FHitResult& Hit)
{
    if (!HasAuthority() || bImpacted || !Definition) return;
    bImpacted = true; // set before GAS/events: no reentrant or repeated hits
    ImpactLocation = Hit.ImpactPoint;
    ImpactNormal = Hit.ImpactNormal.IsNearlyZero() ? -Direction : FVector(Hit.ImpactNormal);
    SetActorLocation(Hit.Location);
    AActor* Target = Hit.GetActor();
    bool bDamageApplied = false;
    if (IsValid(Target) && Target != GetOwner() && Target->Implements<UUmbraAttackable>()
        && IUmbraAttackable::Execute_CanBeAttacked(Target))
    {
        FUmbraDamageRequest Request(EUmbraDamageSource::Skill);
        Request.Config = Definition->Damage;
        Request.EffectClass = Definition->DamageEffect;
        Request.Level = AbilityLevel;
        bDamageApplied = UmbraDamage::Apply(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()),
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target), Request);
        if (bDamageApplied)
        {
            FGameplayEventData Event;
            Event.EventTag = UmbraGameplayTags::Event_Combat_HitReceived;
            Event.Instigator = GetOwner();
            Event.Target = Target;
            UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, Event.EventTag, Event);
        }
    }
    OnRep_Impact();
    ForceNetUpdate();
    SetActorTickEnabled(false);
    SetLifeSpan(.75f); // keep replicated impact state long enough to arrive before actor removal
    UE_LOG(LogUmbra, Log, TEXT("[Pebble] Impact target=%s damage=%d"), *GetNameSafe(Target), bDamageApplied);
}

void AUmbraSkillProjectile::OnRep_Impact()
{
    if (!bImpacted || bImpactPresented || !Definition) return;
    bImpactPresented = true;
    Flight->DeactivateSystem();
    Flight->SetVisibility(false);
    FlightNiagara->DeactivateImmediate();
    FlightNiagara->SetVisibility(false);
    FlightMesh->SetVisibility(false);
    FlightAudio->Stop();
    if (GetNetMode() == NM_DedicatedServer) return;
    if (Definition->ImpactNiagara) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Definition->ImpactNiagara,
        ImpactLocation, FVector(ImpactNormal).Rotation());
    else if (Definition->ImpactEffect) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Definition->ImpactEffect,
        ImpactLocation, FVector(ImpactNormal).Rotation(), FVector::OneVector, true);
    if (Definition->ImpactSound) UGameplayStatics::PlaySoundAtLocation(this, Definition->ImpactSound, ImpactLocation);
}

void AUmbraSkillProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AUmbraSkillProjectile, Definition);
    DOREPLIFETIME(AUmbraSkillProjectile, ImpactLocation);
    DOREPLIFETIME(AUmbraSkillProjectile, ImpactNormal);
    DOREPLIFETIME(AUmbraSkillProjectile, bImpacted);
}

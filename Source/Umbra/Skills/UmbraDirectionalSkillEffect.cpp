#include "Skills/UmbraDirectionalSkillEffect.h"
#include "Skills/UmbraSweepDefinition.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Interfaces/UmbraAttackable.h"
#include "EngineUtils.h"

AUmbraDirectionalSkillEffect::AUmbraDirectionalSkillEffect()
{
    PrimaryActorTick.bCanEverTick=true;
    Decal=CreateDefaultSubobject<UDecalComponent>(TEXT("Shape"));
    SetRootComponent(Decal);
}
void AUmbraDirectionalSkillEffect::Initialize(const UUmbraSweepDefinition* Data,AActor* Caster,FVector Origin,FVector Forward,bool bDamage)
{
    Definition=Data; Source=Caster; CastOrigin=Origin; Direction=Forward.GetSafeNormal2D(); bApplyDamage=bDamage;
    FVector Ground=Origin;
    if (const auto* C=Cast<ACharacter>(Caster)) Ground.Z-=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    SetActorLocation(Ground+FVector(0,0,8)); SetActorRotation(FRotator(-90,0,0));
    Decal->DecalSize=FVector(40,Data->DirectionExtent(),Data->DirectionExtent());
    Decal->SetFadeScreenSize(0.f);
    Material=UMaterialInstanceDynamic::Create(Data->TargetIndicatorMaterial,this);
    Decal->SetDecalMaterial(Material);
    Material->SetVectorParameterValue(TEXT("ShapeOrigin"),FLinearColor(Ground.X,Ground.Y,Ground.Z));
    Material->SetVectorParameterValue(TEXT("ShapeForward"),FLinearColor(Direction.X,Direction.Y,0));
    Material->SetVectorParameterValue(TEXT("Tint"),FLinearColor(1.f,.7f,.12f));
    Material->SetScalarParameterValue(TEXT("Radius"),Data->Radius);
    Material->SetScalarParameterValue(TEXT("HalfAngle"),FMath::DegreesToRadians(Data->ArcDegrees*.5f));
    Material->SetScalarParameterValue(TEXT("Length"),Data->TargetLineLength);
    Material->SetScalarParameterValue(TEXT("HalfWidth"),Data->TargetLineHalfWidth);
    Material->SetScalarParameterValue(TEXT("Shape"),Data->Execution==EUmbraSkillExecution::LinearWave ? 1.f : 0.f);
    Material->SetScalarParameterValue(TEXT("Wave"),Data->Execution==EUmbraSkillExecution::LinearWave ? 1.f : 0.f);
    Material->SetScalarParameterValue(TEXT("Fill"),.18f);
}
void AUmbraDirectionalSkillEffect::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Definition || !Source.IsValid()) { Destroy(); return; }
    Age+=DeltaSeconds;
    const bool Wave=Definition->Execution==EUmbraSkillExecution::LinearWave;
    const float Front=FMath::Min(Definition->TargetLineLength,Age*Definition->TargetWaveSpeed);
    if (Material) Material->SetScalarParameterValue(TEXT("Front"),Front);
    if (bApplyDamage && (Wave || !bResolved))
    {
        auto* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source.Get());
        FCollisionQueryParams Query(SCENE_QUERY_STAT(DirectionalSkill),false,Source.Get());
        Query.AddIgnoredActor(this);
        FCollisionObjectQueryParams WorldGeometry;
        WorldGeometry.AddObjectTypesToQuery(ECC_WorldStatic);
        WorldGeometry.AddObjectTypesToQuery(ECC_WorldDynamic);
        for (TActorIterator<APawn> It(GetWorld()); It; ++It)
        {
            AActor* Target=*It;
            if (Target==Source.Get() || HitActors.Contains(Target) || !Target->Implements<UUmbraAttackable>()
                || !IUmbraAttackable::Execute_CanBeAttacked(Target)
                || !Definition->ContainsTarget(CastOrigin,Direction,Target->GetActorLocation())) continue;
            const float Along=FVector::DotProduct(Target->GetActorLocation()-CastOrigin,Direction);
            // Sweep the advancing 30cm band between frames; never re-hit or hit behind the wave.
            if (Wave && (Along<FMath::Max(0.f,PreviousFront-30.f) || Along>Front)) continue;
            FHitResult Blocker;
            if (GetWorld()->LineTraceSingleByObjectType(Blocker,CastOrigin+FVector(0,0,50),
                Target->GetActorLocation()+FVector(0,0,50),WorldGeometry,Query) && Blocker.GetActor()!=Target) continue;
            FUmbraDamageRequest Request(EUmbraDamageSource::Skill);
            Request.Config=Definition->Damage; Request.EffectClass=Definition->DamageEffect; Request.Level=1.f;
            if (ASC && UmbraDamage::Apply(ASC,UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target),Request))
            {
                HitActors.Add(Target);
                FGameplayEventData Hit; Hit.EventTag=UmbraGameplayTags::Event_Combat_HitReceived;
                Hit.Instigator=Source.Get(); Hit.Target=Target;
                UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target,Hit.EventTag,Hit);
            }
        }
        bResolved=true;
    }
    PreviousFront=Front;
    if (Age>(Wave ? Definition->TargetLineLength/Definition->TargetWaveSpeed : .35f)) Destroy();
}

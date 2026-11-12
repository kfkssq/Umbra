#include "Skills/UmbraSkillTargetingComponent.h"
#include "Skills/UmbraSweepDefinition.h"
#include "Skills/UmbraTargetingPrompt.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "UmbraPlayerController.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Curves/CurveVector.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

UUmbraSkillTargetingComponent::UUmbraSkillTargetingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

bool UUmbraSkillTargetingComponent::EvaluateTarget(ACharacter* C, const UUmbraSweepDefinition* D,
    const FVector& Requested, bool bClamp, FVector& Ground, FText& Reason)
{
    const auto Fail = [&Reason](const TCHAR* Message) { Reason = FText::FromString(Message); return false; };
    Ground = Requested;
    if (!C || !D || !D->UsesTargeting() || Requested.ContainsNaN()) return Fail(TEXT("目标无效"));
    auto* Move = C->GetCharacterMovement();
    if (!Move->IsMovingOnGround()) return Fail(TEXT("需要站在地面上"));
    const FVector Start = C->GetActorLocation();
    if (D->bTargetedDirection)
    {
        const FVector Facing = (Requested-Start).GetSafeNormal2D();
        if (FVector::DistSquared2D(Requested,Start)<FMath::Square(10.f)) return Fail(TEXT("鼠标离角色太近，请选择方向"));
        Ground=Start+Facing*D->DirectionExtent();
        Ground.Z-=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        Reason=FText::FromString(TEXT("鼠标选择方向 · 左键确认 · 右键取消"));
        return true;
    }
    if (!D->TargetLeapCurve) return Fail(TEXT("缺少跃击轨迹"));
    float Radius, Half; C->GetCapsuleComponent()->GetScaledCapsuleSize(Radius, Half);
    FVector Delta = Requested - Start; Delta.Z = 0;
    if (!bClamp && Delta.SizeSquared() > FMath::Square(D->LeapDistance + 1.f)) return Fail(TEXT("超出距离"));
    Delta = Delta.GetClampedToMaxSize(D->LeapDistance);
    FVector XY = Start + Delta;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SkillTargetGround), false, C);
    FHitResult Floor;
    const float FeetZ = Start.Z - Half;
    Ground=FVector(XY.X,XY.Y,FeetZ);
    // Trace only world geometry; a pawn or loot under the cursor must not become a landing surface.
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    if (!C->GetWorld()->LineTraceSingleByObjectType(Floor, FVector(XY.X, XY.Y, FeetZ + D->TargetMaxHeightDifference + 10.f),
        FVector(XY.X, XY.Y, FeetZ - D->TargetMaxHeightDifference - 10.f), Objects, Query)) return Fail(TEXT("没有可落地表面"));
    Ground = Floor.ImpactPoint;
    if (!Move->IsWalkable(Floor) || FMath::Abs(Ground.Z - FeetZ) > D->TargetMaxHeightDifference) return Fail(TEXT("坡度或高度不允许落地"));
    if (!bClamp && FVector::DistSquared(Ground, Requested) > FMath::Square(5.f)) return Fail(TEXT("落点已改变，请重新选择"));
    const FVector End = Ground + FVector(0,0,Half + 2.5f);
    const auto Capsule = FCollisionShape::MakeCapsule(Radius, Half);
    if (C->GetWorld()->OverlapBlockingTestByChannel(End, FQuat::Identity, C->GetCapsuleComponent()->GetCollisionObjectType(), Capsule, Query))
        return Fail(TEXT("落点被阻挡"));
    // Same authored path as MoveToForce. 48 capsule sweeps cover the complete arc, not a ground navigation path.
    FVector Previous = Start;
    const FRotator Facing = (End - Start).GetSafeNormal2D().Rotation();
    for (int32 I=1; I<=48; ++I)
    {
        const float T = float(I)/48.f;
        const FVector Next = FMath::Lerp(Start, End, T) + Facing.RotateVector(D->TargetLeapCurve->GetVectorValue(T));
        FHitResult Hit;
        if (C->GetWorld()->SweepSingleByChannel(Hit, Previous, Next, FQuat::Identity,
            C->GetCapsuleComponent()->GetCollisionObjectType(), Capsule, Query)) return Fail(TEXT("跳跃路径被阻挡"));
        Previous = Next;
    }
    Reason = FText::FromString(TEXT("左键确认 · 右键取消"));
    return true;
}

bool UUmbraSkillTargetingComponent::HandleSkillInput(FGameplayTag Tag)
{
    auto* PC = Cast<AUmbraPlayerController>(GetOwner());
    if (!PC || !PC->IsLocalController()) return false;
    auto* ASC = Cast<UUmbraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC->GetPawn()));
    if (!ASC) return false;
    for (const auto& Spec : ASC->GetActivatableAbilities())
    {
        auto* D = Cast<UUmbraSweepDefinition>(Spec.SourceObject.Get());
        if (!D || !Spec.GetDynamicSpecSourceTags().HasTagExact(Tag)) continue;
        if (!D->UsesTargeting()) { CancelTargeting(); return false; }
        if (IsTargeting() && SpecHandle == Spec.Handle) { if (!bPending) CancelTargeting(); return true; }
        if (bPending) return true;
        CancelTargeting();
        if (PC->IsCharacterMenuOpen() || ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking)
            || ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Dead) || ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Stunned)
            || (UmbraSkills::AreCooldownsEnabled() && ASC->HasMatchingGameplayTag(D->CooldownTag))) return true;
        auto* C = Cast<ACharacter>(PC->GetPawn());
        if (!C || !C->GetCharacterMovement()->IsMovingOnGround()) return true;
        PC->StopPointerActionsForDebugUI(); C->GetCharacterMovement()->StopMovementImmediately();
        Definition=D; SpecHandle=Spec.Handle; AimingPawn=C;
        // Controllers are hidden actors; decals owned by them inherit that hidden state.
        IndicatorActor=GetWorld()->SpawnActor<AActor>();
        if (!IndicatorActor) { CancelTargeting(); return true; }
        Prompt=CreateWidget<UUmbraTargetingPrompt>(PC,UUmbraTargetingPrompt::StaticClass());
        if (Prompt) { Prompt->SetVisibility(ESlateVisibility::HitTestInvisible); Prompt->AddToViewport(30); }
        const auto MakeDecal = [this,D](TObjectPtr<UDecalComponent>& Decal, TObjectPtr<UMaterialInstanceDynamic>& Material)
        {
            Decal=NewObject<UDecalComponent>(IndicatorActor); Decal->RegisterComponent();
            Material=UMaterialInstanceDynamic::Create(D->TargetIndicatorMaterial, this);
            Decal->SetDecalMaterial(Material); Decal->SetWorldRotation(FRotator(-90,0,0));
            Decal->SetFadeScreenSize(0.f);
        };
        MakeDecal(RangeDecal,RangeMaterial); MakeDecal(ImpactDecal,ImpactMaterial);
        RangeMaterial->SetVectorParameterValue(TEXT("Tint"), FLinearColor(.15f,.65f,1.f));
        RangeMaterial->SetScalarParameterValue(TEXT("Fill"),0.015f);
        RangeMaterial->SetScalarParameterValue(TEXT("LineWidth"),2.f/D->LeapDistance);
        ImpactMaterial->SetScalarParameterValue(TEXT("LineWidth"),3.f/D->Radius);
        return true;
    }
    if (Tag.ToString().StartsWith(TEXT("Input.Skill."))) CancelTargeting();
    return false;
}

bool UUmbraSkillTargetingComponent::IsPointerOverUI() const
{
    if (!FSlateApplication::IsInitialized()) return false;
    auto& Slate=FSlateApplication::Get();
    const FWidgetPath Path=Slate.LocateWindowUnderMouse(Slate.GetCursorPos(),Slate.GetInteractiveTopLevelWindows());
    for (int32 I=0; I<Path.Widgets.Num(); ++I)
        if (Path.Widgets[I].Widget->GetTypeAsString()==TEXT("SObjectWidget")) return true;
    return false;
}

bool UUmbraSkillTargetingComponent::UpdateTargetLocation(FVector Requested)
{
    auto* PC=Cast<AUmbraPlayerController>(GetOwner());
    if (!Definition || !PC || bPending) return false;
    bValidTarget=EvaluateTarget(Cast<ACharacter>(PC->GetPawn()), Definition, Requested, true, AimLocation, TargetStatus);
    UpdateIndicators(); return bValidTarget;
}

void UUmbraSkillTargetingComponent::UpdateIndicators()
{
    auto* C=Cast<ACharacter>(AimingPawn.Get());
    if (!C || !Definition || !RangeDecal || !ImpactDecal) return;
    FVector Feet=C->GetActorLocation(); Feet.Z-=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    if (Definition->bTargetedDirection)
    {
        RangeDecal->SetVisibility(false);
        const float Extent=Definition->DirectionExtent();
        const FVector Size(40,Extent,Extent);
        if (ImpactDecal->DecalSize!=Size) { ImpactDecal->DecalSize=Size; ImpactDecal->MarkRenderStateDirty(); }
        ImpactDecal->SetWorldLocation(Feet+FVector(0,0,8));
        // Material uses world XY, avoiding decal UV orientation assumptions.
        ImpactMaterial->SetVectorParameterValue(TEXT("ShapeOrigin"),FLinearColor(Feet.X,Feet.Y,Feet.Z));
        const FVector Facing=(AimLocation-Feet).GetSafeNormal2D();
        ImpactMaterial->SetVectorParameterValue(TEXT("ShapeForward"),FLinearColor(Facing.X,Facing.Y,0));
        ImpactMaterial->SetScalarParameterValue(TEXT("Radius"),Definition->Radius);
        ImpactMaterial->SetScalarParameterValue(TEXT("HalfAngle"),FMath::DegreesToRadians(Definition->ArcDegrees*.5f));
        ImpactMaterial->SetScalarParameterValue(TEXT("Length"),Definition->TargetLineLength);
        ImpactMaterial->SetScalarParameterValue(TEXT("HalfWidth"),Definition->TargetLineHalfWidth);
        ImpactMaterial->SetScalarParameterValue(TEXT("Shape"),Definition->Execution==EUmbraSkillExecution::LinearWave ? 1.f : 0.f);
        ImpactMaterial->SetVectorParameterValue(TEXT("Tint"),bValidTarget ? FLinearColor(.1f,1.f,.65f) : FLinearColor(1.f,.12f,.08f));
        return;
    }
    const FVector RangeSize(40,Definition->LeapDistance,Definition->LeapDistance);
    if (RangeDecal->DecalSize!=RangeSize) { RangeDecal->DecalSize=RangeSize; RangeDecal->MarkRenderStateDirty(); }
    RangeDecal->SetWorldLocation(Feet+FVector(0,0,8));
    const FVector ImpactSize(40,Definition->Radius,Definition->Radius);
    if (ImpactDecal->DecalSize!=ImpactSize) { ImpactDecal->DecalSize=ImpactSize; ImpactDecal->MarkRenderStateDirty(); }
    ImpactDecal->SetWorldLocation(AimLocation+FVector(0,0,8));
    ImpactMaterial->SetVectorParameterValue(TEXT("Tint"), bValidTarget ? FLinearColor(.1f,1.f,.65f) : FLinearColor(1.f,.12f,.08f));
    ImpactMaterial->SetScalarParameterValue(TEXT("Invalid"), bValidTarget ? 0.f : 1.f);
}

void UUmbraSkillTargetingComponent::TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Tick,Function);
    if (!Definition) return;
    auto* PC=Cast<AUmbraPlayerController>(GetOwner());
    auto* ASC=PC ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC->GetPawn()) : nullptr;
    if (!PC || !ASC || PC->GetPawn()!=AimingPawn.Get() || PC->IsCharacterMenuOpen()
        || ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Dead) || ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Stunned))
    { CancelTargeting(); return; }
    if (bPending) return;
    if (ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking)) { CancelTargeting(); return; }
    FVector Origin,Direction; FHitResult Hit;
    FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SkillAimCursor),false,PC->GetPawn());
    if (IsPointerOverUI() || !PC->DeprojectMousePositionToWorld(Origin,Direction)
        || !GetWorld()->LineTraceSingleByObjectType(Hit,Origin,Origin+Direction*100000.f,Objects,Query))
    { bValidTarget=false; TargetStatus=FText::FromString(TEXT("请选择地面位置")); UpdateIndicators(); return; }
    UpdateTargetLocation(Hit.ImpactPoint);
}

bool UUmbraSkillTargetingComponent::HandlePointerKey(FKey Key,EInputEvent Event)
{
    if (ConsumedKeys.Contains(Key))
    { if (Event==IE_Released) ConsumedKeys.Remove(Key); return true; }
    if (!IsTargeting() || (Key!=EKeys::LeftMouseButton && Key!=EKeys::RightMouseButton && Key!=EKeys::Escape)) return false;
    if (Event==IE_Pressed)
    {
        ConsumedKeys.Add(Key);
        if (Key==EKeys::LeftMouseButton) { if (!IsPointerOverUI()) ConfirmTargeting(); }
        else if (!bPending) CancelTargeting();
    }
    return true;
}

void UUmbraSkillTargetingComponent::ConfirmTargeting()
{
    if (!Definition || !bValidTarget || bPending) return;
    bPending=true; ServerConfirm(SpecHandle,AimLocation,++Serial);
}

void UUmbraSkillTargetingComponent::ServerConfirm_Implementation(FGameplayAbilitySpecHandle Handle,FVector_NetQuantize Location,uint32 Request)
{
    auto* PC=Cast<AUmbraPlayerController>(GetOwner());
    auto* ASC=PC ? Cast<UUmbraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC->GetPawn())) : nullptr;
    FText Reason=FText::FromString(TEXT("当前无法释放"));
    bool Accepted=false;
    if (ASC) Accepted=ASC->ActivateTargetedLeap(Handle,Location,Reason);
    ClientConfirmResult(Request,Accepted,Reason);
}

void UUmbraSkillTargetingComponent::ClientConfirmResult_Implementation(uint32 Request,bool bAccepted,const FText& Reason)
{
    if (Request!=Serial || !Definition) return;
    bPending=false;
    if (bAccepted) CancelTargeting();
    else { bValidTarget=false; TargetStatus=Reason; UpdateIndicators(); }
}

void UUmbraSkillTargetingComponent::CancelTargeting()
{
    ++Serial; bPending=false; bValidTarget=false; Definition=nullptr; SpecHandle=FGameplayAbilitySpecHandle(); AimingPawn.Reset();
    if (RangeDecal) RangeDecal->DestroyComponent();
    if (ImpactDecal) ImpactDecal->DestroyComponent();
    RangeDecal=nullptr; ImpactDecal=nullptr; RangeMaterial=nullptr; ImpactMaterial=nullptr;
    if (IndicatorActor) IndicatorActor->Destroy();
    IndicatorActor=nullptr;
    if (Prompt) Prompt->RemoveFromParent();
    Prompt=nullptr;
}
void UUmbraSkillTargetingComponent::EndPlay(const EEndPlayReason::Type Reason)
{ CancelTargeting(); Super::EndPlay(Reason); }

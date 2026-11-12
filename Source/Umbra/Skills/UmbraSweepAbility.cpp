#include "Skills/UmbraSweepAbility.h"
#include "Skills/UmbraSweepDefinition.h"
#include "Skills/UmbraSkillTargetingComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Components/CapsuleComponent.h"
#include "Skills/UmbraSkillProjectile.h"
#include "EngineUtils.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimMontage.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Interfaces/UmbraAttackable.h"
#include "TimerManager.h"
#include "UmbraPlayerController.h"
#include "Umbra.h"

UUmbraSwordStormEffect::UUmbraSwordStormEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
}

UUmbraSkillGuardEffect::UUmbraSkillGuardEffect()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FScalableFloat(1.f);
    for (const FGameplayAttribute Attribute : {UUmbraAttributeSet::GetArmorAttribute(),
        UUmbraAttributeSet::GetSlashingResistanceAttribute(), UUmbraAttributeSet::GetBluntResistanceAttribute(),
        UUmbraAttributeSet::GetPiercingResistanceAttribute()})
    {
        auto& Modifier = Modifiers.AddDefaulted_GetRef();
        Modifier.Attribute = Attribute;
        Modifier.ModifierOp = EGameplayModOp::Additive;
        FSetByCallerFloat Value;
        Value.DataName = Attribute == UUmbraAttributeSet::GetArmorAttribute() ? FName(TEXT("GuardArmor")) : FName(TEXT("GuardResistance"));
        Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Value);
    }
}

UUmbraSkillFocusEffect::UUmbraSkillFocusEffect()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FScalableFloat(6.f);
    auto& Modifier = Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = UUmbraAttributeSet::GetCriticalChanceAttribute();
    Modifier.ModifierOp = EGameplayModOp::Additive;
    FSetByCallerFloat Value;
    Value.DataName = TEXT("FocusCriticalChance");
    Modifier.ModifierMagnitude = Value;
}

UUmbraSkillHealEffect::UUmbraSkillHealEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    auto& Modifier = Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = UUmbraAttributeSet::GetHealthAttribute();
    Modifier.ModifierOp = EGameplayModOp::Additive;
    FSetByCallerFloat Value;
    Value.DataName = TEXT("SkillHeal");
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Value);
}

UUmbraSkillCostEffect::UUmbraSkillCostEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = UUmbraAttributeSet::GetResourceAttribute();
    Modifier.ModifierOp = EGameplayModOp::Additive;
    FSetByCallerFloat Cost;
    Cost.DataName = TEXT("SkillCost");
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Cost);
}

UUmbraSkillCooldownEffect::UUmbraSkillCooldownEffect()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FScalableFloat(1.f);
}

UUmbraSweepAbility::UUmbraSweepAbility()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
    bReplicateInputDirectly = true;
    FGameplayTagContainer Tags;
    Tags.AddTag(UmbraGameplayTags::Ability_Skill_Sweep);
    SetAssetTags(Tags);
    ActivationOwnedTags.AddTag(UmbraGameplayTags::State_Attacking);
    ActivationBlockedTags.AddTag(UmbraGameplayTags::State_Attacking);
    ActivationBlockedTags.AddTag(UmbraGameplayTags::State_Dead);
    ActivationBlockedTags.AddTag(UmbraGameplayTags::State_Stunned);
}

const UUmbraSweepDefinition* UUmbraSweepAbility::FindDefinition(FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* Info) const
{
    const auto* Spec = Info && Info->AbilitySystemComponent.IsValid() ? Info->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
    return Spec ? Cast<UUmbraSweepDefinition>(Spec->SourceObject.Get()) : nullptr;
}

bool UUmbraSweepAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* Tags) const
{
    const auto* Definition = FindDefinition(Handle, Info);
    const auto* Character = Info ? Cast<AUmbraPlayerCharacter>(Info->AvatarActor.Get()) : nullptr;
    FString Reason;
    return Definition && Definition->Validate(Reason) && Character && !Character->IsDead()
        && Character->GetMesh()->GetAnimInstance()
        && (Definition->Execution != EUmbraSkillExecution::LeapSlam || Character->GetCharacterMovement()->IsMovingOnGround())
        && Super::CanActivateAbility(Handle, Info, SourceTags, TargetTags, Tags);
}

bool UUmbraSweepAbility::CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info, FGameplayTagContainer* Tags) const
{
    const auto* Definition = FindDefinition(Handle, Info);
    return Definition && Info->AbilitySystemComponent->GetNumericAttribute(UUmbraAttributeSet::GetResourceAttribute()) >= Definition->ResourceCost;
}

bool UUmbraSweepAbility::CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info, FGameplayTagContainer* Tags) const
{
    const auto* Definition = FindDefinition(Handle, Info);
    return Definition && (!UmbraSkills::AreCooldownsEnabled() || !Info->AbilitySystemComponent->HasMatchingGameplayTag(Definition->CooldownTag));
}

void UUmbraSweepAbility::ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info, FGameplayAbilityActivationInfo ActivationInfo) const
{
    if (!Info->IsNetAuthority()) return;
    const auto* Definition = FindDefinition(Handle, Info);
    auto* ASC = Info->AbilitySystemComponent.Get();
    auto Spec = ASC->MakeOutgoingSpec(UUmbraSkillCostEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("SkillCost")), -Definition->ResourceCost);
    ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UUmbraSweepAbility::ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info, FGameplayAbilityActivationInfo ActivationInfo) const
{
    if (!Info->IsNetAuthority() || !UmbraSkills::AreCooldownsEnabled()) return;
    const auto* Definition = FindDefinition(Handle, Info);
    auto* ASC = Info->AbilitySystemComponent.Get();
    auto Spec = ASC->MakeOutgoingSpec(UUmbraSkillCooldownEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
    Spec.Data->SetDuration(Definition->CooldownSeconds, true);
    Spec.Data->DynamicGrantedTags.AddTag(Definition->CooldownTag);
    ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UUmbraSweepAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* Event)
{
    Super::ActivateAbility(Handle, Info, ActivationInfo, Event);
    bEnding = bStruck = bExecuteMovement = false;
    bChannelReleased = false;
    bLanded = false;
    PulsesRemaining = 0;
    ActivationStartTime = GetWorld()->GetTimeSeconds();
    ActiveDefinition = FindDefinition(Handle, Info);
    auto* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
    if (ActiveDefinition && ActiveDefinition->UsesTargeting())
    {
        auto* ASC=Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
        if (Info->IsNetAuthority() && (!ASC || !ASC->ConsumeTargetedLeap(Handle,ConfirmedGround))) { Interrupt(); return; }
        if (Info->IsNetAuthority() && Character)
            Character->SetActorRotation((ConfirmedGround-Character->GetActorLocation()).GetSafeNormal2D().Rotation());
    }
    if (!ActiveDefinition || !Character || !CommitAbility(Handle, Info, ActivationInfo)) { Interrupt(); return; }
    Origin = Character->GetActorLocation();
    Forward = Character->GetActorForwardVector().GetSafeNormal2D();
    Character->StopPrimaryAttackContinuation();
    if (ActiveDefinition->Execution == EUmbraSkillExecution::SwordStorm)
    {
        if (Info->IsNetAuthority())
        {
            auto* ASC = GetAbilitySystemComponentFromActorInfo();
            auto Storm = ASC->MakeOutgoingSpec(UUmbraSwordStormEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
            Storm.Data->DynamicGrantedTags.AddTag(UmbraGameplayTags::State_SwordStorm);
            StormHandle = ASC->ApplyGameplayEffectSpecToSelf(*Storm.Data.Get());
        }
    }
    else
    {
        if (auto* Controller = Cast<AUmbraPlayerController>(Character->GetController())) Controller->CancelAutoMove();
        Character->GetCharacterMovement()->StopMovementImmediately();
    }
    // Reuse the attack-state movement queue, but not the basic attack's instance or strike counters.
    auto* Movement = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, UmbraGameplayTags::Event_Attack_MovementCommand, nullptr, false, true);
    Movement->EventReceived.AddDynamic(this, &ThisClass::MovementRequested);
    Movement->ReadyForActivation();
    for (const FGameplayTag Tag : {FGameplayTag(UmbraGameplayTags::State_Dead), FGameplayTag(UmbraGameplayTags::State_Stunned)})
    {
        auto* Task = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, Tag, nullptr, true);
        Task->Added.AddDynamic(this, &ThisClass::Interrupt);
        Task->ReadyForActivation();
        if (!IsActive()) return;
    }
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None,
        ActiveDefinition->Montage, ActiveDefinition->GetInitialPlayRate());
    MontageTask->OnCompleted.AddDynamic(this, &ThisClass::MontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::Interrupt);
    MontageTask->OnCancelled.AddDynamic(this, &ThisClass::Interrupt);
    MontageTask->ReadyForActivation();
    if (!IsActive()) return;
    if (ActiveDefinition->WindupDuration > 0.f)
        GetWorld()->GetTimerManager().SetTimer(WindupTimer, this, &ThisClass::FinishWindup, ActiveDefinition->WindupDuration, false);
    if (Info->IsNetAuthority())
    {
        if (auto* ASC = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
            ASC->MulticastSkillActiveEffect(ActiveDefinition, true);
        if (ActiveDefinition->Execution == EUmbraSkillExecution::Guard) BeginGuard();
        else GetWorld()->GetTimerManager().SetTimer(StrikeTimer, this, &ThisClass::Strike,
            ActiveDefinition->GetReleaseDelay(), false);
    }
    UE_LOG(LogUmbra, Log, TEXT("[Skill] Start id=%s authority=%d cost=%.1f cooldown=%.2f"),
        *ActiveDefinition->SkillId.ToString(), Info->IsNetAuthority(), ActiveDefinition->ResourceCost, ActiveDefinition->CooldownSeconds);
}

void UUmbraSweepAbility::FinishWindup()
{
    if (!IsActive() || bEnding || !ActiveDefinition) return;
    if (auto* ASC = GetAbilitySystemComponentFromActorInfo())
        if (ASC->GetCurrentMontage() == ActiveDefinition->Montage)
            ASC->CurrentMontageSetPlayRate(ActiveDefinition->PlayRate);
}

void UUmbraSweepAbility::Strike()
{
    if (!IsActive() || bEnding || bStruck || !ActiveDefinition) return;
    bStruck = true;
    auto* ASC = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!ASC || !CurrentActorInfo->IsNetAuthority()) return;
    if (ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Dead) || ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Stunned)) { Interrupt(); return; }
    if (ActiveDefinition->bTargetedDirection)
    {
        // Freeze the confirmed origin/direction; forced displacement cancels this cast.
        if (FVector::DistSquared2D(Origin,GetAvatarActorFromActorInfo()->GetActorLocation()) > 25.f) { Interrupt(); return; }
        ASC->MulticastSkillPresentation(ActiveDefinition,Origin,Forward.Rotation());
        return;
    }
    if (ActiveDefinition->Execution == EUmbraSkillExecution::BattleFocus)
    {
        // Once resolved, this buff outlives the cast; cancelling recovery must not undo it.
        ASC->RemoveActiveGameplayEffectBySourceEffect(UUmbraSkillFocusEffect::StaticClass(), ASC);
        auto Focus = ASC->MakeOutgoingSpec(UUmbraSkillFocusEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
        Focus.Data->SetDuration(ActiveDefinition->FocusDuration, true);
        Focus.Data->SetSetByCallerMagnitude(FName(TEXT("FocusCriticalChance")), ActiveDefinition->FocusCriticalChance);
        ASC->ApplyGameplayEffectSpecToSelf(*Focus.Data.Get());
        ASC->MulticastSkillPresentation(ActiveDefinition, GetAvatarActorFromActorInfo()->GetActorLocation(), Forward.Rotation());
        return;
    }
    if (ActiveDefinition->Execution == EUmbraSkillExecution::Projectile)
    {
        ASC->MulticastSkillActiveEffect(ActiveDefinition, false);
        LaunchProjectile();
        return;
    }
    if (ActiveDefinition->Execution == EUmbraSkillExecution::LeapSlam)
    {
        auto* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
        if (!Character || !Character->GetCharacterMovement()->IsMovingOnGround()) { Interrupt(); return; }
        if (ActiveDefinition->bTargetedLeap)
        {
            FVector Ground; FText Reason;
            if (!UUmbraSkillTargetingComponent::EvaluateTarget(Character,ActiveDefinition,ConfirmedGround,false,Ground,Reason)) { Interrupt(); return; }
            const FVector End=Ground+FVector(0,0,Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2.5f);
            Character->LandedDelegate.AddDynamic(this,&ThisClass::TargetLeapLanded);
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            TargetLeapTask=UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(this,TEXT("TargetLeap"),End,
                ActiveDefinition->LeapDuration,false,MOVE_Flying,true,ActiveDefinition->TargetLeapCurve,
                ERootMotionFinishVelocityMode::SetVelocity,FVector::ZeroVector,0.f);
            TargetLeapTask->OnTimedOutAndDestinationReached.AddDynamic(this,&ThisClass::TargetLeapArrived);
            TargetLeapTask->OnTimedOut.AddDynamic(this,&ThisClass::Interrupt);
            TargetLeapTask->ReadyForActivation();
            GetWorld()->GetTimerManager().SetTimer(FinishTimer,this,&ThisClass::Interrupt,ActiveDefinition->LeapDuration+2.f,false);
            return;
        }
        // CharacterMovement performs capsule sweeps; no teleport and no impact at an assumed endpoint.
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        LeapTask = UAbilityTask_ApplyRootMotionJumpForce::ApplyRootMotionJumpForce(this, TEXT("SkillLeap"),
            Forward.Rotation(), ActiveDefinition->LeapDistance, ActiveDefinition->LeapHeight,
            ActiveDefinition->LeapDuration, .1f, true, ERootMotionFinishVelocityMode::SetVelocity,
            FVector::ZeroVector, 0.f, nullptr, nullptr);
        LeapTask->OnLanded.AddDynamic(this, &ThisClass::Landed);
        LeapTask->ReadyForActivation();
        // Falling off a ledge or a blocked trajectory must never leave the attack state locked.
        GetWorld()->GetTimerManager().SetTimer(FinishTimer, this, &ThisClass::Interrupt,
            ActiveDefinition->LeapDuration + 2.f, false);
        return;
    }
    if (ActiveDefinition->Execution == EUmbraSkillExecution::StoneRecovery)
    {
        auto Heal = ASC->MakeOutgoingSpec(UUmbraSkillHealEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
        Heal.Data->SetSetByCallerMagnitude(FName(TEXT("SkillHeal")),
            ASC->GetNumericAttribute(UUmbraAttributeSet::GetMaxHealthAttribute()) * ActiveDefinition->HealMaxHealthFraction);
        ASC->ApplyGameplayEffectSpecToSelf(*Heal.Data.Get());
    }
    PulsesRemaining = ActiveDefinition->Execution == EUmbraSkillExecution::SwordStorm ? ActiveDefinition->PulseCount : 1;
    if (ActiveDefinition->Execution == EUmbraSkillExecution::SwordStorm && ActiveDefinition->bChannelWhileHeld)
        GetWorld()->GetTimerManager().SetTimer(StrikeTimer, this, &ThisClass::DamagePulse, ActiveDefinition->GetStormPulseInterval(), true);
    DamagePulse();
}

void UUmbraSweepAbility::BeginGuard()
{
    auto* ASC = GetAbilitySystemComponentFromActorInfo();
    auto Spec = ASC->MakeOutgoingSpec(UUmbraSkillGuardEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
    Spec.Data->SetDuration(ActiveDefinition->GuardDuration, true);
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("GuardArmor")), ActiveDefinition->GuardArmor);
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("GuardResistance")), ActiveDefinition->GuardPhysicalResistance);
    GuardHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    GetWorld()->GetTimerManager().SetTimer(FinishTimer, this, &ThisClass::Complete, ActiveDefinition->GuardDuration, false);
}

void UUmbraSweepAbility::Landed()
{
    if (!IsActive() || bEnding || bLanded || !ActiveDefinition) return;
    bLanded = true;
    PulsesRemaining = 1;
    DamagePulse();
    const float Recovery = FMath::Max(.3f, ActiveDefinition->Montage->GetPlayLength() / ActiveDefinition->PlayRate
        - float(GetWorld()->GetTimeSeconds() - ActivationStartTime));
    GetWorld()->GetTimerManager().SetTimer(FinishTimer, this, &ThisClass::Complete, Recovery, false);
}

void UUmbraSweepAbility::TargetLeapArrived()
{
    if (auto* C=Cast<ACharacter>(GetAvatarActorFromActorInfo())) C->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}
void UUmbraSweepAbility::TargetLeapLanded(const FHitResult& Hit)
{
    if (!IsActive() || bEnding || !ActiveDefinition) return;
    // Do not award damage at a stale selection after dynamic obstruction or forced displacement.
    const FVector Actual=GetAvatarActorFromActorInfo()->GetActorLocation();
    if (FVector::DistSquared2D(Actual,ConfirmedGround)>FMath::Square(5.f)) { Interrupt(); return; }
    Landed();
}

void UUmbraSweepAbility::MontageCompleted()
{
    if (!ActiveDefinition) return;
    if (ActiveDefinition->bTargetedDirection || ActiveDefinition->Execution == EUmbraSkillExecution::Sector || ActiveDefinition->Execution == EUmbraSkillExecution::StoneRecovery
        || ActiveDefinition->Execution == EUmbraSkillExecution::Projectile || ActiveDefinition->Execution == EUmbraSkillExecution::BattleFocus)
        Complete();
    // Storm, guard and leap have gameplay lifetimes beyond the summoning/stance montage.
}

void UUmbraSweepAbility::DamagePulse()
{
    if (!IsActive() || bEnding || !ActiveDefinition || !CurrentActorInfo->IsNetAuthority()) return;
    const bool bChannel = ActiveDefinition->Execution == EUmbraSkillExecution::SwordStorm && ActiveDefinition->bChannelWhileHeld;
    if (!bChannel && PulsesRemaining <= 0) return;
    auto* ASC = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!ASC || ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Dead) || ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Stunned)) { Interrupt(); return; }
    if (PulsesRemaining > 0) --PulsesRemaining;
    Origin = GetAvatarActorFromActorInfo()->GetActorLocation();
    const auto& Definition = *ActiveDefinition;
    ASC->MulticastSkillPresentation(ActiveDefinition, Origin, Forward.Rotation());
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(UmbraSweep), false, GetAvatarActorFromActorInfo());
    GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(FMath::Sqrt(FMath::Square(Definition.Radius) + FMath::Square(Definition.HeightTolerance))), Query);
    TSet<AActor*> Visited;
    int32 Submitted = 0;
    for (const auto& Overlap : Overlaps)
    {
        AActor* Target = Overlap.GetActor();
        if (!IsValid(Target) || Target == GetAvatarActorFromActorInfo() || Visited.Contains(Target)) continue;
        Visited.Add(Target);
        if (!Target->Implements<UUmbraAttackable>() || !IUmbraAttackable::Execute_CanBeAttacked(Target)
            || !Definition.ContainsTarget(Origin, Forward, Target->GetActorLocation())) continue;
        FHitResult Blocker;
        if (GetWorld()->LineTraceSingleByChannel(Blocker, Origin + FVector(0,0,50),
            Target->GetActorLocation() + FVector(0,0,50), ECC_Visibility, Query) && Blocker.GetActor() != Target) continue;
        FUmbraDamageRequest Request(EUmbraDamageSource::Skill);
        Request.Config = Definition.Damage;
        Request.EffectClass = Definition.DamageEffect;
        Request.Level = GetAbilityLevel();
        auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
        const float HealthBefore = TargetASC ? TargetASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()) : 0.f;
        const FVector HitLocation = Target->GetActorLocation();
        if (UmbraDamage::Apply(ASC, TargetASC, Request))
        {
            ++Submitted;
            if (Definition.Execution == EUmbraSkillExecution::SwordStorm && IsValid(TargetASC)
                && TargetASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()) < HealthBefore)
                ASC->MulticastStormHit(ActiveDefinition, HitLocation, (HitLocation-Origin).Rotation());
            FGameplayEventData Hit;
            Hit.EventTag = UmbraGameplayTags::Event_Combat_HitReceived;
            Hit.Instigator = GetAvatarActorFromActorInfo();
            Hit.Target = Target;
            UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, Hit.EventTag, Hit);
        }
    }
    UE_LOG(LogUmbra, Log, TEXT("[Skill] Strike id=%s targets=%d radius=%.1f arc=%.1f"),
        *Definition.SkillId.ToString(), Submitted, Definition.Radius, Definition.ArcDegrees);
    if (Definition.Execution == EUmbraSkillExecution::SwordStorm)
    {
        if (bChannel) return; // Repeating timer preserves cadence instead of accumulating one frame per pulse.
        if (PulsesRemaining > 0) GetWorld()->GetTimerManager().SetTimer(StrikeTimer, this, &ThisClass::DamagePulse, Definition.GetStormPulseInterval(), false);
        else
        {
            const float Recovery = FMath::Max(.2f, Definition.Montage->GetPlayLength() / Definition.PlayRate
                - float(GetWorld()->GetTimeSeconds() - ActivationStartTime));
            GetWorld()->GetTimerManager().SetTimer(FinishTimer, this, &ThisClass::Complete, Recovery, false);
        }
    }
}

void UUmbraSweepAbility::MovementRequested(FGameplayEventData Event)
{
    if (ActiveDefinition && ActiveDefinition->Execution == EUmbraSkillExecution::SwordStorm) return;
    bExecuteMovement = true;
    Interrupt(); // Before the strike: no hit. After the strike: only recovery is cancelled. No refunds.
}

void UUmbraSweepAbility::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    if (Info && Info->IsNetAuthority() && IsActive() && ActiveDefinition
        && ActiveDefinition->Execution == EUmbraSkillExecution::SwordStorm && ActiveDefinition->bChannelWhileHeld)
    {
        bChannelReleased = true;
        Complete(); // Stop damage immediately; montage task blends out without cancelling movement.
    }
}

void UUmbraSweepAbility::LaunchProjectile()
{
    auto* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
    auto* ASC = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!Character || !ASC || !CurrentActorInfo->IsNetAuthority()) return;
    const FVector Center = Character->GetActorLocation();
    FVector Muzzle = Center + Forward * 55.f + FVector(0, 0, 15.f);
    if (!ActiveDefinition->CastSocket.IsNone() && Character->GetMesh()->DoesSocketExist(ActiveDefinition->CastSocket))
        Muzzle = Character->GetMesh()->GetSocketLocation(ActiveDefinition->CastSocket) + Forward * 20.f;
    AActor* Target = nullptr;
    float BestDistance = FMath::Square(ActiveDefinition->ProjectileMaxSpeed * ActiveDefinition->ProjectileLifetime);
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Candidate = *It;
        if (ActiveDefinition->ProjectileTurnRate <= 0.f || Candidate == Character || !Candidate->Implements<UUmbraAttackable>() || !IUmbraAttackable::Execute_CanBeAttacked(Candidate)) continue;
        const FVector Delta = Candidate->GetActorLocation() - Muzzle;
        if (Delta.SizeSquared() >= BestDistance || FVector::DotProduct(Forward, Delta.GetSafeNormal()) < .978f) continue;
        FHitResult Obstruction;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(PebbleTarget), false, Character);
        if (GetWorld()->LineTraceSingleByChannel(Obstruction, Muzzle, Candidate->GetActorLocation(), ECC_Visibility, Query)
            && Obstruction.GetActor() != Candidate) continue;
        Target = Candidate;
        BestDistance = Delta.SizeSquared();
    }
    const FVector Direction = Target ? (Target->GetActorLocation() - Muzzle).GetSafeNormal() : Forward;
    const FTransform Transform(Direction.Rotation(), Muzzle);
    auto* Projectile = GetWorld()->SpawnActorDeferred<AUmbraSkillProjectile>(AUmbraSkillProjectile::StaticClass(), Transform,
        Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Projectile) return;
    Projectile->Initialize(ActiveDefinition, Direction, Target, GetAbilityLevel());
    Projectile->FinishSpawning(Transform);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(PebbleMuzzle), false, Character);
    Query.AddIgnoredActor(Projectile);
    FHitResult Blocker;
    const FCollisionShape Shape = ActiveDefinition->ProjectileHalfWidth > 0.f
        ? FCollisionShape::MakeBox(FVector(ActiveDefinition->ProjectileRadius, ActiveDefinition->ProjectileHalfWidth, ActiveDefinition->ProjectileRadius))
        : FCollisionShape::MakeSphere(ActiveDefinition->ProjectileRadius);
    if (GetWorld()->SweepSingleByChannel(Blocker, Center, Muzzle, Direction.Rotation().Quaternion(), ECC_Visibility,
        Shape, Query)) Projectile->Impact(Blocker);
    ASC->MulticastSkillPresentation(ActiveDefinition, Muzzle, Direction.Rotation());
    UE_LOG(LogUmbra, Log, TEXT("[Pebble] Launched id=%s target=%s"), *ActiveDefinition->SkillId.ToString(), *GetNameSafe(Target));
}

void UUmbraSweepAbility::Complete()
{
    if (IsActive() && !bEnding) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UUmbraSweepAbility::Interrupt()
{
    if (IsActive() && !bEnding) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UUmbraSweepAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (bEnding) return;
    bEnding = true;
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(StrikeTimer);
        GetWorld()->GetTimerManager().ClearTimer(WindupTimer);
        GetWorld()->GetTimerManager().ClearTimer(FinishTimer);
    }
    if (Info && Info->IsNetAuthority())
    {
        if (auto* ASC = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
        {
            if (GuardHandle.IsValid()) ASC->RemoveActiveGameplayEffect(GuardHandle);
            if (StormHandle.IsValid()) ASC->RemoveActiveGameplayEffect(StormHandle);
            if (ActiveDefinition) ASC->MulticastSkillActiveEffect(ActiveDefinition, false, bWasCancelled || bChannelReleased);
        }
    }
    GuardHandle.Invalidate();
    StormHandle.Invalidate();
    auto* Character = Info ? Cast<AUmbraPlayerCharacter>(Info->AvatarActor.Get()) : nullptr;
    if (Character && ActiveDefinition && ActiveDefinition->bTargetedLeap)
    {
        Character->LandedDelegate.RemoveDynamic(this,&ThisClass::TargetLeapLanded);
        if (Character->GetCharacterMovement()->MovementMode==MOVE_Flying)
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    }
    auto* Controller = Character ? Cast<AUmbraPlayerController>(Character->GetController()) : nullptr;
    Super::EndAbility(Handle, Info, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    MontageTask = nullptr;
    LeapTask = nullptr;
    ActiveDefinition = nullptr;
    if (Controller) Controller->NotifyPrimaryAttackAbilityEnded(bWasCancelled, bExecuteMovement || !bWasCancelled);
}

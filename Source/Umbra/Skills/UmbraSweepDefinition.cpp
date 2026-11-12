#include "Skills/UmbraSweepDefinition.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarIgnoreSkillCooldowns(TEXT("umbra.Skills.IgnoreCooldowns"), 0,
    TEXT("Prototype: 1 bypasses skill cooldown checks/application/UI; costs and action locks still apply."));

bool UmbraSkills::AreCooldownsEnabled()
{
    return CVarIgnoreSkillCooldowns.GetValueOnGameThread() == 0;
}

bool UUmbraSweepDefinition::Validate(FString& Reason) const
{
    if (!FMath::IsFinite(WindupEndTime) || !FMath::IsFinite(WindupDuration) || WindupEndTime < 0.f
        || WindupDuration < 0.f || WindupDuration > 3.f
        || ((WindupEndTime > 0.f || WindupDuration > 0.f) && (Execution != EUmbraSkillExecution::Projectile
            || WindupEndTime <= 0.f || WindupDuration < .05f || WindupEndTime >= StrikeTime
            || !Montage || StrikeTime >= Montage->GetPlayLength())))
    { Reason = TEXT("Projectile windup requires a positive duration and 0 < WindupEndTime < StrikeTime < montage length."); return false; }
    if (bTargetedDirection && (bTargetedLeap || !TargetIndicatorMaterial || ResourceCost != 0.f
        || (Execution != EUmbraSkillExecution::Sector && Execution != EUmbraSkillExecution::LinearWave)
        || !FMath::IsFinite(TargetLineLength) || TargetLineLength <= 0.f || TargetLineLength > 2000.f
        || !FMath::IsFinite(TargetLineHalfWidth) || TargetLineHalfWidth <= 0.f || TargetLineHalfWidth > 500.f
        || !FMath::IsFinite(TargetWaveSpeed) || TargetWaveSpeed < 100.f || TargetWaveSpeed > 5000.f))
    { Reason = TEXT("Directional prototype requires a valid shape/material and zero cost."); return false; }
    if (bTargetedLeap && (Execution != EUmbraSkillExecution::LeapSlam || !TargetLeapCurve || !TargetIndicatorMaterial
        || ResourceCost != 0.f || !FMath::IsFinite(TargetMaxHeightDifference) || TargetMaxHeightDifference < 0.f || TargetMaxHeightDifference > 500.f))
    { Reason = TEXT("Targeted leap requires curve/material, bounded height and zero prototype cost."); return false; }
    if (!FMath::IsFinite(SkillSoundVolume) || SkillSoundVolume < 0.f || SkillSoundVolume > 2.f
        || !FMath::IsFinite(SkillVoiceVolume) || SkillVoiceVolume < 0.f || SkillVoiceVolume > 2.f
        || (Execution == EUmbraSkillExecution::SwordStorm && ResolveVoice))
    { Reason = TEXT("Invalid audio gain or repeated voice on storm pulses; use CastVoice for storm."); return false; }
    if ((ActiveEffect && ActiveNiagara) || (StrikeEffect && StrikeNiagara)
        || (ProjectileEffect && ProjectileNiagara) || (ImpactEffect && ImpactNiagara))
    { Reason = TEXT("Choose either Cascade or Niagara for each presentation phase."); return false; }
    if (SkillId.IsNone() || DisplayName.IsEmpty() || !CooldownTag.IsValid()
        || !CooldownTag.ToString().StartsWith(TEXT("Cooldown.Skill.")) || !Montage)
    { Reason = TEXT("Identity, skill cooldown tag and montage are required."); return false; }
    if (!FMath::IsFinite(PlayRate) || PlayRate < 0.1f || PlayRate > 3.f
        || !FMath::IsFinite(StrikeTime) || StrikeTime <= 0.f || StrikeTime >= Montage->GetPlayLength()
        || !FMath::IsFinite(Radius) || Radius <= 0.f || Radius > 2000.f
        || !FMath::IsFinite(ArcDegrees) || ArcDegrees <= 0.f || ArcDegrees > 360.f
        || !FMath::IsFinite(HeightTolerance) || HeightTolerance <= 0.f
        || !FMath::IsFinite(ResourceCost) || ResourceCost < 0.f
        || !FMath::IsFinite(CooldownSeconds) || CooldownSeconds <= 0.f
        || EffectOffset.ContainsNaN() || EffectRotation.ContainsNaN() || EffectScale.ContainsNaN()
        || EffectScale.GetMin() <= 0.f)
    { Reason = TEXT("Invalid timing, sector, cost, cooldown or presentation transform."); return false; }
    if (Execution != EUmbraSkillExecution::Guard && Execution != EUmbraSkillExecution::BattleFocus
        && Damage.Typed.Model == EUmbraDamageModel::Legacy && !DamageEffect)
    { Reason = TEXT("Legacy damage requires an execution effect."); return false; }
    if (ActiveEffectOffset.ContainsNaN() || ActiveEffectScale.ContainsNaN() || ActiveEffectScale.GetMin() <= 0.f
        || (Execution == EUmbraSkillExecution::SwordStorm && (PulseCount < 1 || PulseCount > 20
            || !FMath::IsFinite(PulseInterval) || PulseInterval < .1f || PulseInterval > 5.f))
        || (Execution == EUmbraSkillExecution::Guard && (!FMath::IsFinite(GuardDuration) || GuardDuration <= 0.f || GuardDuration > 30.f
            || !FMath::IsFinite(GuardArmor) || GuardArmor < 0.f || !FMath::IsFinite(GuardPhysicalResistance) || GuardPhysicalResistance < 0.f))
        || (Execution == EUmbraSkillExecution::LeapSlam && (!FMath::IsFinite(LeapDistance) || LeapDistance <= 0.f || LeapDistance > 1500.f
            || !FMath::IsFinite(LeapHeight) || LeapHeight <= 0.f || LeapHeight > 500.f
            || !FMath::IsFinite(LeapDuration) || LeapDuration < .2f || LeapDuration > 3.f))
        || (Execution == EUmbraSkillExecution::StoneRecovery && (!FMath::IsFinite(HealMaxHealthFraction)
            || HealMaxHealthFraction <= 0.f || HealMaxHealthFraction > 1.f)))
    { Reason = TEXT("Invalid execution-specific parameters."); return false; }
    if (Execution == EUmbraSkillExecution::BattleFocus && (!FMath::IsFinite(FocusDuration) || FocusDuration <= 0.f || FocusDuration > 30.f
        || !FMath::IsFinite(FocusCriticalChance) || FocusCriticalChance <= 0.f || FocusCriticalChance > 1.f))
    { Reason = TEXT("Invalid focus duration or critical chance."); return false; }
    Reason.Reset();
    if (Execution == EUmbraSkillExecution::Projectile &&
        (!FMath::IsFinite(ProjectileInitialSpeed) || ProjectileInitialSpeed <= 0.f || ProjectileInitialSpeed > 10000.f
        || !FMath::IsFinite(ProjectileMaxSpeed) || ProjectileMaxSpeed < ProjectileInitialSpeed || ProjectileMaxSpeed > 10000.f
        || !FMath::IsFinite(ProjectileAcceleration) || ProjectileAcceleration < 0.f || ProjectileAcceleration > 20000.f
        || !FMath::IsFinite(ProjectileAccelerationDelay) || ProjectileAccelerationDelay < 0.f
        || !FMath::IsFinite(ProjectileLifetime) || ProjectileLifetime <= 0.f || ProjectileLifetime > 10.f
        || !FMath::IsFinite(ProjectileRadius) || ProjectileRadius <= 0.f || ProjectileRadius > 100.f
        || !FMath::IsFinite(ProjectileTurnRate) || ProjectileTurnRate < 0.f || ProjectileTurnRate > 360.f
        || !FMath::IsFinite(ProjectileHalfWidth) || ProjectileHalfWidth < 0.f || ProjectileHalfWidth > 300.f
        || ProjectileMeshScale.ContainsNaN() || ProjectileMeshScale.GetMin() <= 0.f
        || (!ProjectileEffect && !ProjectileNiagara && !ProjectileMesh) || (!ImpactEffect && !ImpactNiagara)))
    { Reason = TEXT("Invalid projectile movement or missing flight/impact presentation."); return false; }
    return true;
}

float UUmbraSweepDefinition::GetInitialPlayRate() const
{
    return WindupEndTime > 0.f && WindupDuration > 0.f ? WindupEndTime / WindupDuration : PlayRate;
}

float UUmbraSweepDefinition::GetReleaseDelay() const
{
    return WindupEndTime > 0.f && WindupDuration > 0.f
        ? WindupDuration + (StrikeTime - WindupEndTime) / PlayRate : StrikeTime / PlayRate;
}

bool UUmbraSweepDefinition::ContainsTarget(const FVector& Origin, const FVector& Forward, const FVector& Target) const
{
    const FVector Delta = Target - Origin;
    if (bTargetedDirection && Execution == EUmbraSkillExecution::LinearWave)
    {
        const FVector Direction = Forward.GetSafeNormal2D();
        const float Along = FVector::DotProduct(Delta, Direction);
        const float Across = FVector::DotProduct(Delta, FVector(-Direction.Y, Direction.X, 0));
        return !Delta.ContainsNaN() && FMath::Abs(Delta.Z) <= HeightTolerance
            && Along >= 0.f && Along <= TargetLineLength && FMath::Abs(Across) <= TargetLineHalfWidth;
    }
    if (Delta.ContainsNaN() || FMath::Abs(Delta.Z) > HeightTolerance || Delta.SizeSquared2D() > FMath::Square(Radius)) return false;
    return Delta.IsNearlyZero() || Delta.SizeSquared2D() < KINDA_SMALL_NUMBER
        || FVector::DotProduct(Forward.GetSafeNormal2D(), Delta.GetSafeNormal2D())
            >= FMath::Cos(FMath::DegreesToRadians(ArcDegrees * 0.5f)) - KINDA_SMALL_NUMBER;
}

float UUmbraSweepDefinition::GetStormPulseInterval() const
{
    if (bSyncStormToAnimation && Montage && !Montage->SlotAnimTracks.IsEmpty()
        && !Montage->SlotAnimTracks[0].AnimTrack.AnimSegments.IsEmpty())
    {
        const auto& Segment = Montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0];
        const float Cycle = (Segment.AnimEndTime - Segment.AnimStartTime)
            / FMath::Max(.01f, FMath::Abs(Segment.AnimPlayRate) * PlayRate);
        return FMath::Max(.1f, Cycle / FMath::Clamp(StormPulsesPerCycle, 1, 8));
    }
    return PulseInterval;
}

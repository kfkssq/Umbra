#include "UI/UmbraDamageNumber.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "UmbraPlayerController.h"

namespace
{
	template <typename EntryType, typename ValueGetter>
	void SortAndDeduplicateByValue(TArray<EntryType>& Entries, ValueGetter GetValue)
	{
		Entries.StableSort([&GetValue](const EntryType& A, const EntryType& B)
		{
			return GetValue(A) < GetValue(B);
		});
		for (int32 Index = Entries.Num() - 1; Index > 0; --Index)
		{
			if (FMath::IsNearlyEqual(GetValue(Entries[Index]), GetValue(Entries[Index - 1])))
			{
				// Stable sort plus removing the earlier entry makes the last configured duplicate win.
				Entries.RemoveAt(Index - 1);
			}
		}
	}
}

UUmbraDamageNumber::UUmbraDamageNumber(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	const auto AddFontTier = [this](double LowerBound, float MinSize, float MaxSize)
	{
		FUmbraDamageFontTier& Tier = FontSizeTiers.AddDefaulted_GetRef();
		Tier.DamageLowerBound = LowerBound;
		Tier.MinFontSize = MinSize;
		Tier.MaxFontSize = MaxSize;
	};
	AddFontTier(0.0, 16.f, 18.f);
	AddFontTier(1000.0, 18.f, 20.f);
	AddFontTier(1000000.0, 20.f, 22.f);
	AddFontTier(1000000000.0, 22.f, 24.f);

	const auto AddUnit = [this](double Threshold, const TCHAR* Suffix)
	{
		FUmbraDamageAbbreviationUnit& Unit = AbbreviationUnits.AddDefaulted_GetRef();
		Unit.ValueThreshold = Threshold;
		Unit.Suffix = Suffix;
	};
	AddUnit(1000.0, TEXT("k"));
	AddUnit(1000000.0, TEXT("M"));
	AddUnit(1000000000.0, TEXT("B"));
	AddUnit(1000000000000.0, TEXT("T"));
}

TPair<float, float> UUmbraDamageNumber::ResolveFontSizeRange(double Damage,
	const TArray<FUmbraDamageFontTier>& Tiers, float FallbackFontSize)
{
	TArray<FUmbraDamageFontTier> ValidTiers;
	for (const FUmbraDamageFontTier& Tier : Tiers)
	{
		if (!FMath::IsFinite(Tier.DamageLowerBound) || Tier.DamageLowerBound < 0.0
			|| !FMath::IsFinite(Tier.MinFontSize) || !FMath::IsFinite(Tier.MaxFontSize)
			|| Tier.MinFontSize <= 0.f || Tier.MaxFontSize <= 0.f)
		{
			continue;
		}
		FUmbraDamageFontTier Normalized = Tier;
		if (Normalized.MinFontSize > Normalized.MaxFontSize)
		{
			Swap(Normalized.MinFontSize, Normalized.MaxFontSize);
		}
		ValidTiers.Add(Normalized);
	}
	SortAndDeduplicateByValue(ValidTiers,
		[](const FUmbraDamageFontTier& Tier) { return Tier.DamageLowerBound; });

	const float SafeFallback = FMath::IsFinite(FallbackFontSize) && FallbackFontSize > 0.f
		? FallbackFontSize : 18.f;
	if (ValidTiers.IsEmpty())
	{
		return { SafeFallback, SafeFallback };
	}

	const double SafeDamage = FMath::IsFinite(Damage) ? FMath::Max(0.0, Damage) : 0.0;
	const FUmbraDamageFontTier* Selected = &ValidTiers[0];
	for (const FUmbraDamageFontTier& Tier : ValidTiers)
	{
		if (Tier.DamageLowerBound > SafeDamage)
		{
			break;
		}
		Selected = &Tier;
	}
	return { Selected->MinFontSize, Selected->MaxFontSize };
}

FString UUmbraDamageNumber::FormatDamage(double Damage, bool bEnableAbbreviation,
	double StartValue, int32 DecimalPlaces, const TArray<FUmbraDamageAbbreviationUnit>& Units)
{
	const double SafeDamage = FMath::IsFinite(Damage) ? Damage : 0.0;
	const double AbsoluteDamage = FMath::Abs(SafeDamage);
	const double SafeStart = FMath::IsFinite(StartValue) && StartValue >= 1.0 ? StartValue : 1000.0;
	const int32 SafeDecimals = FMath::Clamp(DecimalPlaces, 0, 6);

	TArray<FUmbraDamageAbbreviationUnit> ValidUnits;
	if (bEnableAbbreviation && AbsoluteDamage >= SafeStart)
	{
		for (const FUmbraDamageAbbreviationUnit& Unit : Units)
		{
			if (FMath::IsFinite(Unit.ValueThreshold) && Unit.ValueThreshold >= SafeStart
				&& !Unit.Suffix.IsEmpty())
			{
				ValidUnits.Add(Unit);
			}
		}
		SortAndDeduplicateByValue(ValidUnits,
			[](const FUmbraDamageAbbreviationUnit& Unit) { return Unit.ValueThreshold; });
	}

	if (!ValidUnits.IsEmpty())
	{
		int32 UnitIndex = INDEX_NONE;
		for (int32 Index = 0; Index < ValidUnits.Num(); ++Index)
		{
			if (ValidUnits[Index].ValueThreshold > AbsoluteDamage)
			{
				break;
			}
			UnitIndex = Index;
		}
		if (UnitIndex != INDEX_NONE)
		{
			const double DecimalScale = FMath::Pow(10.0, SafeDecimals);
			while (UnitIndex + 1 < ValidUnits.Num())
			{
				const double Scaled = AbsoluteDamage / ValidUnits[UnitIndex].ValueThreshold;
				const double Rounded = FMath::RoundToDouble(Scaled * DecimalScale) / DecimalScale;
				const double PromotionPoint = ValidUnits[UnitIndex + 1].ValueThreshold
					/ ValidUnits[UnitIndex].ValueThreshold;
				if (Rounded < PromotionPoint)
				{
					break;
				}
				++UnitIndex;
			}

			FNumberFormattingOptions Options;
			Options.SetUseGrouping(false);
			Options.SetMinimumFractionalDigits(SafeDecimals);
			Options.SetMaximumFractionalDigits(SafeDecimals);
			const double ScaledDamage = SafeDamage / ValidUnits[UnitIndex].ValueThreshold;
			return FText::AsNumber(ScaledDamage, &Options).ToString() + ValidUnits[UnitIndex].Suffix;
		}
	}

	FNumberFormattingOptions IntegerOptions;
	IntegerOptions.SetUseGrouping(false);
	IntegerOptions.SetMinimumFractionalDigits(0);
	IntegerOptions.SetMaximumFractionalDigits(0);
	return FText::AsNumber(SafeDamage, &IntegerOptions).ToString();
}

int32 UUmbraDamageNumber::FinalizeFontSize(float SampledSize, bool bCritical,
	float CriticalMultiplier, float FinalSizeCap)
{
	float SafeSize = FMath::IsFinite(SampledSize) ? FMath::Max(1.f, SampledSize) : 18.f;
	if (bCritical)
	{
		SafeSize *= FMath::IsFinite(CriticalMultiplier)
			? FMath::Max(0.01f, CriticalMultiplier) : 1.15f;
	}
	const float SafeCap = FMath::IsFinite(FinalSizeCap) && FinalSizeCap >= 1.f
		? FinalSizeCap : 28.f;
	return FMath::Max(1, FMath::RoundToInt(FMath::Min(SafeSize, SafeCap)));
}

void UUmbraDamageNumber::ApplySnapshotFontSize(double Damage, bool bCritical)
{
	if (!DamageText)
	{
		return;
	}
	FSlateFontInfo Font = DamageText->GetFont();
	const TPair<float, float> Range = ResolveFontSizeRange(Damage, FontSizeTiers, Font.Size);
	// Sample exactly once per spawned number; the pop animation only changes render scale.
	const float FontSize = FMath::FRandRange(Range.Key, Range.Value);
	Font.Size = FinalizeFontSize(FontSize, bCritical, CriticalFontSizeMultiplier, MaxFinalFontSize);
	DamageText->SetFont(Font);
}

void UUmbraDamageNumber::Start(float Damage, bool bMagical, bool bCritical, FVector WorldPosition)
{
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	WorldOrigin = WorldPosition;
	WorldTravel = FVector::ZeroVector;
	const float Spread = FMath::IsFinite(SpreadAngleDegrees) ? FMath::Clamp(SpreadAngleDegrees, 0.f, 360.f) : 120.f;
	const float Angle = FMath::DegreesToRadians(FMath::FRandRange(-Spread * 0.5f, Spread * 0.5f));
	const float Distance = FMath::FRandRange(FMath::Max(0.f, MinDistance), FMath::Max(FMath::Max(0.f, MinDistance), MaxDistance));
	const FVector2D Travel = FVector2D(FMath::Sin(Angle), -FMath::Cos(Angle)) * Distance;
	if (APlayerController* PC = GetOwningPlayer())
	{
		FVector2D Pixels;
		const float DPI = UWidgetLayoutLibrary::GetViewportScale(this);
		// Deprojection uses full-viewport pixels, so do not subtract the local player's origin here.
		if (DPI > 0.f && PC->ProjectWorldLocationToScreen(WorldOrigin, Pixels, false))
		{
			const FVector2D EndPixels = Pixels + Travel * DPI;
			FVector RayOrigin, RayDirection, ViewLocation;
			FRotator ViewRotation;
			PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
			const FVector PlaneNormal = ViewRotation.Vector();
			if (PC->DeprojectScreenPositionToWorld(EndPixels.X, EndPixels.Y, RayOrigin, RayDirection))
			{
				const double Denominator = FVector::DotProduct(RayDirection, PlaneNormal);
				if (!FMath::IsNearlyZero(Denominator))
				{
					const double RayDistance = FVector::DotProduct(WorldOrigin - RayOrigin, PlaneNormal) / Denominator;
					if (FMath::IsFinite(RayDistance) && RayDistance >= 0.0)
					{
						WorldTravel = RayOrigin + RayDirection * RayDistance - WorldOrigin;
					}
				}
			}
		}
	}
	EndScale = bCritical ? CriticalScale : NormalScale;
	if (DamageText)
	{
		ApplySnapshotFontSize(Damage, bCritical);
		FString Text = FormatDamage(Damage, bEnableDamageAbbreviation,
			AbbreviationStartValue, AbbreviationDecimalPlaces, AbbreviationUnits);
		if (bCritical)
		{
			Text += TEXT("!");
		}
		DamageText->SetText(FText::FromString(Text));
		DamageText->SetColorAndOpacity(bMagical ? MagicalColor : PhysicalColor);
	}
	Elapsed = 0.f;
	bStarted = true;
	UpdateAnimation();
}

void UUmbraDamageNumber::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	if (!bStarted)
	{
		return;
	}
	Elapsed += FMath::Max(0.f, DeltaTime);
	UpdateAnimation();
}

void UUmbraDamageNumber::UpdateAnimation()
{
	const float Appear = FMath::Max(0.f, AppearDuration);
	const float Hold = FMath::Max(0.f, HoldDuration);
	const float Fade = FMath::Max(0.f, FadeDuration);
	const float Alpha = Appear > 0.f ? FMath::Clamp(Elapsed / Appear, 0.f, 1.f) : 1.f;
	const float Smooth = Alpha * Alpha * (3.f - 2.f * Alpha);
	bool bOnScreen = false;
	if (APlayerController* PC = GetOwningPlayer())
	{
		FVector2D Pixels;
		const float DPI = UWidgetLayoutLibrary::GetViewportScale(this);
		if (DPI > 0.f && PC->ProjectWorldLocationToScreen(WorldOrigin + WorldTravel * Smooth, Pixels, true))
		{
			int32 Width = 0, Height = 0;
			PC->GetViewportSize(Width, Height);
			const ULocalPlayer* LP = PC->GetLocalPlayer();
			const FVector2D Size = LP ? FVector2D(Width * LP->Size.X, Height * LP->Size.Y) : FVector2D(Width, Height);
			bOnScreen = Pixels.X >= 0.f && Pixels.Y >= 0.f && Pixels.X < Size.X && Pixels.Y < Size.Y;
			if (bOnScreen)
			{
				SetPositionInViewport(Pixels / DPI, false);
			}
		}
	}
	const float Pop = FMath::IsFinite(PopDuration) ? FMath::Max(0.001f, PopDuration) : 0.08f;
	const float ScaleAlpha = FMath::Clamp(Elapsed / Pop, 0.f, 1.f);
	const float ScaleSmooth = ScaleAlpha * ScaleAlpha * (3.f - 2.f * ScaleAlpha);
	SetRenderScale(FVector2D(FMath::Lerp(InitialScale, EndScale, ScaleSmooth)));
	const float FadeElapsed = Elapsed - Appear - Hold;
	SetRenderOpacity(bOnScreen ? (FadeElapsed <= 0.f ? 1.f
		: (Fade > 0.f ? 1.f - FMath::Clamp(FadeElapsed / Fade, 0.f, 1.f) : 0.f)) : 0.f);
	if (Elapsed >= Appear + Hold + Fade)
	{
		bStarted = false;
		if (auto* PC = Cast<AUmbraPlayerController>(GetOwningPlayer()))
		{
			PC->ReleaseDamageNumber(this);
		}
		RemoveFromParent();
	}
}

void UUmbraDamageNumber::NativeDestruct()
{
	bStarted = false;
	if (auto* PC = Cast<AUmbraPlayerController>(GetOwningPlayer()))
	{
		PC->ReleaseDamageNumber(this);
	}
	Super::NativeDestruct();
}

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraDamageNumber.generated.h"

class UTextBlock;

/** One damage-size band. The next band's lower bound is this band's upper bound. */
USTRUCT(BlueprintType)
struct FUmbraDamageFontTier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Number",
		meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Inclusive unformatted damage lower bound."))
	double DamageLowerBound = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Number", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MinFontSize = 16.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Number", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxFontSize = 18.f;
};

/** A display unit. Entries are normalized by threshold at runtime. */
USTRUCT(BlueprintType)
struct FUmbraDamageAbbreviationUnit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Number", meta = (ClampMin = "1.0", UIMin = "1.0"))
	double ValueThreshold = 1000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Number")
	FString Suffix = TEXT("k");
};

UCLASS(Abstract, Blueprintable)
class UMBRA_API UUmbraDamageNumber : public UUserWidget
{
	GENERATED_BODY()

public:
	UUmbraDamageNumber(const FObjectInitializer& ObjectInitializer);
	void Start(float Damage, bool bMagical, bool bCritical, FVector WorldPosition);

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageText;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Animation")
	float InitialScale = 0.4f;
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Animation")
	float NormalScale = 1.f;
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Animation")
	float CriticalScale = 1.3f;

	/** Movement duration; retained under its original name for existing Blueprint defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Animation", meta = (ClampMin = "0"))
	float AppearDuration = 0.25f;
	/** Scaling only; does not change movement or the hold/fade schedule. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Animation", meta = (ClampMin = "0.001", Units = "s"))
	float PopDuration = 0.08f;
	/** Full angular width centered on screen up (negative screen Y). */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Animation", meta = (ClampMin = "0", ClampMax = "360"))
	float SpreadAngleDegrees = 120.f;
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Animation", meta = (ClampMin = "0"))
	float HoldDuration = 0.3f;
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Animation", meta = (ClampMin = "0"))
	float FadeDuration = 0.3f;
	/** UMG units at spawn; converted once into a fixed world-space displacement. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Animation", meta = (ClampMin = "0"))
	float MinDistance = 40.f;
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Animation", meta = (ClampMin = "0"))
	float MaxDistance = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Style")
	FLinearColor PhysicalColor = FLinearColor::White;
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Style")
	FLinearColor MagicalColor = FLinearColor(0.45f, 0.75f, 1.f, 1.f);
	/** Actual damage selects a tier before any display abbreviation is applied. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Number|Style",
		meta = (TitleProperty = "DamageLowerBound", ToolTip = "Sorted at runtime; duplicate bounds use the last valid entry."))
	TArray<FUmbraDamageFontTier> FontSizeTiers;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Number|Style",
		meta = (ClampMin = "0.01", UIMin = "0.01", ToolTip = "Applied to the one-time sampled font size for critical hits."))
	float CriticalFontSizeMultiplier = 1.15f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Number|Style", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxFinalFontSize = 28.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Number|Formatting")
	bool bEnableDamageAbbreviation = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Number|Formatting", meta = (ClampMin = "1.0", UIMin = "1.0"))
	double AbbreviationStartValue = 1000.0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Number|Formatting", meta = (ClampMin = "0", ClampMax = "6", UIMin = "0", UIMax = "3"))
	int32 AbbreviationDecimalPlaces = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Number|Formatting", meta = (TitleProperty = "Suffix"))
	TArray<FUmbraDamageAbbreviationUnit> AbbreviationUnits;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FUmbraWorldPresentationTest;
	friend class FUmbraDamageNumberLogicTest;
#endif
	static TPair<float, float> ResolveFontSizeRange(double Damage,
		const TArray<FUmbraDamageFontTier>& Tiers, float FallbackFontSize);
	static int32 FinalizeFontSize(float SampledSize, bool bCritical,
		float CriticalMultiplier, float FinalSizeCap);
	static FString FormatDamage(double Damage, bool bEnableAbbreviation, double StartValue,
		int32 DecimalPlaces, const TArray<FUmbraDamageAbbreviationUnit>& Units);
	void ApplySnapshotFontSize(double Damage, bool bCritical);
	void UpdateAnimation();

	// Snapshot only: neither the victim nor the camera can move these after Start.
	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldTravel = FVector::ZeroVector;
	float Elapsed = 0.f;
	float EndScale = 1.f;
	bool bStarted = false;
};

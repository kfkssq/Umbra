#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/UmbraDamageNumber.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraDamageNumberLogicTest, "Umbra.UI.DamageNumberLogic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraDamageNumberLogicTest::RunTest(const FString& Parameters)
{
	const FProperty* FontTiersProperty = FindFProperty<FProperty>(
		UUmbraDamageNumber::StaticClass(), GET_MEMBER_NAME_CHECKED(UUmbraDamageNumber, FontSizeTiers));
	if (TestNotNull(TEXT("Font size tiers are reflected"), FontTiersProperty))
	{
		TestTrue(TEXT("Font size tiers are editable in Widget Blueprint defaults"),
			FontTiersProperty->HasAnyPropertyFlags(CPF_Edit));
		TestTrue(TEXT("Font size tiers are exposed to Blueprint graphs"),
			FontTiersProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
		TestFalse(TEXT("Font size tiers are writable rather than Blueprint read-only"),
			FontTiersProperty->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
	}

	TArray<FUmbraDamageAbbreviationUnit> Units;
	const auto AddUnit = [&Units](double Threshold, const TCHAR* Suffix)
	{
		FUmbraDamageAbbreviationUnit& Unit = Units.AddDefaulted_GetRef();
		Unit.ValueThreshold = Threshold;
		Unit.Suffix = Suffix;
	};
	// Intentionally unsorted: runtime validation must normalize editor data.
	AddUnit(1000000000.0, TEXT("B"));
	AddUnit(1000.0, TEXT("k"));
	AddUnit(1000000000000.0, TEXT("T"));
	AddUnit(1000000.0, TEXT("M"));

	TestEqual(TEXT("Below threshold is an integer"),
		UUmbraDamageNumber::FormatDamage(999.0, true, 1000.0, 1, Units), FString(TEXT("999")));
	TestEqual(TEXT("Thousands keep one decimal"),
		UUmbraDamageNumber::FormatDamage(14400.0, true, 1000.0, 1, Units), FString(TEXT("14.4k")));
	TestEqual(TEXT("Threshold includes one decimal"),
		UUmbraDamageNumber::FormatDamage(1000.0, true, 1000.0, 1, Units), FString(TEXT("1.0k")));
	TestEqual(TEXT("Rounded thousands promote to millions"),
		UUmbraDamageNumber::FormatDamage(999960.0, true, 1000.0, 1, Units), FString(TEXT("1.0M")));
	TestEqual(TEXT("Millions format independently of font tier"),
		UUmbraDamageNumber::FormatDamage(1500000.0, true, 1000.0, 1, Units), FString(TEXT("1.5M")));
	TestEqual(TEXT("Disabled abbreviation returns an integer"),
		UUmbraDamageNumber::FormatDamage(14400.0, false, 1000.0, 1, Units), FString(TEXT("14400")));
	TArray<FUmbraDamageAbbreviationUnit> DuplicateUnits;
	FUmbraDamageAbbreviationUnit& EarlierUnit = DuplicateUnits.AddDefaulted_GetRef();
	EarlierUnit.ValueThreshold = 1000.0;
	EarlierUnit.Suffix = TEXT("old");
	FUmbraDamageAbbreviationUnit& LaterUnit = DuplicateUnits.AddDefaulted_GetRef();
	LaterUnit.ValueThreshold = 1000.0;
	LaterUnit.Suffix = TEXT("new");
	TestEqual(TEXT("Last duplicate abbreviation unit wins"),
		UUmbraDamageNumber::FormatDamage(1500.0, true, 1000.0, 1, DuplicateUnits), FString(TEXT("1.5new")));
	const TArray<FUmbraDamageAbbreviationUnit> EmptyUnits;
	TestEqual(TEXT("Empty unit configuration safely falls back to integer"),
		UUmbraDamageNumber::FormatDamage(1500.0, true, 1000.0, 1, EmptyUnits), FString(TEXT("1500")));

	TArray<FUmbraDamageFontTier> Tiers;
	const auto AddTier = [&Tiers](double Lower, float MinSize, float MaxSize)
	{
		FUmbraDamageFontTier& Tier = Tiers.AddDefaulted_GetRef();
		Tier.DamageLowerBound = Lower;
		Tier.MinFontSize = MinSize;
		Tier.MaxFontSize = MaxSize;
	};
	AddTier(1000000.0, 20.f, 22.f);
	AddTier(0.0, 16.f, 18.f);
	AddTier(1000.0, 18.f, 20.f);
	AddTier(1000000000.0, 24.f, 22.f); // Reversed sizes are normalized.
	const TPair<float, float> FirstRange = UUmbraDamageNumber::ResolveFontSizeRange(999.0, Tiers, 13.f);
	TestEqual(TEXT("999 uses first tier minimum"), FirstRange.Key, 16.f);
	TestEqual(TEXT("999 uses first tier maximum"), FirstRange.Value, 18.f);
	const TPair<float, float> SecondRange = UUmbraDamageNumber::ResolveFontSizeRange(1000.0, Tiers, 13.f);
	TestEqual(TEXT("1000 uses inclusive second tier minimum"), SecondRange.Key, 18.f);
	TestEqual(TEXT("1000 uses inclusive second tier maximum"), SecondRange.Value, 20.f);
	const TPair<float, float> LastRange = UUmbraDamageNumber::ResolveFontSizeRange(1000000000.0, Tiers, 13.f);
	TestEqual(TEXT("One billion uses normalized last tier minimum"), LastRange.Key, 22.f);
	TestEqual(TEXT("One billion uses normalized last tier maximum"), LastRange.Value, 24.f);
	const TArray<FUmbraDamageFontTier> EmptyTiers;
	const TPair<float, float> FallbackRange = UUmbraDamageNumber::ResolveFontSizeRange(1000.0, EmptyTiers, 17.f);
	TestEqual(TEXT("Empty tier configuration uses font fallback minimum"), FallbackRange.Key, 17.f);
	TestEqual(TEXT("Empty tier configuration uses font fallback maximum"), FallbackRange.Value, 17.f);
	AddTier(1000.0, 19.f, 21.f);
	const TPair<float, float> DuplicateRange = UUmbraDamageNumber::ResolveFontSizeRange(1000.0, Tiers, 13.f);
	TestEqual(TEXT("Last duplicate font tier wins minimum"), DuplicateRange.Key, 19.f);
	TestEqual(TEXT("Last duplicate font tier wins maximum"), DuplicateRange.Value, 21.f);
	TestEqual(TEXT("Critical multiplier is applied once"),
		UUmbraDamageNumber::FinalizeFontSize(20.f, true, 1.15f, 28.f), 23);
	TestEqual(TEXT("Final size cap is enforced"),
		UUmbraDamageNumber::FinalizeFontSize(24.f, true, 1.5f, 28.f), 28);

	return true;
}

#endif

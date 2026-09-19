#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "UI/UmbraPlayerAttributeBar.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraPlayerAttributeBarStateTest, "Umbra.UI.PlayerAttributeBarState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FUmbraPlayerAttributeBarStateTest::RunTest(const FString& Parameters)
{
	UUmbraAttributeSet* Attributes = NewObject<UUmbraAttributeSet>();
	Attributes->InitMaxHealth(200.f);
	Attributes->InitHealth(50.f);
	Attributes->InitMaxResource(80.f);
	Attributes->InitResource(20.f);

	FUmbraPlayerAttributeBarViewState Health = UUmbraPlayerAttributeBar::MakeViewState(
		Attributes, true, EUmbraPlayerAttributeBarType::Health);
	TestEqual(TEXT("Health type"), Health.AttributeType, EUmbraPlayerAttributeBarType::Health);
	TestTrue(TEXT("Health state is ready"), Health.bReady);
	TestEqual(TEXT("Health current value"), Health.CurrentValue, 50.f);
	TestEqual(TEXT("Health max value"), Health.MaxValue, 200.f);
	TestEqual(TEXT("Health ratio"), Health.Normalized, 0.25f);

	FUmbraPlayerAttributeBarViewState Resource = UUmbraPlayerAttributeBar::MakeViewState(
		Attributes, true, EUmbraPlayerAttributeBarType::Resource);
	TestEqual(TEXT("Resource type"), Resource.AttributeType, EUmbraPlayerAttributeBarType::Resource);
	TestTrue(TEXT("Resource state is ready"), Resource.bReady);
	TestEqual(TEXT("Resource current value"), Resource.CurrentValue, 20.f);
	TestEqual(TEXT("Resource max value"), Resource.MaxValue, 80.f);
	TestEqual(TEXT("Resource ratio"), Resource.Normalized, 0.25f);

	Attributes->InitHealth(300.f);
	Attributes->InitResource(-10.f);
	Health = UUmbraPlayerAttributeBar::MakeViewState(Attributes, true, EUmbraPlayerAttributeBarType::Health);
	Resource = UUmbraPlayerAttributeBar::MakeViewState(Attributes, true, EUmbraPlayerAttributeBarType::Resource);
	TestEqual(TEXT("Health presentation ratio clamps high"), Health.Normalized, 1.f);
	TestEqual(TEXT("Resource presentation ratio clamps low"), Resource.Normalized, 0.f);

	Attributes->InitMaxResource(0.f);
	Resource = UUmbraPlayerAttributeBar::MakeViewState(Attributes, true, EUmbraPlayerAttributeBarType::Resource);
	TestEqual(TEXT("Zero max resource is safe"), Resource.Normalized, 0.f);

	Health = UUmbraPlayerAttributeBar::MakeViewState(nullptr, true, EUmbraPlayerAttributeBarType::Health);
	TestFalse(TEXT("Missing attributes are not ready"), Health.bReady);
	return true;
}

#endif

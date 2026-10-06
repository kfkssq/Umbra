#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/UmbraCombatInfoTestTypes.h"
#include "Misc/AutomationTest.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraCombatInfoTest, "Umbra.UI.CombatInfo.SectionLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraCombatInfoTest::RunTest(const FString& Parameters)
{
    auto* Page = NewObject<UUmbraCombatInfoTestWidget>();
    auto* Attack = NewObject<UButton>(Page);
    auto* Defense = NewObject<UButton>(Page);
    auto* AttackArrow = NewObject<UTextBlock>(Page);
    auto* DefenseArrow = NewObject<UTextBlock>(Page);
    auto* AttackRows = NewObject<UVerticalBox>(Page);
    auto* DefenseRows = NewObject<UVerticalBox>(Page);
    auto* Row = NewObject<UTextBlock>(Page);
    AttackRows->AddChild(Row);
    Page->Configure(Attack, Defense, AttackArrow, DefenseArrow, AttackRows, DefenseRows);
    auto* Utility = NewObject<UButton>(Page);
    auto* UtilityArrow = NewObject<UTextBlock>(Page);
    auto* UtilityRows = NewObject<UVerticalBox>(Page);
    Page->ConfigureUtility(Utility, UtilityArrow, UtilityRows);
    Page->ConstructForTest();
    Page->ConstructForTest();
    TestEqual(TEXT("Attack initially expanded"), AttackRows->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
    TestEqual(TEXT("Defense initially expanded"), DefenseRows->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
    Attack->OnClicked.Broadcast();
    TestEqual(TEXT("Repeated Construct binds once"), AttackRows->GetVisibility(), ESlateVisibility::Collapsed);
    TestEqual(TEXT("Attack arrow follows collapse"), AttackArrow->GetText().ToString(), FString(TEXT("\u25b6")));
    TestEqual(TEXT("Defense independent"), DefenseRows->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
    Utility->OnClicked.Broadcast();
    TestEqual(TEXT("Utility collapses independently"), UtilityRows->GetVisibility(), ESlateVisibility::Collapsed);
    TestEqual(TEXT("Collapse preserves existing row"), AttackRows->GetChildAt(0), static_cast<UWidget*>(Row));
    Page->DestructForTest();
    TestFalse(TEXT("Attack unbound after Destruct"), Attack->OnClicked.IsBound());
    TestFalse(TEXT("Defense unbound after Destruct"), Defense->OnClicked.IsBound());
    TestFalse(TEXT("Utility unbound after Destruct"), Utility->OnClicked.IsBound());
    Page->ConstructForTest();
    TestEqual(TEXT("Reopen preserves collapsed state"), AttackRows->GetVisibility(), ESlateVisibility::Collapsed);
    TestEqual(TEXT("Reopen preserves utility state"), UtilityRows->GetVisibility(), ESlateVisibility::Collapsed);
    Attack->OnClicked.Broadcast();
    TestEqual(TEXT("Rebound header expands"), AttackRows->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
    TestEqual(TEXT("Expanded arrow restored"), AttackArrow->GetText().ToString(), FString(TEXT("\u25bc")));
    Defense->OnClicked.Broadcast();
    TestEqual(TEXT("Defense collapses independently"), DefenseRows->GetVisibility(), ESlateVisibility::Collapsed);
    TestEqual(TEXT("Attack remains expanded"), AttackRows->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
    TestEqual(TEXT("No rows appended on reopen"), AttackRows->GetChildrenCount(), 1);
    Page->DestructForTest();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraCombatTestTextTest, "Umbra.UI.CombatInfo.TestTextWithoutGameplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraCombatTestTextTest::RunTest(const FString& Parameters)
{
    // No world, player or ASC: these are complete display strings, including percent units.
    auto* Entry = NewObject<UUmbraCombatStatEntryTestWidget>();
    Entry->SetDesignerFlags(EWidgetDesignFlags::Designing);
    auto* Name = NewObject<UTextBlock>(Entry);
    auto* Value = NewObject<UTextBlock>(Entry);
    for (const TCHAR* Text : { TEXT("186"), TEXT("122.0%"), TEXT("10.1%"), TEXT("50.0%"), TEXT("2896"), TEXT("19.2%") })
    {
        Entry->Configure(Name, Value, FText::FromString(TEXT("UI test")), FText::FromString(Text));
        Entry->PreConstructForTest();
        TestEqual(TEXT("Test value preserved verbatim"), Value->GetText().ToString(), FString(Text));
        TestEqual(TEXT("Instance supplies the name"), Name->GetText().ToString(), FString(TEXT("UI test")));
    }
    Entry->Configure(Name, Value, FText::FromString(TEXT("Empty")), FText::GetEmpty());
    Entry->PreConstructForTest();
    TestEqual(TEXT("Empty value clears previous row text"), Value->GetText().ToString(), FString(TEXT("—")));
    // SetDesignerFlags adds flags; a fresh instance represents runtime, without Designer state.
    auto* RuntimeEntry = NewObject<UUmbraCombatStatEntryTestWidget>();
    auto* RuntimeName = NewObject<UTextBlock>(RuntimeEntry);
    auto* RuntimeValue = NewObject<UTextBlock>(RuntimeEntry);
    RuntimeEntry->Configure(RuntimeName, RuntimeValue, FText::FromString(TEXT("Runtime")), FText::FromString(TEXT("9999")));
    RuntimeEntry->PreConstructForTest();
    TestEqual(TEXT("Runtime never shows prototype test data"), RuntimeValue->GetText().ToString(), FString(TEXT("—")));
    return true;
}
#endif

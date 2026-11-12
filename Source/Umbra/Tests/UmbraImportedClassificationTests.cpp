#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Items/UmbraItemDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraImportedClassificationTest, "Umbra.Items.ImportedClassification",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraImportedClassificationTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Existing serialized Crossbow identity preserved"), uint8(EUmbraWeaponType::Crossbow), uint8(14));
    auto* Item = NewObject<UUmbraItemDefinition>();
    Item->ItemCategory = EUmbraItemCategory::Weapon;
    const FText Generic = UmbraItemClassification::WeaponTypeName(EUmbraWeaponType::Unknown);
    for (uint8 Value = 15; Value <= uint8(EUmbraWeaponType::BeastClaw); ++Value)
    {
        Item->WeaponType = static_cast<EUmbraWeaponType>(Value);
        TArray<FText> Warnings;
        const FText Name = UmbraItemClassification::Describe(*Item, Warnings);
        TestFalse(TEXT("Imported type has a specific localized title"), Name.EqualTo(Generic));
        TestEqual(TEXT("Draft only warns about missing gameplay profile"), Warnings.Num(), 1);
        TestNull(TEXT("Classification never creates damage profile"), Item->Weapon.Get());
        TestTrue(TEXT("Classification never configures equip slots"), Item->AllowedSlots.IsEmpty());
    }
    Item->WeaponType = static_cast<EUmbraWeaponType>(255);
    TArray<FText> Warnings;
    TestTrue(TEXT("Invalid enum falls back to generic weapon"), UmbraItemClassification::Describe(*Item, Warnings).EqualTo(Generic));
    return true;
}
#endif

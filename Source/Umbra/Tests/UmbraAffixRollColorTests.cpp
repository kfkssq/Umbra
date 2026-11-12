#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Items/UmbraItemTooltipData.h"
#include "Items/UmbraItemGeneration.h"
#include "UI/Items/UmbraTooltipAffixEntry.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraAffixRollColorTest, "Umbra.UI.Items.AffixRollColor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraAffixRollColorTest::RunTest(const FString& Parameters)
{
    FUmbraItemTooltipData Data;
    Data.ItemDefinition = NewObject<UUmbraItemDefinition>();
    Data.ItemDefinition->GenerationProfile = NewObject<UUmbraItemGenerationProfile>();
    auto* Profile = Data.ItemDefinition->GenerationProfile.Get();
    const FColor Colors[] = { FColor(255,255,255), FColor(75,203,99), FColor(68,138,255),
        FColor(176,112,255), FColor(232,62,140), FColor(255,211,77) };
    for (int32 I = 0; I < 6; ++I)
    {
        auto& Rarity = Profile->Rarities.AddDefaulted_GetRef();
        Rarity.Rarity = EUmbraItemRarity(I);
        Rarity.Color = FLinearColor::FromSRGBColor(Colors[I]);
        auto& Stat = Data.Stats.AddDefaulted_GetRef();
        Stat.StatId = TEXT("Strength");
        Stat.Source = EUmbraTooltipStatSource::RolledAffix;
        Stat.bRolled = true;
        Stat.CandidateValues = { 2, 4, 6, 8, 10, 12 };
        Stat.SelectedTier = I;
        Stat.RawValue = Stat.CandidateValues[I];
    }
    Data.Rarity = EUmbraItemRarity::Epic;
    UmbraTooltipFormatting::BuildDisplayText(Data);
    for (int32 I = 0; I < 6; ++I)
        TestEqual(TEXT("Rank color independent of item rarity"), Data.AffixEntries[I].RollColor, Profile->Rarities[I].Color);
    // A configured palette overrides defaults, not the numerical spacing of a roll.
    Profile->Rarities[1].Color = FLinearColor(0.f, 1.f, 1.f, 1.f);
    Data.Stats[1].CandidateValues = { 1, 2, 4, 8, 16, 32 };
    Data.Stats[1].RawValue = 2;
    UmbraTooltipFormatting::BuildDisplayText(Data);
    TestEqual(TEXT("Uses configured rarity color and discrete rank"), Data.AffixEntries[1].RollColor, FLinearColor(0.f, 1.f, 1.f, 1.f));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    auto* PC = World->SpawnActor<APlayerController>();
    PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    UClass* Class = LoadClass<UUmbraTooltipAffixEntry>(nullptr, TEXT("/Game/UI/Tooltip/WBP_TooltipAffixEntry.WBP_TooltipAffixEntry_C"));
    if (TestNotNull(TEXT("Authored affix WBP loads"), Class))
    {
        auto* Widget = CreateWidget<UUmbraTooltipAffixEntry>(PC, Class);
        Widget->TakeWidget();
        for (const auto& Entry : Data.AffixEntries)
        {
            Widget->SetAffixData(Entry);
            for (const FName Name : {FName("AffixName"), FName("AffixValue"), FName("AffixRange")})
            {
                auto* Text = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(Name));
                if (TestNotNull(TEXT("Affix text exists"), Text))
                    TestEqual(TEXT("Reused row applies rank to all text"), Text->GetColorAndOpacity().GetSpecifiedColor(), Entry.RollColor);
            }
        }
        Widget->SetAffixData(FUmbraTooltipAffixDisplayData());
        auto* Text = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("AffixValue")));
        if (Text) TestEqual(TEXT("Reusing with fixed data clears gold"), Text->GetColorAndOpacity().GetSpecifiedColor(), FLinearColor::White);
    }
    for (auto& Stat : Data.Stats) Stat.bConfigurationPreview = true;
    UmbraTooltipFormatting::BuildDisplayText(Data);
    for (const auto& Entry : Data.AffixEntries) TestEqual(TEXT("Preview has no rolled rank"), Entry.RollColor, FLinearColor::White);
    Data.Stats[0].bConfigurationPreview = false;
    Data.Stats[0].SelectedTier = INDEX_NONE;
    UmbraTooltipFormatting::BuildDisplayText(Data);
    TestEqual(TEXT("Invalid rank remains neutral"), Data.AffixEntries[0].RollColor, FLinearColor::White);
    Data.SpecialEffectStats.Reset();
    for (int32 Rank : {4,5,0,3,5,1})
    {
        Data.Rarity = EUmbraItemRarity(Rank);
        UmbraTooltipFormatting::BuildDisplayText(Data);
        TestEqual(TEXT("Tooltip special marker"),Data.bHasSpecialEffect,Rank>=4);
        TestEqual(TEXT("Tooltip unique marker"),Data.bIsUnique,Rank==5);
        TestEqual(TEXT("Only pending special text, no invented power"),Data.SpecialEffects.Num(),Rank==5?2:Rank==4?1:0);
    }
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}
#endif

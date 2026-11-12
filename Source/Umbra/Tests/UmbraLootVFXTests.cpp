#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Loot/UmbraWorldItem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraLootVFXAssetAudit, "Umbra.Loot.VFXAssetAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraLootVFXAssetAudit::RunTest(const FString&)
{
	FString Report;
	for (const TCHAR* Name : {TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"), TEXT("Epic"), TEXT("Legendary"), TEXT("Mythic")})
	{
		const FString Path = FString(TEXT("/Game/LootDropVFX/Systems/Rarity/NS_LootDrop_")) + Name;
		auto* System = LoadObject<UNiagaraSystem>(nullptr, *Path);
		if (!TestNotNull(*Path, System)) continue;
		Report += Path + TEXT("\n");
		TArray<FNiagaraVariable> Variables;
		const auto& Store = System->GetExposedParameters();
		Store.GetParameters(Variables);
		for (auto& Variable : Variables)
		{
			if (const uint8* Data = Store.GetParameterData(Variable)) Variable.SetData(Data);
			Report += Variable.ToString() + TEXT("\n");
		}
	}
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("LootVFX");
	IFileManager::Get().MakeDirectory(*Directory, true);
	FFileHelper::SaveStringToFile(Report, *(Directory / TEXT("parameters.txt")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraLootVFXTest, "Umbra.Loot.VFXLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraLootVFXTest::RunTest(const FString&)
{
	auto* Class = LoadClass<AUmbraWorldItem>(nullptr, TEXT("/Game/Items/Loot/BP_WorldItem.BP_WorldItem_C"));
	if (!TestNotNull(TEXT("Production drop class"), Class)) return false;
	const auto* Defaults = Class->GetDefaultObject<AUmbraWorldItem>();
	if (!TestEqual(TEXT("Six configured VFX styles"), Defaults->LootVFXStyles.Num(), 6)) return false;
	auto* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL()); World->BeginPlay();
	auto* Owner = World->SpawnActor<AActor>();
	for (uint8 Index=0; Index<6; ++Index)
	{
		auto* Definition = NewObject<UUmbraItemDefinition>(Owner);
		Definition->DefinitionId = FName(*FString::Printf(TEXT("VFX.%d"), Index));
		Definition->Rarity = EUmbraItemRarity(Index);
		Definition->RarityColor = FLinearColor(0.1f,0.3f,0.7f,1.f);
		FUmbraItemInstance Instance;
		TestTrue(TEXT("Valid instance"), FUmbraItemInstance::Create(Owner, Definition, FGuid::NewGuid(), Instance));
		auto* Drop = World->SpawnActorDeferred<AUmbraWorldItem>(Class, FTransform::Identity);
		Drop->DropPhysics.bEnabled = false;
		Drop->InitializeInstance(Instance);
		Drop->FinishSpawning(FTransform::Identity);
		if (!Drop->HasActorBegunPlay()) Drop->DispatchBeginPlay();
		const auto& Style = Defaults->LootVFXStyles.FindChecked(EUmbraItemRarity(Index));
		TestEqual(TEXT("Correct rarity system"), Drop->LootVFX->GetAsset(), Style.System.Get());
		TestTrue(TEXT("Upright rotation"), Drop->LootVFX->IsUsingAbsoluteRotation());
		Drop->SetActorRotation(FRotator(40,70,20));
		TestTrue(TEXT("Tumbling body does not tilt beam"), Drop->LootVFX->GetComponentRotation().IsNearlyZero());
		const auto Expected = Style.bOverrideColor ? Style.Color : Instance.GetRarityColor();
		for (FName Parameter : Defaults->LootVFXColorParameters)
		{
			const FNiagaraVariable Variable(FNiagaraTypeDefinition::GetColorDef(), Parameter);
			TestTrue(TEXT("Parameter exposed by asset"), Style.System->GetExposedParameters().IndexOf(Variable) != INDEX_NONE);
			const auto Original = Style.System->GetExposedParameters().GetParameterValue<FLinearColor>(Variable);
			const float Intensity = FMath::Max3(Original.R, Original.G, Original.B);
			const FLinearColor Layer(Expected.R*Intensity, Expected.G*Intensity, Expected.B*Intensity, Expected.A*Original.A);
			TestEqual(TEXT("Runtime color preserves layer intensity"), Drop->LootVFX->GetOverrideParameters().GetParameterValue<FLinearColor>(Variable), Layer);
		}
		Drop->RefreshPresentation(); Drop->RefreshVisual();
		TestEqual(TEXT("Refresh preserves asset"), Drop->LootVFX->GetAsset(), Style.System.Get());
		Drop->bPickedUp = true; Drop->RefreshPresentation();
		TestNull(TEXT("Pickup clears effect asset"), Drop->LootVFX->GetAsset());
		TestFalse(TEXT("Pickup stops effect"), Drop->LootVFX->IsActive());
		Drop->Destroy();
	}
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Items/UmbraItemGeneration.h"
#include "Items/UmbraItemTooltipData.h"
#include "UI/Equipment/UmbraEquipmentTypes.h"
#include "Inventory/UmbraInventoryComponent.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Equipment/UmbraEquipmentEffectContext.h"
#include "Equipment/UmbraEquipmentEffect.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "UObject/CoreNet.h"
#include "Player/UmbraPlayerState.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Loot/UmbraWorldItem.h"
#include "Tests/UmbraLootTestTypes.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include <limits>

namespace UmbraGenerationTests
{
struct FFixture
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AUmbraPlayerState* Owner = World->SpawnActor<AUmbraPlayerState>();
	UUmbraItemDefinition* Item = NewObject<UUmbraItemDefinition>(Owner);
	UUmbraItemGenerationProfile* Profile = NewObject<UUmbraItemGenerationProfile>(Owner);
	FUmbraGenerationContext Context;
	FFixture()
	{
		Item->DefinitionId = TEXT("Test.GeneratedRing"); Item->ItemCategory = EUmbraItemCategory::Accessory;
		Item->AllowedSlots = {EUmbraEquipmentSlot::Ring1}; Item->GenerationProfile = Profile;
		Profile->Pool = NewObject<UUmbraAffixPool>(Owner);
		FUmbraGenerationRarity R; R.Rarity = EUmbraItemRarity::Rare; R.AffixCount = 2; R.Color = FLinearColor::Yellow;
		Profile->Rarities.Add(R); Context.SourceLevel = 10;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			auto* Affix = NewObject<UUmbraAffixDefinition>(Owner);
			Affix->AffixId = Index == 0 ? TEXT("Strength") : TEXT("Armor");
			for (int32 Level : {1, 10})
			{
				FUmbraAffixTier Tier; Tier.TierId = Level == 1 ? TEXT("T1") : TEXT("T2"); Tier.MinimumLevel = Level;
				Tier.Values.Kind = Index == 0 ? EUmbraAffixKind::Primary : EUmbraAffixKind::Attribute;
				Tier.Values.Attribute = EUmbraEquipmentAffixStat::Armor;
				Tier.Values.MinValue = Level; Tier.Values.MaxValue = Level + 10; Tier.Values.bIntegerValues = true;
				Affix->Tiers.Add(Tier);
			}
			FUmbraAffixPoolEntry Entry; Entry.Affix = Affix; Profile->Pool->Entries.Add(Entry);
		}
	}
	~FFixture() { World->DestroyWorld(false); }
	bool Generate(FUmbraItemInstance& Out, FString& Error, int32 Seed = 12345) { return UmbraItemGeneration::Generate(Owner, Item, Context, Seed, FGuid(1,2,3,4), Out, Error); }
};
bool Same(const FUmbraItemInstance& A, const FUmbraItemInstance& B) { return FUmbraItemInstance::StaticStruct()->CompareScriptStruct(&A, &B, 0); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraGenerationRulesTest, "Umbra.Items.Generation.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraGenerationRulesTest::RunTest(const FString& Parameters)
{
	using namespace UmbraGenerationTests;
	FFixture F; FString Error; FUmbraItemInstance A, B;
	if (!TestTrue(TEXT("Valid generation"), F.Generate(A, Error))) { AddError(Error); return false; }
	TestEqual(TEXT("Level metadata"), A.ItemLevel, 10); TestEqual(TEXT("Rarity metadata"), A.Rarity, EUmbraItemRarity::Rare);
	TestEqual(TEXT("Count"), A.RolledAffixes.Num(), 2);
	for (const auto& Roll : A.RolledAffixes) TestEqual(TEXT("Highest eligible content tier"), Roll.AffixTierId, FName(TEXT("T2")));
	for (int32 I = 0; I < 100; ++I) (void)FMath::Rand();
	Swap(F.Profile->Pool->Entries[0], F.Profile->Pool->Entries[1]);
	TestTrue(TEXT("Global RNG and pool order cannot affect payload"), F.Generate(B, Error) && Same(A, B));
	F.Context.SourceLevel = 1; TestTrue(TEXT("Level boundary"), F.Generate(B, Error));
	for (const auto& Roll : B.RolledAffixes) TestEqual(TEXT("Low tier"), Roll.AffixTierId, FName(TEXT("T1")));
	F.Context.SourceLevel = MAX_int32; TestTrue(TEXT("Level clamp"), F.Generate(B, Error) && B.ItemLevel == 100);
	F.Context.SourceLevel = 0; TestFalse(TEXT("Missing source rejected"), F.Generate(B, Error)); TestFalse(TEXT("No partial result"), B.IsValid());
	F.Context.SourceLevel = 10;
	auto* Affix = F.Profile->Pool->Entries[0].Affix.Get();
	Affix->Slots = {EUmbraEquipmentSlot::Head}; TestFalse(TEXT("Slot filtering shortage"), F.Generate(B, Error)); Affix->Slots.Reset();
	Affix->Categories = {EUmbraItemCategory::Weapon}; TestFalse(TEXT("Category filtering"), F.Generate(B, Error)); Affix->Categories.Reset();
	Affix->Rarities = {EUmbraItemRarity::Common}; TestFalse(TEXT("Rarity filtering"), F.Generate(B, Error)); Affix->Rarities.Reset();
	Affix->WeaponTypes = {EUmbraWeaponType::Bow}; TestFalse(TEXT("Weapon filtering"), F.Generate(B, Error)); Affix->WeaponTypes.Reset();
	const auto Tag = FGameplayTag::RequestGameplayTag(FName(TEXT("State.Dead")), false);
	TestTrue(TEXT("Tag fixture registered"), Tag.IsValid());
	if (Tag.IsValid())
	{
		Affix->RequiredSourceTags.AddTag(Tag); TestFalse(TEXT("Source tag filtering"), F.Generate(B, Error));
		F.Context.SourceTags.AddTag(Tag); TestTrue(TEXT("Source tag accepted"), F.Generate(B, Error)); Affix->RequiredSourceTags.Reset();
		Affix->RequiredEquipmentTags.AddTag(Tag); TestFalse(TEXT("Equipment tag filtering"), F.Generate(B, Error));
		F.Item->EquipmentTags.AddTag(Tag); TestTrue(TEXT("Equipment tag accepted"), F.Generate(B, Error));
		Affix->BlockedEquipmentTags.AddTag(Tag); TestFalse(TEXT("Blocked tag"), F.Generate(B, Error));
		Affix->RequiredEquipmentTags.Reset(); Affix->BlockedEquipmentTags.Reset();
	}
	F.Profile->Pool->Entries[0].Weight = 0; TestFalse(TEXT("Zero weight excluded"), F.Generate(B, Error));
	F.Profile->Pool->Entries[0].Weight = std::numeric_limits<float>::quiet_NaN(); TestFalse(TEXT("NaN rejected"), F.Generate(B, Error));
	F.Profile->Pool->Entries[0].Weight = 1;
	for (auto& Entry : F.Profile->Pool->Entries) Entry.Affix->ExclusiveGroup = TEXT("Shared");
	TestFalse(TEXT("Exclusive group shortage"), F.Generate(B, Error));
	for (auto& Entry : F.Profile->Pool->Entries) Entry.Affix->ExclusiveGroup = NAME_None;
	const auto Duplicate = F.Profile->Pool->Entries[0]; F.Profile->Pool->Entries.Add(Duplicate); TestFalse(TEXT("Duplicate ID rejected"), F.Generate(B, Error)); F.Profile->Pool->Entries.Pop();
	F.Item->PrimaryBonuses.Strength = 1; TestFalse(TEXT("Fixed semantic collision"), F.Generate(B, Error)); F.Item->PrimaryBonuses.Strength = 0;
	F.Item->AffixRollConfigs.AddDefaulted(); TestFalse(TEXT("Legacy and pool cannot mix"), F.Generate(B, Error)); F.Item->AffixRollConfigs.Reset();
	F.Profile->Rarities[0].Weight = 0; TestFalse(TEXT("All zero rarity"), F.Generate(B, Error)); F.Profile->Rarities[0].Weight = 1;
	TestFalse(TEXT("Legacy creator cannot bypass generator"), FUmbraItemInstance::Create(F.Owner, F.Item, FGuid::NewGuid(), B));
	B.Definition = F.Item; B.InstanceId = FGuid::NewGuid(); TestFalse(TEXT("Legacy payload cannot bypass profile"), B.IsValid());
	FUmbraGenerationRarity Common; Common.AffixCount = 0; F.Profile->Rarities.Add(Common);
	int32 RareCount = 0; TSet<float> Values;
	for (int32 Seed = 0; Seed < 512; ++Seed) { if (!F.Generate(B, Error, Seed)) { AddError(Error); break; } if (B.Rarity == EUmbraItemRarity::Rare) { ++RareCount; Values.Add(B.RolledAffixes[0].Value); } }
	TestTrue(TEXT("Equal weights broad distribution"), RareCount > 180 && RareCount < 330); TestTrue(TEXT("Different rolls"), Values.Num() > 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraGenerationLifecycleTest, "Umbra.Items.Generation.Lifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraGenerationLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace UmbraGenerationTests;
	FFixture F; FString Error; FUmbraItemInstance A, B;
	if (!TestTrue(TEXT("Generated instance"), F.Generate(A, Error))) return false;
	FUmbraItemInstanceRecord Record, Read;
	TestTrue(TEXT("Capture DTO"), FUmbraItemInstanceRecord::Capture(A, Record));
	TArray<uint8> Bytes;
	{ FMemoryWriter Writer(Bytes); FObjectAndNameAsStringProxyArchive Archive(Writer, false); FUmbraItemInstanceRecord::StaticStruct()->SerializeItem(Archive, &Record, nullptr); }
	{ FMemoryReader Reader(Bytes); FObjectAndNameAsStringProxyArchive Archive(Reader, true); FUmbraItemInstanceRecord::StaticStruct()->SerializeItem(Archive, &Read, nullptr); }
	TestTrue(TEXT("DTO memory round trip"), Read.Restore(F.Item, B) && Same(A,B));
	Read.SchemaVersion = 99; TestFalse(TEXT("Future schema rejected"), Read.Restore(F.Item,B)); Read.SchemaVersion = 1;
	Read.DefinitionId = TEXT("Wrong"); TestFalse(TEXT("Wrong resolver rejected"), Read.Restore(F.Item,B));
	auto* Legacy = NewObject<UUmbraItemDefinition>(F.Owner); Legacy->ItemLevel = 7; Legacy->Rarity = EUmbraItemRarity::Magic;
	B.Definition = Legacy; B.InstanceId = FGuid::NewGuid(); B.RolledAffixes = A.RolledAffixes;
	TestTrue(TEXT("Legacy upgrade"), B.UpgradeLegacy()); TestEqual(TEXT("Legacy level"), B.ItemLevel, 7);
	TestTrue(TEXT("Legacy rolls preserved"), FUmbraRolledAffix::StaticStruct()->CompareScriptStruct(&A.RolledAffixes[0], &B.RolledAffixes[0], 0));
	auto* ASC = F.Owner->GetUmbraAbilitySystemComponent(); ASC->InitializeComponent(); ASC->InitAbilityActorInfo(F.Owner,F.Owner);
	auto* Equipment = F.Owner->FindComponentByClass<UUmbraEquipmentComponent>(); Equipment->bEnableEquipment = true;
	F.Owner->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower = true; F.Owner->InitializeAttributes();
	auto* Controller = F.World->SpawnActor<AUmbraLootTestController>(); Controller->SetPlayerState(F.Owner);
	auto* Pawn = F.World->SpawnActor<AUmbraPlayerCharacter>(); Controller->Possess(Pawn); Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	auto* Drop = F.World->SpawnActor<AUmbraWorldItem>(); Drop->SetActorLocation(Pawn->GetActorLocation());
	TestTrue(TEXT("World initialize"), Drop->InitializeInstance(A));
	TestEqual(TEXT("Real pickup"), Drop->TryPickup(Controller), EUmbraPickupResult::Success);
	TestEqual(TEXT("Second pickup"), Drop->TryPickup(Controller), EUmbraPickupResult::Unavailable);
	auto* Inventory = F.Owner->GetInventoryComponent();
	if (!TestEqual(TEXT("One carried item"), Inventory->GetSnapshot().Items.Num(),1)) return false;
	TestTrue(TEXT("World to bag exact"), Same(A, Inventory->GetSnapshot().Items[0].GetInstance()));
	B = A; ++B.ItemLevel; TestEqual(TEXT("Known GUID metadata tamper"), Inventory->AddInstance(B), EUmbraInventoryResult::InvalidIdentity);
	const float Strength = ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute());
	const float Armor = ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute());
	Equipment->CharacterLevel = A.GetItemLevel(); // Lifecycle test requires a character eligible for the generated level.
	TestEqual(TEXT("Equip"), F.Owner->EquipFromInventory(EUmbraEquipmentSlot::Ring1,A.InstanceId), EUmbraTransferResult::Success);
	if (!TestEqual(TEXT("One equipped item"), Equipment->GetSnapshot().Items.Num(),1)) return false;
	TestTrue(TEXT("Equipped metadata exact"), Same(A,Equipment->GetSnapshot().Items[0].GetInstance()));
	for (const auto& Roll : A.RolledAffixes) TestEqual(TEXT("GAS applies actual selected value"), ASC->GetNumericAttribute(Roll.Kind == EUmbraAffixKind::Primary ? UUmbraAttributeSet::GetStrengthAttribute() : UUmbraAttributeSet::GetArmorAttribute()), (Roll.Kind == EUmbraAffixKind::Primary ? Strength : Armor) + Roll.Value);
	FUmbraInventoryItem Returned;
	TestEqual(TEXT("Unequip to chosen slot"),F.Owner->UnequipToInventorySlot(EUmbraEquipmentSlot::Ring1,A.InstanceId,12,Returned),EUmbraTransferResult::Success);
	TestTrue(TEXT("Returned exact metadata"),Same(A,Returned.GetInstance()));
	TestEqual(TEXT("Strength withdrawn"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), Strength);
	TestEqual(TEXT("Armor withdrawn"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), Armor);
	Inventory->UnregisterComponent(); Inventory->RegisterComponent(); TestTrue(TEXT("Re-register exact"),Same(A,Inventory->GetSnapshot().Items[0].GetInstance()));
	const auto Display = FUmbraEquipmentItemDisplay::FromInstance(A);
	TestEqual(TEXT("Audio uses instance rarity"),Display.AudioContext.Rarity,A.Rarity); TestEqual(TEXT("Frame color"),Display.RarityColor,FLinearColor::Yellow);
	const auto Tooltip = UUmbraItemTooltipDataBuilder::FromInventory(Inventory->GetSnapshot(),12,A.InstanceId,F.Item,nullptr,false,EUmbraEquipmentSlot::Ring1);
	TestEqual(TEXT("Tooltip level"),Tooltip.ItemLevel,A.ItemLevel); TestEqual(TEXT("Tooltip rarity"),Tooltip.Rarity,A.Rarity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraDamagePoolTest, "Umbra.Items.Generation.DamageSnapshot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraDamagePoolTest::RunTest(const FString& Parameters)
{
	using namespace UmbraGenerationTests;
	FFixture F; FString Error; FUmbraItemInstance Instance;
	for (int32 I=0; I<2; ++I) for (auto& Tier : F.Profile->Pool->Entries[I].Affix->Tiers)
	{
		Tier.Values.Kind = EUmbraAffixKind::Damage;
		Tier.Values.DamageBonus.Bucket = I == 0 ? EUmbraDamageBucket::Additive : EUmbraDamageBucket::Multiplicative;
		Tier.Values.DamageBonus.Types = {EUmbraWeaponDamageType::Slashing};
		Tier.Values.DamageBonus.AttackSource = EUmbraBonusAttackSource::BasicAttack;
		Tier.Values.bIntegerValues = false;
		Tier.Values.MinValue = Tier.Values.MaxValue = I == 0 ? .2f : 1.5f;
	}
	if (!TestTrue(TEXT("Pool damage generates"), F.Generate(Instance,Error))) { AddError(Error); return false; }
	auto* ASC = F.Owner->GetUmbraAbilitySystemComponent(); ASC->InitializeComponent(); ASC->InitAbilityActorInfo(F.Owner,F.Owner);
	F.Owner->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower = true;
	auto* Equipment = F.Owner->GetEquipmentComponent(); Equipment->bEnableEquipment = true; F.Owner->InitializeAttributes();
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(),0);
	TestEqual(TEXT("Import"),F.Owner->AcceptLootInstance(Instance),EUmbraInventoryResult::Success);
	Equipment->CharacterLevel = Instance.GetItemLevel(); // Exercise GAS snapshot after satisfying the level gate.
	TestEqual(TEXT("Equip public pool damage"),F.Owner->EquipFromInventory(EUmbraEquipmentSlot::Ring1,Instance.InstanceId),EUmbraTransferResult::Success);
	const FUmbraEquipmentEffectContext* Snapshot = nullptr;
	int32 EquipmentHandles = 0;
	for (const auto Handle : ASC->GetActiveEffects(FGameplayEffectQuery()))
	{
		const auto* Effect = ASC->GetActiveGameplayEffect(Handle);
		if (Effect->Spec.Def->IsA<UUmbraEquipmentEffect>()) { ++EquipmentHandles; Snapshot = FUmbraEquipmentEffectContext::Get(Effect->Spec.GetContext()); }
	}
	TestEqual(TEXT("One equipment handle"),EquipmentHandles,1);
	if (!TestNotNull(TEXT("Typed replicated context"),Snapshot)) return false;
	FUmbraEquipmentEffectContext Send, Receive; Send.DamageBonuses = Snapshot->DamageBonuses;
	const auto Tag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"));
	Send.DamageBonuses[0].TargetRequirements.IgnoreTags.AddTag(Tag);
	Send.DamageBonuses[0].SourceRequirements.TagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(Tag));
	FNetBitWriter Writer(nullptr, 1024*1024); bool Success = false;
	TestTrue(TEXT("Net serializer write"),Send.NetSerialize(Writer,nullptr,Success) && Success);
	FNetBitReader Reader(nullptr,Writer.GetData(),Writer.GetNumBits());
	TestTrue(TEXT("Net serializer read"),Receive.NetSerialize(Reader,nullptr,Success) && Success);
	TestEqual(TEXT("Net array count"),Receive.DamageBonuses.Num(),Send.DamageBonuses.Num());
	for (int32 I=0; I<Receive.DamageBonuses.Num(); ++I) TestTrue(TEXT("Network payload includes queries and filters"),FUmbraDamageBonus::StaticStruct()->CompareScriptStruct(&Send.DamageBonuses[I],&Receive.DamageBonuses[I],0));
	TUniquePtr<FUmbraEquipmentEffectContext> Copy(Send.Duplicate()); Copy->DamageBonuses[0].Types.Reset();
	TestFalse(TEXT("Context duplicate owns array"),Send.DamageBonuses[0].Types.IsEmpty());
	// Changing the pool after creation must not change the existing instance or active GE.
	for (auto& Entry : F.Profile->Pool->Entries) for (auto& Tier : Entry.Affix->Tiers) { Tier.Values.DamageBonus.Types={EUmbraWeaponDamageType::Fire}; Tier.Values.MinValue=Tier.Values.MaxValue=99; }
	auto* TargetOwner=F.World->SpawnActor<AUmbraPlayerState>(); auto* Target=TargetOwner->GetUmbraAbilitySystemComponent();
	Target->InitializeComponent(); Target->InitAbilityActorInfo(TargetOwner,TargetOwner); TargetOwner->InitializeAttributes();
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetMaxHealthAttribute(),10000);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetArmorAttribute(),0);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetSlashingResistanceAttribute(),0);
	auto Hit = [&](EUmbraDamageSource Source, float Expected)
	{
		FUmbraDamageRequest Request(Source); Request.Config.Typed.Model=EUmbraDamageModel::ExplicitChannels; Request.Config.Typed.bUseDamageBuckets=true;
		auto& Channel=Request.Config.Typed.Channels.AddDefaulted_GetRef(); Channel.Type=EUmbraWeaponDamageType::Slashing; Channel.BaseDamage=100;
		Target->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(),10000);
		TestTrue(TEXT("Real hit"),UmbraDamage::Apply(ASC,Target,Request));
		TestTrue(TEXT("Actual health loss"),FMath::IsNearlyEqual(10000-Target->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()),Expected,.01f));
	};
	Hit(EUmbraDamageSource::BasicAttack,180); Hit(EUmbraDamageSource::Skill,100);
	FUmbraInventoryItem Returned;
	TestEqual(TEXT("Withdraw"),F.Owner->UnequipToInventory(EUmbraEquipmentSlot::Ring1,Instance.InstanceId,Returned),EUmbraTransferResult::Success);
	Hit(EUmbraDamageSource::BasicAttack,100);
	TestEqual(TEXT("Re-equip retains saved conditions"),F.Owner->EquipFromInventory(EUmbraEquipmentSlot::Ring1,Instance.InstanceId),EUmbraTransferResult::Success);
	Hit(EUmbraDamageSource::BasicAttack,180);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraTransferProtocolTest, "Umbra.Inventory.RequestProtocol", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraTransferProtocolTest::RunTest(const FString& Parameters)
{
	using namespace UmbraGenerationTests;
	FFixture F; FString Error; FUmbraItemInstance Item; F.Generate(Item,Error);
	auto* Controller=F.World->SpawnActor<AUmbraLootTestController>(); Controller->SetPlayerState(F.Owner); F.Owner->SetOwner(Controller);
	auto* ASC=F.Owner->GetUmbraAbilitySystemComponent(); ASC->InitializeComponent(); ASC->InitAbilityActorInfo(F.Owner,F.Owner);
	F.Owner->GetEquipmentComponent()->bEnableEquipment=true; F.Owner->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower=true; F.Owner->InitializeAttributes();
	// This test exercises request revisions, not the separate level requirement rejection.
	F.Owner->GetEquipmentComponent()->CharacterLevel=Item.GetItemLevel();
	F.Owner->AcceptLootInstance(Item);
	FUmbraItemTransferRequest Request; Request.InstanceId=Item.InstanceId; Request.EquipmentSlot=EUmbraEquipmentSlot::Ring1;
	auto Refresh=[&]() { ++Request.RequestId; Request.InventoryRevision=F.Owner->GetInventoryComponent()->GetSnapshot().Revision; Request.EquipmentRevision=F.Owner->GetEquipmentComponent()->GetSnapshot().Revision; };
	Refresh();
	TestEqual(TEXT("Own item accepted"),Controller->ProcessItemTransfer(Request).Result,EUmbraTransferResult::Success);
	TestEqual(TEXT("Repeated request cannot apply twice"),Controller->ProcessItemTransfer(Request).Result,EUmbraTransferResult::StaleRequest);
	++Request.RequestId; Request.Operation=EUmbraItemTransferOperation::Unequip;
	TestEqual(TEXT("Late view rejected"),Controller->ProcessItemTransfer(Request).Result,EUmbraTransferResult::StaleRequest);
	Refresh(); Request.InstanceId=FGuid::NewGuid();
	TestEqual(TEXT("Foreign or unknown identity rejected"),Controller->ProcessItemTransfer(Request).Result,EUmbraTransferResult::NotFound);
	Request.InstanceId=Item.InstanceId; Refresh();
	TestEqual(TEXT("Unequip accepted"),Controller->ProcessItemTransfer(Request).Result,EUmbraTransferResult::Success);
	Refresh(); Request.Operation=EUmbraItemTransferOperation::Move; Request.TargetCell=23;
	TestEqual(TEXT("Move accepted"),Controller->ProcessItemTransfer(Request).Result,EUmbraTransferResult::Success);
	Refresh(); Request.TargetCell=999;
	TestEqual(TEXT("Invalid target refused"),Controller->ProcessItemTransfer(Request).Result,EUmbraTransferResult::InvalidSlot);
	Refresh(); F.Owner->SetOwner(nullptr);
	TestEqual(TEXT("Controller cannot operate unowned state"),Controller->ProcessItemTransfer(Request).Result,EUmbraTransferResult::NotReady);
	TestTrue(TEXT("No payload lost"),Same(Item,F.Owner->GetInventoryComponent()->GetSnapshot().Items[0].GetInstance()));
	return true;
}
#endif

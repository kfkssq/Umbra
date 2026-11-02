#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Items/UmbraItemInstance.h"
#include "Items/UmbraItemTooltipData.h"
#include "Inventory/UmbraInventoryComponent.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Equipment/UmbraEquipmentEffect.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Player/UmbraPlayerState.h"
#include "Loot/UmbraWorldItem.h"
#include "Engine/World.h"
#include <limits>

namespace UmbraRandomBaseline
{
struct FFixture
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AUmbraPlayerState* Owner = World->SpawnActor<AUmbraPlayerState>();
	UUmbraAbilitySystemComponent* ASC = Owner->GetUmbraAbilitySystemComponent();
	UUmbraInventoryComponent* Inventory = Owner->GetInventoryComponent();
	UUmbraEquipmentComponent* Equipment = Owner->FindComponentByClass<UUmbraEquipmentComponent>();
	FFixture()
	{
		ASC->InitializeComponent();
		ASC->InitAbilityActorInfo(Owner, Owner);
		Owner->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower = true;
		Equipment->bEnableEquipment = true;
		Owner->InitializeAttributes();
	}
	~FFixture() { World->DestroyWorld(false); }
};

FUmbraAffixRollConfig Config(FName Id, EUmbraAffixKind Kind, float Min, float Max)
{
	FUmbraAffixRollConfig Result;
	Result.AffixId = Id; Result.Kind = Kind;
	Result.MinValue = Min; Result.MaxValue = Max;
	return Result;
}

UUmbraItemDefinition* MakeRing(UObject* Outer)
{
	auto* Item = NewObject<UUmbraItemDefinition>(Outer);
	Item->AllowedSlots = {EUmbraEquipmentSlot::Ring1, EUmbraEquipmentSlot::Ring2};
	Item->Armor = 100.f;
	Item->Requirements.Strength = 1000.f;
	Item->AffixRollConfigs.Add(Config(TEXT("Strength"), EUmbraAffixKind::Primary, 10, 20));
	auto Armor = Config(TEXT("Armor"), EUmbraAffixKind::Attribute, 20, 40);
	Armor.Attribute = EUmbraEquipmentAffixStat::Armor;
	Item->AffixRollConfigs.Add(Armor);
	auto Health = Config(TEXT("Health"), EUmbraAffixKind::Attribute, 50, 100);
	Health.Attribute = EUmbraEquipmentAffixStat::MaxHealth;
	Item->AffixRollConfigs.Add(Health);
	auto A = Config(TEXT("Additive"), EUmbraAffixKind::Damage, .1f, .3f);
	A.DamageBonus.Types = {EUmbraWeaponDamageType::Slashing};
	Item->AffixRollConfigs.Add(A);
	auto X = Config(TEXT("Multiplicative"), EUmbraAffixKind::Damage, 1.2f, 1.5f);
	X.DamageBonus.Bucket = EUmbraDamageBucket::Multiplicative;
	X.DamageBonus.AttackSource = EUmbraBonusAttackSource::BasicAttack;
	Item->AffixRollConfigs.Add(X);
	return Item;
}

bool Same(const FUmbraItemInstance& A, const FUmbraItemInstance& B)
{
	// Reflection compares every serialized field, including conditions and candidate arrays.
	return FUmbraItemInstance::StaticStruct()->CompareScriptStruct(&A, &B, 0);
}

FUmbraItemInstance Stored(UUmbraInventoryComponent* Inventory, FGuid Id)
{
	const auto Snapshot = Inventory->GetSnapshot();
	for (const auto& Item : Snapshot.Items) if (Item.InstanceId == Id) return Item.GetInstance();
	return {};
}

FUmbraItemInstance Worn(UUmbraEquipmentComponent* Equipment, FGuid Id)
{
	const auto Snapshot = Equipment->GetSnapshot();
	for (const auto& Item : Snapshot.Items) if (Item.InstanceId == Id) return Item.GetInstance();
	return {};
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraRandomCreationBaseline, "Umbra.Items.RandomBaseline.Creation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraRandomCreationBaseline::RunTest(const FString& Parameters)
{
	using namespace UmbraRandomBaseline;
	FFixture F;
	auto C = Config(TEXT("Candidate"), EUmbraAffixKind::Primary, 10, 20);
	TArray<float> Values;
	TestTrue(TEXT("Six uniform candidates"), C.BuildCandidates(Values));
	TestTrue(TEXT("Uniform endpoints and native units"), Values == TArray<float>({10,12,14,16,18,20}));
	C.TierCount = 5;
	TestTrue(TEXT("Five candidates accepted"), C.BuildCandidates(Values) && Values.Num() == 5);
	C.ExplicitValues = {11, 17, 19};
	TestTrue(TEXT("Explicit candidates override count"), C.BuildCandidates(Values) && Values == C.ExplicitValues);
	C.ExplicitValues = {11, 11};
	TestFalse(TEXT("Duplicate explicit values rejected"), C.BuildCandidates(Values));
	C.ExplicitValues = {20, 10};
	TestFalse(TEXT("Descending explicit values rejected"), C.BuildCandidates(Values));
	C.ExplicitValues = {9, 20};
	TestFalse(TEXT("Out of range explicit values rejected"), C.BuildCandidates(Values));
	C.ExplicitValues.Reset(); C.MinValue = 0; C.MaxValue = 1; C.bIntegerValues = true;
	TestFalse(TEXT("Rounding collisions rejected"), C.BuildCandidates(Values));
	C.MinValue = C.MaxValue = 7;
	TestTrue(TEXT("Constant range yields one candidate"), C.BuildCandidates(Values) && Values == TArray<float>({7}));
	C.MaxValue = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("Nonfinite rejected"), C.BuildCandidates(Values));
	C.MaxValue = 6;
	TestFalse(TEXT("Inverted range rejected"), C.BuildCandidates(Values));
	C.MinValue = -1;
	TestFalse(TEXT("Negative value rejected"), C.BuildCandidates(Values));
	C.MinValue = 0; C.TierCount = 4;
	TestFalse(TEXT("Unsupported count rejected"), C.BuildCandidates(Values));

	auto* Item = MakeRing(F.Owner);
	FUmbraItemInstance Instance;
	const FGuid Id = FGuid::NewGuid();
	if (!TestTrue(TEXT("Authority creates complete instance"), FUmbraItemInstance::Create(F.Owner, Item, Id, Instance))) return false;
	TestTrue(TEXT("Created payload valid"), Instance.IsValid());
	TestEqual(TEXT("Caller identity preserved"), Instance.InstanceId, Id);
	TestEqual(TEXT("All authored affixes rolled"), Instance.RolledAffixes.Num(), Item->AffixRollConfigs.Num());
	for (const auto& Roll : Instance.RolledAffixes)
	{
		TestTrue(TEXT("Value is selected candidate"), Roll.CandidateValues.IsValidIndex(Roll.SelectedTier)
			&& Roll.Value == Roll.CandidateValues[Roll.SelectedTier]);
		if (Roll.Kind == EUmbraAffixKind::Damage)
			TestEqual(TEXT("Damage magnitude is rolled value"), Roll.DamageBonus.Magnitude.GetValueAtLevel(1), Roll.Value);
	}
	FUmbraItemInstance Out = Instance;
	TestFalse(TEXT("Null authority rejected"), FUmbraItemInstance::Create(nullptr, Item, Id, Out));
	TestFalse(TEXT("Failure clears output"), Out.IsValid());
	auto* Client = F.World->SpawnActor<AActor>();
	Client->SetRole(ROLE_SimulatedProxy);
	TestFalse(TEXT("Non-authority actor rejected"), FUmbraItemInstance::Create(Client, Item, Id, Out));
	TestFalse(TEXT("Missing GUID rejected"), FUmbraItemInstance::Create(F.Owner, Item, FGuid(), Out));
	TestFalse(TEXT("Missing definition rejected"), FUmbraItemInstance::Create(F.Owner, nullptr, Id, Out));
	const auto DuplicateConfig = Item->AffixRollConfigs[0];
	Item->AffixRollConfigs.Add(DuplicateConfig);
	AddExpectedError(TEXT("invalid or duplicate affix"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Duplicate ID creation rejected atomically"), FUmbraItemInstance::Create(F.Owner, Item, Id, Out));
	TestTrue(TEXT("No partial payload published"), !Out.InstanceId.IsValid() && Out.RolledAffixes.IsEmpty());
	Item->AffixRollConfigs.Pop();
	auto Bad = Instance;
	Bad.RolledAffixes[0].Value += .125f;
	TestFalse(TEXT("Value inconsistent with candidate rejected"), Bad.IsValid());
	Bad = Instance; Bad.RolledAffixes[0].SelectedTier = INDEX_NONE;
	TestFalse(TEXT("Invalid selected index rejected"), Bad.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraRandomLifecycleBaseline, "Umbra.Items.RandomBaseline.TransferAndEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraRandomLifecycleBaseline::RunTest(const FString& Parameters)
{
	using namespace UmbraRandomBaseline;
	FFixture F;
	auto* Item = MakeRing(F.Owner);
	FUmbraItemInstance Instance;
	if (!TestTrue(TEXT("Create ring"), FUmbraItemInstance::Create(F.Owner, Item, FGuid::NewGuid(), Instance))) return false;
	const auto Id = Instance.InstanceId;
	const auto Slot = EUmbraEquipmentSlot::Ring1;
	const float Strength = F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute());
	const float HealthMax = F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetMaxHealthAttribute());
	const float Health = F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute());
	const float Armor = F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute());
	const int32 InitialEffects = F.ASC->GetActiveEffects(FGameplayEffectQuery()).Num();
	auto* Drop = F.World->SpawnActor<AUmbraWorldItem>();
	TestTrue(TEXT("World accepts complete payload"), Drop->InitializeInstance(Instance));
	TestFalse(TEXT("World rejects second initialization"), Drop->InitializeInstance(Instance));
	TestTrue(TEXT("World preserves payload"), Same(Instance, Drop->GetItemInstance()));
	TestEqual(TEXT("Accept world instance through PlayerState"), F.Owner->AcceptLootInstance(Drop->GetItemInstance()), EUmbraInventoryResult::Success);
	Drop->Destroy();
	TestTrue(TEXT("Inventory payload exact"), Same(Instance, Stored(F.Inventory, Id)));
	TestEqual(TEXT("Duplicate import rejected"), F.Inventory->AddInstance(Instance), EUmbraInventoryResult::DuplicateInstance);
	auto Tampered = Instance;
	auto& Roll = Tampered.RolledAffixes[0];
	Roll.SelectedTier = (Roll.SelectedTier + 1) % Roll.CandidateValues.Num();
	Roll.Value = Roll.CandidateValues[Roll.SelectedTier];
	TestTrue(TEXT("Alternative roll is structurally valid"), Tampered.IsValid());
	TestEqual(TEXT("Known GUID cannot change valid roll"), F.Inventory->AddInstance(Tampered), EUmbraInventoryResult::InvalidIdentity);
	TestEqual(TEXT("Move retains instance"), F.Owner->MoveInventoryItem(Id, 12), EUmbraInventoryResult::Success);
	const auto Preview = UUmbraItemTooltipDataBuilder::FromInventory(F.Inventory->GetSnapshot(), 12, Id, Item, nullptr, false, Slot);
	TestTrue(TEXT("Tooltip accepts random instance"), Preview.bValid);
	for (const auto& Affix : Instance.RolledAffixes)
	{
		const auto& Lines = Affix.Kind == EUmbraAffixKind::Damage && Affix.DamageBonus.Bucket == EUmbraDamageBucket::Multiplicative
			? Preview.SpecialEffectStats : Preview.Stats;
		const auto* Line = Lines.FindByPredicate([&](const auto& Entry) { return Entry.AffixId == Affix.AffixId; });
		TestTrue(TEXT("Tooltip exposes saved value and candidates"), Line && Line->bRolled && Line->RawValue == Affix.Value
			&& Line->CandidateValues == Affix.CandidateValues && Line->SelectedTier == Affix.SelectedTier);
	}
	TestTrue(TEXT("Presentation did not reroll"), Same(Instance, Stored(F.Inventory, Id)));
	TestEqual(TEXT("Wrong slot preserves inventory"), F.Owner->EquipFromInventory(EUmbraEquipmentSlot::Head, Id), EUmbraTransferResult::InvalidSlot);
	Item->RequiredLevel = F.Equipment->CharacterLevel + 1;
	TestEqual(TEXT("Level rejection preserves inventory"), F.Owner->EquipFromInventory(Slot, Id), EUmbraTransferResult::LevelTooLow);
	Item->RequiredLevel = 1;
	TestTrue(TEXT("Rejected transfers preserve all rolls"), Same(Instance, Stored(F.Inventory, Id)));

	FGameplayEffectApplicationQuery Reject;
	Reject.BindLambda([](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec& Spec) { return !Spec.Def->IsA<UUmbraEquipmentEffect>(); });
	const auto RejectHandle = Reject.GetHandle();
	F.ASC->GameplayEffectApplicationQueries.Add(Reject);
	TestEqual(TEXT("Rejected GE preserves inventory"), F.Owner->EquipFromInventory(Slot, Id), EUmbraTransferResult::EffectRejected);
	TestTrue(TEXT("Rejected GE leaves exact payload"), Same(Instance, Stored(F.Inventory, Id)));
	TestEqual(TEXT("Rejected GE leaves no effect"), F.ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), InitialEffects);
	F.ASC->GameplayEffectApplicationQueries.RemoveAll([&](const auto& Query) { return Query.GetHandle() == RejectHandle; });

	// Force a failure after equip, before inventory consumption, without changing production code.
	bool bInterruptTransfer = true;
	const auto Interruption = F.ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetStrengthAttribute()).AddLambda(
		[&](const FOnAttributeChangeData&) { if (bInterruptTransfer) { bInterruptTransfer = false; F.Inventory->UnregisterComponent(); } });
	TestEqual(TEXT("Interrupted consume triggers rollback"), F.Owner->EquipFromInventory(Slot, Id), EUmbraTransferResult::TransferFailed);
	F.ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetStrengthAttribute()).Remove(Interruption);
	F.Inventory->RegisterComponent();
	TestTrue(TEXT("Rollback preserves complete inventory instance"), Same(Instance, Stored(F.Inventory, Id)));
	TestFalse(TEXT("Rollback removes equipped copy"), Worn(F.Equipment, Id).IsValid());
	TestEqual(TEXT("Rollback removes GE"), F.ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), InitialEffects);
	TestEqual(TEXT("Rollback restores strength"), F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), Strength);

	TestEqual(TEXT("Equip complete instance"), F.Owner->EquipFromInventory(Slot, Id), EUmbraTransferResult::Success);
	TestTrue(TEXT("Equipment payload exact"), Same(Instance, Worn(F.Equipment, Id)));
	TestFalse(TEXT("Inventory consumed"), Stored(F.Inventory, Id).IsValid());
	TestEqual(TEXT("Random primary applied"), F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), Strength + Instance.RolledAffixes[0].Value);
	TestEqual(TEXT("Intrinsic penalty excludes random armor"), F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), Armor + 50 + Instance.RolledAffixes[1].Value);
	TestEqual(TEXT("Random health applied"), F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetMaxHealthAttribute()), HealthMax + Instance.RolledAffixes[2].Value);
	TestEqual(TEXT("Equip does not heal"), F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), Health);
	TestFalse(TEXT("Own random strength cannot satisfy requirements"), F.Equipment->GetSnapshot().Items[0].bRequirementsMet);
	TestEqual(TEXT("One GE for all rolls"), F.ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), InitialEffects + 1);
	auto* Filler = NewObject<UUmbraItemDefinition>(F.Owner);
	TArray<FGuid> FillerIds;
	const int32 Capacity = F.Inventory->GetSnapshot().Capacity;
	for (int32 Index = 0; Index < Capacity; ++Index)
	{
		const auto FillerId = FGuid::NewGuid();
		if (F.Inventory->AddItem(Filler, FillerId) == EUmbraInventoryResult::Success) FillerIds.Add(FillerId);
	}
	FUmbraInventoryItem FullReturn;
	TestEqual(TEXT("Full bag refuses random item return"), F.Owner->UnequipToInventory(Slot, Id, FullReturn), EUmbraTransferResult::InventoryFull);
	TestTrue(TEXT("Full bag keeps complete equipped payload"), Same(Instance, Worn(F.Equipment, Id)));
	TestEqual(TEXT("Full bag keeps exactly one GE"), F.ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), InitialEffects + 1);
	for (const auto FillerId : FillerIds) F.Inventory->RemoveItem(FillerId, FullReturn);

	auto* TargetOwner = F.World->SpawnActor<AUmbraPlayerState>();
	auto* Target = TargetOwner->GetUmbraAbilitySystemComponent();
	Target->InitializeComponent(); Target->InitAbilityActorInfo(TargetOwner, TargetOwner); TargetOwner->InitializeAttributes();
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetMaxHealthAttribute(), 10000);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetArmorAttribute(), 0);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetSlashingResistanceAttribute(), 0);
	F.ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), 0);
	auto CheckDamage = [&](bool bEquipped)
	{
		FUmbraDamageRequest Hit(EUmbraDamageSource::BasicAttack);
		Hit.Config.Typed.Model = EUmbraDamageModel::ExplicitChannels;
		Hit.Config.Typed.bUseDamageBuckets = true;
		auto& Channel = Hit.Config.Typed.Channels.AddDefaulted_GetRef();
		Channel.Type = EUmbraWeaponDamageType::Slashing; Channel.BaseDamage = 100;
		Target->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 10000);
		TestTrue(TEXT("Random A/X hit accepted"), UmbraDamage::Apply(F.ASC, Target, Hit));
		const float Expected = bEquipped ? 100 * (1 + Instance.RolledAffixes[3].Value) * Instance.RolledAffixes[4].Value : 100;
		TestTrue(TEXT("Health loss uses actual rolled A/X"), FMath::IsNearlyEqual(10000 - Target->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), Expected, .003f));
	};
	CheckDamage(true);
	F.Equipment->UnregisterComponent();
	TestEqual(TEXT("Unregister removes equipment GE"), F.ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), InitialEffects);
	F.Equipment->RegisterComponent();
	TestTrue(TEXT("Reregister restores complete payload"), Same(Instance, Worn(F.Equipment, Id)));
	TestEqual(TEXT("Reregister applies once"), F.ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), InitialEffects + 1);
	CheckDamage(true);

	FUmbraInventoryItem Returned;
	TestEqual(TEXT("Stale GUID cannot remove"), F.Owner->UnequipToInventory(Slot, FGuid::NewGuid(), Returned), EUmbraTransferResult::NotFound);
	// Force destination failure after removal; rollback must restore the same rolls and one GE.
	bInterruptTransfer = true;
	const auto ReturnInterruption = F.ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetStrengthAttribute()).AddLambda(
		[&](const FOnAttributeChangeData&) { if (bInterruptTransfer) { bInterruptTransfer = false; F.Inventory->UnregisterComponent(); } });
	TestEqual(TEXT("Interrupted return triggers rollback"), F.Owner->UnequipToInventory(Slot, Id, Returned), EUmbraTransferResult::TransferFailed);
	F.ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetStrengthAttribute()).Remove(ReturnInterruption);
	F.Inventory->RegisterComponent();
	TestTrue(TEXT("Return rollback restores exact equipped instance"), Same(Instance, Worn(F.Equipment, Id)));
	TestEqual(TEXT("Return rollback restores one GE"), F.ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), InitialEffects + 1);
	CheckDamage(true);
	TestEqual(TEXT("Unequip to specified slot"), F.Owner->UnequipToInventorySlot(Slot, Id, 17, Returned), EUmbraTransferResult::Success);
	TestTrue(TEXT("Returned payload exact"), Same(Instance, Returned.GetInstance()) && Same(Instance, Stored(F.Inventory, Id)));
	TestEqual(TEXT("Returned slot"), Returned.SlotIndex, 17);
	TestEqual(TEXT("Strength restored"), F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), Strength);
	TestEqual(TEXT("Armor restored"), F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), Armor);
	TestEqual(TEXT("Health capacity restored"), F.ASC->GetNumericAttribute(UUmbraAttributeSet::GetMaxHealthAttribute()), HealthMax);
	CheckDamage(false);
	for (int32 Repeat = 0; Repeat < 3; ++Repeat)
	{
		TestEqual(TEXT("Repeated equip"), F.Owner->EquipFromInventory(Slot, Id), EUmbraTransferResult::Success);
		TestEqual(TEXT("Repeated unequip"), F.Owner->UnequipToInventory(Slot, Id, Returned), EUmbraTransferResult::Success);
		TestTrue(TEXT("Repeated transfer never rerolls"), Same(Instance, Returned.GetInstance()));
	}
	TestEqual(TEXT("No accumulated effects"), F.ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), InitialEffects);
	F.Inventory->RemoveItem(Id, Returned);
	TestEqual(TEXT("Known GUID definition-only regrant"), F.Inventory->AddItem(Item, Id), EUmbraInventoryResult::Success);
	TestTrue(TEXT("Known GUID reuses saved rolls"), Same(Instance, Stored(F.Inventory, Id)));
	return true;
}
#endif

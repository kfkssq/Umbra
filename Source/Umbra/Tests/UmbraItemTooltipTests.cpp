#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Items/UmbraItemTooltipData.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Equipment/UmbraEquipmentEffect.h"
#include "Player/UmbraPlayerState.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraTooltipDefinitionTest, "Umbra.Items.Tooltip.DefinitionAndScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraTooltipDefinitionTest::RunTest(const FString& Parameters)
{
	auto* Item = NewObject<UUmbraItemDefinition>();
	auto Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestTrue(TEXT("Default preview valid"), Data.bValid);
	TestEqual(TEXT("Compatible item level"), Data.ItemLevel, 1);
	TestEqual(TEXT("Compatible rarity"), Data.Rarity, EUmbraItemRarity::Common);
	TestTrue(TEXT("Empty flavor"), Data.FlavorText.IsEmpty());
	TestFalse(TEXT("Preview has no fake identity"), Data.InstanceId.IsValid());
	TestEqual(TEXT("Fallback asset name"), Data.DisplayName.ToString(), Item->GetName());
	TestEqual(TEXT("No character context is unknown"), Data.RequirementState, EUmbraTooltipRequirementState::Unknown);
	TestEqual(TEXT("Nonweapon no damage"), Data.DamageChannels.Num(), 0);
	const TCHAR* Names[] = {TEXT("Common"), TEXT("Magic"), TEXT("Rare"), TEXT("Epic"), TEXT("Legendary"), TEXT("Unique")};
	for (uint8 Index = 0; Index < 6; ++Index)
	{
		Item->Rarity = EUmbraItemRarity(Index);
		TestEqual(TEXT("Stable serialized quality name"), StaticEnum<EUmbraItemRarity>()->GetNameStringByValue(Index), FString(Names[Index]));
		TestEqual(TEXT("Quality projects unchanged"), UUmbraItemTooltipDataBuilder::FromDefinition(Item).Rarity, Item->Rarity);
	}
	Item->ItemLevel = 90;
	Item->RequiredLevel = 3;
	Item->RarityColor = FLinearColor::Green;
	Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestEqual(TEXT("Independent required level"), Data.RequiredLevel, 3);
	TestEqual(TEXT("Display-only item level"), Data.ItemLevel, 90);
	TestEqual(TEXT("Authored color retained"), Data.RarityColor, FLinearColor::Green);
	Item->Weapon = NewObject<UUmbraWeaponProfile>(Item);
	for (uint8 Index = 0; Index < 9; ++Index)
	{
		auto& Channel = Item->Weapon->Damage.Channels.AddDefaulted_GetRef();
		Channel.Type = EUmbraWeaponDamageType(Index);
		Channel.BaseDamage = float(Index + 1);
	}
	Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestEqual(TEXT("Nine types preserved"), Data.DamageChannels.Num(), 9);
	TestEqual(TEXT("Sum of authored base only"), Data.TotalBaseDamage, 45.);
	for (uint8 Index = 0; Index < 9; ++Index)
	{
		TestEqual(TEXT("Damage identity order"), Data.DamageChannels[Index].Type, EUmbraWeaponDamageType(Index));
		TestFalse(TEXT("Every type named"), Data.DamageChannels[Index].Name.IsEmpty());
	}
	for (const auto& Scaling : Data.Scaling) TestEqual(TEXT("No scaling hidden"), Scaling.Grade, EUmbraScalingGrade::None);
	Item->Weapon->Damage.Channels.SetNum(1);
	auto& Channel = Item->Weapon->Damage.Channels[0];
	Channel.BaseDamage = 100.f;
	Channel.Scaling.AddDefaulted_GetRef().Coefficient = 1.f;
	Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestEqual(TEXT("Single channel S"), Data.Scaling[0].Grade, EUmbraScalingGrade::S);
	FUmbraWeaponDamageChannel Second;
	Second.Type = EUmbraWeaponDamageType::Fire;
	Second.BaseDamage = 300.f;
	Second.Scaling.AddDefaulted_GetRef().Coefficient = .2f;
	Item->Weapon->Damage.Channels.Add(Second);
	Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestTrue(TEXT("Weighted .4 not summed1.2"), FMath::IsNearlyEqual(Data.Scaling[0].ReferenceCoefficient, .4, 1.e-7));
	TestEqual(TEXT("Weighted C"), Data.Scaling[0].Grade, EUmbraScalingGrade::C);
	const auto* Settings = GetDefault<UUmbraTooltipSettings>();
	const float Values[] = {0.f, .01f, .399f, .4f, .6f, .8f, 1.f};
	const EUmbraScalingGrade Grades[] = {EUmbraScalingGrade::None, EUmbraScalingGrade::D, EUmbraScalingGrade::D, EUmbraScalingGrade::C, EUmbraScalingGrade::B, EUmbraScalingGrade::A, EUmbraScalingGrade::S};
	for (int32 Index = 0; Index < 7; ++Index) TestEqual(TEXT("Threshold boundary"), Settings->GradeFor(Values[Index]), Grades[Index]);
	Item->Weapon->StrengthGrade.bOverride = true;
	Item->Weapon->StrengthGrade.Grade = EUmbraScalingGrade::None;
	Item->Weapon->FaithGrade.bOverride = true;
	Item->Weapon->FaithGrade.Grade = EUmbraScalingGrade::A;
	Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestEqual(TEXT("Explicit None hides automatic"), Data.Scaling[0].Grade, EUmbraScalingGrade::None);
	TestEqual(TEXT("Manual nonzero grade"), Data.Scaling[3].Grade, EUmbraScalingGrade::A);
	TestTrue(TEXT("Override source retained"), Data.Scaling[3].bManual);
	Item->Weapon->StrengthGrade.bOverride = false;
	auto* Curve = NewObject<UCurveFloat>(Item);
	Curve->FloatCurve.AddKey(0.f, 0.f);
	Curve->FloatCurve.AddKey(100.f, 500.f);
	Item->Weapon->Damage.Channels[0].Scaling[0].PointCurve = Curve;
	Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestTrue(TEXT("Curve reference marker"), Data.Scaling[0].bHasCurve);
	TestEqual(TEXT("Curve not evaluated for grade"), Data.Scaling[0].Grade, EUmbraScalingGrade::C);
	for (auto& Entry : Item->Weapon->Damage.Channels) Entry.BaseDamage = 0.f;
	Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestEqual(TEXT("Zero base hides damage section"), Data.DamageChannels.Num(), 0);
	TestEqual(TEXT("Zero denominator automatic None"), Data.Scaling[0].Grade, EUmbraScalingGrade::None);
	TestEqual(TEXT("Zero base manual still explicit"), Data.Scaling[3].Grade, EUmbraScalingGrade::A);
	Item->Weapon->Damage.Channels.Reset();
	TestTrue(TEXT("Empty profile valid"), UUmbraItemTooltipDataBuilder::FromDefinition(Item).bValid);
	Item->Weapon->Damage.Channels.AddDefaulted_GetRef().BaseDamage = -1.f;
	Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestFalse(TEXT("Invalid profile fails closed"), Data.bValid);
	TestNull(TEXT("Failure clears previous definition"), Data.ItemDefinition.Get());
	// Read a real pre-feature asset without saving it: absent serialized fields inherit C++ defaults.
	auto* Legacy = LoadObject<UUmbraItemDefinition>(nullptr, TEXT("/Game/Items/Weapons/DA_Item_TestSword.DA_Item_TestSword"));
	if (TestNotNull(TEXT("Existing item asset loads"), Legacy))
	{
		TestEqual(TEXT("Old asset default item level"), Legacy->ItemLevel, 1);
		TestEqual(TEXT("Old asset default rarity"), Legacy->Rarity, EUmbraItemRarity::Common);
		TestTrue(TEXT("Old asset default flavor"), Legacy->FlavorText.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraTooltipFormattingTest, "Umbra.Items.Tooltip.Formatting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraTooltipFormattingTest::RunTest(const FString& Parameters)
{
	auto* Item = NewObject<UUmbraItemDefinition>();
	Item->Armor = 12.f;
	Item->MagicResistance = 8.f;
	Item->PrimaryBonuses.Strength = 1.f;
	Item->PrimaryBonuses.Dexterity = 2.f;
	Item->PrimaryBonuses.Intelligence = 3.f;
	Item->PrimaryBonuses.Faith = 4.f;
	for (uint8 Index = 0; Index <= uint8(EUmbraEquipmentAffixStat::ShadowResistance); ++Index)
	{
		auto& Affix = Item->AttributeBonuses.AddDefaulted_GetRef();
		Affix.Stat = EUmbraEquipmentAffixStat(Index);
		Affix.Magnitude = Affix.Stat == EUmbraEquipmentAffixStat::AttackSpeed ? .2f
			: Affix.Stat == EUmbraEquipmentAffixStat::CriticalChance ? .1f : 25.f;
	}
	FUmbraDamageBonus Bonus;
	Bonus.Magnitude = FScalableFloat(.15f);
	Bonus.Types = {EUmbraWeaponDamageType::Fire, EUmbraWeaponDamageType::Cold};
	Bonus.AttackSource = EUmbraBonusAttackSource::Skill;
	Bonus.bRequiresCritical = true;
	Bonus.bRequiresVulnerable = true;
	Bonus.SourceRequirements.RequireTags.AddTag(UmbraGameplayTags::Damage_Source_Skill);
	Bonus.TargetRequirements.IgnoreTags.AddTag(UmbraGameplayTags::State_Vulnerable);
	Bonus.TargetRequirements.TagQuery = FGameplayTagQuery::MakeQuery_MatchTag(UmbraGameplayTags::State_Vulnerable);
	Item->DamageBonuses.Add(Bonus);
	Bonus.Bucket = EUmbraDamageBucket::Multiplicative;
	Bonus.Magnitude = FScalableFloat(1.2f);
	Bonus.AttackSource = EUmbraBonusAttackSource::BasicAttack;
	Item->DamageBonuses.Add(Bonus);
	const auto Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	if (!TestTrue(TEXT("Formatting definition valid"), Data.bValid)) return false;
	TestEqual(TEXT("One unified list"), Data.Stats.Num(), 24);
	TestEqual(TEXT("Intrinsic source"), Data.Stats[0].Source, EUmbraTooltipStatSource::Intrinsic);
	TestEqual(TEXT("Affix source"), Data.Stats[2].Source, EUmbraTooltipStatSource::FixedAffix);
	for (const auto& Line : Data.Stats)
	{
		const FString Text = Line.Text.ToString();
		if (Line.StatId == TEXT("AttackSpeed")) { TestTrue(TEXT("Attack speed +20%"), Text.Contains(TEXT("+20%"))); TestEqual(TEXT("Raw speed fraction"), Line.RawValue, double(.2f)); }
		if (Line.StatId == TEXT("CriticalChance")) TestTrue(TEXT("Crit +10%"), Text.Contains(TEXT("+10%")));
		if (Line.StatId == TEXT("MoveSpeed")) TestTrue(TEXT("Speed in cm/s"), Text.Contains(TEXT("+25 cm/s")));
		if (Line.Unit == EUmbraTooltipUnit::Rating) { TestTrue(TEXT("Resistance rating"), Text.Contains(TEXT("评分"))); TestFalse(TEXT("Rating never percent"), Text.Contains(TEXT("%"))); }
		if (Line.Unit == EUmbraTooltipUnit::Points) TestFalse(TEXT("Flat values never percent"), Text.Contains(TEXT("%")));
		if (Line.bDamageBonus)
		{
			TestTrue(TEXT("Type restriction"), Text.Contains(TEXT("火焰 / 寒冷")));
			TestTrue(TEXT("Crit condition"), Text.Contains(TEXT("仅暴击时")));
			TestTrue(TEXT("Vulnerable condition"), Text.Contains(TEXT("仅目标易伤时")));
			TestTrue(TEXT("Source tags visible"), Text.Contains(TEXT("来源标签条件")));
			TestTrue(TEXT("Target tags visible"), Text.Contains(TEXT("目标标签条件")));
			TestTrue(TEXT("Raw query retained"), !Line.DamageBonus.TargetRequirements.TagQuery.IsEmpty());
			if (Line.Unit == EUmbraTooltipUnit::Percent) { TestTrue(TEXT("A percent"), Text.Contains(TEXT("+15%"))); TestTrue(TEXT("Skill filter"), Text.Contains(TEXT("仅技能"))); TestFalse(TEXT("No internal A marker"), Text.Contains(TEXT("加算区"))); }
			else { TestTrue(TEXT("X is factor"), Text.Contains(TEXT("×1.2"))); TestFalse(TEXT("No internal X marker"), Text.Contains(TEXT("独立乘区"))); TestTrue(TEXT("Basic filter"), Text.Contains(TEXT("仅普通攻击"))); }
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraTooltipIdentityTest, "Umbra.Items.Tooltip.IdentityAndRequirements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraTooltipIdentityTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Owner = World->SpawnActor<AUmbraPlayerState>();
	auto* ASC = Owner->GetUmbraAbilitySystemComponent();
	ASC->InitializeComponent();
	ASC->InitAbilityActorInfo(Owner, Owner);
	Owner->InitializeAttributes();
	auto* Equipment = Owner->FindComponentByClass<UUmbraEquipmentComponent>();
	auto* Derived = Owner->FindComponentByClass<UUmbraDerivedStatsComponent>();
	auto* Inventory = Owner->GetInventoryComponent();
	Derived->bUseWeaponDerivedPower = true;
	Equipment->bEnableEquipment = true;
	Equipment->UnmetDefenseMultiplier = .3f;
	Equipment->UnmetWeaponBaseMultiplier = .7f;
	Owner->InitializeAttributes();
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 10.f);
	auto* Item = NewObject<UUmbraItemDefinition>(Owner);
	Item->AllowedSlots = {EUmbraEquipmentSlot::MainHand};
	Item->Requirements.Strength = 15.f;
	Item->PrimaryBonuses.Strength = 10.f;
	Item->Weapon = NewObject<UUmbraWeaponProfile>(Item);
	Item->Weapon->Damage.Channels.AddDefaulted_GetRef().BaseDamage = 100.f;
	const FGuid First = FGuid::NewGuid(), Second = FGuid::NewGuid();
	Inventory->AddItem(Item, First);
	Inventory->AddItem(Item, Second);
	auto Snapshot = Inventory->GetSnapshot();
	const int32 FirstSlot = Snapshot.Items.FindByPredicate([First](const auto& E) { return E.InstanceId == First; })->SlotIndex;
	const int32 SecondSlot = Snapshot.Items.FindByPredicate([Second](const auto& E) { return E.InstanceId == Second; })->SlotIndex;
	const auto Read = [&](const FUmbraInventorySnapshot& Input, FGuid Id, int32 Slot)
	{
		return UUmbraItemTooltipDataBuilder::FromInventory(Input, Slot, Id, Item, Equipment, true, EUmbraEquipmentSlot::MainHand);
	};
	const auto EffectsBefore = ASC->GetActiveEffects(FGameplayEffectQuery());
	const int32 RevisionBefore = Equipment->GetSnapshot().Revision;
	const auto Attributes = UUmbraEquipmentEffect::Attributes();
	TArray<float> ValuesBefore;
	for (const auto& Attribute : Attributes) ValuesBefore.Add(ASC->GetNumericAttribute(Attribute));
	Item->RequiredLevel = 2;
	TestEqual(TEXT("Level blocks wearing"), Read(Snapshot, First, FirstSlot).RequirementState, EUmbraTooltipRequirementState::LevelTooLow);
	Item->RequiredLevel = 1;
	auto Data = Read(Snapshot, First, FirstSlot);
	TestEqual(TEXT("Primary shortage allows penalized wearing"), Data.RequirementState, EUmbraTooltipRequirementState::PrimaryPenalty);
	TestEqual(TEXT("Candidate own +10 excluded"), Data.Requirements.CurrentPrimaries[0], 10.f);
	TestEqual(TEXT("Configured penalty read"), Data.Requirements.WeaponBaseMultiplier, .7f);
	TestTrue(TEXT("Configured penalty explained"), Data.PenaltyText.ToString().Contains(TEXT("0.7")));
	TestEqual(TEXT("First instance preserved"), Data.InstanceId, First);
	TestEqual(TEXT("Same definition second identity"), Read(Snapshot, Second, SecondSlot).InstanceId, Second);
	TestEqual(TEXT("No inventory mutation"), Inventory->GetSnapshot().Items.Num(), Snapshot.Items.Num());
	TestEqual(TEXT("No equipment refresh"), Equipment->GetSnapshot().Revision, RevisionBefore);
	TestTrue(TEXT("No effect mutation"), ASC->GetActiveEffects(FGameplayEffectQuery()) == EffectsBefore);
	for (int32 Index = 0; Index < Attributes.Num(); ++Index) TestEqual(TEXT("No GAS mutation"), ASC->GetNumericAttribute(Attributes[Index]), ValuesBefore[Index]);
	TestFalse(TEXT("Zero GUID"), Read(Snapshot, FGuid(), FirstSlot).bValid);
	TestFalse(TEXT("Wrong identity"), Read(Snapshot, Second, FirstSlot).bValid);
	auto Bad = Snapshot;
	Bad.bReady = false;
	TestFalse(TEXT("Not ready"), Read(Bad, First, FirstSlot).bValid);
	Bad = Snapshot;
	Bad.Items[0].Definition = nullptr;
	TestFalse(TEXT("Missing definition"), Read(Bad, First, FirstSlot).bValid);
	Bad = Snapshot;
	Bad.Items[0].InstanceId = FGuid();
	TestFalse(TEXT("Snapshot zero GUID"), Read(Bad, Second, SecondSlot).bValid);
	Bad = Snapshot;
	const FUmbraInventoryItem DuplicateInventory = Bad.Items[0];
	Bad.Items.Add(DuplicateInventory);
	TestFalse(TEXT("Duplicate snapshot"), Read(Bad, First, FirstSlot).bValid);
	TestFalse(TEXT("Invalid slot"), Read(Snapshot, First, 999).bValid);
	TestFalse(TEXT("Mismatched expected definition"), UUmbraItemTooltipDataBuilder::FromInventory(Snapshot, FirstSlot, First,
		NewObject<UUmbraItemDefinition>(), Equipment, false, EUmbraEquipmentSlot::Head).bValid);
	Owner->EquipFromInventory(EUmbraEquipmentSlot::MainHand, First);
	TestFalse(TEXT("Old inventory now equipped rejected"), Read(Snapshot, First, FirstSlot).bValid);
	auto Worn = Equipment->GetSnapshot();
	Data = UUmbraItemTooltipDataBuilder::FromEquipment(Worn, EUmbraEquipmentSlot::MainHand, First, Item, Equipment);
	TestTrue(TEXT("Equipment source"), Data.bEquipped);
	TestEqual(TEXT("Authored base excludes .7 penalty"), Data.TotalBaseDamage, 100.);
	TestFalse(TEXT("Snapshot status used"), Data.bRequirementsMet);
	TestEqual(TEXT("Equipped self excluded"), Data.Requirements.CurrentPrimaries[0], 10.f);
	TestEqual(TEXT("Other candidate target excludes worn item"), Read(Inventory->GetSnapshot(), Second, SecondSlot).Requirements.CurrentPrimaries[0], 10.f);
	auto* Multiplier = NewObject<UGameplayEffect>(Owner);
	Multiplier->DurationPolicy = EGameplayEffectDurationType::Infinite;
	auto& Modifier = Multiplier->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UUmbraAttributeSet::GetStrengthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Multiplicitive;
	Modifier.ModifierMagnitude = FScalableFloat(1.4f);
	const auto Handle = ASC->ApplyGameplayEffectToSelf(Multiplier, 1.f, ASC->MakeEffectContext());
	Worn = Equipment->GetSnapshot();
	Data = UUmbraItemTooltipDataBuilder::FromEquipment(Worn, EUmbraEquipmentSlot::MainHand, First, Item, Equipment);
	TestEqual(TEXT("Filtered GAS 14 not raw subtraction18"), Data.Requirements.CurrentPrimaries[0], 14.f);
	TestFalse(TEXT("Multiplier still insufficient"), Data.bRequirementsMet);
	const auto NoContext = UUmbraItemTooltipDataBuilder::FromEquipment(Worn, EUmbraEquipmentSlot::MainHand, First, Item, nullptr);
	TestTrue(TEXT("Snapshot aggregate usable without ASC"), NoContext.bPrimaryStatusKnown);
	TestFalse(TEXT("No invented detailed current values"), NoContext.Requirements.bKnown);
	ASC->RemoveActiveGameplayEffect(Handle);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 15.f);
	TestFalse(TEXT("Stale equipment revision"), UUmbraItemTooltipDataBuilder::FromEquipment(Worn, EUmbraEquipmentSlot::MainHand, First, Item, Equipment).bValid);
	Worn = Equipment->GetSnapshot();
	Data = UUmbraItemTooltipDataBuilder::FromEquipment(Worn, EUmbraEquipmentSlot::MainHand, First, Item, Equipment);
	TestEqual(TEXT("All requirements met"), Data.RequirementState, EUmbraTooltipRequirementState::Met);
	TestFalse(TEXT("No penalty"), Data.bHasEquipmentPenalty);
	const FUmbraEquippedItem DuplicateEquipment = Worn.Items[0];
	Worn.Items.Add(DuplicateEquipment);
	TestFalse(TEXT("Duplicate equipment slot and identity"), UUmbraItemTooltipDataBuilder::FromEquipment(Worn, EUmbraEquipmentSlot::MainHand, First, Item, nullptr).bValid);
	Worn.bValid = false;
	Data = UUmbraItemTooltipDataBuilder::FromEquipment(Worn, EUmbraEquipmentSlot::MainHand, First, Item, nullptr);
	TestFalse(TEXT("Invalid equipment"), Data.bValid);
	TestNull(TEXT("Failure discards previous item"), Data.ItemDefinition.Get());
	TestTrue(TEXT("Failure clears stats"), Data.Stats.IsEmpty());
	World->DestroyWorld(false);
	return true;
}
#endif

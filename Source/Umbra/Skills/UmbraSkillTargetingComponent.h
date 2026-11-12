#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "UmbraSkillTargetingComponent.generated.h"
class UUmbraSweepDefinition;
class ACharacter;
class UDecalComponent;
class UMaterialInstanceDynamic;

/** Local aim presentation; confirmation is revalidated on the owning controller's server. */
UCLASS(ClassGroup=(Skills))
class UMBRA_API UUmbraSkillTargetingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UUmbraSkillTargetingComponent();
    bool HandleSkillInput(FGameplayTag Tag);
    bool HandlePointerKey(FKey Key, EInputEvent Event);
    UFUNCTION(BlueprintCallable) void CancelTargeting();
    UFUNCTION(BlueprintCallable) void ConfirmTargeting();
    UFUNCTION(BlueprintPure) bool IsTargeting() const { return Definition != nullptr; }
    UPROPERTY(BlueprintReadOnly) FVector AimLocation;
    UPROPERTY(BlueprintReadOnly) bool bValidTarget = false;
    UPROPERTY(BlueprintReadOnly) FText TargetStatus;
    /** Also used by authority at confirmation and takeoff. bClamp only for local cursor selection. */
    static bool EvaluateTarget(ACharacter* Character, const UUmbraSweepDefinition* Data, const FVector& Requested,
        bool bClamp, FVector& Ground, FText& Reason);
    UFUNCTION(BlueprintCallable) bool UpdateTargetLocation(FVector Requested);
    virtual void TickComponent(float Delta, ELevelTick Tick, FActorComponentTickFunction* Function) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UFUNCTION(Server, Reliable) void ServerConfirm(FGameplayAbilitySpecHandle Handle, FVector_NetQuantize Location, uint32 Request);
    UFUNCTION(Client, Reliable) void ClientConfirmResult(uint32 Request, bool bAccepted, const FText& Reason);
    bool IsPointerOverUI() const;
    void UpdateIndicators();
    UPROPERTY(Transient) TObjectPtr<UUmbraSweepDefinition> Definition;
    UPROPERTY(Transient) TObjectPtr<AActor> IndicatorActor;
    UPROPERTY(Transient) TObjectPtr<UDecalComponent> RangeDecal;
    UPROPERTY(Transient) TObjectPtr<UDecalComponent> ImpactDecal;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RangeMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ImpactMaterial;
    UPROPERTY(Transient) TObjectPtr<class UUmbraTargetingPrompt> Prompt;
    FGameplayAbilitySpecHandle SpecHandle;
    TWeakObjectPtr<APawn> AimingPawn;
    TSet<FKey> ConsumedKeys;
    uint32 Serial = 0;
    bool bPending = false;
};

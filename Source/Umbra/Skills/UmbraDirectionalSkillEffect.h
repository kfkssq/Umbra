#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UmbraDirectionalSkillEffect.generated.h"

class UUmbraSweepDefinition;
/** One resolved directional cast. Cosmetic on clients; authority submits center-based GAS hits. */
UCLASS()
class UMBRA_API AUmbraDirectionalSkillEffect : public AActor
{
    GENERATED_BODY()
public:
    AUmbraDirectionalSkillEffect();
    void Initialize(const UUmbraSweepDefinition* Data, AActor* Caster, FVector Origin, FVector Forward, bool bDamage);
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY() TObjectPtr<const UUmbraSweepDefinition> Definition;
    UPROPERTY() TObjectPtr<class UDecalComponent> Decal;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> Material;
    TWeakObjectPtr<AActor> Source;
    TSet<TWeakObjectPtr<AActor>> HitActors;
    FVector CastOrigin;
    FVector Direction;
    float Age = 0.f;
    float PreviousFront = 0.f;
    bool bApplyDamage = false;
    bool bResolved = false;
};

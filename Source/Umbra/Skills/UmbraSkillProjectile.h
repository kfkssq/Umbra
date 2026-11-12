#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UmbraSkillProjectile.generated.h"

class UUmbraSweepDefinition;
class UParticleSystemComponent;
class UAudioComponent;
class UNiagaraComponent;

/** Single-hit authoritative swept projectile. Cosmetics cannot apply damage. */
UCLASS()
class UMBRA_API AUmbraSkillProjectile : public AActor
{
    GENERATED_BODY()
public:
    AUmbraSkillProjectile();
    void Initialize(const UUmbraSweepDefinition* InDefinition, FVector InDirection, AActor* InTarget, int32 InLevel);
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    /** Also used for the owner-to-muzzle clearance sweep, preventing wall bypass. */
    void Impact(const FHitResult& Hit);
private:
    UPROPERTY(Replicated) TObjectPtr<const UUmbraSweepDefinition> Definition;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UParticleSystemComponent> Flight;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> FlightAudio;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> FlightNiagara;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> FlightMesh;
    UPROPERTY(ReplicatedUsing=OnRep_Impact) bool bImpacted = false;
    UPROPERTY(Replicated) FVector_NetQuantize ImpactLocation;
    UPROPERTY(Replicated) FVector_NetQuantizeNormal ImpactNormal;
    UFUNCTION() void OnRep_Impact();
    TWeakObjectPtr<AActor> HomingTarget;
    FVector Direction = FVector::ForwardVector;
    float Age = 0.f;
    float Speed = 0.f;
    int32 AbilityLevel = 1;
    bool bImpactPresented = false;
};

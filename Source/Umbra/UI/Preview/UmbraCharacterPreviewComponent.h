#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UmbraCharacterPreviewComponent.generated.h"

class ACharacter;
class UAnimationAsset;
class UAnimInstance;
class USceneCaptureComponent2D;
class USkeletalMeshComponent;
class UTextureRenderTarget2D;
class ULightComponent;

USTRUCT(BlueprintType)
struct UMBRA_API FUmbraCharacterPreviewSettings
{
	GENERATED_BODY()

	/** Use the existing BP components' transforms, animation and TextureTarget directly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
	bool bUseBlueprintConfiguration = true;

	/** Local to the reused preview actor, in cm. Defaults frame a roughly 200 cm hero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (EditCondition = "!bUseBlueprintConfiguration", EditConditionHides))
	FTransform CameraTransform = FTransform(FRotator(0, 180, 0), FVector(450, 0, 100));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (EditCondition = "!bUseBlueprintConfiguration", EditConditionHides, ClampMin = "5", ClampMax = "90", Units = "deg"))
	float CameraFOV = 30.f;
	/** Local mesh transform; scale here controls the displayed model, not the gameplay actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (EditCondition = "!bUseBlueprintConfiguration", EditConditionHides))
	FTransform CharacterTransform = FTransform(FRotator(0, -90, 0));
	/** Optional template, duplicated per preview. Never writes into a shared asset or HUD target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (EditCondition = "!bUseBlueprintConfiguration", EditConditionHides))
	TObjectPtr<UTextureRenderTarget2D> RenderTargetTemplate = nullptr;
	/** Used when no template is supplied. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (EditCondition = "!bUseBlueprintConfiguration", EditConditionHides, ClampMin = "64", ClampMax = "2048"))
	FIntPoint OutputSize = FIntPoint(512, 1024);
	/** Explicit Idle wins over AnimBP, then the existing BP's single-node Idle is reused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (EditCondition = "!bUseBlueprintConfiguration", EditConditionHides))
	TObjectPtr<UAnimationAsset> IdleAnimation = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (EditCondition = "!bUseBlueprintConfiguration", EditConditionHides))
	TSubclassOf<UAnimInstance> PreviewAnimClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (ClampMin = "1", ClampMax = "60", Units = "Hz"))
	float CaptureRate = 30.f;
};

/** Adapts the existing portrait BP's mesh, capture and lights; creates no duplicate rig. */
UCLASS(Blueprintable, ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class UMBRA_API UUmbraCharacterPreviewComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUmbraCharacterPreviewComponent();
	/** Components must belong to this independent preview actor, never to SourceCharacter. */
	UFUNCTION(BlueprintCallable, Category = "Preview")
	bool InitializePreview(ACharacter* SourceCharacter, USkeletalMeshComponent* InMesh,
		USceneCaptureComponent2D* InCapture, const FUmbraCharacterPreviewSettings& InSettings);
	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetPreviewActive(bool bActive);
	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetSourceCharacter(ACharacter* SourceCharacter);
	UFUNCTION(BlueprintCallable, Category = "Preview")
	void RefreshAppearance();
	UFUNCTION(BlueprintCallable, Category = "Preview")
	void RefreshEquipmentVisuals();
	UFUNCTION(BlueprintPure, Category = "Preview")
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }
	UFUNCTION(BlueprintPure, Category = "Preview")
	bool IsPreviewActive() const { return bPreviewActive; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	/** Future equipment owner calls RefreshEquipmentVisuals on change. No item polling. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Preview")
	void BP_RefreshEquipmentVisuals(ACharacter* SourceCharacter, USkeletalMeshComponent* PreviewMesh);

private:
	void CaptureFrame();
	void PreparePreviewPrimitives();
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> Mesh;
	UPROPERTY(Transient)
	TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;
	UPROPERTY(Transient)
	FUmbraCharacterPreviewSettings Settings;
	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> InheritedIdle;
	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> InheritedAnimClass;
	TWeakObjectPtr<ACharacter> Source;
	FTimerHandle CaptureTimer;
	TMap<TWeakObjectPtr<ULightComponent>, bool> AuthoredLightVisibility;
	bool bPreviewActive = false;
};

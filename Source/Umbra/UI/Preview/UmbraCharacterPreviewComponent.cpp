#include "UI/Preview/UmbraCharacterPreviewComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/LightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Umbra.h"
#include "UObject/UObjectIterator.h"

UUmbraCharacterPreviewComponent::UUmbraCharacterPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UUmbraCharacterPreviewComponent::InitializePreview(ACharacter* SourceCharacter,
	USkeletalMeshComponent* InMesh, USceneCaptureComponent2D* InCapture,
	const FUmbraCharacterPreviewSettings& InSettings)
{
	SetPreviewActive(false);
	if (!IsValid(InMesh) || !IsValid(InCapture) || !GetOwner() ||
		GetOwner() == SourceCharacter || InMesh->GetOwner() != GetOwner() || InCapture->GetOwner() != GetOwner())
	{
		UE_LOG(LogUmbra, Warning, TEXT("Preview requires mesh and capture owned by an independent preview actor."));
		return false;
	}
	Mesh = InMesh;
	Capture = InCapture;
	Settings = InSettings;
	InheritedIdle = Mesh->AnimationData.AnimToPlay;
	InheritedAnimClass = Mesh->GetAnimationMode() == EAnimationMode::AnimationBlueprint ? Mesh->GetAnimClass() : nullptr;
	GetOwner()->SetReplicates(false);
	GetOwner()->SetActorEnableCollision(false);
	GetOwner()->SetActorTickEnabled(false);
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->Deactivate();
	if (!Settings.bUseBlueprintConfiguration)
	{
		Capture->SetRelativeTransform(Settings.CameraTransform);
		Capture->FOVAngle = FMath::Clamp(Settings.CameraFOV, 5.f, 90.f);
		Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
		Capture->ShowFlags.SetAtmosphere(false);
		Capture->ShowFlags.SetFog(false);
		Capture->ShowFlags.SetVolumetricFog(false);
		Mesh->SetRelativeTransform(Settings.CharacterTransform);
	}
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh->SetSimulatePhysics(false);
	Mesh->SetEnableGravity(false);

	if (Settings.bUseBlueprintConfiguration)
	{
		RenderTarget = Capture->TextureTarget;
		if (!RenderTarget)
		{
			UE_LOG(LogUmbra, Warning, TEXT("Preview %s needs a dedicated full-body TextureTarget on its Blueprint Capture component."), *GetNameSafe(GetOwner()));
			return false;
		}
		// An asset RT has one writer. Refuse to take over the HUD's target or another local preview.
		for (TObjectIterator<USceneCaptureComponent2D> It; It; ++It)
		{
			if (*It != Capture && It->GetWorld() && It->GetWorld()->IsGameWorld() &&
				It->IsRegistered() && It->TextureTarget == RenderTarget)
			{
				UE_LOG(LogUmbra, Warning, TEXT("Preview TextureTarget %s is already assigned to another capture. Use a dedicated full-body RT."), *GetNameSafe(RenderTarget));
				RenderTarget = nullptr;
				return false;
			}
		}
	}
	else
	{
		RenderTarget = Settings.RenderTargetTemplate
			? DuplicateObject<UTextureRenderTarget2D>(Settings.RenderTargetTemplate, this)
			: NewObject<UTextureRenderTarget2D>(this);
		// SceneColorHDR stores inverse opacity. The UI material uses OneMinus(alpha).
		RenderTarget->ClearColor = FLinearColor(0, 0, 0, 1);
		if (!Settings.RenderTargetTemplate)
		{
			RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
			RenderTarget->InitAutoFormat(FMath::Clamp(Settings.OutputSize.X, 64, 2048), FMath::Clamp(Settings.OutputSize.Y, 64, 2048));
		}
		RenderTarget->UpdateResourceImmediate(true);
	}
	Capture->TextureTarget = RenderTarget;
	SetSourceCharacter(SourceCharacter);
	SetPreviewActive(false);
	return true;
}

void UUmbraCharacterPreviewComponent::SetSourceCharacter(ACharacter* SourceCharacter)
{
	Source = SourceCharacter != GetOwner() ? SourceCharacter : nullptr;
	RefreshAppearance();
}

void UUmbraCharacterPreviewComponent::RefreshAppearance()
{
	if (!Mesh) return;
	const ACharacter* Character = Source.Get();
	const USkeletalMeshComponent* SourceMesh = Character ? Character->GetMesh() : nullptr;
	if (SourceMesh && SourceMesh->GetSkeletalMeshAsset())
	{
		Mesh->SetSkeletalMeshAsset(SourceMesh->GetSkeletalMeshAsset());
		Mesh->EmptyOverrideMaterials();
		for (int32 Index = 0; Index < SourceMesh->GetNumMaterials(); ++Index)
		{
			Mesh->SetMaterial(Index, SourceMesh->GetMaterial(Index));
		}
	}
	const TSubclassOf<UAnimInstance> AnimClass = Settings.bUseBlueprintConfiguration ? InheritedAnimClass : Settings.PreviewAnimClass;
	UAnimationAsset* Idle = Settings.bUseBlueprintConfiguration
		? (AnimClass ? nullptr : InheritedIdle.Get())
		: (Settings.IdleAnimation ? Settings.IdleAnimation.Get() : (AnimClass ? nullptr : InheritedIdle.Get()));
	if (Idle)
	{
		Mesh->PlayAnimation(Idle, true);
		// Extract root motion without applying it; no CharacterMovement is involved.
		if (UAnimSingleNodeInstance* Instance = Mesh->GetSingleNodeInstance()) Instance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
	else if (AnimClass)
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		Mesh->SetAnimInstanceClass(AnimClass);
		if (UAnimInstance* Instance = Mesh->GetAnimInstance()) Instance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
	else
	{
		UE_LOG(LogUmbra, Warning, TEXT("Preview %s has no stable Idle configured."), *GetNameSafe(GetOwner()));
	}
	RefreshEquipmentVisuals();
}

void UUmbraCharacterPreviewComponent::PreparePreviewPrimitives()
{
	TInlineComponentArray<UPrimitiveComponent*> Primitives(GetOwner());
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Primitive->SetGenerateOverlapEvents(false);
		Primitive->SetVisibleInSceneCaptureOnly(true);
		Primitive->SetCastShadow(false);
	}
	if (Capture)
	{
		Capture->ClearShowOnlyComponents();
		Capture->ShowOnlyActors.Reset();
		Capture->ShowOnlyActorComponents(GetOwner());
	}
}

void UUmbraCharacterPreviewComponent::RefreshEquipmentVisuals()
{
	BP_RefreshEquipmentVisuals(Source.Get(), Mesh);
	PreparePreviewPrimitives();
	if (bPreviewActive) CaptureFrame();
}

void UUmbraCharacterPreviewComponent::SetPreviewActive(bool bActive)
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(CaptureTimer);
	bPreviewActive = bActive && IsValid(Source.Get()) && Mesh && Capture && RenderTarget;
	if (Mesh) Mesh->SetComponentTickEnabled(bPreviewActive);
	if (GetOwner())
	{
		TInlineComponentArray<ULightComponent*> Lights(GetOwner());
		for (ULightComponent* Light : Lights)
		{
			const bool bAuthoredVisible = AuthoredLightVisibility.FindOrAdd(Light, Light->IsVisible());
			Light->SetVisibility(bPreviewActive && bAuthoredVisible);
		}
	}
	if (!Capture) return;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->SetComponentTickEnabled(false);
	if (bPreviewActive && GetWorld())
	{
		Capture->Activate();
		CaptureFrame();
		GetWorld()->GetTimerManager().SetTimer(CaptureTimer, this, &ThisClass::CaptureFrame,
			1.f / FMath::Clamp(Settings.CaptureRate, 1.f, 60.f), true);
	}
	else Capture->Deactivate();
}

void UUmbraCharacterPreviewComponent::CaptureFrame()
{
	if (!Source.IsValid())
	{
		SetPreviewActive(false);
		return;
	}
	if (bPreviewActive && Capture) Capture->CaptureScene();
}

void UUmbraCharacterPreviewComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	SetPreviewActive(false);
	if (Capture) Capture->TextureTarget = nullptr;
	Super::EndPlay(Reason);
}

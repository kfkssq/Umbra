// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UmbraPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class USpringArmComponent;
struct FInputActionValue;

/**
 * Minimal top-down player character for the Umbra prototype.
 * Asset references and presentation are configured by a derived Blueprint.
 */
UCLASS()
class UMBRA_API AUmbraPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AUmbraPlayerCharacter();

	/** Returns the fixed top-down camera boom. */
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns the top-down camera. */
	UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Two-dimensional movement action configured by the derived Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

private:
	void Move(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;
};

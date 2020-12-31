// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TPPPlayerController.generated.h"

class ATPPPlayerCharacter;

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API ATPPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ATPPPlayerController(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	/** Threshold of axis value to begin weapon fire */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMax = "1.0", UIMax = "1.0", ClampMin = "0.0", UIMin = "0.0"))
	float FireWeaponThreshold = .8f;

protected:

	// Direction the player intends to move based on keys held.
	UPROPERTY(Transient)
	FVector DesiredMovementDirection;

	UPROPERTY(Transient)
	bool bIsMovementInputEnabled = true;

public:

	void SetMovementInputEnabled(bool bIsEnabled);

	UFUNCTION(BlueprintPure)
	FVector GetDesiredMovementDirection() const { return DesiredMovementDirection.GetSafeNormal2D(); }

	/** Gets the rotation of desired movement relative to the player controller */
	UFUNCTION(BlueprintPure)
	FRotator GetRelativeControllerMovementRotation() const;

	virtual void SetupInputComponent() override;

protected:

	UFUNCTION()
	void MoveForward(float value);

	UFUNCTION()
	void MoveRight(float value);

	UFUNCTION()
	void TurnRate(float value);

	UFUNCTION()
	void LookUpRate(float value);

	UFUNCTION()
	void OnLockOnPressed();

	UFUNCTION()
	void OnJumpPressed();
	
	UFUNCTION()
	void OnJumpReleased();

	UFUNCTION()
	void OnSprintPressed();

	UFUNCTION()
	void OnSprintReleased();

	UFUNCTION()
	void OnCrouchPressed();

	UFUNCTION()
	void OnCrouchReleased();

	UFUNCTION()
	void OnMovementAbilityPressed();

	UFUNCTION()
	void HandleWeaponFireAxis(float value);

	UFUNCTION()
	void OnAimWeaponPressed();

	UFUNCTION()
	void OnAimWeaponReleased();

public:

	ATPPPlayerCharacter* GetOwnerCharacter();

protected:

	UPROPERTY(Transient)
	ATPPPlayerCharacter* CachedOwnerCharacter = nullptr;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TPPPlayerController.generated.h"

class ATPPPlayerCharacter;

/** Enum detailing input actions */
UENUM(BlueprintType)
enum class EPlayerInputAction : uint8
{
	Jump,
	MovementAbility,
	Sprint,
	Crouch,
	ADS,
	Reload,
	Pause
};

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

	virtual void UpdateRotation(float DeltaTime);

	/** Threshold of axis value to begin weapon fire */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMax = "1.0", UIMax = "1.0", ClampMin = "0.0", UIMin = "0.0"))
	float FireWeaponThreshold = .8f;

	/** Time to hold key for held button action to register */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HoldKeyThreshold = .5f;

protected:

	// Direction the player intends to move based on keys held.
	UPROPERTY(Transient)
	FVector DesiredMovementDirection;

	UPROPERTY(Transient)
	bool bIsMovementInputEnabled = true;

	/** Map of keys that are held down */
	UPROPERTY(Transient)
	TMap<EPlayerInputAction, float> KeyHoldTimers;

protected:

	/** Ticks all key hold timers */
	void TickKeyHoldTimers(float DeltaTime);

public:

	void SetMovementInputEnabled(bool bIsEnabled);

	UFUNCTION(BlueprintPure)
	FVector GetDesiredMovementDirection() const { return DesiredMovementDirection.GetSafeNormal2D(); }

	/** Gets the rotation of desired movement relative to the player controller */
	UFUNCTION(BlueprintPure)
	FRotator GetRelativeControllerMovementRotation() const;

	/** Gets the desired movement direction relative to the rotation of the player controller. Returns zero vector if desired movement direction is zero. */
	UFUNCTION(BlueprintPure)
	FVector GetControllerRelativeDesiredMovementDirection() const;

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

	UFUNCTION()
	void OnReloadPressed();

	UFUNCTION()
	void OnPausePressed();

public:

	ATPPPlayerCharacter* GetOwnerCharacter();

protected:

	UPROPERTY(Transient)
	ATPPPlayerCharacter* CachedOwnerCharacter = nullptr;

public:

	/** Gets the forward vector of the controller relative to its current rotation */
	UFUNCTION(BlueprintPure)
	FVector GetControllerRelativeForwardVector(bool bIncludeVertical = true) const;
	
	/** Gets the right vector of the controller relative to its current rotation */
	UFUNCTION(BlueprintPure)
	FVector GetControllerRelativeRightVector(bool bIncludeVertical = true) const;

	/** Gets the up vector of the controller relative to its current rotation */
	UFUNCTION(BlueprintPure)
	FVector GetControllerRelativeUpVector() const;

protected:

	/* Cached control rotation. Used to track intended aim direction without recoil */
	UPROPERTY(Transient, VisibleAnywhere)
	FRotator DesiredControlRotation = FRotator::ZeroRotator;

	/** Current recoil to be applied to the camera */
	UPROPERTY(Transient, VisibleAnywhere)
	FRotator CurrentCameraRecoil = FRotator::ZeroRotator;

	/** Recoil that the camera should be interpolating to */
	UPROPERTY(Transient, VisibleAnywhere)
	FRotator TargetCameraRecoil = FRotator::ZeroRotator;

	/** Cached aim restoration delta per frame */
	UPROPERTY(Transient)
	float CachedAimRestorationDelta = 0.0f;


public:

	void AddCameraRecoil(const float RecoilToAdd);

	void ResetCameraRecoil();

	FRotator GetTargetCameraRecoil() const { return TargetCameraRecoil; }

};

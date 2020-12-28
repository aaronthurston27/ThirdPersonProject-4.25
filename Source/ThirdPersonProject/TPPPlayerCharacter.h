// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.h"
#include "ThirdPersonHUD.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/DataTable.h"
#include "BaseAbility.h"
#include "TPPSpecialMove.h"
#include "TPPPlayerCharacter.generated.h"

class ATPPPlayerController;
class UTPPMovementComponent;
class ATPPWeaponBase;

#pragma region Structs_And_Enums

UENUM(BlueprintType)
enum class EAnimationBlendSlot : uint8
{
	/** No blending. Should use the default pose */
	None = 0,
	/** No blending in Anim Instance, but use defaut slot for full body anim */
	FullBody = 1,
	/** Blend lower body only */
	LowerBody = 2,
	/** Blend the upper body only */
	UpperBody = 3,
	/** Full body animation */
};

UENUM(BlueprintType)
enum class ELogLevel : uint8 {
	TRACE			UMETA(DisplayName = "Trace"),
	DEBUG			UMETA(DisplayName = "Debug"),
	INFO			UMETA(DisplayName = "Info"),
	WARNING			UMETA(DisplayName = "Warning"),
	ERROR			UMETA(DisplayName = "Error")
};

UENUM(BlueprintType)
enum class ELogOutput : uint8 {
	ALL				UMETA(DisplayName = "All levels"),
	OUTPUT_LOG		UMETA(DisplayName = "Output log"),
	SCREEN			UMETA(DisplayName = "Screen")
};

#pragma endregion Structs_And_Enums

UCLASS(config=Game,Blueprintable)
class ATPPPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	EAnimationBlendSlot CurrentAnimationBlendSlot;
		
public:
	ATPPPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	void Tick(float DeltaTime) override;

	void BeginPlay() override;

protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	// End of APawn interface

public:

	/** Ability to activate for special movement key */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UBaseAbility> MovementAbilityClass;
	

public:

	UPROPERTY(EditDefaultsOnly)
	float DefaultSpeed;

	UPROPERTY(EditDefaultsOnly)
	float SprintingSpeed;

	UPROPERTY(EditDefaultsOnly)
	float CrouchingSpeed;

	UPROPERTY(EditDefaultsOnly)
	float DefaultRotationRate;

	UPROPERTY(EditDefaultsOnly)
	float SprintRotationRate;

protected:

	UPROPERTY(Transient)
	bool bWantsToSprint = false;

public:

public:

	UFUNCTION(BlueprintPure)
	bool CanSprint() const;

	void BeginSprint();

	void StopSprint();

public:

	virtual bool CanCrouch() const override;

	virtual void Crouch(bool bClientSimulation) override;

	virtual void UnCrouch(bool bClientSimulation) override;

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

public:

	UFUNCTION(BlueprintPure)
	bool CanSlide() const;

	virtual void OnStartSlide();

	virtual void OnEndSlide();

public:

	virtual bool CanJumpInternal_Implementation() const override;

	virtual void Landed(const FHitResult& LandHit) override;

public:

	UFUNCTION(BlueprintPure)
	UTPPMovementComponent* GetTPPMovementComponent() const;

	UFUNCTION(BlueprintPure)
	ATPPPlayerController* GetTPPPlayerController() const;

	UFUNCTION(BlueprintPure)
	FRotator GetAimRotationDelta() const;

protected:

	/** Ability being used */
	UPROPERTY(Transient)
	UBaseAbility* CurrentAbility;

	/** Current special move to track */
	UPROPERTY(Transient)
	UTPPSpecialMove* CurrentSpecialMove;

public:

	void BeginMovementAbility();

	/** Gets current special move */
	UFUNCTION(BlueprintPure)
	UTPPSpecialMove* GetCurrentSpecialMove() const { return CurrentSpecialMove; }

	/** Gets current ability */
	UFUNCTION(BlueprintPure)
	UBaseAbility* GetCurrentAbility() const { return CurrentAbility;  }

	void OnSpecialMoveEnded(UTPPSpecialMove* SpecialMove);

protected:

	UFUNCTION(BlueprintCallable)
	void ExecuteSpecialMove(TSubclassOf<UTPPSpecialMove> SpecialMove);

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintPure, Category = Animation)
	bool ShouldBlendAnimation() const { return CurrentAnimationBlendSlot > EAnimationBlendSlot::FullBody;}

	UFUNCTION(BlueprintPure, Category = Animation)
	EAnimationBlendSlot GetCurrentAnimationBlendSlot() const { return CurrentAnimationBlendSlot;}

	void SetAnimationBlendSlot(const EAnimationBlendSlot NewSlot);

	UFUNCTION(BlueprintCallable)
	void ResetCameraToPlayerRotation();

private:

	void OnLockOnPressed();

	/**
	* Log - prints a message to all the log outputs with a specific color
	* @param LogLevel {@see ELogLevel} affects color of log
	* @param FString the message for display
	* @param ELogOutput - All, Output Log or Screen
	*/
	void Log(ELogLevel LogLevel, FString Message, ELogOutput LogOutput = ELogOutput::ALL);

protected:

	ATPPWeaponBase* CurrentWeapon = nullptr;

public:

	UFUNCTION(BlueprintCallable)
	void SetCurrentEquippedWeapon(ATPPWeaponBase* WeaponEquipped);

	UFUNCTION(BlueprintPure)
	ATPPWeaponBase* GetCurrentEquippedWeapon() const { return CurrentWeapon; }
};


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
#include "ThirdPersonProjectCharacter.generated.h"

class UTPPMovementComponent;

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

UCLASS(config=Game)
class AThirdPersonProjectCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY()
	AThirdPersonHUD* TargetingHud;

	UPROPERTY(VisibleAnywhere)
	UAudioComponent* CharacterAudioComponent;

	EAnimationBlendSlot CurrentAnimationBlendSlot;

	FVector LockOnTarget;
		
public:
	AThirdPersonProjectCharacter(const FObjectInitializer& ObjectInitializer);

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	void Tick(float DeltaTime) override;

	void BeginPlay() override;

protected:

	UPROPERTY(Transient)
	bool bIsMovementInputEnabled = true;

protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	UFUNCTION(BlueprintCallable)
	void ResetCameraToPlayerRotation();

public:

	/** Ability to activate for special movement key */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UBaseAbility> MovementAbilityClass;

	void SetMovementInputEnabled(bool bIsEnabled);
	

public:

	UPROPERTY(EditDefaultsOnly)
	float DefaultSpeed;

	UPROPERTY(EditDefaultsOnly)
	float SprintingSpeed;

	UPROPERTY(EditDefaultsOnly)
	float CrouchingSpeed;

	UPROPERTY(EditDefaultsOnly)
	float SlidingGroundFriction;

	UPROPERTY(EditDefaultsOnly)
	float DefaultRotationRate;

	UPROPERTY(EditDefaultsOnly)
	float SprintRotationRate;

protected:

	UPROPERTY(Transient)
	bool bWantsToSprint = false;

	UFUNCTION()
	void BeginMovementAbility();

	UFUNCTION()
	void OnSprintPressed();

	UFUNCTION()
	void OnSprintReleased();

public:

	UPROPERTY(Transient)
	float CachedGroundFriction;

	bool CanSlide() const;

	UFUNCTION()
	void OnCrouchPressed();

	UFUNCTION()
	void OnCrouchReleased();

	virtual void Crouch(bool bClientSimulation) override;

	virtual void UnCrouch(bool bClientSimulation) override;

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void OnStartSlide();

	virtual void OnEndSlide();

public:

	virtual bool CanJumpInternal_Implementation() const override;

	virtual void Landed(const FHitResult& LandHit) override;

public:

	UFUNCTION(BlueprintPure)
	UTPPMovementComponent* GetTPPMovementComponent() const;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowProtectedAccess = true), Category = "Lock-On")
	FVector LockOnOffset;

	UPROPERTY(EditDefaultsOnly, Category = "Lock-On", meta=(ClampMin="0.001", ClampMax = "1.0"))
	float LockOnRotationLerp;

	ABaseEnemy* EnemyLockedOnTo;

	UPROPERTY(EditDefaultsOnly, Category = "Lock-On")
	FTimeline CameraLockOnTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Lock-On")
	UCurveFloat* LockOnCurve;

protected:

	/** Ability being used */
	UPROPERTY(Transient)
	UBaseAbility* CurrentAbility;

	/** Current special move to track */
	UPROPERTY(Transient)
	UTPPSpecialMove* CurrentSpecialMove;

protected:

	UFUNCTION(BlueprintCallable)
	void ExecuteSpecialMove(TSubclassOf<UTPPSpecialMove> SpecialMove);

	void OnSpecialMoveEnded(UTPPSpecialMove* SpecialMove);

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintPure)
	bool IsCharacterLockedOn();

	UFUNCTION(BlueprintPure, Category = Animation)
	bool ShouldBlendAnimation() const { return CurrentAnimationBlendSlot > EAnimationBlendSlot::FullBody;}

	UFUNCTION(BlueprintPure, Category = Animation)
	EAnimationBlendSlot GetCurrentAnimationBlendSlot() const { return CurrentAnimationBlendSlot;}

	void SetAnimationBlendSlot(const EAnimationBlendSlot NewSlot);

	UFUNCTION()
	void OnLockOnCameraMoveFinished();

private:

	void RotateSideways(float value);

	void RotateUpwards(float value);

	void OnLockOnPressed();

	void RotateToTargetEnemy();

	UFUNCTION()
	void MoveLockOnCamera();

	/**
	* Log - prints a message to all the log outputs with a specific color
	* @param LogLevel {@see ELogLevel} affects color of log
	* @param FString the message for display
	* @param ELogOutput - All, Output Log or Screen
	*/
	void Log(ELogLevel LogLevel, FString Message, ELogOutput LogOutput = ELogOutput::ALL);
};


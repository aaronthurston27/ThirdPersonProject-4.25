// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/DataTable.h"
#include "TPPAbilityBase.h"
#include "Weapon/TPPWeaponBase.h"
#include "TPPHealthComponent.h"
#include "SpecialMove/TPPSpecialMove.h"
#include "SpecialMove/TPP_SPM_Defeated.h"
#include "SpecialMove/TPP_SPM_LedgeClimb.h"
#include "SpecialMove/TPP_SPM_LedgeHang.h"
#include "SpecialMove/TPP_SPM_WallRun.h"
#include "TPPPlayerCharacter.generated.h"

class ATPPPlayerController;
class UTPPMovementComponent;
class TPPHUD;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquipped, ATPPWeaponBase*, WeaponEquipped);

#pragma region Structs_And_Enums

/** Wall run direction */
UENUM(BlueprintType)
enum class EWallMovementState : uint8
{
	None,
	/** Player is clinging to wall */
	WallLedgeHang = 1,
	/** Running along wall to the players left */
	WallRunLeft = 2,
	/** Running along wall to the players right */
	WallRunRight = 3,
	/** Running upwards along wall */
	WallRunUp = 4,
	MAX UMETA(Hidden)
};

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

/** Struct defining hit react montages to play upon taking damage */
USTRUCT(Blueprintable)
struct FTPPHitReactions
{
	GENERATED_BODY()

	/** Hit react to play upon taking damage to the head */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* HeadHitReactMontage = nullptr;

	/** Hit react to play upon taking damage to upper body */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* UpperBodyHitReactMontage = nullptr;
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

	/** Offset of the camera when not zooming in. Should be adjusted so camera is not directly behind player character and obstructs view. */
	UPROPERTY(EditDefaultsOnly, Category = Camera)
	FVector HipAimCameraOffset = FVector(0.0f, 50.f, 20.0f);

	/** Length of camera boom when aiming from the hip. Use this instead of changing X value of camera offset to prevent collision issues. */
	UPROPERTY(EditDefaultsOnly, Category = Camera)
	float HipAimCameraArmLength = 293.0f;

	/** Offset of the camera when the player aims down the sights. */
	UPROPERTY(EditDefaultsOnly, Category = Camera)
	FVector ADSCameraOffset = FVector(0.0f, 60.0f, 47.0f);

	/** Length of camera boom when aiming down the sights. Use this instead of changing X value of camera offset to prevent collision issues. */
	UPROPERTY(EditDefaultsOnly, Category = Camera)
	float ADSCameraArmLength = 100.f;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_AnimationBlendSlot)
	EAnimationBlendSlot CurrentAnimationBlendSlot;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	UTPPHealthComponent* HealthComponent;

public:

	/** Returns the player's health component */
	UFUNCTION(BlueprintPure)
	UTPPHealthComponent* GetHealthComponent() const { return HealthComponent; }
		
public:
	ATPPPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	void Tick(float DeltaTime) override;

	void BeginPlay() override;

protected:

	// Required network scaffolding
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool ReplicateSubobjects(UActorChannel* channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

public:

	UPROPERTY(Replicated, EditDefaultsOnly, Category = "Character|Movement")
	float DefaultRotationRate;

	UPROPERTY(Replicated, EditDefaultsOnly, Category = "Character|Movement")
	float SprintRotationRate;

	UPROPERTY(Replicated, EditDefaultsOnly, Category = "Character|Movement")
	float ADSRotationRate;

protected:

	UPROPERTY(Transient)
	bool bWantsToSprint = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting, Transient)
	bool bIsSprinting = false;

public:

	UFUNCTION(BlueprintPure)
	bool CanSprint() const;

	UFUNCTION(BlueprintPure)
	bool IsSprinting() const { return bIsSprinting; }

	void SetWantsToSprint(bool bPlayerWantsToSprint);

	UFUNCTION(Server, Reliable)
	void ServerBeginSprint();

	void ServerBeginSprint_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerStopSprint();

	void ServerStopSprint_Implementation();

	UFUNCTION()
	void OnRep_IsSprinting();

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

	void AttemptToJump();

public:

	UFUNCTION(BlueprintPure)
	UTPPMovementComponent* GetTPPMovementComponent() const;

	UFUNCTION(BlueprintPure)
	ATPPPlayerController* GetTPPPlayerController() const;

protected:

	/** Ability being used */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_CurrentAbility)
	UTPPAbilityBase* CurrentAbility;

	/** Current special move to track */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_SpecialMove)
	UTPPSpecialMove* CurrentSpecialMove;

public:

	/** Ability to activate for special movement key */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UTPPAbilityBase> MovementAbilityClass;

public:

	void TryActivateAbility();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBeginMovementAbility();

	/** Gets current special move */
	UFUNCTION(BlueprintPure)
	UTPPSpecialMove* GetCurrentSpecialMove() const { return CurrentSpecialMove; }

	/** Gets current ability */
	UFUNCTION(BlueprintPure)
	UTPPAbilityBase* GetCurrentAbility() const { return CurrentAbility;  }

	UFUNCTION()
	void OnRep_CurrentAbility();

	UFUNCTION(NetMulticast, Reliable)
	void OnSpecialMoveEnded(UTPPSpecialMove* SpecialMove);

public:

	UFUNCTION(BlueprintCallable)
	void ExecuteSpecialMoveByClass(TSubclassOf<UTPPSpecialMove> SpecialMoveClass, bool bShouldInterrupCurrentMove = false);

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void ExecuteSpecialMove(UTPPSpecialMove* SpecialMove, bool bShouldInterruptCurrentMove = false);

protected:

	UFUNCTION()
	void OnRep_SpecialMove();

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

protected:

	UFUNCTION()
	void OnRep_AnimationBlendSlot();

private:

	/**
	* Log - prints a message to all the log outputs with a specific color
	* @param LogLevel {@see ELogLevel} affects color of log
	* @param FString the message for display
	* @param ELogOutput - All, Output Log or Screen
	*/
	void Log(ELogLevel LogLevel, FString Message, ELogOutput LogOutput = ELogOutput::ALL);

public:

	/** Name of the socket to attach weapons. Should usually be the name of the hand socket */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	FName WeaponAttachmentSocketName = TEXT("hand_rSocket");

protected:

	/** Currently equipped weapon */
	UPROPERTY(Transient, Replicated)
	ATPPWeaponBase* CurrentWeapon = nullptr;

	/** True if the player intends to aim down the sights when able */
	UPROPERTY(Transient)
	bool bWantsToAim = false;

	/** True if the player has begun aiming down the sights. Can be delayed by special moves */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_IsAiming)
	bool bIsAiming = false;

	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	FRotator AimRotationDelta = FRotator::ZeroRotator;

	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	FVector ControllerRelativeMovementSpeed = FVector::ZeroVector;

public:

	UPROPERTY(BlueprintAssignable)
	FOnWeaponEquipped OnWeaponEquipped;

	/** Equips a weapon to the player. */
	UFUNCTION(BlueprintCallable)
	void EquipWeapon(ATPPWeaponBase* WeaponEquipped);

	/** Returns the currently equipped weapon */
	UFUNCTION(BlueprintPure)
	ATPPWeaponBase* GetCurrentEquippedWeapon() const { return CurrentWeapon; }

	/** Tries to fire the currently equipped weapon */
	void TryToFireWeapon();

	/** Try to reload weapon */
	void TryToReloadWeapon();

	/** Set player's intent to begin aiming */
	UFUNCTION(BlueprintCallable)
	void SetPlayerWantsToAim(bool bIsTryingToAim);

	/** Returns true if the player is trying to aim */
	UFUNCTION(BlueprintPure)
	bool DoesPlayerWantToAim() const { return bWantsToAim; }

	/** Returns true if the player is currently aiming */
	UFUNCTION(BlueprintPure)
	bool IsPlayerAiming() const { return bIsAiming; }

	/** Returns true if the player is not inhibited by any mechanics/moves that block aiming */
	UFUNCTION(BlueprintPure)
	bool CanPlayerBeginAiming() const;

	/** Updates the speed of the character relative to the controller's rotation. */
	UFUNCTION(Server,Reliable)
	void UpdateControllerRelativeMovementSpeed();

	void UpdateControllerRelativeMovementSpeed_Implementation();

	/** Updates aim rotation delta for animation aim offset */
	UFUNCTION(Server,Reliable)
	void UpdateAimRotationDelta();

	void UpdateAimRotationDelta_Implementation();

protected:

	/** Have the player start aiming down the sights */
	UFUNCTION(Server, Reliable)
	void ServerBeginAiming();

	/** Stops the player from aiming down the sights */
	UFUNCTION(Server, Reliable)
	void ServerStopAiming();

	UFUNCTION()
	void OnRep_IsAiming();

public:

	/** Returns true if the character is alive */
	UFUNCTION(BlueprintPure)
	bool IsCharacterAlive() const;

	virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:

	/** Last damage event that hurt the player */
	FDamageEvent LastDamageEvent = FDamageEvent();

	/** Last actor that hurt the player */
	TWeakObjectPtr<AActor> LastDamageInstigator = nullptr;

	/** Hit react definitions */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Gameplay|Damage")
	FTPPHitReactions HitReactions;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UTPP_SPM_Defeated> DeathSpecialMove;

	/** Called when the player runs out of health */
	UFUNCTION()
	void OnPlayerHealthDepleted();

	/** Stop player movement and become inactive */
	void BecomeDefeated();

public:

	void OnDeath();

	void BeginRagdoll();

public:

	UFUNCTION(BlueprintPure)
	ATPPHUD* GetCharacterHUD() const;

protected:

	/** Set to true if the player has wall kicked while in the air. */
	UPROPERTY(Transient, BlueprintReadOnly)
	bool bHasWallKicked = false;

	/** Wallkick cooldown timer handle */
	UPROPERTY(Transient)
	FTimerHandle WallKickCooldownTimerHandle;

protected:

	/** Reset wallkick flag */
	UFUNCTION()
	void OnWallKickTimerExpired();

	bool CanPlayerWallKick(FHitResult& OutKickoffHitResult) const;

public:

	void DoWallKick(const FHitResult& WallKickHitResult);

public:

	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	float WallKickMaxDistance = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall", meta = (ClampMax = "1.0", UIMax = "1.0", ClampMin = "0.0", UIMin = "0.0"))
	float WallKickNormalMinDot = .65f;

	/** Velocity to add to the player when kicking off the wall. Multiplied by the direction of the walls normal. */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	FVector MinWallKickoffVelocity = FVector(700.0f, 700.0f, 500.0f);

	/** Time that wall kick will be disabled after using it */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	float WallKickCooldownTime = .9f;

	/** Distance in front of the player that the wall cling sweep capsule will be projected. */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	float WallClingSweepDistance = 10.0f;

	/** Time the player can cling to the wall before falling */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	float WallClingTime = 1.2f;

	/** Height below the ledge for the character to skip hanging and pull themselves up */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	float AutoLedgeClimbMaxHeight = 40.0f;

	/** Min height below the ledge of a wall to start a ledge hang. */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	float LedgeGrabMaxHeight = 175.0f;

	/** Vector to offset the player from the grab point when grabbing a ledge */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	FVector WallLedgeGrabOffset = FVector(0.0f, 0.0f, -20.0f);

	/** Minimum dot product of desired movement direction and wall normal to end hang */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	float EndHangInputDot = .8f;

	/** Minimum dot product of desired movement direction and wall normal to climg the ledge that is held onto. */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	float HangToClimbInputDot = .8f;

	/** Ledge climb special move to use when hanging from a ledge. */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	TSubclassOf<UTPP_SPM_LedgeHang> LedgeHangClass;

	/** Ledge climb special move to use when the player should climb over a ledge without hanging from it. */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	TSubclassOf<UTPP_SPM_LedgeClimb> AutoLedgeClimbClass;

	/** Wall run special move to use attaching to wall with a high ledge */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Movement|Wall")
	TSubclassOf<UTPP_SPM_WallRun> WallRunClass;

public:

	/** Returns true if the player has a valid wall to cling to */
	bool CanAttachToWall(FHitResult& WallHitResult, FVector& OutAttachPoint, float& WallLedgeHeight) const;

	/** Returns true if the player can climb up a wall ledge from the attach point */
	bool CanClimbUpLedge(const FHitResult& WallHitResult, const FVector& AttachPoint, FVector& ExitPoint);

protected:

	/** Wall cling state of the player */
	UPROPERTY(Transient, BlueprintReadOnly)
	EWallMovementState WallMovementState = EWallMovementState::None;

	/** True if player has wall climbed and is on cooldown until theey land */
	UPROPERTY(Transient, BlueprintReadOnly)
	bool bIsWallRunCooldownActive = false;

	/** Cached wall cling impact trace result */
	UPROPERTY(Transient)
	FHitResult WallTraceImpactResult;

	/** Cached wall climg attach point */
	UPROPERTY(Transient)
	FVector WallAttachPoint;

public:

	EWallMovementState GetWallMovementState() const { return WallMovementState; }

	void SetWallMovementState(EWallMovementState NewMovementState);

	void GetCurrentWallClimbProperties(FHitResult& TraceImpactResult, FVector& AttachPoint) const { TraceImpactResult = WallTraceImpactResult; AttachPoint = WallAttachPoint; }

protected:

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
};


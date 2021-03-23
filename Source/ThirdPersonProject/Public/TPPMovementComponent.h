// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TPPMovementComponent.generated.h"

UENUM()
enum class ECustomMovementMode : uint8 
{
	Sliding = 0,
};

/**
 * 
 */
UCLASS(Blueprintable)
class THIRDPERSONPROJECT_API UTPPMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	UTPPMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	UPROPERTY(EditDefaultsOnly, Category = "Character Movement|Movement Speed")
	float DefaultWalkSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Character Movement|Movement Speed")
	float SprintingSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Character Movement|Movement Speed")
	float ADSWalkSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Character Movement|Movement Speed")
	float CrouchingADSSpeed;

public:

	UPROPERTY(EditDefaultsOnly, Category = "CustomMovement|Sliding")
	float MinimumSlidingSpeed;

	// Speed that when friction causes velocity to go under will end slide.
	UPROPERTY(EditDefaultsOnly, Category = "CustomMovement|Sliding")
	float EndSlideSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "CustomMovement|Sliding")
	float SlidingFriction;

public:

	/** Friction (Air drag) value */
	UPROPERTY(EditDefaultsOnly, meta = (UIMin = "0.0", ClampMin = "0.0"), Category = "Character Movement|Air Movement")
	float AirFriction;

public:

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	void PhysSlide(float DeltaTime, int32 Iterations);

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

public:

	bool IsInCustomMovementMode(ECustomMovementMode MovementModeIndex) const { return MovementMode == EMovementMode::MOVE_Custom && CustomMovementMode == (uint8)MovementModeIndex; }

public:

	UPROPERTY(Transient)
	bool bWantsToSlide = false;

	UPROPERTY(Transient)
	bool bHasCharacterStartedSlide = false;

	UFUNCTION(BlueprintPure)
	bool DoesCharacterWantToSlide() const { return bWantsToSlide; }

	UFUNCTION(BlueprintPure)
	bool HasCharacterStartedSlide() const { return bHasCharacterStartedSlide; }

	bool CanSlide() const;

	UFUNCTION(BlueprintPure)
	bool IsSliding() const;

	void SlideStarted();
	
	void SlideEnded();

	virtual void UnCrouch(bool bClientSimulation) override;

	virtual bool CanCrouchInCurrentState() const override;

public:

	virtual bool IsMovingOnGround() const override;

	virtual float GetMaxBrakingDeceleration() const override;

	virtual float GetMaxSpeed() const override;

protected:

	UPROPERTY(Transient)
	float Cached2DMinimumSlidingSpeed;

	UPROPERTY(Transient)
	float CachedEndSlideSpeed;

	UPROPERTY(Transient)
	float CachedGroundFriction;

	UPROPERTY(Transient)
	float CachedBrakingDeceleration;

	UPROPERTY(Transient)
	float CachedMaxAirSpeed = MaxWalkSpeed;

public:

	UFUNCTION(Server, Reliable)
	void ServerSetOrientRotationToMovement(const bool bShouldOrientToMovement);

	UFUNCTION(NetMulticast, Reliable)
	void SetOrientRotationToMovement(const bool bShouldOrientToMovement);

	UFUNCTION(Server, Reliable)
	void ServerSetUseControllerDesiredRotation(const bool bShouldUseDesiredRotation);

	UFUNCTION(NetMulticast, Reliable)
	void SetUseControllerDesiredRotation(const bool bShouldUseDesiredRotation);

	UFUNCTION(Server, Reliable)
	void ServerAddRootMotionSource(const FRootMotionSource_ConstantForce& RootMotionSource);

	UFUNCTION(Server, Reliable)
	void ServerEndRootMotionSource(const FName& SourceName);

	UFUNCTION(Server, Reliable)
	void ServerOverrideCharacterVelocity(const FVector& NewVelocity);
	
};

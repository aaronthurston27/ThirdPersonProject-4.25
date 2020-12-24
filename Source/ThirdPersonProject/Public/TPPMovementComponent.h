// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TPPMovementComponent.generated.h"

UENUM()
enum class ECustomMovementMode : uint8 {
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

	UPROPERTY(EditDefaultsOnly, Category = "CustomMovement|Sliding")
	float MinimumSlidingSpeed;

	// Speed that when friction causes velocity to go under will end slide.
	UPROPERTY(EditDefaultsOnly, Category = "CustomMovement|Sliding")
	float EndSlideSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "CustomMovement|Sliding")
	float SlidingFriction;

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

public:

	virtual bool IsMovingOnGround() const override;

	virtual float GetMaxBrakingDeceleration() const override;

protected:

	UPROPERTY(Transient)
	float Cached2DMinimumSlidingSpeed;

	UPROPERTY(Transient)
	float CachedEndSlideSpeed;

	UPROPERTY(Transient)
	float CachedGroundFriction;

	UPROPERTY(Transient)
	float CachedBrakingDeceleration;
	
};

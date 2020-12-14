// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPMovementComponent.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPPMovementComponent::UTPPMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MinimumSlidingSpeed = MaxWalkSpeed + 150.0f;
	EndSlideSpeed = 200.f;
	SlidingFriction = 30.f;
	EndSlideSpeed = 0.0f;
	bUseSeparateBrakingFriction = true;
}

void UTPPMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	Cached2DMinimumSlidingSpeed = MinimumSlidingSpeed * MinimumSlidingSpeed;
	CachedEndSlideSpeed = EndSlideSpeed * EndSlideSpeed;
	MaxCustomMovementSpeed = 1000.f;
	EndSlideSpeed = 300.0f;
}

void UTPPMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UTPPMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	switch (MovementMode)
	{
	case EMovementMode::MOVE_Custom:
		if (CustomMovementMode == (uint8)ECustomMovementMode::Sliding)
		{
			return;
		}
	default:
		if (bWantsToSlide && PreviousMovementMode == EMovementMode::MOVE_Custom && PreviousCustomMode == (uint8)ECustomMovementMode::Sliding)
		{
			bWantsToSlide = false;
			SlideEnded();
		}
	}
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UTPPMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	ECustomMovementMode CurrentCustomMode = (ECustomMovementMode)(CustomMovementMode);
	switch (CurrentCustomMode)
	{
		case ECustomMovementMode::Sliding:
			PhysSlide(DeltaTime, Iterations);
			break;
	}
}

void UTPPMovementComponent::PhysSlide(float DeltaTime, int32 Iterations)
{
	if (!IsInCustomMovementMode(ECustomMovementMode::Sliding))
	{
		bWantsToSlide = false;
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}

	if (!bWantsToSlide)
	{
		SetMovementMode(EMovementMode::MOVE_Walking);
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}

	const float MaxSpeed = GetMaxSpeed();

	PhysWalking(DeltaTime, Iterations);
}

void UTPPMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	if (bWantsToSlide)
	{
		if (!CanSlide())
		{
			bWantsToSlide = false;
		}
		else if (!IsInCustomMovementMode(ECustomMovementMode::Sliding))
		{
			SetMovementMode(EMovementMode::MOVE_Custom, (uint8)ECustomMovementMode::Sliding);
		}
	}
}

bool UTPPMovementComponent::CanSlide() const
{
	const float Velocity2D = Velocity.SizeSquared2D();
	const bool bIsSliding = IsSliding();
	UE_LOG(LogTemp, Warning, TEXT("Slide Speed: %f, Is: %d"), Velocity2D, (int)bIsSliding);
	// If currently sliding, end slide when threshold speed reached
	return (IsMovingOnGround() || IsInCustomMovementMode(ECustomMovementMode::Sliding)) && Velocity2D >= (!bIsSliding ? Cached2DMinimumSlidingSpeed : CachedEndSlideSpeed);
}

void UTPPMovementComponent::SlideStarted()
{
	if (!IsCrouching() && bWantsToSlide)
	{
		Crouch();
		// Set wants to crouch to true since we want to move to crouching when finished.
		bWantsToCrouch = true;
		CachedBrakingDeceleration = BrakingDecelerationWalking;
		CachedGroundFriction = GroundFriction;

		BrakingFrictionFactor = 1.0f;
		BrakingFriction = SlidingFriction;
		GroundFriction = 1.f;
		BrakingDecelerationWalking = 0.0;

		MovementState.bCanJump = false;

		ATPPPlayerCharacter* TPPCharacter = Cast<ATPPPlayerCharacter>(CharacterOwner);
		if (TPPCharacter)
		{
			TPPCharacter->OnStartSlide();
		}
	}
}

void UTPPMovementComponent::SlideEnded()
{
	BrakingDecelerationWalking = CachedBrakingDeceleration;
	BrakingFriction = 1.f;
	BrakingFrictionFactor = 2.0f;
	GroundFriction = CachedGroundFriction;

	MovementState.bCanJump = true;

	if (!bWantsToCrouch)
	{
		UnCrouch(false);
	}

	ATPPPlayerCharacter* TPPCharacter = Cast<ATPPPlayerCharacter>(CharacterOwner);
	if (TPPCharacter)
	{
		TPPCharacter->OnEndSlide();
	}
}

bool UTPPMovementComponent::IsSliding() const
{
	return IsInCustomMovementMode(ECustomMovementMode::Sliding);
}

void UTPPMovementComponent::UnCrouch(bool bClientSimulation)
{
	if (!IsSliding())
	{
		Super::UnCrouch(bClientSimulation);
	}
}

bool UTPPMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsSliding();
}

float UTPPMovementComponent::GetMaxBrakingDeceleration() const
{
	switch (MovementMode)
	{
	case EMovementMode::MOVE_Custom:
		if (CustomMovementMode == (uint8)ECustomMovementMode::Sliding)
		{
			return .8f;
		}
	}

	return Super::GetMaxBrakingDeceleration();
}
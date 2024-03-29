// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPPMovementComponent::UTPPMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MinimumSlidingSpeed = MaxWalkSpeed + 150.0f;
	EndSlideSpeed = 200.f;
	SlidingFriction = .8f;
	bUseSeparateBrakingFriction = true;

	DefaultWalkSpeed = 400.f;
	ADSWalkSpeed = 250.f;
	SprintingSpeed = 1150.f;
	MaxWalkSpeedCrouched = 250.f;
	CrouchingADSSpeed = 200.f;

	AirFriction = .5f;
}

void UTPPMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	Cached2DMinimumSlidingSpeed = MinimumSlidingSpeed * MinimumSlidingSpeed;
	CachedEndSlideSpeed = EndSlideSpeed * EndSlideSpeed;
	CachedBrakingDeceleration = BrakingDecelerationWalking;
	CachedGroundFriction = GroundFriction;
	MaxWalkSpeed = DefaultWalkSpeed;
	MaxCustomMovementSpeed = 1000.f;
	EndSlideSpeed = 300.0f;

	bWantsToSlide = false;
	bHasCharacterStartedSlide = false;
}

void UTPPMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (MovementMode == EMovementMode::MOVE_Falling)
	{
		// If the player decelerates lateraly while in the air, decrease their max air speed to prevent speeding back up to their original speed.
		// Momentum should be preserved in the air unless the player decelerates through directional influence.
		CachedMaxAirSpeed = FMath::Max(Velocity.Size2D(), MaxWalkSpeed);
	}
}

void UTPPMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (PreviousMovementMode == EMovementMode::MOVE_Custom && PreviousCustomMode == (uint8)ECustomMovementMode::Sliding)
	{
		bWantsToSlide = false;
		SlideEnded();
	}

	switch (MovementMode)
	{
	case EMovementMode::MOVE_Custom:
		if (CustomMovementMode == (uint8)ECustomMovementMode::Sliding)
		{
			return;
		}
	case EMovementMode::MOVE_Falling:
		BrakingFriction = AirFriction;
		CachedMaxAirSpeed = Velocity.Size2D();
		break;
	case EMovementMode::MOVE_Walking:
		BrakingFriction = 1.0;
		break;
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
		else if (!IsInCustomMovementMode(ECustomMovementMode::Sliding) && IsMovingOnGround())
		{
			SetMovementMode(EMovementMode::MOVE_Custom, (uint8)ECustomMovementMode::Sliding);
		}
	}
}

bool UTPPMovementComponent::CanSlide() const
{
	const float Velocity2D = Velocity.SizeSquared2D();
	const bool bIsSliding = IsSliding();
	return Velocity2D >= (!bIsSliding ? Cached2DMinimumSlidingSpeed : CachedEndSlideSpeed);
}

void UTPPMovementComponent::SlideStarted()
{
	if (!IsCrouching() && bWantsToSlide)
	{
		Crouch();
		// Set wants to crouch to true since we want to move to crouching when finished.
		bWantsToCrouch = true;

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

		bHasCharacterStartedSlide = true;
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

	bHasCharacterStartedSlide = false;
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

bool UTPPMovementComponent::CanCrouchInCurrentState() const
{
	return (CanEverCrouch() && IsMovingOnGround()) && UpdatedComponent && !UpdatedComponent->IsSimulatingPhysics();
}

bool UTPPMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsSliding();
}

float UTPPMovementComponent::GetMaxSpeed() const
{
	ATPPPlayerCharacter* TPPCharacter = Cast<ATPPPlayerCharacter>(CharacterOwner);
	if (!TPPCharacter)
	{
		return 0.0f;
	}

	if (MovementMode == EMovementMode::MOVE_Falling)
	{
		return CachedMaxAirSpeed;
	}

	if (TPPCharacter->IsPlayerAiming())
	{
		return IsCrouching() ? CrouchingADSSpeed : ADSWalkSpeed;
	}
	else if (TPPCharacter->IsSprinting())
	{
		return SprintingSpeed;
	}

	return Super::GetMaxSpeed();

}

void UTPPMovementComponent::ServerSetOrientRotationToMovement_Implementation(const bool bShouldOrientToMovement)
{
	bOrientRotationToMovement = bShouldOrientToMovement;
	SetOrientRotationToMovement(bShouldOrientToMovement);
}

void UTPPMovementComponent::SetOrientRotationToMovement_Implementation(const bool bShouldOrientToMovement)
{
	bOrientRotationToMovement = bShouldOrientToMovement;
}

void UTPPMovementComponent::ServerSetUseControllerDesiredRotation_Implementation(const bool bShouldUseDesiredRotation)
{
	bUseControllerDesiredRotation = bShouldUseDesiredRotation;
	SetUseControllerDesiredRotation(bShouldUseDesiredRotation);
}

void UTPPMovementComponent::SetUseControllerDesiredRotation_Implementation(const bool bShouldUseDesiredRotation)
{
	bUseControllerDesiredRotation = bShouldUseDesiredRotation;
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

void UTPPMovementComponent::ServerAddRootMotionSource_Implementation(const FRootMotionSource_ConstantForce& RootMotionSource)
{
	TSharedPtr<FRootMotionSource_ConstantForce> RootMotionSourcePtr = MakeShared<FRootMotionSource_ConstantForce>(RootMotionSource);
	ApplyRootMotionSource(RootMotionSourcePtr);
}

void UTPPMovementComponent::ServerEndRootMotionSource_Implementation(const FName& SourceName)
{
	RemoveRootMotionSource(SourceName);
}

void UTPPMovementComponent::ServerOverrideCharacterVelocity_Implementation(const FVector& NewVelocity)
{
	Velocity = NewVelocity;
}
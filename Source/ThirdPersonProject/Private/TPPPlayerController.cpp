// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPPlayerController.h"

ATPPPlayerController::ATPPPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsMovementInputEnabled = true;
}

void ATPPPlayerController::BeginPlay()
{
	bIsMovementInputEnabled = true;
}

void ATPPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAxis("MoveForward", this, &ATPPPlayerController::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &ATPPPlayerController::MoveRight);
}

void ATPPPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATPPPlayerController::MoveForward(float Value)
{
	// find out which way is forward
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// get forward vector
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	DesiredMovementDirection.X = Value;
	if (bIsMovementInputEnabled)
	{
		GetPawn()->AddMovementInput(Direction, Value);
	}
}

void ATPPPlayerController::MoveRight(float Value)
{
	// find out which way is right
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// get right vector 
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	DesiredMovementDirection.Y = Value;

	// add movement in that direction
	if (bIsMovementInputEnabled)
	{
		GetPawn()->AddMovementInput(Direction, Value);
	}
}

void ATPPPlayerController::SetMovementInputEnabled(bool bIsEnabled)
{
	bIsMovementInputEnabled = bIsEnabled;
}

FRotator ATPPPlayerController::GetRelativeControllerMovementDirection() const
{
	const FRotator DesiredDirection = GetDesiredMovementDirection().ToOrientationRotator();
	const FRotator ControlRot = GetControlRotation();

	// Add the desired movement rotation to the player controller rotation instead of world space.
	return FRotator(0.0f, DesiredDirection.Yaw + ControlRot.Yaw, 0.0f);
}
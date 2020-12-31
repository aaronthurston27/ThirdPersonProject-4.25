// Fill out your copyright notice in the Description page of Project Settings.

#include "TPPPlayerController.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

ATPPPlayerController::ATPPPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsMovementInputEnabled = true;
}

void ATPPPlayerController::BeginPlay()
{
	CachedOwnerCharacter = GetOwnerCharacter();
	bIsMovementInputEnabled = true;
}

void ATPPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAxis("MoveForward", this, &ATPPPlayerController::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &ATPPPlayerController::MoveRight);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ATPPPlayerController::OnJumpPressed);
	InputComponent->BindAction("Jump", IE_Released, this, &ATPPPlayerController::OnJumpReleased);

	InputComponent->BindAxis("Turn", this, &ATPPPlayerController::AddYawInput);
	InputComponent->BindAxis("TurnRate", this, &ATPPPlayerController::TurnRate);
	InputComponent->BindAxis("LookUp", this, &ATPPPlayerController::AddPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &ATPPPlayerController::LookUpRate);

	InputComponent->BindAction("LockOn", IE_Pressed, this, &ATPPPlayerController::OnLockOnPressed);

	InputComponent->BindAction("MovementAbility", IE_Pressed, this, &ATPPPlayerController::OnMovementAbilityPressed);
	InputComponent->BindAction("Sprint", IE_Pressed, this, &ATPPPlayerController::OnSprintPressed);
	InputComponent->BindAction("Sprint", IE_Released, this, &ATPPPlayerController::OnSprintReleased);
	InputComponent->BindAction("Crouch", IE_Pressed, this, &ATPPPlayerController::OnCrouchPressed);
	InputComponent->BindAction("Crouch", IE_Released, this, &ATPPPlayerController::OnCrouchReleased);

	InputComponent->BindAxis("FireWeapon", this, &ATPPPlayerController::HandleWeaponFireAxis);
	InputComponent->BindAction("ADS", IE_Pressed, this, &ATPPPlayerController::OnAimWeaponPressed);
	InputComponent->BindAction("ADS", IE_Released, this, &ATPPPlayerController::OnAimWeaponReleased);
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

void ATPPPlayerController::LookUpRate(float value)
{

}

void ATPPPlayerController::TurnRate(float value)
{

}

void ATPPPlayerController::OnLockOnPressed()
{
	CachedOwnerCharacter->ResetCameraToPlayerRotation();
}

void ATPPPlayerController::OnJumpPressed()
{
	CachedOwnerCharacter->Jump();
}

void ATPPPlayerController::OnJumpReleased()
{
	CachedOwnerCharacter->StopJumping();
}

void ATPPPlayerController::OnSprintPressed()
{
	CachedOwnerCharacter->BeginSprint();
}

void ATPPPlayerController::OnSprintReleased()
{
	CachedOwnerCharacter->StopSprint();
}

void ATPPPlayerController::OnCrouchPressed()
{
	CachedOwnerCharacter->Crouch(false);
}

void ATPPPlayerController::OnCrouchReleased()
{
	CachedOwnerCharacter->UnCrouch(false);
}

void ATPPPlayerController::OnMovementAbilityPressed()
{
	CachedOwnerCharacter->BeginMovementAbility();
}

void ATPPPlayerController::SetMovementInputEnabled(bool bIsEnabled)
{
	bIsMovementInputEnabled = bIsEnabled;
}

void ATPPPlayerController::HandleWeaponFireAxis(float Value)
{
	if (Value >= FireWeaponThreshold && CachedOwnerCharacter)
	{
		CachedOwnerCharacter->TryToFireWeapon();
	}
}

void ATPPPlayerController::OnAimWeaponPressed()
{
	CachedOwnerCharacter->SetPlayerWantsToAim(true);
}

void ATPPPlayerController::OnAimWeaponReleased()
{
	CachedOwnerCharacter->SetPlayerWantsToAim(false);
}

FRotator ATPPPlayerController::GetRelativeControllerMovementRotation() const
{
	const FRotator DesiredDirection = GetDesiredMovementDirection().ToOrientationRotator();
	const FRotator ControlRot = GetControlRotation();

	// Add the desired movement rotation to the player controller rotation instead of world space.
	return FRotator(0.0f, DesiredDirection.Yaw + ControlRot.Yaw, 0.0f);
}

ATPPPlayerCharacter* ATPPPlayerController::GetOwnerCharacter()
{
	return Cast<ATPPPlayerCharacter>(GetPawn());
}
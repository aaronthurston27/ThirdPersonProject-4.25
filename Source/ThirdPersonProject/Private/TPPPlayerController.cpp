// Fill out your copyright notice in the Description page of Project Settings.

#include "TPPPlayerController.h"
#include "TPPWeaponBase.h"
#include "TPPWeaponFirearm.h"
#include "TPPAimProperties.h"
#include "TPPGameInstance.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

ATPPPlayerController::ATPPPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsMovementInputEnabled = true;
}

void ATPPPlayerController::BeginPlay()
{
	CachedOwnerCharacter = GetOwnerCharacter();
	bIsMovementInputEnabled = true;
	DesiredControlRotation = GetControlRotation();
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

	InputComponent->BindAction("MovementAbility", IE_Pressed, this, &ATPPPlayerController::OnMovementAbilityPressed);
	InputComponent->BindAction("Sprint", IE_Pressed, this, &ATPPPlayerController::OnSprintPressed);
	InputComponent->BindAction("Sprint", IE_Released, this, &ATPPPlayerController::OnSprintReleased);
	InputComponent->BindAction("Crouch", IE_Pressed, this, &ATPPPlayerController::OnCrouchPressed);
	InputComponent->BindAction("Crouch", IE_Released, this, &ATPPPlayerController::OnCrouchReleased);

	InputComponent->BindAxis("FireWeapon", this, &ATPPPlayerController::HandleWeaponFireAxis);
	InputComponent->BindAction("ADS", IE_Pressed, this, &ATPPPlayerController::OnAimWeaponPressed);
	InputComponent->BindAction("ADS", IE_Released, this, &ATPPPlayerController::OnAimWeaponReleased);
	InputComponent->BindAction("Reload", IE_Pressed, this, &ATPPPlayerController::OnReloadPressed);
}

void ATPPPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATPPPlayerController::UpdateRotation(float DeltaTime)
{
	if (!PlayerCameraManager)
	{
		return;
	}

	// Calculate Delta to be applied on ViewRotation
	FRotator DeltaRot(RotationInput);
	
	const UTPPGameInstance* GameInstance = UTPPGameInstance::Get();
	const UTPPAimProperties* AimProperties = GameInstance ? GameInstance->GetAimProperties() : nullptr;

	float CurrentPitch = GetControlRotation().Pitch;
	if (CurrentPitch >= 180.0f)
	{
		CurrentPitch -= 360.0f;
	}

	const bool bIsRecoilIncreasing = TargetCameraRecoil.Pitch > 0.0f;
	if (bIsRecoilIncreasing && CurrentPitch < PlayerCameraManager->ViewPitchMax)
	{
		CurrentCameraRecoil.Pitch += AimProperties->WeaponRecoilAccumulationPerFrame * DeltaTime;
		CurrentCameraRecoil.Pitch = FMath::Min(CurrentCameraRecoil.Pitch, TargetCameraRecoil.Pitch);
	}

	// Add recoil compensation if pulling the camera down while recoil is increasing.
	if (bIsRecoilIncreasing && DeltaRot.Pitch < 0.0f)
	{
		const float Compensation = AimProperties->RecoilCompensationScale * DeltaRot.Pitch;
		TargetCameraRecoil.Pitch = FMath::Max(TargetCameraRecoil.Pitch + Compensation, 0.0f);
		CurrentCameraRecoil.Pitch = FMath::Max(CurrentCameraRecoil.Pitch + Compensation, 0.0f);
		DeltaRot.Pitch = 0.0f;
	}

	DesiredControlRotation += DeltaRot;

	FRotator ViewRotation = DesiredControlRotation + CurrentCameraRecoil;
	PlayerCameraManager->ProcessViewRotation(DeltaTime, ViewRotation, DeltaRot);

	AActor* ViewTarget = GetViewTarget();
	SetControlRotation(ViewRotation);

	APawn* const P = GetPawnOrSpectator();
	if (P)
	{
		P->FaceRotation(ViewRotation, DeltaTime);
	}
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
	CachedOwnerCharacter->SetWantsToSprint(true);
}

void ATPPPlayerController::OnSprintReleased()
{
	CachedOwnerCharacter->SetWantsToSprint(false);
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

void ATPPPlayerController::OnReloadPressed()
{
	CachedOwnerCharacter->TryToReloadWeapon();
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

FVector ATPPPlayerController::GetControllerRelativeForwardVector() const
{
	const FRotator CurrentRotation = GetControlRotation();
	const FRotationMatrix RotMatrix = FRotationMatrix(CurrentRotation);

	return RotMatrix.GetUnitAxis(EAxis::X);
}

FVector ATPPPlayerController::GetControllerRelativeRightVector() const
{
	const FRotator CurrentRotation = GetControlRotation();
	const FRotationMatrix RotMatrix = FRotationMatrix(CurrentRotation);

	return RotMatrix.GetUnitAxis(EAxis::Y);
}

FVector ATPPPlayerController::GetControllerRelativeUpVector() const
{
	const FRotator CurrentRotation = GetControlRotation();
	const FRotationMatrix RotMatrix = FRotationMatrix(CurrentRotation);

	return RotMatrix.GetUnitAxis(EAxis::Z);
}

void ATPPPlayerController::AddCameraRecoil(const float RecoilToAdd)
{
	TargetCameraRecoil.Pitch += RecoilToAdd;
}

void ATPPPlayerController::ResetCameraRecoil()
{
	TargetCameraRecoil = FRotator::ZeroRotator;
	CurrentCameraRecoil = FRotator::ZeroRotator;
	DesiredControlRotation = GetControlRotation();
}
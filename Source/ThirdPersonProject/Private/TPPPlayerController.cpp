// Fill out your copyright notice in the Description page of Project Settings.

#include "TPPPlayerController.h"
#include "Weapon/TPPWeaponBase.h"
#include "Weapon/TPPWeaponFirearm.h"
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

	InputComponent->BindAction("Pause", IE_Pressed, this, &ATPPPlayerController::OnPausePressed);
}

void ATPPPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickKeyHoldTimers(DeltaTime);
}

void ATPPPlayerController::TickKeyHoldTimers(float DeltaTime)
{
	TMap<EPlayerInputAction, float> HoldTimerCopy = KeyHoldTimers;
	for (auto& HoldTimer : HoldTimerCopy)
	{
		const float TimerValue = HoldTimer.Value - DeltaTime;
		if (TimerValue <= 0.0f)
		{
			KeyHoldTimers.Remove(HoldTimer.Key);
		}
		else
		{
			KeyHoldTimers[HoldTimer.Key] = TimerValue;
		}
	}
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

	const FRotator CurrentRotation = GetControlRotation();
	const bool bIsRecoilIncreasing = TargetCameraRecoil.Pitch > 0.0f;
	if (bIsRecoilIncreasing)
	{
		if (!FMath::IsNearlyEqual(CurrentRotation.Pitch, PlayerCameraManager->ViewPitchMax))
		{
			CurrentCameraRecoil.Pitch = FMath::Lerp(CurrentCameraRecoil.Pitch, TargetCameraRecoil.Pitch, AimProperties->AimRecoilInterpAlpha);
		}

		// Add recoil compensation if pulling the camera down while recoil is increasing.
		if (DeltaRot.Pitch < 0.0f)
		{
			const float NewRecoil = TargetCameraRecoil.Pitch - CurrentCameraRecoil.Pitch;
			CurrentCameraRecoil.Pitch = FMath::Max(0.0f, DeltaRot.Pitch + CurrentCameraRecoil.Pitch);
			TargetCameraRecoil.Pitch = FMath::Max(TargetCameraRecoil.Pitch + DeltaRot.Pitch, NewRecoil);
			if (!FMath::IsNearlyZero(CurrentCameraRecoil.Pitch, .01f))
			{
				DeltaRot.Pitch = 0.0f;
			}
		}
	}
	else if (CurrentCameraRecoil.Pitch > TargetCameraRecoil.Pitch)
	{
		CurrentCameraRecoil.Pitch = FMath::Lerp(CurrentCameraRecoil.Pitch, TargetCameraRecoil.Pitch, AimProperties->AimRecoilRestorationInterpAlpha);
		if (FMath::IsNearlyZero(CurrentCameraRecoil.Pitch, .2f))
		{
			CurrentCameraRecoil = FRotator::ZeroRotator;
		}
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
	CachedOwnerCharacter->TryJump();
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

void ATPPPlayerController::OnPausePressed()
{
#if !WITH_EDITOR

	FGenericPlatformMisc::RequestExit(false);

#endif !WITH_EDITOR
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

FVector ATPPPlayerController::GetControllerRelativeForwardVector(bool bIncludeVertical) const
{
	const FRotator CurrentRotation = GetControlRotation();
	const FRotationMatrix RotMatrix = FRotationMatrix(CurrentRotation);

	FVector ForwardVec = RotMatrix.GetUnitAxis(EAxis::X);
	if (!bIncludeVertical)
	{
		ForwardVec.Z = 0.0f;
	}

	return ForwardVec;
}

FVector ATPPPlayerController::GetControllerRelativeRightVector(bool bIncludeVertical) const
{
	const FRotator CurrentRotation = GetControlRotation();
	const FRotationMatrix RotMatrix = FRotationMatrix(CurrentRotation);

	FVector RightVec = RotMatrix.GetUnitAxis(EAxis::Y);
	if (!bIncludeVertical)
	{
		RightVec.Z = 0.0f;
	}

	return RightVec;
}

FVector ATPPPlayerController::GetControllerRelativeUpVector() const
{
	const FRotator CurrentRotation = GetControlRotation();
	const FRotationMatrix RotMatrix = FRotationMatrix(CurrentRotation);

	return RotMatrix.GetUnitAxis(EAxis::Z);
}

void ATPPPlayerController::AddCameraRecoil(const float RecoilToAdd)
{
	if (PlayerCameraManager && !FMath::IsNearlyEqual(GetControlRotation().Pitch, PlayerCameraManager->ViewPitchMax))
	{
		// Add to the existing pitch if we have begun firing while the recoil was being restored.
		if (CurrentCameraRecoil.Pitch > TargetCameraRecoil.Pitch)
		{
			TargetCameraRecoil = CurrentCameraRecoil;
		}
		TargetCameraRecoil.Pitch += RecoilToAdd;
	}
}

void ATPPPlayerController::ResetCameraRecoil()
{
	TargetCameraRecoil = FRotator::ZeroRotator;
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "TPP_SPM_DodgeRoll.h"
#include "TPPMovementComponent.h"
#include "TPPPlayerController.h"
#include "DrawDebugHelpers.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPP_SPM_DodgeRoll::UTPP_SPM_DodgeRoll(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDisablesMovementInput = true;
	bDisablesAiming = true;
	bDisablesJump = true;
	bDisablesSprint = true;
	bDisablesCrouch = true;

	RollSpeed = 300.f;
	RollRampDownSpeed = 150.f;
}

void UTPP_SPM_DodgeRoll::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	CharacterMovementComponent = OwningCharacter->GetTPPMovementComponent();
	CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
	CharacterMovementComponent->bOrientRotationToMovement = false;

	OwningCharacter->UnCrouch(false);

	ATPPPlayerController* PlayerController = Cast<ATPPPlayerController>(OwningCharacter->GetController());
	if (PlayerController)
	{
		const FRotator RollRotation = PlayerController->GetRelativeControllerMovementRotation();
		CachedRollDirection = RollRotation.Vector();
		OwningCharacter->SetActorRelativeRotation(RollRotation);
	}

	if (AnimMontage)
	{
		SetAnimRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
		PlayAnimMontage(AnimMontage);
	}
}

void UTPP_SPM_DodgeRoll::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const FVector RollVelocity = CachedRollDirection * RollSpeed;
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->Velocity = RollVelocity;
	}

}

void UTPP_SPM_DodgeRoll::EndSpecialMove_Implementation()
{
	if (OwningCharacter)
	{
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);
		SetAnimRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	}

	CharacterMovementComponent->bOrientRotationToMovement = true;
	CharacterMovementComponent = nullptr;

	Super::EndSpecialMove_Implementation();
}

void UTPP_SPM_DodgeRoll::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (AnimMontage == Montage)
	{
		ATPPPlayerController* PlayerController = OwningCharacter->GetTPPPlayerController();
		if (PlayerController)
		{
			const FVector CurrentDesiredMovementDirection = PlayerController->GetDesiredMovementDirection();

			FVector EndVelocity = FVector::ZeroVector;
			// If no input detected, decelerate the roll in its current direction.
			if (CurrentDesiredMovementDirection.IsNearlyZero())
			{
				EndVelocity = CachedRollDirection * RollRampDownSpeed;
			}
			// If the player desires to move in a direction coming out of the roll, accelerate in that direction.
			else
			{
				const FVector RelativeControllerDesiredDirection = PlayerController->GetRelativeControllerMovementRotation().Vector();
				EndVelocity = RelativeControllerDesiredDirection * CharacterMovementComponent->MaxWalkSpeed;
			}
			CharacterMovementComponent->Velocity = EndVelocity;
		}
		EndSpecialMove();
	}

	Super::OnMontageEnded(Montage, bInterrupted);
}

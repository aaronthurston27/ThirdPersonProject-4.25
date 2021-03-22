// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecialMove/TPP_SPM_DodgeRoll.h"
#include "TPPMovementComponent.h"
#include "TPPPlayerController.h"
#include "DrawDebugHelpers.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPP_SPM_DodgeRoll::UTPP_SPM_DodgeRoll(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDisablesMovementInput = true;
	bDisablesJump = true;
	bDisablesSprint = true;
	bDisablesCrouch = true;
	bDisablesWeaponUseOnStart = true;
	bDisablesCharacterRotation = true;
	bInterruptsReload = true;

	RollSpeed = 300.f;
	RollRampDownSpeed = 150.f;
}

void UTPP_SPM_DodgeRoll::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTPP_SPM_DodgeRoll, CachedRollDirection);
	DOREPLIFETIME(UTPP_SPM_DodgeRoll, CharacterMovementComponent);
}

void UTPP_SPM_DodgeRoll::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	CharacterMovementComponent = OwningCharacter->GetTPPMovementComponent();
	CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
	CharacterMovementComponent->ServerSetOrientRotationToMovement(false);
	CharacterMovementComponent->ServerSetUseControllerDesiredRotation(false);

	OwningCharacter->UnCrouch(false);

	ATPPPlayerController* PlayerController = Cast<ATPPPlayerController>(OwningCharacter->GetController());
	if (PlayerController)
	{
		const FRotator RollRotation = PlayerController->GetControllerRelativeMovementRotation();
		CachedRollDirection = RollRotation.Vector();
		OwningCharacter->ServerSetCharacterRotation(RollRotation);
	}

	if (AnimMontage)
	{
		OwningCharacter->ServerSetAnimRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
		OwningCharacter->ServerPlaySpecialMoveMontage(AnimMontage);
	}
}

void UTPP_SPM_DodgeRoll::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const FVector RollVelocity = FVector(CachedRollDirection * RollSpeed);
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->Velocity = FVector(RollVelocity.X, RollVelocity.Y, CharacterMovementComponent->Velocity.Z);
	}

}

void UTPP_SPM_DodgeRoll::EndSpecialMove_Implementation()
{
	if (OwningCharacter)
	{
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);
		OwningCharacter->ServerSetAnimRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	}

	CharacterMovementComponent->ServerSetOrientRotationToMovement(true);
	CharacterMovementComponent = nullptr;

	Super::EndSpecialMove_Implementation();
}

void UTPP_SPM_DodgeRoll::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (AnimMontage == Montage)
	{
		ATPPPlayerController* PlayerController = OwningCharacter->GetTPPPlayerController();
		if (PlayerController && OwningCharacter->GetTPPMovementComponent()->IsMovingOnGround())
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
				const FVector RelativeControllerDesiredDirection = PlayerController->GetControllerRelativeMovementRotation().Vector();
				EndVelocity = RelativeControllerDesiredDirection * RollSpeed * (RelativeControllerDesiredDirection | CachedRollDirection);
			}
			CharacterMovementComponent->Velocity = EndVelocity;
		}
		EndSpecialMove();
	}

	Super::OnMontageEnded(Montage, bInterrupted);
}

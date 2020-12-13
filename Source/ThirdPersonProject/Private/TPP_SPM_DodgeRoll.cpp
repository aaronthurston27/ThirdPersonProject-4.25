// Fill out your copyright notice in the Description page of Project Settings.


#include "TPP_SPM_DodgeRoll.h"
#include "TPPMovementComponent.h"
#include "TPPPlayerController.h"
#include "ThirdPersonProject/ThirdPersonProjectCharacter.h"

UTPP_SPM_DodgeRoll::UTPP_SPM_DodgeRoll(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDisablesMovementInput = true;
	bDisablesAiming = true;
	bDisablesJump = true;
	bDisablesSprint = true;

	RollSpeed = 300.f;
	RollRampDownSpeed = 150.f;
}

void UTPP_SPM_DodgeRoll::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	ATPPPlayerController* PlayerController = Cast<ATPPPlayerController>(OwningCharacter->GetController());
	if (PlayerController)
	{
		const FRotator RollRotation = PlayerController->GetRelativeControllerMovementDirection();
		CachedRollDirection = RollRotation.Vector();
		OwningCharacter->SetActorRelativeRotation(RollRotation);
	}

	CharacterMovementComponent = OwningCharacter->GetTPPMovementComponent();
	CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);

	
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
			const FVector CurrentDesiredMovementDirection = PlayerController->GetRelativeControllerMovementDirection().Vector();
			FVector EndVelocity = FVector::ZeroVector;
			if (CurrentDesiredMovementDirection.IsNearlyZero())
			{
				EndVelocity = CachedRollDirection * RollRampDownSpeed;
			}
			else
			{
				EndVelocity = CurrentDesiredMovementDirection * CharacterMovementComponent->MaxWalkSpeed;
				CharacterMovementComponent->Velocity = EndVelocity;
			}
		}
		EndSpecialMove();
	}

	Super::OnMontageEnded(Montage, bInterrupted);
}

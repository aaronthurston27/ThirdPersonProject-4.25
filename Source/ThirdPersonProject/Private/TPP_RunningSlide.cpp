// Fill out your copyright notice in the Description page of Project Settings.


#include "TPP_RunningSlide.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "TPPMovementComponent.h"

UTPP_RunningSlide::UTPP_RunningSlide(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDurationBased = false;
}

void UTPP_RunningSlide::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void UTPP_RunningSlide::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();
}

void UTPP_RunningSlide::EndSpecialMove_Implementation()
{
	Super::EndSpecialMove_Implementation();
}

void UTPP_RunningSlide::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage)
	{
		switch (CurrentSlideState)
		{
		case ESlideState::Intro:
			if (Montage == IntroSlideAnim)
			{
				CurrentSlideState = ESlideState::Sliding;
				PlayAnimMontage(SlideLoopAnim);
			}
			break;
		case ESlideState::Exiting:
			if (Montage == ExitAnim)
			{
				EndSpecialMove();
			}
			break;
		}
	}
}

void UTPP_RunningSlide::OnCharacterMovementModeChanged(ACharacter* Character, EMovementMode PreviousMode, uint8 CustomModeType)
{
	if (Character == OwningCharacter)
	{
		if (Character->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
		{
			EndSpecialMove();
		}
	}
}
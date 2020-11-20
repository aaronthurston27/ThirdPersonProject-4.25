// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPSpecialMove.h"
#include "ThirdPersonProject/ThirdPersonProjectCharacter.h"

UTPPSpecialMove::UTPPSpecialMove(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

UTPPSpecialMove::~UTPPSpecialMove()
{

}

void UTPPSpecialMove::Tick(float DeltaSeconds)
{
	if (bDurationBased)
	{
		TimeRemaining -= DeltaSeconds;

		if (TimeRemaining <= 0.0f)
		{
			EndSpecialMove();
		}
	}
}

void UTPPSpecialMove::BeginSpecialMove_Implementation()
{
	TimeRemaining = Duration;

	UAnimInstance* AnimInstance = OwningCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.AddDynamic(this, &UTPPSpecialMove::OnMontageEnded);
	}

	if (bDisablesMovementInput)
	{
		OwningCharacter->SetMovementInputEnabled(false);
	}
}

void UTPPSpecialMove::EndSpecialMove_Implementation()
{
	TimeRemaining = 0.f;

	UAnimInstance* AnimInstance = OwningCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &UTPPSpecialMove::OnMontageEnded);
	}

	if (bDisablesMovementInput)
	{
		OwningCharacter->SetMovementInputEnabled(true);
	}

	OwningCharacter = nullptr;
}

void UTPPSpecialMove::PlayAnimMontage(UAnimMontage* Montage)
{
	if (Montage && OwningCharacter)
	{
		UAnimInstance* AnimInstance = OwningCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(Montage, 1.0f);
		}
	}
}

void UTPPSpecialMove::EndAnimMontage(UAnimMontage* MontageToEnd)
{
	if (MontageToEnd && OwningCharacter)
	{
		UAnimInstance* AnimInstance = OwningCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Stop(0, MontageToEnd);
		}
	}
}

void UTPPSpecialMove::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{

}

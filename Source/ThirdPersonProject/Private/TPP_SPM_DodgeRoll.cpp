// Fill out your copyright notice in the Description page of Project Settings.


#include "TPP_SPM_DodgeRoll.h"
#include "TPPMovementComponent.h"
#include "ThirdPersonProject/ThirdPersonProjectCharacter.h"

UTPP_SPM_DodgeRoll::UTPP_SPM_DodgeRoll(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDisablesMovementInput = true;
}

void UTPP_SPM_DodgeRoll::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	if (OwningCharacter)
	{
		OwningCharacter->SetMovementInputEnabled(false);
	}

	if (AnimMontage)
	{
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
		PlayAnimMontage(AnimMontage);
	}
}

void UTPP_SPM_DodgeRoll::Tick(float DeltaTime)
{
	UTPPMovementComponent* MovementComponent = OwningCharacter ? Cast<UTPPMovementComponent>(OwningCharacter->GetMovementComponent()) : nullptr;
	if (MovementComponent)
	{
		MovementComponent->RequestDirectMove(FVector(50.f, 50.f, 50.f), true);
	}

	Super::Tick(DeltaTime);
}

void UTPP_SPM_DodgeRoll::EndSpecialMove_Implementation()
{
	if (OwningCharacter)
	{
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);
		OwningCharacter->SetMovementInputEnabled(true);
	}

	Super::EndSpecialMove_Implementation();
}

void UTPP_SPM_DodgeRoll::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (AnimMontage == Montage)
	{
		EndSpecialMove();
	}

	Super::OnMontageEnded(Montage, bInterrupted);
}

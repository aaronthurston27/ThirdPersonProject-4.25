 // Fill out your copyright notice in the Description page of Project Settings.


#include "SpecialMove/TPPSpecialMove.h"
#include "TPPPlayerController.h"
#include "Weapon/TPPWeaponBase.h"
#include "Net/UnrealNetwork.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPPSpecialMove::UTPPSpecialMove(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) 
{

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
			OnDurationExceeded();
		}
	}
}

void UTPPSpecialMove::BeginSpecialMove_Implementation()
{
	TimeRemaining = Duration;

	USkeletalMeshComponent* SkeletalMesh = OwningCharacter->GetMesh();
	UAnimInstance* AnimInstance = SkeletalMesh ? SkeletalMesh->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.AddDynamic(this, &UTPPSpecialMove::OnMontageEnded);
		AnimInstance->OnMontageBlendingOut.AddDynamic(this, &UTPPSpecialMove::OnMontageBlendOut);
	}

	if (bDisablesMovementInput)
	{
		ATPPPlayerController* PlayerController = Cast<ATPPPlayerController>(OwningCharacter->Controller);
		if (PlayerController)
		{
			PlayerController->SetMovementInputEnabled(false);
		}
	}

	if (bInterruptsReload)
	{
		ATPPWeaponBase* Firearm = OwningCharacter->GetCurrentEquippedWeapon();
		if (Firearm)
		{
			Firearm->InterruptReload();
		}
	}

	bIsWeaponUseDisabled = bDisablesWeaponUseOnStart;
}

void UTPPSpecialMove::OnDurationExceeded_Implementation()
{
	EndSpecialMove();
}

void UTPPSpecialMove::EndSpecialMove_Implementation()
{
	TimeRemaining = 0.f;

	USkeletalMeshComponent* SkeletalMesh = OwningCharacter->GetMesh();
	UAnimInstance* AnimInstance = SkeletalMesh ? SkeletalMesh->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &UTPPSpecialMove::OnMontageEnded);
		AnimInstance->OnMontageBlendingOut.RemoveDynamic(this, &UTPPSpecialMove::OnMontageBlendOut);
	}

	if (bDisablesMovementInput)
	{
		ATPPPlayerController* PlayerController = Cast<ATPPPlayerController>(OwningCharacter->Controller);
		if (PlayerController)
		{
			PlayerController->SetMovementInputEnabled(true);
		}
	}

	OwningCharacter->OnSpecialMoveEnded(this);
	OwningCharacter = nullptr;
}

void UTPPSpecialMove::InterruptSpecialMove()
{
	bWasInterrupted = true;
	EndSpecialMove();
}

void UTPPSpecialMove::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{

}

void UTPPSpecialMove::OnMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{

}

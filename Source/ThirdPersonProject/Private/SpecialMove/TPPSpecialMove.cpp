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
	if (OwningCharacter && OwningCharacter->HasAuthority())
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
}

void UTPPSpecialMove::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTPPSpecialMove, bIsWeaponUseDisabled);
	DOREPLIFETIME(UTPPSpecialMove, OwningCharacter);
	DOREPLIFETIME(UTPPSpecialMove, TimeRemaining);
	DOREPLIFETIME(UTPPSpecialMove, bWasInterrupted);
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

	Client_SpecialMoveStarted();
}

void UTPPSpecialMove::Client_SpecialMoveStarted_Implementation()
{

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

void UTPPSpecialMove::InterruptSpecialMove_Implementation()
{
	bWasInterrupted = true;
	EndSpecialMove();
}

void UTPPSpecialMove::PlayAnimMontage_Implementation(UAnimMontage* Montage, bool bShouldEndAllMontages)
{
	if (Montage && OwningCharacter)
	{
		UAnimInstance* AnimInstance = OwningCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(Montage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, bShouldEndAllMontages);
		}
	}
}

void UTPPSpecialMove::EndAnimMontage_Implementation(UAnimMontage* MontageToEnd)
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

void UTPPSpecialMove::SetAnimRootMotionMode(TEnumAsByte<ERootMotionMode::Type> NewMode)
{
	USkeletalMeshComponent* SkeletalMesh = OwningCharacter ? OwningCharacter->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = SkeletalMesh ? SkeletalMesh->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		AnimInstance->SetRootMotionMode(NewMode);
		OnRootMotionModeSet();
	}
}

void UTPPSpecialMove::OnRootMotionModeSet_Implementation()
{

}

void UTPPSpecialMove::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{

}

void UTPPSpecialMove::OnMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{

}

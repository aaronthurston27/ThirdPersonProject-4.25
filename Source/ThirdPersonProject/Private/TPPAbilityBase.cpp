// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPAbilityBase.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPPAbilityBase::UTPPAbilityBase()
{
	bIsInSweetSpot = false;
}

UTPPAbilityBase::~UTPPAbilityBase()
{
}

bool UTPPAbilityBase::ActivateAbility()
{
	OnAbilityActivated();
	return true;
}

bool UTPPAbilityBase::CanActivate_Implementation()
{
	return OwningCharacter && OwningCharacter->GetCurrentSpecialMove() == nullptr
		&& OwningCharacter->IsCharacterAlive();
}

void UTPPAbilityBase::SetOwningCharacter(ATPPPlayerCharacter* Character)
{
	OwningCharacter = Character;
}



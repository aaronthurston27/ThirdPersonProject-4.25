// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAbility.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UBaseAbility::UBaseAbility()
{
	bIsInSweetSpot = false;
}

UBaseAbility::~UBaseAbility()
{
}

bool UBaseAbility::ActivateAbility()
{
	OnAbilityActivated();
	return true;
}

bool UBaseAbility::CanActivate_Implementation()
{
	return OwningCharacter && OwningCharacter->GetCurrentSpecialMove() == nullptr;
}

void UBaseAbility::SetOwningCharacter(ATPPPlayerCharacter* Character)
{
	OwningCharacter = Character;
}



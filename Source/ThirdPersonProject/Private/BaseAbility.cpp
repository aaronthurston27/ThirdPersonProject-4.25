// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAbility.h"
#include "ThirdPersonProject/ThirdPersonProjectCharacter.h"

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

void UBaseAbility::SetOwningCharacter(AThirdPersonProjectCharacter* Character)
{
	OwningCharacter = Character;
}



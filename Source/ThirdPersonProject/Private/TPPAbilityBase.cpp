// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPAbilityBase.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "Net/UnrealNetwork.h"

UTPPAbilityBase::UTPPAbilityBase()
{
	LastAbilityUseTime = -BIG_NUMBER;
}

UTPPAbilityBase::~UTPPAbilityBase()
{
}

bool UTPPAbilityBase::ActivateAbility()
{
	OnAbilityActivated();
	LastAbilityUseTime = GetWorld()->GetTimeSeconds();
	return true;
}

bool UTPPAbilityBase::CanActivate_Implementation() const
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	UE_LOG(LogTemp, Warning, TEXT("%f"), CurrentTime);
	return OwningCharacter && OwningCharacter->GetCurrentSpecialMove() == nullptr
		&& OwningCharacter->IsCharacterAlive() && CurrentTime >= LastAbilityUseTime + AbilityCooldownTime;
}

void UTPPAbilityBase::SetOwningCharacter(ATPPPlayerCharacter* Character)
{
	OwningCharacter = Character;
}

void UTPPAbilityBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTPPAbilityBase, OwningCharacter);
	DOREPLIFETIME(UTPPAbilityBase, LastAbilityUseTime);
}



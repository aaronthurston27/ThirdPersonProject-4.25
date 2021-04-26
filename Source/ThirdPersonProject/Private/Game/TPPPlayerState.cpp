// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/TPPPlayerState.h"
#include "Net/UnrealNetwork.h"

void ATPPPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPPPlayerState, PlayerHealth);
	DOREPLIFETIME(ATPPPlayerState, bIsPlayerAlive);
}

void ATPPPlayerState::OnRep_PlayerHealth()
{

}

void ATPPPlayerState::SetPlayerHealth_Implementation(float Health)
{
	PlayerHealth = Health;
	bIsPlayerAlive = Health > 0.0f;
	OnRep_PlayerHealth();
}

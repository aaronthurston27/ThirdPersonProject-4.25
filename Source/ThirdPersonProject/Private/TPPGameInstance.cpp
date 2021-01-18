// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPGameInstance.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "TPPHUD.h"
#include "Kismet/GameplayStatics.h"

void UTPPGameInstance::Init()
{
	Super::Init();
}

ATPPPlayerCharacter* UTPPGameInstance::GetPlayerCharacter() const
{
	AActor* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(),0);
	return Cast<ATPPPlayerCharacter>(PlayerCharacter);
}
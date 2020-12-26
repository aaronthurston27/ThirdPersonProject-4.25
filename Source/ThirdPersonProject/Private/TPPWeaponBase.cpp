// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponBase.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

// Sets default values
ATPPWeaponBase::ATPPWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
}

void ATPPWeaponBase::BeginPlay()
{
	Super::BeginPlay();
}

void ATPPWeaponBase::SetWeaponOwner(ATPPPlayerCharacter* NewWeaponOwner)
{
	CharacterOwner = NewWeaponOwner;
}


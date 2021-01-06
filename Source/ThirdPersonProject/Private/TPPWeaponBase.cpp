// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponBase.h"

// Sets default values
ATPPWeaponBase::ATPPWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	SetRootComponent(WeaponMesh);
	PrimaryActorTick.bCanEverTick = false;
}

void ATPPWeaponBase::BeginPlay()
{
	Super::BeginPlay();
}

void ATPPWeaponBase::SetWeaponOwner(ATPPPlayerCharacter* NewWeaponOwner)
{
	CharacterOwner = NewWeaponOwner;
}

void ATPPWeaponBase::ConsumeLoadedAmmo(int32 AmmoToConsume)
{
	LoadedAmmoCount = FMath::Max(0, LoadedAmmoCount - AmmoToConsume);
}

bool ATPPWeaponBase::CanFireWeapon_Implementation()
{
	return LoadedAmmoCount > 0 && CharacterOwner != nullptr;
}

void ATPPWeaponBase::FireWeapon_Implementation()
{

}


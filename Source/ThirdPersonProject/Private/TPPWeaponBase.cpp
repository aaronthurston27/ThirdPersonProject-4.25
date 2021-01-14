// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponBase.h"

// Sets default values
ATPPWeaponBase::ATPPWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
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

void ATPPWeaponBase::ModifyWeaponAmmo(const int32 ChamberAmmoChange, const int32 PooledAmmoChange)
{
	LoadedAmmo = FMath::Clamp(LoadedAmmo + ChamberAmmoChange, 0, MaxLoadedAmmo);
	CurrentAmmoPool = FMath::Clamp(CurrentAmmoPool + PooledAmmoChange, 0, MaxAmmoInPool);
}

void ATPPWeaponBase::SetWeaponReady(bool bWeaponReady)
{
	bIsWeaponReady = bWeaponReady;
}

bool ATPPWeaponBase::CanFireWeapon_Implementation()
{
	return bIsWeaponReady && LoadedAmmo > 0 && CharacterOwner != nullptr;
}

void ATPPWeaponBase::FireWeapon_Implementation()
{
	OnWeaponFired.Broadcast();
}



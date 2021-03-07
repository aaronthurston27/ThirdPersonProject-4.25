// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/TPPWeaponBase.h"
#include "TPPBlueprintFunctionLibrary.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "Components/DecalComponent.h"
#include "TPPHUD.h"
#include "Kismet/GameplayStatics.h"
#include "TPPDamageType.h"
#include "Weapon/TPPWeaponBase.h"

// Sets default values
ATPPWeaponBase::ATPPWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	SetRootComponent(WeaponMesh);

	BaseWeaponDamage = 8.0f;
}

void ATPPWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	SetWeaponReady(true);
}

void ATPPWeaponBase::SetWeaponOwner(ATPPPlayerCharacter* NewWeaponOwner)
{
	CharacterOwner = NewWeaponOwner;
}

void ATPPWeaponBase::Equip()
{
	WeaponMesh->SetCollisionProfileName(FName(TEXT("No Collision")));
	WeaponMesh->SetSimulatePhysics(false);
}

void ATPPWeaponBase::Unequip()
{
}

void ATPPWeaponBase::Drop(bool bShouldBecomePickup)
{
	WeaponMesh->SetCollisionProfileName(FName(TEXT("WeaponPickup")));
	WeaponMesh->SetSimulatePhysics(true);

	InterruptReload();
	SetIsReloading(false);
	SetWeaponReady(false);

	SetWeaponOwner(nullptr);

	PrimaryActorTick.bCanEverTick = false;
}

void ATPPWeaponBase::ModifyWeaponAmmo(const int32 ChamberAmmoChange, const int32 PooledAmmoChange)
{
	LoadedAmmo = FMath::Clamp(LoadedAmmo + ChamberAmmoChange, 0, MaxLoadedAmmo);
	CurrentAmmoPool = FMath::Clamp(CurrentAmmoPool + PooledAmmoChange, 0, MaxAmmoInPool);

	if (OnWeaponAmmoUpdated.IsBound())
	{
		OnWeaponAmmoUpdated.Broadcast();
	}

}

void ATPPWeaponBase::SetWeaponReady(bool bWeaponReady)
{
	bIsWeaponReady = bWeaponReady;
}

bool ATPPWeaponBase::CanFireWeapon_Implementation() const
{
	const bool bIsOwnerAlive = CharacterOwner && CharacterOwner->IsCharacterAlive();
	return bIsWeaponReady && LoadedAmmo > 0 && bIsOwnerAlive;
}

void ATPPWeaponBase::FireWeapon_Implementation()
{
	OnWeaponFired.Broadcast();
}

void ATPPWeaponBase::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
}

bool ATPPWeaponBase::CanReloadWeapon_Implementation()
{
	const bool bIsOwnerAlive = CharacterOwner && CharacterOwner->IsCharacterAlive();
	return bIsOwnerAlive && bIsWeaponReady && !bIsReloading && LoadedAmmo < MaxLoadedAmmo &&
		CurrentAmmoPool > 0;
}

void ATPPWeaponBase::SetIsReloading(bool bIsWeaponReloading)
{
	bIsReloading = bIsWeaponReloading;
}

void ATPPWeaponBase::StartWeaponReload()
{
}

void ATPPWeaponBase::ReloadActual()
{
	const int32 AmmoToChamber = FMath::Min(CurrentAmmoPool, MaxLoadedAmmo);
	ModifyWeaponAmmo(AmmoToChamber, -AmmoToChamber);

	if (OnWeaponAmmoUpdated.IsBound())
	{
		OnWeaponAmmoUpdated.Broadcast();
	}
}

void ATPPWeaponBase::InterruptReload()
{

}

void ATPPWeaponBase::OnWeaponHit_Implementation(const FHitResult& HitResult, const float DamageApplied)
{
	if (DamageApplied > 0.0f)
	{
		ATPPHUD* TPPHUD = CharacterOwner->GetCharacterHUD();
		if (TPPHUD)
		{
			TPPHUD->OnWeaponHit(this, HitResult, DamageApplied);
		}
	}
}

void ATPPWeaponBase::ApplyWeaponPointDamage(const FHitResult& HitResult, const FVector& StartingLocation)
{
	if (HitResult.bBlockingHit && HitResult.Component != nullptr && CharacterOwner)
	{
		ATPPPlayerCharacter* CharacterHit = Cast<ATPPPlayerCharacter>(HitResult.Actor.Get());
		UTPPDamageType* DamageType = Cast<UTPPDamageType>(HitDamageClass.GetDefaultObject());
		if (CharacterHit && CharacterHit->IsCharacterAlive() && DamageType)
		{
			float BaseDamage = BaseWeaponDamage;
			if (HitResult.BoneName.IsEqual("head"))
			{
				BaseDamage *= DamageType->DamageHeadshotMultiplier;
			}
			const float DamageApplied = UGameplayStatics::ApplyPointDamage(CharacterHit, BaseDamage, StartingLocation.GetSafeNormal(), HitResult, CharacterOwner->GetController(), CharacterOwner, HitDamageClass);
			OnWeaponHit(HitResult, DamageApplied);
		}
		else if (!CharacterHit)
		{
			UPrimitiveComponent* PrimitiveComp = HitResult.Component.Get();
			UDecalComponent* SpawnedDecal = UTPPBlueprintFunctionLibrary::SpawnDecalWithParameters(PrimitiveComp, ImpactProperties.WeaponHitMaterial, 10.0f, HitResult.ImpactPoint, HitResult.ImpactNormal.Rotation(), ImpactProperties.WeaponHitDecalSize);
			if (SpawnedDecal)
			{
				SpawnedDecal->Activate();
			}
		}
	}
}

void ATPPWeaponBase::ApplyWeaponBlastDamage(const FVector& BlastCenter)
{

}




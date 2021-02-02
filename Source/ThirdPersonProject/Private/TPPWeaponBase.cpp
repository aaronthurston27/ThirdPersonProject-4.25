// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponBase.h"
#include "TPPBlueprintFunctionLibrary.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "Components/DecalComponent.h"
#include "..\Public\TPPWeaponBase.h"

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

bool ATPPWeaponBase::CanFireWeapon_Implementation() const
{
	return bIsWeaponReady && LoadedAmmo > 0 && CharacterOwner != nullptr;
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
	return bIsWeaponReady && !bIsReloading && LoadedAmmo < MaxLoadedAmmo&&
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

	if (OnWeaponReloaded.IsBound())
	{
		OnWeaponReloaded.Broadcast();
	}
}

void ATPPWeaponBase::InterruptReload()
{

}

void ATPPWeaponBase::OnWeaponHit_Implementation(const FHitResult& HitResult, FDamageEvent& DamageEvent)
{
	if (HitResult.bBlockingHit && HitResult.Component != nullptr && CharacterOwner)
	{
		ATPPPlayerCharacter* CharacterHit = Cast<ATPPPlayerCharacter>(HitResult.Actor.Get());
		if (CharacterHit && CharacterHit->IsCharacterAlive())
		{
			FPointDamageEvent* PointDamage = static_cast<FPointDamageEvent*>(&DamageEvent);
			CharacterHit->TakeDamage(BaseWeaponDamage, DamageEvent, CharacterOwner->GetController(), CharacterOwner);
		}
		else if (!CharacterHit)
		{
			UPrimitiveComponent* PrimitiveComp = HitResult.Component.Get();
			UDecalComponent* Hmm = UTPPBlueprintFunctionLibrary::SpawnDecalWithParameters(PrimitiveComp, ImpactProperties.WeaponHitMaterial, 10.0f, HitResult.ImpactPoint, HitResult.ImpactNormal.Rotation(), ImpactProperties.WeaponHitDecalSize);
			if (Hmm)
			{
				Hmm->Activate();
			}
		}
	}
}




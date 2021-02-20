// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Weapon/TPPWeaponBase.h"
#include "Weapon/TPPWeaponFirearm.h"
#include "TPPWeaponData.generated.h"

/**
 * Data table row representing weapon attributes.
 */
USTRUCT()
struct THIRDPERSONPROJECT_API FTPPWeaponData : public FTableRowBase
{
	GENERATED_BODY()
	
	/** Ammo type to consume for this weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo", BlueprintReadOnly)
	EWeaponAmmoType AmmoType;

	/** Maximum ammo storable in the chamber */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo", BlueprintReadOnly)
	int32 MaxLoadedAmmo;

	/** If true, ammo can be stored in the pool for reloading */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo", BlueprintReadOnly)
	bool bHasAmmoPool = false;

	/** Maximum ammo to store in the pool */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo", BlueprintReadOnly, meta = (EditCondition = "bHasAmmoPool", UIMin = "0", ClampMin = "0"))
	int32 MaxAmmoInPool = 0;

	/** Ammo to consume per shot */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo", BlueprintReadOnly)
	int32 AmmoConsumedPerShot = 1;

};


/**
* Data table row representing firearm attributes 
*/
USTRUCT()
struct THIRDPERSONPROJECT_API FTPPWeaponFirearmData : public FTableRowBase
{

	GENERATED_BODY()

	/** Weapon firing mode */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing")
	EWeaponHitType WeaponFireType = EWeaponHitType::Hitscan;

	/** Cooldown time between consective shots of this weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing", BlueprintReadOnly)
	float WeaponFireRate = .1f;
};

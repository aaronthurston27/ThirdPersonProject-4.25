// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPPWeaponBase.h"
#include "TPPWeaponFirearm.generated.h"

UENUM(BlueprintType)
enum class EWeaponFireType : uint8
{
	Hitscan,
	Projectile,
	MAX
};

/**
 * Base class for weapons that behave similar to firearms (reload, ammo pool, etc)
 */
UCLASS()
class THIRDPERSONPROJECT_API ATPPWeaponFirearm : public ATPPWeaponBase
{
	GENERATED_BODY()

public:

	ATPPWeaponFirearm();

	virtual void BeginPlay() override;

public:

	/** Weapon firing mode */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing", BlueprintReadOnly)
	EWeaponFireType WeaponFireType = EWeaponFireType::Hitscan;

	/** Cooldown time between consective shots of this weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing", BlueprintReadOnly)
	float WeaponFireRate = .1f;

	/** Maximum ammo to store in the pool */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo", BlueprintReadOnly)
	int32 MaxAmmoInPool;

public:

	/** Weapon firing sound */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Audio")
	USoundWave* FiringSound;

	/** Weapon reload sound */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Audio")
	USoundWave* ReloadSound;

protected:

	/** Ammo held in the pool reserve. Added to current ammo when reloading */
	UPROPERTY(Transient)
	uint32 CurrentAmmoPool = 0;

	/** Time since weapon was last fired * */
	UPROPERTY(Transient)
	float TimeSinceLastShot = 0.0f;

public:

	virtual bool CanFireWeapon_Implementation() override;

	virtual void FireWeapon_Implementation() override;

protected:

	/** Line trace towards the player's camera and check for a hit. */
	void HitscanFire();

	/** Spawn a projectile from the weapon. */
	void ProjectileFire();
};

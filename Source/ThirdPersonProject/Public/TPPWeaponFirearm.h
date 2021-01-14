// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPPWeaponBase.h"
#include "TPPWeaponFirearm.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponReloaded);

/** Hit logic to use for this weapon */
UENUM(BlueprintType)
enum class EWeaponHitType : uint8
{
	Hitscan,
	Projectile,
	MAX
};

/** Firing mode to use for this weapon */
UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	FullAuto,
	SemiAuto,
	Burst
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
	EWeaponHitType WeaponFireType = EWeaponHitType::Hitscan;

	/** Cooldown time between consective shots of this weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing", BlueprintReadOnly)
	float WeaponFireRate = .1f;

	/** Weapon fire montage to be played by owning character */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation", BlueprintReadOnly)
	UAnimMontage* WeaponFireCharacterMontage;

	/** Weapon reload montage to be played by owning character */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	UAnimMontage* WeaponReloadCharacterMontage;

protected:

	/** Current firing mode */
	UPROPERTY(Transient)
	EWeaponFireMode CurrentFiringMode;

	/** Time since weapon was last fired * */
	UPROPERTY(Transient)
	float TimeSinceLastShot = 0.0f;

	/** True if this weapon is being reloaded */
	UPROPERTY(Transient)
	bool bIsReloading = false;

public:

	UPROPERTY(BlueprintAssignable)
	FOnWeaponReloaded OnWeaponReloaded;

	virtual bool CanFireWeapon_Implementation() override;

	virtual void FireWeapon_Implementation() override;

protected:

	/** Line trace towards the player's camera and check for a hit. */
	void HitscanFire();

	/** Spawn a projectile from the weapon. */
	void ProjectileFire();

public:

	/** Evaluate whether or not this weapon can be reloaded */
	UFUNCTION(BlueprintNativeEvent)
	bool CanReloadWeapon();

	virtual bool CanReloadWeapon_Implementation();

	/** Starts the reload process and animation */
	virtual void StartWeaponReload();

	void SetIsReloading(bool bReloading);

protected:

	/** Actual reload process of adding ammo to chamber */
	void ReloadActual();
};

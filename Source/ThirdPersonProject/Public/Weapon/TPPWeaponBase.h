// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "Components/Image.h"
#include "TPPWeaponBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponAmmoUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponDamageActor, const FHitResult&, HitResult, const float, DamageApplied);

class ATPPPlayerCharacter;

UENUM(BlueprintType)
enum class EWeaponAmmoType : uint8
{
	None,
	Rifle,
	Pistol,
	Shotgun,
	Launcher,
	Battery,
	Thrown,
	MAX UMETA(Hidden)
};

/** Struct defining properties to apply to objects when hit by a weapon */
USTRUCT(Blueprintable)
struct FTPPWeaponImpactProperties
{
	GENERATED_BODY()

	/** Weapon hit material to apply */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UMaterial* WeaponHitMaterial = nullptr;

	/** Size of weapon hit decal */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector WeaponHitDecalSize = FVector::OneVector * 2.0f;
};

/**
* Base class that all weapons (or items that can hurt people) should derive from
*/
UCLASS(Abstract, BlueprintType)
class THIRDPERSONPROJECT_API ATPPWeaponBase : public AActor
{
	GENERATED_BODY()

public:

	/** Mesh associated with this weapon */
	UPROPERTY(VisibleAnywhere, Category = "Mesh", BlueprintReadOnly)
	USkeletalMeshComponent* WeaponMesh;

	/** Audio component for playing weapon related sounds */
	UPROPERTY(VisibleAnywhere, Category = "Audio", BlueprintReadOnly)
	UAudioComponent* AudioComponent;

public:

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

	/** Weapon image to display on HUD */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|HUD", BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> HUDImage;

	/** Weapon hit base damage */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Hit", BlueprintReadOnly)
	float BaseWeaponDamage;

	/** Damage type class to use when hit by this weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Hit", BlueprintReadOnly)
	TSubclassOf<UDamageType> HitDamageClass;

	/** Properties to use on weapon hit */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Hit", BlueprintReadOnly)
	FTPPWeaponImpactProperties ImpactProperties;

protected:

	/** Ammo loaded and ready to be fired. */
	UPROPERTY(Transient)
	int32 LoadedAmmo = 100;

	/** Ammo held in the pool reserve. Added to current ammo when reloading */
	UPROPERTY(Transient)
	int32 CurrentAmmoPool = 0;

	/** True if this weapon is ready to be fired */
	UPROPERTY(Transient)
	bool bIsWeaponReady = true;

public:

	/** Modifes ammo count of weapon */
	UFUNCTION(BlueprintCallable)
	virtual void ModifyWeaponAmmo(const int32 ChamberAmmoChange = 0, const int32 PooledAmmoChange = 0);
	
public:	

	ATPPWeaponBase();

	virtual void BeginPlay() override;

protected:

	/** Current owner of this weapon */
	UPROPERTY(Transient)
	ATPPPlayerCharacter* CharacterOwner = nullptr;

public:

	/** Sets the owner of this weapon */
	UFUNCTION(BlueprintCallable)
	void SetWeaponOwner(ATPPPlayerCharacter* NewOwner);

	UFUNCTION(BlueprintCallable)
	virtual void Equip();

	UFUNCTION(BlueprintCallable)
	virtual void Unequip();
	
	UFUNCTION(BlueprintCallable)
	virtual void Drop(bool bShouldBecomePickup = false);

public:

	UPROPERTY(BlueprintAssignable)
	FOnWeaponFired OnWeaponFired;

	UPROPERTY(BlueprintAssignable)
	FOnWeaponDamageActor OnWeaponDamageActor;

	UFUNCTION(BlueprintCallable)
	void SetWeaponReady(bool bWeaponReady);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool CanFireWeapon() const;

	virtual bool CanFireWeapon_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void FireWeapon();

	virtual void FireWeapon_Implementation();

protected:

	UFUNCTION()
	virtual void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected:

	UFUNCTION(BlueprintNativeEvent)
	void OnWeaponHit(const FHitResult& HitResult, const float DamageApplied);

	void OnWeaponHit_Implementation(const FHitResult& HitResult, const float DamageApplied);

	void ApplyWeaponPointDamage(const FHitResult& HitResult, const FVector& StartingLocation);

	void ApplyWeaponBlastDamage(const FVector& BlastCenter);

public:

	UPROPERTY(BlueprintAssignable)
	FOnWeaponAmmoUpdated OnWeaponAmmoUpdated;

	/** Evaluate whether or not this weapon can be reloaded */
	UFUNCTION(BlueprintNativeEvent)
	bool CanReloadWeapon();

	virtual bool CanReloadWeapon_Implementation();

	void SetIsReloading(bool bIsWeaponReloading);

	/** Starts the reload process and animation */
	virtual void StartWeaponReload();

	/** Actual reload process of adding ammo to chamber */
	virtual void ReloadActual();

	/** Interrups the currently playing reload animation, if any. */
	UFUNCTION(BlueprintCallable)
	virtual void InterruptReload();

protected:

	/** True if weapon is being reloaded */
	UPROPERTY(Transient)
	bool bIsReloading = false;

public:

	UFUNCTION(BlueprintCallable)
	int32 GetCurrentPooledAmmo() const { return CurrentAmmoPool; }

	UFUNCTION(BlueprintCallable)
	int32 GetLoadedAmmoCount() const { return LoadedAmmo; }
};

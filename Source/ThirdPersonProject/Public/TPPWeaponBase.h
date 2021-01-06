// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "TPPWeaponBase.generated.h"

class ATPPPlayerCharacter;

UENUM(BlueprintType)
enum class EWeaponAmmoType : uint8
{
	Rifle,
	Melee,
	MAX UMETA(Hidden)
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

	/** Maximum ammo to store in the chamber */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo", BlueprintReadOnly)
	int32 MaxLoadedAmmo;

protected:

	/** Ammo loaded and ready to be fired. */
	UPROPERTY(Transient)
	int32 LoadedAmmoCount = 100;

protected:

	/** Consumes ammo store in the chamber */
	void ConsumeLoadedAmmo(int32 AmmoToConsume);
	
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

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool CanFireWeapon();

	virtual bool CanFireWeapon_Implementation();

	UFUNCTION(BlueprintNativeEvent)
	void FireWeapon();

	virtual void FireWeapon_Implementation();
};

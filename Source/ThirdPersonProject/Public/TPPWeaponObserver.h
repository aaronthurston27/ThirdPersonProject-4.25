// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TPPWeaponObserver.generated.h"

class ATPPWeaponBase;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTPPWeaponObserver : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class THIRDPERSONPROJECT_API ITPPWeaponObserver
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual void SetObservedWeapon(ATPPWeaponBase* Weapon) = 0;

protected:

	UFUNCTION(BlueprintNativeEvent)
	void OnWeaponFired();

	virtual void OnWeaponFired_Implementation() = 0;

	UFUNCTION(BlueprintNativeEvent)
	void OnWeaponReloaded();

	virtual void OnWeaponReloaded_Implementation() = 0;

	virtual void AssignWeaponDelegates(ATPPWeaponBase* Weapon) = 0;

	virtual void RemoveWeaponDelegates(ATPPWeaponBase* Weapon) = 0;
};

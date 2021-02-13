// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "TPPWeaponBase.h"
#include "TPPWeaponObserver.h"
#include "TPPCrosshairWidget.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API UTPPCrosshairWidget : public UUserWidget, public ITPPWeaponObserver
{
	GENERATED_BODY()

protected:

	UPROPERTY(Transient, BlueprintReadOnly)
	ATPPWeaponBase* ObservedWeapon = nullptr;

public:

	void SetObservedWeapon(ATPPWeaponBase* WeaponToObserve);

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void OnWeaponChanged();

	virtual void OnWeaponFired_Implementation();

	virtual void OnWeaponReloaded_Implementation();

	virtual void OnWeaponHit_Implementation(const FHitResult& HitResult, const float DamageApplied);

	virtual void AssignWeaponDelegates(ATPPWeaponBase* Weapon);

	virtual void RemoveWeaponDelegates(ATPPWeaponBase* Weapon);
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TPPCrosshairWidget.h"
#include "TPPWeaponInfoWidget.h"
#include "TPPHUD.generated.h"

class ATPPPlayerCharacter;
class ATPPWeaponBase;

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API ATPPHUD : public AHUD
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	void InitializeHUD(ATPPPlayerCharacter* Character);

public:

	UPROPERTY(EditDefaultsOnly, Category = "WidgetClasses")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "WidgetClasses")
	TSubclassOf<UUserWidget> WeaponInfoWidgetClass;

protected:

	/** Character this HUD shows relevant info for*/
	UPROPERTY(Transient, BlueprintReadOnly)
	ATPPPlayerCharacter* ObservedCharacter;


	UPROPERTY(Transient)
	UTPPCrosshairWidget* CrosshairWidget;

	UPROPERTY(Transient)
	UTPPWeaponInfoWidget* WeaponInfoWidget;

protected:

	UFUNCTION()
	void OnPlayerEquippedWeapon(ATPPWeaponBase* WeaponEquipped);

public:
	// Called when a player equipped weapon makes contact and applies damage. This can be called publicly without a delegate due to some weapons applying damage without being equipped at the time of impact (e.g. projectiles, grenades) */
	UFUNCTION()
	void OnWeaponHit(const ATPPWeaponBase* UsedWeapon, const FHitResult& HitResult, const float DamageApplied);
	
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPHUD.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "TPPWeaponBase.h"
#include "Kismet/GameplayStatics.h"

void ATPPHUD::BeginPlay()
{
	Super::BeginPlay();

	if (CrosshairWidgetClass)
	{
		CrosshairWidget = CreateWidget<UTPPCrosshairWidget>(GetWorld(), CrosshairWidgetClass);
		if (CrosshairWidget)
		{
			CrosshairWidget->AddToViewport();
		}
	}

	if (WeaponInfoWidgetClass)
	{
		WeaponInfoWidget = CreateWidget<UTPPWeaponInfoWidget>(GetWorld(), WeaponInfoWidgetClass);
		if (WeaponInfoWidget)
		{
			WeaponInfoWidget->AddToViewport();
		}
	}
}

void ATPPHUD::InitializeHUD(ATPPPlayerCharacter* PlayerCharacter)
{
	if (PlayerCharacter)
	{
		PlayerCharacter->OnWeaponEquipped.AddDynamic(this, &ATPPHUD::OnPlayerEquippedWeapon);
	}
}

void ATPPHUD::OnPlayerEquippedWeapon(ATPPWeaponBase* WeaponEquipped)
{
	if (CrosshairWidget)
	{
		CrosshairWidget->SetObservedWeapon(WeaponEquipped);
	}

	if (WeaponInfoWidget)
	{
		WeaponInfoWidget->SetObservedWeapon(WeaponEquipped);
	}
}

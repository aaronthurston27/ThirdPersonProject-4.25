// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPCrosshairWidget.h"

void UTPPCrosshairWidget::SetObservedWeapon(ATPPWeaponBase* WeaponToObserve)
{
	if (WeaponToObserve && WeaponToObserve != ObservedWeapon)
	{
		RemoveWeaponDelegates(ObservedWeapon);
	}

	ObservedWeapon = WeaponToObserve;
	AssignWeaponDelegates(ObservedWeapon);

	OnWeaponChanged();
}

void UTPPCrosshairWidget::OnWeaponFired_Implementation()
{
}

void UTPPCrosshairWidget::OnWeaponReloaded_Implementation()
{

}

void UTPPCrosshairWidget::OnWeaponHit_Implementation(const FHitResult& HitResult, const float DamageApplied)
{

}

void UTPPCrosshairWidget::AssignWeaponDelegates(ATPPWeaponBase* Weapon)
{
	if (Weapon)
	{
		Weapon->OnWeaponFired.AddDynamic(this, &UTPPCrosshairWidget::OnWeaponFired);
	}
}

void UTPPCrosshairWidget::RemoveWeaponDelegates(ATPPWeaponBase* Weapon)
{
	if (Weapon)
	{
		Weapon->OnWeaponFired.RemoveDynamic(this, &UTPPCrosshairWidget::OnWeaponFired);
	}
}
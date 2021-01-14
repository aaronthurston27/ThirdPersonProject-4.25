// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponInfoWidget.h"
#include "TPPWeaponFirearm.h"

void UTPPWeaponInfoWidget::SetObservedWeapon(ATPPWeaponBase* WeaponToObserve)
{
	if (ObservedWeapon && ObservedWeapon != WeaponToObserve)
	{
		RemoveWeaponDelegates(ObservedWeapon);
	}

	ObservedWeapon = WeaponToObserve;
	AssignWeaponDelegates(ObservedWeapon);
	OnWeaponChanged();
}

void UTPPWeaponInfoWidget::OnWeaponFired_Implementation()
{

}

void UTPPWeaponInfoWidget::OnWeaponReloaded_Implementation()
{

}

void UTPPWeaponInfoWidget::AssignWeaponDelegates(ATPPWeaponBase* Weapon)
{
	if (Weapon)
	{
		Weapon->OnWeaponFired.AddDynamic(this, &UTPPWeaponInfoWidget::OnWeaponFired);

		ATPPWeaponFirearm* Firearm = Cast<ATPPWeaponFirearm>(Weapon);
		if (Firearm)
		{
			Firearm->OnWeaponReloaded.AddDynamic(this, &UTPPWeaponInfoWidget::OnWeaponReloaded);
		}
	}
}

void UTPPWeaponInfoWidget::RemoveWeaponDelegates(ATPPWeaponBase* Weapon)
{
	if (Weapon)
	{
		Weapon->OnWeaponFired.RemoveDynamic(this, &UTPPWeaponInfoWidget::OnWeaponFired);

		ATPPWeaponFirearm* Firearm = Cast<ATPPWeaponFirearm>(Weapon);
		if (Firearm)
		{
			Firearm->OnWeaponReloaded.RemoveDynamic(this, &UTPPWeaponInfoWidget::OnWeaponReloaded);
		}
	}
}

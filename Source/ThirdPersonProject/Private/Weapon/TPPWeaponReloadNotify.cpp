// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/TPPWeaponReloadNotify.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "Weapon/TPPWeaponBase.h"

void UTPPWeaponReloadNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ATPPPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATPPPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	ATPPWeaponBase* WeaponToReload = PlayerCharacter ? PlayerCharacter->GetCurrentEquippedWeapon() : nullptr;
	if (WeaponToReload)
	{
		WeaponToReload->ReloadActual();
	}
}
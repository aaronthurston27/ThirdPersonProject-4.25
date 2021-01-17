// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponReloadNotify.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "TPPWeaponFirearm.h"

void UTPPWeaponReloadNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ATPPPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATPPPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	ATPPWeaponFirearm* FirearmToReload = PlayerCharacter ? Cast<ATPPWeaponFirearm>(PlayerCharacter->GetCurrentEquippedWeapon()) : nullptr;
	if (FirearmToReload)
	{
		FirearmToReload->ReloadActual();
	}
}
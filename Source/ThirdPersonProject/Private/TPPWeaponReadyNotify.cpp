// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponReadyNotify.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "TPPWeaponBase.h"

void UTPPWeaponReadyNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ATPPPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATPPPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	ATPPWeaponBase* Weapon = PlayerCharacter ? PlayerCharacter->GetCurrentEquippedWeapon() : nullptr;
	if (Weapon)
	{
		Weapon->SetWeaponReady(bShouldWeaponBeReady);
	}
}
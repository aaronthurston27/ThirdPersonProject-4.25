// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponReloadNotifyState.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "TPPWeaponBase.h"

void UTPPWeaponReloadNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	ATPPPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATPPPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	ATPPWeaponBase* WeaponToReload = PlayerCharacter ? PlayerCharacter->GetCurrentEquippedWeapon() : nullptr;
	if (WeaponToReload)
	{
		WeaponToReload->SetIsReloading(true);
	}
}

void UTPPWeaponReloadNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ATPPPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATPPPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	ATPPWeaponBase* WeaponToReload = PlayerCharacter ? PlayerCharacter->GetCurrentEquippedWeapon() : nullptr;
	if (WeaponToReload)
	{
		WeaponToReload->SetIsReloading(false);
	}
}

void UTPPWeaponReloadNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{

}
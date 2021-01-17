// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponReloadNotifyState.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "TPPWeaponFirearm.h"

void UTPPWeaponReloadNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	ATPPPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATPPPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	ATPPWeaponFirearm* FirearmToReload = PlayerCharacter ? Cast<ATPPWeaponFirearm>(PlayerCharacter->GetCurrentEquippedWeapon()) : nullptr;
	if (FirearmToReload)
	{
		FirearmToReload->SetIsReloading(true);
	}
}

void UTPPWeaponReloadNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ATPPPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATPPPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	ATPPWeaponFirearm* FirearmToReload = PlayerCharacter ? Cast<ATPPWeaponFirearm>(PlayerCharacter->GetCurrentEquippedWeapon()) : nullptr;
	if (FirearmToReload)
	{
		FirearmToReload->SetIsReloading(false);
	}
}

void UTPPWeaponReloadNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	ATPPPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATPPPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	ATPPWeaponFirearm* FirearmToReload = PlayerCharacter ? Cast<ATPPWeaponFirearm>(PlayerCharacter->GetCurrentEquippedWeapon()) : nullptr;
}
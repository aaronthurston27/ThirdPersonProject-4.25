// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecialMove/TPP_SPM_LedgeHang.h"
#include "TPPMovementComponent.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPP_SPM_LedgeHang::UTPP_SPM_LedgeHang(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDisablesMovementInput = true;
	bDisablesAiming = true;
	bDisablesSprint = true;
	bDisablesWeaponUseOnStart = true;
	bInterruptsReload = true;
	bDisablesCharacterRotation = true;
}

void UTPP_SPM_LedgeHang::BeginSpecialMove_Implementation()
{

	Super::BeginSpecialMove_Implementation();

	OwningCharacter->GetCurrentWallClimbProperties(ImpactResult, TargetAttachPoint);

	UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
	MovementComp->SetMovementMode(EMovementMode::MOVE_None);

	OwningCharacter->SetWallMovementState(EWallMovementState::WallCling);
	OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
	
	FRotator Rotation = (-1.0f * ImpactResult.ImpactNormal).Rotation();
	OwningCharacter->SetActorRotation(Rotation);
	OwningCharacter->SetActorLocation(TargetAttachPoint + OwningCharacter->WallLedgeGrabOffset);

	const float LocationDiff = (CachedLedgeDelta).Size();

	bIsAligningToWall = LocationDiff > 1.0f;
}

void UTPP_SPM_LedgeHang::Tick(float DeltaTime)
{
}

void UTPP_SPM_LedgeHang::EndSpecialMove_Implementation()
{
}

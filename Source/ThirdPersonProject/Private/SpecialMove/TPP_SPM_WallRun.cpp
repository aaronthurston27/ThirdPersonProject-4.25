// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecialMove/TPP_SPM_WallRun.h"
#include "TPPPlayerController.h"
#include "TPPMovementComponent.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPP_SPM_WallRun::UTPP_SPM_WallRun(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDisablesMovementInput = true;
	bDisablesSprint = true;
	bDisablesJump = true;
	bDisablesCrouch = true;
	bDisablesWeaponUseOnStart = true;
	bInterruptsReload = true;
	bDisablesCharacterRotation = true;
}

void UTPP_SPM_WallRun::SetWallRunDestination(const FVector& WallRunDestination)
{
	WallRunDestinationPoint = WallRunDestination;
}

void UTPP_SPM_WallRun::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	OwningCharacter->GetCurrentWallClimbProperties(TargetWallImpactResult, TargetAttachPoint);

	UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
	if (!MovementComp || TargetAttachPoint.IsNearlyZero())
	{
		EndSpecialMove();
		return;
	}

	float WallLedgeHeight = OwningCharacter->GetCachedLedgeHeight();
	const float DestinationZ = WallLedgeHeight > 0.0f ? FMath::Min(WallLedgeHeight, WallRunMaxVerticalDistance) : WallRunMaxVerticalDistance;
	WallRunDestinationPoint = TargetAttachPoint + FVector(0.0f, 0.0f, DestinationZ);

	const FRotator ToWallRotation = (-1.0f * TargetWallImpactResult.ImpactNormal).Rotation();
	OwningCharacter->SetActorRotation(ToWallRotation);
	OwningCharacter->SetActorLocation(TargetAttachPoint);

	OwningCharacter->SetWallMovementState(EWallMovementState::WallRunUp);
	MovementComp->SetMovementMode(EMovementMode::MOVE_Flying);

	OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
}

void UTPP_SPM_WallRun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
	MovementComp->Velocity = FVector(0.0f, 0.0f, WallRunVerticalSpeed);

	float CurrentZ = OwningCharacter->GetActorLocation().Z;
	if (CurrentZ >= WallRunDestinationPoint.Z)
	{
		OnWallRunDestinationReached();
	}
}

void UTPP_SPM_WallRun::OnWallRunDestinationReached()
{
	EndSpecialMove();
}

void UTPP_SPM_WallRun::EndSpecialMove_Implementation()
{	
	UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();

	if (!bWasInterrupted)
	{
		OwningCharacter->SetWallMovementState(EWallMovementState::None);
		MovementComp->SetMovementMode(EMovementMode::MOVE_Falling);
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);
	}

	Super::EndSpecialMove_Implementation();
}

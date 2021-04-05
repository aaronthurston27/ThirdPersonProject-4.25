// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecialMove/TPP_SPM_WallRun.h"
#include "TPPPlayerController.h"
#include "TPPMovementComponent.h"
#include "TPPPlayerController.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPP_SPM_WallRun::UTPP_SPM_WallRun(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDurationBased = true;
	Duration = 1.5f;
	bDisablesMovementInput = true;
	bDisablesSprint = true;
	bDisablesJump = true;
	bDisablesCrouch = true;
	bDisablesWeaponUseOnStart = true;
	bInterruptsReload = true;
	bDisablesCharacterRotation = true;
}

void UTPP_SPM_WallRun::SetWallRunProperties(const FHitResult& WallTraceHitResult, const FVector& AttachPoint, const float LedgeHeight)
{
	TargetWallImpactResult = WallTraceHitResult;
	TargetAttachPoint = AttachPoint;
	WallLedgeHeight = LedgeHeight;
}

void UTPP_SPM_WallRun::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
	if (!MovementComp || TargetAttachPoint.IsNearlyZero())
	{
		EndSpecialMove();
		return;
	}

	const float WallRunMaxDistance = bDurationBased ? WallRunVerticalSpeed * Duration : WallRunMaxVerticalDistance;
	// Added 15.0f to the ledge ledge grab offset to account for the wall attach trace starting at the players foot instead of the hips (center).
	const float DestinationZ = WallLedgeHeight > 0.0f ? FMath::Min(WallLedgeHeight - OwningCharacter->LedgeGrabMaxHeight + 15.0f, WallRunMaxDistance) : WallRunMaxDistance;
	WallRunDestinationPoint = TargetAttachPoint + FVector(0.0f, 0.0f, DestinationZ);

	const FRotator ToWallRotation = (-1.0f * TargetWallImpactResult.ImpactNormal).Rotation();
	OwningCharacter->SetActorRotation(ToWallRotation);
	OwningCharacter->SetActorLocation(TargetAttachPoint);

	OwningCharacter->SetWallMovementState(EWallMovementState::WallRunUp);
	MovementComp->SetMovementMode(EMovementMode::MOVE_Flying);

	OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);

	ATPPPlayerController* PC = OwningCharacter->GetTPPPlayerController();
	if (PC)
	{
		CachedDirectionInput = PC->GetDesiredMovementDirection();
	}
}

void UTPP_SPM_WallRun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (OwningCharacter)
	{
		ATPPPlayerController* PC = OwningCharacter ? OwningCharacter->GetTPPPlayerController() : nullptr;
		const FVector CurrentDesiredMovementDirection = PC ? PC->GetDesiredMovementDirection() : FVector::ZeroVector;
		const bool bChangedXInput = CachedDirectionInput.X != 0.0f && CachedDirectionInput.X != CurrentDesiredMovementDirection.X;
		const bool bChangedYInput = CachedDirectionInput.Y != 0.0f && CachedDirectionInput.Y != CurrentDesiredMovementDirection.Y;
		if (bChangedXInput || (bChangedYInput && CurrentDesiredMovementDirection.X <= 0.0f))
		{
			TimeSinceInputChanged += DeltaTime;
			if (TimeSinceInputChanged >= InputDelay)
			{
				EndSpecialMove();
				return;
			}
		}
		else
		{
			TimeSinceInputChanged = 0.0f;
		}

		UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
		MovementComp->Velocity = FVector(0.0f, 0.0f, WallRunVerticalSpeed);

		float CurrentZ = OwningCharacter->GetActorLocation().Z;
		if (CurrentZ >= WallRunDestinationPoint.Z)
		{
			OnWallRunDestinationReached();
		}
	}
}

void UTPP_SPM_WallRun::OnDurationExceeded_Implementation()
{
	OnWallRunDestinationReached();
}

void UTPP_SPM_WallRun::OnWallRunDestinationReached()
{
	UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
	if (MovementComp)
	{
		MovementComp->SetMovementMode(EMovementMode::MOVE_Falling);
	}

	FHitResult ImpactResult;
	FVector AttachPoint = FVector::ZeroVector;
	float LedgeHeight = 0.0f;
	const bool bCanGrabLedge = OwningCharacter ? OwningCharacter->CanAttachToWall(ImpactResult, AttachPoint, LedgeHeight) : false;

	if (bCanGrabLedge && LedgeHeight > OwningCharacter->AutoLedgeClimbMaxHeight && LedgeHeight <= OwningCharacter->LedgeGrabMaxHeight && OwningCharacter->LedgeHangClass)
	{
		UTPP_SPM_LedgeHang* LedgeHangSPM = NewObject <UTPP_SPM_LedgeHang>(this, OwningCharacter->LedgeHangClass);
		if (LedgeHangSPM)
		{
			//LedgeHangSPM->SetLedgeHangProperties(ImpactResult, AttachPoint);
			OwningCharacter->ExecuteSpecialMove(LedgeHangSPM, true);
			return;
		}
	}
	EndSpecialMove();
}

void UTPP_SPM_WallRun::EndSpecialMove_Implementation()
{	
	if (!bWasInterrupted && OwningCharacter)
	{
		OwningCharacter->GetTPPMovementComponent()->SetMovementMode(EMovementMode::MOVE_Falling);
		OwningCharacter->SetWallMovementState(EWallMovementState::None);
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);
	}

	Super::EndSpecialMove_Implementation();
}

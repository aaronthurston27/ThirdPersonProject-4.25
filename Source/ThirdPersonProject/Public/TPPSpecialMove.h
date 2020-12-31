// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TPPSpecialMove.generated.h"

class ATPPPlayerCharacter;

/**
*
*/
UCLASS(Blueprintable, Abstract)
class THIRDPERSONPROJECT_API UTPPSpecialMove : public UObject
{
	GENERATED_BODY()

public:

	UTPPSpecialMove(const FObjectInitializer& ObjectInitializer);
	~UTPPSpecialMove();

public:

	/** True if this move should end when the set duration expires. Otherwise, it should be ended manually */
	UPROPERTY(EditDefaultsOnly)
	bool bDurationBased = false;

	UPROPERTY(EditDefaultsOnly, meta = (editCondition = "bDurationBased"))
	float Duration = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesMovementInput = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesAiming = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesSprint = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesJump = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesCrouch = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesWeaponFireOnStart = true;

protected:

	UPROPERTY(Transient)
	bool bIsWeaponFiringDisabled = false;

public:

	UFUNCTION(BlueprintPure)
	bool IsMoveBlockingWeaponFire() const { return bIsWeaponFiringDisabled; }

public:

	UPROPERTY(Transient)
	ATPPPlayerCharacter* OwningCharacter = nullptr;

protected:

	UPROPERTY(Transient)
	float TimeRemaining = 0.f;

public:

	UFUNCTION(BlueprintNativeEvent)
	void BeginSpecialMove();

	virtual void BeginSpecialMove_Implementation();

	virtual void Tick(float DeltaSeconds);

	UFUNCTION(BlueprintNativeEvent)
	void EndSpecialMove();

	virtual void EndSpecialMove_Implementation();

protected:

	void PlayAnimMontage(UAnimMontage* Montage);

	void EndAnimMontage(UAnimMontage* MontageToEnd);

	void SetAnimRootMotionMode(TEnumAsByte<ERootMotionMode::Type> NewMode);

	UFUNCTION()
	virtual void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};

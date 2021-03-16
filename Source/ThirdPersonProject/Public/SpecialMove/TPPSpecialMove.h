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
	virtual bool IsSupportedForNetworking() const override{ return true; }

public:

	/** True if this move should end when the set duration expires. Otherwise, it should be ended manually */
	UPROPERTY(EditDefaultsOnly)
	bool bDurationBased = false;

	UPROPERTY(EditDefaultsOnly, meta = (editCondition = "bDurationBased"))
	float Duration = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesMovementInput = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesCameraMovementInput = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesAiming = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesSprint = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesJump = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bDisablesCrouch = false;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool bDisablesWeaponUseOnStart = true;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool bInterruptsReload = false;

public:

	/** If true, disables character rotation forced by the player controller or movement component */
	UPROPERTY(EditDefaultsOnly, Category = "Character")
	bool bDisablesCharacterRotation = false;

protected:

	UPROPERTY(Transient, Replicated)
	bool bIsWeaponUseDisabled = false;

public:

	UFUNCTION(BlueprintPure)
	bool IsMoveBlockingWeaponUse() const { return bIsWeaponUseDisabled; }

public:

	UPROPERTY(Transient, Replicated)
	ATPPPlayerCharacter* OwningCharacter = nullptr;

protected:

	UPROPERTY(Transient, Replicated)
	float TimeRemaining = 0.f;

	UPROPERTY(Transient, Replicated)
	bool bWasInterrupted = false;

public:

	UFUNCTION(NetMulticast, Reliable)
	void BeginSpecialMove();

	virtual void BeginSpecialMove_Implementation();

	UFUNCTION(Client, Reliable)
	virtual void Client_SpecialMoveStarted();

	virtual void Tick(float DeltaSeconds);

	/** To be called if the move should end prematurely. Sets flag internally that should bypass some ending logic */
	UFUNCTION(Server, Reliable)
	void InterruptSpecialMove();

	UFUNCTION(Server, Reliable)
	void EndSpecialMove();

	virtual void EndSpecialMove_Implementation();

protected:

	// Required network scaffolding
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(NetMulticast, Reliable)
	void PlayAnimMontage(UAnimMontage* Montage, bool bShouldEndAllMontages = false);

	UFUNCTION(NetMulticast, Reliable)
	void EndAnimMontage(UAnimMontage* MontageToEnd);

	void SetAnimRootMotionMode(TEnumAsByte<ERootMotionMode::Type> NewMode);

	UFUNCTION(Client, Reliable)
	void OnRootMotionModeSet();

	UFUNCTION()
	virtual void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	virtual void OnMontageBlendOut(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION(BlueprintNativeEvent)
	void OnDurationExceeded();

	virtual void OnDurationExceeded_Implementation();
};

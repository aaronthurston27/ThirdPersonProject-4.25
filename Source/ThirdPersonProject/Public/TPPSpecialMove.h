// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPPSpecialMove.generated.h"

class AThirdPersonProjectCharacter;

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

public:

	UPROPERTY(Transient)
	AThirdPersonProjectCharacter* OwningCharacter = nullptr;

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

	UFUNCTION()
	virtual void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};

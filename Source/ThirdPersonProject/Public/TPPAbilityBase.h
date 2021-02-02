// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPPAbilityBase.generated.h"

class ATPPPlayerCharacter;

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class THIRDPERSONPROJECT_API UTPPAbilityBase : public UObject
{
	GENERATED_BODY()

public:

	UTPPAbilityBase();
	~UTPPAbilityBase();

public: 

	virtual bool ActivateAbility();

	UFUNCTION(BlueprintImplementableEvent)
	void OnAbilityActivated();

	UFUNCTION()
	void SetOwningCharacter(ATPPPlayerCharacter* Character);

	UFUNCTION(BlueprintNativeEvent)
	bool CanActivate();

	virtual bool CanActivate_Implementation();

protected:

	/** Character that owns this ability */
	UPROPERTY(BlueprintReadOnly)
	ATPPPlayerCharacter* OwningCharacter;

	/** True if the ability is playing and has reached the sweet spot for cancelling */
	UPROPERTY(Transient)
	bool bIsInSweetSpot;
};
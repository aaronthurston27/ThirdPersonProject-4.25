// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseAbility.generated.h"

class AThirdPersonProjectCharacter;

/**
 * 
 */
UCLASS(Blueprintable)
class THIRDPERSONPROJECT_API UBaseAbility : public UObject
{
	GENERATED_BODY()

public:

	UBaseAbility();
	~UBaseAbility();

public: 

	virtual bool ActivateAbility();

	UFUNCTION(BlueprintImplementableEvent)
	void OnAbilityActivated();

	UFUNCTION()
	void SetOwningCharacter(AThirdPersonProjectCharacter* Character);

	UFUNCTION(BlueprintNativeEvent)
	bool CanActivate();

	virtual bool CanActivate_Implementation();

protected:

	/** Character that owns this ability */
	UPROPERTY(BlueprintReadOnly)
	AThirdPersonProjectCharacter* OwningCharacter;

	/** True if the ability is playing and has reached the sweet spot for cancelling */
	UPROPERTY(Transient)
	bool bIsInSweetSpot;
};

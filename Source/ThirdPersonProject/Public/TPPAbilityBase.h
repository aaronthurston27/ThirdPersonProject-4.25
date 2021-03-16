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

	virtual bool IsSupportedForNetworking() const override{ return true; }

public:

	/** Ability cooldown time */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (UIMin = "0.0", ClampMin = "0.0"))
	float AbilityCooldownTime = 0.0f;

public: 

	virtual bool ActivateAbility();

	UFUNCTION(BlueprintImplementableEvent)
	void OnAbilityActivated();

	UFUNCTION()
	void SetOwningCharacter(ATPPPlayerCharacter* Character);

	UFUNCTION(BlueprintNativeEvent)
	bool CanActivate() const;

	virtual bool CanActivate_Implementation() const;

protected:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	/** Character that owns this ability */
	UPROPERTY(BlueprintReadOnly, Replicated)
	ATPPPlayerCharacter* OwningCharacter;

	/** Time since ability was last used. Used for tracking cooldown. */
	UPROPERTY(Transient, BlueprintReadOnly, Replicated)
	float LastAbilityUseTime = 0.0f;
};

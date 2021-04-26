// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TPPPlayerState.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class THIRDPERSONPROJECT_API ATPPPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	// Required network scaffolding
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:

	UPROPERTY(ReplicatedUsing = OnRep_PlayerHealth, meta = (ClampMin="0.0"))
	float PlayerHealth = 125.0f;

	UPROPERTY(Replicated)
	bool bIsPlayerAlive = true;

public:

	UFUNCTION(BlueprintCallable)
	float GetHealth() const { return PlayerHealth; }

	UFUNCTION(BlueprintCallable)
	bool IsPlayerCharacterAlive() const { return bIsPlayerAlive; }

	UFUNCTION(Server, Reliable)
	void SetPlayerHealth(float HealthDelta);

protected:

	UFUNCTION()
	void OnRep_PlayerHealth();

};

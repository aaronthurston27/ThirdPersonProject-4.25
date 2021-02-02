// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPPHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthDamaged, float, HealthLost, const FDamageEvent&, DamageEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthRestored, float, HealthGained);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDepleted);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THIRDPERSONPROJECT_API UTPPHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTPPHealthComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	/** Max health this player can have */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth;

	/** True if this component can regenerate health */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	bool bCanRegenerateHealth = true;

	/** Time delay before health begins regenerating */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (EditCondition = "bCanRegenerateHealth"))
	float HealthRegenDelay;

	/** Health to regenerate per second */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (EditCondition = "bCanRegenerateHealth"))
	float HealthRegenRate;

protected:

	/** Current health of the player */
	UPROPERTY(Transient, VisibleAnywhere)
	float Health;

	/** True if regen can begin */
	UPROPERTY(Transient)
	bool bShouldRegenHealth = false;

	/** Timer handle for starting health regen */
	UPROPERTY(Transient)
	FTimerHandle HealthRegenTimerHandle;

	/** Called when health regen timer expires and component can begin regenerating */
	UFUNCTION()
	void OnHealthRegenTimerExpired();

public:

	/** Health damaged delegate */
	FOnHealthDamaged HealthDamaged;

	/** Health restored delegated */
	FOnHealthRestored HealthRestored;

	/** Health depleted delegate */
	FOnHealthDepleted HealthDepleted;

	/** Returns current health */
	UFUNCTION(BlueprintPure)
	float GetHealth() const { return Health; }

	/** Returns true if health regen is active */
	UFUNCTION(BlueprintPure)
	bool IsRegeneratingHealth() const { return bShouldRegenHealth; }

	/** Damages health. Returns amount of health that was lost */
	UFUNCTION(BlueprintCallable)
	float DamageHealth(float HealthDamage, const FDamageEvent& DamageCausedBy, const AActor* Instigator);
	
	/** Regains health */
	void GainHealth(float HealthToGain);
};
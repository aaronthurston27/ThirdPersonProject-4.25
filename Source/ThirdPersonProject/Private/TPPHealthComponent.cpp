// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPHealthComponent.h"

// Sets default values for this component's properties
UTPPHealthComponent::UTPPHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	MaxHealth = 125.0f;
	HealthRegenDelay = 6.0f;
	HealthRegenRate = 18.0f;

	// ...
}


// Called when the game starts
void UTPPHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	bShouldRegenHealth = false;
}


// Called every frame
void UTPPHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bShouldRegenHealth)
	{
		GainHealth(DeltaTime * HealthRegenRate);
		bShouldRegenHealth = Health < MaxHealth;
	}
}

float UTPPHealthComponent::DamageHealth(float HealthDamage, const FDamageEvent& DamageCausedBy, const AActor* Instigator)
{
	const float OldHealth = Health;
	Health = FMath::Max(Health - HealthDamage, 0.0f);
	HealthDamaged.Broadcast(HealthDamage, DamageCausedBy);

	if (Health <= 0)
	{
		bShouldRegenHealth = false;
		GetWorld()->GetTimerManager().ClearTimer(HealthRegenTimerHandle);
		HealthDepleted.Broadcast();
	}
	else
	{
		FTimerManager& TimerManager = GetWorld()->GetTimerManager();
		TimerManager.ClearTimer(HealthRegenTimerHandle);
		TimerManager.SetTimer(HealthRegenTimerHandle, this, &UTPPHealthComponent::OnHealthRegenTimerExpired, HealthRegenDelay, false);
	}

	return OldHealth - Health;
}

void UTPPHealthComponent::GainHealth(float HealthToGain)
{
	const float OldHealth = Health;
	Health = FMath::Max(Health + HealthToGain, MaxHealth);
	HealthRestored.Broadcast(Health - OldHealth);
}

void UTPPHealthComponent::OnHealthRegenTimerExpired()
{
	if (Health < MaxHealth)
	{
		bShouldRegenHealth = true;
	}
}


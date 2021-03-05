// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPPPickupBase.generated.h"

class ATPPPlayerCharacter;

/*
* Base class for items that spawn in the world and can be picked up during gameplay.
*/
UCLASS(Abstract,Blueprintable)
class THIRDPERSONPROJECT_API ATPPPickupBase : public AActor
{
	GENERATED_BODY()

public:

	/** If true, this pickup can only be interacted with using the interact key */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	bool bRequiresPlayerInteractionKey = false;

	/** If true, pickup should destroy itself when picked up */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bDestroyOnPickedUp = false;

	/** Respawn time for this pickup */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "!bDestroyOnPickedUp"))
	float RespawnTime = 6.0f;

protected:

	/** True if the pickup is active and can be picked up */
	UPROPERTY(Transient, BlueprintReadOnly)
	bool bIsActive = false;

	/** Respawn timer handle */
	UPROPERTY(Transient)
	FTimerHandle RespawnTimerHandle;
	
public:	
	// Sets default values for this actor's properties
	ATPPPickupBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	virtual void NotifyActorBeginOverlap(AActor* OtherActor);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	/** Returns true if this pickup can be consumed its current state */
	UFUNCTION(BlueprintImplementableEvent)
	bool CanPickup(const ATPPPlayerCharacter* CharacterInstigator) const;

	/** Consume the pickup */
	UFUNCTION(BlueprintNativeEvent)
	void ObtainPickup(ATPPPlayerCharacter* InstigatorCharacter);

	virtual void ObtainPickup_Implementation(ATPPPlayerCharacter* InstigatorCharacter);

	/** Activates the pickup */
	void Activate();

	/** Deactivates the pickup */
	void Deactivate();

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void OnActiveStateChanged(bool bIsPickupActive);

	UFUNCTION()
	void OnRespawnTimerExpired();

};

// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/TPPPickupBase.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

// Sets default values
ATPPPickupBase::ATPPPickupBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATPPPickupBase::BeginPlay()
{
	Super::BeginPlay();
	Activate();
}

// Called every frame
void ATPPPickupBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATPPPickupBase::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (!bRequiresPlayerInteractionKey)
	{
		ATPPPlayerCharacter* CharacterOverlapped = Cast<ATPPPlayerCharacter>(OtherActor);
		if (CharacterOverlapped && CanPickup(CharacterOverlapped))
		{
			ObtainPickup(CharacterOverlapped);
		}
	}
}

void ATPPPickupBase::ObtainPickup_Implementation(ATPPPlayerCharacter* InstigatorCharacter)
{
	if (bIsActive && InstigatorCharacter)
	{
		Deactivate();

		if (bDestroyOnPickedUp)
		{
			Destroy();
		}
		else
		{
			FTimerManager& WorldTimerManager = GetWorldTimerManager();
			WorldTimerManager.ClearTimer(RespawnTimerHandle);
			WorldTimerManager.SetTimer(RespawnTimerHandle, this, &ATPPPickupBase::OnRespawnTimerExpired, RespawnTime, false);
		}
	}
}

void ATPPPickupBase::OnRespawnTimerExpired()
{
	if (this)
	{
		Activate();
	}
}

void ATPPPickupBase::Activate()
{
	bIsActive = true;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	OnActiveStateChanged(true);
}

void ATPPPickupBase::Deactivate()
{
	bIsActive = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	OnActiveStateChanged(false);
}


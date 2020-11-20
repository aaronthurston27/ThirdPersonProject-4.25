// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseEnemy.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ABaseEnemy::ABaseEnemy()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

FVector ABaseEnemy::GetLockOnLocation()
{
	return GetActorLocation();
}

// Called when the game starts or when spawned
void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();
	StartingPosition = GetActorLocation();
}

// Called every frame
void ABaseEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float yValue = Amplitude * FMath::Sin(UKismetSystemLibrary::GetGameTimeInSeconds(GetWorld()) * PeriodScale);
	FVector newPosition(StartingPosition);
	newPosition.Y+= yValue;
	SetActorLocation(newPosition);
}


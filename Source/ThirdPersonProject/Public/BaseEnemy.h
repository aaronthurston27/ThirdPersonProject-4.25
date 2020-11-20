// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseEnemy.generated.h"

UCLASS()
class THIRDPERSONPROJECT_API ABaseEnemy : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseEnemy();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool CanEnemyBeLockedOnto() const {
		return bCanBeLockedOnto;
	}

	UFUNCTION(BlueprintCallable)
	virtual FVector GetLockOnLocation();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float Amplitude;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float PeriodScale;


private:

	UPROPERTY(VisibleAnywhere)
	bool bCanBeLockedOnto;

	FVector StartingPosition;

public:	

};

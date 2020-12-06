// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TPPPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API ATPPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ATPPPlayerController(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

protected:

	// Direction the player intends to move based on keys held.
	UPROPERTY(Transient)
	FVector DesiredMovementDirection;

	UPROPERTY(Transient)
	bool bIsMovementInputEnabled = true;

public:

	void SetMovementInputEnabled(bool bIsEnabled);

	UFUNCTION(BlueprintPure)
	FVector GetDesiredMovementDirection() const { return DesiredMovementDirection.GetSafeNormal2D(); }

	/** Gets the rotation of desired movement relative to the player controller */
	UFUNCTION(BlueprintPure)
	FRotator GetRelativeControllerMovementDirection() const;

	virtual void SetupInputComponent() override;

protected:

	UFUNCTION()
	void MoveForward(float value);

	UFUNCTION()
	void MoveRight(float value);
	
};

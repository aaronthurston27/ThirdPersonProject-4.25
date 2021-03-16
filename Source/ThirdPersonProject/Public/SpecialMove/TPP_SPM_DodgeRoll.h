// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPPSpecialMove.h"
#include "Net/UnrealNetwork.h"
#include "TPP_SPM_DodgeRoll.generated.h"

class UTPPMovementComponent;

/**
 * 
 */
UCLASS(Abstract)
class THIRDPERSONPROJECT_API UTPP_SPM_DodgeRoll : public UTPPSpecialMove
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* AnimMontage;

	UPROPERTY(EditDefaultsOnly)
	float RollSpeed;

	/** Speed when coming out of the roll. Might need to replace with curve */
	UPROPERTY(EditDefaultsOnly)
	float RollRampDownSpeed;

protected:

	UPROPERTY(Transient, Replicated)
	FVector CachedRollDirection;

	UPROPERTY(Transient, Replicated)
	UTPPMovementComponent* CharacterMovementComponent;

public:

	UTPP_SPM_DodgeRoll(const FObjectInitializer& ObjectInitializer);

	virtual void BeginSpecialMove_Implementation() override;

	virtual void Tick(float DeltaTime) override;

	virtual void EndSpecialMove_Implementation() override;

protected:

	virtual void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted) override;

	// Required network scaffolding
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

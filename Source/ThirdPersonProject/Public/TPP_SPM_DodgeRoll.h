// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPPSpecialMove.h"
#include "TPP_SPM_DodgeRoll.generated.h"

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

public:

	virtual void BeginSpecialMove_Implementation() override;

	virtual void Tick(float DeltaTime) override;

	virtual void EndSpecialMove_Implementation() override;

protected:

	virtual void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted) override;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPPSpecialMove.h"
#include "TPP_SPM_Defeated.generated.h"

/**
 * Special move to play when a character is defeated
 */
UCLASS(Abstract,Blueprintable)
class THIRDPERSONPROJECT_API UTPP_SPM_Defeated : public UTPPSpecialMove
{
	GENERATED_BODY()

public:

	/** Defeated anim montage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* DeathAnim;

public:

	UTPP_SPM_Defeated(const FObjectInitializer& ObjectInitializer);

	virtual void BeginSpecialMove_Implementation() override;

	virtual void EndSpecialMove_Implementation() override;

protected:

	/** Called when death montage ends */
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};

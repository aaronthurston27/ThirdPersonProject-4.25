// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPPSpecialMove.h"
#include "TPP_RunningSlide.generated.h"

UENUM(BlueprintType)
enum class ESlideState : uint8
{
	Intro,
	Sliding,
	Exiting
};

/**
 * Special move for player sliding on the ground while moving
 */
UCLASS(Abstract)
class THIRDPERSONPROJECT_API UTPP_RunningSlide : public UTPPSpecialMove
{
	GENERATED_BODY()

public:

	UTPP_RunningSlide(const FObjectInitializer& ObjectInitializer);

	virtual void BeginSpecialMove_Implementation() override;

	virtual void EndSpecialMove_Implementation() override;

	virtual void Tick(float DeltaTime) override;

public:

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* IntroSlideAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* SlideLoopAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* ExitAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Slide Parameters")
	float MinimumStartingSlideSpeed;

protected:

	UPROPERTY(Transient)
	ESlideState CurrentSlideState;

protected:

	virtual void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted) override;

	UFUNCTION()
	void OnCharacterMovementModeChanged(ACharacter* Character, EMovementMode PreviousMode, uint8 CustomModeType);
	
};

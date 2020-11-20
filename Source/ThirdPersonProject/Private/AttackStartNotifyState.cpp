// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackStartNotifyState.h"
#include "ThirdPersonProject/ThirdPersonProjectCharacter.h"
#include "Engine.h"

void UAttackStartNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, __FUNCTION__);
}

void UAttackStartNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::NotifyEnd(MeshComp, Animation);
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, __FUNCTION__);
}

void UAttackStartNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);
}

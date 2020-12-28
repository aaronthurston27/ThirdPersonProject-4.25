// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPPWeaponBase.generated.h"

class ATPPPlayerCharacter;

/**
* Base class that all weapons (or items that can hurt people) should derive from
*/
UCLASS(Abstract, BlueprintType)
class THIRDPERSONPROJECT_API ATPPWeaponBase : public AActor
{
	GENERATED_BODY()

public:

	/** Mesh associated with this weapon */
	UPROPERTY(VisibleAnywhere, Category = "Mesh", BlueprintReadOnly)
	USkeletalMeshComponent* WeaponMesh;
	
public:	

	ATPPWeaponBase();

	virtual void BeginPlay() override;

protected:

	/** Current owner of this weapon */
	UPROPERTY(Transient)
	ATPPPlayerCharacter* CharacterOwner = nullptr;

public:

	/** Sets the owner of this weapon */
	UFUNCTION(BlueprintCallable)
	void SetWeaponOwner(ATPPPlayerCharacter* NewOwner);
};

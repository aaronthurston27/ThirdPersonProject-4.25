// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TPPAimProperties.h"
#include "TPPGameInstance.generated.h"

class ATPPHUD;
class ATPPPlayerCharacter;

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API UTPPGameInstance : public UGameInstance
{
	GENERATED_BODY()

private:

	static UTPPGameInstance* Instance;

protected:

	/** Reference to default aim properties object */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UTPPAimProperties> AimPropertiesClass;

protected:

	/** Instantiated aim properties asset */
	UPROPERTY(Transient)
	UTPPAimProperties* AimProperties;

public:

	static UTPPGameInstance* Get() { return Instance; }

	/** Init method for setting up objects in game instance */
	virtual void Init() override;

	/** Get a pointer to the player character */
	UFUNCTION(BlueprintPure)
	ATPPPlayerCharacter* GetPlayerCharacter() const;

	/** Get a pointer to the aim properties object */
	UFUNCTION(BlueprintCallable)
	UTPPAimProperties* GetAimProperties() const { return AimProperties; }
};

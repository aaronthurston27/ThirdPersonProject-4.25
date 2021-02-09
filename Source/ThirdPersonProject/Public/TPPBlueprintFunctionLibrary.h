// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TPPGameInstance.h"
#include "TPPAimProperties.h"
#include "TPPBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API UTPPBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static UTPPGameInstance* GetTPPGameInstance();

	static UTPPAimProperties* GetAimProperties();

public:

	static UDecalComponent* SpawnDecalWithParameters(UPrimitiveComponent* ComponentTarget, UMaterial* Material, const float Lifetime = 10.f,
		const FVector& AttachLocation = FVector::ZeroVector, const FRotator& Rotation = FRotator::ZeroRotator, const FVector& Scale = FVector::OneVector);

public:

	static FVector ReflectVectorOverNormal(const FVector& VectorToReflect, const FVector& NormalVector);
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPBlueprintFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

UTPPGameInstance* UTPPBlueprintFunctionLibrary::GetTPPGameInstance()
{
	return UTPPGameInstance::Get();
}

UTPPAimProperties* UTPPBlueprintFunctionLibrary::GetAimProperties()
{
	UTPPGameInstance* GI = GetTPPGameInstance();
	return GI ? GI->GetAimProperties() : nullptr;
}

UDecalComponent* UTPPBlueprintFunctionLibrary::SpawnDecalWithParameters(UPrimitiveComponent* ComponentTarget, UMaterial* Material, const float Lifetime,
	const FVector& AttachLocation, const FRotator& Rotation, const FVector& Scale)
{
	if (ComponentTarget && Material)
	{
		return UGameplayStatics::SpawnDecalAttached(Material, Scale,
			ComponentTarget, FName(TEXT("None")),
			AttachLocation, Rotation, EAttachLocation::KeepWorldPosition,
			Lifetime);
	}

	return nullptr;
}

FVector UTPPBlueprintFunctionLibrary::ReflectVectorOverNormal(const FVector& VectorToReflect, const FVector& NormalVector)
{
	const float VectorNormalDot = FVector::DotProduct(VectorToReflect.GetSafeNormal(), NormalVector.GetSafeNormal());
	const FVector VectorNormalProj = VectorNormalDot * NormalVector.GetSafeNormal();
	return VectorToReflect - (2 * VectorNormalProj);
}
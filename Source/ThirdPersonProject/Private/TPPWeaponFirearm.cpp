// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponFirearm.h"
#include "TPPSpecialMove.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

ATPPWeaponFirearm::ATPPWeaponFirearm()
{
	AudioComponent->SetWorldLocation(WeaponMesh ? WeaponMesh->GetSocketLocation(TEXT("Muzzle")) : FVector::ZeroVector);
}

void ATPPWeaponFirearm::BeginPlay()
{
	Super::BeginPlay();
}

bool ATPPWeaponFirearm::CanFireWeapon_Implementation()
{
	const UWorld* World = GetWorld();
	const float GameTimeInSeconds = World ? World->GetTimeSeconds() : 0.0f;
	return GameTimeInSeconds - TimeSinceLastShot >= WeaponFireRate && Super::CanFireWeapon_Implementation();
}

void ATPPWeaponFirearm::FireWeapon_Implementation()
{
	switch (WeaponFireType)
	{
	case EWeaponFireType::Hitscan:
		HitscanFire();
	case EWeaponFireType::Projectile:
		ProjectileFire();
	}
}

void ATPPWeaponFirearm::HitscanFire()
{
	static const float HitScanLength = 2000.f;

	UWorld* World = GetWorld();
	const UCameraComponent* PlayerCamera = CharacterOwner->GetFollowCamera();
	if (!World || !PlayerCamera)
	{
		return;
	}

	const FVector StartingLocation = PlayerCamera->GetComponentLocation();
	const FVector FireDirection = PlayerCamera->GetForwardVector();
	const FVector EndLocation = PlayerCamera->GetComponentLocation() + (FireDirection * HitScanLength);

	TArray<FHitResult> TraceResults;
	FCollisionQueryParams QueryParams(FName(TEXT("Weapon")));
	QueryParams.AddIgnoredActor(CharacterOwner);
	QueryParams.AddIgnoredActor(this);

	World->LineTraceMultiByChannel(TraceResults, StartingLocation, EndLocation, ECollisionChannel::ECC_GameTraceChannel1, QueryParams);
	const FVector EndDebugDrawLocation = TraceResults.Num() > 0 ? TraceResults[0].Location : EndLocation;
	DrawDebugLine(World, WeaponMesh->GetSocketLocation("Muzzle"), EndDebugDrawLocation, FColor::Blue, false, 1.5f, 0, 1.5f);
	if (TraceResults.Num() > 0)
	{
		DrawDebugSphere(World, TraceResults[0].Location, 25.f, 2, FColor::Green, false, 1.5f, 0, 1.5f);
	}

	if (AudioComponent && FiringSound)
	{
		AudioComponent->SetSound(FiringSound);
		AudioComponent->Play();
	}

	TimeSinceLastShot = World->GetTimeSeconds();
}

void ATPPWeaponFirearm::ProjectileFire()
{

}


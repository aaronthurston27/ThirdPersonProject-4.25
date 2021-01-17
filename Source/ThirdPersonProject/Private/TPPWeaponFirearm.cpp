// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponFirearm.h"
#include "TPPSpecialMove.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

ATPPWeaponFirearm::ATPPWeaponFirearm()
{
	AudioComponent->SetWorldLocation(WeaponMesh ? WeaponMesh->GetSocketLocation(TEXT("Muzzle")) : FVector::ZeroVector);
	bHasAmmoPool = true;
}

void ATPPWeaponFirearm::BeginPlay()
{
	Super::BeginPlay();
	LoadedAmmo = MaxLoadedAmmo;
	CurrentAmmoPool = MaxAmmoInPool;
}

bool ATPPWeaponFirearm::CanFireWeapon_Implementation()
{
	const UWorld* World = GetWorld();
	const float GameTimeInSeconds = World ? World->GetTimeSeconds() : 0.0f;
	return GameTimeInSeconds - TimeSinceLastShot >= WeaponFireRate && !bIsReloading &&
		Super::CanFireWeapon_Implementation();
}

void ATPPWeaponFirearm::FireWeapon_Implementation()
{
	if (CanFireWeapon())
	{
		switch (WeaponFireType)
		{
		case EWeaponHitType::Hitscan:
			HitscanFire();
		case EWeaponHitType::Projectile:
			ProjectileFire();
		}

		Super::FireWeapon_Implementation();
	}
	else if (LoadedAmmo <= 0 && CanReloadWeapon())
	{
		StartWeaponReload();
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

	TimeSinceLastShot = World->GetTimeSeconds();

	if (WeaponFireCharacterMontage)
	{
		CharacterOwner->SetAnimationBlendSlot(EAnimationBlendSlot::UpperBody);
		CharacterOwner->PlayAnimMontage(WeaponFireCharacterMontage);
	}

	const int32 AmmoToConsume = FMath::Min(AmmoConsumedPerShot, LoadedAmmo);
	ModifyWeaponAmmo(-AmmoConsumedPerShot, 0);
}

void ATPPWeaponFirearm::ProjectileFire()
{

}

bool ATPPWeaponFirearm::CanReloadWeapon_Implementation()
{
	return !bIsReloading && LoadedAmmo < MaxLoadedAmmo && 
		CurrentAmmoPool > 0;
}

void ATPPWeaponFirearm::SetIsReloading(bool bReloading)
{
	bIsReloading = bReloading;
}

void ATPPWeaponFirearm::StartWeaponReload()
{
	if (CharacterOwner)
	{
		SetIsReloading(true);
		ReloadActual();
		SetIsReloading(false);
	}
}

void ATPPWeaponFirearm::ReloadActual()
{
	const int32 AmmoToChamber = FMath::Min(CurrentAmmoPool, MaxLoadedAmmo);
	ModifyWeaponAmmo(AmmoToChamber, -AmmoToChamber);
	
	if (OnWeaponReloaded.IsBound())
	{
		OnWeaponReloaded.Broadcast();
	}
}


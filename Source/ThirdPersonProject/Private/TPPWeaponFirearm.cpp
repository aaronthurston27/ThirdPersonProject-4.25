// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponFirearm.h"
#include "TPPSpecialMove.h"
#include "Camera/CameraComponent.h"
#include "TPPMovementComponent.h"
#include "TPPGameInstance.h"
#include "TPPAimProperties.h"
#include "DrawDebugHelpers.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

ATPPWeaponFirearm::ATPPWeaponFirearm()
{
	AudioComponent->SetWorldLocation(WeaponMesh ? WeaponMesh->GetSocketLocation(TEXT("Muzzle")) : FVector::ZeroVector);
	bHasAmmoPool = true;

	// Tick to update weapon spread;
	PrimaryActorTick.bCanEverTick = true;
}

void ATPPWeaponFirearm::BeginPlay()
{
	Super::BeginPlay();
	LoadedAmmo = MaxLoadedAmmo;
	CurrentAmmoPool = MaxAmmoInPool;
	SetIsReloading(false);
}

void ATPPWeaponFirearm::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateWeaponSpreadRadius();
}

void ATPPWeaponFirearm::UpdateWeaponSpreadRadius()
{
	UTPPMovementComponent* MovementComponent = CharacterOwner ? CharacterOwner->GetTPPMovementComponent() : nullptr;
	UTPPGameInstance* GameInstance = Cast<UTPPGameInstance>(GetGameInstance());
	UTPPAimProperties* AimProperties = GameInstance ? GameInstance->GetAimProperties() : nullptr;
	if (!MovementComponent || !AimProperties)
	{
		return;
	}

	float SpreadRadius = AimProperties->InaccuracySpreadMaxRadius;

	const bool bIsMovingOnGround = MovementComponent->IsMovingOnGround();
	if (bIsMovingOnGround)
	{
		const bool bIsCrouching = MovementComponent->IsCrouching();
		SpreadRadius = bIsCrouching ? AimProperties->CrouchingAimSpreadRadius : AimProperties->StandingAimSpreadRadius;

		const float Speed2DSquared = MovementComponent->Velocity.Size2D();
		const float MovementPenalty = Speed2DSquared * AimProperties->MovementSpeedToWeaponSpreadRatio;

		SpreadRadius += MovementPenalty;
	}

	const bool bIsAiming = CharacterOwner->IsPlayerAiming();
	if (bIsAiming)
	{
		SpreadRadius *= AimProperties->ADSAimMultiplier;
	}

	CurrentWeaponSpreadRadius = FMath::Min(SpreadRadius, AimProperties->InaccuracySpreadMaxRadius);
}

FVector ATPPWeaponFirearm::GetWeaponInnacuracyFromSpread() const
{
	float HorizontalSpread = 0.0f;
	float VerticalSpread = 0.0f;

	// Calculate a random point in a circle. Radius is defined by the current weapon spread.
	float RandAngleRad = FMath::DegreesToRadians(FMath::RandRange(0.0f, 360.f));
	float HorizontalRadius = FMath::RandRange(0.0f, CurrentWeaponSpreadRadius);
	float VerticalRadius = FMath::RandRange(0.0f, CurrentWeaponSpreadRadius);

	HorizontalSpread = HorizontalRadius * FMath::Cos(RandAngleRad);
	VerticalSpread = VerticalRadius * FMath::Sin(RandAngleRad);

	return FVector(0.0f, HorizontalSpread, VerticalSpread);
}

bool ATPPWeaponFirearm::CanFireWeapon_Implementation()
{
	const UWorld* World = GetWorld();
	const float GameTimeInSeconds = World ? World->GetTimeSeconds() : 0.0f;
	return GameTimeInSeconds - TimeSinceLastShot >= WeaponFireRate && !bIsReloading && Super::CanFireWeapon_Implementation();
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
	const FVector WeaponInaccuracyVector = GetWeaponInnacuracyFromSpread();
	const FVector FireDirection = PlayerCamera->GetForwardVector();

	const FVector CameraEndLocation = PlayerCamera->GetComponentLocation() + (FireDirection * HitScanLength);
	const FVector ActualEndLocation = CameraEndLocation + WeaponInaccuracyVector;

	DrawDebugLine(World, StartingLocation + FVector(10.f,0.f,0.f), CameraEndLocation, FColor::Blue, false, 31.5f, 0, 1.5f);
	DrawDebugLine(World, StartingLocation + FVector(10.f,0.f,0.f), ActualEndLocation, FColor::Red, false, 31.5f, 0, 1.5f);

	TArray<FHitResult> TraceResults;
	FCollisionQueryParams QueryParams(FName(TEXT("Weapon")));
	QueryParams.AddIgnoredActor(CharacterOwner);
	QueryParams.AddIgnoredActor(this);

	World->LineTraceMultiByChannel(TraceResults, StartingLocation, ActualEndLocation, ECollisionChannel::ECC_GameTraceChannel1, QueryParams);
	const FVector EndDebugDrawLocation = TraceResults.Num() > 0 ? TraceResults[0].Location : ActualEndLocation;
	DrawDebugLine(World, WeaponMesh->GetSocketLocation("Muzzle"), EndDebugDrawLocation, FColor::Yellow, false, 1.5f, 0, 1.5f);
	if (TraceResults.Num() > 0)
	{
		DrawDebugSphere(World, TraceResults[0].Location, 25.f, 2, FColor::Green, false, 1.5f, 0, 1.5f);
	}

	TimeSinceLastShot = World->GetTimeSeconds();

	const bool bIsAiming = CharacterOwner->IsPlayerAiming();
	UAnimMontage* MontageToPlay = bIsAiming ? WeaponFireADSCharacterMontage : WeaponFireCharacterMontage;
	if (MontageToPlay)
	{
		CharacterOwner->SetAnimationBlendSlot(EAnimationBlendSlot::UpperBody);
		CharacterOwner->PlayAnimMontage(MontageToPlay);
	}

	if (WeaponFireSound)
	{
		AudioComponent->SetSound(WeaponFireSound);
		AudioComponent->Play();
	}

	const int32 AmmoToConsume = FMath::Min(AmmoConsumedPerShot, LoadedAmmo);
	ModifyWeaponAmmo(-AmmoConsumedPerShot, 0);
}

void ATPPWeaponFirearm::ProjectileFire()
{

}

bool ATPPWeaponFirearm::CanReloadWeapon_Implementation()
{
	return bIsWeaponReady && !bIsReloading && LoadedAmmo < MaxLoadedAmmo && 
		CurrentAmmoPool > 0;
}

void ATPPWeaponFirearm::StartWeaponReload()
{
	if (CharacterOwner && WeaponFireCharacterMontage)
	{
		const UAnimInstance* AnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();
		if (WeaponReloadCharacterMontage && !AnimInstance->Montage_IsPlaying(WeaponReloadCharacterMontage))
		{
			CharacterOwner->SetAnimationBlendSlot(EAnimationBlendSlot::UpperBody);
			CharacterOwner->PlayAnimMontage(WeaponReloadCharacterMontage);
		}
	}
}

void ATPPWeaponFirearm::SetIsReloading(bool bIsWeaponReloading)
{
	bIsReloading = bIsWeaponReloading;
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


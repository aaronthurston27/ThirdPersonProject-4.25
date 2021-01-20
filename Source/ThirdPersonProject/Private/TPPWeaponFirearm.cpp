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

	float SpreadRadius = AimProperties->InaccuracySpreadMaxAngle;

	const bool bIsMovingOnGround = MovementComponent->IsMovingOnGround();
	if (bIsMovingOnGround)
	{
		const bool bIsCrouching = MovementComponent->IsCrouching();
		SpreadRadius = bIsCrouching ? CrouchingAimSpreadAngle : StandingAimSpreadAngle;

		const float Speed2DSquared = MovementComponent->Velocity.Size2D();
		const float MaxSprintSpeed = MovementComponent->SprintingSpeed + 100.f;

		// Increase spread as speed approaches max.
		const float SpreadRadiusToSpeedRatio = (AimProperties->InaccuracySpreadMaxAngle - SpreadRadius) / MaxSprintSpeed;
		const float MovementPenalty = Speed2DSquared * SpreadRadiusToSpeedRatio;

		SpreadRadius += MovementPenalty;
	}

	const bool bIsAiming = CharacterOwner->IsPlayerAiming();
	if (bIsAiming)
	{
		SpreadRadius *= ADSAimMultiplier;
	}

	CurrentWeaponSpreadAngle = FMath::Min(SpreadRadius, AimProperties->InaccuracySpreadMaxAngle);
}

void ATPPWeaponFirearm::ModifyAimVectorFromSpread(FVector& AimingVector)
{
	float HorizontalSpread = 0.0f;
	float VerticalSpread = 0.0f;

	// Calculate a random angle to adjust the initial aimed vector
	float HorizontalAngleSpread = FMath::RandRange(-CurrentWeaponSpreadAngle, CurrentWeaponSpreadAngle);
	float VerticalAngleSpread = FMath::RandRange(-CurrentWeaponSpreadAngle, CurrentWeaponSpreadAngle);

	const FRotationMatrix ControllerRotationMatrix = FRotationMatrix(CharacterOwner->GetControlRotation());
	FVector Up, Right, Forward;
	ControllerRotationMatrix.GetUnitAxes(Forward, Right, Up);

	AimingVector = AimingVector.RotateAngleAxis(HorizontalAngleSpread, Up);
	AimingVector = AimingVector.RotateAngleAxis(VerticalAngleSpread, Right);
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
	static const float HitScanLength = 5000.f;

	UWorld* World = GetWorld();
	const UCameraComponent* PlayerCamera = CharacterOwner->GetFollowCamera();
	if (!World || !PlayerCamera)
	{
		return;
	}

	const FVector StartingLocation = PlayerCamera->GetComponentLocation();
	const FVector FireDirection = PlayerCamera->GetForwardVector();
	FVector WeaponInaccuracyVector = FireDirection;
	ModifyAimVectorFromSpread(WeaponInaccuracyVector);

	const FVector CameraEndLocation = PlayerCamera->GetComponentLocation() + (FireDirection * HitScanLength);
	const FVector ActualEndLocation = PlayerCamera->GetComponentLocation() + (WeaponInaccuracyVector * HitScanLength);
	DrawDebugLine(World, StartingLocation + FVector(10.f,0.f,0.f), CameraEndLocation, FColor::Blue, false, 10.5f, 0, 1.5f);
	DrawDebugLine(World, StartingLocation + FVector(10.f,0.f,0.f), ActualEndLocation, FColor::Red, false, 10.5f, 0, 1.5f);

	TArray<FHitResult> TraceResults;
	FCollisionQueryParams QueryParams(FName(TEXT("Weapon")));
	QueryParams.AddIgnoredActor(CharacterOwner);
	QueryParams.AddIgnoredActor(this);

	World->LineTraceMultiByChannel(TraceResults, StartingLocation, ActualEndLocation, ECollisionChannel::ECC_GameTraceChannel1, QueryParams);
	const FVector EndDebugDrawLocation = TraceResults.Num() > 0 ? TraceResults[0].Location : ActualEndLocation;
	DrawDebugLine(World, WeaponMesh->GetSocketLocation("Muzzle"), EndDebugDrawLocation, FColor::Yellow, false, 1.5f, 0, 1.5f);
	if (TraceResults.Num() > 0)
	{
		DrawDebugSphere(World, TraceResults[0].Location, 25.f, 2, FColor::Green, false, 10.5f, 0, 1.5f);
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


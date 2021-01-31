// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPWeaponFirearm.h"
#include "TPPSpecialMove.h"
#include "Camera/CameraComponent.h"
#include "TPPMovementComponent.h"
#include "TPPGameInstance.h"
#include "TPPAimProperties.h"
#include "TPPPlayerController.h"
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
	UTPPGameInstance* GameInstance = UTPPGameInstance::Get();
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

bool ATPPWeaponFirearm::CanFireWeapon_Implementation() const
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

bool ATPPWeaponFirearm::ShouldUseWeaponIk_Implementation() const
{
	UAnimInstance* AnimInstance = CharacterOwner ? CharacterOwner->GetMesh()->GetAnimInstance() : nullptr;
	const bool bIsPlayingReloadAnim = AnimInstance && AnimInstance->Montage_IsPlaying(WeaponReloadCharacterMontage);
	return bShouldUseLeftHandIK && bIsWeaponReady && AnimInstance && !bIsPlayingReloadAnim && CharacterOwner->GetCurrentAnimationBlendSlot() != EAnimationBlendSlot::FullBody;
}

FRotator ATPPWeaponFirearm::CalculateRecoil() const
{
	if (BurstCount < 0 || RecoilPatternEntries.Num() == 0)
	{
		return FRotator::ZeroRotator;
	}

	return RecoilPatternEntries[FMath::Min(BurstCount,RecoilPatternEntries.Num() - 1)];
}

void ATPPWeaponFirearm::HitscanFire()
{
	static const float HitScanLength = 5000.f;

	UWorld* World = GetWorld();
	const UCameraComponent* PlayerCamera = CharacterOwner ? CharacterOwner->GetFollowCamera() : nullptr;
	ATPPPlayerController* PlayerController = CharacterOwner ? CharacterOwner->GetTPPPlayerController() : nullptr;
	const UTPPGameInstance* GameInstance = UTPPGameInstance::Get();
	const UTPPAimProperties* AimProperties = GameInstance ? GameInstance->GetAimProperties() : nullptr;
	if (!World || !PlayerCamera || !PlayerController || !AimProperties)
	{
		return;
	}

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

	const float TimeInSeconds = World->GetTimeSeconds();

	BurstCount -= (int32)((TimeInSeconds - TimeSinceLastShot) / BurstRecoveryTime);
	BurstCount = FMath::Max(BurstCount, 0);
	
	const FRotator RecoilRotator = CalculateRecoil();
	PlayerController->AddCameraRecoil(RecoilRotator.Pitch);

	BurstCount = FMath::Min(++BurstCount, RecoilPatternEntries.Num() - 1);
	TimeSinceLastShot = TimeInSeconds;

	GetWorldTimerManager().ClearTimer(WeaponRecoilResetTimer);
	GetWorldTimerManager().SetTimer(WeaponRecoilResetTimer, this, &ATPPWeaponFirearm::OnWeaponRecoilReset, .15f, false);

	const FVector StartingLocation = PlayerCamera->GetComponentLocation();
	const FVector FireDirection = PlayerCamera->GetForwardVector();
	FVector WeaponInaccuracyVector = FireDirection;
	ModifyAimVectorFromSpread(WeaponInaccuracyVector);

	const FVector CameraEndLocation = PlayerCamera->GetComponentLocation() + (FireDirection * HitScanLength);
	const FVector ActualEndLocation = PlayerCamera->GetComponentLocation() + (WeaponInaccuracyVector * HitScanLength);
	//DrawDebugLine(World, StartingLocation + FVector(10.f,0.f,0.f), CameraEndLocation, FColor::Blue, false, 10.5f, 0, 1.5f);
	//DrawDebugLine(World, StartingLocation + FVector(10.f,0.f,0.f), ActualEndLocation, FColor::Red, false, 10.5f, 0, 1.5f);

	TArray<FHitResult> TraceResults;
	FCollisionQueryParams QueryParams(FName(TEXT("WeaponFire")));
	QueryParams.AddIgnoredActor(CharacterOwner);
	QueryParams.AddIgnoredActor(this);

	World->LineTraceMultiByChannel(TraceResults, StartingLocation, ActualEndLocation, ECollisionChannel::ECC_GameTraceChannel1, QueryParams);
	const FVector EndDebugDrawLocation = TraceResults.Num() > 0 ? TraceResults[0].Location : ActualEndLocation;
	//DrawDebugLine(World, WeaponMesh->GetSocketLocation("Muzzle"), EndDebugDrawLocation, FColor::Yellow, false, 1.5f, 0, 1.5f);
	if (TraceResults.Num() > 0)
	{
		const FHitResult HitTrace = TraceResults[0];
		DrawDebugSphere(World, HitTrace.Location, 25.f, 2, FColor::Green, false, 10.5f, 0, 1.5f);

		FPointDamageEvent PointDamage;
		PointDamage.ShotDirection = StartingLocation;
		PointDamage.DamageTypeClass = HitDamageClass;
		PointDamage.HitInfo = HitTrace;
		OnWeaponHit(HitTrace, PointDamage);
	}
}

void ATPPWeaponFirearm::ProjectileFire()
{

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

void ATPPWeaponFirearm::ReloadActual()
{
	const int32 AmmoToChamber = FMath::Min(CurrentAmmoPool, MaxLoadedAmmo);
	ModifyWeaponAmmo(AmmoToChamber, -AmmoToChamber);
	
	if (OnWeaponReloaded.IsBound())
	{
		OnWeaponReloaded.Broadcast();
	}
}

void ATPPWeaponFirearm::InterruptReload()
{
	const UAnimInstance* AnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();
	if (WeaponReloadCharacterMontage && AnimInstance->Montage_IsPlaying(WeaponReloadCharacterMontage))
	{
		CharacterOwner->StopAnimMontage(WeaponReloadCharacterMontage);
	}
}

void ATPPWeaponFirearm::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	Super::OnMontageEnded(Montage, bInterrupted);
}

void ATPPWeaponFirearm::OnWeaponRecoilReset()
{
	ATPPPlayerController* PlayerController = CharacterOwner ? CharacterOwner->GetTPPPlayerController() : nullptr;
	if (PlayerController)
	{
		PlayerController->ResetCameraRecoil();
	}
}

void ATPPWeaponFirearm::Equip()
{
	Super::Equip();
	
	UAnimInstance* AnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();
	AnimInstance->OnMontageEnded.AddDynamic(this, &ATPPWeaponFirearm::OnMontageEnded);
}


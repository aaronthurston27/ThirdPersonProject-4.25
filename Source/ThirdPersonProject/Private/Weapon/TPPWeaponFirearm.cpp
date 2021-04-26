// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/TPPWeaponFirearm.h"
#include "SpecialMove/TPPSpecialMove.h"
#include "Camera/CameraComponent.h"
#include "TPPMovementComponent.h"
#include "Game/TPPGameInstance.h"
#include "TPPAimProperties.h"
#include "TPPPlayerController.h"
#include "DrawDebugHelpers.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

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
	SetIsReloading(false);
}

void ATPPWeaponFirearm::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateWeaponSpreadRadius();
}

void ATPPWeaponFirearm::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATPPWeaponFirearm, CurrentFiringMode);
	DOREPLIFETIME(ATPPWeaponFirearm, TimeSinceLastShot);
	DOREPLIFETIME(ATPPWeaponFirearm, BurstCount);
	DOREPLIFETIME(ATPPWeaponFirearm, CurrentWeaponSpreadAngle);
	DOREPLIFETIME(ATPPWeaponFirearm, WeaponRecoilResetTimer);
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

void ATPPWeaponFirearm::CalculateHitscanFireVectors(FVector& StartingLocation, FVector& EndingLocation)
{
	const UCameraComponent* PlayerCamera = CharacterOwner ? CharacterOwner->GetFollowCamera() : nullptr;
	ATPPPlayerController* PlayerController = CharacterOwner ? CharacterOwner->GetTPPPlayerController() : nullptr;
	const UTPPGameInstance* GameInstance = UTPPGameInstance::Get();
	const UTPPAimProperties* AimProperties = GameInstance ? GameInstance->GetAimProperties() : nullptr;
	
	if (!PlayerCamera || !AimProperties || !PlayerController || !AimProperties)
	{

	}

	StartingLocation = PlayerController ? PlayerCamera->GetComponentLocation() : CharacterOwner->GetActorLocation();
	const FVector FireDirection = PlayerController ? PlayerController->GetReplicatedControlRotation().Vector() : CharacterOwner->GetControlRotation().Vector();
	FVector WeaponInaccuracyVector = FireDirection;
	ModifyAimVectorFromSpread(WeaponInaccuracyVector);

	EndingLocation = PlayerCamera->GetComponentLocation() + (WeaponInaccuracyVector * AimProperties->HitScanLength);
}

void ATPPWeaponFirearm::HitscanFire()
{
	UWorld* World = GetWorld();
	const UCameraComponent* PlayerCamera = CharacterOwner ? CharacterOwner->GetFollowCamera() : nullptr;
	ATPPPlayerController* PlayerController = CharacterOwner ? CharacterOwner->GetTPPPlayerController() : nullptr;
	const UTPPGameInstance* GameInstance = UTPPGameInstance::Get();
	const UTPPAimProperties* AimProperties = GameInstance ? GameInstance->GetAimProperties() : nullptr;
	if (!World || !PlayerCamera || !AimProperties || !PlayerController)
	{
		return;
	}

	const bool bIsAiming = CharacterOwner->IsPlayerAiming();
	UAnimMontage* MontageToPlay = bIsAiming ? WeaponFireADSCharacterMontage : WeaponFireCharacterMontage;
	if (MontageToPlay)
	{
		CharacterOwner->SetAnimationBlendSlot(EAnimationBlendSlot::UpperBody);
		CharacterOwner->ServerPlaySpecialMoveMontage(MontageToPlay, true);
	}

	PlayWeaponFireSound();

	const FRotator RecoilRotator = CalculateRecoil();
	if (PlayerController)
	{
		PlayerController->AddCameraRecoil(RecoilRotator.Pitch);
	}
	BurstCount = FMath::Min(++BurstCount, RecoilPatternEntries.Num() - 1);
	TimeSinceLastShot = World->GetTimeSeconds();

	GetWorldTimerManager().ClearTimer(WeaponRecoilResetTimer);
	GetWorldTimerManager().SetTimer(WeaponRecoilResetTimer, this, &ATPPWeaponFirearm::OnWeaponRecoilReset, .15f, false);

	FVector StartingLocation;
	FVector EndLocation;
	CalculateHitscanFireVectors(StartingLocation, EndLocation);

	//const FVector CameraEndLocation = PlayerCamera->GetComponentLocation() + (FireDirection * AimPropertiesHitScanLength);
	//DrawDebugLine(World, StartingLocation + FVector(10.f,0.f,0.f), CameraEndLocation, FColor::Blue, false, 10.5f, 0, 1.5f);
	//DrawDebugLine(World, StartingLocation + FVector(10.f,0.f,0.f), ActualEndLocation, FColor::Red, false, 10.5f, 0, 1.5f);

	TArray<FHitResult> TraceResults;
	FCollisionQueryParams QueryParams(FName(TEXT("WeaponFire")));
	QueryParams.AddIgnoredActor(CharacterOwner);
	QueryParams.AddIgnoredActor(this);

	World->LineTraceMultiByChannel(TraceResults, StartingLocation, EndLocation, ECollisionChannel::ECC_GameTraceChannel1, QueryParams);
	const FHitResult HitResultToUse = TraceResults.Num() > 0 ? TraceResults[0] : FHitResult();
	ServerHitscanFire(HitResultToUse);

	//DrawDebugSphere(World, HitTrace.Location, 15.f, 2, FColor::Green, false, 3.5f, 0, 1.5f);

	const FVector ParticleTrailEndLocation = HitResultToUse.Actor.IsValid() ? HitResultToUse.ImpactPoint : EndLocation;
	const FVector MuzzleLocation = WeaponMesh->GetSocketLocation("Muzzle");
	UParticleSystemComponent* ParticleSystemComp = UGameplayStatics::SpawnEmitterAtLocation(World, WeaponTrailEffect, MuzzleLocation);
	if (ParticleSystemComp)
	{
		ParticleSystemComp->SetVectorParameter(TrailTargetParam, ParticleTrailEndLocation);
	}
}

void ATPPWeaponFirearm::ServerHitscanFire_Implementation(const FHitResult& ClientHitResult)
{
	UWorld* World = GetWorld();
	const UTPPGameInstance* GameInstance = UTPPGameInstance::Get();
	const UTPPAimProperties* AimProperties = GameInstance ? GameInstance->GetAimProperties() : nullptr;
	if (!World || !AimProperties)
	{
		return;
	}

	const int32 AmmoToConsume = FMath::Min(AmmoConsumedPerShot, LoadedAmmo);
	ServerModifyWeaponAmmo(-AmmoConsumedPerShot, 0);

	BurstCount -= (int32)((World->GetTimeSeconds() - TimeSinceLastShot) / BurstRecoveryTime);
	BurstCount = FMath::Max(BurstCount, 0);

	/*

	FVector StartingLocation;
	FVector EndLocation;
	CalculateHitscanFireVectors(StartingLocation, EndLocation);

	TArray<FHitResult> TraceResults;
	FCollisionQueryParams QueryParams(FName(TEXT("WeaponFire")));
	QueryParams.AddIgnoredActor(CharacterOwner);
	QueryParams.AddIgnoredActor(this);

	World->LineTraceMultiByChannel(TraceResults, StartingLocation, EndLocation, ECollisionChannel::ECC_GameTraceChannel1, QueryParams);
	const FHitResult HitResult = TraceResults.Num() > 0 ? TraceResults[0] : FHitResult();
	if (HitResult.Actor != ClientHitResult.Actor)
	{
		ATPPPlayerCharacter* CharacterHit = Cast<ATPPPlayerCharacter>(HitResult.Actor.Get());
		if (CharacterHit)
		{
			// Calculate difference in aim the client would have had to make to actually hit the (assumed) intended target.
			const FVector ClientAimVector = (ClientHitResult.TraceEnd - ClientHitResult.TraceStart).Normalize();
			const FVector ServerAimVector = (EndLocation - StartingLocation).Normalize();

			const float PitchBetweenVectors = FMath::Asin((ServerAimVector - ClientAimVector).Z);
			const float YawBetweenVectors = FMath::Acos((ServerAimVector * ClientAimVector));
		}
	}
	*/

	ClientHitscanFired(ClientHitResult);
	ApplyWeaponPointDamage(ClientHitResult, ClientHitResult.TraceStart);
}

void ATPPWeaponFirearm::ClientHitscanFired_Implementation(const FHitResult& ClientHitResult)
{
	const ENetRole NetRole = CharacterOwner ? CharacterOwner->GetLocalRole() : ENetRole::ROLE_None;
	if (NetRole == ENetRole::ROLE_SimulatedProxy)
	{
		const FVector ParticleTrailEndLocation = ClientHitResult.Actor.IsValid() ? ClientHitResult.ImpactPoint : ClientHitResult.TraceEnd;
		const FVector MuzzleLocation = WeaponMesh->GetSocketLocation("Muzzle");
		UParticleSystemComponent* ParticleSystemComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), WeaponTrailEffect, MuzzleLocation);
		if (ParticleSystemComp)
		{
			ParticleSystemComp->SetVectorParameter(TrailTargetParam, ParticleTrailEndLocation);
		}
	}

	ATPPPlayerCharacter* CharacterHit = Cast<ATPPPlayerCharacter>(ClientHitResult.Actor.Get());
	if (NetRole != ENetRole::ROLE_None && NetRole != ENetRole::ROLE_Authority && !CharacterHit && ClientHitResult.Actor.IsValid())
	{
		SpawnWeaponImpactDecal(ClientHitResult);
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
	const int32 AmmoToChamber = FMath::Min(CurrentAmmoPool, MaxLoadedAmmo) - LoadedAmmo;
	ServerModifyWeaponAmmo(AmmoToChamber, -AmmoToChamber);
}

void ATPPWeaponFirearm::InterruptReload()
{
	USkeletalMeshComponent* SkeletalMeshComp = CharacterOwner ? CharacterOwner->GetMesh() : nullptr;
	const UAnimInstance* AnimInstance = SkeletalMeshComp ? SkeletalMeshComp->GetAnimInstance() : nullptr;
	if (AnimInstance && WeaponReloadCharacterMontage && AnimInstance->Montage_IsPlaying(WeaponReloadCharacterMontage))
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

void ATPPWeaponFirearm::ServerEquip_Implementation(ATPPPlayerCharacter* NewWeaponOwner)
{
	Super::ServerEquip_Implementation(NewWeaponOwner);

	if (CharacterOwner)
	{
		UAnimInstance* AnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->OnMontageEnded.AddDynamic(this, &ATPPWeaponFirearm::OnMontageEnded);
		}
		PrimaryActorTick.bCanEverTick = true;
	}
}

void ATPPWeaponFirearm::ClientWeaponEquipped_Implementation()
{
	Super::ClientWeaponEquipped_Implementation();
	AddActorWorldRotation(FRotator(0.0f, 90.0f, 0.0f));
}


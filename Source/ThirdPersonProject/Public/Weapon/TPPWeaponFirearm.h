// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/TPPWeaponBase.h"
#include "TPPWeaponFirearm.generated.h"

/** Hit logic to use for this weapon */
UENUM(BlueprintType)
enum class EWeaponHitType : uint8
{
	Hitscan,
	Projectile,
	MAX
};

/** Firing mode to use for this weapon */
UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	FullAuto,
	SemiAuto,
	Burst
};

/**
 * Base class for weapons that behave similar to firearms (reload, ammo pool, etc)
 */
UCLASS()
class THIRDPERSONPROJECT_API ATPPWeaponFirearm : public ATPPWeaponBase
{
	GENERATED_BODY()

public:

	ATPPWeaponFirearm();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

protected:

	// Required network scaffolding
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:

	/** Weapon firing mode */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing", BlueprintReadOnly)
	EWeaponHitType WeaponFireType = EWeaponHitType::Hitscan;

	/** Cooldown time between consective shots of this weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing", BlueprintReadOnly)
	float WeaponFireRate = .1f;

	/** Inaccuracy Multiplier when aiming down the sights */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing", BlueprintReadOnly, meta = (UIMax = "1.0", ClampMax = "1.0"))
	float ADSAimMultiplier = .40f;

	/** Inaccuracy angle to use when standing */
	UPROPERTY(EditAnywhere, Category = "Weapon|Firing|Spread", BlueprintReadWrite)
	float StandingAimSpreadAngle = 1.1;

	/** Inaccuracy angle to use when the player is crouching */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing|Spread", BlueprintReadOnly)
	float CrouchingAimSpreadAngle = .5f;

	/** Vector containing the recoil offsets to use while consecutively firing this weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing|Recoil")
	TArray<FRotator> RecoilPatternEntries;

	/** Time needed to decrease the recoil pattern index by one shot */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing|Recoil")
	float BurstRecoveryTime = .2f;

	/** Time to apply complete recoil recovery over */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Firing|Recoil")
	float RecoilRecoveryTime = .8f;

	/** True if this weapon should use IK to control the non-dominant hand. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation", BlueprintReadOnly)
	bool bShouldUseLeftHandIK = true;

	/** Weapon fire montage to be played by owning character */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation", BlueprintReadOnly)
	UAnimMontage* WeaponFireCharacterMontage;

	/** Weapon fire montage to be played when aiming. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation", BlueprintReadOnly)
	UAnimMontage* WeaponFireADSCharacterMontage;

	/** Weapon reload montage to be played by owning character */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	UAnimMontage* WeaponReloadCharacterMontage;

	/** Sound to play when firing */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Audio")
	USoundWave* WeaponFireSound;

	/** Weapon trail effect to spawn after firing */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX")
	UParticleSystem* WeaponTrailEffect = nullptr;

	/** param name for trace target location */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName TrailTargetParam = FName(TEXT("BeamEnd"));

protected:

	/** Current firing mode */
	UPROPERTY(Transient, Replicated)
	EWeaponFireMode CurrentFiringMode;

	/** Time since weapon was last fired * */
	UPROPERTY(Transient, Replicated)
	float TimeSinceLastShot = 0.0f;

	/** Recoil shot index. Used to access recoil pattern array */
	UPROPERTY(Transient, VisibleAnywhere, Replicated)
	int32 BurstCount = 0;

public:

	virtual void ServerEquip_Implementation(ATPPPlayerCharacter* NewWeaponOwner) override;

protected:

	virtual void ClientWeaponEquipped_Implementation() override;

public:

	virtual bool CanFireWeapon_Implementation() const override;

	virtual void FireWeapon_Implementation() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure)
	bool ShouldUseWeaponIk() const;

	virtual bool ShouldUseWeaponIk_Implementation() const;

protected:

	/** Line trace towards the player's camera and check for a hit. */
	void HitscanFire();

	/** Spawn a projectile from the weapon. */
	void ProjectileFire();

public:

	/** Starts the reload process and animation */
	virtual void StartWeaponReload() override;

	/** Actual reload process of adding ammo to chamber */
	void ReloadActual() override;

	virtual void InterruptReload() override;
	
	virtual void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted) override;

protected:

	/** Cached weapon angle calculated based on movement parameters. */
	UPROPERTY(Transient, Replicated)
	float CurrentWeaponSpreadAngle;

	/** Calculates weapon spread based on movement parameters */
	void UpdateWeaponSpreadRadius();

	void ModifyAimVectorFromSpread(FVector& AimingVector);

protected:

	/** Timer for resetting accumlated weapon recoil */
	UPROPERTY(Transient, Replicated)
	FTimerHandle WeaponRecoilResetTimer;

	/** Resets recoil when weapon has stopped firing after period of time */
	void OnWeaponRecoilReset();

	/** Calculates the current recoil offset of the weapon */
	FRotator CalculateRecoil() const;

public:

	UFUNCTION(BlueprintPure)
	float GetWeaponSpreadAngle() const { return CurrentWeaponSpreadAngle; }
};

// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TPPPlayerCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "TPPAbilityBase.h"
#include "TPPMovementComponent.h"
#include "Engine.h"
#include "TPPPlayerController.h"
#include "Weapon/TPPWeaponBase.h"
#include "Weapon/TPPWeaponFirearm.h"
#include "TPPHUD.h"
#include "TPPDamageType.h"
#include "DrawDebugHelpers.h"
#include "TPPBlueprintFunctionLibrary.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

ATPPPlayerCharacter::ATPPPlayerCharacter(const FObjectInitializer& ObjectInitialzer) :
	Super(ObjectInitialzer.SetDefaultSubobjectClass<UTPPMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	DefaultRotationRate = 540.f;
	SprintRotationRate = 220.f;
	ADSRotationRate = 200.0f;
	
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	MaxHealth = 125.0f;
	HealthRegenDelay = 6.0f;
	HealthRegenTime = 2.1f;
}

void ATPPPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CurrentAnimationBlendSlot = EAnimationBlendSlot::None;

		UCharacterMovementComponent* MovementComp = GetTPPMovementComponent();
		if (MovementComp)
		{
			MovementComp->RotationRate = FRotator(0.f, DefaultRotationRate, 0.f);
		}

		CurrentAbility = MovementAbilityClass ? NewObject<UTPPAbilityBase>(this, MovementAbilityClass) : nullptr;
		if (CurrentAbility)
		{
			CurrentAbility->SetOwningCharacter(this);
		}
		OnRep_CurrentAbility();

		ServerStopAiming();
		SetWallMovementState(EWallMovementState::None);

		bShouldRegenHealth = false;
		CachedHealthRegenDelta = MaxHealth / HealthRegenTime;
	}

	ATPPPlayerController* PlayerController = GetTPPPlayerController();
	if (PlayerController)
	{
		ATPPHUD* HUD = Cast<ATPPHUD>(PlayerController->GetHUD());
		if (HUD)
		{
			HUD->InitializeHUD(this);
			if (EquippedWeapon)
			{
				OnWeaponEquipped.Broadcast(EquippedWeapon);
			}
		}
	}

}

void ATPPPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPPPlayerCharacter, bIsSprinting);
	DOREPLIFETIME(ATPPPlayerCharacter, bIsAiming);

	DOREPLIFETIME(ATPPPlayerCharacter, CurrentAnimationBlendSlot);

	DOREPLIFETIME(ATPPPlayerCharacter, DefaultRotationRate);
	DOREPLIFETIME(ATPPPlayerCharacter, SprintRotationRate);
	DOREPLIFETIME(ATPPPlayerCharacter, ADSRotationRate);

	DOREPLIFETIME(ATPPPlayerCharacter, AimRotationDelta);
	DOREPLIFETIME(ATPPPlayerCharacter, ControllerRelativeMovementSpeed);

	DOREPLIFETIME(ATPPPlayerCharacter, CurrentAbility);

	DOREPLIFETIME(ATPPPlayerCharacter, WallMovementState);
	DOREPLIFETIME(ATPPPlayerCharacter, CurrentWallMovementProperties);

	DOREPLIFETIME(ATPPPlayerCharacter, EquippedWeapon);

	DOREPLIFETIME(ATPPPlayerCharacter, bShouldRegenHealth);
}

bool ATPPPlayerCharacter::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	bWroteSomething |= Channel->ReplicateSubobject(CurrentAbility, *Bunch, *RepFlags);
	
	return bWroteSomething;
}

void ATPPPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentSpecialMove)
	{
		CurrentSpecialMove->Tick(DeltaTime);
	}

	if (IsCharacterAlive())
	{
		if (IsLocallyControlled() && Controller && Controller->GetClass()->IsChildOf(ATPPPlayerController::StaticClass()))
		{
			if (bWantsToAim && !bIsAiming && CanPlayerBeginAiming())
			{
				ServerBeginAiming();
			}
			else if (!bWantsToAim && bIsAiming)
			{
				ServerStopAiming();
			}
			else if (bIsAiming && (!CurrentSpecialMove || !CurrentSpecialMove->bDisablesCharacterRotation))
			{
				ServerSetCharacterRotation(GetActorRotation());
			}

			if (bWantsToSprint)
			{
				if (!bIsSprinting && CanSprint())
				{
					ServerBeginSprint();
				}
			}
			else if (bIsSprinting && !bWantsToSprint)
			{
				ServerStopSprint();
			}

			UpdateAimRotationDelta();
			UpdateControllerRelativeMovementSpeed();

			if (!GetCharacterMovement()->IsMovingOnGround())
			{
				ServerTryBeginWallMovement();
			}
		}

		if (HasAuthority())
		{
			if (bShouldRegenHealth)
			{
				ATPPPlayerState* PS = GetTPPPlayerState();
				if (PS)
				{
					ModifyHealth(CachedHealthRegenDelta * DeltaTime);
					bShouldRegenHealth = PS->GetHealth() < MaxHealth;
				}
			}
		}
	}
}

void ATPPPlayerCharacter::SetWantsToSprint(bool bPlayerWantsToSprint)
{
	bWantsToSprint = bPlayerWantsToSprint;
}

void ATPPPlayerCharacter::ServerBeginSprint_Implementation()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		bIsSprinting = true;
		OnRep_IsSprinting();
	}
}

void ATPPPlayerCharacter::ServerStopSprint_Implementation()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		bIsSprinting = false;
		OnRep_IsSprinting();
	}
}

void ATPPPlayerCharacter::OnRep_IsSprinting()
{
	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	if (CharacterMovementComponent)
	{
		if (bIsSprinting)
		{
			CharacterMovementComponent->RotationRate = FRotator(0.f, SprintRotationRate, 0.f);
		}
		else
		{
			CharacterMovementComponent->RotationRate = FRotator(0.f, DefaultRotationRate, 0.f);
		}
	}
}

bool ATPPPlayerCharacter::CanSprint() const
{
	const bool bBlockedBySpecialMove = CurrentSpecialMove && CurrentSpecialMove->bDisablesSprint;
	const UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	const ATPPPlayerController* PC = GetTPPPlayerController();
	return !bBlockedBySpecialMove && !bIsAiming && MovementComp->IsMovingOnGround() && !bIsCrouched && PC->GetInputAxisValue(FName("FireWeapon")) < PC->FireWeaponThreshold;
}

bool ATPPPlayerCharacter::CanCrouch() const
{
	return !(CurrentSpecialMove && CurrentSpecialMove->bDisablesCrouch) && Super::CanCrouch();
}

void ATPPPlayerCharacter::Crouch(bool bIsClientSimulation)
{
	if (WallMovementState == EWallMovementState::WallLedgeHang && CurrentSpecialMove && CurrentSpecialMove->GetClass()->IsChildOf(UTPP_SPM_LedgeHang::StaticClass()))
	{
		UTPP_SPM_LedgeHang* LedgeHangSPM = Cast<UTPP_SPM_LedgeHang>(CurrentSpecialMove);
		if (LedgeHangSPM)
		{
			LedgeHangSPM->EndSpecialMove();
		}
	}
	else
	{
		UTPPMovementComponent* MovementComponent = Cast<UTPPMovementComponent>(GetCharacterMovement());
		if (MovementComponent)
		{
			if (MovementComponent->IsSliding())
			{
				MovementComponent->bWantsToCrouch = true;
			}
			else if (CanSlide())
			{
				MovementComponent->bWantsToSlide = true;
			}
			else
			{
				Super::Crouch(bIsClientSimulation);
			}
		}
	}
}

void ATPPPlayerCharacter::UnCrouch(bool bIsClientSimulation)
{
	UTPPMovementComponent* MovementComponent = Cast<UTPPMovementComponent>(GetCharacterMovement());
	if (MovementComponent)
	{
		MovementComponent->bWantsToSlide = false;
		Super::UnCrouch(bIsClientSimulation);
	}
}

void ATPPPlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	ServerStopSprint();
}

void ATPPPlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

bool ATPPPlayerCharacter::CanSlide() const
{
	UTPPMovementComponent* MovementComponent = Cast<UTPPMovementComponent>(GetCharacterMovement());
	ATPPPlayerController* PlayerController = GetTPPPlayerController();
	if (MovementComponent && PlayerController)
	{	
		return MovementComponent->CanSlide() && !PlayerController->GetDesiredMovementDirection().IsNearlyZero() &&
			!(CurrentSpecialMove && CurrentSpecialMove->bDisablesCrouch);
	}

	return false;
}

void ATPPPlayerCharacter::OnStartSlide()
{
	GetTPPPlayerController()->SetMovementInputEnabled(false);
}

void ATPPPlayerCharacter::OnEndSlide()
{
	GetTPPPlayerController()->SetMovementInputEnabled(true);
}

bool ATPPPlayerCharacter::CanJumpInternal_Implementation() const
{
	UTPPMovementComponent* MovementComponent = GetTPPMovementComponent();
	const bool bBlockedBySpecialMove = CurrentSpecialMove && CurrentSpecialMove->bDisablesJump;
	return MovementComponent && !MovementComponent->IsSliding() && !bBlockedBySpecialMove && Super::CanJumpInternal_Implementation();
}

void ATPPPlayerCharacter::AttemptToJump()
{
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp->IsMovingOnGround())
	{
		Jump();
	}
	else
	{
		const bool bCanExecuteWallKick = WallMovementState == EWallMovementState::WallLedgeHang || WallMovementState == EWallMovementState::WallRunUp;
		if (bCanExecuteWallKick)
		{
			ServerDoWallKick();
		}
	}
}

void ATPPPlayerCharacter::Landed(const FHitResult& HitResult)
{
	UTPPMovementComponent* MovementComponent = GetTPPMovementComponent();
	if (MovementComponent && MovementComponent->bWantsToCrouch && CanSlide())
	{
		MovementComponent->bWantsToCrouch = false;
		MovementComponent->bWantsToSlide = true;
	}

	bHasWallKicked = false;
	GetWorldTimerManager().ClearTimer(WallKickCooldownTimerHandle);
	if (!bIsAiming && IsCharacterAlive())
	{
		MovementComponent->RotationRate = FRotator(0.0f, DefaultRotationRate, 0.0f);
	}

	Super::Landed(HitResult);
}

UTPPMovementComponent* ATPPPlayerCharacter::GetTPPMovementComponent() const
{
	return Cast<UTPPMovementComponent>(GetCharacterMovement());
}

ATPPPlayerController* ATPPPlayerCharacter::GetTPPPlayerController() const
{
	return Cast<ATPPPlayerController>(GetController());
}

void ATPPPlayerCharacter::UpdateAimRotationDelta_Implementation()
{
	const bool bSpecialMoveDisablesAiming = CurrentSpecialMove ? CurrentSpecialMove->bDisablesAiming : false;
	const ATPPPlayerController* PC = GetTPPPlayerController();
	if (PC)
	{
		AimRotationDelta = bSpecialMoveDisablesAiming ? FRotator::ZeroRotator : (PC->GetReplicatedControlRotation() - GetActorRotation());
	}
}

void ATPPPlayerCharacter::ServerSetCharacterRotation_Implementation(const FRotator& NewRotation)
{
	SetActorRotation(NewRotation);
	SetCharacterRotation(NewRotation);
}

void ATPPPlayerCharacter::SetCharacterRotation_Implementation(const FRotator& NewRotation)
{
	SetActorRotation(NewRotation);
}

void ATPPPlayerCharacter::UpdateControllerRelativeMovementSpeed_Implementation()
{
	const FRotator YawRotation(0, GetTPPPlayerController()->GetReplicatedControlRotation().Yaw, 0);
	const FVector Velocity = GetVelocity();

	const float ForwardSpeed = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) | Velocity;
	const float RightSpeed = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) | Velocity;
	ControllerRelativeMovementSpeed = FVector(ForwardSpeed, RightSpeed, 0.0f);
}

void ATPPPlayerCharacter::ResetCameraToPlayerRotation()
{
	FRotator IntendedRotation = GetActorRotation();
	IntendedRotation.Pitch = 0.f;
	IntendedRotation.Roll = 0;

	Controller->SetControlRotation(IntendedRotation);
}

void ATPPPlayerCharacter::OnRep_CurrentAbility()
{

}

void ATPPPlayerCharacter::TryActivateAbility()
{
	if (!CurrentSpecialMove && CurrentAbility && CurrentAbility->CanActivate())
	{
		CurrentAbility->ActivateAbility();
	}
}

void ATPPPlayerCharacter::ExecuteSpecialMoveByClass(TSubclassOf<UTPPSpecialMove> SpecialMoveClass, bool bShouldInterruptCurrentMove)
{
	if (SpecialMoveClass)
	{
		UTPPSpecialMove* SpecialMove = NewObject<UTPPSpecialMove>(this, SpecialMoveClass);
		if (SpecialMove)
		{
			ExecuteSpecialMove(SpecialMove, bShouldInterruptCurrentMove);
		}
	}
}

void ATPPPlayerCharacter::ExecuteSpecialMove(UTPPSpecialMove* SpecialMove, bool bShouldInterruptCurrentMove)
{
	if (SpecialMove && !CurrentSpecialMove || bShouldInterruptCurrentMove)
	{
		if (CurrentSpecialMove && bShouldInterruptCurrentMove)
		{
			CurrentSpecialMove->InterruptSpecialMove();
		}

		CurrentSpecialMove = SpecialMove;
		CurrentSpecialMove->OwningCharacter = this;

		CurrentSpecialMove->BeginSpecialMove();
	}
}

void ATPPPlayerCharacter::OnSpecialMoveEnded(UTPPSpecialMove* SpecialMove)
{
	bool bOldSpecialMoveEnded = false;
	if (SpecialMove == CurrentSpecialMove)
	{
		bOldSpecialMoveEnded = true;
		CurrentSpecialMove = nullptr;
	}

	if (IsLocallyControlled())
	{
		if (bOldSpecialMoveEnded && DoesPlayerWantToAim() && CanPlayerBeginAiming())
		{
			if (!bIsAiming)
			{
				ServerBeginAiming();
			}
			else if (SpecialMove->bDisablesCharacterRotation)
			{
				// If the special move interrupts the needed movement rotation settings for aiming, restore the settings.
				// Usually in a standalone game, this would be handled by OnRep_IsAiming. But, (I think), since the variable hasn't changed, it won't be called?
				UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
				MovementComp->ServerSetOrientRotationToMovement(false);
				MovementComp->ServerSetUseControllerDesiredRotation(true);
			}
		}
	}
}

void ATPPPlayerCharacter::ServerPlaySpecialMoveMontage_Implementation(UAnimMontage* Montage, bool bShouldEndAllMontages)
{
	if (HasAuthority())
	{
		PlaySpecialMoveAnimMontage(Montage, bShouldEndAllMontages);
	}
}

void ATPPPlayerCharacter::PlaySpecialMoveAnimMontage_Implementation(UAnimMontage* Montage, bool bShouldEndAllMontages)
{
	if (Montage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(Montage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, bShouldEndAllMontages);
		}
	}
}

void ATPPPlayerCharacter::ServerEndAnimMontage_Implementation(UAnimMontage* Montage)
{
	if (HasAuthority())
	{
		EndAnimMontage(Montage);
	}
}

void ATPPPlayerCharacter::EndAnimMontage_Implementation(UAnimMontage* MontageToEnd)
{
	if (MontageToEnd)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Stop(0, MontageToEnd);
		}
	}
}

void ATPPPlayerCharacter::SetAnimationBlendSlot_Implementation(const EAnimationBlendSlot NewSlot)
{
	CurrentAnimationBlendSlot = NewSlot;
	OnRep_AnimationBlendSlot();
}

void ATPPPlayerCharacter::ServerSetAnimRootMotionMode_Implementation(ERootMotionMode::Type NewMode)
{
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	UAnimInstance* AnimInstance = SkeletalMesh ? SkeletalMesh->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		AnimInstance->SetRootMotionMode(NewMode);
		ClientSetRootMode(NewMode);
	}
}

void ATPPPlayerCharacter::ClientSetRootMode_Implementation(ERootMotionMode::Type NewMode)
{
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	UAnimInstance* AnimInstance = SkeletalMesh ? SkeletalMesh->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		AnimInstance->SetRootMotionMode(NewMode);
	}
}

void ATPPPlayerCharacter::OnRep_AnimationBlendSlot()
{

}

void ATPPPlayerCharacter::ServerEquipWeapon_Implementation(ATPPWeaponBase* NewEquippedWeapon)
{
	if (NewEquippedWeapon)
	{
		NewEquippedWeapon->ServerEquip(this);

		EquippedWeapon = NewEquippedWeapon;
		OnRep_EquippedWeapon();
	}
}

void ATPPPlayerCharacter::OnRep_EquippedWeapon()
{
	OnWeaponEquipped.Broadcast(EquippedWeapon);
}

void ATPPPlayerCharacter::TryToFireWeapon()
{
	if (!EquippedWeapon)
	{
		return;
	}

	if (CurrentSpecialMove && CurrentSpecialMove->IsMoveBlockingWeaponUse())
	{
		return;
	}

	if (IsSprinting())
	{
		ServerStopSprint();
	}

	EquippedWeapon->FireWeapon();
}

void ATPPPlayerCharacter::SetPlayerWantsToAim(bool bIsTryingToAim)
{
	bWantsToAim = bIsTryingToAim;
}

bool ATPPPlayerCharacter::CanPlayerBeginAiming() const
{
	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	return EquippedWeapon && (MovementComp && !MovementComp->IsSliding()) && (!CurrentSpecialMove || !CurrentSpecialMove->bDisablesAiming);
}

void ATPPPlayerCharacter::ServerBeginAiming_Implementation()
{
	bIsAiming = true;

	ServerStopSprint();

	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	MovementComp->ServerSetOrientRotationToMovement(false);

	MovementComp->RotationRate = FRotator(0.0f, ADSRotationRate, 0.0f);

	OnRep_IsAiming();
}

void ATPPPlayerCharacter::ServerStopAiming_Implementation()
{
	bIsAiming = false;

	bUseControllerRotationYaw = false;

	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	MovementComp->ServerSetOrientRotationToMovement(true);
	MovementComp->RotationRate = FRotator(0.0f, DefaultRotationRate, 0.0f);

	OnRep_IsAiming();
}

void ATPPPlayerCharacter::OnRep_IsAiming()
{
	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();

	if (bIsAiming)
	{
		FollowCamera->SetRelativeLocation(ADSCameraOffset);

		if (IsLocallyControlled() && (!CurrentSpecialMove || !CurrentSpecialMove->bDisablesCharacterRotation))
		{
			MovementComp->ServerSetUseControllerDesiredRotation(true);
		}
		MovementComp->RotationRate = FRotator(0.0f, ADSRotationRate, 0.0f);

		CameraBoom->TargetArmLength = ADSCameraArmLength;
	}
	else
	{
		FollowCamera->SetRelativeLocation(HipAimCameraOffset);
		bUseControllerRotationYaw = false;

		MovementComp->bOrientRotationToMovement = true;
		MovementComp->RotationRate = FRotator(0.0f, DefaultRotationRate, 0.0f);

		CameraBoom->TargetArmLength = HipAimCameraArmLength;
	}
}

void ATPPPlayerCharacter::TryToReloadWeapon()
{
	ATPPWeaponFirearm* WeaponFirearm = Cast<ATPPWeaponFirearm>(EquippedWeapon);
	if (!WeaponFirearm)
	{
		return;
	}

	if (CurrentSpecialMove && CurrentSpecialMove->IsMoveBlockingWeaponUse())
	{
		return;
	}

	UTPPMovementComponent* MoveComp = GetTPPMovementComponent();
	if (MoveComp->HasCharacterStartedSlide())
	{
		return;
	}

	if (WeaponFirearm->CanReloadWeapon())
	{
		WeaponFirearm->StartWeaponReload();
	}
}

void ATPPPlayerCharacter::Log(ELogLevel LoggingLevel, FString Message, ELogOutput LogOutput)
{
	// only print when screen is selected and the GEngine object is available
	if ((LogOutput == ELogOutput::ALL || LogOutput == ELogOutput::SCREEN) && GEngine)
	{
		// default color
		FColor LogColor = FColor::Cyan;
		// flip the color based on the type
		switch (LoggingLevel)
		{
		case ELogLevel::TRACE:
			LogColor = FColor::Green;
			break;
		case ELogLevel::DEBUG:
			LogColor = FColor::Cyan;
			break;
		case ELogLevel::INFO:
			LogColor = FColor::White;
			break;
		case ELogLevel::WARNING:
			LogColor = FColor::Yellow;
			break;
		case ELogLevel::ERROR:
			LogColor = FColor::Red;
			break;
		default:
			break;
		}
		// print the message and leave it on screen ( 2.5f controls the duration )
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, LogColor, Message);
	}
}

float ATPPPlayerCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	ATPPPlayerState* PS = GetTPPPlayerState();

	if (!CanBeDamaged() || !IsCharacterAlive() || !PS)
	{
		return 0.0f;
	}

	const float HealthDamaged = FMath::Min(Damage, PS->GetHealth());
	PS->SetPlayerHealth(PS->GetHealth() - HealthDamaged);

	bShouldRegenHealth = false;
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(HealthRegenTimerHandle);

	if (!PS->IsPlayerCharacterAlive())
	{
		
	}
	else
	{
		TimerManager.SetTimer(HealthRegenTimerHandle, this, &ATPPPlayerCharacter::OnHealthRegenTimerExpired, HealthRegenDelay, false);
	}

	ClientOnDamageTaken(HealthDamaged, DamageEvent, EventInstigator, DamageCauser);
	return HealthDamaged;
}

void ATPPPlayerCharacter::ClientOnDamageTaken_Implementation(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	bool bWasHitInHead = false;
	if (EventInstigator && DamageCauser)
	{
		static const FName HeadBoneName = FName(TEXT("head"));
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			FPointDamageEvent* const PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
			bWasHitInHead = PointDamageEvent->HitInfo.BoneName.IsEqual(HeadBoneName);
		}
	}

	if (Damage > 0 && IsCharacterAlive() && !CurrentSpecialMove)
	{
		UAnimMontage* HitReactMontage = bWasHitInHead ? HitReactions.HeadHitReactMontage : HitReactions.UpperBodyHitReactMontage;
		if (HitReactMontage)
		{
			PlaySpecialMoveAnimMontage(HitReactMontage);
		}
	}

	DamageReceived.Broadcast(Damage, DamageEvent);
}

void ATPPPlayerCharacter::ModifyHealth_Implementation(float HealthToGain)
{
	ATPPPlayerState* PS = GetTPPPlayerState();
	if (PS)
	{
		const float NewHealth = FMath::Max(0.0f, FMath::Min(MaxHealth, PS->GetHealth() + HealthToGain));
		PS->SetPlayerHealth(NewHealth);
	}
}

float ATPPPlayerCharacter::GetPlayerHealth() const
{
	ATPPPlayerState* PS = Cast<ATPPPlayerState>(GetPlayerState());
	return PS ? PS->GetHealth() : 0.0f;
}

void ATPPPlayerCharacter::OnHealthRegenTimerExpired()
{
	bShouldRegenHealth = true;
}

void ATPPPlayerCharacter::OnPlayerHealthDepleted()
{
	BecomeDefeated();
}

void ATPPPlayerCharacter::BecomeDefeated()
{
	ATPPPlayerController* PC = GetTPPPlayerController();
	if (PC)
	{
		DisableInput(PC);
	}

	const UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	if (MovementComp && MovementComp->IsMovingOnGround() && DeathSpecialMove)
	{
		ExecuteSpecialMoveByClass(DeathSpecialMove, true);
	}
	else
	{
		OnDeath();
	}
}

bool ATPPPlayerCharacter::IsCharacterAlive() const
{
	ATPPPlayerState* PS = Cast<ATPPPlayerState>(GetPlayerState());
	return PS ? PS->IsPlayerCharacterAlive() : false;
}

void ATPPPlayerCharacter::OnDeath()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->ServerUnequip();
		EquippedWeapon->ServerDrop(true);
		OnWeaponEquipped.Broadcast(nullptr);
	}

	BeginRagdoll();
}

void ATPPPlayerCharacter::BeginRagdoll()
{
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	CapsuleComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (SkeletalMesh)
	{
		SkeletalMesh->SetCollisionProfileName(FName(TEXT("Ragdoll")));
		SkeletalMesh->SetAllBodiesBelowSimulatePhysics(FName(TEXT("Root")), true, true);
	}

	UMovementComponent* MovementComp = GetCharacterMovement();
	MovementComp->Deactivate();
}

ATPPHUD* ATPPPlayerCharacter::GetCharacterHUD() const
{
	const ATPPPlayerController* PC = GetTPPPlayerController();
	return PC ? Cast<ATPPHUD>(PC->GetHUD()) : nullptr;
}

bool ATPPPlayerCharacter::CanPlayerWallKick(FHitResult& OutKickoffHitResult) const
{
	const ATPPPlayerController* PC = GetTPPPlayerController();
	if (!PC)
	{
		return false;
	}

	const FVector ControllerForwardVector = PC->GetControllerRelativeForwardVector(false);
	const FVector ControllerBackwardVector = -ControllerForwardVector;
	const FVector ControllerRightVector = PC->GetControllerRelativeRightVector(false);
	const FVector ControllerLeftVector = -ControllerRightVector;

	const float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector FeetLocation = GetActorLocation() - FVector(0.0f, 0.0f, 10.0f);

	UWorld* World = GetWorld();
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.MobilityType = EQueryMobilityType::Static;
	FCollisionObjectQueryParams ObjectQueryParams;

	// Check wall in front first
	{
		const FVector EndTraceLocation = FeetLocation + (ControllerForwardVector * WallKickMaxDistance);
		World->LineTraceMultiByObjectType(HitResults, FeetLocation, EndTraceLocation, ECollisionChannel::ECC_WorldStatic, QueryParams);
		//DrawDebugLine(World, FeetLocation, EndTraceLocation, FColor::Red, false, .8f, 0, .8f);
		for (const FHitResult& HitResult : HitResults)
		{
			if (HitResult.bBlockingHit)
			{
				//DrawDebugSphere(World, OutKickoffHitResult.Location, 11.0f, 4.0f, FColor::Red, false, 4.0f, 0, .8f);
				//DrawDebugLine(World, OutKickoffHitResult.Location, OutKickoffHitResult.Location + (OutKickoffHitResult.ImpactNormal * WallKickMaxDistance), FColor::Black, false, 2.0f, 0, .8f);

				const FVector FeetToWallVec = (HitResult.Location - FeetLocation).GetSafeNormal2D();
				// Take negative dot since normal and vector to kick-off should be in opposite directions
				const float FeetWallNormalDot = -FVector::DotProduct(FeetToWallVec, HitResult.ImpactNormal);
				if (FeetWallNormalDot >= WallKickNormalMinDot)
				{
					OutKickoffHitResult = HitResult;
					return true;
				}
			}
		}

		HitResults.Empty();
	}

	// Check wall behind.
	{
		const FVector EndTraceLocation = FeetLocation + (ControllerBackwardVector * WallKickMaxDistance);
		World->LineTraceMultiByObjectType(HitResults, FeetLocation, EndTraceLocation, ECollisionChannel::ECC_WorldStatic, QueryParams);

		for (const FHitResult& HitResult : HitResults)
		{
			if (HitResult.bBlockingHit)
			{
				//DrawDebugSphere(World, OutKickoffHitResult.Location, 11.0f, 4.0f, FColor::Red, false, 4.0f, 0, .8f);
				//DrawDebugLine(World, OutKickoffHitResult.Location, OutKickoffHitResult.Location + (OutKickoffHitResult.ImpactNormal * WallKickMaxDistance), FColor::Black, false, 2.0f, 0, .8f);

				const FVector FeetToWallVec = (HitResult.Location - FeetLocation).GetSafeNormal2D();
				// Take negative dot since normal and vector to kick-off should be in opposite directions
				const float FeetWallNormalDot = -FVector::DotProduct(FeetToWallVec, HitResult.ImpactNormal);
				if (FeetWallNormalDot >= WallKickNormalMinDot)
				{
					OutKickoffHitResult = HitResult;
					return true;
				}
			}
		}

		HitResults.Empty();
	}

	// Check wall to the right
	{
		const FVector EndTraceLocation = FeetLocation + (ControllerRightVector * WallKickMaxDistance);
		World->LineTraceMultiByObjectType(HitResults, FeetLocation, EndTraceLocation, ECollisionChannel::ECC_WorldStatic, QueryParams);
		//DrawDebugLine(World, FeetLocation, EndTraceLocation, FColor::Red, false, .8f, 0, .8f);
		for (const FHitResult& HitResult : HitResults)
		{
			if (HitResult.bBlockingHit)
			{
				//DrawDebugSphere(World, OutKickoffHitResult.Location, 11.0f, 4.0f, FColor::Red, false, 4.0f, 0, .8f);
				//DrawDebugLine(World, OutKickoffHitResult.Location, OutKickoffHitResult.Location + (OutKickoffHitResult.ImpactNormal * WallKickMaxDistance), FColor::Black, false, 2.0f, 0, .8f);

				const FVector FeetToWallVec = (HitResult.Location - FeetLocation).GetSafeNormal2D();
				// Take negative dot since normal and vector to kick-off should be in opposite directions
				const float FeetWallNormalDot = -FVector::DotProduct(FeetToWallVec, HitResult.ImpactNormal);
				if (FeetWallNormalDot >= WallKickNormalMinDot)
				{
					OutKickoffHitResult = HitResult;
					return true;
				}
			}
		}

		HitResults.Empty();
	}

	// Check wall to the left
	{
		const FVector EndTraceLocation = FeetLocation + (ControllerLeftVector * WallKickMaxDistance);
		World->LineTraceMultiByObjectType(HitResults, FeetLocation, EndTraceLocation, ECollisionChannel::ECC_WorldStatic, QueryParams);

		for (const FHitResult& HitResult : HitResults)
		{
			if (HitResult.bBlockingHit)
			{
				//DrawDebugSphere(World, OutKickoffHitResult.Location, 11.0f, 4.0f, FColor::Red, false, 4.0f, 0, .8f);
				//DrawDebugLine(World, OutKickoffHitResult.Location, OutKickoffHitResult.Location + (OutKickoffHitResult.ImpactNormal * WallKickMaxDistance), FColor::Black, false, 2.0f, 0, .8f);

				const FVector FeetToWallVec = (HitResult.Location - FeetLocation).GetSafeNormal2D();
				// Take negative dot since normal and vector to kick-off should be in opposite directions
				const float FeetWallNormalDot = -FVector::DotProduct(FeetToWallVec, HitResult.ImpactNormal);
				if (FeetWallNormalDot >= WallKickNormalMinDot)
				{
					OutKickoffHitResult = HitResult;
					return true;
				}
			}
		}
	}


	return false;
}

void ATPPPlayerCharacter::ServerDoWallKick_Implementation()
{
	const FHitResult WallKickHitResult = CurrentWallMovementProperties.WallTraceImpactResult;
	if (WallKickHitResult.ImpactNormal.IsNearlyZero() || !WallKickHitResult.Actor.IsValid())
	{
		return;
	}

	SetWallMovementState(EWallMovementState::None);

	// Set z to 1 for scaling by min kick off velocity so that we have some vertical gain.
	const FVector WallKickOffDirection = WallKickHitResult.ImpactNormal + FVector::UpVector;
	const UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp)
	{
		// Set the new velocity.
		const FVector WallKickVelocity = WallKickOffDirection * MinWallKickoffVelocity;
		const FVector NewVelocity = FVector(MovementComp->Velocity.X + WallKickVelocity.X,
			MovementComp->Velocity.Y + WallKickVelocity.Y,
			WallKickVelocity.Z);

		// Don't rotate the character mesh if we are aiming.
		FRotator NewRotation = GetActorRotation();
		if (!bIsAiming)
		{
			const FVector DirectionNormalizedVec = NewVelocity.GetSafeNormal2D();
			const FRotator Rotator = FRotator(0.0f, DirectionNormalizedVec.ToOrientationRotator().Yaw, 0.0f);
			NewRotation = Rotator;
		}

		bHasWallKicked = true;
		OnWallKicked(NewVelocity, NewRotation);
	}
}

void ATPPPlayerCharacter::OnWallKicked_Implementation(const FVector& NewVelocity, const FRotator& NewRotation)
{
	if (CurrentSpecialMove)
	{
		CurrentSpecialMove->EndSpecialMove();
	}

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	MovementComp->Velocity = NewVelocity;
	if (!bIsAiming)
	{
		SetActorRotation(NewRotation);
		MovementComp->RotationRate = FRotator::ZeroRotator;
	}

	GetWorldTimerManager().SetTimer(WallKickCooldownTimerHandle, this, &ATPPPlayerCharacter::OnWallKickTimerExpired, WallKickCooldownTime, false);
}

void ATPPPlayerCharacter::OnWallKickTimerExpired()
{
	if (HasAuthority())
	{
		bHasWallKicked = false;
	}
	UTPPMovementComponent* MovementComponent = GetTPPMovementComponent();
	if (MovementComponent && !bIsAiming && IsCharacterAlive())
	{
		MovementComponent->RotationRate = FRotator(0.0f, DefaultRotationRate, 0.0f);
	}
}

void ATPPPlayerCharacter::ServerTryBeginWallMovement_Implementation()
{
	if (HasAuthority())
	{
		if (WallMovementState == EWallMovementState::None)
		{
			FHitResult WallImpactResult;
			FVector TargetAttachPoint;
			float WallLedgeHeight = 0.0f;
			const bool bCanAttachToWall = CanAttachToWall(WallImpactResult, TargetAttachPoint, WallLedgeHeight);

			if (bCanAttachToWall && !WallImpactResult.ImpactNormal.IsNearlyZero())
			{
				FTPPWallMovementProps WallMoveProps;

				if (WallLedgeHeight <= AutoLedgeClimbMaxHeight && AutoLedgeClimbClass)
				{
					FVector ClimbExitPoint;
					const bool bCanClimbLedge = CanClimbUpLedge(WallImpactResult, TargetAttachPoint, ClimbExitPoint);
					if (bCanClimbLedge && AutoLedgeClimbClass)
					{
						WallMoveProps.WallTraceImpactResult = WallImpactResult;
						WallMoveProps.WallAttachPoint = TargetAttachPoint;
						WallMoveProps.WallClimbExitPoint = ClimbExitPoint;
						WallMoveProps.ClimbLateralThresholdPoint = TargetAttachPoint;

						UTPP_SPM_LedgeClimb* LedgeClimbCDO = Cast<UTPP_SPM_LedgeClimb>(AutoLedgeClimbClass.GetDefaultObject());
						if (LedgeClimbCDO)
						{
							WallMoveProps.ClimbAnimLength = LedgeClimbCDO->ClimbMontage->GetPlayLength() / LedgeClimbCDO->ClimbMontage->RateScale;
							SetWallMovementState(EWallMovementState::WallLedgeClimb, WallMoveProps);
						}
					}
				}
				else if (WallLedgeHeight > AutoLedgeClimbMaxHeight && WallLedgeHeight <= LedgeGrabMaxHeight)
				{
					WallMoveProps.WallTraceImpactResult = WallImpactResult;
					WallMoveProps.WallAttachPoint = TargetAttachPoint + WallLedgeGrabOffset;

					SetWallMovementState(EWallMovementState::WallLedgeHang, WallMoveProps);
				}
				// Just start a regular wall run if wall is to high to climb or grab ledge
				else if (!bIsWallRunCooldownActive && WallRunClass)
				{
					WallMoveProps.WallTraceImpactResult = WallImpactResult;
					WallMoveProps.WallAttachPoint = TargetAttachPoint;

					UTPP_SPM_WallRun* WallRunCDO = Cast<UTPP_SPM_WallRun>(WallRunClass.GetDefaultObject());
					const bool bDurationBasedRun = WallRunCDO->bDurationBased;
					const float MaxDistance = bDurationBasedRun ? WallRunCDO->WallRunVerticalSpeed * WallRunCDO->Duration : WallRunCDO->WallRunMaxVerticalDistance;
					// Added 15.0f to the ledge ledge grab offset to account for the wall attach trace starting at the players foot instead of the hips (center).
					const float DestinationZ = FMath::Min(WallLedgeHeight - LedgeGrabMaxHeight + 15.0f, MaxDistance);
					WallMoveProps.WallRunDestination = TargetAttachPoint + FVector::UpVector * DestinationZ;
					SetWallMovementState(EWallMovementState::WallRunUp, WallMoveProps);
					bIsWallRunCooldownActive = true;
				}
			}
		}
		else
		{
			switch (WallMovementState)
			{
			case EWallMovementState::WallLedgeClimb:
				DoLedgeClimb();
				break;
			// Ledge hang should be handled by special move logic in owning character
			case EWallMovementState::WallRunUp:
				DoWallRun();
				break;
			}
		}
	}
}

void ATPPPlayerCharacter::DoLedgeHang_Implementation()
{
	ATPPPlayerController* PC = GetTPPPlayerController();
	const FVector DesiredMovementDirection = PC ? PC->GetDesiredMovementDirection() : FVector::ZeroVector;
	if (PC && !DesiredMovementDirection.IsNearlyZero())
	{
		const FVector ControllerRelativeMovementDirection = PC->GetControllerRelativeMovementRotation().Vector();
		const float DesiredDirectionWallDot = FVector::DotProduct(ControllerRelativeMovementDirection, CurrentWallMovementProperties.WallTraceImpactResult.ImpactNormal);
		if (-DesiredDirectionWallDot >= HangToClimbInputDot)
		{
			FVector ClimbExitPoint;
			const bool bCanClimbCurrentLedge = CanClimbUpLedge(CurrentWallMovementProperties.WallTraceImpactResult, CurrentWallMovementProperties.WallAttachPoint - WallLedgeGrabOffset, ClimbExitPoint);
			if (bCanClimbCurrentLedge && LedgeClimbClass)
			{
				CurrentWallMovementProperties.WallClimbExitPoint = ClimbExitPoint;
				CurrentWallMovementProperties.ClimbLateralThresholdPoint = CurrentWallMovementProperties.WallAttachPoint - WallLedgeGrabOffset;

				UTPP_SPM_LedgeClimb* LedgeClimbCDO = Cast<UTPP_SPM_LedgeClimb>(LedgeClimbClass.GetDefaultObject());
				if (LedgeClimbCDO)
				{
					CurrentWallMovementProperties.ClimbAnimLength = LedgeClimbCDO->ClimbMontage->GetPlayLength() / LedgeClimbCDO->ClimbMontage->RateScale;
					SetWallMovementState(EWallMovementState::WallLedgeClimb, CurrentWallMovementProperties);
				}

				//SetWallMovementState(EWallMovementState::WallLedgeClimb, WallMoveProps);
			}
		}
		else if (-DesiredDirectionWallDot <= -EndHangInputDot)
		{
			SetWallMovementState(EWallMovementState::None);
		}
	}
	
}

void ATPPPlayerCharacter::DoLedgeClimb_Implementation()
{
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	if (CurrentWallMovementProperties.ClimbAnimLength > 0.0f)
	{
		const float AnimTimeRatio = CurrentWallMovementProperties.ClimbElapsedTime / CurrentWallMovementProperties.ClimbAnimLength;
		const float NewZ = FMath::Lerp(CurrentWallMovementProperties.WallAttachPoint.Z, CurrentWallMovementProperties.WallClimbExitPoint.Z, AnimTimeRatio);
		float NewX = CurrentWallMovementProperties.WallAttachPoint.X;
		float NewY = CurrentWallMovementProperties.WallAttachPoint.Y;
		if (NewZ >= CurrentWallMovementProperties.ClimbLateralThresholdPoint.Z)
		{
			if (CurrentWallMovementProperties.ClimbLateralAnimLength == 0.0f)
			{
				CurrentWallMovementProperties.ClimbLateralAnimLength = CurrentWallMovementProperties.ClimbAnimLength - CurrentWallMovementProperties.ClimbElapsedTime;
			}

			const float LateralElapsedTimeRatio = CurrentWallMovementProperties.ClimbLateralLerpElapsedTime / CurrentWallMovementProperties.ClimbLateralAnimLength;
			NewX = FMath::Lerp(CurrentWallMovementProperties.WallAttachPoint.X, CurrentWallMovementProperties.WallClimbExitPoint.X, LateralElapsedTimeRatio);
			NewY = FMath::Lerp(CurrentWallMovementProperties.WallAttachPoint.Y, CurrentWallMovementProperties.WallClimbExitPoint.Y, LateralElapsedTimeRatio);

			CurrentWallMovementProperties.ClimbLateralLerpElapsedTime += DeltaTime;
		}

		const FVector FinalPosition = FVector(NewX, NewY, NewZ);
		SetActorLocation(FinalPosition);
		ClientLedgeClimbUpdate(GetActorLocation());
	}
	else
	{
		SetWallMovementState(EWallMovementState::None);
	}

	CurrentWallMovementProperties.ClimbElapsedTime += DeltaTime;

}

void ATPPPlayerCharacter::ClientLedgeClimbUpdate_Implementation(const FVector& NewLocation)
{
	SetActorLocation(NewLocation);
}

void ATPPPlayerCharacter::DoWallRun_Implementation()
{
	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	MovementComp->Velocity = FVector(0.0f, 0.0f, WallRunVerticalSpeed);

	float CurrentZ = GetActorLocation().Z;
	if (CurrentZ >= CurrentWallMovementProperties.WallRunDestination.Z)
	{
		MovementComp->SetMovementMode(EMovementMode::MOVE_Falling);

		FHitResult ImpactResult;
		FVector AttachPoint = FVector::ZeroVector;
		float LedgeHeight = 0.0f;
		const bool bCanGrabLedge = CanAttachToWall(ImpactResult, AttachPoint, LedgeHeight);

		if (bCanGrabLedge && LedgeHeight > AutoLedgeClimbMaxHeight && LedgeHeight <= LedgeGrabMaxHeight)
		{
			FTPPWallMovementProps MoveProps = CurrentWallMovementProperties;
			MoveProps.WallAttachPoint = AttachPoint + WallLedgeGrabOffset;
			SetWallMovementState(EWallMovementState::WallLedgeHang, MoveProps);
		}
	}
}

bool ATPPPlayerCharacter::CanAttachToWall(FHitResult& WallImpactResult, FVector& AttachPoint, float& WallLedgeHeight) const
{
	const UCapsuleComponent* PlayerCapsule = GetCapsuleComponent();
	const ATPPPlayerController* PlayerController = GetTPPPlayerController();
	UWorld* World = GetWorld();
	if (!World || !PlayerCapsule || !PlayerController || PlayerController->GetDesiredMovementDirection().IsNearlyZero())
	{
		return false;
	}

	const FVector WallClingDesiredDirection = PlayerController->GetControllerRelativeMovementRotation().Vector();
	const FVector StartLocation = GetActorLocation() - FVector(0.0f,0.0f,10.0f);
	const FVector TraceEndLocation = StartLocation + (WallClingDesiredDirection * WallKickMaxDistance);

	// TODO: Replace line trace with sphere sweep. Proceed if colliding with wall and only wall. No other actors can be hit.
	FHitResult TraceResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	World->LineTraceSingleByObjectType(TraceResult, StartLocation, TraceEndLocation, ECollisionChannel::ECC_WorldStatic, QueryParams);
	//DrawDebugLine(GetWorld(), StartLocation, TraceEndLocation, FColor::Green, false, .8f, 0, .5f);
	if (TraceResult.bBlockingHit && TraceResult.Actor.IsValid())
	{
		//DrawDebugSphere(World, TraceResult.ImpactPoint, 10.0f, 5, FColor::Purple, false, 1.0f, 0, .8f);
		//DrawDebugLine(World, TraceResult.ImpactPoint, TraceResult.ImpactPoint + (TraceResult.ImpactNormal * 30.0f), FColor::Purple, false, .8f, 0, .5f);

		const float WallClingNormalDot = FVector::DotProduct(WallClingDesiredDirection, TraceResult.ImpactNormal);
		// Negate the dot product since the desired direction should be going into the wall.
		if (-WallClingNormalDot < WallKickNormalMinDot)
		{
			return false;
		}

		const AActor* TraceActor = TraceResult.Actor.Get();
		TArray<FHitResult> HitResults;
		FCollisionShape CapsuleShape;
		CapsuleShape.SetCapsule(PlayerCapsule->GetUnscaledCapsuleRadius(), PlayerCapsule->GetUnscaledCapsuleHalfHeight());
		QueryParams.AddIgnoredActor(TraceActor);

		// The capsule doesn't have to perfectly intersect with the wall, so subtract the radius so that the capsule is just touching the wall. 
		const FVector SweepEndLocation = TraceResult.ImpactPoint - (WallClingDesiredDirection * (PlayerCapsule->GetUnscaledCapsuleRadius() - 2.0f));

		World->SweepMultiByProfile(HitResults, StartLocation, SweepEndLocation, GetActorRotation().Quaternion(), PlayerCapsule->GetCollisionProfileName(), CapsuleShape, QueryParams);
		//DrawDebugCapsule(World, SweepEndLocation, PlayerCapsule->GetUnscaledCapsuleHalfHeight(), PlayerCapsule->GetUnscaledCapsuleRadius(), GetActorRotation().Quaternion(), FColor::Green, false, .9f, 0, .4f);
		for (const FHitResult& HitResult : HitResults)
		{
 			if (HitResult.bBlockingHit)
			{
				// No clear path to wall we want to cling to.
				return false;
			}
		}

		const FVector CapsuleDownwardSweepStart = SweepEndLocation + (FVector::UpVector * 1000.0f);
		FCollisionQueryParams WallHeightQueryParams;
		WallHeightQueryParams.AddIgnoredActor(this);
		FHitResult SweepResult;
		World->SweepSingleByChannel(SweepResult, CapsuleDownwardSweepStart, SweepEndLocation, GetActorRotation().Quaternion(), ECollisionChannel::ECC_WorldDynamic, CapsuleShape, WallHeightQueryParams);
		//DrawDebugSphere(World, CapsuleDownwardSweepStart, 7.0f, 4, FColor::Blue, false, 2.0f, 0, .7f);
		if (SweepResult.Actor.Get() == TraceActor)
		{
			const FVector HitLocation = SweepResult.ImpactPoint;
			FColor SphereColor = FColor::Green;

			const float WallHeightAbovePlayer = FMath::Abs(GetActorLocation().Z - SweepResult.ImpactPoint.Z);
			FVector TargetAttachPoint = SweepEndLocation;
			// Output the relevant impact point of the two wall traces. If the ledge is low enough for us to grab, return the ledge impact point. Else, return the point where a wall run/scramble should begin.
			if (WallHeightAbovePlayer <= LedgeGrabMaxHeight)
			{
				SphereColor = FColor::Yellow;
				TargetAttachPoint.Z = HitLocation.Z;
			}

			WallImpactResult = TraceResult;
			AttachPoint = TargetAttachPoint;

			//DrawDebugSphere(World, SweepResult.ImpactPoint, 6.0f, 3, SphereColor, false, 1.5f, 0, .6f);
			WallLedgeHeight = WallHeightAbovePlayer;
		}

		return true;
	}

	return false;
}

bool ATPPPlayerCharacter::CanClimbUpLedge(const FHitResult& WallHitResult, const FVector& AttachPoint, FVector& ExitPoint)
{
	const FVector DesiredExitPoint = AttachPoint + (-WallHitResult.ImpactNormal * 85.0f) + (FVector::UpVector * 99.0f);
	const UCapsuleComponent* CapsuleComp = GetCapsuleComponent();

	FHitResult SweepResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(WallHitResult.Actor.Get());
	FCollisionShape CollisionCap;
	CollisionCap.SetCapsule(CapsuleComp->GetUnscaledCapsuleRadius(), CapsuleComp->GetUnscaledCapsuleHalfHeight());
	GetWorld()->SweepSingleByChannel(SweepResult, DesiredExitPoint, DesiredExitPoint, GetActorRotation().Quaternion(), ECollisionChannel::ECC_WorldStatic, CollisionCap, QueryParams);

	//DrawDebugCapsule(GetWorld(), DesiredExitPoint, CapsuleComp->GetUnscaledCapsuleHalfHeight(), CapsuleComp->GetUnscaledCapsuleRadius(), GetActorRotation().Quaternion(), SweepResult.bBlockingHit ? FColor::Red : FColor::Green, false, 1.0f, 0.0f, .8f);
	if (!SweepResult.bBlockingHit)
	{
		ExitPoint = DesiredExitPoint;
		return true;
	}

	return false;
}

void ATPPPlayerCharacter::SetWallMovementState_Implementation(EWallMovementState NewWallMovementState, const FTPPWallMovementProps& WallMoveProps)
{
	UTPPMovementComponent* MoveComp = GetTPPMovementComponent();
	const EWallMovementState PrevState = WallMovementState;

	if (NewWallMovementState != EWallMovementState::None)
	{
		const FRotator Rotation = (-1.0f * WallMoveProps.WallTraceImpactResult.ImpactNormal).Rotation();
		ServerSetCharacterRotation(Rotation);
		SetActorLocation(WallMoveProps.WallAttachPoint);
		SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
	}

	switch (NewWallMovementState)
	{
		case EWallMovementState::None:
			if (WallMovementState == EWallMovementState::WallLedgeClimb)
			{
				MoveComp->SetMovementMode(EMovementMode::MOVE_Walking);
			}
			else
			{
				MoveComp->SetMovementMode(EMovementMode::MOVE_Falling);
			}
			break;
		case EWallMovementState::WallLedgeHang:
		case EWallMovementState::WallLedgeClimb:
			MoveComp->SetMovementMode(EMovementMode::MOVE_None);
			break;
		case EWallMovementState::WallRunUp:
			MoveComp->SetMovementMode(EMovementMode::MOVE_Flying);
			break;
	}

	WallMovementState = NewWallMovementState;
	CurrentWallMovementProperties = WallMoveProps;

	OnRep_WallMovementState(PrevState);
}

void ATPPPlayerCharacter::OnRep_WallMovementState(EWallMovementState PreviousState)
{
	if (IsLocallyControlled())
	{
		if (WallMovementState != EWallMovementState::None && CurrentSpecialMove)
		{
			CurrentSpecialMove->InterruptSpecialMove();
		}

		switch (WallMovementState)
		{
		case EWallMovementState::None:
		{	
			if (CurrentSpecialMove)
			{
			CurrentSpecialMove->EndSpecialMove();
			}
			break;
		}
		case EWallMovementState::WallLedgeClimb:
		{
			UTPP_SPM_LedgeClimb* LedgeClimbSPM = NewObject<UTPP_SPM_LedgeClimb>(this, PreviousState == EWallMovementState::WallLedgeHang ? LedgeClimbClass : AutoLedgeClimbClass);
			if (LedgeClimbSPM)
			{
				ExecuteSpecialMove(LedgeClimbSPM);
			}
			break;
		}
		case EWallMovementState::WallLedgeHang:
		{
			UTPP_SPM_LedgeHang* LedgeHangSPM = NewObject<UTPP_SPM_LedgeHang>(this, LedgeHangClass);
			if (LedgeHangSPM)
			{
				ExecuteSpecialMove(LedgeHangSPM);
			}
			break;
		}
		case EWallMovementState::WallRunUp:
		{
			UTPP_SPM_WallRun* WallRunSPM = NewObject<UTPP_SPM_WallRun>(this, WallRunClass);
			if (WallRunSPM)
			{
				ExecuteSpecialMove(WallRunSPM);
			}
			break;
		}
		}
	}
}

void ATPPPlayerCharacter::ServerAttachToWall_Implementation(const FVector& WallAttachPoint)
{
	SetActorLocation(WallAttachPoint);
}

void ATPPPlayerCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	const EMovementMode MovementMode = GetCharacterMovement()->MovementMode;
	if (MovementMode == EMovementMode::MOVE_Walking)
	{
		bIsWallRunCooldownActive = false;
	}
}

void ATPPPlayerCharacter::OnRep_PlayerState()
{

}

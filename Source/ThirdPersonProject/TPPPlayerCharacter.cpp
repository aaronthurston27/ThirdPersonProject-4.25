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

	HealthComponent = CreateDefaultSubobject<UTPPHealthComponent>(TEXT("HealthComponent"));

	DefaultRotationRate = 540.f;
	SprintRotationRate = 220.f;
	ADSRotationRate = 200.0f;
	
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
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
	}

	ATPPPlayerController* PlayerController = GetTPPPlayerController();
	if (PlayerController)
	{
		ATPPHUD* HUD = Cast<ATPPHUD>(PlayerController->GetHUD());
		if (HUD)
		{
			HUD->InitializeHUD(this);
		}
	}

	OnRep_CurrentAbility();

	HealthComponent->HealthDepleted.AddDynamic(this, &ATPPPlayerCharacter::OnPlayerHealthDepleted);
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
	DOREPLIFETIME(ATPPPlayerCharacter, CurrentSpecialMove);
}

bool ATPPPlayerCharacter::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	bWroteSomething |= Channel->ReplicateSubobject(CurrentSpecialMove, *Bunch, *RepFlags);
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
		if (GetLocalRole() == ENetRole::ROLE_AutonomousProxy)
		{
			if (bWantsToAim && !bIsAiming && CanPlayerBeginAiming())
			{
				ServerBeginAiming();
			}
			else if (!bWantsToAim && bIsAiming)
			{
				ServerStopAiming();
			}

			if (bWantsToSprint && !bIsSprinting && CanSprint())
			{
				ServerBeginSprint();
			}
			else if (bIsSprinting && !bWantsToSprint)
			{
				ServerStopSprint();
			}

			UpdateAimRotationDelta();
			UpdateControllerRelativeMovementSpeed();
		}

		if (GetCharacterMovement()->IsFalling() && !CurrentSpecialMove && WallMovementState == EWallMovementState::None)
		{
			FHitResult WallImpactResult;
			FVector TargetAttachPoint;
			float WallLedgeHeight = 0.0f;
			const bool bCanAttachToWall = CanAttachToWall(WallImpactResult, TargetAttachPoint, WallLedgeHeight);

			if (bCanAttachToWall && !WallImpactResult.ImpactNormal.IsNearlyZero())
			{
				WallTraceImpactResult = WallImpactResult;

				if (WallLedgeHeight <= AutoLedgeClimbMaxHeight && AutoLedgeClimbClass)
				{
					FVector ClimbExitPoint;
					const bool bCanClimbLedge = CanClimbUpLedge(WallImpactResult, TargetAttachPoint, ClimbExitPoint);
					UTPP_SPM_LedgeClimb* LedgeClimbSPM = bCanClimbLedge ? NewObject<UTPP_SPM_LedgeClimb>(this, AutoLedgeClimbClass) : nullptr;
					if (LedgeClimbSPM)
					{
						LedgeClimbSPM->SetClimbProperties(WallImpactResult, TargetAttachPoint, ClimbExitPoint);
						ExecuteSpecialMove(LedgeClimbSPM);
					}
				}
				else if (WallLedgeHeight > AutoLedgeClimbMaxHeight && WallLedgeHeight <= LedgeGrabMaxHeight && LedgeHangClass)
				{
					UTPP_SPM_LedgeHang* LedgeHangSPM = NewObject<UTPP_SPM_LedgeHang>(this, LedgeHangClass);
					if (LedgeHangSPM)
					{
						LedgeHangSPM->SetLedgeHangProperties(WallImpactResult, TargetAttachPoint);
						ExecuteSpecialMove(LedgeHangSPM);
					}
				}
				// Just start a regular wall run if wall is to high to climb or grab ledge
				else if (WallRunClass && !bIsWallRunCooldownActive)
				{
					UTPP_SPM_WallRun* WallRunSPM = NewObject<UTPP_SPM_WallRun>(this, WallRunClass);
					if (WallRunSPM)
					{
						WallRunSPM->SetWallRunProperties(WallImpactResult, TargetAttachPoint, WallLedgeHeight);
						ExecuteSpecialMove(WallRunSPM);
						bIsWallRunCooldownActive = CurrentSpecialMove != nullptr;
					}
				}
			}
		}
		else if (WallMovementState != EWallMovementState::None)
		{
			switch (WallMovementState)
			{
			case EWallMovementState::WallLedgeHang:
				break;
			case EWallMovementState::WallRunUp:
				break;

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
			if (CurrentSpecialMove)
			{
				CurrentSpecialMove->EndSpecialMove();
			}

			DoWallKick(WallTraceImpactResult);
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
	AimRotationDelta = bSpecialMoveDisablesAiming ? FRotator::ZeroRotator : (GetTPPPlayerController()->GetReplicatedControlRotation() - GetActorRotation());
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

void ATPPPlayerCharacter::TryActivateAbility()
{
	if (!CurrentSpecialMove && CurrentAbility && CurrentAbility->CanActivate())
	{
		ServerBeginMovementAbility();
	}
}

bool ATPPPlayerCharacter::ServerBeginMovementAbility_Validate()
{
	return true;
}

void ATPPPlayerCharacter::ServerBeginMovementAbility_Implementation()
{
	if (CurrentAbility && !CurrentSpecialMove)
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

bool ATPPPlayerCharacter::ExecuteSpecialMove_Validate(UTPPSpecialMove* SpecialMove, bool bShouldInterruptCurrentMove)
{
	return true;
}

void ATPPPlayerCharacter::ExecuteSpecialMove_Implementation(UTPPSpecialMove* SpecialMove, bool bShouldInterruptCurrentMove)
{
	if (SpecialMove && !CurrentSpecialMove || bShouldInterruptCurrentMove)
	{
		if (CurrentSpecialMove && bShouldInterruptCurrentMove)
		{
			CurrentSpecialMove->InterruptSpecialMove();
		}
		CurrentSpecialMove = SpecialMove;
		CurrentSpecialMove->OwningCharacter = this;
		OnRep_SpecialMove();
		CurrentSpecialMove->BeginSpecialMove();
	}
}

void ATPPPlayerCharacter::OnRep_SpecialMove()
{
	if (CurrentSpecialMove)
	{
		UE_LOG(LogTemp, Warning, TEXT("Testing"));
	}
}

void ATPPPlayerCharacter::OnRep_CurrentAbility()
{
	if (CurrentAbility)
	{
		UE_LOG(LogTemp, Warning, TEXT("Testing2: %d"), GetLocalRole());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("kasnmd"));
	}
}

void ATPPPlayerCharacter::OnSpecialMoveEnded_Implementation(UTPPSpecialMove* SpecialMove)
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
			ServerBeginAiming();
		}
	}
}

void ATPPPlayerCharacter::SetAnimationBlendSlot(const EAnimationBlendSlot NewSlot)
{
	if (HasAuthority())
	{
		CurrentAnimationBlendSlot = NewSlot;
		OnRep_AnimationBlendSlot();
	}
}

void ATPPPlayerCharacter::OnRep_AnimationBlendSlot()
{

}

void ATPPPlayerCharacter::EquipWeapon(ATPPWeaponBase* NewEquippedWeapon)
{
	if (NewEquippedWeapon)
	{
		NewEquippedWeapon->SetWeaponOwner(this);
		NewEquippedWeapon->Equip();
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, false);
		NewEquippedWeapon->WeaponMesh->AttachToComponent(GetMesh(), AttachmentRules, WeaponAttachmentSocketName);
		NewEquippedWeapon->AddActorWorldRotation(FRotator(0.0f, 90.0f, 0.0f));
	}

	CurrentWeapon = NewEquippedWeapon;
	OnWeaponEquipped.Broadcast(CurrentWeapon);
}

void ATPPPlayerCharacter::TryToFireWeapon()
{
	if (!CurrentWeapon)
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

	CurrentWeapon->FireWeapon();
}

void ATPPPlayerCharacter::SetPlayerWantsToAim(bool bIsTryingToAim)
{
	bWantsToAim = bIsTryingToAim;
}

bool ATPPPlayerCharacter::CanPlayerBeginAiming() const
{
	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	return CurrentWeapon && (MovementComp && !MovementComp->IsSliding()) && (!CurrentSpecialMove || !CurrentSpecialMove->bDisablesAiming);
}

void ATPPPlayerCharacter::ServerBeginAiming_Implementation()
{
	bIsAiming = true;
	OnRep_IsAiming();
}

void ATPPPlayerCharacter::ServerStopAiming_Implementation()
{
	bIsAiming = false;
	OnRep_IsAiming();
}

void ATPPPlayerCharacter::OnRep_IsAiming()
{
	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();

	if (bIsAiming)
	{
		FollowCamera->SetRelativeLocation(ADSCameraOffset);
		ServerStopSprint();

		MovementComp->bOrientRotationToMovement = false;
		if (!CurrentSpecialMove || !CurrentSpecialMove->bDisablesCharacterRotation)
		{
			MovementComp->bUseControllerDesiredRotation = true;
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
	ATPPWeaponFirearm* WeaponFirearm = Cast<ATPPWeaponFirearm>(CurrentWeapon);
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

	if (LogOutput == ELogOutput::ALL || LogOutput == ELogOutput::OUTPUT_LOG)
	{
		// flip the message type based on error level
		switch (LoggingLevel)
		{
		case ELogLevel::TRACE:
			UE_LOG(LogTemp, VeryVerbose, TEXT("%s"), *Message);
			break;
		case ELogLevel::DEBUG:
			UE_LOG(LogTemp, Verbose, TEXT("%s"), *Message);
			break;
		case ELogLevel::INFO:
			UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
			break;
		case ELogLevel::WARNING:
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
			break;
		case ELogLevel::ERROR:
			UE_LOG(LogTemp, Error, TEXT("%s"), *Message);
			break;
		default:
			UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
			break;
		}
	}
}

float ATPPPlayerCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (!CanBeDamaged() || !IsCharacterAlive())
	{
		return 0.0f;
	}

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

	float HealthDamaged = 0.0f;

	if (HealthComponent)
	{
		HealthDamaged =  HealthComponent->DamageHealth(Damage, DamageEvent, DamageCauser);

		if (HealthDamaged > 0 && HealthComponent->GetHealth() > 0 && !CurrentSpecialMove)
		{
			UAnimMontage* HitReactMontage = bWasHitInHead ? HitReactions.HeadHitReactMontage : HitReactions.UpperBodyHitReactMontage;
			if (HitReactMontage)
			{
				PlayAnimMontage(HitReactMontage);
			}
		}
	}

	return HealthDamaged;
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
	return HealthComponent ? HealthComponent->GetHealth() > 0 : false;
}

void ATPPPlayerCharacter::OnDeath()
{
	HealthComponent->PrimaryComponentTick.bCanEverTick = false;
	if (CurrentWeapon)
	{
		CurrentWeapon->Drop(true);
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

void ATPPPlayerCharacter::DoWallKick(const FHitResult& WallKickHitResult)
{
	if (WallKickHitResult.ImpactNormal.IsNearlyZero() || !WallKickHitResult.Actor.IsValid())
	{
		return;
	}

	if (CurrentSpecialMove)
	{
		CurrentSpecialMove->EndSpecialMove();
	}
	UE_LOG(LogTemp, Warning, TEXT("Direction: %s"), *WallKickHitResult.ImpactNormal.ToString());
	// Set z to 1 for scaling by min kick off velocity so that we have some vertical gain.
	const FVector WallKickOffDirection = WallKickHitResult.ImpactNormal + FVector::UpVector;

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp)
	{
		// Set the new velocity.
		const FVector WallKickVelocity = WallKickOffDirection * MinWallKickoffVelocity;
		const FVector NewVelocity = FVector(MovementComp->Velocity.X + WallKickVelocity.X,
			MovementComp->Velocity.Y + WallKickVelocity.Y,
			WallKickVelocity.Z);
		MovementComp->Velocity = NewVelocity;

		bHasWallKicked = true;

		// Don't rotate the character mesh if we are aiming.
		if (!bIsAiming)
		{
			const FVector DirectionNormalizedVec = NewVelocity.GetSafeNormal2D();
			const FRotator Rotator = FRotator(0.0f, DirectionNormalizedVec.ToOrientationRotator().Yaw, 0.0f);
			SetActorRotation(Rotator);

			MovementComp->RotationRate = FRotator::ZeroRotator;
		}

		GetWorldTimerManager().SetTimer(WallKickCooldownTimerHandle, this, &ATPPPlayerCharacter::OnWallKickTimerExpired, WallKickCooldownTime, false);
	}
}

void ATPPPlayerCharacter::OnWallKickTimerExpired()
{
	bHasWallKicked = false;
	UTPPMovementComponent* MovementComponent = GetTPPMovementComponent();
	if (MovementComponent && !bIsAiming && IsCharacterAlive())
	{
		MovementComponent->RotationRate = FRotator(0.0f, DefaultRotationRate, 0.0f);
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

		// The capsule doesn't have to perfectly interset with the wall, so subtract the radius so that the capsule is just touching the wall. 
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

void ATPPPlayerCharacter::SetWallMovementState(EWallMovementState NewWallMovementState)
{
	WallMovementState = NewWallMovementState;
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

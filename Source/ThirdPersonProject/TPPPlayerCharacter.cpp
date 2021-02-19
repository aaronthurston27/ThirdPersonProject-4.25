

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
#include "TPPWeaponBase.h"
#include "TPPWeaponFirearm.h"
#include "TPPHUD.h"
#include "TPPDamageType.h"
#include "DrawDebugHelpers.h"
#include "TPPBlueprintFunctionLibrary.h"
#include "GameFramework/SpringArmComponent.h"

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
}

void ATPPPlayerCharacter::BeginPlay()
{
	CurrentAnimationBlendSlot = EAnimationBlendSlot::None;

	CurrentAbility = MovementAbilityClass ? NewObject<UTPPAbilityBase>(this, MovementAbilityClass) : nullptr;
	if (CurrentAbility)
	{
		CurrentAbility->SetOwningCharacter(this);
	}

	UCharacterMovementComponent* MovementComp = GetTPPMovementComponent();
	if (MovementComp)
	{
		MovementComp->RotationRate = FRotator(0.f, DefaultRotationRate, 0.f);
	}

	StopAiming();

	ATPPPlayerController* PlayerController = GetTPPPlayerController();
	if (PlayerController)
	{
		ATPPHUD* HUD = Cast<ATPPHUD>(PlayerController->GetHUD());
		if (HUD)
		{
			HUD->InitializeHUD(this);
		}
	}

	HealthComponent->HealthDepleted.AddDynamic(this, &ATPPPlayerCharacter::OnPlayerHealthDepleted);

	Super::BeginPlay();
}

void ATPPPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentSpecialMove)
	{
		CurrentSpecialMove->Tick(DeltaTime);
	}

	if (bWantsToAim && !bIsAiming && CanPlayerBeginAiming())
	{
		StartAiming();
	}
	else if (!bWantsToAim && bIsAiming)
	{
		StopAiming();
	}

	if (bWantsToSprint && !bIsSprinting && CanSprint())
	{
		BeginSprint();
	}
	else if (bIsSprinting && !bWantsToSprint)
	{
		StopSprint();
	}

	if (GetCharacterMovement()->IsFalling() && !CurrentSpecialMove && WallMovementState == EWallMovementState::None)
	{
		FHitResult WallImpactResult;
		FVector WallAttachPoint;
		const float WallLedgeHeight = GetDesiredWallLedgeHeight(WallImpactResult, WallAttachPoint);
		if (WallLedgeHeight > 0.0f && !WallImpactResult.ImpactNormal.IsNearlyZero())
		{
			if (WallLedgeHeight <= AutoLedgeClimbMaxHeight && AutoLedgeClimbClass)
			{
				FVector ClimbExitPoint;
				bool bCanClimbLedge = CanClimbUpLedge(WallImpactResult, WallAttachPoint, ClimbExitPoint);
				UTPP_SPM_LedgeClimb* LedgeClimbSPM = bCanClimbLedge ? NewObject<UTPP_SPM_LedgeClimb>(this, AutoLedgeClimbClass) : nullptr;

				if (LedgeClimbSPM)
				{
					LedgeClimbSPM->SetLedgeClimbParameters(WallImpactResult, WallAttachPoint, ClimbExitPoint);
					ExecuteSpecialMove(LedgeClimbSPM);
				}
			}
			else if (WallLedgeHeight > AutoLedgeClimbMaxHeight && WallLedgeHeight <= LedgeGrabMaxHeight)
			{
				BeginWallLedgeGrab(WallImpactResult, WallAttachPoint);
			}
			else if (WallLedgeHeight > LedgeGrabMaxHeight)
			{
				// TODO: Wall run logic? Idk, I'm fucking tired.
			}
		}
	}
	else if (WallMovementState != EWallMovementState::None)
	{
		ATPPPlayerController* PC = GetTPPPlayerController();
		switch (WallMovementState)
		{
			case EWallMovementState::WallCling:
				const FVector DesiredMovementDirection = PC->GetRelativeControllerMovementRotation().Vector();
				if (FVector::DotProduct(DesiredMovementDirection, GetActorRotation().Vector()) >= .8f && !CurrentSpecialMove)
				{
					//ExecuteSpecialMoveByClass(LedgeClimbClass);
				}
				break;
		}
	}
}

void ATPPPlayerCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ATPPPlayerCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ATPPPlayerCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ATPPPlayerCharacter::SetWantsToSprint(bool bPlayerWantsToSprint)
{
	bWantsToSprint = bPlayerWantsToSprint;
}

void ATPPPlayerCharacter::BeginSprint()
{
	bIsSprinting = true;
	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->RotationRate = FRotator(0.f, SprintRotationRate, 0.f);
	}
}

void ATPPPlayerCharacter::StopSprint()
{
	bIsSprinting = false;
	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->RotationRate = FRotator(0.f, DefaultRotationRate, 0.f);
	}
}

bool ATPPPlayerCharacter::CanSprint() const
{
	const bool bBlockedBySpecialMove = CurrentSpecialMove && CurrentSpecialMove->bDisablesSprint;
	const UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	const ATPPPlayerController* PC = GetTPPPlayerController();
	return !bBlockedBySpecialMove && !bIsAiming && MovementComp->IsMovingOnGround() && !bIsCrouched && PC->GetInputAxisValue(FName("FireWeapon")) < PC->FireWeaponThreshold;
}

void ATPPPlayerCharacter::TryToDash()
{

}

bool ATPPPlayerCharacter::CanDash() const
{
	return false;
}

bool ATPPPlayerCharacter::CanCrouch() const
{
	return !(CurrentSpecialMove && CurrentSpecialMove->bDisablesCrouch) && Super::CanCrouch();
}

void ATPPPlayerCharacter::Crouch(bool bIsClientSimulation)
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
	StopSprint();
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

void ATPPPlayerCharacter::TryJump()
{
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp->IsMovingOnGround())
	{
		Jump();
	}
	else if (WallMovementState == EWallMovementState::WallCling)
	{
		EndWallLedgeGrab();
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

FRotator ATPPPlayerCharacter::GetAimRotationDelta() const
{
	const bool bSpecialMoveDisablesAiming = CurrentSpecialMove ? CurrentSpecialMove->bDisablesAiming : false;
	return bSpecialMoveDisablesAiming ? FRotator::ZeroRotator :(GetControlRotation() - GetActorRotation());
}

void ATPPPlayerCharacter::ResetCameraToPlayerRotation()
{
	FRotator IntendedRotation = GetActorRotation();
	IntendedRotation.Pitch = 0.f;
	IntendedRotation.Roll = 0;

	Controller->SetControlRotation(IntendedRotation);
}

void ATPPPlayerCharacter::BeginMovementAbility()
{
	if (CurrentAbility && CurrentAbility->CanActivate() && !CurrentSpecialMove)
	{
		CurrentAbility->ActivateAbility();
	}
}

void ATPPPlayerCharacter::ExecuteSpecialMoveByClass(TSubclassOf<UTPPSpecialMove> SpecialMoveClass)
{
	if (SpecialMoveClass)
	{
		UTPPSpecialMove* SpecialMove = NewObject<UTPPSpecialMove>(this, SpecialMoveClass);
		if (SpecialMove)
		{
			ExecuteSpecialMove(SpecialMove);
		}
	}
}

void ATPPPlayerCharacter::ExecuteSpecialMove(UTPPSpecialMove* SpecialMove)
{
	if (SpecialMove)
	{
		CurrentSpecialMove = SpecialMove;
		CurrentSpecialMove->OwningCharacter = this;
		CurrentSpecialMove->BeginSpecialMove();
	}
}

void ATPPPlayerCharacter::OnSpecialMoveEnded(UTPPSpecialMove* SpecialMove)
{
	if (SpecialMove == CurrentSpecialMove)
	{
		CurrentSpecialMove = nullptr;
		if (DoesPlayerWantToAim() && CanPlayerBeginAiming())
		{
			StartAiming();
		}
	}
}

void ATPPPlayerCharacter::SetAnimationBlendSlot(const EAnimationBlendSlot NewSlot)
{
	CurrentAnimationBlendSlot = NewSlot;
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
		StopSprint();
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

void ATPPPlayerCharacter::StartAiming()
{
	bIsAiming = true;
	FollowCamera->SetRelativeLocation(ADSCameraOffset);
	StopSprint();
	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	if (MovementComp)
	{
		MovementComp->bOrientRotationToMovement = false;
		if (!CurrentSpecialMove || !CurrentSpecialMove->bDisablesCharacterRotation)
		{
			MovementComp->bUseControllerDesiredRotation = true;
		}
		MovementComp->RotationRate = FRotator(0.0f, ADSRotationRate, 0.0f);
		
	}
	CameraBoom->TargetArmLength = ADSCameraArmLength;
}

void ATPPPlayerCharacter::StopAiming()
{
	bIsAiming = false;
	FollowCamera->SetRelativeLocation(HipAimCameraOffset);
	bUseControllerRotationYaw = false;
	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	if (MovementComp)
	{
		MovementComp->bOrientRotationToMovement = true;
		MovementComp->RotationRate = FRotator(0.0f, DefaultRotationRate, 0.0f);
	}
	CameraBoom->TargetArmLength = HipAimCameraArmLength;
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

FVector ATPPPlayerCharacter::GetControllerRelativeMovementSpeed() const
{
	const FRotator YawRotation(0, GetControlRotation().Yaw, 0);
	const FVector Velocity = GetVelocity();

	const float ForwardSpeed = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) | Velocity;
	const float RightSpeed = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) | Velocity;
	return FVector(ForwardSpeed, RightSpeed, 0.0f);
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
	UTPPDamageType* DamageType = Cast<UTPPDamageType>(DamageEvent.DamageTypeClass.GetDefaultObject());

	if (!CanBeDamaged() || !IsCharacterAlive() || !DamageType)
	{
		return 0.0f;
	}

	bool bWasHitInHead = false;
	if (EventInstigator && DamageCauser)
	{
		static const FName HeadBoneName = FName(TEXT("head"));
		const FPointDamageEvent* PointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);
		if (PointDamage && PointDamage->HitInfo.BoneName == HeadBoneName)
		{
			bWasHitInHead = true;
			Damage *= DamageType->DamageHeadshotMultiplier;
		}
	}

	float HealthDamaged = 0.0f;

	if (HealthComponent)
	{
		HealthDamaged =  HealthComponent->DamageHealth(Damage, DamageEvent, DamageCauser);

		if (HealthDamaged > 0 && HealthComponent->GetHealth() > 0)
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
	const UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	if (MovementComp && MovementComp->IsMovingOnGround() && DeathSpecialMove)
	{
		ExecuteSpecialMoveByClass(DeathSpecialMove);
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

FVector ATPPPlayerCharacter::CalculateWallKickDirection(const FHitResult& HitResult) const
{
	return HitResult.ImpactNormal;
}

void ATPPPlayerCharacter::DoWallKick(const FHitResult& WallKickHitResult)
{
	FVector WallKickOffDirection = CalculateWallKickDirection(WallKickHitResult);

	const ATPPPlayerController* PC = GetTPPPlayerController();
	FVector DesiredMoveDirection = FVector::ZeroVector;
	if (PC && !PC->GetDesiredMovementDirection().IsNearlyZero())
	{
		DesiredMoveDirection = PC->GetRelativeControllerMovementRotation().Vector();
	}
	// Set a z of 1 so that we can kick with some vertical velocity.
	DesiredMoveDirection.Z = 1.0f;

	const FVector FinalWallKickDirection = WallKickOffDirection + DesiredMoveDirection;

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp)
	{
		// Set the new velocity.
		const FVector WallKickVelocity = FinalWallKickDirection * MinWallKickoffVelocity;
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

float ATPPPlayerCharacter::GetDesiredWallLedgeHeight(FHitResult& WallImpactResult, FVector& AttachPoint) const
{
	const UCapsuleComponent* PlayerCapsule = GetCapsuleComponent();
	const ATPPPlayerController* PlayerController = GetTPPPlayerController();
	UWorld* World = GetWorld();
	if (!World || !PlayerCapsule || !PlayerController || PlayerController->GetDesiredMovementDirection().IsNearlyZero())
	{
		return -1.0f;
	}

	const FVector WallClingDesiredDirection = PlayerController->GetRelativeControllerMovementRotation().Vector();
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
		DrawDebugSphere(World, TraceResult.ImpactPoint, 10.0f, 5, FColor::Purple, false, 1.0f, 0, .8f);
		//DrawDebugLine(World, TraceResult.ImpactPoint, TraceResult.ImpactPoint + (TraceResult.ImpactNormal * 30.0f), FColor::Purple, false, .8f, 0, .5f);

		const float WallClingNormalDot = FVector::DotProduct(WallClingDesiredDirection, TraceResult.ImpactNormal);
		// Negate the dot product since the desired direction should be going into the wall.
		if (-WallClingNormalDot < WallKickNormalMinDot)
		{
			return -1.0f;
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
				return -1.0f;
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
			FVector WallAttachPoint = SweepEndLocation;
			// Output the relevant impact point of the two wall traces. If the ledge is low enough for us to grab, return the ledge impact point. Else, return the point where a wall run/scramble should begin.
			if (WallHeightAbovePlayer <= LedgeGrabMaxHeight)
			{
				SphereColor = FColor::Yellow;
				WallAttachPoint.Z = HitLocation.Z;
			}

			WallImpactResult = TraceResult;
			AttachPoint = WallAttachPoint;

			DrawDebugSphere(World, SweepResult.ImpactPoint, 6.0f, 3, SphereColor, false, 1.5f, 0, .6f);
			UE_LOG(LogTemp, Warning, TEXT("Wall height: %f"), WallHeightAbovePlayer);
			return WallHeightAbovePlayer;
		}
	}

	return -1.0f;
}

bool ATPPPlayerCharacter::CanClimbUpLedge(const FHitResult& WallHitResult, const FVector& AttachPoint, FVector& ExitPoint)
{
	const FVector DesiredExitPoint = AttachPoint + (-WallHitResult.ImpactNormal * 90.0f) + (FVector::UpVector * 96.0f);
	const UCapsuleComponent* CapsuleComp = GetCapsuleComponent();

	FHitResult SweepResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(WallHitResult.Actor.Get());
	FCollisionShape CollisionCap;
	CollisionCap.SetCapsule(CapsuleComp->GetUnscaledCapsuleRadius(), CapsuleComp->GetUnscaledCapsuleHalfHeight());
	GetWorld()->SweepSingleByChannel(SweepResult, DesiredExitPoint, DesiredExitPoint, GetActorRotation().Quaternion(), ECollisionChannel::ECC_WorldStatic, CollisionCap, QueryParams);

	DrawDebugCapsule(GetWorld(), DesiredExitPoint, CapsuleComp->GetUnscaledCapsuleHalfHeight(), CapsuleComp->GetUnscaledCapsuleRadius(), GetActorRotation().Quaternion(), SweepResult.bBlockingHit ? FColor::Red : FColor::Green, false, 1.0f, 0.0f, .8f);
	if (!SweepResult.bBlockingHit)
	{
		ExitPoint = DesiredExitPoint;
		return true;
	}

	return false;
}

void ATPPPlayerCharacter::BeginWallLedgeGrab(FHitResult& WallTraceImpactPoint, FVector& WallAttachPoint)
{
	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	if (!MovementComp)
	{
		return;
	}

	MovementComp->SetMovementMode(EMovementMode::MOVE_None);

	SetActorLocation(WallAttachPoint - WallLedgeGrabOffset);
	const FRotator Rotation = (-1.0f * WallTraceImpactPoint.ImpactNormal).Rotation();
	SetActorRotation(Rotation);

	SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
	WallMovementState = EWallMovementState::WallCling;
}

void ATPPPlayerCharacter::EndWallLedgeGrab()
{
	WallMovementState = EWallMovementState::None;

	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	if (MovementComp)
	{
		MovementComp->SetMovementMode(EMovementMode::MOVE_Falling);
	}
}

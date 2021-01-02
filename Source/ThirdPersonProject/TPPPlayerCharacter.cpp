

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
#include "BaseAbility.h"
#include "TPPMovementComponent.h"
#include "Engine.h"
#include "TPPPlayerController.h"
#include "TPPWeaponBase.h"
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

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	
	PrimaryActorTick.bCanEverTick = true;

	DefaultWalkSpeed = 400.f;
	ADSWalkSpeed = 250.f;
	CrouchingSpeed = 250.f;
	SprintingSpeed = 1150.f;
	DefaultRotationRate = 540.f;
	SprintRotationRate = 220.f;
}

void ATPPPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentAnimationBlendSlot = EAnimationBlendSlot::None;

	CurrentAbility = MovementAbilityClass ? NewObject<UBaseAbility>(this, MovementAbilityClass) : nullptr;
	if (CurrentAbility)
	{
		CurrentAbility->SetOwningCharacter(this);
	}

	StopAiming();
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
		CharacterMovementComponent->MaxWalkSpeed = SprintingSpeed;
	}
}

void ATPPPlayerCharacter::StopSprint()
{
	bIsSprinting = false;
	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->MaxWalkSpeed = DefaultWalkSpeed;
		CharacterMovementComponent->RotationRate = FRotator(0.f, DefaultRotationRate, 0.f);
	}
}

bool ATPPPlayerCharacter::CanSprint() const
{
	const bool bBlockedBySpecialMove = CurrentSpecialMove && CurrentSpecialMove->bDisablesSprint;
	const UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	return !bBlockedBySpecialMove && !bIsAiming && MovementComp->IsMovingOnGround();
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
	UE_LOG(LogTemp, Warning, TEXT("Ending slide"));
	GetTPPPlayerController()->SetMovementInputEnabled(true);
}

bool ATPPPlayerCharacter::CanJumpInternal_Implementation() const
{
	UTPPMovementComponent* MovementComponent = GetTPPMovementComponent();
	const bool bBlockedBySpecialMove = CurrentSpecialMove && CurrentSpecialMove->bDisablesJump;
	return MovementComponent && !MovementComponent->IsSliding() && !bBlockedBySpecialMove && Super::CanJumpInternal_Implementation();
}

void ATPPPlayerCharacter::Landed(const FHitResult& HitResult)
{
	UTPPMovementComponent* MovementComponent = GetTPPMovementComponent();
	if (MovementComponent && MovementComponent->bWantsToCrouch && CanSlide())
	{
		MovementComponent->bWantsToCrouch = false;
		MovementComponent->bWantsToSlide = true;
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

void ATPPPlayerCharacter::ExecuteSpecialMove(TSubclassOf<UTPPSpecialMove> SpecialMoveToExecute)
{
	if (SpecialMoveToExecute)
	{
		CurrentSpecialMove = NewObject<UTPPSpecialMove>(this, SpecialMoveToExecute);
		if (CurrentSpecialMove)
		{
			CurrentSpecialMove->OwningCharacter = this;
			CurrentSpecialMove->BeginSpecialMove();
		}
	}
}

void ATPPPlayerCharacter::OnSpecialMoveEnded(UTPPSpecialMove* SpecialMove)
{
	if (SpecialMove == CurrentSpecialMove)
	{
		CurrentSpecialMove = nullptr;
	}
}

void ATPPPlayerCharacter::SetAnimationBlendSlot(const EAnimationBlendSlot NewSlot)
{
	CurrentAnimationBlendSlot = NewSlot;
}

void ATPPPlayerCharacter::SetCurrentEquippedWeapon(ATPPWeaponBase* NewEquippedWeapon)
{
	CurrentWeapon = NewEquippedWeapon;
}

void ATPPPlayerCharacter::TryToFireWeapon()
{
	if (!CurrentWeapon || !CurrentWeapon->CanFireWeapon())
	{
		return;
	}

	if (CurrentSpecialMove && CurrentSpecialMove->IsMoveBlockingWeaponFire())
	{
		return;
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
	bUseControllerRotationYaw = true;
	StopSprint();
	UTPPMovementComponent* MovementComp = GetTPPMovementComponent();
	if (MovementComp)
	{
		MovementComp->bOrientRotationToMovement = false;
		MovementComp->MaxWalkSpeed = ADSWalkSpeed;
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
		MovementComp->MaxWalkSpeed = DefaultWalkSpeed;
	}
	CameraBoom->TargetArmLength = HipAimCameraArmLength;
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

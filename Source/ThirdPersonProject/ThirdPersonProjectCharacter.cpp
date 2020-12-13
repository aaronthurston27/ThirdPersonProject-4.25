

// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonProjectCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "ThirdPersonHUD.h"
#include "BaseAbility.h"
#include "TPPMovementComponent.h"
#include "Engine.h"
#include "TPPPlayerController.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// AThirdPersonProjectCharacter

AThirdPersonProjectCharacter::AThirdPersonProjectCharacter(const FObjectInitializer& ObjectInitialzer) :
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

	CharacterAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("PunchAudioComponent"));
	
	FAttachmentTransformRules Rules(EAttachmentRule::KeepRelative, false);
	CharacterAudioComponent->AttachToComponent(RootComponent, Rules);

	DefaultSpeed = 400.f;
	CrouchingSpeed = 250.f;
	SprintingSpeed = 1150.f;
	DefaultRotationRate = 540.f;
	SprintRotationRate = 220.f;
}

void AThirdPersonProjectCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentAnimationBlendSlot = EAnimationBlendSlot::None;

	CurrentAbility = MovementAbilityClass ? NewObject<UBaseAbility>(this, MovementAbilityClass) : nullptr;
	if (CurrentAbility)
	{
		CurrentAbility->SetOwningCharacter(this);
	}
}

void AThirdPersonProjectCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (CurrentSpecialMove)
	{
		CurrentSpecialMove->Tick(DeltaTime);
	}
}

void AThirdPersonProjectCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AThirdPersonProjectCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AThirdPersonProjectCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AThirdPersonProjectCharacter::RotateSideways(float value)
{
	AddControllerYawInput(value);
}

void AThirdPersonProjectCharacter::RotateUpwards(float value)
{
	AddControllerPitchInput(value);
}

void AThirdPersonProjectCharacter::BeginSprint()
{
	bWantsToSprint = true;
	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->RotationRate = FRotator(0.f, SprintRotationRate, 0.f);
		CharacterMovementComponent->MaxWalkSpeed = SprintingSpeed;
	}
}

void AThirdPersonProjectCharacter::StopSprint()
{
	bWantsToSprint = false;
	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->MaxWalkSpeed = DefaultSpeed;
		CharacterMovementComponent->RotationRate = FRotator(0.f, DefaultRotationRate, 0.f);
	}
}

bool AThirdPersonProjectCharacter::CanSprint() const
{
	const bool bBlockedBySpecialMove = CurrentSpecialMove && CurrentSpecialMove->bDisablesSprint;
	return !bBlockedBySpecialMove;
}

bool AThirdPersonProjectCharacter::CanCrouch() const
{
	return !(CurrentSpecialMove && CurrentSpecialMove->bDisablesCrouch) && Super::CanCrouch();
}

void AThirdPersonProjectCharacter::Crouch(bool bIsClientSimulation)
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

void AThirdPersonProjectCharacter::UnCrouch(bool bIsClientSimulation)
{
	UTPPMovementComponent* MovementComponent = Cast<UTPPMovementComponent>(GetCharacterMovement());
	if (MovementComponent)
	{
		MovementComponent->bWantsToSlide = false;
		Super::UnCrouch(bIsClientSimulation);
	}
}

void AThirdPersonProjectCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AThirdPersonProjectCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

bool AThirdPersonProjectCharacter::CanSlide() const
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

void AThirdPersonProjectCharacter::OnStartSlide()
{
	GetTPPPlayerController()->SetMovementInputEnabled(false);
}

void AThirdPersonProjectCharacter::OnEndSlide()
{
	GetTPPPlayerController()->SetMovementInputEnabled(true);
}

bool AThirdPersonProjectCharacter::CanJumpInternal_Implementation() const
{
	UTPPMovementComponent* MovementComponent = GetTPPMovementComponent();
	const bool bBlockedBySpecialMove = CurrentSpecialMove && CurrentSpecialMove->bDisablesJump;
	return MovementComponent && !MovementComponent->IsSliding() && !bBlockedBySpecialMove && Super::CanJumpInternal_Implementation();
}

void AThirdPersonProjectCharacter::Landed(const FHitResult& HitResult)
{
	UTPPMovementComponent* MovementComponent = GetTPPMovementComponent();
	if (MovementComponent && MovementComponent->bWantsToCrouch && CanSlide())
	{
		MovementComponent->bWantsToCrouch = false;
		MovementComponent->bWantsToSlide = true;
	}

	Super::Landed(HitResult);
}

UTPPMovementComponent* AThirdPersonProjectCharacter::GetTPPMovementComponent() const
{
	return Cast<UTPPMovementComponent>(GetCharacterMovement());
}

ATPPPlayerController* AThirdPersonProjectCharacter::GetTPPPlayerController() const
{
	return Cast<ATPPPlayerController>(GetController());
}

FRotator AThirdPersonProjectCharacter::GetAimRotationDelta() const
{
	const bool bSpecialMoveDisablesAiming = CurrentSpecialMove ? CurrentSpecialMove->bDisablesAiming : false;
	return bSpecialMoveDisablesAiming ? FRotator::ZeroRotator :(GetControlRotation() - GetActorRotation());
}

void AThirdPersonProjectCharacter::OnLockOnPressed()
{
	ResetCameraToPlayerRotation();
}

void AThirdPersonProjectCharacter::ResetCameraToPlayerRotation()
{
	FRotator IntendedRotation = GetActorRotation();
	IntendedRotation.Pitch = 0.f;
	IntendedRotation.Roll = 0;

	Controller->SetControlRotation(IntendedRotation);
}

void AThirdPersonProjectCharacter::BeginMovementAbility()
{
	if (CurrentAbility && CurrentAbility->CanActivate() && !CurrentSpecialMove)
	{
		CurrentAbility->ActivateAbility();
	}
}

void AThirdPersonProjectCharacter::ExecuteSpecialMove(TSubclassOf<UTPPSpecialMove> SpecialMoveToExecute)
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

void AThirdPersonProjectCharacter::OnSpecialMoveEnded(UTPPSpecialMove* SpecialMove)
{
	if (SpecialMove == CurrentSpecialMove)
	{
		CurrentSpecialMove = nullptr;
	}
}

void AThirdPersonProjectCharacter::SetAnimationBlendSlot(const EAnimationBlendSlot NewSlot)
{
	CurrentAnimationBlendSlot = NewSlot;
}

bool AThirdPersonProjectCharacter::IsCharacterLockedOn()
{
	return EnemyLockedOnTo != NULL;
}

void AThirdPersonProjectCharacter::RotateToTargetEnemy()
{
	if (EnemyLockedOnTo)
	{
		const FVector VectorToEnemy = (EnemyLockedOnTo->GetLockOnLocation() - GetActorLocation()).GetSafeNormal();

		FRotator TargetRotation = VectorToEnemy.Rotation();
		FRotator NewRotation = GetControlRotation();
		NewRotation = UKismetMathLibrary::RLerp(NewRotation, TargetRotation, LockOnRotationLerp, true);
		NewRotation.Roll = 0;
		Controller->SetControlRotation(NewRotation);
		NewRotation.Pitch = 0;
		SetActorRotation(NewRotation);

		if (TargetingHud)
		{
			TargetingHud->UpdateTargetLocation(EnemyLockedOnTo->GetLockOnLocation());
		}
	}
}

void AThirdPersonProjectCharacter::MoveLockOnCamera()
{
	float timelinePlaybackPosition = CameraLockOnTimeline.GetPlaybackPosition() / CameraLockOnTimeline.GetTimelineLength();
	FVector targetCameraPosition = UKismetMathLibrary::VLerp(FVector::ZeroVector, LockOnTarget, timelinePlaybackPosition);
	CameraBoom->SetRelativeLocation(targetCameraPosition);
}

void AThirdPersonProjectCharacter::OnLockOnCameraMoveFinished()
{
	
}

void AThirdPersonProjectCharacter::Log(ELogLevel LoggingLevel, FString Message, ELogOutput LogOutput)
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

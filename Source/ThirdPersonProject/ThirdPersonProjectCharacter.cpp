

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
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// AThirdPersonProjectCharacter

AThirdPersonProjectCharacter::AThirdPersonProjectCharacter(const FObjectInitializer& ObjectInitialzer) :
	Super(ObjectInitialzer.SetDefaultSubobjectClass<UTPPMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

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

	bIsAnimationBlended = false;
	bIsMovementInputEnabled = true;
}

void AThirdPersonProjectCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (CurrentSpecialMove)
	{
		CurrentSpecialMove->Tick(DeltaTime);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AThirdPersonProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AThirdPersonProjectCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AThirdPersonProjectCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &AThirdPersonProjectCharacter::RotateSideways);
	PlayerInputComponent->BindAxis("TurnRate", this, &AThirdPersonProjectCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &AThirdPersonProjectCharacter::RotateUpwards);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AThirdPersonProjectCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AThirdPersonProjectCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AThirdPersonProjectCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AThirdPersonProjectCharacter::OnResetVR);

	// Lock-on methods
	PlayerInputComponent->BindAction("LockOn", IE_Pressed, this, &AThirdPersonProjectCharacter::OnLockOnPressed);

	// Movement methods
	PlayerInputComponent->BindAction("MovementAbility", IE_Pressed, this, &AThirdPersonProjectCharacter::BeginMovementAbility);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AThirdPersonProjectCharacter::OnSprintPressed);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AThirdPersonProjectCharacter::OnSprintReleased);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AThirdPersonProjectCharacter::OnCrouchPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AThirdPersonProjectCharacter::OnCrouchReleased);

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

void AThirdPersonProjectCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AThirdPersonProjectCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AThirdPersonProjectCharacter::MoveForward(float Value) {

	if ((Controller != NULL) && (Value != 0.0f) && bIsMovementInputEnabled)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AThirdPersonProjectCharacter::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f) && bIsMovementInputEnabled)
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AThirdPersonProjectCharacter::RotateSideways(float value)
{
	AddControllerYawInput(value);
}

void AThirdPersonProjectCharacter::RotateUpwards(float value)
{
	AddControllerPitchInput(value);
}

void AThirdPersonProjectCharacter::SetMovementInputEnabled(bool bIsEnabled)
{
	bIsMovementInputEnabled = bIsEnabled;
}

void AThirdPersonProjectCharacter::OnSprintPressed()
{
	bWantsToSprint = true;

	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->RotationRate = FRotator(0.f, SprintRotationRate, 0.f);
		CharacterMovementComponent->MaxWalkSpeed = SprintingSpeed;
	}
}

void AThirdPersonProjectCharacter::OnSprintReleased()
{
	bWantsToSprint = false;
	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->MaxWalkSpeed = DefaultSpeed;
		CharacterMovementComponent->RotationRate = FRotator(0.f, DefaultRotationRate, 0.f);
	}
}

void AThirdPersonProjectCharacter::OnCrouchPressed()
{
	Crouch(false);
}

void AThirdPersonProjectCharacter::OnCrouchReleased()
{
	UnCrouch(false);
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
	if (MovementComponent)
	{		
		return MovementComponent->CanSlide();
	}

	return false;
}

void AThirdPersonProjectCharacter::OnStartSlide()
{
	bIsMovementInputEnabled = false;
}

void AThirdPersonProjectCharacter::OnEndSlide()
{
	bIsMovementInputEnabled = true;
}

bool AThirdPersonProjectCharacter::CanJumpInternal_Implementation() const
{
	UTPPMovementComponent* MovementComponent = GetTPPMovementComponent();
	return MovementComponent && !MovementComponent->IsSliding()&& Super::CanJumpInternal_Implementation();
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
	if (MovementAbilityClass)
	{
		CurrentAbility = NewObject<UBaseAbility>(this, MovementAbilityClass);
		if (CurrentAbility)
		{
			CurrentAbility->SetOwningCharacter(this);
			CurrentAbility->ActivateAbility();
		}
	}
}

void AThirdPersonProjectCharacter::ExecuteSpecialMove(TSubclassOf<UTPPSpecialMove> SpecialMoveToExecute)
{
	if (SpecialMoveToExecute)
	{
		UTPPSpecialMove* SpecialMove = NewObject<UTPPSpecialMove>(this, SpecialMoveToExecute);
		if (SpecialMove)
		{
			CurrentSpecialMove = SpecialMove;
			SpecialMove->OwningCharacter = this;
			SpecialMove->BeginSpecialMove();
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

void AThirdPersonProjectCharacter::SetIsAnimationBlended(bool bIsAnimBlended)
{
	bIsAnimationBlended = bIsAnimBlended;
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

void AThirdPersonProjectCharacter::AttachSocketCollisionBoxes(EAttackType attackType)
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

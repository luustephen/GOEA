// Copyright Epic Games, Inc. All Rights Reserved.

#include "GOEACharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

#include "DrawDebugHelpers.h"

//////////////////////////////////////////////////////////////////////////
// AGOEACharacter

AGOEACharacter::AGOEACharacter()
{

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	ClimbStamina = 2.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	CharMovement = GetCharacterMovement();
	CharMovement->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	CharMovement->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	CharMovement->JumpZVelocity = 600.f;
	CharMovement->AirControl = 0.2f;

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
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGOEACharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AGOEACharacter::Jump);
	//PlayerInputComponent->BindAction("Jump", IE_Repeat, this, &AGOEACharacter::CheckClimb);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AGOEACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGOEACharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGOEACharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AGOEACharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AGOEACharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AGOEACharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AGOEACharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AGOEACharacter::OnResetVR);

	if (InitPos.IsZero() && InitRot.IsZero()) {
		InitPos = GetActorLocation();
		InitRot = GetActorRotation();
	}
}

void AGOEACharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CharMovement->IsMovingOnGround() && HeldJump)
		CheckClimb(DeltaTime);
	else {
		Climbing = false;
		ClimbStamina = 2.f;
	}
}

bool AGOEACharacter::IsClimbing() {
	return Climbing;
}

void AGOEACharacter::Respawn() {
	TeleportTo(InitPos,InitRot,true);
}

void AGOEACharacter::OnResetVR()
{
	// If GOEA is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in GOEA.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AGOEACharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AGOEACharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AGOEACharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AGOEACharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AGOEACharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f) && !Climbing)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AGOEACharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) && !Climbing)
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

void AGOEACharacter::CheckClimb(float DeltaTime) 
{
	if ((Controller != nullptr) && CharMovement->Velocity.Z > 0 && ClimbStamina > 0)
	{
		// find out which way is right
		const FVector ForwardVector = GetActorForwardVector();
		const FVector UpVector = GetActorUpVector();

		// Set parameters to use line tracing
		FHitResult Hit;
		FCollisionQueryParams TraceParams(FName(TEXT("")), false, GetOwner());  // false to ignore complex collisions and GetOwner() to ignore self

		GetWorld()->LineTraceSingleByObjectType(
			OUT Hit,
			GetActorLocation(),
			GetActorLocation() + ForwardVector * 50,
			FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
			TraceParams
		);

		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + ForwardVector * 50, FColor::Green, false, 1, 0, 5);

		// See what if anything has been hit and return what
		AActor* ActorHit = Hit.GetActor();

		if (ActorHit) {
			FVector wallNorm = (Hit.ImpactPoint - GetActorLocation()).GetSafeNormal();
			float fwdWallDot = FVector::DotProduct(wallNorm, Hit.ImpactNormal);
			if (fwdWallDot < -.9 && HeldJump) {
				Climbing = true;
				LaunchCharacter(UpVector * 600, true, true);
				ClimbStamina -= DeltaTime;
			}
			else 
				Climbing = false;
		}
		else 
			Climbing = false;
	}
	else
		Climbing = false;
}

void AGOEACharacter::StopJumping()
{
	ACharacter::StopJumping();

	if ((Controller != nullptr))
	{
		HeldJump = false;
		Climbing = false;
	}
}

void AGOEACharacter::Jump()
{
	ACharacter::Jump();

	if ((Controller != nullptr))
	{
		HeldJump = true;
	}
}

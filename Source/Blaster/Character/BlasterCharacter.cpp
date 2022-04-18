// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;


	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh()); // We attach to the mesh because we will later use crouching, so when we change the capsule half height, this wont affect our springarm elevation
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true; // Rotate the camera along with our controller when we add mouse input

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // Does not need to use pawn control rotation as it's attached to the boom

	bUseControllerRotationYaw = false; // We don't want our character to rotate with our controller rotation for now (unequipped)
	GetCharacterMovement()->bOrientRotationToMovement = true; // To look where we are moving

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat) // We know it will be initialize in this point, but just checking null for safety
	{
		Combat->Character = this; // Notice how we can acces the private variable Character, this is because this class is marked as a friend class in CombatComponent.h
	}
}

void ABlasterCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f); // Not using getactor forward vector, as we want to move forward int the controllers direction not the root capsule component
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X)); // Getting the FORWARD vector from our yaw rotation. GetUnitAxis(X) returns a vector that represents the direction of this YawRotation. Since it is zeroed in the pitch and roll, this returns a vector parallel to the ground
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f); // Not using getactor forward vector, as we want to move forward int the controllers direction not the root capsule component
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y)); // Getting the RIGHT vector from our yaw rotation. GetUnitAxis(Y) returns a vector that represents the direction of this YawRotation. Since it is zeroed in the pitch and roll, this returns a vector parallel to the ground
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::EquipButtonPressed()
{
	/* Remember that this is called on any machine that presses the E key(server and client)
	/ Keep this in mind because things like equipping weapons should be done by the server*/


	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			// We are the client, so call the RPC.
			ServerEquipButtonPressed();
		}
		
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	// No need to check for authority because we know a server RPC will only be executed on a server
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

// This function is only called on the server through AWeapon::BeginPlay OnComponentBegin/EndOverlap
void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	/* Before setting the overlapping weapon, we check if the overlapping weapon is valid, and hide the pickup widget
	* This handles the case of when we are the server as OnRep_OverlappingWeapon is not called on the server, so we don't hide the pickup widget */
	if (OverlappingWeapon && IsLocallyControlled()) // We also check IsLocallyControlled, as if the server is overlapping with the weapon, and another client comes up and overlaps with it then end overlap, the widget will also be hidden on the server
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	// Setting the overlapping weapon with the new weapon, which will get replicated
	OverlappingWeapon = Weapon;

	/* Making sure we only show the widget on the character that's controlling the pawn, by making sure we are on the character that's controlling the pawn
	* We know that this function is only called on the server, because it is called from OnSphereOverlap on AWeapon, which is only called from the server
	* IsLocallyControlled ss only true on the character that is being controlled.
	* So if we enter this if, we know we are on the character being controlled by the player that is hosting the game(ie the server)
	* If that is the case, we know OverlappingWeapon will NOT be replicated to anyone because we have the COND_OwnerOnly condition set,
	* and none of the clients will be owners of this character as it is locally controlled on the server
	* In this case, all we need to do is show the pickup widget */
	if (IsLocallyControlled())
	{
		// We show the widget (doing the same logic as in OnRep_OverlappingWeapon, so it is better to just call OnRep_OverlappingWeapon?)
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}

}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	// OnOverlapEnd: We still need to null check as the server can set the OverlappingWeapon to null (OnEndOverlap) and that still would replicate
	if (OverlappingWeapon) 
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	// OnOverlapEnd: The last value will still be valid even if we set OverlappingWeapon to null, so we can still call functions in the LastWeapon, even though the new replicated value is null
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}






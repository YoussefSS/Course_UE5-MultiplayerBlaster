// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}


void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming; // We can set this, even for clients, because if we depended on the RPC, this would take time for the character to aim down sights depending on the clients internet speed. So with cosmetic things, we can set them directly without having to be on the server
	ServerSetAiming(bIsAiming); // Note that we don't check if we are a client (using !Character->HasAuthority), as running a Server function from a server, is like running a regular function (see RPC documentation)
	if (Character)
	{
		// Note that setting the MaxWalkSpeed on the client only is very bad, as the server still thinks you use the default MaxWalkSpeed, and the movement will become very choppy as the server tries to reset you to normal speed
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed; 
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult HitResult;
	TraceUnderCrosshairs(HitResult);
}

// Called from the server from BlasterCharacter
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	// Attaching the weapon to a socket on our character
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	// Attaching actors IS propagated to clients
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	/* Making the character the owner of the weapon, we don't need to do this on the client, as SetOwner is propagated to clients
	This is because inside it Owner is set which is ReplicatedUsing OnRep_Owner, which is a virtual function that we can override if we want*/
	EquippedWeapon->SetOwner(Character);

	// We want our character to look where we are aiming when they equip something. Not that this is only set on the server, so we will take care of this for the clients in a RepNotify
	Character->GetCharacterMovement()->bOrientRotationToMovement = false; 
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	// We want our character to look where we are aiming when they equip something. Handles the client
	if (EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

// This is called locally on a client or server
void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	
	if (bFireButtonPressed)
	{
		ServerFire();
	}
}

void UCombatComponent::ServerFire_Implementation()
{
	MulticastFire();
}

void UCombatComponent::MulticastFire_Implementation()
{
	if (EquippedWeapon == nullptr) return;

	if (Character)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(HitTarget);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	// Tracing from the center of the screen
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f); // Center of our viewport

	// The crosshair location is in viewport/screen space. We need to convert from screen to world space
	FVector CrosshairWorldPosition; // out param
	FVector CrosshairWorldDirection; // out param

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0), // index 0 gives the local player
		CrosshairLocation, // The location we want to deproject to world
		CrosshairWorldPosition, // The center of our screen
		CrosshairWorldDirection // The direction our screen is facing
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH; // Trace length is a macro defined in CombatComponent.h

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult, // Out param
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (!TraceHitResult.bBlockingHit) // If we didn't hit anything, we set the impact point so we can still have somewhere to fire our projectile
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
		}
		else
		{
			HitTarget = TraceHitResult.ImpactPoint;
			DrawDebugSphere(GetWorld(), TraceHitResult.ImpactPoint, 12.f, 12, FColor::Red);
		}
	}
}

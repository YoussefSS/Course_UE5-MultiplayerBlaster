// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // FALSE

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
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

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
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

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (EquippedWeapon == nullptr) return;

	if (Character && bFireButtonPressed)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire();
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterTypes/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}

	if (BlasterCharacter == nullptr) return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped(); // EquippedWeapon is replicated to all clients, so this will work for all clients
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bIsCrouched = BlasterCharacter->bIsCrouched; // This variable is already replicated from the base Character class
	bAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
	bElimmed = BlasterCharacter->IsElimmed();

// OFFSET YAW FOR STRAFING
	/* Which which direction we're aiming / our controlling is pointing at, and which direction we're moving
	*  These 2 values will return the same value if we are looking and moving in the same direction 
	*  With debug strings, we can see that these values are replicating on the controlling as well as simulating client. (Well, AimRotation only replicated when we move on simulatedProxy, but for Autonomous it works without moving) */
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation(); // This is a global rotation, not local or depending on which direction we're moving in. It goes up to 180 then -180 to 0. It's like the rotation of our controller / the way the camera is facing
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity()); // Takes a direction vector and returns a rotation. It also goes up to 180 then -180 to 0. It's like the rotation the character is moving in. It is also global and corresponds to the same global location as the base aim rotation

	/* We can get the delta between the two to set the animation values.
	*  If we are looking straight ahead but holding down the A key and moving right, the delta would be 90 */
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f); // We use this instead of regular float interp/interp in blendspace because of the reasons described in UE5 Animation note. ie going instantly from -180 to 180, without interping to 0 first
	YawOffset = DeltaRotation.Yaw;
// END OFFSET YAW FOR STRAFING

// LEAN
	// The lean has to do with the character yaw rotation itself, and the rotation from the previous frame
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime; // Making the delta value bigger as it's very small, and also using DeltaTime so we're frame independant
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f); // Interping so there is no jerking in our lean
	Lean = FMath::Clamp(Interp, -90, 90);
// END LEAN

	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh()) // Why do we check character mesh?
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		BlasterCharacter->GetMesh()->TransformToBoneSpace("hand_r", LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation); // Getting the LeftHandSocket relative to our right hand. This is because the weapon should not be adjusted relative to the right hand at runtime
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
		// Now we have our LeftHandTransform with its location set to the LeftHandSockets location but in hand_r bone space

		if (BlasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget())); // We add because if we don't, the weapon will face the other direction
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
	}

	bUseFABRIK = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	if (BlasterCharacter->IsLocallyControlled() && BlasterCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade) // reloading client side prediction
	{
		bUseFABRIK = !BlasterCharacter->IsLocallyReloading();
	}
	bUseAimOffsets = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableGameplay();
	bTransformRightHand = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableGameplay();
}
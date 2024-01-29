// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget); // This is fine to call on clients as we want it to do things that happen on all machines

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	// Spawning the projectile
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* World = GetWorld();
	if (MuzzleFlashSocket && World)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		
		FVector ToTarget = HitTarget - SocketTransform.GetLocation(); // From muzle flash socket to hit location from TraceUnderCrosshairs
		FRotator TargetRotation = ToTarget.Rotation(); // Getting the rotation that the spawned projectile should be

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner(); // We set the owner of the weapon to the character when we equip it in CombatComponent::EquipWeapon
		SpawnParams.Instigator = InstigatorPawn;

		/* 
		*	Explanation (Spawning The Projectiles): 
		*	https://www.evernote.com/shard/s457/nl/82208803/6cb1c8c6-5a81-4855-ffe5-569f1175f746?title=UE5%20Server-Side%20Rewind%20for%20Projectiles
		*/
		AProjectile* SpawnedProjectile = nullptr;
		if (bUseServerSideRewind)
		{
			if (InstigatorPawn->HasAuthority()) // server
			{
				if (InstigatorPawn->IsLocallyControlled()) // server, hosting player - use replicated projectile (ProjectileClass), no SSR as we're already on the server
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->Damage = Damage;
				}
				else // server, not locally controlled - spawn non-replicated projectile (ServerSideRewindProjectileClass), set SSR to true as SSR bullets will not cause damage on the server - check AProjectileBullet::OnHit
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
				}
			}
			else // client
			{
				if (InstigatorPawn->IsLocallyControlled()) // client, locally controlled - spawn non-replicated projectile (ServerSideRewindProjectileClass), use SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
					SpawnedProjectile->Damage = Damage; // Unecessary as the server will check the damage on the weapon
				}
				else // client, not locally controlled - spawn non-replicated projectile (ServerSideRewindProjectileClass), no SSR (just a visual for the host)
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else // weapon not using SSR
		{
			// all server characters should spawn replicated projectiles (ProjectileClass). Clients will have to wait for the projectile to replicate down to them
			if (InstigatorPawn->HasAuthority()) 
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage;
			}
		}
	}
}

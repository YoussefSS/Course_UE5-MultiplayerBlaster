// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Blaster.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(), // If we wanted to attach the emitter to one of th ebones on the skeleton. We don't want to, so we just pass in an empty FName
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition // Spawns the particle at the world position of the collision box, and keep it there (follow along)
		);
	}

	if (HasAuthority()) // We only want hit events to happen on the server
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit); // We use OnComponentHit instead of OnComponentBeginOverlap. 
	}
}

// Only called from the server
void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}

void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false // bAutodestroy to false as we want to deactivate the system manually
		);
	}
}

void AProjectile::ExplodeDamage()
{
	APawn* FiringPawn = GetInstigator(); // Returns the pawn that owns this projectile .. this is set in ProjectileWeapon::Fire
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff( // this has 2 radii, 1 inner that takes in full base damage, 1 outer that takes the minimum damage
				this, // World context object
				Damage, // Base damage
				10.f, // Minimum damage
				GetActorLocation(), // Origin (center)
				DamageInnerRadius, // Inner radius
				DamageOuterRadius, // Outer radius
				1.f, // Damage fallof .. 1 means linear
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(), // Ignoreactors, we pass in an empty array
				this, // Damage causer
				FiringController // InstigatorController
			);
		}
	}

}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimerFinished,
		DestroyTime);

}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

// Called when the projectile is destroyed from OnHit, as destruction of a replicated actor is propagated to clients, and AActor::Destroyed is called on server and all clients
void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}


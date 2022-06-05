// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	APawn* FiringPawn = GetInstigator(); // Returns the pawn that owns this projectile .. this is set in ProjectileWeapon::Fire
	if (FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff( // this has 2 radii, 1 inner that takes in full base damage, 1 outer that takes the minimum damage
				this, // World context object
				Damage, // Base damage
				10.f, // Minimum damage
				GetActorLocation(), // Origin (center)
				200.f, // Inner radius
				500.f, // Outer radius
				1.f, // Damage fallof .. 1 means linear
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(), // Ignoreactors, we pass in an empty array
				this, // Damage causer
				FiringController // InstigatorController
				);
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
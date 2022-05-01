// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true); // We need this so that OnComponentHit events happen when we have SetSimulatePhysics to true
	ShellEjectionImpulse = 10.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
	
	if (CasingMesh)
	{
		CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
		CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
	}
	
	SetLifeSpan(2.f); // Destroy after 3 seconds
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound)
	{
		// We can also do something more fancy like waiting a few seconds before destroying so that the shells linger a bit on the ground
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}
	
	CasingMesh->SetNotifyRigidBodyCollision(false); // Disable more collision so we don't play the sound again
}


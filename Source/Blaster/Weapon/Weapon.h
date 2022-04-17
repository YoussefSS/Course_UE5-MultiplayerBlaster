// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8 
{
	EWS_Initial		UMETA(DisplayName = "InitialState"), // A state for when the weapon is sitting in the world, it has never been picked up yet, and our char can walk up to it and equip it
	EWS_Equipped	UMETA(DisplayName = "Equipped"),
	EWS_Dropped		UMETA(DisplayName = "Dropped"),
	EWS_MAX			UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere; // Overlap volume so we can enable equipping this weapon when the character overlaps

	UPROPERTY(VisibleAnywhere)
	EWeaponState WeaponState;
public:	
	

};

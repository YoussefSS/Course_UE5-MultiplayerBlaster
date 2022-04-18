// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override; // This is the earliest time we can get access to a component

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true")) // Allowing this variable to be BlueprintReadOnly even though it's private
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon; // As soon as the value of OverlappingWeapon changes, it will replicate

	/* RepNotifies CAN have a parameter, but it can only have a parameter of the input type of the variable that is being replicated
	* Notice the AWeapon* input here, this input is the value of the variable before it got changed/replicated. So if it were set to null, we can still access the old value with the in pointer */
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon); 

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

public:	
	// This function is only called on the server through AWeapon::BeginPlay OnComponentBegin/EndOverlap
	void SetOverlappingWeapon(AWeapon* Weapon);

};

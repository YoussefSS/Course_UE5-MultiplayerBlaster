// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override; // This is the earliest time we can get access to a component
	virtual void Destroyed() override;
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();

	virtual void OnRep_ReplicatedMovement() override;

	
	void Elim(); // Server only
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	void SpawnDefaultWeapon();
protected:
	virtual void BeginPlay() override;

	virtual void Jump() override;
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void ReloadButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void GrenadeButtonPressed();
	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();

	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();

	void PlayHitReactMontage();
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	// Poll for any relevant classes and initialize our HUD (ex PlayerState which is null on the first frame)
	void PollInit();

	void RotateInPlace(float DeltaTime);
	
	/**
	* Hit boxes used for server-side rewind
	* We'll name the hitboxes the same name as the bones for convenience
	*/
	UPROPERTY(EditAnywhere)
	class UBoxComponent* head;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* pelvis;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* spine_02;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* spine_03;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* upperarm_l;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* upperarm_r;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* lowerarm_l;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* lowerarm_r;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* hand_l;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* hand_r;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* backpack;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* blanket;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* thigh_l;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* thigh_r;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* calf_l;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* calf_r;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* foot_l;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* foot_r;

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

	/**
	* Blaster components
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* LagCompensation;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	// Aim offsets
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/**
	 * Animation Montages
	 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage;

	// Hides the character when we are backed up against a wall and can't see anything except our char
	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	// Should this character rotate the root bone
	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

	/**
	 * Player Health
	 */

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	/**
	 * Player Shield
	 */

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player Stats")
	float Shield = 0;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

	/**
	 * Dissolve Effect
	 */

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	// Callback function called every frame
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	void StartDissolve();

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// Material instance set on BP, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	/* End dissolve effect */
	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

	/*
	* Grenade
	*/

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	/*
	* Default weapon
	*/

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

public:	
	// This function is only called on the server through AWeapon::BeginPlay OnComponentBegin/EndOverlap
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	ECombatState GetCombatState() const;
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	bool IsLocallyReloading();

};

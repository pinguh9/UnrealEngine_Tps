// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy.h"
#include "Enemy_Rifle.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT1_API AEnemy_Rifle : public AEnemy
{
	GENERATED_BODY()

public:
	AEnemy_Rifle();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	class AActor* MyOwner;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float BulletSpread;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	class USoundCue* FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	class UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	UParticleSystem* ImpactEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectileWeapon")
	TSubclassOf<AActor> ProjectileClass;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AController* WeaponInstigator;

	FORCEINLINE void SetInstigator(AController* Inst) { WeaponInstigator = Inst; }

	UFUNCTION(BlueprintCallable, category = "Combat")
	void Fire();

	UFUNCTION(BlueprintCallable, category = "Combat")
	void StartFire();

	UFUNCTION(BlueprintCallable, category = "Combat")
	void stopFire();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float RateOfFire;

	FTimerHandle TimerHandle_TimeBetShot;

	float TimeBetShots;

	float LastFiredTime;
};

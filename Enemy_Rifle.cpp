// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_Rifle.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "Kismet/GamePlayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MainCharacter.h"
#include "Weapon.h"
#include "Sound/SoundCue.h"
#include "DrawDebugHelpers.h"
#include "Enemy_Rifle_AIController.h"

AEnemy_Rifle::AEnemy_Rifle()
{
	MuzzleSocketName = "MuzzleSocket";
	WeaponInstigator = this->GetController();
	RateOfFire = 600;
	BulletSpread = 10.0f;

	AIControllerClass = AEnemy_Rifle_AIController::StaticClass();
}

void AEnemy_Rifle::BeginPlay()
{
	Super::BeginPlay();

	TimeBetShots = 60 / RateOfFire;
	MyOwner = this;

	AIController = Cast<AAIController>(GetController());
}

void AEnemy_Rifle::Fire()
{
	if (MyOwner)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		EyeRotation = this->GetControlRotation();
		FVector ShotDirection = EyeRotation.Vector();

		//Bullet Spread
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);
		FVector TraceEndPoint = TraceEnd;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.bTraceComplex = true;

		UGameplayStatics::PlaySound2D(this, FireSound);

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, ECC_Pawn, QueryParams))
		{
			//Blocking hit! process damage
			AActor* HitActor = Hit.GetActor();
			AMainCharacter* target = Cast<AMainCharacter>(HitActor);

			if (target && DamageTypeClass)
			{
				UGameplayStatics::ApplyDamage(target, 10, WeaponInstigator, this, DamageTypeClass);
				if (ImpactEffect) {
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
				}

				TraceEndPoint = Hit.ImpactPoint;
			}
		}

		if (MuzzleEffect) {
			UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, this->GetMesh(), MuzzleSocketName);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AActor>(ProjectileClass, this->GetMesh()->GetSocketLocation(MuzzleSocketName), EyeRotation, SpawnParams);
	}
}

void AEnemy_Rifle::StartFire()
{
	float FirstDelay = FMath::Max(LastFiredTime + TimeBetShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetShot, this, &AEnemy_Rifle::Fire, TimeBetShots, true, 0.0f);
}

void AEnemy_Rifle::stopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetShot);
}

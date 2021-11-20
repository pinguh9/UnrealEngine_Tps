// Fill out your copyright notice in the Description page of Project Settings.


#include "MainAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon.h"
#include "WeaponType.h"
#include "MainCharacter.h"

UMainAnimInstance::UMainAnimInstance()
{
	MovementSpeed = 0.f;
	bIsInAir = false;
	MovementOffset = 0.f;
	bAiming = false;
	CharacterRotation = FRotator(0.f);
	CharacterRotationLastFrame = FRotator(0.f);
	TIPCharacterYaw = 0.f;
	TIPCharacterYawLastFrame = 0.f;
	YawDelta = 0.f;
	RootYawOffset = 0.f;
	Pitch = 0.f;
	bReloading = false;
	OffsetState = EOffsetState::EOS_Hip;
	RecoilWeight = 1.0f;
	bTurningInPlace = false;
	EquippedWeaponType = EWeaponType::EWT_MAX;
	bShouldUseFABRIK = false;
}

void UMainAnimInstance::NativeInitializeAnimation()
{
	Main = Cast<AMainCharacter>(TryGetPawnOwner());
}

void UMainAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (Pawn == nullptr)
	{
		Pawn = TryGetPawnOwner();
	}
	if (Pawn)
	{
		if (Main == nullptr)
		{
			Main = Cast<AMainCharacter>(Pawn);
		}


		bIsInAir = Pawn->GetMovementComponent()->IsFalling();
		bAiming = Main->GetAiming();
		bCrouching = Main->GetCrouching();
		bReloading = Main->GetCombatStatus() == ECombatState::ECS_Reloading;
		bEquipping = Main->GetCombatStatus() == ECombatState::ECS_Equipping;
		bShouldUseFABRIK = (Main->GetCombatStatus() == ECombatState::ECS_Unoccupied) ||
			(Main->GetCombatStatus() == ECombatState::ECS_FireTimerInProgress);
		FVector Speed = Main->GetVelocity();
		FVector LateralSpeed = FVector(Speed.X, Speed.Y, 0.f);
		
		FRotator AimRotation = Main->GetBaseAimRotation();
		
		FRotator MovementRotation =
			UKismetMathLibrary::MakeRotFromX(Main->GetVelocity());
		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(
			MovementRotation,
			AimRotation).Yaw;
		MovementSpeed = LateralSpeed.Size();

		if (Main->GetVelocity().Size() > 0.f)
		{
			LastMovementOffsetYaw = MovementOffsetYaw;
		}
		
		if (bReloading)
		{
			OffsetState = EOffsetState::EOS_Reloading;
		}
		else if (bIsInAir)
		{
			OffsetState = EOffsetState::EOS_InAir;
		}
		else if (Main->GetAiming())
		{
			OffsetState = EOffsetState::EOS_Aiming;
		}
		else
		{
			OffsetState = EOffsetState::EOS_Hip;
		}

		if (Main->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f)
		{
			bIsAccelerating = true;
		}
		else 
		{
			bIsAccelerating = false;
		}

		if (Main->GetEquippedWeapon())
		{
			EquippedWeaponType = Main->GetEquippedWeapon()->GetWeaponType();
		}
	}
	TurnInPlace();
	Lean(DeltaTime);
}

void UMainAnimInstance::TurnInPlace()
{
	if (Main == nullptr) return;
	
	Pitch = Main->GetBaseAimRotation().Pitch;

	if (MovementSpeed > 0 || bIsInAir)
	{
		RootYawOffset = 0.f;
		TIPCharacterYaw = Main->GetActorRotation().Yaw;
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		RotationCurveLastFrame = 0.f;
		RotationCurve = 0.f;
	}
	else
	{
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		TIPCharacterYaw = Main->GetActorRotation().Yaw;
		const float TIPYawDelta{ TIPCharacterYaw - TIPCharacterYawLastFrame };

		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - TIPYawDelta);

		const float Turning{ GetCurveValue(TEXT("Turning")) };
		if (Turning > 0)
		{
			bTurningInPlace = true;
			RotationCurveLastFrame = RotationCurve;
			RotationCurve = GetCurveValue(TEXT("Rotation"));
			const float DeltaRotation{ RotationCurve - RotationCurveLastFrame };

			RootYawOffset > 0 ? RootYawOffset -= DeltaRotation : RootYawOffset += DeltaRotation;

			const float ABSRootYawOffset{ FMath::Abs(RootYawOffset) };
			if (ABSRootYawOffset > 90.f)
			{
				const float YawExcess{ ABSRootYawOffset - 90.f };
				RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
			}
		}
		else
		{
			bTurningInPlace = false;
		}
	}

	//set the recoil weight
	if (bTurningInPlace)
	{
		if (bReloading || bEquipping)
		{
			RecoilWeight = 1.f;
		}
		else
		{
			RecoilWeight = 0.f;
		}
	}
	else // not turning in place
	{
		if (bCrouching)
		{
			if (bReloading || bEquipping)
			{
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.1f;
			}
		}
		else
		{
			if (bAiming || bReloading || bEquipping)
			{
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.5f;
			}
		}
	}
}


void UMainAnimInstance::Lean(float DeltaTime)
{
	if (Main == nullptr) return;
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = Main->GetActorRotation();

	FRotator Delta{ UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame) };

	const float Target{ Delta.Yaw / DeltaTime };
	const float Interp{ FMath::FInterpTo(YawDelta, Target, DeltaTime, 6.f) };
	YawDelta = FMath::Clamp(Interp, -90.f, 90.f);
}

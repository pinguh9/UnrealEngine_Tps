
// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon.h"
#include "Item.h"
#include "Ammo.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/WidgetComponent.h"
#include "Animation/AnimInstance.h"
#include "Sound/SoundCue.h"
#include "Enemy.h"
#include "MainPlayerController.h"
#include "FirstSaveGame.h"
#include "Blade.h"
#include "Particles/ParticleSystemComponent.h"
#include "Interactable.h"
#include "ItemStorage.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "BulletHitInterface.h"
#include "EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MyStatComponent.h"
#include "project1.h"

// Sets default values
AMainCharacter::AMainCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 240.f;//카메라가 떨어져 있는 거리
	CameraBoom->bUsePawnControlRotation = true;//콘트롤러 기반으로 암 회전
	CameraBoom->SocketOffset = FVector(0.f, 60.f, 80.f);

	//Set size for collision
	GetCapsuleComponent()->SetCapsuleSize(48.f, 105.f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	//Attach the camera to the end of the boom and let the boom adjust to match
	//the controller orientation
	FollowCamera->bUsePawnControlRotation = false;
	CameraDefaultFov = 0.f;
	CameraZoomedFov = 40.f;
	CameraCurrentFov = 0.f;
	ZoomInterpSpeed = 30.f;

	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;

	HipTurnRate = 90.f;
	HipLookUpRate = 90.f;
	AimingTurnRate = 20.f;
	AimingLookUpRate = 20.f;

	MouseHipTurnRate = 1.0f;
	MouseHipLookUpRate = 1.0f;
	MouseAimingLookUpRate = 0.6f;
	MouseAimingTurnRate = 0.6f;

	//Don't rotate when the controller rotate
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	//configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;//Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.f, 0.0f);//...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 400.f;
	GetCharacterMovement()->AirControl = 0.2f;

	MaxHealth = 100.f;
	Health = 100.f;
	MaxStamina = 350.f;
	Stamina = 120.f;
	Coins = 0;
	OverlappedItemCount = 0;
	StunChance = 0.1f;

	S_PickUpMax = 8;
	S_PickUpCnt = 0;

	Starting9mmAmmo = 85;
	StartingARAmmo = 120;

	RunningSpeed = 650.f;
	BackwardSpeed = 600.f;
	SprintingSpeed = 950.f;
	CrouchMovementSpeed = 300.f;

	bShiftKeyDown = false;
	bLMBDown = false;
	bESCDown = false;
	bWeaponIsGun = false;

	MovementStatus = EMovementStatus::EMS_Normal;
	StatminaStatus = EStaminaStatus::ESS_Normal;
	CombatState = ECombatState::ECS_Unoccupied;
	StaminaDrainRate = 50.f;
	MinSprintStamina = 50.f;

	bAiming = false;
	bHasCombatTarget = false;
	bMovingFoward = false;
	bMovingBackward = false;
	bMovingRight = false;
	bShouldTraceForItems = false;
	bShouldFire = true;
	bCrouching = false;
	bAimingButtonPressed = false;

	CameraInterpDistance = 200.f;
	CameraInterpElevation = 50.f;

	StandingCapsuleHalfHeight = 88.f;
	CrouchCapsuleHalfHeight = 44.f;

	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));
	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("Weapon Interpolation Component"));
	WeaponInterpComp->SetupAttachment(GetFollowCamera());

	HighlightedSlot = -1;

	InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 1"));
	InterpComp1->SetupAttachment(GetFollowCamera());

	InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 2"));
	InterpComp2->SetupAttachment(GetFollowCamera());

	InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 3"));
	InterpComp3->SetupAttachment(GetFollowCamera());

	InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 4"));
	InterpComp4->SetupAttachment(GetFollowCamera());

	InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 5"));
	InterpComp5->SetupAttachment(GetFollowCamera());

	InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 6"));
	InterpComp6->SetupAttachment(GetFollowCamera());

	Stat = CreateDefaultSubobject<UMyStatComponent>(TEXT("Stat"));
}


// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	MainPlayerController = Cast<AMainPlayerController>(GetController());//컨트롤러가 먼저 설정되어야함
	//LoadGameNoSwitch();
	InitializeAmmoMap();
	InitializeInterpLocation();

	if (FollowCamera)
	{
		CameraDefaultFov = GetFollowCamera()->FieldOfView;
		CameraCurrentFov = CameraDefaultFov;
	}
	if (MainPlayerController)
	{
		MainPlayerController->GameModeOnly();
	}
	if (DefaultWeaponClass)
	{
		EquipWeapon(SpawnDefaultWeapon());
		Inventory.Add(EquippedWeapon);
		EquippedWeapon->SetSlotIndex(0);
		EquippedWeapon->DisableCustomDepth();
		EquippedWeapon->DisableGlowMaterial();
		EquippedWeapon->SetCharacter(this);
	}
}

void AMainCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	if (Inventory.Num() - 1 >= EquippedWeapon->GetSlotIndex())
	{
		Inventory[EquippedWeapon->GetSlotIndex()] = WeaponToSwap;
		WeaponToSwap->SetSlotIndex(EquippedWeapon->GetSlotIndex());
	}

	DropWeapon();
	EquipWeapon(WeaponToSwap, true);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

void AMainCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool AMainCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon == nullptr)return false;
	return EquippedWeapon->GetAmmo() > 0;
}

void AMainCharacter::PlayFireSound()
{
	if (EquippedWeapon->GetFireSound())
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
	}
}

void AMainCharacter::SendBullet()
{
	const USkeletalMeshSocket* MuzzleSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("MuzzleSocket");
	if (MuzzleSocket)
	{
		const FTransform SocketTransform = MuzzleSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());
		if (EquippedWeapon->GetMuzzleEffect())
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetMuzzleEffect(), SocketTransform);
		}

		FHitResult BeamHitResult;
		bool bBeamEnd = GetBeamEndLocation(BeamHitResult);
		if (bBeamEnd)
		{
			if (BeamHitResult.Actor.IsValid())
			{
				IBulletHitInterface* BulletHitInterface = Cast<IBulletHitInterface>(BeamHitResult.Actor.Get());
				if (BulletHitInterface)
				{
					BulletHitInterface->BulletHit_Implementation(BeamHitResult, this, GetController());
				}
				else
				{
					if (EquippedWeapon->GetImpactEffect())
					{
						UGameplayStatics::SpawnEmitterAtLocation(
							GetWorld(),
							EquippedWeapon->GetImpactEffect(),
							BeamHitResult.Location);
					}
				}
				AEnemy* HitEnemy = Cast<AEnemy>(BeamHitResult.Actor.Get());
				if (HitEnemy)
				{
					int32 Damage{};
					if (BeamHitResult.BoneName.ToString() == HitEnemy->GetHeadBoneName())
					{
						Damage = EquippedWeapon->GetHeadshotDamage();
						HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, true);
					}
					else
					{
						Damage = EquippedWeapon->GetDamage();
						HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, false);
					}
					UGameplayStatics::ApplyDamage(
						BeamHitResult.Actor.Get(),
						Damage,
						GetController(),
						this,
						UDamageType::StaticClass());
					
				}
			}
		}
		if (EquippedWeapon->GetProjectileClass())
		{
			FVector EyeLocation;
			FRotator EyeRotation;
			GetActorEyesViewPoint(EyeLocation, EyeRotation);

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GetWorld()->SpawnActor<AActor>(EquippedWeapon->GetProjectileClass(), SocketTransform.GetLocation(), EyeRotation, SpawnParams);
		}
	}
}

void AMainCharacter::PlayGunFireMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && RifleMontage)
	{
		AnimInstance->Montage_Play(RifleMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"), RifleMontage);
	}
}

void AMainCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void AMainCharacter::ReloadWeapon()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	
	if (EquippedWeapon == nullptr) return;

	if (CarryingAmmo() && !EquippedWeapon->ClipIsFull())
	{
		if (bAiming)
		{
			StopAiming();
		}

		CombatState = ECombatState::ECS_Reloading;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && ReloadMontage)
		{
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
		}
	}
}

void AMainCharacter::FinishReloading()
{
	if (CombatState == ECombatState::ECS_Stunned)return;
	CombatState = ECombatState::ECS_Unoccupied;

	if (bAimingButtonPressed)Aim();

	if (EquippedWeapon == nullptr)return;
	const auto AmmoType{ EquippedWeapon->GetAmmoType() };

	if (AmmoMap.Contains(AmmoType))
	{
		int32 CarriedAmmo = AmmoMap[AmmoType];

		const int32 MagEmptySpace =
			EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();

		if (MagEmptySpace > CarriedAmmo)
		{
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
		else
		{
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}
}

void AMainCharacter::FinishEquipping()
{
	if (CombatState == ECombatState::ECS_Stunned)return;
	CombatState = ECombatState::ECS_Unoccupied;
	if (bAimingButtonPressed)
	{
		Aim();
	}
}

bool AMainCharacter::CarryingAmmo()
{
	if (EquippedWeapon == nullptr)return false;

	auto AmmoType = EquippedWeapon->GetAmmoType();
	if (AmmoMap.Contains(AmmoType))
	{
		return AmmoMap[AmmoType] > 0;
	}

	return false;
}

void AMainCharacter::GrabClip()
{
	if (EquippedWeapon == nullptr) return;
	if (HandSceneComponent == nullptr) return;

	int32 ClipBoneIndex{ EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName()) };
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("Hand_L")));
	HandSceneComponent->SetWorldTransform(ClipTransform);

	EquippedWeapon->SetMovingClip(true);
}

void AMainCharacter::ReleaseClip()
{
	EquippedWeapon->SetMovingClip(false);
}

void AMainCharacter::CrouchBtnPressed()
{
	if (!GetCharacterMovement()->IsFalling())
	{
		bCrouching = !bCrouching;
	}
}

void AMainCharacter::InterpCapsuleHalfHeight(float DeltaTime)
{
	float TargetCapsuleHalfHeight{};
	bCrouching ? TargetCapsuleHalfHeight
		= CrouchCapsuleHalfHeight : TargetCapsuleHalfHeight = StandingCapsuleHalfHeight;
	const float InterpHalfHeight{ FMath::FInterpTo(
		GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		, TargetCapsuleHalfHeight,
		DeltaTime,
		20.f) };

	const float DeltaCapsuleHalfHeight{ InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() };
	const FVector MeshOffset{ 0.f, 0.f, -DeltaCapsuleHalfHeight };
	GetMesh()->AddLocalOffset(MeshOffset);

	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);
}

void AMainCharacter::Aim()
{
	bAiming = true;
}

void AMainCharacter::StopAiming()
{
	bAiming = false;
}

void AMainCharacter::PickupAmmo(AAmmo* Ammo)
{
	if (AmmoMap.Find(Ammo->GetAmmoType()))
	{
		int32 AmmoCount{ AmmoMap[Ammo->GetAmmoType()] };
		AmmoCount += Ammo->GetItemCount();
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
	}

	if (EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType())
	{
		if (EquippedWeapon->GetAmmo() == 0)
		{
			ReloadWeapon();
		}
	}

	Ammo->Destroy();
}

void AMainCharacter::InitializeInterpLocation()
{
	FInterpLocation WeaponLocation{ WeaponInterpComp, 0 };
	InterpLocations.Add(WeaponLocation);

	FInterpLocation InterpLoc1{ InterpComp1, 0 };
	InterpLocations.Add(InterpLoc1);

	FInterpLocation InterpLoc2{ InterpComp2, 0 };
	InterpLocations.Add(InterpLoc2);

	FInterpLocation InterpLoc3{ InterpComp3, 0 };
	InterpLocations.Add(InterpLoc3);
	
	FInterpLocation InterpLoc4{ InterpComp4, 0 };
	InterpLocations.Add(InterpLoc4);
	
	FInterpLocation InterpLoc5{ InterpComp5, 0 };
	InterpLocations.Add(InterpLoc5);

	FInterpLocation InterpLoc6{ InterpComp6, 0 };
	InterpLocations.Add(InterpLoc6);
	
}

void AMainCharacter::FKeyDown()
{
	if (CombatState != ECombatState::ECS_Unoccupied)return;
	if (ActiveOverlappingInteract)
	{
		ActiveOverlappingInteract->Interact(this);
	}
	else if (TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this);
		TraceHitItem = nullptr;
	}
	else {
		if (EquippedWeapon->GetSlotIndex() == 0)return;
		ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 0);
	}
}

void AMainCharacter::FKeyUp()
{
	
}

void AMainCharacter::OneKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 1)return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 1);
}

void AMainCharacter::TwoKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 2)return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 2);
}

void AMainCharacter::ThreekeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 3)return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 3);
}

void AMainCharacter::fourkeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 4)return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 4);
}

void AMainCharacter::fiveKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 5)return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 5);
}

void AMainCharacter::ExchangeInventoryItems(int32 CurrentitemIndex, int32 NewItemIndex)
{
	if ((CurrentitemIndex != NewItemIndex) && (NewItemIndex < Inventory.Num()) 
		&& (CombatState == ECombatState::ECS_Unoccupied || CombatState != ECombatState::ECS_Equipping))
	{
		if (bAiming)
		{
			StopAiming();
		}

		auto OldEuippedWeapon = EquippedWeapon;
		auto NewWeapon = Cast<AWeapon>(Inventory[NewItemIndex]);
		EquipWeapon(NewWeapon);

		OldEuippedWeapon->SetItemState(EItemState::EIS_PickedUp);
		NewWeapon->SetItemState(EItemState::EIS_Equipped);

		CombatState = ECombatState::ECS_Equipping;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && EquipMontage)
		{
			AnimInstance->Montage_Play(EquipMontage, 1.0f);
			AnimInstance->Montage_JumpToSection(FName("Equip"));
		}
		if (NewWeapon->GetEquipSound())
		{
			UGameplayStatics::PlaySound2D(this, NewWeapon->GetEquipSound());
		}
	}
}

int32 AMainCharacter::GetEmptyInventorySlot()
{
	for (int i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] == nullptr)
		{
			return i;
		}
	}
	if (Inventory.Num() < INVENTORY_CAPACITY)
	{
		return Inventory.Num();
	}

	return -1;
}

void AMainCharacter::HighlightInventorySlot()
{
	const int32 EmptySlot{ GetEmptyInventorySlot() };
	HightlightIconDelegate.Broadcast(EmptySlot, true);
	HighlightedSlot = EmptySlot;
}

EPhysicalSurface AMainCharacter::GetSurfaceType()
{
	FHitResult HitResult;
	const FVector Start{ GetActorLocation() };
	const FVector End{ Start + FVector(0.f, 0.f, -400.f) };
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;

	GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECollisionChannel::ECC_Visibility,
		QueryParams);
	
	return UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());
}

void AMainCharacter::EndStun()
{
	CombatState = ECombatState::ECS_Unoccupied;
	if (bAimingButtonPressed)
	{
		Aim();
	}
}

void AMainCharacter::UnHighlightInventorySlot()
{
	HightlightIconDelegate.Broadcast(HighlightedSlot, false);
	HighlightedSlot = -1;
}

int32 AMainCharacter::GetInterpLocationIndex()
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;

	for (int32 i = 1; i < InterpLocations.Num(); i++)
	{
		if (InterpLocations[i].ItemCount < LowestCount)
		{
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}
	}

	return LowestIndex;
}

void AMainCharacter::IncrementInterpLocItemCnt(int32 Index, int32 Amount)
{
	if (Amount < -1 || Amount > 1)return;

	if (InterpLocations.Num() >= Index)
	{
		InterpLocations[Index].ItemCount += Amount;
	}
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	if (MovementStatus == EMovementStatus::EMS_Dead) return;
	float DeltaStamina = StaminaDrainRate * DeltaTime;

	CameraInterpZoom(DeltaTime);
	SetLookRates();
	TraceForItems();
	InterpCapsuleHalfHeight(DeltaTime);

	switch (StatminaStatus)
	{
	case EStaminaStatus::ESS_Normal:
		if (bShiftKeyDown)
		{
			if (bMovingFoward || bMovingRight) {
				SetMovementStatus(EMovementStatus::EMS_Sprinting);
				if (Stamina - DeltaStamina <= MinSprintStamina)
				{
					SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
					Stamina -= DeltaStamina;
				}
				else
				{
					Stamina -= DeltaStamina;
				}
			}
			else
			{
				if (bCrouching) SetMovementStatus(EMovementStatus::EMS_Crouching);
				else if (bMovingBackward)SetMovementStatus(EMovementStatus::EMS_Backward);
				else SetMovementStatus(EMovementStatus::EMS_Normal);

				if (Stamina + DeltaStamina >= MaxStamina)
				{
					Stamina = MaxStamina;
				}
				else
				{
					Stamina += DeltaStamina;
				}
			}
		}
		else //Shift key up
		{
			if (Stamina + DeltaStamina >= MaxStamina)
			{
				Stamina = MaxStamina;
			}
			else
			{
				Stamina += DeltaStamina;
			}

			if (bCrouching)SetMovementStatus(EMovementStatus::EMS_Crouching);
			else if (bMovingBackward)SetMovementStatus(EMovementStatus::EMS_Backward);
			else SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_BelowMinimum:
		if (bShiftKeyDown)
		{
			if (Stamina - DeltaStamina <= 0.f)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Exhauseted);
				Stamina = 0;
				if (bMovingBackward)SetMovementStatus(EMovementStatus::EMS_Backward);
				else SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			else
			{
				Stamina -= DeltaStamina;
				SetMovementStatus(EMovementStatus::EMS_Sprinting);
			}
		}
		else//shift key up
		{
			if (Stamina + DeltaStamina > MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
				Stamina += DeltaStamina;
			}
			else
			{
				Stamina += DeltaStamina;
			}
			if (bCrouching)SetMovementStatus(EMovementStatus::EMS_Crouching);
			else if (bMovingBackward)SetMovementStatus(EMovementStatus::EMS_Backward);
			else SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_Exhauseted:
		if (bShiftKeyDown)
		{
			Stamina = 0.f;
		}
		else
		{
			SetStaminaStatus(EStaminaStatus::ESS_ExhausetedRecovering);
			Stamina += DeltaStamina;
		}
		if (bCrouching)SetMovementStatus(EMovementStatus::EMS_Crouching);
		else if (bMovingBackward)SetMovementStatus(EMovementStatus::EMS_Backward);
		else SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	case EStaminaStatus::ESS_ExhausetedRecovering:
		if (Stamina + DeltaStamina >= MinSprintStamina)
		{
			if (bCrouching)SetMovementStatus(EMovementStatus::EMS_Crouching);
			else if (bMovingBackward)SetMovementStatus(EMovementStatus::EMS_Backward);
			else SetMovementStatus(EMovementStatus::EMS_Normal);
			Stamina += DeltaStamina;
		}
		else
		{
			Stamina += DeltaStamina;
		}
		if (bCrouching)SetMovementStatus(EMovementStatus::EMS_Crouching);
		else if (bMovingBackward)SetMovementStatus(EMovementStatus::EMS_Backward);
		else SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	default:
		;
	}
	if (CombatTarget)
	{
		CombatTargetLocation = CombatTarget->GetActorLocation();
		if (MainPlayerController)
		{
			MainPlayerController->EnemyLocation = CombatTargetLocation;
		}
	}
}


FRotator AMainCharacter::GetLookAtRotationYaws(FVector Target)
{
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtRotationYaw(0.f, LookAtRotation.Yaw, 0.f);
	return LookAtRotationYaw;
}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMainCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMainCharacter::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMainCharacter::ShiftKeyUp);

	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMainCharacter::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMainCharacter::LMBUp);

	PlayerInputComponent->BindAction("RMB", IE_Pressed, this, &AMainCharacter::RMBDown);
	PlayerInputComponent->BindAction("RMB", IE_Released, this, &AMainCharacter::RMBUp);

	PlayerInputComponent->BindAction("ESC", IE_Pressed, this, &AMainCharacter::ESCDown);
	PlayerInputComponent->BindAction("ESC", IE_Released, this, &AMainCharacter::ESCUp);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMainCharacter::FKeyDown);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &AMainCharacter::FKeyUp);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AMainCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AMainCharacter::CrouchBtnPressed);

	PlayerInputComponent->BindAction("1Key", IE_Pressed, this, &AMainCharacter::OneKeyPressed);
	PlayerInputComponent->BindAction("2Key", IE_Pressed, this, &AMainCharacter::TwoKeyPressed);
	PlayerInputComponent->BindAction("3Key", IE_Pressed, this, &AMainCharacter::ThreekeyPressed);
	PlayerInputComponent->BindAction("4Key", IE_Pressed, this, &AMainCharacter::fourkeyPressed);
	PlayerInputComponent->BindAction("5Key", IE_Pressed, this, &AMainCharacter::fiveKeyPressed);

	PlayerInputComponent->BindAxis("MoveFoward", this, &AMainCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveBackward", this, &AMainCharacter::MoveBackward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMainCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &AMainCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMainCharacter::LookUp);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMainCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMainCharacter::LookUpAtRate);
}

bool AMainCharacter::CanMove(float value)
{
	if (MainPlayerController) {
		return (value != 0.0f) &&
			(MovementStatus != EMovementStatus::EMS_Dead) &&
			!(MainPlayerController->bPauseMenuVisible);
	}
	return false;
}

void AMainCharacter::MoveForward(float value)
{
	bMovingFoward = false;
	if (CanMove(value))
	{
		//find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, value);
		bMovingFoward = true;
	}
}

void AMainCharacter::MoveBackward(float value)
{
	bMovingBackward = false;
	if (CanMove(value))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, value);
		bMovingBackward = true;
	}
}


void AMainCharacter::MoveRight(float value)
{
	bMovingRight = false;
	if (CanMove(value))
	{
		//find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, value);
		bMovingRight = true;
	}
}

void AMainCharacter::Turn(float value)
{
	if (CanMove(value))
	{
		float TurnScaleFactor{};
		if (bAiming)
		{
			TurnScaleFactor = MouseAimingTurnRate;
		}
		else
		{
			TurnScaleFactor = MouseHipTurnRate;
		}
		AddControllerYawInput(value * TurnScaleFactor);
	}
}

void AMainCharacter::LookUp(float value)
{
	if (CanMove(value))
	{
		float TurnScaleFactor{};
		if (bAiming)
		{
			TurnScaleFactor = MouseAimingLookUpRate;
		}
		else
		{
			TurnScaleFactor = MouseHipLookUpRate;
		}
		AddControllerPitchInput(value * TurnScaleFactor);
	}
}

void AMainCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}


void AMainCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMainCharacter::LMBDown()
{
	if (MovementStatus == EMovementStatus::EMS_Dead)return;
	if (MainPlayerController) if (MainPlayerController->bPauseMenuVisible)return;

	bLMBDown = true;
	Attack();
}

void AMainCharacter::LMBUp()
{
	bLMBDown = false;
}

void AMainCharacter::RMBDown()
{
	if (!bWeaponIsGun)return;
	bAimingButtonPressed = true;
	if (CombatState != ECombatState::ECS_Reloading && CombatState != ECombatState::ECS_Equipping && CombatState != ECombatState::ECS_Stunned)
	{
		Aim();
	}
}

void AMainCharacter::RMBUp()
{
	if (!bWeaponIsGun)return;
	bAimingButtonPressed = false;
	StopAiming();
}

void AMainCharacter::CameraInterpZoom(float DeltaTime)
{
	if (bAiming)
	{
		CameraCurrentFov = FMath::FInterpTo(
			CameraCurrentFov,
			CameraZoomedFov,
			DeltaTime,
			ZoomInterpSpeed);
	}
	else
	{
		CameraCurrentFov = FMath::FInterpTo(
			CameraCurrentFov,
			CameraDefaultFov,
			DeltaTime,
			ZoomInterpSpeed);
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFov);
}

void AMainCharacter::ESCDown()
{
	bESCDown = true;

	if (MainPlayerController)
	{
		MainPlayerController->TogglePauseMenu();
	}
}

void AMainCharacter::ESCUp()
{
	bESCDown = false;
}


void AMainCharacter::IncrementCoin(int32 Amount)
{
	Coins += Amount;
}

void AMainCharacter::IncrementSPickUp(int32 cnt)
{
	S_PickUpCnt += cnt;
}

void AMainCharacter::IncrementHealth(float Amount)
{
	if (Health + Amount >= MaxHealth)
	{
		Health = MaxHealth;
	}
	else
	{
		Health += Amount;
	}
}

void AMainCharacter::Die()
{
	UAnimInstance* Animinstance = GetMesh()->GetAnimInstance();
	if (Animinstance && DeathMontage)
	{
		Animinstance->Montage_Play(DeathMontage);
	}
}

void AMainCharacter::FinishDeath()
{
	GetMesh()->bPauseAnims = true;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		DisableInput(PC);
	}
}

void AMainCharacter::Jump()
{
	if (MainPlayerController->bPauseMenuVisible)return;
	if (bCrouching)
	{
		bCrouching = false;
	}
	else
	{
		if (MovementStatus != EMovementStatus::EMS_Dead)
		{
			Super::Jump();
		}
	}
}

void AMainCharacter::SetMovementStatus(EMovementStatus status)
{
	MovementStatus = status;
	if (MovementStatus == EMovementStatus::EMS_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else if (MovementStatus == EMovementStatus::EMS_Backward)
	{
		GetCharacterMovement()->MaxWalkSpeed = BackwardSpeed;
	}
	else if (MovementStatus == EMovementStatus::EMS_Crouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

void AMainCharacter::ShiftKeyDown()
{
	bShiftKeyDown = true;
}

void AMainCharacter::ShiftKeyUp()
{
	bShiftKeyDown = false;
}


void AMainCharacter::SetEquippedWeapon(AWeapon* WeaponToSet)
{
	EquippedWeapon = WeaponToSet;
	bWeaponIsGun = true;
}

void AMainCharacter::EquipWeapon(AWeapon* WeaponToEquip, bool bSwapping)
{
	if (WeaponToEquip)
	{
		const USkeletalMeshSocket* RightHandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (RightHandSocket)
		{
			RightHandSocket->AttachActor(WeaponToEquip, GetMesh());
			WeaponToEquip->bRotate = false;
		}

		if (EquippedWeapon == nullptr)
		{
			EquipItemDelegate.Broadcast(-1,WeaponToEquip->GetSlotIndex());
		}
		else if(!bSwapping)
		{
			EquipItemDelegate.Broadcast(EquippedWeapon->GetSlotIndex(), WeaponToEquip->GetSlotIndex());
		}

		if (!WeaponToEquip->bWeaponParticles)WeaponToEquip->IdleParticlesComponent->Deactivate();
		SetEquippedWeapon(WeaponToEquip);
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void AMainCharacter::DropWeapon()
{
	if (EquippedWeapon)
	{
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

void AMainCharacter::Attack()
{
	if (EquippedWeapon == nullptr)return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (WeaponHasAmmo())
	{
		PlayFireSound();
		SendBullet();
		PlayGunFireMontage();
		EquippedWeapon->DecrementAmmo();

		StartFireTimer();

		if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol)
		{
			EquippedWeapon->StartSlideTimer();
		}
	}
}

void AMainCharacter::Attack_Blade() {
	if (!bAttacking && MovementStatus != EMovementStatus::EMS_Dead) {
		bAttacking = true;

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && BladeMontage)
		{
			int32 Section = FMath::RandRange(0, 1);
			switch (Section) {
			case 0:
				AnimInstance->Montage_Play(BladeMontage, 2.2f);
				AnimInstance->Montage_JumpToSection(FName("Attack_1"), BladeMontage);
				break;
			case 1:
				AnimInstance->Montage_Play(BladeMontage, 1.8f);
				AnimInstance->Montage_JumpToSection(FName("Attack_2"), BladeMontage);
				break;
			default:
				;
			}
		}
	}
}

void AMainCharacter::AttackEnd()
{
	bAttacking = false;
	if (bLMBDown)
	{
		Attack();
	}
}

void AMainCharacter::DecrementHealth(float Amount)
{
	if (Health - Amount <= 0.f)
	{
		Health -= Amount;
		Die();
	}
	else
	{
		Health -= Amount;
	}
}

float AMainCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.f)
	{
		Health = 0.f;
		Die();

		auto EnemyController = Cast<AEnemyController>(EventInstigator);
		if (EnemyController)
		{
			EnemyController->GetBlackboardComponent()->SetValueAsBool(FName(TEXT("CharacterDead")),true);
		}
	}
	else
	{
		Health -= DamageAmount;
	}
	return DamageAmount;
}

void AMainCharacter::UpdateCombatTarget(AEnemy* enemy)
{
	if (enemy == nullptr)
	{
		if (MainPlayerController)
		{
			MainPlayerController->RemoveEnemyHealthBar();
		}
	}
	else if (enemy != CombatTarget)
	{
		SetCombatTarget(enemy);
		if (MainPlayerController)
		{
			MainPlayerController->DisplayEnemyHealthBar();
		}
		bHasCombatTarget = true;
	}
}

void AMainCharacter::SetLookRates()
{
	if (bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

AWeapon* AMainCharacter::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass)
	{
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}

	return nullptr;
}

bool AMainCharacter::GetBeamEndLocation(FHitResult& OutHitResult)
{
	FVector OutBeamLocation;
	//FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(OutHitResult, OutBeamLocation);

	if (bCrosshairHit)
	{
		OutHitResult.Location = OutBeamLocation;
		return true;
	}
	return false;
}

bool AMainCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ Start + CrosshairWorldDirection * 50000.f };
		OutHitLocation = End;
		GetWorld()->LineTraceSingleByChannel(
			OutHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility);
		if (OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;
		}
	}

	return false;
}

void AMainCharacter::StartFireTimer()
{
	if (EquippedWeapon == nullptr)return;
	CombatState = ECombatState::ECS_FireTimerInProgress;

	GetWorldTimerManager().SetTimer(
		AutoFireTimer,
		this,
		&AMainCharacter::AutoFireReset,
		EquippedWeapon->GetAutoFireRate());	
}

void AMainCharacter::AutoFireReset()
{
	if (CombatState == ECombatState::ECS_Stunned) return;
	CombatState = ECombatState::ECS_Unoccupied;
	if (EquippedWeapon == nullptr) return;
	if (WeaponHasAmmo())
	{
		if (bLMBDown)
		{
			Attack();
		}
	}
	else
	{
		ReloadWeapon();
	}
}

void AMainCharacter::IncrementOverllapedItemCount(int8 Amount)
{
	if (OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}
}

void AMainCharacter::TraceForItems()
{
	if (bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshairs(ItemTraceResult, HitLocation);
		if (ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.GetActor());
			const auto TraceHitWeapon = Cast<AWeapon>(TraceHitItem);
			if (TraceHitWeapon)
			{
				if (HighlightedSlot == -1)
				{
					HighlightInventorySlot();
				}
			}
			else
			{
				if (HighlightedSlot != -1)
				{
					UnHighlightInventorySlot();
				}
			}

			if (TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EquipInterping)
			{
				TraceHitItem = nullptr;
			}

			if (TraceHitItem && TraceHitItem->GetPickupWidget())
			{
				TraceHitItem->GetPickupWidget()->SetVisibility(true);
				TraceHitItem->EnableCustomDepth();

				if (Inventory.Num() >= INVENTORY_CAPACITY)
				{
					TraceHitItem->SetCharacterInventoryFull(true);
				}
				else
				{
					TraceHitItem->SetCharacterInventoryFull(false);
				}
			}

			if (TraceHitItemLastFrame)
			{
				if (TraceHitItem != TraceHitItemLastFrame)
				{
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
					TraceHitItemLastFrame->DisableCustomDepth();
				}
			}
			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if (TraceHitItemLastFrame)
	{
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
		TraceHitItemLastFrame->DisableCustomDepth();
	}
}

void AMainCharacter::GetPickupItem(AItem* Item)
{
	if (Item->GetEquipSound())
	{
		UGameplayStatics::PlaySound2D(this, Item->GetEquipSound());
	}

	auto Weapon = Cast<AWeapon>(Item);
	if (Weapon)
	{
		if (Inventory.Num() < INVENTORY_CAPACITY)
		{
			Weapon->SetSlotIndex(Inventory.Num());
			Inventory.Add(Weapon);
			Weapon->SetItemState(EItemState::EIS_PickedUp);
		}
		else
		{
			SwapWeapon(Weapon);
		}
	}

	auto Ammo = Cast<AAmmo>(Item);
	if (Ammo)
	{
		PickupAmmo(Ammo);
	}
}


void AMainCharacter::SaveGame()
{
	UFirstSaveGame* SaveGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	SaveGameInstance->CharacterStats.Health = Health;
	SaveGameInstance->CharacterStats.Stamina = Stamina;
	SaveGameInstance->CharacterStats.Coins = Coins;

	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

	SaveGameInstance->CharacterStats.LevelName = MapName;

	if (EquippedWeapon)
	{
		SaveGameInstance->CharacterStats.WeaponName = EquippedWeapon->GetItemName();
	}

	SaveGameInstance->CharacterStats.Location = GetActorLocation();
	SaveGameInstance->CharacterStats.Rotation = GetActorRotation();

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->PlayerName, SaveGameInstance->UserIndex);
}

void AMainCharacter::LoadGame(bool setPosition)
{
	UFirstSaveGame* LoadGameInstace = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	LoadGameInstace = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstace->PlayerName, LoadGameInstace->UserIndex));

	Health = LoadGameInstace->CharacterStats.Health;
	Stamina = LoadGameInstace->CharacterStats.Stamina;
	Coins = LoadGameInstace->CharacterStats.Coins;

	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponName = LoadGameInstace->CharacterStats.WeaponName;

			if (Weapons->WeaponMap.Contains(WeaponName)) {

				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
				EquipWeapon(WeaponToEquip);
			}
		}
	}

	if (setPosition)
	{
		SetActorLocation(LoadGameInstace->CharacterStats.Location);
		SetActorRotation(LoadGameInstace->CharacterStats.Rotation);
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;

	if (LoadGameInstace->CharacterStats.LevelName != TEXT(""))
	{
		FName LevelName(*LoadGameInstace->CharacterStats.LevelName);
		//SwitchLevel(LevelName);
	}
}

void AMainCharacter::LoadGameNoSwitch()
{
	UFirstSaveGame* LoadGameInstace = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	LoadGameInstace = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstace->PlayerName, LoadGameInstace->UserIndex));

	Health = LoadGameInstace->CharacterStats.Health;
	Stamina = LoadGameInstace->CharacterStats.Stamina;
	Coins = LoadGameInstace->CharacterStats.Coins;

	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponName = LoadGameInstace->CharacterStats.WeaponName;

			if (Weapons->WeaponMap.Contains(WeaponName)) {

				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
				EquipWeapon(WeaponToEquip);
			}
		}
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;
}

FInterpLocation AMainCharacter::GetInterpLocation(int32 Index)
{
	if (Index <= InterpLocations.Num())
	{
		return InterpLocations[Index];
	}
	return FInterpLocation();
}

void AMainCharacter::Stun()
{
	if (Health <= 0.f)return;
	CombatState = ECombatState::ECS_Stunned;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
	}
}


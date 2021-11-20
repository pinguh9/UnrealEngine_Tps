#pragma once

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_Blade UMETA(DisplayName = "Blade"),
	EWT_Rifle UMETA(DisplayName = "Rifle"),
	EWT_GrenadeLauncher UMETA(DisplayName = "GrenadeLauncher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),

	EWT_MAX  UMETA(DisplayName = "DefaultMax"),
};

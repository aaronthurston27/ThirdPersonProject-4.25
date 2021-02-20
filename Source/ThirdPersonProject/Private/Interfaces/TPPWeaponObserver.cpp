// Fill out your copyright notice in the Description page of Project Settings.


#include "Interfaces/TPPWeaponObserver.h"
#include "Weapon/TPPWeaponBase.h"
#include "Weapon/TPPWeaponFirearm.h"

// Add default functionality here for any ITPPWeaponObserver functions that are not pure virtual.

void ITPPWeaponObserver::OnWeaponHit_Implementation(const FHitResult& HitResult, const float DamageApplied)
{

}

void ITPPWeaponObserver::OnWeaponFired_Implementation()
{

}

void ITPPWeaponObserver::OnWeaponReloaded_Implementation()
{

}
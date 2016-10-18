// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#include "RuntimeMeshTerrain.h"
#include "TerrainSection.h"
#include "SculptComponent.h"


USculptComponent::USculptComponent()
{
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
}



void USculptComponent::BeginPlay()
{
	Super::BeginPlay();
}


void USculptComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	FHitResult Hit;
	if (GetHitResult(OUT Hit))
	{
		CurrentSculptLocation = Hit.Location;
	}
}


void USculptComponent::SculptStart(UCameraComponent* Camera)
{
	OwnerCamera = Camera;
	SculptInput = ESculptInput::ST_Started;
	GetWorld()->GetTimerManager().SetTimer(SculptTimerHandle, this, &USculptComponent::Sculpt, GetWorld()->DeltaTimeSeconds, true);
}


void USculptComponent::SculptStop()
{
	SculptInput = ESculptInput::ST_Stopped;
	GetWorld()->GetTimerManager().PauseTimer(SculptTimerHandle);
	Sculpt();
}


void USculptComponent::Sculpt()
{
	FHitResult Hit;
	if (!GetHitResult(OUT Hit)) { return; }
	if (InSleepDistance(Hit.Location) && (SculptInput != ESculptInput::ST_Stopped)) { return; }

	// Cast to owner of hit section
	ATerrainSection* HitSection = dynamic_cast<ATerrainSection*>(Hit.GetActor());
	if (!HitSection) { return; }

	if (SculptInput == ESculptInput::ST_Started) 
	{ 
		StartLocation = Hit.Location - HitSection->GetActorLocation(); 
		HitSection->RequestSculpting(SculptSettings, Hit.Location, SculptInput, StartLocation);
		SculptInput = ESculptInput::ST_Ongoing;
	}
	else
	{
		HitSection->RequestSculpting(SculptSettings, Hit.Location, SculptInput, StartLocation);
	}
}


bool USculptComponent::GetHitResult(FHitResult &Hit)
{
	if (bUseMouseMode)
	{
		GetWorld()->GetFirstPlayerController()->GetHitResultUnderCursor(ECollisionChannel::ECC_WorldStatic, true, OUT Hit); // TODO get controller through owner reference
	}
	else if (OwnerCamera)
	{
		FVector Start = OwnerCamera->GetComponentLocation();
		FVector End = Start + (OwnerCamera->GetForwardVector() * InteractionDistance);
		GetWorld()->LineTraceSingleByChannel(OUT Hit, Start, End, ECollisionChannel::ECC_WorldStatic);
	}
	return Hit.bBlockingHit;
}


bool USculptComponent::InSleepDistance(FVector CurrentLocation)
{
	CurrentLocation *= FVector(1, 1, 1);
	if (FVector::Dist(LastLocation, CurrentLocation) < SleepDistance) { return true; }
	LastLocation =  CurrentLocation;
	return false;
}
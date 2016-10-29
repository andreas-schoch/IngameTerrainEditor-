// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#include "RuntimeMeshTerrain.h"
#include "TerrainSection.h"
#include "SculptComponent.h"


USculptComponent::USculptComponent()
{
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = false;
}


void USculptComponent::SculptStart()
{
	InputInfo.SculptInput = ESculptInput::ST_Started;
	InputInfo.StartLocation = HitResultOwner.Location;
	GetWorld()->GetTimerManager().SetTimer(SculptTimerHandle, this, &USculptComponent::Sculpt, GetWorld()->DeltaTimeSeconds, true);
}


void USculptComponent::SculptStop()
{
	InputInfo.SculptInput = ESculptInput::ST_Stopped;
	GetWorld()->GetTimerManager().PauseTimer(SculptTimerHandle);
	Sculpt();
}


void USculptComponent::Sculpt()
{
	InputInfo.CurrentLocation = HitResultOwner.Location;
	if (InSleepDistance()) { return; }
	if (InputInfo.SculptInput == ESculptInput::ST_Stopped) { return; }

	// Cast to owner of hit section
	ATerrainSection* HitSection = dynamic_cast<ATerrainSection*>(HitResultOwner.GetActor());
	if (!HitSection) { return; }

	HitSection->RequestSculpting(SculptSettings, InputInfo);
	if (InputInfo.SculptInput == ESculptInput::ST_Started) { InputInfo.SculptInput = ESculptInput::ST_Ongoing; }
}


bool USculptComponent::InSleepDistance()
{
	if (FVector::Dist(InputInfo.LastLocation, InputInfo.CurrentLocation) < SleepDistance) { return true; }
	InputInfo.LastLocation = InputInfo.CurrentLocation;
	return false;
}
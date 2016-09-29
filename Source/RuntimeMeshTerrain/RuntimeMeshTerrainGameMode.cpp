// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "RuntimeMeshTerrain.h"
#include "RuntimeMeshTerrainGameMode.h"
#include "RuntimeMeshTerrainHUD.h"
#include "RuntimeMeshTerrainCharacter.h"

ARuntimeMeshTerrainGameMode::ARuntimeMeshTerrainGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ARuntimeMeshTerrainHUD::StaticClass();
}

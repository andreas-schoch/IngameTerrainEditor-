// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#include "RuntimeMeshTerrain.h"
#include "TerrainEditorStuff.h"


// Sets default values
ATerrainEditorStuff::ATerrainEditorStuff()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ATerrainEditorStuff::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATerrainEditorStuff::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}


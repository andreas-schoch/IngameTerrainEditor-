// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#include "RuntimeMeshTerrain.h"
#include "RuntimeMeshComponent.h"
#include "TerrainGenerator.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "TerrainSection.h"


ATerrainSection::ATerrainSection()
{
	PrimaryActorTick.bCanEverTick = false;
	RuntimeMeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponent"));
	RootComponent = RuntimeMeshComponent;
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
	ProceduralMeshComponent->SetupAttachment(RootComponent);
}


void ATerrainSection::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(VisibilityTimerHandle, this, &ATerrainSection::SetVisibility, 0.1, true);
}


void ATerrainSection::InitializeOnSpawn(int32 SectionIndex, FVector2D ComponentCoordinates, ATerrainGenerator* Terrain)
{
	OwningTerrain = Terrain;
	SectionIndexLocal = SectionIndex;
	SectionCoordinates = FVector2D(SectionIndex / OwningTerrain->GetSectionXY(), SectionIndex % OwningTerrain->GetSectionXY());
	auto SectionSideInCM = (OwningTerrain->GetSectionXY()-1) * OwningTerrain->GetQuadSize();
	auto Half = SectionSideInCM / 2;
	SectionCenterWorldLocation2D = FVector2D(GetActorLocation().X + ComponentCoordinates.X * SectionSideInCM + Half, GetActorLocation().Y + ComponentCoordinates.Y * SectionSideInCM + Half);

	PlayerControllerReference = GetWorld()->GetFirstPlayerController();
}


void ATerrainSection::CreateSection()
{
	// Called from Terrain Generator on spawn
	FSectionProperties* SectionPropertiesPtr = &OwningTerrain->SectionProperties;

	if (bUseRuntimeMeshComponent)
	{
		RuntimeMeshComponent->CreateMeshSection(
			0,
			SectionPropertiesPtr->Vertices,
			SectionPropertiesPtr->Triangles,
			SectionPropertiesPtr->Normals,
			SectionPropertiesPtr->UV,
			SectionPropertiesPtr->VertexColors,
			*OwningTerrain->GetDummyTangentsRuntime(),
			true,
			EUpdateFrequency::Frequent);
	}
	else
	{
		ProceduralMeshComponent->CreateMeshSection(
			0,
			SectionPropertiesPtr->Vertices,
			SectionPropertiesPtr->Triangles,
			SectionPropertiesPtr->Normals,
			SectionPropertiesPtr->UV,
			SectionPropertiesPtr->VertexColors,
			*OwningTerrain->GetDummyTangents(),
			true);
	}
}


void ATerrainSection::UpdateSection()
{
	//FSectionProperties* SectionPropertiesPtr = &OwningTerrain->SectionProperties;
	
	if (bUseRuntimeMeshComponent)
	{
		RuntimeMeshComponent->UpdateMeshSection(
			0,
			OwningTerrain->GetSectionProperties()->Vertices,
			OwningTerrain->GetSectionProperties()->Normals,
			OwningTerrain->GetSectionProperties()->UV,
			OwningTerrain->GetSectionProperties()->VertexColors,
			*OwningTerrain->GetDummyTangentsRuntime());
		OwningTerrain->SectionUpdateFinished();
	}
	else
	{
		ProceduralMeshComponent->UpdateMeshSection(
			0,
			OwningTerrain->GetSectionProperties()->Vertices,
			OwningTerrain->GetSectionProperties()->Normals,
			OwningTerrain->GetSectionProperties()->UV,
			OwningTerrain->GetSectionProperties()->VertexColors,
			*OwningTerrain->GetDummyTangents());
	}
}


void ATerrainSection::SetVisibility()
{
	if (!ensure(PlayerControllerReference)) { return; }
	auto PlayerPawnWorldLocation = PlayerControllerReference->GetPawn()->GetActorLocation();
	FVector2D PlayerPawnWorldLocation2D = FVector2D(PlayerPawnWorldLocation.X, PlayerPawnWorldLocation.Y);
	float DistanceToPawn = FVector2D::Distance(PlayerPawnWorldLocation2D, SectionCenterWorldLocation2D);

	if (DistanceToPawn > OwningTerrain->SectionVisibilityRange && (bUseRuntimeMeshComponent ? RuntimeMeshComponent->IsVisible() : ProceduralMeshComponent->IsVisible()))
	{
		if (bUseRuntimeMeshComponent)
		{
			RuntimeMeshComponent->SetVisibility(false);
			//RuntimeMeshComponent->SetMeshSectionCollisionEnabled(0, false); // needs queue
		}
		else
		{
			ProceduralMeshComponent->SetVisibility(false);
		}
	}
	else if (DistanceToPawn < OwningTerrain->SectionVisibilityRange && !(bUseRuntimeMeshComponent ? RuntimeMeshComponent->IsVisible() : ProceduralMeshComponent->IsVisible()))
	{
		if (bUseRuntimeMeshComponent)
		{
			RuntimeMeshComponent->SetVisibility(true);
			//RuntimeMeshComponent->SetMeshSectionCollisionEnabled(0, true);
		}
		else
		{
			ProceduralMeshComponent->SetVisibility(true);
		}		
	}
}


void ATerrainSection::RequestSculpting(FSculptSettings SculptSettings, FVector HitLocation, ESculptInput SculptInput, FVector StartLocation)
{
	OwningTerrain->SectionRequestsUpdate(SectionIndexLocal, SculptSettings, HitLocation, SculptInput, StartLocation);
}
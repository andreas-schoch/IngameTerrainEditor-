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

	// LODs
	RuntimeMeshComponentLOD1 = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponentLOD1"));
	RuntimeMeshComponentLOD1->SetupAttachment(RootComponent);
	ProceduralMeshComponentLOD1 = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponentLOD1"));
	ProceduralMeshComponentLOD1->SetupAttachment(RootComponent);
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
	OwningTerrain->FillSectionVertStructLOD(SectionIndexLocal);

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

	CreateLOD1();
}


void ATerrainSection::UpdateSection()
{
	FSectionProperties* SectionPropertiesPtr = &OwningTerrain->SectionProperties;
	OwningTerrain->FillSectionVertStructLOD(SectionIndexLocal);

	if (bUseRuntimeMeshComponent)
	{
		RuntimeMeshComponent->UpdateMeshSection(
			0,
			SectionPropertiesPtr->Vertices,
			SectionPropertiesPtr->Normals,
			SectionPropertiesPtr->UV,
			SectionPropertiesPtr->VertexColors,
			*OwningTerrain->GetDummyTangentsRuntime());
		OwningTerrain->SectionUpdateFinished();
	}
	else
	{
		ProceduralMeshComponent->UpdateMeshSection(
			0,
			SectionPropertiesPtr->Vertices,
			SectionPropertiesPtr->Normals,
			SectionPropertiesPtr->UV,
			SectionPropertiesPtr->VertexColors,
			*OwningTerrain->GetDummyTangents());
	}
	UpdateLOD1();
}


void ATerrainSection::SetVisibility()
{
	if (!ensure(PlayerControllerReference)) { return; }
	FVector PlayerPawnWorldLocation = PlayerControllerReference->GetPawn()->GetActorLocation();
	FVector2D PlayerPawnWorldLocation2D = FVector2D(PlayerPawnWorldLocation.X, PlayerPawnWorldLocation.Y);
	float DistanceToPawn = FVector2D::Distance(PlayerPawnWorldLocation2D, SectionCenterWorldLocation2D);

	if (DistanceToPawn > OwningTerrain->SectionVisibilityRange && (bUseRuntimeMeshComponent ? RuntimeMeshComponent->IsVisible() : ProceduralMeshComponent->IsVisible()))
	{
		if (bUseRuntimeMeshComponent)
		{
			RuntimeMeshComponent->SetVisibility(false);
			RuntimeMeshComponentLOD1->SetVisibility(true);
		}
		else
		{
			ProceduralMeshComponent->SetVisibility(false);
			ProceduralMeshComponentLOD1->SetVisibility(true);
		}
	}
	else if (DistanceToPawn < OwningTerrain->SectionVisibilityRange && !(bUseRuntimeMeshComponent ? RuntimeMeshComponent->IsVisible() : ProceduralMeshComponent->IsVisible()))
	{
		if (bUseRuntimeMeshComponent)
		{
			RuntimeMeshComponent->SetVisibility(true);
			RuntimeMeshComponentLOD1->SetVisibility(false);
		}
		else
		{
			ProceduralMeshComponent->SetVisibility(true);
			ProceduralMeshComponentLOD1->SetVisibility(false);
		}
	}
}


void ATerrainSection::RequestSculpting(FSculptSettings SculptSettings, FVector HitLocation, ESculptInput SculptInput, FVector StartLocation)
{
	OwningTerrain->SectionRequestsUpdate(SectionIndexLocal, SculptSettings, HitLocation, SculptInput, StartLocation);
}


void ATerrainSection::CreateLOD1()
{
	FSectionProperties* SectionPropertiesLOD1Ptr = &OwningTerrain->SectionPropertiesLOD1;

	if (bUseRuntimeMeshComponent)
	{
		RuntimeMeshComponentLOD1->CreateMeshSection(
			0,
			SectionPropertiesLOD1Ptr->Vertices,
			SectionPropertiesLOD1Ptr->Triangles,
			SectionPropertiesLOD1Ptr->Normals,
			SectionPropertiesLOD1Ptr->UV,
			SectionPropertiesLOD1Ptr->VertexColors,
			*OwningTerrain->GetDummyTangentsRuntime(),
			false,
			EUpdateFrequency::Frequent);
	}
	else
	{
		ProceduralMeshComponentLOD1->CreateMeshSection(
			0,
			SectionPropertiesLOD1Ptr->Vertices,
			SectionPropertiesLOD1Ptr->Triangles,
			SectionPropertiesLOD1Ptr->Normals,
			SectionPropertiesLOD1Ptr->UV,
			SectionPropertiesLOD1Ptr->VertexColors,
			*OwningTerrain->GetDummyTangents(),
			false);
	}
}

void ATerrainSection::UpdateLOD1()
{
	FSectionProperties* SectionPropertiesLOD1Ptr = &OwningTerrain->SectionPropertiesLOD1;

	if (bUseRuntimeMeshComponent)
	{
		RuntimeMeshComponentLOD1->UpdateMeshSection(
			0,
			SectionPropertiesLOD1Ptr->Vertices,
			SectionPropertiesLOD1Ptr->Normals,
			SectionPropertiesLOD1Ptr->UV,
			SectionPropertiesLOD1Ptr->VertexColors,
			*OwningTerrain->GetDummyTangentsRuntime());
		OwningTerrain->SectionUpdateFinished();
	}
	else
	{
		ProceduralMeshComponentLOD1->UpdateMeshSection(
			0,
			SectionPropertiesLOD1Ptr->Vertices,
			SectionPropertiesLOD1Ptr->Normals,
			SectionPropertiesLOD1Ptr->UV,
			SectionPropertiesLOD1Ptr->VertexColors,
			*OwningTerrain->GetDummyTangents());
	}
}
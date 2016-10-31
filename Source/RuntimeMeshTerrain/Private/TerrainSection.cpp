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
	//Root
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// LOD0   TODO add all LOD components dynamically 
	RuntimeMeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponent"));
	RuntimeMeshComponent->SetupAttachment(RootComponent);
	RuntimeMeshLODs.Add(RuntimeMeshComponent);
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
	ProceduralMeshComponent->SetupAttachment(RootComponent);
	ProceduralMeshLODs.Add(ProceduralMeshComponent);

	// LOD1
	RuntimeMeshComponentLOD1 = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponentLOD1"));
	RuntimeMeshComponentLOD1->SetupAttachment(RootComponent);
	RuntimeMeshLODs.Add(RuntimeMeshComponentLOD1);
	ProceduralMeshComponentLOD1 = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponentLOD1"));
	ProceduralMeshComponentLOD1->SetupAttachment(RootComponent);
	ProceduralMeshLODs.Add(ProceduralMeshComponentLOD1);

	// LOD2
	RuntimeMeshComponentLOD2 = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponentLOD2"));
	RuntimeMeshComponentLOD2->SetupAttachment(RootComponent);
	RuntimeMeshLODs.Add(RuntimeMeshComponentLOD2);
	ProceduralMeshComponentLOD2 = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponentLOD2"));
	ProceduralMeshComponentLOD2->SetupAttachment(RootComponent);
	ProceduralMeshLODs.Add(ProceduralMeshComponentLOD2);

	// LOD3
	RuntimeMeshComponentLOD3 = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponentLOD3"));
	RuntimeMeshComponentLOD3->SetupAttachment(RootComponent);
	RuntimeMeshLODs.Add(RuntimeMeshComponentLOD3);
	ProceduralMeshComponentLOD3 = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponentLOD3"));
	ProceduralMeshComponentLOD3->SetupAttachment(RootComponent);
	ProceduralMeshLODs.Add(ProceduralMeshComponentLOD3);

	// LOD4
	RuntimeMeshComponentLOD4 = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponentLOD4"));
	RuntimeMeshComponentLOD4->SetupAttachment(RootComponent);
	RuntimeMeshLODs.Add(RuntimeMeshComponentLOD4);
	ProceduralMeshComponentLOD4 = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponentLOD4"));
	ProceduralMeshComponentLOD4->SetupAttachment(RootComponent);
	ProceduralMeshLODs.Add(ProceduralMeshComponentLOD4);
}


void ATerrainSection::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(VisibilityTimerHandle, this, &ATerrainSection::SetVisibility, 0.1, true);
}


void ATerrainSection::InitializeOnSpawn(int32 SectionIndex, ATerrainGenerator* Terrain)
{
	OwningTerrain = Terrain;
	SectionIndexLocal = SectionIndex;
	SectionCoordinates = FVector(SectionIndex / OwningTerrain->GetComponentXY(), SectionIndex % OwningTerrain->GetComponentXY(), 0);

	float SideLength = (OwningTerrain->GetSectionXY() - 1) * OwningTerrain->GetQuadSize();
	CenterLocation = (GetActorLocation() + (SectionCoordinates * SideLength) + (SideLength / 2)) * FVector(1, 1, 0);
	PlayerControllerReference = GetWorld()->GetFirstPlayerController();
}


void ATerrainSection::CreateSection()
{
	// Called from Terrain Generator on spawn
	OwningTerrain->FillSectionVertStructLOD(SectionIndexLocal);

	if (bUseRuntimeMeshComponent)
	{
		TArray<FRuntimeMeshTangent> DummyTangentsRuntime;
		for (int32 i = 0; i < RuntimeMeshLODs.Num(); i++)
		{
			RuntimeMeshLODs[i]->CreateMeshSection(
				0,
				OwningTerrain->LODProperties[i]->Vertices,
				OwningTerrain->LODProperties[i]->Triangles,
				OwningTerrain->LODProperties[i]->Normals,
				OwningTerrain->LODProperties[i]->UV,
				OwningTerrain->LODProperties[i]->VertexColors,
				DummyTangentsRuntime,
				(i == 0) ? true : false,
				EUpdateFrequency::Frequent);

			if (i > 3)
			{
				RuntimeMeshLODs[i]->SetCastShadow(false);
			}
		}
	}
	else
	{
		TArray<FProcMeshTangent> DummyTangents;
		for (int32 i = 0; i < ProceduralMeshLODs.Num(); i++)
		{
			ProceduralMeshLODs[i]->CreateMeshSection(
				0,
				OwningTerrain->LODProperties[i]->Vertices,
				OwningTerrain->LODProperties[i]->Triangles,
				OwningTerrain->LODProperties[i]->Normals,
				OwningTerrain->LODProperties[i]->UV,
				OwningTerrain->LODProperties[i]->VertexColors,
				DummyTangents,
				(i == 0) ? true : false);

			if (i > 2)
			{
				ProceduralMeshLODs[i]->SetCastShadow(false);
			}
		}
	}
}


void ATerrainSection::UpdateSection()
{
	OwningTerrain->FillSectionVertStructLOD(SectionIndexLocal);

	if (bUseRuntimeMeshComponent)
	{
		TArray<FRuntimeMeshTangent> DummyTangentsRuntime;
		for (int32 i = 0; i < RuntimeMeshLODs.Num(); i++)
		{
			RuntimeMeshLODs[i]->UpdateMeshSection(
				0,
				OwningTerrain->LODProperties[i]->Vertices,
				OwningTerrain->LODProperties[i]->Normals,
				OwningTerrain->LODProperties[i]->UV,
				OwningTerrain->LODProperties[i]->VertexColors,
				DummyTangentsRuntime);
		}
	}
	else
	{
		TArray<FProcMeshTangent> DummyTangents;
		for (int32 i = 0; i < ProceduralMeshLODs.Num(); i++)
		{
			ProceduralMeshLODs[i]->UpdateMeshSection(
				0,
				OwningTerrain->LODProperties[i]->Vertices,
				OwningTerrain->LODProperties[i]->Normals,
				OwningTerrain->LODProperties[i]->UV,
				OwningTerrain->LODProperties[i]->VertexColors,
				DummyTangents);
		}
	}
	OwningTerrain->SectionUpdateFinished();
}


void ATerrainSection::SetVisibility()
{
	if (!ensure(PlayerControllerReference)) { return; }
	FVector PlayerLocation = PlayerControllerReference->GetPawn()->GetActorLocation() * FVector(1, 1, 0);
	float DistanceX = FVector::Dist(PlayerLocation, FVector(CenterLocation.X, PlayerLocation.Y, 0));
	float DistanceY = FVector::Dist(PlayerLocation, FVector(PlayerLocation.X, CenterLocation.Y, 0));
	float DistanceToPawn = (DistanceX > DistanceY) ? DistanceX : DistanceY;

	//check if it should be visible
	bool bIsVisibleLOD0 = (DistanceToPawn < OwningTerrain->VisibilityLOD0) ? true : false;
	bool bIsVisibleLOD1 = (DistanceToPawn > OwningTerrain->VisibilityLOD0 && DistanceToPawn < OwningTerrain->VisibilityLOD1) ? true : false;
	bool bIsVisibleLOD2 = (DistanceToPawn > OwningTerrain->VisibilityLOD1 && DistanceToPawn < OwningTerrain->VisibilityLOD2) ? true : false;
	bool bIsVisibleLOD3 = (DistanceToPawn > OwningTerrain->VisibilityLOD2 && DistanceToPawn < OwningTerrain->VisibilityLOD3) ? true : false;
	bool bIsVisibleLOD4 = (DistanceToPawn > OwningTerrain->VisibilityLOD3 && DistanceToPawn < OwningTerrain->VisibilityLOD4) ? true : false;

	//Set visibility
	if (bUseRuntimeMeshComponent)
	{
		RuntimeMeshComponent->SetVisibility(bIsVisibleLOD0);
		RuntimeMeshComponentLOD1->SetVisibility(bIsVisibleLOD1);
		RuntimeMeshComponentLOD2->SetVisibility(bIsVisibleLOD2);
		RuntimeMeshComponentLOD3->SetVisibility(bIsVisibleLOD3);
		RuntimeMeshComponentLOD4->SetVisibility(bIsVisibleLOD4);
	}
	else
	{
		ProceduralMeshComponent->SetVisibility(bIsVisibleLOD0);
		ProceduralMeshComponentLOD1->SetVisibility(bIsVisibleLOD1);
		ProceduralMeshComponentLOD2->SetVisibility(bIsVisibleLOD2);
		ProceduralMeshComponentLOD3->SetVisibility(bIsVisibleLOD3);
		ProceduralMeshComponentLOD4->SetVisibility(bIsVisibleLOD4);
	}
}


void ATerrainSection::RequestSculpting(const FSculptSettings& Settings, const FSculptInputInfo& InputInfo)
{
	OwningTerrain->SectionRequestsUpdate(SectionIndexLocal, Settings, InputInfo);
}

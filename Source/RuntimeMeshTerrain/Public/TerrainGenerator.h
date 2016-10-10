// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "TerrainEditorStuff.h"
#include "ProceduralMeshComponent.h"
#include "RuntimeMeshComponent.h"
#include "TerrainGenerator.generated.h"


class ATerrainSection;


UCLASS()
class RUNTIMEMESHTERRAIN_API ATerrainGenerator : public AActor
{
	GENERATED_BODY()
	
public:
	void SectionRequestsUpdate(int32 SectionIndex, FSculptSettings SculptSettings, FVector HitLocation, ESculptInput SculptInput, FVector StartLocation);

	// Getters 
	int32 GetSectionXY() const { return SectionXY; }
	float GetQuadSize() const { return QuadSize; }
	int32 GetComponentXY() const { return ComponentXY; }
	FSectionProperties* GetSectionProperties() { return &SectionProperties; }

	auto* GetDummyTangents() { return &DummyTangents; }
	auto* GetDummyTangentsRuntime() { return &DummyTangentsRuntime; }

	void FillSectionVertStruct(int32 SectionIndex);
	void FillSectionVertStructLOD(int32 SectionIndex);
	void SectionUpdateFinished();

	FSectionProperties SectionProperties;
	FSectionProperties SectionPropertiesLOD1;
	FSectionProperties SectionPropertiesLOD2;
	FSectionProperties SectionPropertiesLOD3;
	FSectionProperties SectionPropertiesLOD4;

	TArray<FSectionProperties*> LODProperties;


	// Number of Components/Sections on each side 
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	int32 ComponentXY = 1;

	// Number of Vertices on each side of section
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	int32 SectionXY = 20;

	// Distance from one Vertex to another in cm
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float QuadSize = 100;

	// How far from generator reaches down to copy height
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float LineTraceLength = 10000;

	// Z Offset from copied height 
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float LineTraceHeightOffset = 100;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float VisibilityLOD0 = 20000;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float VisibilityLOD1 = 60000;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float VisibilityLOD2 = 100000;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float VisibilityLOD3 = 150000;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float VisibilityLOD4 = 200000;

	// using a timer prevents the gamethread from freezing at begin play 
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	bool bUseTimerforGeneration = true;

	// Timer delay if using timed generation
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float CreateSectionTimerDelay = 0.1;

	// Timer delay if using timed generation
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	int32 MaxZValueOffsetPerUpdate = 300;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	UCurveFloat* Curve = nullptr;

	// Define Class that will be spawned as section. 
	UPROPERTY(EditDefaultsOnly, Category = "ProceduralMeshGeneration")
	TSubclassOf<ATerrainSection> ClassToSpawnAsSection;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration LOD")
	int32 FactorLOD1 = 2;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration LOD")
	int32 FactorLOD2 = 4;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration LOD")
	int32 FactorLOD3 = 8;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration LOD")
	int32 FactorLOD4 = 16;

	UPROPERTY(EditAnywhere, Category = "Noise")
		float NoiseMultiplier = 1000;

	UPROPERTY(EditAnywhere, Category = "Noise")
		float NoiseDensity = 100;

private:
	ATerrainGenerator();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	void GenerateMesh();
	void InitializeProperties();
	void FillIndexBuffer();
	void AddBorderVerticesToSectionProperties();
	void FillGlobalProperties();
	void CopyLandscapeHeightBelow(FVector& Coordinates, FVector& Normal);
	void SpawnSectionActors();
	void MakeCrater(int32 SectionIndex, FSculptSettings SculptSettings, FVector HitLocation, ESculptInput SculptInput, FVector StartLocation);

	FVector CalculateVertexNormal(int32 VertexIndex);


	void FillGlobalNormals();


	void AddAffectedSections(int32 SectionIndex, int32 VertexIndex, OUT TArray<int32> &AffectedSections);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	URuntimeMeshComponent* RuntimeMeshComponent = nullptr;

	UPROPERTY()
	TArray<FVertexData> GlobalVertexData;

	TArray<int32> IndexBuffer;

	TArray<int32> SectionUpdateQueue; // TODO replace with TQueue
	TQueue<int32> SectionUpdateQueue2;
	TArray<int32> CreateTangentsForMeshQueue;
	bool bAllowCreatingTangents = false;
	bool bAllowedToUpdateSection = true;
	TArray<ATerrainSection*> SectionActors;

	// needed when using "GenerateMeshTimed()" function
	void GenerateMeshTimed();
	void FillGlobalPropertiesTimed();
	void FillIndexBufferTimed(); // TODO give better name
	FTimerHandle SectionCreateTimerHandle;
	int32 SectionIndexIter = 0;
	int32 IndexBufferIter = 0;
	int32 GlobalXIter = 0;

	TArray<FProcMeshTangent> DummyTangents;
	TArray<FRuntimeMeshTangent> DummyTangentsRuntime;

};

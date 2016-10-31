// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "TerrainEditorStuff.h"
#include "TerrainGenerator.generated.h"


class ATerrainSection;
class UProceduralMeshComponent;
class URuntimeMeshComponent;


UCLASS()
class RUNTIMEMESHTERRAIN_API ATerrainGenerator : public AActor
{
	GENERATED_BODY()
	
public:
	void SectionRequestsUpdate(int32 SectionIndex, const FSculptSettings& Settings, const FSculptInputInfo& InputInfo);

	// Getters 
	UFUNCTION(BlueprintCallable, Category = "TerrainGeneration")
	int32 GetSectionXY() const { return SectionXY; }
	
	UFUNCTION(BlueprintCallable, Category = "TerrainGeneration")
	float GetQuadSize() const { return QuadSize; }

	UFUNCTION(BlueprintCallable, Category = "TerrainGeneration")
	int32 GetComponentXY() const { return ComponentXY; }

	FSectionProperties* GetSectionProperties() { return &SectionProperties; }

	void FillSectionVertStructLOD(int32 SectionIndex);

	bool SetLODVertexData(int32 LOD, int32 LoopIter, int32 Index, int32 DivideFactor, FVertexData VertexData);

	void SectionUpdateFinished();

	FSectionProperties SectionProperties;

	FSectionProperties SectionPropertiesLOD1;

	FSectionProperties SectionPropertiesLOD2;

	FSectionProperties SectionPropertiesLOD3;

	FSectionProperties SectionPropertiesLOD4;

	TArray<FSectionProperties*> LODProperties;

	
	// Number of Components/Sections on each side 
	UPROPERTY(EditAnywhere, Category = "TerrainGeneration")
	int32 ComponentXY = 3;

	// Number of Vertices on each side of section
	UPROPERTY(EditAnywhere, Category = "TerrainGeneration")
	int32 SectionXY = 33;

	// Distance from one Vertex to another in cm
	UPROPERTY(EditAnywhere, Category = "TerrainGeneration")
	float QuadSize = 200;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration")
	ETerrainGeneration TerrainZ;

	// Define Class that will be spawned as section. 
	UPROPERTY(EditDefaultsOnly, Category = "TerrainGeneration|Setup")
	TSubclassOf<ATerrainSection> ClassToSpawnAsSection;

	// Z Offset from copied height 
	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LineTrace")
	float LineTraceHeightOffset = 500;

	// How far trace reaches down from Generator Z to copy
	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LineTrace")
	float LineTraceLength = 10000;

	// using a timer prevents the gamethread from freezing when generating terrain
	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Timed")
	bool bUseTimerforGeneration = true;

	// Delays the spawning of sections to prevent freezing
	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Timed")
	float CreateSectionTimerDelay = 0.1;

	// Delays X axis iteration when filling vertex data
	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Timed")
	float FillVertexDataTimerDelay = 0.005;


	// Max difference in Z height when using "Raise" or "Lower" 
	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Sculpting")
	int32 MaxZValueOffsetPerUpdate = 300;

	// Define the falloff that is used when sculpting
	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Sculpting")
	UCurveFloat* Curve = nullptr;


	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LOD")
	int32 FactorLOD1 = 2;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LOD")
	int32 FactorLOD2 = 4;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LOD")
	int32 FactorLOD3 = 8;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LOD")
	int32 FactorLOD4 = 16;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LOD")
	float VisibilityLOD0 = 20000;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LOD")
	float VisibilityLOD1 = 60000;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LOD")
	float VisibilityLOD2 = 100000;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LOD")
	float VisibilityLOD3 = 150000;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|LOD")
	float VisibilityLOD4 = 200000;


	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	float NoiseMultiplier1 = 100000;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	float NoiseMultiplier2 = 10000;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	float NoiseMultiplier3 = 1000;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	float NoiseMultiplier4 = 100;


	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	float Seed = 134250;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	float Scale1 = 2000;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	float Scale2 = 1000;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	float Scale3 = 500;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	float Scale4 = 250;


	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	bool bUseOctave2 = true;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	bool bUseOctave3 = true;

	UPROPERTY(EditAnywhere, Category = "TerrainGeneration|Noise")
	bool bUseOctave4 = true;


private:
	ATerrainGenerator();

	virtual void BeginPlay() override;

	void SetLODVisibility();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	void GenerateMesh();

	float GetHeightByNoise(int32 XCoords, int32 YCoords);

	void InitializeProperties();

	void FillIndexBuffer();

	void AddBorderVerticesToSectionProperties();

	void FillGlobalVertexData();

	void CopyLandscapeHeightBelow(FVector& Coordinates, FVector& Normal);

	void SpawnSectionActors();

	void ModifyTerrain(int32 SectionIndex, const FSculptSettings& Settings, const FSculptInputInfo& InputInfo);

	FVector CalculateVertexNormal(int32 VertexIndex);

	void VertexSmooth(int32 VertexIndex, float DistanceFraction, const FSculptSettings& Settings);

	void VertexChangeHeight(int32 VertexIndex, float DistanceFraction, const FSculptSettings& Settings);

	void VertexAddNoise(int32 VertexIndex, float DistanceFraction, const FSculptSettings& Settings);

	void VertexPaint(int32 VertexIndex, float DistanceFraction, const FSculptSettings& Settings);

	void VertexFlatten(int32 VertexIndex, float DistanceFraction, const FSculptSettings& Settings, const FSculptInputInfo& InputInfo);

	FVector CalculateVertexNormalByNoise(FVector Coordinates);

	void FillGlobalNormals();

	void AddAffectedSections(int32 SectionIndex, int32 VertexIndex, OUT TArray<int32> &AffectedSections);

	FColor CombineColors(FColor A, FColor B);

	FColor ColorSubtract(FColor A, FColor B);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY()
	TArray<FVertexData> GlobalVertexData;

	TArray<int32> IndexBuffer;

	TArray<int32> SectionUpdateQueue; // TODO replace with TQueue

	//TQueue<int32> SectionUpdateQueue2;

	//TArray<int32> CreateTangentsForMeshQueue;

	//bool bAllowCreatingTangents = false;

	bool bAllowedToUpdateSection = true;

	TArray<ATerrainSection*> SectionActors;

	void GenerateMeshTimed();

	void FillGlobalVertexDataTimed();

	void FillIndexBufferTimed(); // TODO give better name

	FTimerHandle SectionCreateTimerHandle;

	int32 SectionIndexIter = 0;

	int32 IndexBufferIter = 0;

	int32 GlobalXIter = 0;

	//UPROPERTY()
	//TArray<UProceduralMeshComponent*> ProceduralMeshSections;

	//APlayerController* PlayerControllerRef = nullptr;
};

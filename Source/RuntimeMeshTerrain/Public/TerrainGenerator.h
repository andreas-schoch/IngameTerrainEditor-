// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "TerrainEditorStuff.h"
#include "ProceduralMeshComponent.h"
#include "RuntimeMeshComponent.h"
#include "TerrainGenerator.generated.h"


class ATerrainSection;
//class URuntimeMeshComponent;


/*UENUM(BlueprintType)
enum class ESectionPosition : uint8
{
	SB_NotOnBorder		UMETA(DisplayName = "NotOnBorder"),
	SB_BorderLeft 		UMETA(DisplayName = "BorderLeft"),
	SB_BorderRight		UMETA(DisplayName = "BorderRight"),
	SB_BorderTop 		UMETA(DisplayName = "BorderTop"),
	SB_BorderBottom 	UMETA(DisplayName = "BorderBottom"),
	SB_EdgeTopLeft 		UMETA(DisplayName = "EdgeTopLeft"),
	SB_EdgeTopRight 	UMETA(DisplayName = "EdgeTopRight"),
	SB_EdgeBottomLeft 	UMETA(DisplayName = "EdgeBottomLeft"),
	SB_EdgeBottomRight 	UMETA(DisplayName = "EdgeBottomRight")
};


UENUM(BlueprintType)
enum class ESculptMode : uint8
{
	ST_Raise		UMETA(DisplayName = "Raise"),
	ST_Lower		UMETA(DisplayName = "Lower"),
	ST_Flatten		UMETA(DisplayName = "Flatten"),
	ST_Smooth		UMETA(DisplayName = "Smooth"),
};


// Struct that constains global Vertex information
USTRUCT(BlueprintType)
struct FVertexData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	FVector Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	FVector2D UV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	FVector Normals;

	// Vertex Color will be stored here aswell, currently used to visualize section borders

	FVertexData()
	{
	}
};


// Struct that constains necessary Vertex information of a single section
USTRUCT(BlueprintType)
struct FSectionProperties
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	TArray<FVector> Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	TArray<FVector2D> UV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	TArray<FVector> Normals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct") // TODO remember to store vertexcolor in global vert data
	TArray<FColor> VertexColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	TArray<int32> Triangles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	TArray<ESectionPosition> SectionPosition;

	FSectionProperties()
	{
	}
};
*/

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

	// visibility range from player pawn in cm
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float SectionVisibilityRange = 80000;

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

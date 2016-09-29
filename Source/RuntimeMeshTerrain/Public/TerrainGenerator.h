// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "TerrainGenerator.generated.h"


class ATerrainSection;
class URuntimeMeshComponent;


UENUM(BlueprintType)
enum class EVertPositionInsideSection : uint8
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


// Struct that constains global Vertex information
USTRUCT(BlueprintType)
struct FGlobalProperties
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
		TArray<FVector> Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
		TArray<FVector2D> UV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
		TArray<FVector> Normals;

	FGlobalProperties()
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
		TArray<FColor> VertexColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
		TArray<int32> Triangles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
		TArray<EVertPositionInsideSection> PositionInsideSection;

	FSectionProperties()
	{
	}
};


UCLASS()
class RUNTIMEMESHTERRAIN_API ATerrainGenerator : public AActor
{
	GENERATED_BODY()
	
public:
	// Main function to generate Mesh
	UFUNCTION(BlueprintCallable, Category = "ProceduralMeshGeneration")
	void GenerateMesh();


	// Getters 
	UFUNCTION(BlueprintPure, Category = "ProceduralMeshGeneration")
	int32 GetSectionXY() const { return SectionXY; }
	UFUNCTION(BlueprintPure, Category = "ProceduralMeshGeneration")
	float GetQuadSize() const { return QuadSize; }
	UFUNCTION(BlueprintPure, Category = "ProceduralMeshGeneration")
	int32 GetComponentXY() const { return ComponentXY; }

	// called from section actor on projectile hit
	void SectionRequestsUpdate(int32 SectionIndex, FVector HitLocation);
	void SectionUpdateFinished();

	FSectionProperties SectionProperties;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	int32 ComponentXY = 1;
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	int32 SectionXY = 20;
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float QuadSize = 100;
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float LineTraceLength = 10000;
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float LineTraceHeightOffset = 100;
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float SectionVisibilityRange = 80000;
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	int32 HitRadius = 2;
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	bool bUseUpdateQueue = true;
	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	float CreateSectionTimerDelay = 0.1;

	// class that acts as a mesh section
	UPROPERTY(EditDefaultsOnly, Category = "ProceduralMeshGeneration")
	TSubclassOf<ATerrainSection> ClassToSpawnAsSection;

private:
	ATerrainGenerator();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void InitializeProperties();
	void FillIndexBuffer();
	void AddBorderVerticesToSectionProperties();

	void FillGlobalProperties();
	void CopyLandscapeHeightBelow(FVector& Coordinates, FVector& Normal);

	void SpawnSectionActors();
	void SpawnSectionActorsWithTimer();
	void FillSectionVertStruct(int32 SectionIndex);
	//void FillIndexBufferSection(int32 XComp, int32 YComp);

	void MakeCrater(int32 SectionIndex, FVector HitLocation);



	UPROPERTY(VisibleAnywhere, Category = "Components")
	URuntimeMeshComponent* RuntimeMeshComponent = nullptr;

	FGlobalProperties GlobalProperties;
	TArray<int32> IndexBuffer;

	TArray<int32> SectionUpdateQueue; // TODO replace with TQueue
	bool bAllowedToUpdateSection = true;
	TArray<int32> SectionCreateQueue;
	TArray<ATerrainSection*> SectionActors;

	FTimerHandle SectionCreateTimerHandle;
	int32 SectionIndexIter = 0;
	
	
};

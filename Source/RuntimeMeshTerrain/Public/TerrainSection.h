// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "TerrainEditorStuff.h"
#include "TerrainSection.generated.h"

class URuntimeMeshComponent;
class UProceduralMeshComponent;
class ATerrainGenerator;

UCLASS()
class RUNTIMEMESHTERRAIN_API ATerrainSection : public AActor
{
	GENERATED_BODY()
	
public:	
	ATerrainSection();
	void InitializeOnSpawn(int32 SectionIndex, FVector2D ComponentCoordinates, ATerrainGenerator* Terrain);
	void CreateSection();
	void UpdateSection();
	void CreateLOD1();
	void UpdateLOD1();


	FVector2D SectionCoordinates;
	FVector2D SectionCenterWorldLocation2D;

	UPROPERTY(EditAnywhere, Category = "ProceduralMeshGeneration")
	bool bUseRuntimeMeshComponent = true;

	UFUNCTION(BlueprintCallable, Category = "ProceduralMeshGeneration")
	void RequestSculpting(FSculptSettings SculptSettings, FVector HitLocation, ESculptInput SculptInput, FVector StartLocation);

private:
	virtual void BeginPlay() override;
	void SetVisibility();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	URuntimeMeshComponent* RuntimeMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UProceduralMeshComponent* ProceduralMeshComponent = nullptr;

	URuntimeMeshComponent* RuntimeMeshComponentLOD1 = nullptr;
	UProceduralMeshComponent* ProceduralMeshComponentLOD1 = nullptr;



	ATerrainGenerator* OwningTerrain = nullptr;

	APlayerController* PlayerControllerReference = nullptr;

	int32 SectionIndexLocal = 0;

	TArray<int32> IndexBufferLocal;

	FTimerHandle VisibilityTimerHandle;
};

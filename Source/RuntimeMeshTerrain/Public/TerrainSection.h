// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "TerrainGenerator.h" //TODO move ESculptEnum to standalone class to prevent having to include generator everywhere
#include "TerrainSection.generated.h"

class URuntimeMeshComponent;

UCLASS()
class RUNTIMEMESHTERRAIN_API ATerrainSection : public AActor
{
	GENERATED_BODY()
	
public:	
	ATerrainSection();

	void InitializeOnSpawn(int32 SectionIndex, FVector2D ComponentCoordinates, ATerrainGenerator* Terrain);
	void CreateSection();
	void UpdateSection();
	FVector2D SectionCoordinates;
	FVector2D SectionCenterWorldLocation2D;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, FVector NormalImpulse, const FHitResult & Hit);

	UFUNCTION(BlueprintCallable, Category = "ProceduralMeshGeneration")
	void RequestSculpting(FVector HitLocation, ESculptMode SculptMode, float ToolStrength, float ToolRadius, bool bUseUpdateQueue);

	TArray<FVector*> SectionVerticesPtr;
	TIndirectArray<FVector*> SectionVerticesPtr2; // TODO test if performance benefits from storing a ptr to all verts needed from globalproperties

private:
	virtual void BeginPlay() override;
	void SetVisibility();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	URuntimeMeshComponent* RuntimeMeshComponent = nullptr;

	ATerrainGenerator* OwningTerrain = nullptr;

	APlayerController* PlayerControllerReference = nullptr;

	int32 SectionIndexLocal = 0;

	TArray<int32> IndexBufferLocal;

	FTimerHandle VisibilityTimerHandle;
};

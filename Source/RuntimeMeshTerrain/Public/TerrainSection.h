// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "TerrainSection.generated.h"


class ATerrainGenerator;
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


private:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetVisibility();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	URuntimeMeshComponent* RuntimeMeshComponent = nullptr;

	ATerrainGenerator* OwningTerrain = nullptr;

	int32 SectionIndexLocal = 0;
	TArray<int32> IndexBufferLocal;


	APlayerController* PlayerControllerReference = nullptr;

	FTimerHandle VisibilityTimerHandle;


	
	
};

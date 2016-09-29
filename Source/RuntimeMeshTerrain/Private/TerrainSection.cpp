// Fill out your copyright notice in the Description page of Project Settings.

#include "RuntimeMeshTerrain.h"
#include "RuntimeMeshComponent.h"
#include "TerrainGenerator.h"
#include "TerrainSection.h"


ATerrainSection::ATerrainSection()
{
	PrimaryActorTick.bCanEverTick = true;
	// Create a RuntimeMeshComponent and make it the root
	RuntimeMeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponent"));
	RootComponent = RuntimeMeshComponent;
}


void ATerrainSection::BeginPlay()
{
	Super::BeginPlay();
	RuntimeMeshComponent->OnComponentHit.AddDynamic(this, &ATerrainSection::OnHit);
}


// References the owning Terrain Generator and initializes some variables
void ATerrainSection::InitializeOnSpawn(int32 SectionIndex, FVector2D ComponentCoordinates, ATerrainGenerator* Terrain)
{
	OwningTerrain = Terrain;
	SectionIndexLocal = SectionIndex;
	SectionCoordinates = ComponentCoordinates;

	auto SectionSideInCM = OwningTerrain->GetSectionXY() * OwningTerrain->GetQuadSize();
	SectionCenterWorldLocation2D = FVector2D(ComponentCoordinates.X * SectionSideInCM, ComponentCoordinates.Y * SectionSideInCM) + FVector2D(GetActorLocation().X, GetActorLocation().Y);

	PlayerControllerReference = GetWorld()->GetFirstPlayerController();
}


void ATerrainSection::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Set Visibility
	if (!ensure(PlayerControllerReference)) { return; }
	auto PlayerPawnWorldLocation = PlayerControllerReference->GetPawn()->GetActorLocation();
	FVector2D PlayerPawnWorldLocation2D = FVector2D(PlayerPawnWorldLocation.X, PlayerPawnWorldLocation.Y);
	float DistanceToPlayerPawn2D = FVector2D::Distance(PlayerPawnWorldLocation2D, SectionCenterWorldLocation2D);

	if (DistanceToPlayerPawn2D > OwningTerrain->SectionVisibilityRange && RuntimeMeshComponent->IsVisible())
	{
		RuntimeMeshComponent->SetVisibility(false);
	}
	else if (DistanceToPlayerPawn2D < OwningTerrain->SectionVisibilityRange && !RuntimeMeshComponent->IsVisible())
	{
		RuntimeMeshComponent->SetVisibility(true);
		//RuntimeMeshComponent->SetMeshSectionCollisionEnabled(0, true);
	}
}


// Called from Terrain Generator on spawn
void ATerrainSection::CreateSection()
{
	TArray<FRuntimeMeshTangent> Tangents;
	FSectionProperties* SectionPropertiesPtr = &OwningTerrain->SectionProperties;

	RuntimeMeshComponent->CreateMeshSection(
		0,
		SectionPropertiesPtr->Vertices,
		SectionPropertiesPtr->Triangles,
		SectionPropertiesPtr->Normals,
		SectionPropertiesPtr->UV,
		SectionPropertiesPtr->VertexColors,
		Tangents,
		true,
		EUpdateFrequency::Frequent);
}


// Send request to Terrain Generator whenever a projectile hits the RuntimeMeshComponent of this section
void ATerrainSection::OnHit(UPrimitiveComponent* HitComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, FVector NormalImpulse, const FHitResult & Hit)
{
	if (!ensure(OwningTerrain)) { return; }
	if (OtherActor == PlayerControllerReference->GetPawn()) { return; }

	OwningTerrain->SectionRequestsUpdate(SectionIndexLocal, Hit.Location);

	UE_LOG(LogTemp, Error, TEXT("%s was HIT by Actor: %s  Component: %s"), *HitComponent->GetName(), *OtherActor->GetName(), *OtherComp->GetName())

}


// Called from Terrain Generator after receiving update request
void ATerrainSection::UpdateSection()
{
	//auto SectionProperties = OwningTerrain->GetSectionProperties();
	TArray<FRuntimeMeshTangent> Tangents;
	//FSectionProperties SectionPropertiesPtr = OwningTerrain->SectionProperties;
	FSectionProperties* SectionPropertiesPtr = &OwningTerrain->SectionProperties;

	RuntimeMeshComponent->UpdateMeshSection(
		0,
		SectionPropertiesPtr->Vertices,
		SectionPropertiesPtr->Normals,
		SectionPropertiesPtr->UV,
		SectionPropertiesPtr->VertexColors,
		Tangents);

	OwningTerrain->SectionUpdateFinished();
}
// Fill out your copyright notice in the Description page of Project Settings.

#include "RuntimeMeshTerrain.h"
#include "RuntimeMeshComponent.h"
#include "TerrainGenerator.h"
#include "TerrainSection.h"


ATerrainSection::ATerrainSection()
{
	PrimaryActorTick.bCanEverTick = false;
	RuntimeMeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponent"));
	RootComponent = RuntimeMeshComponent;
}


void ATerrainSection::BeginPlay()
{
	Super::BeginPlay();
	RuntimeMeshComponent->OnComponentHit.AddDynamic(this, &ATerrainSection::OnHit);
	GetWorld()->GetTimerManager().SetTimer(VisibilityTimerHandle, this, &ATerrainSection::SetVisibility, 0.3, true, 0.3);
}


void ATerrainSection::InitializeOnSpawn(int32 SectionIndex, FVector2D ComponentCoordinates, ATerrainGenerator* Terrain)
{
	OwningTerrain = Terrain;
	SectionIndexLocal = SectionIndex;
	SectionCoordinates = FVector2D(SectionIndex / OwningTerrain->GetSectionXY(), SectionIndex % OwningTerrain->GetSectionXY());

	auto SectionSideInCM = (OwningTerrain->GetSectionXY()-1) * OwningTerrain->GetQuadSize();
	auto Half = SectionSideInCM / 2;
	SectionCenterWorldLocation2D = FVector2D(GetActorLocation().X + ComponentCoordinates.X * SectionSideInCM + Half, GetActorLocation().Y + ComponentCoordinates.Y * SectionSideInCM + Half);// FVector2D(ComponentCoordinates.X * SectionSideInCM, ComponentCoordinates.Y * SectionSideInCM) + FVector2D(GetActorLocation().X, GetActorLocation().Y);

	PlayerControllerReference = GetWorld()->GetFirstPlayerController();
}


void ATerrainSection::CreateSection()
{
	// Called from Terrain Generator on spawn
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


void ATerrainSection::OnHit(UPrimitiveComponent* HitComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, FVector NormalImpulse, const FHitResult & Hit)
{
	// Send request to Terrain Generator whenever a projectile hits the RuntimeMeshComponent of this section
	if (!ensure(OwningTerrain)) { return; }
	if (OtherActor == PlayerControllerReference->GetPawn()) { return; }
	OwningTerrain->SectionRequestsUpdate(SectionIndexLocal, Hit.Location, ESculptMode::ST_Sculpt, 1, 500, true);
	//UE_LOG(LogTemp, Error, TEXT("%s was HIT by Actor: %s  Component: %s"), *HitComponent->GetName(), *OtherActor->GetName(), *OtherComp->GetName())
}


void ATerrainSection::UpdateSection()
{
	// Called from Terrain Generator after receiving update request
	TArray<FRuntimeMeshTangent> Tangents;
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


void ATerrainSection::SetVisibility()
{
	if (!ensure(PlayerControllerReference)) { return; }
	auto PlayerPawnWorldLocation = PlayerControllerReference->GetPawn()->GetActorLocation();
	FVector2D PlayerPawnWorldLocation2D = FVector2D(PlayerPawnWorldLocation.X, PlayerPawnWorldLocation.Y);
	float DistanceToPawn = FVector2D::Distance(PlayerPawnWorldLocation2D, SectionCenterWorldLocation2D);

	if (DistanceToPawn > OwningTerrain->SectionVisibilityRange && RuntimeMeshComponent->IsVisible())
	{
		RuntimeMeshComponent->SetVisibility(false);
		RuntimeMeshComponent->SetMeshSectionCollisionEnabled(0, false);
	}
	else if (DistanceToPawn < OwningTerrain->SectionVisibilityRange && !RuntimeMeshComponent->IsVisible())
	{
		RuntimeMeshComponent->SetVisibility(true);
		RuntimeMeshComponent->SetMeshSectionCollisionEnabled(0, true);
	}
}


void ATerrainSection::RequestSculpting(FVector HitLocation, ESculptMode SculptMode, float ToolStrength, float ToolRadius, bool bUseUpdateQueue)
{
	OwningTerrain->SectionRequestsUpdate(SectionIndexLocal, HitLocation, SculptMode, ToolStrength, ToolRadius, bUseUpdateQueue);
}
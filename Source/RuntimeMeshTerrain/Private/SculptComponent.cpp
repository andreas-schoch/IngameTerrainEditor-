// Fill out your copyright notice in the Description page of Project Settings.

#include "RuntimeMeshTerrain.h"
#include "TerrainSection.h"
#include "SculptComponent.h"


USculptComponent::USculptComponent()
{
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
}


void USculptComponent::BeginPlay()
{
	Super::BeginPlay();
}


void USculptComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
}


void USculptComponent::SculptStart(UCameraComponent* Camera)
{
	OwnerCamera = Camera;
	GetWorld()->GetTimerManager().SetTimer(SculptTimerHandle, this, &USculptComponent::Sculpt, GetWorld()->DeltaTimeSeconds, true);
}


void USculptComponent::SculptStop()
{
	GetWorld()->GetTimerManager().PauseTimer(SculptTimerHandle);
}


void USculptComponent::Sculpt()
{
	FHitResult Hit;
	TArray<AActor*> ToIgnore;
	FVector TraceStart = OwnerCamera->GetComponentLocation();
	FVector TraceEnd = TraceStart + (OwnerCamera->GetForwardVector() * 100000); // TODO add under mouse option
	UKismetSystemLibrary::LineTraceSingle_NEW(
		this,
		TraceStart,
		TraceEnd,
		UEngineTypes::ConvertToTraceType(ECC_WorldStatic),
		false,
		ToIgnore,
		EDrawDebugTrace::ForOneFrame,
		OUT Hit,
		true);

	// only send sculpt request if distance greater than xxx TODO remove magic number
	FVector2D HitLocation2D = FVector2D(Hit.Location.X, Hit.Location.Y);
	if (FVector2D::Distance(LastSculptLocation2D, HitLocation2D) < 300) { return; }
	LastSculptLocation2D = HitLocation2D;

	// Send Sculpt request to hit section
	ATerrainSection* HitSection = dynamic_cast<ATerrainSection*>(Hit.GetActor());
	if (!ensure(HitSection)) { return; }
	HitSection->RequestSculpting(Hit.Location, SculptMode, ToolStrength, SculptRadius, bUseQueueToUpdate);
}
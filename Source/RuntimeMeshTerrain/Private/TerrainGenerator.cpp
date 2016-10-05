// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#include "RuntimeMeshTerrain.h"
#include "RuntimeMeshComponent.h" 
#include "RuntimeMeshLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetProceduralMeshLibrary.h"
#include "TerrainSection.h"
#include "TerrainGenerator.h"


ATerrainGenerator::ATerrainGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	RuntimeMeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponent"));
	RootComponent = RuntimeMeshComponent;
}


void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();
	bUseTimerforGeneration ? GenerateMeshTimed() : GenerateMesh();
}


void ATerrainGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// check if any sections request to be updated
	if (SectionUpdateQueue.Num() > 0 && bAllowedToUpdateSection)
	{
		bAllowedToUpdateSection = false;
		if (!SectionActors.IsValidIndex(SectionUpdateQueue[0])) { return; }
		FillSectionVertStruct(SectionUpdateQueue[0]);
		SectionActors[SectionUpdateQueue[0]]->UpdateSection();
	}

	// check if any sections request to update tangents
	if (CreateTangentsForMeshQueue.Num() > 0 && bAllowCreatingTangents)
	{
		bAllowedToUpdateSection = false;
		if (!SectionActors.IsValidIndex(CreateTangentsForMeshQueue[0])) { return; }
		FillSectionVertStruct(CreateTangentsForMeshQueue[0]);

		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
		SectionProperties.Vertices,
		SectionProperties.Triangles,
		SectionProperties.UV,
		OUT SectionProperties.Normals,
		OUT DummyTangents);

		SectionActors[CreateTangentsForMeshQueue[0]]->UpdateSection();
		UE_LOG(LogTemp, Warning, TEXT("Run 'CreateTangentsForMesh' for Section: %i"), CreateTangentsForMeshQueue[0]);
		if (CreateTangentsForMeshQueue.IsValidIndex(0)) { CreateTangentsForMeshQueue.RemoveAt(0); }
	}
}


void ATerrainGenerator::GenerateMesh()
{
	// Main Function (freezes gamethread with big terrains)
	InitializeProperties();
	FillGlobalProperties();
	FillIndexBuffer();
	SpawnSectionActors();
}


void ATerrainGenerator::GenerateMeshTimed()
{
	// Main Function alternative (no gamethread freezing but takes some time too)
	InitializeProperties();
	FillGlobalPropertiesTimed(); // TODO rename
}


void ATerrainGenerator::InitializeProperties()
{
	int32 ArraySizeGlobal = SectionXY * SectionXY * ComponentXY * ComponentXY;
	IndexBuffer.SetNum(ArraySizeGlobal, true);

	int32 NumOfSections = ComponentXY * ComponentXY;
	SectionActors.SetNum(NumOfSections, true);

	// Init GlobalProperties
	int32 MeshVertsPerSide = SectionXY * ComponentXY - (ComponentXY - 1);
	int32 TotalNumOfVerts = MeshVertsPerSide * MeshVertsPerSide;
	GlobalVertexData.SetNum(TotalNumOfVerts, true);

	// Init SectionProperties
	int32 SectionNumVerts = SectionXY * SectionXY;
	SectionProperties.Vertices.SetNum(SectionNumVerts, true);
	SectionProperties.UV.SetNum(SectionNumVerts, true);
	SectionProperties.Normals.SetNum(SectionNumVerts, true);
	SectionProperties.VertexColors.SetNum(SectionNumVerts, true);
	SectionProperties.SectionPosition.SetNum(SectionNumVerts, true);
	URuntimeMeshLibrary::CreateGridMeshTriangles(SectionXY, SectionXY, false, OUT SectionProperties.Triangles);
	AddBorderVerticesToSectionProperties();
}


void ATerrainGenerator::FillIndexBuffer()
{
	int32 ArraySizeGlobal = SectionXY * SectionXY * ComponentXY * ComponentXY;
	IndexBuffer.SetNum(ArraySizeGlobal, true);

	int32 Iterator = 0;
	int32 QuadsPerSide = SectionXY - 1;
	int32 GlobalXYVerts = QuadsPerSide * ComponentXY + 1;
	for (int XComp = 0; XComp < ComponentXY; XComp++)
	{
		for (int YComp = 0; YComp < ComponentXY; YComp++)
		{
			int32 SectionRoot = ((GlobalXYVerts * QuadsPerSide) * XComp) + (QuadsPerSide * YComp);
			for (int XSection = 0; XSection < SectionXY; XSection++)
			{
				for (int YSection = 0; YSection < SectionXY; YSection++)
				{
					int32 IndexToAdd = GlobalXYVerts * XSection + YSection;
					int32 IndexTotal = SectionRoot + IndexToAdd;
					IndexBuffer[Iterator] = IndexTotal;
					Iterator++;
				}
			}
		}
	}
}


void ATerrainGenerator::FillIndexBufferTimed()
{
	// Fill Index Buffer (Partially, then spawn coresponding section)
	int32 XComp = SectionIndexIter / ComponentXY;
	int32 YComp = SectionIndexIter % ComponentXY;

	int32 QuadsPerSide = SectionXY - 1;
	int32 GlobalXYVerts = QuadsPerSide * ComponentXY + 1;
	int32 SectionRoot = ((GlobalXYVerts * QuadsPerSide) * XComp) + (QuadsPerSide * YComp);
	for (int XSection = 0; XSection < SectionXY; XSection++)
	{
		for (int YSection = 0; YSection < SectionXY; YSection++)
		{
			int32 IndexToAdd = GlobalXYVerts * XSection + YSection;
			int32 IndexTotal = SectionRoot + IndexToAdd;
			IndexBuffer[IndexBufferIter] = IndexTotal;
			IndexBufferIter++;
		}
	}

	// Spawn TerrainSectionActor
	SectionActors[SectionIndexIter] = GetWorld()->SpawnActor<ATerrainSection>(
		ClassToSpawnAsSection,
		GetActorLocation(),
		GetActorRotation());
	SectionActors[SectionIndexIter]->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	SectionActors[SectionIndexIter]->InitializeOnSpawn(SectionIndexIter, FVector2D(XComp, YComp), this);

	// Create Section
	FillSectionVertStruct(SectionIndexIter);
	SectionActors[SectionIndexIter]->CreateSection();

	// Recursive function call with a timer to prevent freezing of the gamethread
	SectionIndexIter++;
	if (SectionIndexIter >= ComponentXY * ComponentXY) { return; }
	GetWorld()->GetTimerManager().SetTimer(SectionCreateTimerHandle, this, &ATerrainGenerator::FillIndexBufferTimed, CreateSectionTimerDelay, false);
}


void ATerrainGenerator::AddBorderVerticesToSectionProperties()
{
	for (int32 X = 0; X < SectionXY; X++)
	{
		for (int32 Y = 0; Y < SectionXY; Y++)
		{
			int32 i = X * SectionXY + Y;
			FVector2D Ratio = FVector2D(X, Y) / FVector2D(SectionXY - 1, SectionXY - 1);
			ESectionPosition VertPositionInsideSection = ESectionPosition::SB_NotOnBorder;

			if (Ratio.Equals(FVector2D(0, 0))) { VertPositionInsideSection = ESectionPosition::SB_EdgeBottomLeft; }
			if (Ratio.Equals(FVector2D(1, 0))) { VertPositionInsideSection = ESectionPosition::SB_EdgeTopLeft; }
			if (Ratio.Equals(FVector2D(0, 1))) { VertPositionInsideSection = ESectionPosition::SB_EdgeBottomRight; }
			if (Ratio.Equals(FVector2D(1, 1))) { VertPositionInsideSection = ESectionPosition::SB_EdgeTopRight; }

			if (VertPositionInsideSection == ESectionPosition::SB_NotOnBorder)
			{
				if (FMath::IsNearlyEqual(Ratio.X, 0)) { VertPositionInsideSection = ESectionPosition::SB_BorderBottom; }
				if (FMath::IsNearlyEqual(Ratio.X, 1)) { VertPositionInsideSection = ESectionPosition::SB_BorderTop; }
				if (FMath::IsNearlyEqual(Ratio.Y, 0)) { VertPositionInsideSection = ESectionPosition::SB_BorderLeft; }
				if (FMath::IsNearlyEqual(Ratio.Y, 1)) { VertPositionInsideSection = ESectionPosition::SB_BorderRight; }
			}

			SectionProperties.SectionPosition[i] = VertPositionInsideSection;
			SectionProperties.VertexColors[i] = (VertPositionInsideSection == ESectionPosition::SB_NotOnBorder) ? (FColor(255, 255, 255, 0.0)) : (FColor(255, 0, 0, 1));
		}
	}
}


void ATerrainGenerator::FillGlobalProperties()
{
	// Get GlobalProperties Vertex & UV Coordinates
	int32 i = 0;
	int32 VertsPerSide = (ComponentXY * SectionXY - (ComponentXY - 1));
	for (int32 X = 0; X < VertsPerSide; X++)
	{
		for (int32 Y = 0; Y < VertsPerSide; Y++)
		{
			// Set Vertex, UV, Normal
			FVector VertCoords = FVector(X, Y, 0) * QuadSize;
			CopyLandscapeHeightBelow(OUT VertCoords, OUT GlobalVertexData[i].Normals);
			GlobalVertexData[i].Vertices = VertCoords;
			GlobalVertexData[i].UV = FVector2D(X, Y);
			i++;
		}
	}
}


void ATerrainGenerator::FillGlobalPropertiesTimed()
{
	int32 MeshVertsPerSide = SectionXY * ComponentXY - (ComponentXY - 1);
	int32 TotalNumOfVerts = MeshVertsPerSide * MeshVertsPerSide;

	// Get GlobalProperties Vertex & UV Coordinates
	for (int i = 0; i < MeshVertsPerSide; i++)
	{
		int32 X = GlobalXIter;
		int32 Y = i;
		int32 PropertiesIndex = X * MeshVertsPerSide + Y;
		if (!GlobalVertexData.IsValidIndex(PropertiesIndex)) { UE_LOG(LogTemp, Error, TEXT("IndexNotValid")); }

		// Set Vertex, UV, Normal
		FVector VertCoords = FVector(X, Y, 0) * QuadSize;
		CopyLandscapeHeightBelow(OUT VertCoords, OUT GlobalVertexData[PropertiesIndex].Normals);
		GlobalVertexData[PropertiesIndex].Vertices	= VertCoords;
		GlobalVertexData[PropertiesIndex].UV		= FVector2D(X, Y);
	}

	UE_LOG(LogTemp, Warning, TEXT("GlobalPropertiesPercentFilled: %f % "), ((float)GlobalXIter/ (float)MeshVertsPerSide) * 100.f);

	// Recursive function call with a timer to prevent freezing of the gamethread
	GlobalXIter++;
	if (GlobalXIter + 1 > MeshVertsPerSide) { FillIndexBufferTimed(); return; }
	GetWorld()->GetTimerManager().SetTimer(SectionCreateTimerHandle, this, &ATerrainGenerator::FillGlobalPropertiesTimed, CreateSectionTimerDelay,false);
}


void ATerrainGenerator::CopyLandscapeHeightBelow(FVector &Coordinates, FVector& Normal)
{
	FHitResult Hit;
	FVector Start = Coordinates + GetActorLocation();
	FVector End = Start - FVector(0, 0, LineTraceLength);
	
	GetWorld()->LineTraceSingleByChannel(OUT Hit, Start, End, ECollisionChannel::ECC_WorldStatic);

	float LineTraceHeight = Hit.Location.Z - GetActorLocation().Z + LineTraceHeightOffset;
	Coordinates = FVector(Coordinates.X, Coordinates.Y, LineTraceHeight);
	Normal = (Hit.bBlockingHit) ? Normal = Hit.Normal : Normal = FVector(0, 0, 1);
}


void ATerrainGenerator::SpawnSectionActors()
{
	if (!ClassToSpawnAsSection) { UE_LOG(LogTemp, Error, TEXT("ClassToSpawnAsSection Not Set in TerrainGenerator BP")); return; }

	// Iterate through amount of Components/Sections
	for (int32 X = 0; X < ComponentXY; X++)
	{
		for (int32 Y = 0; Y < ComponentXY; Y++)
		{
			// Spawn the SectionActor & Attach to this
			int32 SectionIndex = X * ComponentXY + Y;
			SectionActors[SectionIndex] = GetWorld()->SpawnActor<ATerrainSection>(
				ClassToSpawnAsSection,
				GetActorLocation(),
				GetActorRotation());
			SectionActors[SectionIndex]->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			SectionActors[SectionIndex]->InitializeOnSpawn(SectionIndex, FVector2D(X, Y), this);

			// Fill SectionProperties and create section inside SectionActor
			FillSectionVertStruct(SectionIndex);
			SectionActors[SectionIndex]->CreateSection();
		}
	}
}


void ATerrainGenerator::FillSectionVertStruct(int32 SectionIndex)
{
	int32 IndexStart = SectionXY * SectionXY * SectionIndex;
	int32 IndexEnd = IndexStart + (SectionXY * SectionXY);
	for (int i = 0; i + IndexStart < IndexEnd; i++)
	{
		if (SectionProperties.Vertices.IsValidIndex(i))
		{
			if (!IndexBuffer.IsValidIndex(i + IndexStart)) { return; }
			int32 Index = IndexBuffer[i + IndexStart];
			SectionProperties.Vertices[i]	= GlobalVertexData[Index].Vertices;
			SectionProperties.Normals[i]	= GlobalVertexData[Index].Normals;
			SectionProperties.UV[i]			= GlobalVertexData[Index].UV;
		}
	}
}


void ATerrainGenerator::SectionRequestsUpdate(int32 SectionIndex, FSculptSettings SculptSettings, FVector HitLocation, ESculptInput SculptInput, FVector StartLocation)
{
	MakeCrater(SectionIndex, SculptSettings, HitLocation, SculptInput, StartLocation);
}


void ATerrainGenerator::MakeCrater(int32 SectionIndex, FSculptSettings SculptSettings, FVector HitLocation, ESculptInput SculptInput, FVector StartLocation)
{
	switch (SculptInput)
	{
	case ESculptInput::ST_Started:
		bAllowCreatingTangents = false;
		UE_LOG(LogTemp, Warning, TEXT("ST_Started"));
		break;
	case ESculptInput::ST_Ongoing:
		UE_LOG(LogTemp, Warning, TEXT("ST_Ongoing"));
		break;
	case ESculptInput::ST_Stopped:
		bAllowCreatingTangents = true;
		UE_LOG(LogTemp, Warning, TEXT("ST_Stopped"));
		break;
	}

	
	TArray<int32> AffectedSections;
	FVector RelativeHitLocation = (HitLocation - GetActorLocation());
	FVector CenterCoordinates = FVector(FMath::RoundToInt(RelativeHitLocation.X / QuadSize), FMath::RoundToInt(RelativeHitLocation.Y / QuadSize), 0);
	int32 VertsPerSide = ((SectionXY - 1) * ComponentXY + 1);
	int32 CenterIndex = CenterCoordinates.X * VertsPerSide + CenterCoordinates.Y;
	FVector SectionCoordinates = FVector(SectionIndex / (ComponentXY), SectionIndex % (ComponentXY), 0);
	int32 ScaledZStrength = MaxZValueOffsetPerUpdate * SculptSettings.ToolStrength;

	// Modify Verts around impact to make a crater
	int32 RadiusInVerts = SculptSettings.SculptRadius / QuadSize;
	for (int32 X = -RadiusInVerts; X <= RadiusInVerts; X++)
	{
		for (int32 Y = -RadiusInVerts; Y <= RadiusInVerts; Y++)
		{
			// Continue loop if Vert doesn't exist
			int32 CurrentIndex = CenterIndex + (X * VertsPerSide) + Y;
			if (!GlobalVertexData.IsValidIndex(CurrentIndex)) { continue; }

			// Continue if not in radius
			FVector CurrentVertCoords = FVector(FMath::RoundToInt(GlobalVertexData[CurrentIndex].Vertices.X / QuadSize), FMath::RoundToInt(GlobalVertexData[CurrentIndex].Vertices.Y / QuadSize), 0);
			float DistanceFromCenter = FVector::Dist(CenterCoordinates, CurrentVertCoords);
			if (DistanceFromCenter > RadiusInVerts) { continue; }
			/////////////////////////////////////////////////////////////////////////////////////////////////////////
			
			if (SculptInput != ESculptInput::ST_Stopped)
			{
				float DistanceFraction = DistanceFromCenter / RadiusInVerts;
				float Alpha;
				float ZValue;
				switch (SculptSettings.SculptMode)
				{
				case ESculptMode::ST_Lower:
					Alpha = Curve->GetFloatValue(DistanceFraction) * SculptSettings.Falloff;
					ZValue = FMath::Lerp(ScaledZStrength, 0, Alpha);
					GlobalVertexData[CurrentIndex].Vertices -= FVector(0, 0, ZValue);
					break;

				case ESculptMode::ST_Raise:
					Alpha = Curve->GetFloatValue(DistanceFraction) * SculptSettings.Falloff;
					ZValue = FMath::Lerp(ScaledZStrength, 0, Alpha);
					GlobalVertexData[CurrentIndex].Vertices += FVector(0, 0, ZValue);
					break;

				case ESculptMode::ST_Flatten: // TODO fix center index issue
					 Alpha = Curve->GetFloatValue(DistanceFraction) * SculptSettings.Falloff;
					ZValue = FMath::Lerp(StartLocation.Z, GlobalVertexData[CurrentIndex].Vertices.Z, Alpha);
					float ZValue2 = FMath::Lerp(GlobalVertexData[CurrentIndex].Vertices.Z, ZValue, SculptSettings.ToolStrength);


					FVector Flatted = FVector(GlobalVertexData[CurrentIndex].Vertices.X, GlobalVertexData[CurrentIndex].Vertices.Y, ZValue2);
					GlobalVertexData[CurrentIndex].Vertices = Flatted;
					break;
				}
			}

			/*// Temporary get normals via line trace
			FHitResult Hit;
			FVector Start = GlobalVertexData[CurrentIndex].Vertices + GetActorLocation() + FVector(3,3,0);
			FVector End = Start - FVector(0, 0, LineTraceLength);
			GetWorld()->LineTraceSingleByChannel(OUT Hit, Start, End, ECollisionChannel::ECC_WorldStatic);
			GlobalVertexData[CurrentIndex].Normals = Hit.Normal;*/


			FVector SectionVertCoords = CurrentVertCoords - (SectionCoordinates * SectionXY - SectionCoordinates);
			int32 SectionVertIndex = SectionVertCoords.X * SectionXY + SectionVertCoords.Y;

			if (SectionVertCoords.X > SectionXY - 1 || SectionVertCoords.X < 0 || SectionVertCoords.Y > SectionXY - 1 || SectionVertCoords.Y < 0) { continue; }
			if (!SectionProperties.SectionPosition.IsValidIndex(SectionVertIndex)) { continue; }

			AddAffectedSections(SectionIndex, SectionVertIndex, OUT AffectedSections);
		}
	}
	for (int32 Iter : AffectedSections)
	{
		if (!SectionActors.IsValidIndex(Iter)) { continue; }
		
		// Add all sections that are getting updated to a queue
		if (SculptInput != ESculptInput::ST_Stopped && !CreateTangentsForMeshQueue.Contains(Iter))
		{
			CreateTangentsForMeshQueue.Add(Iter);
			UE_LOG(LogTemp, Error, TEXT("ADDED Section: %i to CreateTangentsForMeshQueue"), Iter);
		}


		if (SculptSettings.bUseUpdateQueue && !SectionUpdateQueue.Contains(Iter))
		{
			SectionUpdateQueue.Add(Iter);
		}
		else
		{
			FillSectionVertStruct(Iter);
			SectionActors[Iter]->UpdateSection();
		}
	}
}


void ATerrainGenerator::AddAffectedSections(int32 SectionIndex, int32 VertexIndex, OUT TArray<int32> &AffectedSections)
{
	int32 NeighbourSection;
	switch (SectionProperties.SectionPosition[VertexIndex])
	{
	case ESectionPosition::SB_NotOnBorder:
		if (!AffectedSections.Contains(SectionIndex)) { AffectedSections.Add(SectionIndex); }
		break;

	case ESectionPosition::SB_BorderRight:
		NeighbourSection = SectionIndex + 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_BorderLeft:
		NeighbourSection = SectionIndex - 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_BorderTop:
		NeighbourSection = SectionIndex + ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_BorderBottom:
		NeighbourSection = SectionIndex - ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_EdgeBottomLeft:
		NeighbourSection = SectionIndex - 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex - ComponentXY - 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex - ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_EdgeBottomRight:
		NeighbourSection = SectionIndex + 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex - ComponentXY + 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex - ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_EdgeTopLeft:
		NeighbourSection = SectionIndex - 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex + ComponentXY - 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex + ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_EdgeTopRight:
		NeighbourSection = SectionIndex + 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex + ComponentXY + 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex + ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;
	}
}


void ATerrainGenerator::SectionUpdateFinished()
{
	bAllowedToUpdateSection = true;
	if (SectionUpdateQueue.IsValidIndex(0)) { SectionUpdateQueue.RemoveAt(0); }
	
	//if (CreateTangentsForMeshQueue.IsValidIndex(0)) { CreateTangentsForMeshQueue.RemoveAt(0); }
}
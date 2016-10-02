// Fill out your copyright notice in the Description page of Project Settings.

#include "RuntimeMeshTerrain.h"
#include "RuntimeMeshComponent.h" 
#include "RuntimeMeshLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "TerrainSection.h"
#include "TerrainGenerator.h"


ATerrainGenerator::ATerrainGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	RuntimeMeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponent"));
	RootComponent = RuntimeMeshComponent;
}


// C++ Equivalent of Construction Script
void ATerrainGenerator::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);
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
		//UE_LOG(LogTemp, Warning, TEXT("Updated with UpdateQueue"))
	}
}


// Main Function (freezes gamethread with big terrains)
void ATerrainGenerator::GenerateMesh()
{
	InitializeProperties();
	FillGlobalProperties();
	FillIndexBuffer();
	SpawnSectionActors();
}


// Main Function alternative (no gamethread freezing but takes some time too)
void ATerrainGenerator::GenerateMeshTimed()
{
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
	GlobalProperties.Vertices.SetNum(TotalNumOfVerts, true);
	GlobalProperties.UV.SetNum(TotalNumOfVerts, true);
	GlobalProperties.Normals.SetNum(TotalNumOfVerts, true);

	// Init SectionProperties
	int32 NumOfVertsInSingleSection = SectionXY * SectionXY;
	SectionProperties.Vertices.SetNum(NumOfVertsInSingleSection, true);
	SectionProperties.UV.SetNum(NumOfVertsInSingleSection, true);
	SectionProperties.Normals.SetNum(NumOfVertsInSingleSection, true);
	SectionProperties.VertexColors.SetNum(NumOfVertsInSingleSection, true);
	SectionProperties.PositionInsideSection.SetNum(NumOfVertsInSingleSection, true);
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
			EVertPositionInsideSection VertPositionInsideSection = EVertPositionInsideSection::SB_NotOnBorder;

			if (Ratio.Equals(FVector2D(0, 0))) { VertPositionInsideSection = EVertPositionInsideSection::SB_EdgeBottomLeft; }
			if (Ratio.Equals(FVector2D(1, 0))) { VertPositionInsideSection = EVertPositionInsideSection::SB_EdgeTopLeft; }
			if (Ratio.Equals(FVector2D(0, 1))) { VertPositionInsideSection = EVertPositionInsideSection::SB_EdgeBottomRight; }
			if (Ratio.Equals(FVector2D(1, 1))) { VertPositionInsideSection = EVertPositionInsideSection::SB_EdgeTopRight; }

			if (VertPositionInsideSection == EVertPositionInsideSection::SB_NotOnBorder)
			{
				if (FMath::IsNearlyEqual(Ratio.X, 0)) { VertPositionInsideSection = EVertPositionInsideSection::SB_BorderBottom; }
				if (FMath::IsNearlyEqual(Ratio.X, 1)) { VertPositionInsideSection = EVertPositionInsideSection::SB_BorderTop; }
				if (FMath::IsNearlyEqual(Ratio.Y, 0)) { VertPositionInsideSection = EVertPositionInsideSection::SB_BorderLeft; }
				if (FMath::IsNearlyEqual(Ratio.Y, 1)) { VertPositionInsideSection = EVertPositionInsideSection::SB_BorderRight; }
			}

			SectionProperties.PositionInsideSection[i] = VertPositionInsideSection;
			SectionProperties.VertexColors[i] = (VertPositionInsideSection == EVertPositionInsideSection::SB_NotOnBorder) ? (FColor(255, 255, 255, 0.0)) : (FColor(255, 0, 0, 1));
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
			CopyLandscapeHeightBelow(OUT VertCoords, OUT GlobalProperties.Normals[i]);
			GlobalProperties.Vertices[i] = VertCoords;
			GlobalProperties.UV[i] = FVector2D(X, Y);
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
		if (!GlobalProperties.UV.IsValidIndex(PropertiesIndex)) { UE_LOG(LogTemp, Error, TEXT("IndexNotValid")); }


		// Set Vertex, UV, Normal
		FVector VertCoords = FVector(X, Y, 0) * QuadSize;
		CopyLandscapeHeightBelow(OUT VertCoords, OUT GlobalProperties.Normals[PropertiesIndex]);
		GlobalProperties.Vertices[PropertiesIndex] = VertCoords;
		GlobalProperties.UV[PropertiesIndex] = FVector2D(X, Y);
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
			SectionProperties.Vertices[i]	= GlobalProperties.Vertices[Index];;
			SectionProperties.Normals[i]	= GlobalProperties.Normals[Index];
			SectionProperties.UV[i]			= GlobalProperties.UV[Index];
		}
	}
}


void ATerrainGenerator::SectionRequestsUpdate(int32 SectionIndex, FVector HitLocation, ESculptMode SculptMode, float ToolStrength, float ToolRadius, bool bUseUpdateQueue)
{
	MakeCrater(SectionIndex, HitLocation, SculptMode, ToolStrength, ToolRadius, bUseUpdateQueue);
}


void ATerrainGenerator::MakeCrater(int32 SectionIndex, FVector HitLocation, ESculptMode SculptMode, float ToolStrength, float ToolRadius, bool bUseUpdateQueue)
{
	UE_LOG(LogTemp, Warning, TEXT("HELLLO"));
	TArray<int32> AffectedSections;
	FVector RelativeHitLocation = (HitLocation - GetActorLocation());
	FVector CenterCoordinates = FVector(FMath::RoundToInt(RelativeHitLocation.X / QuadSize), FMath::RoundToInt(RelativeHitLocation.Y / QuadSize), 0);
	int32 VertsPerSide = ((SectionXY - 1) * ComponentXY + 1);
	int32 CenterIndex = CenterCoordinates.X * VertsPerSide + CenterCoordinates.Y;
	FVector SectionCoordinates = FVector(SectionIndex / (ComponentXY), SectionIndex % (ComponentXY), 0);
	int32 MaxDig = 1000 * ToolStrength;
	int32 RadiusInVerts = ToolRadius / QuadSize;

	// Modify Verts around impact to make a crater
	for (int32 X = -RadiusInVerts; X <= RadiusInVerts; X++)
	{
		for (int32 Y = -RadiusInVerts; Y <= RadiusInVerts; Y++)
		{
			// Continue loop if Vert doesn't exist
			int32 CurrentIndex = CenterIndex + (X * VertsPerSide) + Y;
			if (!GlobalProperties.Vertices.IsValidIndex(CurrentIndex)) { continue; }
			// Continue if not in radius
			FVector CurrentVertCoords = FVector(FMath::RoundToInt(GlobalProperties.Vertices[CurrentIndex].X / QuadSize), FMath::RoundToInt(GlobalProperties.Vertices[CurrentIndex].Y / QuadSize), 0);
			float DistanceFromCenter = FVector::Dist(CenterCoordinates, CurrentVertCoords);
			if (DistanceFromCenter > RadiusInVerts) { continue; }

			// update Vertex location and normal
			float DigFalloff = DistanceFromCenter / RadiusInVerts;
			float Test = FMath::Lerp(MaxDig, 0, DigFalloff);
			GlobalProperties.Vertices[CurrentIndex] -= FVector(0, 0, Test);
			GlobalProperties.Normals[CurrentIndex] = FVector(0, 0.5, 0.5); // Temporary

			FVector SectionVertCoords = CurrentVertCoords - (SectionCoordinates * SectionXY - SectionCoordinates);
			int32 SectionVertIndex = SectionVertCoords.X * SectionXY + SectionVertCoords.Y;

			if (SectionVertCoords.X > SectionXY - 1 || SectionVertCoords.X < 0 || SectionVertCoords.Y > SectionXY - 1 || SectionVertCoords.Y < 0) { continue; }
			if (!SectionProperties.PositionInsideSection.IsValidIndex(SectionVertIndex)) { continue; }

			int32 NeighbourSection;
			switch (SectionProperties.PositionInsideSection[SectionVertIndex])
			{
			case EVertPositionInsideSection::SB_NotOnBorder:
			{
				if (!AffectedSections.Contains(SectionIndex)) { AffectedSections.Add(SectionIndex); }
				break;
			}
			case EVertPositionInsideSection::SB_BorderRight:
			{
				NeighbourSection = SectionIndex + 1;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				break;
			}
			case EVertPositionInsideSection::SB_BorderLeft:
			{
				NeighbourSection = SectionIndex - 1;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				break;
			}
			case EVertPositionInsideSection::SB_BorderTop:
			{
				NeighbourSection = SectionIndex + ComponentXY;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				break;
			}
			case EVertPositionInsideSection::SB_BorderBottom:
			{
				NeighbourSection = SectionIndex - ComponentXY;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				break;
			}
			case EVertPositionInsideSection::SB_EdgeBottomLeft:
			{
				NeighbourSection = SectionIndex - 1;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				NeighbourSection = SectionIndex - ComponentXY - 1;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				NeighbourSection = SectionIndex - ComponentXY;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				break;
			}
			case EVertPositionInsideSection::SB_EdgeBottomRight:
			{
				NeighbourSection = SectionIndex + 1;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				NeighbourSection = SectionIndex - ComponentXY + 1;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				NeighbourSection = SectionIndex - ComponentXY;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				break;
			}
			case EVertPositionInsideSection::SB_EdgeTopLeft:
			{
				NeighbourSection = SectionIndex - 1;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				NeighbourSection = SectionIndex + ComponentXY - 1;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				NeighbourSection = SectionIndex + ComponentXY;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				break;
			}
			case EVertPositionInsideSection::SB_EdgeTopRight:
			{
				NeighbourSection = SectionIndex + 1;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				NeighbourSection = SectionIndex + ComponentXY + 1;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				NeighbourSection = SectionIndex + ComponentXY;
				if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
				break;
			}
			}
		}
	}
	for (int32 Iter : AffectedSections)
	{
		if (!SectionActors.IsValidIndex(Iter)) { continue; }

		if (bUseUpdateQueue)
		{
			if (!SectionUpdateQueue.Contains(Iter)) { SectionUpdateQueue.Add(Iter); }
		}
		else
		{
			FillSectionVertStruct(Iter);
			SectionActors[Iter]->UpdateSection();
		}
		//UE_LOG(LogTemp, Warning, TEXT("SECTION TO UPDATE: %i"), Iter);
	}
}


void ATerrainGenerator::SectionUpdateFinished()
{
	bAllowedToUpdateSection = true;
	if (!SectionUpdateQueue.IsValidIndex(0)) { return; }
	SectionUpdateQueue.RemoveAt(0);
}
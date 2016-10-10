// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#include "RuntimeMeshTerrain.h"
#include "RuntimeMeshComponent.h" 
#include "RuntimeMeshLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetProceduralMeshLibrary.h"
#include "SimplexNoiseBPLibrary.h"
#include "TerrainSection.h"
#include "TerrainGenerator.h"


ATerrainGenerator::ATerrainGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	RuntimeMeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponent"));
	RootComponent = RuntimeMeshComponent;

	LODProperties.Add(&SectionProperties);
	LODProperties.Add(&SectionPropertiesLOD1);
	LODProperties.Add(&SectionPropertiesLOD2);
	LODProperties.Add(&SectionPropertiesLOD3);
	LODProperties.Add(&SectionPropertiesLOD4);
}


void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();
	USimplexNoiseBPLibrary::setNoiseSeed(134250);
	bUseTimerforGeneration ? GenerateMeshTimed() : GenerateMesh();
}


void ATerrainGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// check if any sections request to be updated via queue
	if (SectionUpdateQueue.Num() > 0 && bAllowedToUpdateSection)
	{
		bAllowedToUpdateSection = false;
		if (!SectionActors.IsValidIndex(SectionUpdateQueue[0])) { return; }
		SectionActors[SectionUpdateQueue[0]]->UpdateSection();
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

	// Init SectionPropertiesLOD1
	int32 SectionXYLOD1 = ((SectionXY - 1) / FactorLOD1) + 1;
	int32 LOD1NumVerts = SectionXYLOD1 * SectionXYLOD1;
	
	SectionPropertiesLOD1.Vertices.SetNum(LOD1NumVerts, true);
	SectionPropertiesLOD1.UV.SetNum(LOD1NumVerts, true);
	SectionPropertiesLOD1.Normals.SetNum(LOD1NumVerts, true);
	URuntimeMeshLibrary::CreateGridMeshTriangles(SectionXYLOD1, SectionXYLOD1, false, OUT SectionPropertiesLOD1.Triangles);

	
	// Init SectionPropertiesLOD2
	int32 SectionXYLOD2 = ((SectionXY - 1) / FactorLOD2) + 1;
	int32 LOD2NumVerts = SectionXYLOD2 * SectionXYLOD2;

	SectionPropertiesLOD2.Vertices.SetNum(LOD2NumVerts, true);
	SectionPropertiesLOD2.UV.SetNum(LOD2NumVerts, true);
	SectionPropertiesLOD2.Normals.SetNum(LOD2NumVerts, true);
	URuntimeMeshLibrary::CreateGridMeshTriangles(SectionXYLOD2, SectionXYLOD2, false, OUT SectionPropertiesLOD2.Triangles);

	// Init SectionPropertiesLOD3
	int32 SectionXYLOD3 = ((SectionXY - 1) / FactorLOD3) + 1;
	int32 LOD3NumVerts = SectionXYLOD3 * SectionXYLOD3;

	SectionPropertiesLOD3.Vertices.SetNum(LOD3NumVerts, true);
	SectionPropertiesLOD3.UV.SetNum(LOD3NumVerts, true);
	SectionPropertiesLOD3.Normals.SetNum(LOD3NumVerts, true);
	URuntimeMeshLibrary::CreateGridMeshTriangles(SectionXYLOD3, SectionXYLOD3, false, OUT SectionPropertiesLOD3.Triangles);

	// Init SectionPropertiesLOD4
	int32 SectionXYLOD4 = ((SectionXY - 1) / FactorLOD4) + 1;
	int32 LOD4NumVerts = SectionXYLOD4 * SectionXYLOD4;

	SectionPropertiesLOD4.Vertices.SetNum(LOD4NumVerts, true);
	SectionPropertiesLOD4.UV.SetNum(LOD4NumVerts, true);
	SectionPropertiesLOD4.Normals.SetNum(LOD4NumVerts, true);
	URuntimeMeshLibrary::CreateGridMeshTriangles(SectionXYLOD4, SectionXYLOD4, false, OUT SectionPropertiesLOD4.Triangles);


	UE_LOG(LogTemp, Warning, TEXT("Triangles - LOD0: %i, LOD1: %i, LOD2: %i"), SectionProperties.Triangles.Num(), SectionPropertiesLOD1.Triangles.Num(), SectionPropertiesLOD2.Triangles.Num());
	UE_LOG(LogTemp, Warning, TEXT("Vertices  - LOD0: %i, LOD1: %i, LOD2: %i"), SectionProperties.Vertices.Num(), SectionPropertiesLOD1.Vertices.Num(), SectionPropertiesLOD2.Vertices.Num());
	UE_LOG(LogTemp, Warning, TEXT("%i  %i  %i  %i  %i  %i"),
		SectionProperties.Triangles.Num(),
		SectionProperties.Vertices.Num(),
		SectionProperties.VertexColors.Num(),
		SectionProperties.UV.Num(),
		SectionProperties.Normals.Num(),
		SectionProperties.SectionPosition.Num());
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
	//FillSectionVertStruct(SectionIndexIter);
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
			//Noise Test
			float Z = USimplexNoiseBPLibrary::SimplexNoise2D(X / NoiseDensity, Y / NoiseDensity);
			FVector VertCoordsNoise = FVector(X*QuadSize, Y*QuadSize, Z * NoiseMultiplier);

			// Set Vertex, UV, Normal
			FVector VertCoords = FVector(X, Y, 0) * QuadSize;
			//CopyLandscapeHeightBelow(OUT VertCoords, OUT GlobalVertexData[i].Normals);
			GlobalVertexData[i].Vertices = VertCoordsNoise;
			GlobalVertexData[i].UV = FVector2D(X, Y);
			i++;
		}
	}
	FillGlobalNormals();
}


void ATerrainGenerator::FillGlobalNormals()
{
	for (int32 i = 0; i < GlobalVertexData.Num(); i++)
	{
		GlobalVertexData[i].Normals = CalculateVertexNormal(i);
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

		//Noise Test
		float Z = USimplexNoiseBPLibrary::SimplexNoise2D(X / NoiseDensity, Y / NoiseDensity);
		FVector VertCoordsNoise = FVector(X*QuadSize, Y*QuadSize, Z * NoiseMultiplier);

		// Set Vertex, UV, Normal
		FVector VertCoords = FVector(X, Y, 0) * QuadSize;
		//CopyLandscapeHeightBelow(OUT VertCoords, OUT GlobalVertexData[PropertiesIndex].Normals);
		GlobalVertexData[PropertiesIndex].Vertices	= VertCoordsNoise;
		GlobalVertexData[PropertiesIndex].UV		= FVector2D(X, Y);
	}

	UE_LOG(LogTemp, Warning, TEXT("GlobalPropertiesPercentFilled: %f % "), ((float)GlobalXIter/ (float)MeshVertsPerSide) * 100.f);

	// Recursive function call with a timer to prevent freezing of the gamethread
	GlobalXIter++;
	if (GlobalXIter + 1 > MeshVertsPerSide) { FillGlobalNormals(); FillIndexBufferTimed(); return; }
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
	Normal = FVector(0, 0, 1);//(Hit.bBlockingHit) ? Normal = Hit.Normal : Normal = FVector(0, 0, 1);
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


void ATerrainGenerator::FillSectionVertStructLOD(int32 SectionIndex)
{
	int32 IndexStart = SectionXY * SectionXY * SectionIndex;
	int32 IndexEnd = IndexStart + (SectionXY * SectionXY);
	int32 LOD1Vertices = (SectionXY * SectionXY) / 2;
	int32 L1 = 0;
	int32 L2 = 0;
	int32 L3 = 0;
	int32 L4 = 0;


	for (int i = 0; i + IndexStart < IndexEnd; i++)
	{
		if (!SectionProperties.Vertices.IsValidIndex(i)) { return; }
		if (!IndexBuffer.IsValidIndex(i + IndexStart)) { return; }

		int32 Index = IndexBuffer[i + IndexStart];
		FVector Vertex	= GlobalVertexData[Index].Vertices;
		FVector Normal	= GlobalVertexData[Index].Normals;
		FVector2D UV	= GlobalVertexData[Index].UV;

		// LOD0
		SectionProperties.Vertices[i]	= Vertex;
		SectionProperties.Normals[i]	= Normal;
		SectionProperties.UV[i]			= UV;

		// LOD01
		if (i % FactorLOD1 == 0 && (i / SectionXY) % FactorLOD1 == 0)
		{
			if (!SectionPropertiesLOD1.Vertices.IsValidIndex(L1)) { continue; }
			SectionPropertiesLOD1.Vertices[L1]	= Vertex;
			SectionPropertiesLOD1.Normals[L1]	= Normal;
			SectionPropertiesLOD1.UV[L1]		= UV;
			L1++;
		}
		
		// LOD02
		if (i % FactorLOD2 == 0 && (i / SectionXY) % FactorLOD2 == 0)
		{
			if (!SectionPropertiesLOD2.Vertices.IsValidIndex(L2)) { continue; }
			SectionPropertiesLOD2.Vertices[L2]	= Vertex;
			SectionPropertiesLOD2.Normals[L2]	= Normal;
			SectionPropertiesLOD2.UV[L2]		= UV;
			L2++;
		}

		// LOD03
		if (i % FactorLOD3 == 0 && (i / SectionXY) % FactorLOD3 == 0)
		{
			if (!SectionPropertiesLOD3.Vertices.IsValidIndex(L3)) { continue; }
			SectionPropertiesLOD3.Vertices[L3] = Vertex;
			SectionPropertiesLOD3.Normals[L3] = Normal;
			SectionPropertiesLOD3.UV[L3] = UV;
			L3++;
		}

		// LOD03
		if (i % FactorLOD4 == 0 && (i / SectionXY) % FactorLOD4 == 0)
		{
			if (!SectionPropertiesLOD4.Vertices.IsValidIndex(L4)) { continue; }
			SectionPropertiesLOD4.Vertices[L4] = Vertex;
			SectionPropertiesLOD4.Normals[L4] = Normal;
			SectionPropertiesLOD4.UV[L4] = UV;
			L4++;
		}
	}
}


void ATerrainGenerator::SectionRequestsUpdate(int32 SectionIndex, FSculptSettings SculptSettings, FVector HitLocation, ESculptInput SculptInput, FVector StartLocation)
{
	MakeCrater(SectionIndex, SculptSettings, HitLocation, SculptInput, StartLocation);
}


void ATerrainGenerator::MakeCrater(int32 SectionIndex, FSculptSettings SculptSettings, FVector HitLocation, ESculptInput SculptInput, FVector StartLocation)
{	
	TArray<int32> AffectedSections;
	TArray<int32> AffectedVertNormals;
	FVector RelativeHitLocation = (HitLocation - GetActorLocation());
	FVector CenterCoordinates = FVector(FMath::RoundToInt(RelativeHitLocation.X / QuadSize), FMath::RoundToInt(RelativeHitLocation.Y / QuadSize), 0);
	int32 VertsPerSide = ((SectionXY - 1) * ComponentXY + 1);
	int32 CenterIndex = CenterCoordinates.X * VertsPerSide + CenterCoordinates.Y;
	FVector SectionCoordinates = FVector(SectionIndex / (ComponentXY), SectionIndex % (ComponentXY), 0);
	int32 ScaledZStrength = MaxZValueOffsetPerUpdate * SculptSettings.ToolStrength;

	// Modify Verts around given radius
	int32 RadiusInVerts = SculptSettings.SculptRadius / QuadSize;
	int32 RadiusExtended = RadiusInVerts + 1;

	for (int32 X = -RadiusExtended; X <= RadiusExtended; X++)
	{
		for (int32 Y = -RadiusExtended; Y <= RadiusExtended; Y++)
		{
			// Continue loop if Vert doesn't exist
			int32 CurrentIndex = CenterIndex + (X * VertsPerSide) + Y;
			if (!GlobalVertexData.IsValidIndex(CurrentIndex)) { continue; }

			FVector CurrentVertCoords = FVector(
				FMath::RoundToInt(GlobalVertexData[CurrentIndex].Vertices.X / QuadSize),
				FMath::RoundToInt(GlobalVertexData[CurrentIndex].Vertices.Y / QuadSize),
				0);
			float DistanceFromCenter = FVector::Dist(CenterCoordinates, CurrentVertCoords);

			// affected normals are added to array, and calculated after loop
			if (DistanceFromCenter > RadiusExtended) { CalculateVertexNormal(CurrentIndex); continue; }
			AffectedVertNormals.Add(CurrentIndex);

			// Check real radius
			if (DistanceFromCenter > RadiusInVerts) { CalculateVertexNormal(CurrentIndex); continue; }


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

			FVector SectionVertCoords = CurrentVertCoords - (SectionCoordinates * SectionXY - SectionCoordinates);
			int32 SectionVertIndex = SectionVertCoords.X * SectionXY + SectionVertCoords.Y;

			if (SectionVertCoords.X > SectionXY - 1 || SectionVertCoords.X < 0 || SectionVertCoords.Y > SectionXY - 1 || SectionVertCoords.Y < 0) { continue; }
			if (!SectionProperties.SectionPosition.IsValidIndex(SectionVertIndex)) { continue; }

			AddAffectedSections(SectionIndex, SectionVertIndex, OUT AffectedSections);
		}
	}

	// Update Normals
	for (int32 Vert : AffectedVertNormals)
	{
		GlobalVertexData[Vert].Normals = CalculateVertexNormal(Vert);
	}
	//UE_LOG(LogTemp, Error, TEXT("ExtendedRadius Num of Verts: %i"), AffectedVertNormals.Num());


	for (int32 Iter : AffectedSections)
	{
		if (!SectionActors.IsValidIndex(Iter)) { continue; }
		(SculptSettings.bUseUpdateQueue && !SectionUpdateQueue.Contains(Iter)) ? (SectionUpdateQueue.Add(Iter)) : (SectionActors[Iter]->UpdateSection());
	}
}


void ATerrainGenerator::AddAffectedSections(int32 SectionIndex, int32 VertexIndex, OUT TArray<int32> &AffectedSections)
{
	// TODO find better way to detect border vertices (modulo?) and find a way to know which sectionindex to add 
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
}


FVector ATerrainGenerator::CalculateVertexNormal(int32 VertexIndex)
{
	int32 VertsPerSide = (ComponentXY * SectionXY - (ComponentXY - 1));
	int32 U = VertexIndex + VertsPerSide;
	int32 D = VertexIndex - VertsPerSide;
	int32 R = VertexIndex + 1;
	int32 L = VertexIndex - 1;

	if (L < 0 || D < 0 || U >= GlobalVertexData.Num() || R >= GlobalVertexData.Num()) { return FVector(0, 0, 1); }

	FVector X = GlobalVertexData[U].Vertices - GlobalVertexData[D].Vertices;
	FVector Y = GlobalVertexData[R].Vertices - GlobalVertexData[L].Vertices;

	return FVector::CrossProduct(X, Y).GetSafeNormal();
}
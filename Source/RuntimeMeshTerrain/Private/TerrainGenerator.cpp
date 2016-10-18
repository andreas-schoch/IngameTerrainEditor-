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
	USimplexNoiseBPLibrary::setNoiseSeed(Seed);
	
	float SectionLength = (SectionXY - 1) * QuadSize;
	VisibilityLOD0 = SectionLength * 1.5;
	VisibilityLOD1 = SectionLength * 2.5;
	VisibilityLOD2 = SectionLength * 3.5;
	VisibilityLOD3 = SectionLength * 4.5;
	VisibilityLOD4 = SectionLength * 9.5;

	bUseTimerforGeneration ? GenerateMeshTimed() : GenerateMesh();
}


void ATerrainGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Warning, TEXT("ENDPLAY Allocated: %i"), GlobalVertexData.GetAllocatedSize()/1000000);

	for (FSectionProperties* Iter : LODProperties)
	{
		Iter->Vertices.Empty();
		Iter->UV.Empty();
		Iter->Normals.Empty();
		Iter->VertexColors.Empty();

		Iter->SectionPosition.Empty();
		Iter->Triangles.Empty();
	}
	GlobalVertexData.Empty();
	UE_LOG(LogTemp, Warning, TEXT("ENDPLAY Allocated: %i"), GlobalVertexData.GetAllocatedSize() / 1000000);

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
	SectionPropertiesLOD1.VertexColors.SetNum(LOD1NumVerts, true);
	URuntimeMeshLibrary::CreateGridMeshTriangles(SectionXYLOD1, SectionXYLOD1, false, OUT SectionPropertiesLOD1.Triangles);

	
	// Init SectionPropertiesLOD2
	int32 SectionXYLOD2 = ((SectionXY - 1) / FactorLOD2) + 1;
	int32 LOD2NumVerts = SectionXYLOD2 * SectionXYLOD2;

	SectionPropertiesLOD2.Vertices.SetNum(LOD2NumVerts, true);
	SectionPropertiesLOD2.UV.SetNum(LOD2NumVerts, true);
	SectionPropertiesLOD2.Normals.SetNum(LOD2NumVerts, true);
	SectionPropertiesLOD2.VertexColors.SetNum(LOD2NumVerts, true);
	URuntimeMeshLibrary::CreateGridMeshTriangles(SectionXYLOD2, SectionXYLOD2, false, OUT SectionPropertiesLOD2.Triangles);

	// Init SectionPropertiesLOD3
	int32 SectionXYLOD3 = ((SectionXY - 1) / FactorLOD3) + 1;
	int32 LOD3NumVerts = SectionXYLOD3 * SectionXYLOD3;

	SectionPropertiesLOD3.Vertices.SetNum(LOD3NumVerts, true);
	SectionPropertiesLOD3.UV.SetNum(LOD3NumVerts, true);
	SectionPropertiesLOD3.Normals.SetNum(LOD3NumVerts, true);
	SectionPropertiesLOD3.VertexColors.SetNum(LOD3NumVerts, true);
	URuntimeMeshLibrary::CreateGridMeshTriangles(SectionXYLOD3, SectionXYLOD3, false, OUT SectionPropertiesLOD3.Triangles);

	// Init SectionPropertiesLOD4
	int32 SectionXYLOD4 = ((SectionXY - 1) / FactorLOD4) + 1;
	int32 LOD4NumVerts = SectionXYLOD4 * SectionXYLOD4;

	SectionPropertiesLOD4.Vertices.SetNum(LOD4NumVerts, true);
	SectionPropertiesLOD4.UV.SetNum(LOD4NumVerts, true);
	SectionPropertiesLOD4.Normals.SetNum(LOD4NumVerts, true);
	SectionPropertiesLOD4.VertexColors.SetNum(LOD4NumVerts, true);
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
			//SectionProperties.VertexColors[i] = (VertPositionInsideSection == ESectionPosition::SB_NotOnBorder) ? (FColor(255, 255, 255, 0.0)) : (FColor(255, 0, 0, 1));
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
			FVector VertCoords;
			FVector Normal = FVector(0, 0, 1);
			switch (TerrainZ)
			{
			case ETerrainGeneration::TG_Flat:
				VertCoords = FVector(X * QuadSize, Y * QuadSize, 0);
				Normal = FVector(0, 0, 1);
				break;

			case ETerrainGeneration::TG_Noise:
				VertCoords = FVector(X * QuadSize, Y * QuadSize, GetHeightByNoise(X, Y));
				Normal = CalculateVertexNormalByNoise(VertCoords / QuadSize);
				break;

			case ETerrainGeneration::TG_LineTrace:
				VertCoords = FVector(X * QuadSize, Y * QuadSize, 0);
				CopyLandscapeHeightBelow(OUT VertCoords, OUT Normal);
				break;
			}

			// Set Vertex, UV, Normal
			GlobalVertexData[i].Vertices = VertCoords;
			GlobalVertexData[i].UV = FVector2D(X, Y);
			GlobalVertexData[i].Normals = Normal;
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

		float QuadCorrection = QuadSize / 100;
		//Noise Test
		float Noise;
		float Octave1 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(X * Scale1 * QuadCorrection, Y * Scale1 * QuadCorrection, 0, NoiseMultiplier1);
		float Octave2 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(X * Scale2 * QuadCorrection, Y * Scale2 * QuadCorrection, 0, NoiseMultiplier2);
		float Octave3 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(X * Scale3 * QuadCorrection, Y * Scale3 * QuadCorrection, 0, NoiseMultiplier3);
		float Octave4 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(X * Scale4 * QuadCorrection, Y * Scale4 * QuadCorrection, 0, NoiseMultiplier4);
	
		Noise = Octave1;
		Noise += bUseOctave2 ? Octave2 : 0;
		Noise += bUseOctave3 ? Octave3 : 0;
		Noise += bUseOctave4 ? Octave4 : 0;

		FVector VertCoordsNoise = FVector(X*QuadSize, Y*QuadSize, Noise);

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
	GetWorld()->GetTimerManager().SetTimer(SectionCreateTimerHandle, this, &ATerrainGenerator::FillGlobalPropertiesTimed, FillVertexDataTimerDelay,false);
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
			SectionProperties.Vertices[i]		= GlobalVertexData[Index].Vertices;
			SectionProperties.Normals[i]		= GlobalVertexData[Index].Normals;
			SectionProperties.UV[i]				= GlobalVertexData[Index].UV;
			SectionProperties.VertexColors[i]	= GlobalVertexData[Index].VertexColor;
		}
	}
}


void ATerrainGenerator::FillSectionVertStructLOD(int32 SectionIndex)
{
	int32 IndexStart = SectionXY * SectionXY * SectionIndex;
	int32 IndexEnd = IndexStart + (SectionXY * SectionXY);

	int32 L1 = 0;
	int32 L2 = 0;
	int32 L3 = 0;
	int32 L4 = 0;

	for (int i = 0; i + IndexStart < IndexEnd; i++)
	{
		if (!SectionProperties.Vertices.IsValidIndex(i)) { return; }
		if (!IndexBuffer.IsValidIndex(i + IndexStart)) { return; }

		int32 Index = IndexBuffer[i + IndexStart];

		FVertexData IterVertexData;
		IterVertexData.Vertices		= GlobalVertexData[Index].Vertices;
		IterVertexData.Normals		= GlobalVertexData[Index].Normals;
		IterVertexData.UV			= GlobalVertexData[Index].UV;
		IterVertexData.VertexColor	= GlobalVertexData[Index].VertexColor;


		// Set Vertex Data of all LODs
		SetLODVertexData(0, i, i, 1, IterVertexData);
		if (SetLODVertexData(1, i, L1, FactorLOD1, IterVertexData)) { L1++; }
		if (SetLODVertexData(2, i, L2, FactorLOD2, IterVertexData)) { L2++; }
		if (SetLODVertexData(3, i, L3, FactorLOD3, IterVertexData)) { L3++; }
		if (SetLODVertexData(4, i, L4, FactorLOD4, IterVertexData)) { L4++; }
	}
}


bool ATerrainGenerator::SetLODVertexData(int32 LOD, int32 LoopIter, int32 Index, int32 DivideFactor, FVertexData VertexData)
{
	if (LoopIter % DivideFactor == 0 && (LoopIter / SectionXY) % DivideFactor == 0)
	{
		if (!LODProperties[LOD]->Vertices.IsValidIndex(Index)) { return false; }
		LODProperties[LOD]->Vertices[Index]		= VertexData.Vertices;
		LODProperties[LOD]->Normals[Index]		= VertexData.Normals;
		LODProperties[LOD]->UV[Index]			= VertexData.UV;
		LODProperties[LOD]->VertexColors[Index]	= VertexData.VertexColor;

		return true;
	}
	return false;
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
				float ZValue = 0;

				switch (SculptSettings.SculptMode) // TODO maybe change switch statement to ifs, or add a additive/subtractive bool or enum as input parameter 
				{
				case ESculptMode::ST_Lower:
				{
					Alpha = Curve->GetFloatValue(DistanceFraction) * SculptSettings.Falloff;
					ZValue = FMath::Lerp(ScaledZStrength, 0, Alpha);
					GlobalVertexData[CurrentIndex].Vertices -= FVector(0, 0, ZValue);
					break;
				}

				case ESculptMode::ST_Raise:
				{
					Alpha = Curve->GetFloatValue(DistanceFraction) * SculptSettings.Falloff;
					ZValue = FMath::Lerp(ScaledZStrength, 0, Alpha);
					GlobalVertexData[CurrentIndex].Vertices += FVector(0, 0, ZValue);
					break;
				}

				case ESculptMode::ST_Flatten: // TODO fix center index issue
				{
					Alpha = Curve->GetFloatValue(DistanceFraction) * SculptSettings.Falloff;
					ZValue = FMath::Lerp(StartLocation.Z, GlobalVertexData[CurrentIndex].Vertices.Z, Alpha);
					float ZValue2 = FMath::Lerp(GlobalVertexData[CurrentIndex].Vertices.Z, ZValue, SculptSettings.ToolStrength);
					FVector Flatted = FVector(GlobalVertexData[CurrentIndex].Vertices.X, GlobalVertexData[CurrentIndex].Vertices.Y, ZValue2);
					GlobalVertexData[CurrentIndex].Vertices = Flatted;
					break;
				}

				case ESculptMode::ST_PaintAdditive:
				{
					Alpha = Curve->GetFloatValue(DistanceFraction) * SculptSettings.Falloff;
					uint8 R = FMath::Lerp(SculptSettings.Color.R, (uint8)0, Alpha);
					uint8 G = FMath::Lerp(SculptSettings.Color.G, (uint8)0, Alpha);
					uint8 B = FMath::Lerp(SculptSettings.Color.B, (uint8)0, Alpha);
					uint8 A = FMath::Lerp(SculptSettings.Color.A, (uint8)0, Alpha);

					FColor ColorWithFalloff = FColor(R, G, B, A);

					GlobalVertexData[CurrentIndex].VertexColor = CombineColorsClamped(GlobalVertexData[CurrentIndex].VertexColor, ColorWithFalloff);
					break;
				}
				case ESculptMode::ST_PaintSubtractive:
				{
					Alpha = Curve->GetFloatValue(DistanceFraction) * SculptSettings.Falloff;
					uint8 R = FMath::Lerp(SculptSettings.Color.R, (uint8)0, Alpha);
					uint8 G = FMath::Lerp(SculptSettings.Color.G, (uint8)0, Alpha);
					uint8 B = FMath::Lerp(SculptSettings.Color.B, (uint8)0, Alpha);
					uint8 A = FMath::Lerp(SculptSettings.Color.A, (uint8)0, Alpha);

					FColor ColorWithFalloff = FColor(R, G, B, A);

					GlobalVertexData[CurrentIndex].VertexColor = ColorSubtract(GlobalVertexData[CurrentIndex].VertexColor, ColorWithFalloff);
					break;
				}

				case ESculptMode::ST_NoiseAdditive:
				{
					float QuadCorrection = QuadSize / 100;
					float Noise = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(CurrentVertCoords.X * Scale4 * QuadCorrection, CurrentVertCoords.Y * Scale4 * QuadCorrection, 0, NoiseMultiplier4);

					Alpha = Curve->GetFloatValue(DistanceFraction) * SculptSettings.Falloff;
					ZValue = FMath::Lerp((int32)Noise, 0, Alpha);

					GlobalVertexData[CurrentIndex].Vertices += FVector(0, 0, ZValue);
					break;
				}

				case ESculptMode::ST_NoiseSubtractive:
				{
					float QuadCorrection = QuadSize / 100;
					float Noise = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(CurrentVertCoords.X * Scale4 * QuadCorrection, CurrentVertCoords.Y * Scale4 * QuadCorrection, 0, NoiseMultiplier4);

					Alpha = Curve->GetFloatValue(DistanceFraction) * SculptSettings.Falloff;
					ZValue = FMath::Lerp((int32)Noise, 0, Alpha);

					GlobalVertexData[CurrentIndex].Vertices -= FVector(0, 0, ZValue);
					break;
				}

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

	if (L < 0 || D < 0 || U >= GlobalVertexData.Num() || R >= GlobalVertexData.Num()) { return FVector(0, 0, 0); }

	FVector X = GlobalVertexData[U].Vertices - GlobalVertexData[D].Vertices;
	FVector Y = GlobalVertexData[R].Vertices - GlobalVertexData[L].Vertices;

	return FVector::CrossProduct(X, Y).GetSafeNormal();
}


FVector ATerrainGenerator::CalculateVertexNormalByNoise(FVector Coordinates)
{
	FVector ULoc = FVector((Coordinates.X + 1) * QuadSize, Coordinates.Y * QuadSize, GetHeightByNoise(Coordinates.X + 1, Coordinates.Y));
	FVector DLoc = FVector((Coordinates.X - 1) * QuadSize, Coordinates.Y * QuadSize, GetHeightByNoise(Coordinates.X - 1, Coordinates.Y));
	FVector RLoc = FVector(Coordinates.X * QuadSize, (Coordinates.Y + 1) * QuadSize, GetHeightByNoise(Coordinates.X, Coordinates.Y + 1));
	FVector LLoc = FVector(Coordinates.X * QuadSize, (Coordinates.Y - 1) * QuadSize, GetHeightByNoise(Coordinates.X, Coordinates.Y - 2));


	FVector X = ULoc - DLoc;
	FVector Y = RLoc - LLoc;

	return FVector::CrossProduct(X, Y).GetSafeNormal();
}


float ATerrainGenerator::GetHeightByNoise(int32 XCoords, int32 YCoords)
{
	float QuadCorrection = QuadSize / 100;
	float Noise;

	float Octave1 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(XCoords * Scale1 * QuadCorrection, YCoords * Scale1 * QuadCorrection, 0, NoiseMultiplier1);
	float Octave2 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(XCoords * Scale2 * QuadCorrection, YCoords * Scale2 * QuadCorrection, 0, NoiseMultiplier2);
	float Octave3 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(XCoords * Scale3 * QuadCorrection, YCoords * Scale3 * QuadCorrection, 0, NoiseMultiplier3);
	float Octave4 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(XCoords * Scale4 * QuadCorrection, YCoords * Scale4 * QuadCorrection, 0, NoiseMultiplier4);

	Noise = Octave1;
	Noise += bUseOctave2 ? Octave2 : 0;
	Noise += bUseOctave3 ? Octave3 : 0;
	Noise += bUseOctave4 ? Octave4 : 0;

	return Noise;
}

FColor ATerrainGenerator::CombineColors(FColor A, FColor B)
{
	FColor C;
	C = FColor(
		FMath::Clamp(A.R + B.R, 0, 255),
		FMath::Clamp(A.G + B.G, 0, 255),
		FMath::Clamp(A.B + B.B, 0, 255),
		FMath::Clamp(A.A + B.A, 0, 255));

		return C;
}


FColor ATerrainGenerator::ColorSubtract(FColor A, FColor B)
{
	FColor C;
	C = FColor(
		FMath::Clamp(A.R - B.R, 0, 255),
		FMath::Clamp(A.G - B.G, 0, 255),
		FMath::Clamp(A.B - B.B, 0, 255),
		FMath::Clamp(A.A - B.A, 0, 255));

	return C;
}


FColor ATerrainGenerator::CombineColorsClamped(FColor A, FColor B)
{
	// find the strongest layer from B. Only the strongest will be considered for painting
	TArray<uint8> RGBA;
	RGBA.Add(B.R);
	RGBA.Add(B.G);
	RGBA.Add(B.B);
	RGBA.Add(B.A);

	uint8 Temp = 0;
	uint8 Index;
	for (int32 i = 0; i < RGBA.Num(); i++)
	{
		if (RGBA[i] > Temp)
		{
			Temp = RGBA[i];
			Index = i;
		}
	}

	// check if all layers combined reached the max value of 255
	FColor TempColor = CombineColors(A, B);
	int32 Value = TempColor.R + TempColor.G + TempColor.B + TempColor.A;
	bool bRemove = (Value > 255) ? true : false;
	int32 ToRemove = (Value -255) / 3;

	uint8 Red;
	uint8 Green;
	uint8 Blue;
	uint8 Alpha;

	// reduce paint on unused layers equally to make room for painting on used layer
	if (bRemove)
	{
		Red		= (Index == 0) ? 0 : ToRemove;
		Green	= (Index == 1) ? 0 : ToRemove;
		Blue	= (Index == 2) ? 0 : ToRemove;
		Alpha	= (Index == 3) ? 0 : ToRemove;
		TempColor = ColorSubtract(A, FColor(Red, Green, Blue, Alpha));
	}

	// Paint used layer
	Red		= (Index == 0) ? B.R : 0;
	Green	= (Index == 1) ? B.G : 0;
	Blue	= (Index == 2) ? B.B : 0;
	Alpha	= (Index == 3) ? B.A : 0;

	TempColor = bRemove ? TempColor : A;
	FColor EndResult = CombineColors(TempColor, FColor(Red, Green, Blue, Alpha));

	return EndResult;
}

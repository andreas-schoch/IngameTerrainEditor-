// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "TerrainEditorStuff.generated.h"

/*
I made this class only to store the Structs and Enums that are needed in multiple classes
I tried adding only a header and even making a static library, but failed at both.
So in the meantime, this has to do the job
*/



UENUM(BlueprintType)
enum class ESectionPosition : uint8
{
	SB_NotOnBorder		UMETA(DisplayName = "NotOnBorder"),
	SB_BorderLeft 		UMETA(DisplayName = "BorderLeft"),
	SB_BorderRight		UMETA(DisplayName = "BorderRight"),
	SB_BorderTop 		UMETA(DisplayName = "BorderTop"),
	SB_BorderBottom 	UMETA(DisplayName = "BorderBottom"),
	SB_EdgeTopLeft 		UMETA(DisplayName = "EdgeTopLeft"),
	SB_EdgeTopRight 	UMETA(DisplayName = "EdgeTopRight"),
	SB_EdgeBottomLeft 	UMETA(DisplayName = "EdgeBottomLeft"),
	SB_EdgeBottomRight 	UMETA(DisplayName = "EdgeBottomRight")
};


UENUM(BlueprintType)
enum class ESculptMode : uint8
{
	ST_Sculpt		UMETA(DisplayName = "Sculpt"),
	ST_Flatten		UMETA(DisplayName = "Flatten"),
	ST_Smooth		UMETA(DisplayName = "Smooth"),
	ST_Noise		UMETA(DisplayName = "Noise"),
	ST_Paint		UMETA(DisplayName = "Paint"),
};


UENUM(BlueprintType)
enum class ESculptInput : uint8
{
	ST_Started		UMETA(DisplayName = "Started"),
	ST_Ongoing		UMETA(DisplayName = "Ongoing"),
	ST_Stopped		UMETA(DisplayName = "Stopped"),
};


UENUM(BlueprintType)
enum class ETerrainGeneration : uint8
{
	TG_LineTrace	UMETA(DisplayName = "LineTrace"),
	TG_Flat			UMETA(DisplayName = "Flat"),
	TG_Noise		UMETA(DisplayName = "Noise"),
};


// Struct that constains global Vertex information
USTRUCT(BlueprintType)
struct FVertexData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	FVector Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	FVector2D UV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	FVector Normals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	FColor VertexColor;

	FVertexData()
	{
		VertexColor = FColor(0, 0, 0, 0);
	}
};


// Struct that constains necessary Vertex information of a single section
USTRUCT(BlueprintType)
struct FSectionProperties
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	TArray<FVector> Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	TArray<FVector2D> UV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	TArray<FVector> Normals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct") // TODO remember to store vertexcolor in global vert data
	TArray<FColor> VertexColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	TArray<int32> Triangles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struct")
	TArray<ESectionPosition> SectionPosition;

	FSectionProperties()
	{
	}
};



USTRUCT(BlueprintType)
struct FSculptSettings
{
	GENERATED_USTRUCT_BODY()

	// Select Tool
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	ESculptMode SculptMode;

	// Invert direction for some tools (raise, lower).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	bool bInvertToolDirection = false;

	// Defines affected Vertices
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	float SculptRadius = 500.f;

	// Strength of the tool. 0 = min, 1 = max. (Max Value is defined in Terrain Generator)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ToolStrength = 0.25f;

	// Distance falloff from center of sculpt location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Falloff = 1.f;

	// Using a Queue to update TerrainSection is more performant but causes gaps on borders
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	bool bUseUpdateQueue = false;

	// Vertex Color that will be painted when in paint mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	FColor Color = FColor(0, 0, 0, 255);

	// Use Max Layer strength
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	bool bUseColorTargetLayer = false;

	// Limit layer strength Min = 0, Max = 255;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting", meta = (ClampMin = "0.0", ClampMax = "255.0"))
	float ColorTargetLayerStrength = 255.0f;

	// Lower values result in more dense noise
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting", meta = (ClampMin = "1.0", ClampMax = "500.0"))
	float NoiseScale = 10.f;

	FSculptSettings()
	{
	}
};


USTRUCT(BlueprintType)
struct FSculptInputInfo
{
	GENERATED_USTRUCT_BODY()

	// was input trigger just pressed, is ongoing or released
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	ESculptInput SculptInput;

	//Sculpt location when sculpting actor just triggered the sculpt function
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	FVector StartLocation;

	// The location the sculpting actor requests to sculpt
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	FVector CurrentLocation;

	// The last location that the sculpt component send an update request
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	FVector LastLocation;


	FSculptInputInfo()
	{
		/*StartLocation = FVector(0.f, 0.f, 0.f);
		CurrentLocation = FVector(0.f, 0.f, 0.f);
		SculptInput = ESculptInput::ST_Stopped;*/
	}
};


UCLASS()
class RUNTIMEMESHTERRAIN_API ATerrainEditorStuff : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATerrainEditorStuff();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

};

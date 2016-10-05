// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "TerrainEditorStuff.generated.h"

/*
I made this class only to store the Structs and Enums that are needed in multiple classes
I tried adding only a header and even making a static library, but failed at both.
So for the meantime, this has to do the job
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
	ST_Raise		UMETA(DisplayName = "Raise"),
	ST_Lower		UMETA(DisplayName = "Lower"),
	ST_Flatten		UMETA(DisplayName = "Flatten"),
	ST_Smooth		UMETA(DisplayName = "Smooth"),
};


UENUM(BlueprintType)
enum class ESculptInput : uint8
{
	ST_Started		UMETA(DisplayName = "Started"),
	ST_Ongoing		UMETA(DisplayName = "Ongoing"),
	ST_Stopped		UMETA(DisplayName = "Stopped"),
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

	// Vertex Color will be stored here aswell, currently used to visualize section borders

	FVertexData()
	{
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

	// Defines affected Vertices
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	float SculptRadius = 500;

	// Strength of the tool. 0 = min, 1 = max. (Max Value is defined in Terrain Generator)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	float ToolStrength = 0.25;

	// Distance falloff from center of sculpt location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting") // TODO add min max
	float Falloff = 1;

	// Using a Queue to update TerrainSection is more performant but causes gaps on borders
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	bool bUseUpdateQueue = true;

	// was input trigger just pressed, is ongoing or released
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	ESculptInput SculptInput;

	FSculptSettings()
	{
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

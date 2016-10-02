// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "TerrainGenerator.h"
#include "SculptComponent.generated.h"



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RUNTIMEMESHTERRAIN_API USculptComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USculptComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	void Sculpt();



protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	ESculptMode SculptMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	float SculptRadius = 500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	float ToolStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting") // TODO add min max
	float Falloff = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	bool bUseQueueToUpdate = true;


public:
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SculptStart(UCameraComponent* Camera);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SculptStop();

	FVector2D LastSculptLocation2D;
	FTimerHandle SculptTimerHandle;
	UCameraComponent* OwnerCamera;

};

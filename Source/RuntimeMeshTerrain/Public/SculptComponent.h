// Copyright 2016 Andreas Schoch (aka Minaosis). All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "TerrainEditorStuff.h"
#include "SculptComponent.generated.h"



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RUNTIMEMESHTERRAIN_API USculptComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USculptComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	// starts sculpting action till it gets stoped by "SculptStop"
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SculptStart(UCameraComponent* Camera);

	// Stops sculpting action
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SculptStop();


protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	FSculptSettings SculptSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	ESculptInput SculptInput;

	// Switch between using camera aim, or mouse cursor as sculpt location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	bool bUseMouseMode = false;

	// Distance you have to move away from previous sculpt location to send next sculpt request
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	float SleepDistance = 300;

	// Distance from camera that you can interact (not used in mouse mode)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	float InteractionDistance = 100000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	TSubclassOf<AActor> DecalActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	FVector CurrentSculptLocation;


	AActor* DecalActorRef;


	void Sculpt();

	bool InSleepDistance(FVector CurrentLocation);

	bool GetHitResult(FHitResult &Hit);

	FVector LastLocation;
	FTimerHandle SculptTimerHandle;
	UCameraComponent* OwnerCamera;

	FVector StartLocation;
};

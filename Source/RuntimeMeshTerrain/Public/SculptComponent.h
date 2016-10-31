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

	// starts sculpting action till it gets stopped by "SculptStop"
	UFUNCTION(BlueprintCallable, Category = "Sculpting")
	void SculptStart();

	// Stops sculpting action
	UFUNCTION(BlueprintCallable, Category = "Sculpting")
	void SculptStop();

	// Use this if you want to implement sculpt behaviour yourself.
	UFUNCTION(BlueprintCallable, Category = "Sculpting")
	bool SculptSingle(FSculptInputInfo InputInfo);

	// Has to be called from owner class
	UFUNCTION(BlueprintCallable, Category = "Sculpting")
	void SetHitResult(FHitResult HitResult) { HitResultOwner = HitResult; }

	FHitResult HitResultOwner;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	FSculptSettings SculptSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sculpting")
	FSculptInputInfo InputInfo;

	// Distance you have to move away from previous sculpt location to send next sculpt request
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	float SleepDistance = 300;

	void Sculpt();

	bool InSleepDistance();

	FTimerHandle SculptTimerHandle;
};

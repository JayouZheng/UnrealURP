// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnrealURPStruct.h"
#include "CustomMeshPassBase.generated.h"

UCLASS(NotBlueprintable, abstract)
class UNREALURP_API UCustomMeshPassBaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCustomMeshPassBaseComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

public:

	UPROPERTY(Transient)
	bool bDeferredRegistered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	FGraphicPassParameters GraphicPassParameters;
};

// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnrealURPStruct.h"
#include "ProgrammableRenderingPipelineBase.generated.h"

UCLASS(Blueprintable, Abstract, ClassGroup = UnrealURP)
class UNREALURP_API AProgrammableRenderingPipelineBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProgrammableRenderingPipelineBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Render"))
	void Render(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void AddPass();
};

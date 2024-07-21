// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnrealURPStruct.h"
#include "ComputeShaderPass.generated.h"

UCLASS(NotBlueprintable, meta = (BlueprintSpawnableComponent))
class UNREALURP_API UComputeShaderPassComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UComputeShaderPassComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ComputeShaderPassSetup)
	FComputePassParameters ComputePassParameters;
};

UCLASS(NotBlueprintable)
class UNREALURP_API AComputeShaderPass : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AComputeShaderPass();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UComputeShaderPassComponent> ComputeShaderPassComponent;
};
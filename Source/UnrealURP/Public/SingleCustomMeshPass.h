// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomMeshPassBase.h"
#include "UnrealURPStruct.h"
#include "SingleCustomMeshPass.generated.h"

UCLASS(NotBlueprintable, meta = (BlueprintSpawnableComponent))
class UNREALURP_API USingleCustomMeshPassComponent final : public UCustomMeshPassBaseComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USingleCustomMeshPassComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual bool ShouldCreateRenderState() const override { return true; }

	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void DestroyRenderState_Concurrent() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
};

UCLASS(NotBlueprintable)
class UNREALURP_API ASingleCustomMeshPass : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASingleCustomMeshPass();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USingleCustomMeshPassComponent> SingleCustomMeshPassComponent;
};

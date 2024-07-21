// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "UnrealURPStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "UnrealURPManager.generated.h"

#define CPD_MARK_VALUE_BIT_NUM 24

class FUnrealURPRenderer;
class UCustomMeshPassBaseComponent;
class UPrimitiveComponent;
class ACameraActor;
struct FPassContext;

UCLASS(NotBlueprintable)
class UNREALURP_API UUnrealURPManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	UUnrealURPManager();
	
	// FTickableGameObject implementation Begin
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	// FTickableGameObject implementation End

	// UWorldSubsystem implementation Begin
	/** Override to support water subsystems in editor preview worlds */
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	// UWorldSubsystem implementation End

	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// USubsystem implementation End

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void AddGameObjectToPass(const FRenderableGameObject& InGameObject, FName InPassName);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void AddGameObjectsToPass(const TArray<FRenderableGameObject>& InGameObjects, FName InPassName);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void RemoveGameObjectFromPass(const FRenderableGameObject& InGameObject, FName InPassName);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void RemoveGameObjectsFromPass(const TArray<FRenderableGameObject>& InGameObjects, FName InPassName);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void AddRenderableComponentToPass(const FRenderableComponent& InComp, FName InPassName);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void AddRenderableComponentsToPass(const TArray<FRenderableComponent>& InComps, FName InPassName);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void RemoveRenderableComponentFromPass(const FRenderableComponent& InComp, FName InPassName);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void RemoveRenderableComponentsFromPass(const TArray<FRenderableComponent>& InComps, FName InPassName);

	// TODO: Deprecated
	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void AddPrimitiveComponentToPass(UPrimitiveComponent* InPrimComps, FName InPassName);

	// TODO: Deprecated
	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void AddPrimitiveComponentsToPass(const TArray<UPrimitiveComponent*>& InPrimComps, FName InPassName);

	// TODO: Deprecated
	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void RemovePrimitiveComponentFromPass(UPrimitiveComponent* InPrimComp, FName InPassName);

	// TODO: Deprecated
	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void RemovePrimitiveComponentsFromPass(const TArray<UPrimitiveComponent*>& InPrimComps, FName InPassName);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void SuspendPass(FName InPassName);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void ResumePass(FName InPassName);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void UpdatePassPriority(FName InPassName, int32 InPassPriority);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void UpdatePassCamera(FName InPassName, ACameraActor* InCamera = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Rendering|UnrealURP")
	void ResizeRenderTarget2D(FName InPassName, int32 InSlotIndex, int32 InSizeX, int32 InSizeY);

	static UUnrealURPManager* GetUnrealURPManager(const UWorld* InWorld);

private:

	friend class USingleCustomMeshPassComponent;
	friend class UMultiCustomMeshPassComponent;

	void RegisterCustomMeshPass(UCustomMeshPassBaseComponent* InPassComp);
	void UnRegisterCustomMeshPass(UCustomMeshPassBaseComponent* InPassComp);

	void AddPassContext(FPassContext* InPassContext);
	void RemovePassContext(const FName& InPassName);

	void DeferredRegisterCustomMeshPass(UCustomMeshPassBaseComponent* InPassComp);

private:

	TSharedPtr<FUnrealURPRenderer, ESPMode::ThreadSafe> UnrealURPRenderer;

	struct FCPDMarkValueTable
	{
		FName PassName;
		uint32 ReferenceCount = 0;
	};
	FCPDMarkValueTable CPDMarkValueTable[CPD_MARK_VALUE_BIT_NUM];

	UPROPERTY()
	TArray<TWeakObjectPtr<UCustomMeshPassBaseComponent>> CustomMeshPassComponents;
};

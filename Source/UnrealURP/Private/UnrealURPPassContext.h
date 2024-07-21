// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnrealURPStruct.h"
#include "RHIDefinitions.h"

#define MAX_RT MaxSimultaneousRenderTargets
#define MAX_RT_INDEX (MAX_RT - 1)

class USingleCustomMeshPass;
class FTextureRenderTargetResource;
class FRHIDepthStencilState;
class FRHIBlendState;
class UStaticMeshComponent;
class FPrimitiveSceneProxy;

struct FPassContext
{
	static FPassContext* Create(const FGraphicPassParameters& InPassParameters);

	FPassContext();

	FRHIDepthStencilState* GetDepthStencilStateRHI();
	FRHIDepthStencilState* GetResetStencilStateRHI();
	FRHIBlendState* GetBlendStateRHI();

	bool IsSceneViewRelevanceRenderTarget() const;
	bool IsSafeToRender() const;
	bool IsOutputDepthTarget() const;
	bool IsUseNaniteFallback() const;

	inline bool operator==(const FPassContext& Other) const
	{
		return PassName == Other.PassName;
	}

	void AdvanceFrame(EPassInjectionPoint InPassInjectionPoint, TFunctionRef<void()> Lambda);

	struct FRenderableObject
	{
		FPrimitiveComponentId RenderObjectID;
		FMultiPassFlags MultiPassFlags;
		int32 MaxDrawInfo = 0;

		FRenderableObject() {}
		FRenderableObject(const FPrimitiveComponentId& InCompID)
			: RenderObjectID(InCompID)
		{}

		inline bool operator==(const FRenderableObject& Other) const
		{
			return RenderObjectID == Other.RenderObjectID;
		}
	};

	struct FNaniteFallbackRenderInfo
	{
		int32 LODIndex = -1;
		int32 NumSections = -1;
		int32 PrimitiveIndex = -1;

		FPrimitiveSceneProxy* SourceProxy = nullptr;

		FPrimitiveComponentId RenderObjectID;
		FMultiPassFlags MultiPassFlags;

		FNaniteFallbackRenderInfo() {}
		FNaniteFallbackRenderInfo(const FPrimitiveComponentId& InCompID)
			: RenderObjectID(InCompID)
		{}

		inline bool operator==(const FNaniteFallbackRenderInfo& Other) const
		{
			return RenderObjectID == Other.RenderObjectID;
		}
	};

	struct FRenderTargetInternal
	{
		FLinearColor ClearColor = FLinearColor(ForceInit);
		FTextureRenderTargetResource* RenderTarget = nullptr;

		inline bool operator==(const FRenderTargetInternal& Other) const
		{
			return RenderTarget == Other.RenderTarget;
		}

		inline bool IsValid() const
		{
			return RenderTarget != nullptr;
		}

		inline float GetDepthClearValue() const
		{
			return ClearColor.R;
		}

		inline uint8 GetStencilClearValue() const
		{
			return (uint8)FMath::Clamp(ClearColor.G, 0.0f, 255.0f);
		}
	};

	uint8 bSuspend : 1;

	uint8 bUseNaniteFallback : 1;
	uint8 bUseManualCulling : 1;
	uint8 bEnableMultiPassSupport : 1;
	uint8 bUseCustomViewProj : 1;
	uint8 bUseDepthStencilOnly : 1;
	uint8 bUseRenderTargetAssets : 1;
	uint8 bUseScreenViewportSize : 1;

	uint8 bUseDepthStencilTargetAsset : 1;

	uint8 bResetStencilAfterUse : 1;

	EPassType PassType = EPassType::Graphics;
	EFrameRate FrameRate = EFrameRate::Full;
	EBuiltInDepthStencilTarget BuiltInDepthStencilTarget = EBuiltInDepthStencilTarget::None;
	EPassInjectionPoint InjectionPoint = EPassInjectionPoint::None;

	int32 PassPriority = -1;
	int32 NaniteCPDMarkIndex = -1;
	int32 NaniteCPDMarkValue = -1;
	int32 StencilReference = -1;

	int32 FrameCount = 0;
	int32 ActiveFrameCount = 0;

	uint32 NumOutRenderTarget = 0;
	
	float MaxDrawDistanceScale = 0.0f;

	FMatrix CustomViewProj = FMatrix::Identity;

	FRenderTargetInternal DepthStencilTarget;

	FMaterialRenderProxy* MaterialProxy = nullptr;

	FPipelineState PipelineState;

	FName PassName;

	TArray<EBuiltInRenderTarget> BuiltInRenderTargetTypes;
	TArray<FRenderableObject> RenderObjects;
	TArray<FNaniteFallbackRenderInfo> NaniteFallbackRenderInfos;
	TArray<UStaticMeshComponent*> NaniteFallbackStaticMeshComps; // Empty right after use.

	TArray<FRenderTargetInternal> RenderTargets;

	FDepthStencilStateRHIRef DepthStencilStateRHIRef;
	FDepthStencilStateRHIRef StencilClearStateRHIRef;
	FBlendStateRHIRef BlendStateRHIRef;
};

struct FGraphicPassContext : public FPassContext
{

};

struct FComputePassContext : public FPassContext
{

};

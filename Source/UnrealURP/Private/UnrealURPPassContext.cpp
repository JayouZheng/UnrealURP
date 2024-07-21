// Copyright Jayou, Inc. All Rights Reserved.

#include "UnrealURPPassContext.h"
#include "SingleCustomMeshPass.h"
#include "Engine/StaticMeshActor.h"
#include "Animation/SkeletalMeshActor.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TextureResource.h"
#include "UnrealURPRenderTarget.h"
#include "RHIResources.h"
#include "RHI.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"

FPassContext* FPassContext::Create(const FGraphicPassParameters& InPassParameters)
{
	if (InPassParameters.PassName == FName(TEXT("None")) || InPassParameters.PassPriority < 0)
		return nullptr;

	FPassContext* Context = new FPassContext;
	Context->PassType = EPassType::Graphics;
	Context->PassPriority = InPassParameters.PassPriority;
	Context->PassName = InPassParameters.PassName;
	Context->FrameRate = InPassParameters.FrameRate;
	Context->MaterialProxy = InPassParameters.ShaderScript ? InPassParameters.ShaderScript->GetRenderProxy() : nullptr;
	Context->StencilReference = InPassParameters.PipelineState.DepthStencilState.StencilReference;
	Context->PipelineState = InPassParameters.PipelineState;
	Context->bUseNaniteFallback = InPassParameters.bUseNaniteFallback;
	Context->NaniteCPDMarkIndex = InPassParameters.NaniteCPDMarkIndex;
	Context->NaniteCPDMarkValue = InPassParameters.NaniteCPDMarkValue;
	Context->bUseManualCulling = InPassParameters.bUseManualCulling;
	Context->bEnableMultiPassSupport = InPassParameters.bEnableMultiPassSupport;
	Context->MaxDrawDistanceScale = InPassParameters.MaxDrawDistanceScale;
	Context->bUseDepthStencilOnly = InPassParameters.bUseDepthStencilOnly;
	Context->bUseRenderTargetAssets = InPassParameters.bUseRenderTargetAssets;
	Context->bUseScreenViewportSize = InPassParameters.bUseScreenViewportSize;
	Context->bUseDepthStencilTargetAsset = InPassParameters.bUseDepthStencilTargetAsset;
	Context->bResetStencilAfterUse = InPassParameters.bResetStencilAfterUse;

	if (Context->bEnableMultiPassSupport)
	{
		Context->MaterialProxy = nullptr;
	}

	if (InPassParameters.Camera)
	{
		FMinimalViewInfo ViewInfo;
		InPassParameters.Camera->GetCameraComponent()->GetCameraView(0.0f, ViewInfo);

		FMatrix View;
		FMatrix Proj;
		UGameplayStatics::GetViewProjectionMatrix(ViewInfo, View, Proj, Context->CustomViewProj);

		Context->bUseCustomViewProj = true;
	}

	if (Context->bUseRenderTargetAssets)
	{
		for (auto& RT : InPassParameters.RenderTargetAssets)
		{
			if (RT.RenderTargetAsset && RT.RenderTargetAsset->CanBeUseAsRenderTarget())
			{
				FTextureRenderTargetResource* RTResource = RT.RenderTargetAsset->GameThread_GetRenderTargetResource();
				if (RTResource != nullptr)
				{
					FRenderTargetInternal RenderTargetInternal;
					RenderTargetInternal.RenderTarget = RTResource;
					RenderTargetInternal.ClearColor = RT.ClearColor;

					Context->RenderTargets.AddUnique(RenderTargetInternal);

					if (Context->RenderTargets.Num() >= MAX_RT)
						break;
				}
			}		
		}

		Context->NumOutRenderTarget = Context->RenderTargets.Num();
	}
	else
	{
		for (auto& RT : InPassParameters.RenderTargets)
		{
			if (RT == EBuiltInRenderTarget::None)
				continue;

			Context->BuiltInRenderTargetTypes.AddUnique(RT);

			if (Context->BuiltInRenderTargetTypes.Num() >= MAX_RT)
				break;
		}

		Context->NumOutRenderTarget = Context->BuiltInRenderTargetTypes.Num();
	}

	if (Context->bUseDepthStencilTargetAsset)
	{
		auto& RT = InPassParameters.DepthStencilTargetAsset;
		
		if (RT.RenderTargetAsset && RT.RenderTargetAsset->CanBeUseAsDepthTarget())
		{
			FTextureRenderTargetResource* RTResource = RT.RenderTargetAsset->GameThread_GetRenderTargetResource();
			if (RTResource != nullptr)
			{
				FRenderTargetInternal RenderTargetInternal;
				RenderTargetInternal.RenderTarget = RTResource;
				RenderTargetInternal.ClearColor = RT.ClearColor;

				Context->DepthStencilTarget = RenderTargetInternal;
			}
		}

		Context->BuiltInDepthStencilTarget = EBuiltInDepthStencilTarget::None;
	}
	else
	{
		Context->BuiltInDepthStencilTarget = InPassParameters.DepthStencilTarget;
	}

	Context->InjectionPoint = InPassParameters.InjectionPoint;

	return Context;
}

FPassContext::FPassContext()
	: bSuspend(false)
	, bUseNaniteFallback(true)
	, bUseManualCulling(false)
	, bEnableMultiPassSupport(false)
	, bUseCustomViewProj(false)
	, bUseDepthStencilOnly(false)
	, bUseRenderTargetAssets(true)
	, bUseScreenViewportSize(false)
	, bResetStencilAfterUse(false)
{
}

FRHIDepthStencilState* FPassContext::GetDepthStencilStateRHI()
{
	if (!DepthStencilStateRHIRef.IsValid())
	{
		FDepthStencilStateInitializerRHI Initializer(
			PipelineState.DepthStencilState.bEnableDepthWrite,
			(ECompareFunction)PipelineState.DepthStencilState.DepthTest,
			PipelineState.DepthStencilState.FrontFaceStencilTest.bEnableStencil,
			(ECompareFunction)PipelineState.DepthStencilState.FrontFaceStencilTest.StencilTest,
			(EStencilOp)PipelineState.DepthStencilState.FrontFaceStencilTest.StencilTestFailOp,
			(EStencilOp)PipelineState.DepthStencilState.FrontFaceStencilTest.StencilTestPassDepthTestFailOp,
			(EStencilOp)PipelineState.DepthStencilState.FrontFaceStencilTest.StencilTestPassDepthTestPassOp,
			PipelineState.DepthStencilState.BackFaceStencilTest.bEnableStencil,
			(ECompareFunction)PipelineState.DepthStencilState.BackFaceStencilTest.StencilTest,
			(EStencilOp)PipelineState.DepthStencilState.BackFaceStencilTest.StencilTestFailOp,
			(EStencilOp)PipelineState.DepthStencilState.BackFaceStencilTest.StencilTestPassDepthTestFailOp,
			(EStencilOp)PipelineState.DepthStencilState.BackFaceStencilTest.StencilTestPassDepthTestPassOp,
			(uint8)PipelineState.DepthStencilState.StencilReadMask,
			(uint8)PipelineState.DepthStencilState.StencilWriteMask);

		DepthStencilStateRHIRef = RHICreateDepthStencilState(Initializer);
	}

	return DepthStencilStateRHIRef.GetReference();
}

FRHIDepthStencilState* FPassContext::GetResetStencilStateRHI()
{
	if (!StencilClearStateRHIRef.IsValid())
	{
		FDepthStencilStateInitializerRHI Initializer(
			false,
			CF_Always,
			PipelineState.DepthStencilState.FrontFaceStencilTest.bEnableStencil,
			CF_Equal,
			SO_Keep,
			SO_Keep,
			SO_Zero,
			PipelineState.DepthStencilState.BackFaceStencilTest.bEnableStencil,
			CF_Equal,
			SO_Keep,
			SO_Keep,
			SO_Zero,
			(uint8)PipelineState.DepthStencilState.StencilReadMask,
			(uint8)PipelineState.DepthStencilState.StencilWriteMask);

		StencilClearStateRHIRef = RHICreateDepthStencilState(Initializer);
	}

	return StencilClearStateRHIRef.GetReference();
}

FRHIBlendState* FPassContext::GetBlendStateRHI()
{
	if (!BlendStateRHIRef.IsValid())
	{
		TStaticArray<FBlendStateInitializerRHI::FRenderTarget, 8> RenderTargetBlendStates;
		for (int32 Index = 0; Index < 8; Index++)
		{
			if (PipelineState.BlendState.IsValidIndex(Index))
			{
				RenderTargetBlendStates[Index] = FBlendStateInitializerRHI::FRenderTarget(
					(EBlendOperation)PipelineState.BlendState[Index].ColorBlendOp,
					(EBlendFactor)PipelineState.BlendState[Index].ColorBlendOpSrcFactor,
					(EBlendFactor)PipelineState.BlendState[Index].ColorBlendOpDstFactor,
					(EBlendOperation)PipelineState.BlendState[Index].AlphaBlendOp,
					(EBlendFactor)PipelineState.BlendState[Index].AlphaBlendOpSrcFactor,
					(EBlendFactor)PipelineState.BlendState[Index].AlphaBlendOpDstFactor,
					(EColorWriteMask)PipelineState.BlendState[Index].ColorWriteMask);
			}
			else
			{
				RenderTargetBlendStates[Index] = FBlendStateInitializerRHI::FRenderTarget();
			}
		}

		BlendStateRHIRef = RHICreateBlendState(FBlendStateInitializerRHI(RenderTargetBlendStates, PipelineState.bUseAlphaToCoverage));
	}

	return BlendStateRHIRef;
}

bool FPassContext::IsSceneViewRelevanceRenderTarget() const
{
	bool bResult = false;
	bResult = !bUseRenderTargetAssets || bUseScreenViewportSize || bUseDepthStencilOnly;
	return bResult;
}

bool FPassContext::IsSafeToRender() const
{
	bool bResult = true;

	if (!IsOutputDepthTarget() &&
		(PipelineState.DepthStencilState.DepthTest != EDepthStencilCompareFunction::Always ||
			PipelineState.DepthStencilState.bEnableDepthWrite ||
			PipelineState.DepthStencilState.FrontFaceStencilTest.bEnableStencil ||
			PipelineState.DepthStencilState.BackFaceStencilTest.bEnableStencil ||
			bResetStencilAfterUse))
	{
		bResult = false;
	}

	return bResult;
}

bool FPassContext::IsOutputDepthTarget() const
{
	bool bResult = false;

	bResult = BuiltInDepthStencilTarget != EBuiltInDepthStencilTarget::None ||
		(bUseDepthStencilTargetAsset && DepthStencilTarget.IsValid());

	return bResult;
}

bool FPassContext::IsUseNaniteFallback() const
{
	return bUseNaniteFallback || bUseCustomViewProj;
}

void FPassContext::AdvanceFrame(EPassInjectionPoint InPassInjectionPoint, TFunctionRef<void()> Lambda)
{
	if (PassPriority < 0)
		return;

	if (bSuspend)
		return;

	if (!IsSafeToRender())
		return;

	if (InjectionPoint != InPassInjectionPoint)
		return;

	bool bActiveThisFrame = false;

	if (FrameRate == EFrameRate::Full)
	{
		bActiveThisFrame = true;
	}
	else if (FrameRate == EFrameRate::Half)
	{
		if (FrameCount % 2 == 0)
		{
			bActiveThisFrame = true;
			ActiveFrameCount++;
		}
	}
	else if (FrameRate == EFrameRate::Quarter)
	{
		if (FrameCount % 4 == 0)
		{
			bActiveThisFrame = true;
			ActiveFrameCount++;
		}
	}

	if (bActiveThisFrame)
	{
		Lambda();
	}

	FrameCount++;
}

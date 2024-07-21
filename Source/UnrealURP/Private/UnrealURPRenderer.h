// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "SceneViewExtension.h"
#include "UnrealURPConfig.h"
#include "UnrealURPStruct.h"

class FScene;
class FUnrealURPManager;
struct FPassContext;
class FStaticMeshSceneProxy;
class UStaticMeshComponent;
class UPrimitiveComponent;

class FUnrealURPRenderer : public FWorldSceneViewExtension
{
public:

	FUnrealURPRenderer(const FAutoRegister& AutoReg, UWorld* InWorld);
	~FUnrealURPRenderer() {}

	/**
	 * Called on game thread when creating the view family.
	 */
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}

	/**
	 * Called on game thread when creating the view.
	 */
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}

	/**
	* Called when creating the viewpoint, before culling, in case an external tracking device needs to modify the base location of the view
	*/
	virtual void SetupViewPoint(APlayerController* Player, FMinimalViewInfo& InViewInfo) override {}

	/**
	 * Called when creating the view, in case non-stereo devices need to update projection matrix.
	 */
	virtual void SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData) override {}

	/**
	 * Called on game thread when view family is about to be rendered.
	 */
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	/**
	 * Called on render thread at the start of rendering.
	 */
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {}

	/**
	 * Called on render thread at the start of rendering, for each view, after PreRenderViewFamily_RenderThread call.
	 */
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;

	/**
	 * Called on render thread prior to initializing views.
	 */
	virtual void PreInitViews_RenderThread(FRDGBuilder& GraphBuilder) override {}

	void PreRendering_RenderThread(FRDGBuilder& GraphBuilder);

#if ENGINE_MODIFY
	virtual void PostPrePass_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;
#endif

	/**
	 * Called right after Base Pass rendering finished when using the deferred renderer.
	 */
	virtual void PostRenderBasePassDeferred_RenderThread(
		FRDGBuilder& GraphBuilder,
		FSceneView& InView,
		const FRenderTargetBindingSlots& RenderTargets,
		TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;

	/**
	 * Called right after Base Pass rendering finished when using the mobile renderer.
	 */
#if UE_VERSION > UE5_2
	virtual void PostRenderBasePassMobile_RenderThread(FRHICommandList& RHICmdList, FSceneView& InView) override {}
#else
	virtual void PostRenderBasePassMobile_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}
#endif

	/**
	 * Called right before Translucency Pass rendering begins
	 */
	void PreTranslucencyPass_RenderThread(FPostOpaqueRenderParameters& InParameters);

	/**
	 * Called right before Post Processing rendering begins
	 */
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;

	/**
	* This will be called at the beginning of post processing to make sure that each view extension gets a chance to subscribe to an after pass event.
	*/
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

	FScreenPassTexture PostProcessPassAfterSSRInput_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);
	FScreenPassTexture PostProcessPassAfterMotionBlur_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);
	FScreenPassTexture PostProcessPassAfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);
	FScreenPassTexture PostProcessPassAfterFXAA_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);

	/**
	 * Allows to render content after the 3D content scene, useful for debugging
	 */
	virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {}

	/**
	 * Allows to render content after the 3D content scene, useful for debugging
	 */
	virtual void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {}

	/**
	 * Called to determine view extensions priority in relation to other view extensions, higher comes first
	 */
	virtual int32 GetPriority() const override { return -1; }

public:

	void AddPassContext_GameThread(FPassContext* InPassContext);
	void RemovePassContext_GameThread(const FName& InPassName);
	void DeletePassContexts_GameThread();
	void Deinitialize();

	void AddRenderObjectToPass_GameThread(const UPrimitiveComponent* InPrimitiveComp, const FMultiPassFlags& InMultiPassFlags, const FName& InPassName);
	void AddNaniteFallbackToPass_GameThread(UStaticMeshComponent* InNaniteFallbackComp, const FMultiPassFlags& InMultiPassFlags, const FName& InPassName);

	void RemoveRenderObjectFromPass_GameThread(const FPrimitiveComponentId& InRenderObjectID, const FName& InPassName);

	void SuspendPass_GameThread(const FName& InPassName);
	void ResumePass_GameThread(const FName& InPassName);

	void UpdatePassPriority_GameThread(const FName& InPassName, int32 InPassPriority);
	void UpdatePassCamera_GameThread(FName InPassName, const FMatrix& InCustomViewProj);

private:

	void RedirectRenderPassByInjectionPoint(FRDGBuilder& GraphBuilder, const FSceneView& View, const uint8& InjectionPoint);
	void RenderInternal(FRDGBuilder& GraphBuilder, FScene* Scene, const FViewInfo& View, FSceneTextures& SceneTextures, FPassContext& PassContext);
	void RenderNaniteScreenQuadPass(FRDGBuilder& GraphBuilder, FScene* Scene, const FViewInfo& View, const FSceneTextures& SceneTextures, FPassContext& PassContext, const FRenderTargetBindingSlots& RenderTargets);
	void ResetStencilAfterUse(FRDGBuilder& GraphBuilder, const FViewInfo& View, FRDGTextureRef StencilClearTarget, FPassContext& PassContext);
	void SortPass();

	FScreenPassTexture ReturnUntouchedSceneColorForPostProcessing(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs);

private:

	EWorldType::Type WorldType;
	TArray<const FSceneView*> ViewsForPreRendering;

	struct FNaniteFallbackPerPass
	{
		FName PassName;
		TArray<FPrimitiveComponentId> NaniteFallbackRenderObjectIDs;

		FNaniteFallbackPerPass() {}
		FNaniteFallbackPerPass(const FName& InPassName)
			: PassName(InPassName)
		{}

		inline bool operator==(const FNaniteFallbackPerPass& Other) const
		{
			return PassName == Other.PassName;
		}
	};

	TArray<FNaniteFallbackPerPass> NaniteFallbackPerPassArray;

	struct FNaniteFallback
	{
		FPrimitiveComponentId NaniteFallbackRenderObjectID;
		uint32 ReferenceCount = 1; // One ref as soon as construt.

		FNaniteFallback() {}
		FNaniteFallback(const FPrimitiveComponentId& InCompID)
			: NaniteFallbackRenderObjectID(InCompID)
		{}

		inline bool operator==(const FNaniteFallback& Other) const
		{
			return NaniteFallbackRenderObjectID == Other.NaniteFallbackRenderObjectID;
		}
	};

	TArray<FNaniteFallback> NaniteFallbacks; // GameThread R/W Only.
	TArray<FStaticMeshSceneProxy*> NaniteFallbackProxies;

	FDelegateHandle PrivatePreRenderingHandle;
	FDelegateHandle PrivatePreTranslucencyPassRenderHandle;
	
	// GameThread created, RenderThread use and delete.
	TArray<FPassContext*> PassContexts;
};

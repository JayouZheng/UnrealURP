// Copyright Jayou, Inc. All Rights Reserved.

#pragma optimize("", off)

#include "UnrealURPRenderer.h"
#include "UnrealURPManager.h"
#include "UnrealURPStruct.h"
#include "UnrealURPPassContext.h"

#include "EngineModule.h"
#include "RenderGraph.h"
#include "GlobalShader.h"
#include "MaterialShader.h"
#include "SceneRendering.h"
#include "SystemTextures.h"
#include "ScenePrivate.h"
#include "SceneTextures.h"
#include "PostProcess/PostProcessMaterial.h"
#include "ScreenPass.h"
#include "SceneManagement.h"
#include "CommonRenderResources.h"
#include "HAL/IConsoleManager.h"
#include "StaticMeshResources.h"
#include "PixelShaderUtils.h"
#include "ClearQuad.h"

#include "Nanite/NaniteCullRaster.h"
#include "Nanite/NaniteShared.h"
#include "Rendering/NaniteStreamingManager.h"

#include "Runtime/Launch/Resources/Version.h"

#if UE_VERSION > UE5_1
#include "StaticMeshBatch.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "Materials/MaterialRenderProxy.h"
#include "StaticMeshSceneProxy.h"
#endif

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FCustomMeshPassUniformParameters, )
	SHADER_PARAMETER(FMatrix44f, CustomViewProj)
	SHADER_PARAMETER(FIntVector4, PageConstants)
	SHADER_PARAMETER_RDG_BUFFER_SRV(ByteAddressBuffer, VisibleClustersSWHW)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D<UlongType>, VisBuffer64)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

IMPLEMENT_STATIC_UNIFORM_BUFFER_SLOT(CustomMeshPass)
IMPLEMENT_STATIC_UNIFORM_BUFFER_STRUCT(FCustomMeshPassUniformParameters, "CustomMeshPassUBO", CustomMeshPass);

BEGIN_SHADER_PARAMETER_STRUCT(FCustomMeshPassParameters, )
	SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
	SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureShaderParameters, SceneTextures)
#if UE_VERSION > UE5_2
	SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneUniformParameters, Scene)
#endif
	SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FCustomMeshPassUniformParameters, PassUniformBuffer)
	SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FInstanceCullingGlobalUniforms, InstanceCulling)
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FCustomMeshPassSingleDrawUBO, )
	SHADER_PARAMETER(FVector4f, TilePosition)
	SHADER_PARAMETER(FMatrix44f, LocalToRelativeWorld)
	SHADER_PARAMETER(FVector4f, CompressedLocalToRelativeWorldRotation)
	SHADER_PARAMETER(FVector4f, CompressedLocalToRelativeWorldTranslation)
	SHADER_PARAMETER(uint32, PrimitiveComponentId)
	SHADER_PARAMETER(uint32, InstanceSceneDataOffset)
	SHADER_PARAMETER(FUintVector2, Padding)
	SHADER_PARAMETER_ARRAY(FUintVector4, CustomPrimitiveData, [FCustomPrimitiveData::NumCustomPrimitiveDataFloat4s])
END_GLOBAL_SHADER_PARAMETER_STRUCT()

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FCustomMeshPassSingleDrawUBO, "CustomMeshPassSingleDrawUBO");

DECLARE_GPU_STAT_NAMED(CustomMeshPass, TEXT("CustomMeshPass"));

DECLARE_CYCLE_STAT(TEXT("CustomMeshPass"), STAT_CustomMeshPass, STATGROUP_SceneRendering);

class FCustomMeshPassVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FCustomMeshPassVS, MeshMaterial);

	class FMultiPassFlag : SHADER_PERMUTATION_INT("PERMUTATION_MULTIPASSFLAG", 4);
	using FPermutationDomain = TShaderPermutationDomain<FMultiPassFlag>;

public:

	FCustomMeshPassVS() = default;
	FCustomMeshPassVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
		VertexFactoryParameters = Initializer.VertexFactoryType->CreateShaderParameters(Initializer.Target.GetFrequency(), Initializer.ParameterMap);

		// Per Mesh Update Parameter Bind.
		CustomMeshPassSingleDrawUBO.Bind(Initializer.ParameterMap, TEXT("CustomMeshPassSingleDrawUBO"));
	}

	static void ModifyCompilationEnvironment(const FMeshMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		// Set Define in Shader. 
		OutEnvironment.SetDefine(TEXT("GPUSKIN"), Parameters.VertexFactoryType->GetFName() == FName(TEXT("TGPUSkinVertexFactoryDefault")));
		OutEnvironment.SetDefine(TEXT("USE_MESH_PARTICLE"), Parameters.VertexFactoryType->GetFName() == FName(TEXT("FNiagaraMeshVertexFactory")));
		OutEnvironment.SetDefine(TEXT("USE_SPRITE_PARTICLE"), Parameters.VertexFactoryType->GetFName() == FName(TEXT("FNiagaraSpriteVertexFactory")));
		OutEnvironment.SetDefine(TEXT("USE_LOCALVF"), Parameters.VertexFactoryType->GetFName() == FName(TEXT("FLocalVertexFactory")));
		OutEnvironment.SetDefine(TEXT("VERTEX_SHADER"), 1);
		OutEnvironment.SetDefine(TEXT("CUSTOM_MESH_PASS"), 1);
		OutEnvironment.SetDefine(TEXT("CUSTOM_MESH_PASS_NANITE"), 0);
		OutEnvironment.SetDefine(TEXT("UE_VERSION"), UE_VERSION);
	}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
#if SHADER_PERMUTATION_OPTIMIZE
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
			!Parameters.MaterialParameters.bIsDefaultMaterial &&
			!Parameters.MaterialParameters.bIsSpecialEngineMaterial &&
			Parameters.MaterialParameters.bIsUsedWithUEURP &&
			Parameters.VertexFactoryType->GetFName() != FName(TEXT("Nanite::FVertexFactory"));
#else
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
			Parameters.MaterialParameters.MaterialDomain == MD_Surface &&
			(Parameters.MaterialParameters.BlendMode == BLEND_Opaque ||
				Parameters.MaterialParameters.BlendMode == BLEND_Masked ||
				Parameters.MaterialParameters.BlendMode == BLEND_Translucent) &&
			(Parameters.MaterialParameters.ShadingModels == MSM_Unlit || 
				Parameters.MaterialParameters.ShadingModels == MSM_DefaultLit) &&
			!Parameters.MaterialParameters.bIsDefaultMaterial &&
			!Parameters.MaterialParameters.bIsSpecialEngineMaterial &&
			(Parameters.VertexFactoryType->GetFName() == FName(TEXT("TGPUSkinVertexFactoryDefault")) ||
				Parameters.VertexFactoryType->GetFName() == FName(TEXT("FLocalVertexFactory")) ||
				Parameters.VertexFactoryType->GetFName() == FName(TEXT("FNiagaraMeshVertexFactory")) ||
				Parameters.VertexFactoryType->GetFName() == FName(TEXT("FNiagaraSpriteVertexFactory")));
#endif
	}

	void GetShaderBindings(
		const FScene* Scene,
		const FStaticFeatureLevel FeatureLevel,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material,
		FMeshDrawSingleShaderBindings& ShaderBindings,
		const TUniformBufferRef<FCustomMeshPassSingleDrawUBO>& SingleDrawUBOParameters) const
	{
		FMaterialShader::GetShaderBindings(Scene, FeatureLevel, MaterialRenderProxy, Material, ShaderBindings);

		ShaderBindings.Add(CustomMeshPassSingleDrawUBO, SingleDrawUBOParameters);
	}

	void GetVertexFactoryShaderBindings(
		const FVertexFactoryType* VertexFactoryType,
		const FScene* Scene,
		const FSceneView* ViewIfDynamicMeshCommand,
		const FVertexFactory* VertexFactory,
		const EVertexInputStreamType InputStreamType,
		const FStaticFeatureLevel FeatureLevel,
		const FMeshBatchElement& BatchElement,
		FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams)
	{
		if (VertexFactoryType)
		{
			const FVertexFactoryShaderParameters* VFParameters = VertexFactoryParameters.Get();
			if (VFParameters)
			{
				VertexFactoryType->GetShaderParameterElementShaderBindings(GetFrequency(), VFParameters, Scene, ViewIfDynamicMeshCommand, this, InputStreamType, FeatureLevel, VertexFactory, BatchElement, ShaderBindings, VertexStreams);
			}
		}
	}

private:
	void WriteFrozenVertexFactoryParameters(FMemoryImageWriter& Writer, const TMemoryImagePtr<FVertexFactoryShaderParameters>& InVertexFactoryParameters) const
	{
		const FVertexFactoryType* VertexFactoryType = GetVertexFactoryType(Writer.TryGetPrevPointerTable());
		InVertexFactoryParameters.WriteMemoryImageWithDerivedType(Writer, VertexFactoryType->GetShaderParameterLayout(GetFrequency()));
	}
	LAYOUT_FIELD_WITH_WRITER(TMemoryImagePtr<FVertexFactoryShaderParameters>, VertexFactoryParameters, WriteFrozenVertexFactoryParameters);

private:

	// Per Mesh Update Parameter Bind.
	LAYOUT_FIELD(FShaderUniformBufferParameter, CustomMeshPassSingleDrawUBO);
};

class FCustomMeshPassPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FCustomMeshPassPS, MeshMaterial);

	// Add shader permutation here.
	class FOutputRenderTargetCount : SHADER_PERMUTATION_INT("PERMUTATION_OUTPUTRENDERTARGETCOUNT", MAX_RT);
	class FOutputDepthTarget : SHADER_PERMUTATION_BOOL("PERMUTATION_OUTPUTDEPTHTARGET");
	class FIsNaniteFallback : SHADER_PERMUTATION_BOOL("PERMUTATION_ISNANITEFALLBACK");
	class FMultiPassFlag : SHADER_PERMUTATION_INT("PERMUTATION_MULTIPASSFLAG", 4);
	using FPermutationDomain = TShaderPermutationDomain<FOutputRenderTargetCount, FOutputDepthTarget, FIsNaniteFallback, FMultiPassFlag>;

public:

	FCustomMeshPassPS() = default;
	FCustomMeshPassPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
		// Per Mesh Update Parameter Bind.
		CustomMeshPassSingleDrawUBO.Bind(Initializer.ParameterMap, TEXT("CustomMeshPassSingleDrawUBO"));
	}

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		// Set Define in Shader. 
		OutEnvironment.SetDefine(TEXT("PIXEL_SHADER"), 1);
		OutEnvironment.SetDefine(TEXT("CUSTOM_MESH_PASS"), 1);
		OutEnvironment.SetDefine(TEXT("CUSTOM_MESH_PASS_NANITE"), 0);
	}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
#if SHADER_PERMUTATION_OPTIMIZE
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
			!Parameters.MaterialParameters.bIsDefaultMaterial &&
			!Parameters.MaterialParameters.bIsSpecialEngineMaterial &&
			Parameters.MaterialParameters.bIsUsedWithUEURP &&
			Parameters.VertexFactoryType->GetFName() != FName(TEXT("Nanite::FVertexFactory"));
#else
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
			Parameters.MaterialParameters.MaterialDomain == MD_Surface &&
			(Parameters.MaterialParameters.BlendMode == BLEND_Opaque ||
				Parameters.MaterialParameters.BlendMode == BLEND_Masked ||
				Parameters.MaterialParameters.BlendMode == BLEND_Translucent ||
				Parameters.MaterialParameters.BlendMode == BLEND_Additive) &&
			(Parameters.MaterialParameters.ShadingModels == MSM_Unlit ||
				Parameters.MaterialParameters.ShadingModels == MSM_DefaultLit) &&
			!Parameters.MaterialParameters.bIsDefaultMaterial &&
			!Parameters.MaterialParameters.bIsSpecialEngineMaterial &&
			(Parameters.VertexFactoryType->GetFName() == FName(TEXT("TGPUSkinVertexFactoryDefault")) ||
				Parameters.VertexFactoryType->GetFName() == FName(TEXT("FLocalVertexFactory")) ||
				Parameters.VertexFactoryType->GetFName() == FName(TEXT("FNiagaraMeshVertexFactory")) ||
				Parameters.VertexFactoryType->GetFName() == FName(TEXT("FNiagaraSpriteVertexFactory")));
#endif
	}

	void GetShaderBindings(
		const FScene* Scene,
		const FStaticFeatureLevel FeatureLevel,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material,
		FMeshDrawSingleShaderBindings& ShaderBindings,
		const TUniformBufferRef<FCustomMeshPassSingleDrawUBO>& SingleDrawUBOParameters) const
	{
		FMaterialShader::GetShaderBindings(Scene, FeatureLevel, MaterialRenderProxy, Material, ShaderBindings);

		ShaderBindings.Add(CustomMeshPassSingleDrawUBO, SingleDrawUBOParameters);
	}

private:

	// Per Mesh Update Parameter Bind.
	LAYOUT_FIELD(FShaderUniformBufferParameter, CustomMeshPassSingleDrawUBO);
};

class FNaniteScreenQuadPS : public FMaterialShader
{
	DECLARE_SHADER_TYPE(FNaniteScreenQuadPS, Material);
	SHADER_USE_PARAMETER_STRUCT_WITH_LEGACY_BASE(FNaniteScreenQuadPS, FMaterialShader);

	class FOutputRenderTargetCount : SHADER_PERMUTATION_INT("PERMUTATION_OUTPUTRENDERTARGETCOUNT", MAX_RT);
	class FOutputDepthTarget : SHADER_PERMUTATION_BOOL("PERMUTATION_OUTPUTDEPTHTARGET");
	class FUseDepthStencilOnly : SHADER_PERMUTATION_BOOL("PERMUTATION_USEDEPTHSTENCILONLY");
	//class FMultiPassFlag : SHADER_PERMUTATION_INT("PERMUTATION_MULTIPASSFLAG", 4);
	using FPermutationDomain = TShaderPermutationDomain<FOutputRenderTargetCount, FOutputDepthTarget, FUseDepthStencilOnly/*, FMultiPassFlag*/>;

	static bool ShouldCompilePermutation(const FMaterialShaderPermutationParameters& Parameters)
	{
#if SHADER_PERMUTATION_OPTIMIZE
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
			!Parameters.MaterialParameters.bIsDefaultMaterial &&
			!Parameters.MaterialParameters.bIsSpecialEngineMaterial &&
			Parameters.MaterialParameters.bIsUsedWithUEURP;
#else
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
			Parameters.MaterialParameters.MaterialDomain == MD_Surface &&
			Parameters.MaterialParameters.BlendMode == BLEND_Opaque &&
			Parameters.MaterialParameters.ShadingModels == MSM_Unlit &&
			!Parameters.MaterialParameters.bIsDefaultMaterial &&
			!Parameters.MaterialParameters.bIsSpecialEngineMaterial;
#endif
	}

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

#if UE_VERSION >= UE5_5_4
		// Force shader model 6.0+
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceDXC);
		OutEnvironment.CompilerFlags.Add(CFLAG_HLSL2021);
		OutEnvironment.CompilerFlags.Add(CFLAG_ShaderBundle);
		OutEnvironment.CompilerFlags.Add(CFLAG_RootConstants);
#endif

		OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"), 1);
		OutEnvironment.SetDefine(TEXT("NANITE_USE_UNIFORM_BUFFER"), 0);
		OutEnvironment.SetDefine(TEXT("IS_NANITE_SHADING_PASS"), 0);
		OutEnvironment.SetDefine(TEXT("NANITE_USE_VIEW_UNIFORM_BUFFER"), 0);
		OutEnvironment.SetDefine(TEXT("IS_NANITE_PASS"), 1);

		OutEnvironment.SetDefine(TEXT("CUSTOM_MESH_PASS"), 0);
		OutEnvironment.SetDefine(TEXT("CUSTOM_MESH_PASS_NANITE"), 1);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER(FIntVector4, PageConstants)
		SHADER_PARAMETER_RDG_BUFFER_SRV(ByteAddressBuffer, ClusterPageData)
		SHADER_PARAMETER_RDG_BUFFER_SRV(ByteAddressBuffer, VisibleClustersSWHW)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<UlongType>, VisBuffer64)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FPackedNaniteView>, InViews)
		SHADER_PARAMETER(FUintVector4, NaniteCPDMark)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FCustomMeshPassVS, TEXT("/Plugin/UnrealURP/Private/CustomMeshPassShader.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FCustomMeshPassPS, TEXT("/Plugin/UnrealURP/Private/CustomMeshPassShader.usf"), TEXT("MainPS"), SF_Pixel);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FNaniteScreenQuadPS, TEXT("/Plugin/UnrealURP/Private/NaniteScreenQuadPS.usf"), TEXT("MainPS"), SF_Pixel);

class FFullScreenQuadVS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFullScreenQuadVS);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	FFullScreenQuadVS() = default;
	FFullScreenQuadVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{}
};

IMPLEMENT_GLOBAL_SHADER(FFullScreenQuadVS, "/Plugin/UnrealURP/Private/FullScreenQuad.usf", "MainVS", SF_Vertex);

FInt32Range GetDynamicMeshElementRange(const FViewInfo& View, uint32 PrimitiveIndex)
{
#if UE_VERSION > UE5_2
	// DynamicMeshEndIndices contains valid values only for visible primitives with bDynamicRelevance.
	if (View.PrimitiveVisibilityMap[PrimitiveIndex])
	{
		const FPrimitiveViewRelevance& ViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveIndex];
		if (ViewRelevance.bDynamicRelevance)
		{

			return FInt32Range(View.DynamicMeshElementRanges[PrimitiveIndex].X, View.DynamicMeshElementRanges[PrimitiveIndex].Y);
		}
	}

	return FInt32Range::Empty();
#else
	int32 Start = 0;	// inclusive
	int32 AfterEnd = 0;	// exclusive

	// DynamicMeshEndIndices contains valid values only for visible primitives with bDynamicRelevance.
	if (View.PrimitiveVisibilityMap[PrimitiveIndex])
	{
		const FPrimitiveViewRelevance& ViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveIndex];
		if (ViewRelevance.bDynamicRelevance)
		{
			Start = (PrimitiveIndex == 0) ? 0 : View.DynamicMeshEndIndices[PrimitiveIndex - 1];
			AfterEnd = View.DynamicMeshEndIndices[PrimitiveIndex];
		}
	}

	return FInt32Range(Start, AfterEnd);
#endif
}

struct FMeshDrawInfo
{
	EVertexInputStreamType VertexInputStreamType = EVertexInputStreamType::Default;
	FVertexFactoryType* VertexFactoryType = nullptr;

	const FVertexFactory* VertexFactory = nullptr;
	const FPrimitiveSceneProxy* Proxy = nullptr;
	const FMaterialRenderProxy* MultiPassMat = nullptr;
	const FMeshBatchElement* BatchElement = nullptr;

	FVertexInputStreamArray VertexInputStreamArray;
	FRHIBuffer* IndexBuffer;

	uint32 NumVertices;
	uint32 NumPrimitives;
	uint32 NumInstances;

	uint32 FirstIndex;
	uint32 BaseVertexIndex;

	EPrimitiveType OverridePrimitiveType = EPrimitiveType::PT_Num;

	FMeshDrawShaderBindings ShaderBindings;

	FCustomMeshPassSingleDrawUBO Parameters;
	TUniformBufferRef<FCustomMeshPassSingleDrawUBO> SingleDrawUBO;

	bool bHasValidShader = false;
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	FGraphicsMinimalPipelineStateId CachedPipelineId;

	const FPassContext* PassContext = nullptr;

	void Init(
		const FViewInfo& View,
		FPrimitiveSceneProxy* InProxy,
		const FMeshBatch& InMeshBatch,
		const FMeshBatchElement& InBatchElement)
	{
		VertexInputStreamType = EVertexInputStreamType::Default;
		VertexFactory = InMeshBatch.VertexFactory;
		Proxy = InProxy;
		MultiPassMat = InMeshBatch.MaterialRenderProxy;
		BatchElement = &InBatchElement;

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = InMeshBatch.VertexFactory->GetDeclaration(VertexInputStreamType);
		InMeshBatch.VertexFactory->GetStreams(View.GetFeatureLevel(), VertexInputStreamType, VertexInputStreamArray);
		IndexBuffer = InBatchElement.IndexBuffer->IndexBufferRHI.GetReference();

		NumVertices = InBatchElement.MaxVertexIndex - InBatchElement.MinVertexIndex + 1;
		NumPrimitives = InBatchElement.NumPrimitives;
		NumInstances = InBatchElement.NumInstances;

		FirstIndex = InBatchElement.FirstIndex;
		BaseVertexIndex = InBatchElement.BaseVertexIndex;

		VertexFactoryType = InMeshBatch.VertexFactory->GetType();

		OverridePrimitiveType = EPrimitiveType(InMeshBatch.Type);

		// If the mesh batch has a valid dynamic index buffer, use it instead
		///////////////////////////////////////////////////////////////////////////////////////////////
		//if (InBatchElement.DynamicIndexBuffer.IsValid())
		//{
		//	IndexBuffer = InBatchElement.DynamicIndexBuffer.IndexBuffer->IndexBufferRHI.GetReference();
		//	FirstIndex = InBatchElement.DynamicIndexBuffer.FirstIndex;
		//	OverridePrimitiveType = EPrimitiveType(InBatchElement.DynamicIndexBuffer.PrimitiveType);
		//}
		///////////////////////////////////////////////////////////////////////////////////////////////

		{
			const FLargeWorldRenderPosition AbsoluteWorldPosition(Proxy->GetLocalToWorld().GetOrigin());
			const FVector TilePositionOffset = AbsoluteWorldPosition.GetTileOffset();

			Parameters.TilePosition = AbsoluteWorldPosition.GetTile();

			// Inverse on FMatrix44f can generate NaNs if the source matrix contains large scaling, so do it in double precision.
			FMatrix LocalToRelativeWorld = FLargeWorldRenderScalar::MakeToRelativeWorldMatrixDouble(TilePositionOffset, Proxy->GetLocalToWorld());
			Parameters.LocalToRelativeWorld = FMatrix44f(LocalToRelativeWorld);

			FCompressedTransform CompressedLocalToWorld(LocalToRelativeWorld);
			Parameters.CompressedLocalToRelativeWorldRotation = *(const FVector4f*)&CompressedLocalToWorld.Rotation[0];
			Parameters.CompressedLocalToRelativeWorldTranslation = *(const FVector3f*)&CompressedLocalToWorld.Translation;

			Parameters.PrimitiveComponentId = Proxy->GetPrimitiveComponentId().PrimIDValue;
			Parameters.InstanceSceneDataOffset = Proxy->GetPrimitiveSceneInfo()->GetInstanceSceneDataOffset();

			if (VertexFactoryType->GetFName() == FName(TEXT("FNiagaraMeshVertexFactory")))
			{
				Parameters.InstanceSceneDataOffset = View.DynamicPrimitiveCollector.GetInstanceSceneDataOffset() + InBatchElement.DynamicPrimitiveInstanceSceneDataOffset;
			}

			FMemory::Memzero(Parameters.CustomPrimitiveData);
			FMemory::Memcpy(
				&Parameters.CustomPrimitiveData,
				Proxy->GetCustomPrimitiveData()->Data.GetData(),
				Proxy->GetCustomPrimitiveData()->Data.GetTypeSize() * FMath::Min(Proxy->GetCustomPrimitiveData()->Data.Num(),
					FCustomPrimitiveData::NumCustomPrimitiveDataFloats)
			);
		}

		SingleDrawUBO = CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleDraw);
	}

	// TODO: Is set pso per DrawInfo necessary? make something better...
	void SetupPSO(FPassContext& InPassContext)
	{
		GraphicsPSOInit.DepthStencilState = InPassContext.GetDepthStencilStateRHI();
		GraphicsPSOInit.BlendState = InPassContext.GetBlendStateRHI();
		GraphicsPSOInit.RasterizerState = GetStaticRasterizerState<false>(
			(ERasterizerFillMode)InPassContext.PipelineState.FillMode,
			(ERasterizerCullMode)InPassContext.PipelineState.CullMode);

		GraphicsPSOInit.PrimitiveType = (EPrimitiveType)InPassContext.PipelineState.PrimitiveType;

		if (OverridePrimitiveType != EPrimitiveType::PT_Num)
		{
			GraphicsPSOInit.PrimitiveType = OverridePrimitiveType;
		}

		PassContext = &InPassContext;
	}

	~FMeshDrawInfo()
	{
		SingleDrawUBO.SafeRelease();
	}
};

struct FCustomMeshPassDrawContext
{
	class FMeshDrawInfoSetupAsyncTask : public FNonAbandonableTask
	{
		friend class FAsyncTask<FMeshDrawInfoSetupAsyncTask>;

		FCustomMeshPassDrawContext* DrawContext;

		FMeshDrawInfoSetupAsyncTask(FCustomMeshPassDrawContext* InDrawContext)
			: DrawContext(InDrawContext)
		{
		}

		void DoWork()
		{
			FOptionalTaskTagScope Scope(ETaskTag::EParallelRenderingThread);
			DrawContext->AsyncSetupMeshDrawInfos();
		}

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FMeshDrawInfoSetupAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
		}
	};

	const FScene* Scene = nullptr;
	const FViewInfo* View = nullptr;
	FPassContext* PassContext = nullptr;
	TArray<FStaticMeshSceneProxy*>* NaniteFallbackProxies;

	bool bAsyncFlag = false;
	FAsyncTask<FMeshDrawInfoSetupAsyncTask>* MeshDrawInfoSetupAsyncTask = nullptr;

	int32 AllocatedMeshBatchCount = 0;
	int32 AllocatedMeshDrawInfoCount = 0;

	FMeshBatch* CachedNaniteFallbackMeshBatches = nullptr;
	FMeshDrawInfo* DrawList = nullptr; // Need sort.

	FPassContext::FNaniteFallbackRenderInfo** VisibleNaniteFallbackRenderInfos = nullptr;
	int32* VisiblePrimitiveIndices = nullptr;
	FMultiPassFlags* VisibleMultiPassFlags = nullptr;

	int32 NumCachedNaniteFallbackMeshBatches = 0;
	int32 NumMeshDrawInfos = 0;

	FCustomMeshPassDrawContext(FRDGBuilder& GraphBuilder, const FScene* InScene, const FViewInfo* InView, FPassContext* InPassContext, TArray<FStaticMeshSceneProxy*>* InNaniteFallbackProxies)
		: Scene(InScene)
		, View(InView)
		, PassContext(InPassContext)
		, NaniteFallbackProxies(InNaniteFallbackProxies)
	{
		SCOPED_NAMED_EVENT(FCustomMeshPassDrawContext, FColor::Emerald);

		MeshDrawInfoSetupAsyncTask = GraphBuilder.AllocObject<FAsyncTask<FMeshDrawInfoSetupAsyncTask>>(this);

		VisibleNaniteFallbackRenderInfos = GraphBuilder.AllocPODArray<FPassContext::FNaniteFallbackRenderInfo*>(PassContext->NaniteFallbackRenderInfos.Num());
		VisiblePrimitiveIndices = GraphBuilder.AllocPODArray<int32>(PassContext->RenderObjects.Num());
		VisibleMultiPassFlags = GraphBuilder.AllocPODArray<FMultiPassFlags>(PassContext->RenderObjects.Num());

		for (int32 Index = 0; Index < InPassContext->NaniteFallbackRenderInfos.Num(); Index++)
		{
			for (int32 SectionIndex = 0; SectionIndex < InPassContext->NaniteFallbackRenderInfos[Index].NumSections; SectionIndex++)
			{
				NumCachedNaniteFallbackMeshBatches++;
			}			
		}

		int32 MaxDrawInfo = NumCachedNaniteFallbackMeshBatches + InView->DynamicMeshElements.Num();

		for (int32 Index = 0; Index < InPassContext->RenderObjects.Num(); Index++)
		{
			for (int32 DrawInfoIndex = 0; DrawInfoIndex < InPassContext->RenderObjects[Index].MaxDrawInfo; DrawInfoIndex++)
			{
				MaxDrawInfo++;
			}
		}
		
		for (int32 Index = 0; Index < MaxDrawInfo * 4; Index++)
		{
			NumMeshDrawInfos++;
		}

		CachedNaniteFallbackMeshBatches = GraphBuilder.AllocPODArray<FMeshBatch>(NumCachedNaniteFallbackMeshBatches);
		DrawList = GraphBuilder.AllocPODArray<FMeshDrawInfo>(NumMeshDrawInfos);
	}
	
	~FCustomMeshPassDrawContext()
	{
		for (int32 Index = 0; Index < AllocatedMeshBatchCount; Index++)
		{
			CachedNaniteFallbackMeshBatches[Index].~FMeshBatch();
		}

		for (int32 Index = 0; Index < AllocatedMeshDrawInfoCount; Index++)
		{
			DrawList[Index].~FMeshDrawInfo();
		}
	}

private:

	FMeshBatch& AllocNaniteFallbackMeshBatch()
	{
		new (CachedNaniteFallbackMeshBatches + AllocatedMeshBatchCount) FMeshBatch();
		return CachedNaniteFallbackMeshBatches[AllocatedMeshBatchCount++];
	}

	FMeshDrawInfo& AllocMeshDrawInfo()
	{
		new (DrawList + AllocatedMeshDrawInfoCount) FMeshDrawInfo();
		return DrawList[AllocatedMeshDrawInfoCount++];
	}

	void FinalizeMeshDrawInfo(FMeshDrawInfo& InDrawInfo, uint8 MultiPassFlag, bool bIsNaniteFallback = false)
	{
		TMeshProcessorShaders<
			FCustomMeshPassVS,
			FCustomMeshPassPS> PassShaders;

		const FMaterialRenderProxy* MaterialRenderProxy = InDrawInfo.PassContext->bEnableMultiPassSupport ? 
			InDrawInfo.MultiPassMat : InDrawInfo.PassContext->MaterialProxy;
		const FMaterial* Material = MaterialRenderProxy->GetMaterialNoFallback(Scene->GetFeatureLevel());

		FCustomMeshPassVS::FPermutationDomain PermutationVectorVS;
		PermutationVectorVS.Set<FCustomMeshPassVS::FMultiPassFlag>(MultiPassFlag);

		FCustomMeshPassPS::FPermutationDomain PermutationVectorPS;
		PermutationVectorPS.Set<FCustomMeshPassPS::FOutputRenderTargetCount>(InDrawInfo.PassContext->NumOutRenderTarget);
		PermutationVectorPS.Set<FCustomMeshPassPS::FOutputDepthTarget>(InDrawInfo.PassContext->IsOutputDepthTarget());
		PermutationVectorPS.Set<FCustomMeshPassPS::FIsNaniteFallback>(bIsNaniteFallback);
		PermutationVectorPS.Set<FCustomMeshPassPS::FMultiPassFlag>(MultiPassFlag);

		auto TryGetShader = [&](FMaterialShaders& OutShaders) -> bool
		{
			while (MaterialRenderProxy)
			{
				Material = MaterialRenderProxy->GetMaterialNoFallback(Scene->GetFeatureLevel());
				if (Material && Material->GetRenderingThreadShaderMap())
				{
					FMaterialShaderTypes ShaderTypes;
					ShaderTypes.AddShaderType<FCustomMeshPassVS>(PermutationVectorVS.ToDimensionValueId());
					ShaderTypes.AddShaderType<FCustomMeshPassPS>(PermutationVectorPS.ToDimensionValueId());

					if (Material->TryGetShaders(ShaderTypes, InDrawInfo.VertexFactoryType, OutShaders))
					{
						return true;
					}
				}

				MaterialRenderProxy = MaterialRenderProxy->GetFallback(Scene->GetFeatureLevel());
			}
			return false;
		};

		// Try Get Shader.
		FMaterialShaders Shaders;
		if (TryGetShader(Shaders))
		{
			InDrawInfo.bHasValidShader = true;
		}
		else
		{
			InDrawInfo.bHasValidShader = false;
			return;
		}

		Shaders.TryGetVertexShader(PassShaders.VertexShader);
		Shaders.TryGetPixelShader(PassShaders.PixelShader);

		if (InDrawInfo.PassContext->bUseDepthStencilOnly)
		{
			PassShaders.PixelShader.Reset();
		}

		// Init ShaderBindings.
		InDrawInfo.ShaderBindings.Initialize(PassShaders.GetUntypedShaders());

		int32 DataOffset = 0;
		if (PassShaders.VertexShader.IsValid())
		{
			FMeshDrawSingleShaderBindings SingleShaderBindings = InDrawInfo.ShaderBindings.GetSingleShaderBindings(SF_Vertex, DataOffset);
			PassShaders.VertexShader->GetShaderBindings(
				Scene,
				Scene->GetFeatureLevel(),
				*MaterialRenderProxy,
				*Material,
				SingleShaderBindings,
				InDrawInfo.SingleDrawUBO);

			PassShaders.VertexShader->GetVertexFactoryShaderBindings(
				InDrawInfo.VertexFactoryType,
				Scene,
				View,
				InDrawInfo.VertexFactory,
				InDrawInfo.VertexInputStreamType,
				Scene->GetFeatureLevel(),
				*InDrawInfo.BatchElement,
				SingleShaderBindings,
				InDrawInfo.VertexInputStreamArray);
		}
		if (PassShaders.PixelShader.IsValid())
		{
			FMeshDrawSingleShaderBindings SingleShaderBindings = InDrawInfo.ShaderBindings.GetSingleShaderBindings(SF_Pixel, DataOffset);
			PassShaders.PixelShader->GetShaderBindings(
				Scene,
				Scene->GetFeatureLevel(),
				*MaterialRenderProxy,
				*Material,
				SingleShaderBindings,
				InDrawInfo.SingleDrawUBO);
		}

		// Init PSO.
		// TODO: Is set pso per DrawInfo necessary? make something better...
		InDrawInfo.GraphicsPSOInit.BoundShaderState.VertexShaderRHI = PassShaders.VertexShader.GetVertexShader();
		InDrawInfo.GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PassShaders.PixelShader.GetPixelShader();
	}

	void SetupStaticMeshDrawInfos(const FPrimitiveSceneInfo& PrimitiveSceneInfo, const FMultiPassFlags& MultiPassFlags)
	{
		SCOPED_NAMED_EVENT(SetupStaticMeshDrawInfos, FColor::Emerald);

		int8 MinLODIndex, MaxLODIndex;
		PrimitiveSceneInfo.GetStaticMeshesLODRange(MinLODIndex, MaxLODIndex);
		for (int32 StaticMeshIdx = 0; StaticMeshIdx < PrimitiveSceneInfo.StaticMeshes.Num(); StaticMeshIdx++)
		{
			const FStaticMeshBatch& StaticMesh = PrimitiveSceneInfo.StaticMeshes[StaticMeshIdx];

			// Auto select max lod for rendering static mesh.
			if (StaticMesh.LODIndex == MaxLODIndex && StaticMesh.bUseForMaterial)
			{
				const FMeshBatchElement& BatchElement = StaticMesh.Elements[0];

				if (MultiPassFlags.bEnablePass0)
				{
					FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
					MeshDrawInfo.Init(*View, PrimitiveSceneInfo.Proxy, StaticMesh, BatchElement);
					MeshDrawInfo.SetupPSO(*PassContext);
					FinalizeMeshDrawInfo(MeshDrawInfo, 0);
				}
				if (MultiPassFlags.bEnablePass1)
				{
					FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
					MeshDrawInfo.Init(*View, PrimitiveSceneInfo.Proxy, StaticMesh, BatchElement);
					MeshDrawInfo.SetupPSO(*PassContext);
					FinalizeMeshDrawInfo(MeshDrawInfo, 1);
				}
				if (MultiPassFlags.bEnablePass2)
				{
					FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
					MeshDrawInfo.Init(*View, PrimitiveSceneInfo.Proxy, StaticMesh, BatchElement);
					MeshDrawInfo.SetupPSO(*PassContext);
					FinalizeMeshDrawInfo(MeshDrawInfo, 2);
				}
				if (MultiPassFlags.bEnablePass3)
				{
					FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
					MeshDrawInfo.Init(*View, PrimitiveSceneInfo.Proxy, StaticMesh, BatchElement);
					MeshDrawInfo.SetupPSO(*PassContext);
					FinalizeMeshDrawInfo(MeshDrawInfo, 3);
				}
			}
		}
	}

	void SetupDynamicMeshDrawInfos(const FPrimitiveSceneInfo& PrimitiveSceneInfo, const FMultiPassFlags& MultiPassFlags)
	{
		SCOPED_NAMED_EVENT(SetupDynamicMeshDrawInfos, FColor::Emerald);

		const FInt32Range MeshBatchRange = GetDynamicMeshElementRange(*View, PrimitiveSceneInfo.GetIndex());

		for (int32 MeshBatchIndex = MeshBatchRange.GetLowerBoundValue(); MeshBatchIndex < MeshBatchRange.GetUpperBoundValue(); ++MeshBatchIndex)
		{
			const FMeshBatchAndRelevance& MeshAndRelevance = View->DynamicMeshElements[MeshBatchIndex];
			const FMeshBatch& MeshBatch = *MeshAndRelevance.Mesh;
			const FMeshBatchElement& BatchElement = MeshBatch.Elements[0];

			if (MultiPassFlags.bEnablePass0)
			{
				FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
				MeshDrawInfo.Init(*View, PrimitiveSceneInfo.Proxy, MeshBatch, BatchElement);
				MeshDrawInfo.SetupPSO(*PassContext);
				FinalizeMeshDrawInfo(MeshDrawInfo, 0);
			}
			if (MultiPassFlags.bEnablePass1)
			{
				FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
				MeshDrawInfo.Init(*View, PrimitiveSceneInfo.Proxy, MeshBatch, BatchElement);
				MeshDrawInfo.SetupPSO(*PassContext);
				FinalizeMeshDrawInfo(MeshDrawInfo, 1);
			}
			if (MultiPassFlags.bEnablePass2)
			{
				FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
				MeshDrawInfo.Init(*View, PrimitiveSceneInfo.Proxy, MeshBatch, BatchElement);
				MeshDrawInfo.SetupPSO(*PassContext);
				FinalizeMeshDrawInfo(MeshDrawInfo, 2);
			}
			if (MultiPassFlags.bEnablePass3)
			{
				FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
				MeshDrawInfo.Init(*View, PrimitiveSceneInfo.Proxy, MeshBatch, BatchElement);
				MeshDrawInfo.SetupPSO(*PassContext);
				FinalizeMeshDrawInfo(MeshDrawInfo, 3);
			}
		}
	}

	void SetupNaniteFallbackMeshDrawInfos(const FPassContext::FNaniteFallbackRenderInfo& RenderInfo, const FStaticMeshSceneProxy& Proxy)
	{
		SCOPED_NAMED_EVENT(SetupNaniteFallbackMeshDrawInfos, FColor::Emerald);

		for (int32 SectionIndex = 0; SectionIndex < RenderInfo.NumSections; SectionIndex++)
		{
			FMeshBatch& MeshBatch = AllocNaniteFallbackMeshBatch();
			Proxy.GetMeshElement(RenderInfo.LODIndex, Proxy.GetNumMeshBatches(), SectionIndex, SDPG_World, false, true, MeshBatch);

			const FMeshBatchElement& BatchElement = MeshBatch.Elements[0];

			if (RenderInfo.MultiPassFlags.bEnablePass0)
			{
				FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
				MeshDrawInfo.Init(*View, RenderInfo.SourceProxy, MeshBatch, BatchElement);
				MeshDrawInfo.SetupPSO(*PassContext);
				FinalizeMeshDrawInfo(MeshDrawInfo, 0, true);
			}
			if (RenderInfo.MultiPassFlags.bEnablePass1)
			{
				FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
				MeshDrawInfo.Init(*View, RenderInfo.SourceProxy, MeshBatch, BatchElement);
				MeshDrawInfo.SetupPSO(*PassContext);
				FinalizeMeshDrawInfo(MeshDrawInfo, 1, true);
			}
			if (RenderInfo.MultiPassFlags.bEnablePass2)
			{
				FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
				MeshDrawInfo.Init(*View, RenderInfo.SourceProxy, MeshBatch, BatchElement);
				MeshDrawInfo.SetupPSO(*PassContext);
				FinalizeMeshDrawInfo(MeshDrawInfo, 2, true);
			}
			if (RenderInfo.MultiPassFlags.bEnablePass3)
			{
				FMeshDrawInfo& MeshDrawInfo = AllocMeshDrawInfo();
				MeshDrawInfo.Init(*View, RenderInfo.SourceProxy, MeshBatch, BatchElement);
				MeshDrawInfo.SetupPSO(*PassContext);
				FinalizeMeshDrawInfo(MeshDrawInfo, 3, true);
			}
		}
	}

	void AsyncSetupMeshDrawInfos()
	{
		// Gather MeshBatch from Scene->Primitives.
		SCOPED_NAMED_EVENT(GatherMeshBatch, FColor::Emerald);

		{
			SCOPED_NAMED_EVENT(GatherVisiblePrimitiveSceneInfos, FColor::Emerald);

			ParallelFor(PassContext->RenderObjects.Num(), [&](int32 Index)
				{
					int32 PrimitiveIndex = Scene->PrimitiveComponentIds.Find(PassContext->RenderObjects[Index].RenderObjectID);

					if (PrimitiveIndex == INDEX_NONE)
					{
						VisiblePrimitiveIndices[Index] = INDEX_NONE;
						VisibleMultiPassFlags[Index] = FMultiPassFlags(EForceInit::ForceInitToZero);
						return;
					}				

					const FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[PrimitiveIndex];

					if (View->PrimitiveVisibilityMap[PrimitiveSceneInfo->GetIndex()] || PassContext->bUseManualCulling)
					{
						FBoxSphereBounds CurrentBound = PrimitiveSceneInfo->Proxy->GetBounds();

						bool bIsVisible = true;

						if (PassContext->bUseManualCulling)
						{
							FConvexVolume ViewFrustum;
							GetViewFrustumBounds(ViewFrustum, PassContext->CustomViewProj, false);
							ViewFrustum = PassContext->bUseCustomViewProj ? ViewFrustum : View->ViewFrustum;

							FVector ViewOrigin = PassContext->bUseCustomViewProj ? PassContext->CustomViewProj.GetOrigin() : View->ViewMatrices.GetViewOrigin();

							if (!ViewFrustum.IntersectBox(CurrentBound.Origin, CurrentBound.BoxExtent))
								bIsVisible = false; // Outside view frustum.

							// Inside view frustum, processing distance cull.
							if (bIsVisible)
							{
								const FPrimitiveBounds& RESTRICT Bounds = Scene->PrimitiveBounds[PrimitiveIndex];

								// Preserve infinite draw distance
								bool bHasMaxDrawDistance = Bounds.MaxCullDistance < FLT_MAX;
								bool bHasMinDrawDistance = Bounds.MinDrawDistance > 0;

								if (bHasMaxDrawDistance || bHasMinDrawDistance)
								{
									float MaxDrawDistance = Bounds.MaxCullDistance * PassContext->MaxDrawDistanceScale;
									float MinDrawDistanceSq = FMath::Square(Bounds.MinDrawDistance);
									float DistanceSquared = FVector::DistSquared(Bounds.BoxSphereBounds.Origin, ViewOrigin);

									// Check for distance culling first
									const bool bFarDistanceCulled = bHasMaxDrawDistance && (DistanceSquared > FMath::Square(MaxDrawDistance));
									const bool bNearDistanceCulled = bHasMinDrawDistance && (DistanceSquared < MinDrawDistanceSq);
									bool bIsDistanceCulled = bNearDistanceCulled || bFarDistanceCulled;

									if (bIsDistanceCulled)
									{
										bIsVisible = false;
									}
								}
							}
						}

						if (!bIsVisible)
						{
							VisiblePrimitiveIndices[Index] = INDEX_NONE;
							VisibleMultiPassFlags[Index] = FMultiPassFlags(EForceInit::ForceInitToZero);
							return;
						}

						VisiblePrimitiveIndices[Index] = PrimitiveIndex;
						VisibleMultiPassFlags[Index] = PassContext->RenderObjects[Index].MultiPassFlags;
					}
					else
					{
						VisiblePrimitiveIndices[Index] = INDEX_NONE;
						VisibleMultiPassFlags[Index] = FMultiPassFlags(EForceInit::ForceInitToZero);
					}

				}, EParallelForFlags::Unbalanced);

			// Nanite fallback.
			ParallelFor(PassContext->NaniteFallbackRenderInfos.Num(), [&](int32 Index)
				{
					int32 PrimitiveIndex = Scene->PrimitiveComponentIds.Find(PassContext->NaniteFallbackRenderInfos[Index].RenderObjectID);

					if (PrimitiveIndex == INDEX_NONE)
					{
						PassContext->NaniteFallbackRenderInfos[Index].PrimitiveIndex = INDEX_NONE;
						VisibleNaniteFallbackRenderInfos[Index] = nullptr;
						return;
					}

					const FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[PrimitiveIndex];

					if (View->PrimitiveVisibilityMap[PrimitiveSceneInfo->GetIndex()] || PassContext->bUseManualCulling)
					{
						FBoxSphereBounds CurrentBound = PrimitiveSceneInfo->Proxy->GetBounds();

						bool bIsVisible = true;

						if (PassContext->bUseManualCulling)
						{
							FConvexVolume ViewFrustum;
							GetViewFrustumBounds(ViewFrustum, PassContext->CustomViewProj, false);
							ViewFrustum = PassContext->bUseCustomViewProj ? ViewFrustum : View->ViewFrustum;

							FVector ViewOrigin = PassContext->bUseCustomViewProj ? PassContext->CustomViewProj.GetOrigin() : View->ViewMatrices.GetViewOrigin();

							if (!ViewFrustum.IntersectBox(CurrentBound.Origin, CurrentBound.BoxExtent))
								bIsVisible = false; // Outside view frustum.

							// Inside view frustum, processing distance cull.
							if (bIsVisible)
							{
								const FPrimitiveBounds& RESTRICT Bounds = Scene->PrimitiveBounds[PrimitiveIndex];

								// Preserve infinite draw distance
								bool bHasMaxDrawDistance = Bounds.MaxCullDistance < FLT_MAX;
								bool bHasMinDrawDistance = Bounds.MinDrawDistance > 0;

								if (bHasMaxDrawDistance || bHasMinDrawDistance)
								{
									float MaxDrawDistance = Bounds.MaxCullDistance * PassContext->MaxDrawDistanceScale;
									float MinDrawDistanceSq = FMath::Square(Bounds.MinDrawDistance);
									float DistanceSquared = FVector::DistSquared(Bounds.BoxSphereBounds.Origin, ViewOrigin);

									// Check for distance culling first
									const bool bFarDistanceCulled = bHasMaxDrawDistance && (DistanceSquared > FMath::Square(MaxDrawDistance));
									const bool bNearDistanceCulled = bHasMinDrawDistance && (DistanceSquared < MinDrawDistanceSq);
									bool bIsDistanceCulled = bNearDistanceCulled || bFarDistanceCulled;

									if (bIsDistanceCulled)
									{
										bIsVisible = false;
									}
								}
							}
						}

						if (!bIsVisible)
						{
							PassContext->NaniteFallbackRenderInfos[Index].PrimitiveIndex = INDEX_NONE;
							VisibleNaniteFallbackRenderInfos[Index] = nullptr;
							return;
						}

						PassContext->NaniteFallbackRenderInfos[Index].PrimitiveIndex = PrimitiveIndex;
						VisibleNaniteFallbackRenderInfos[Index] = &PassContext->NaniteFallbackRenderInfos[Index];
					}
					else
					{
						VisibleNaniteFallbackRenderInfos[Index] = nullptr;
					}

				}, EParallelForFlags::Unbalanced);
		}

		{
			SCOPED_NAMED_EVENT(SetupMeshDrawInfos, FColor::Emerald);

			for (int32 Index = 0; Index < PassContext->RenderObjects.Num(); Index++)
			{
				if (!Scene->Primitives.IsValidIndex(VisiblePrimitiveIndices[Index]))
					continue;

				const FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[VisiblePrimitiveIndices[Index]];
				if (PrimitiveSceneInfo == nullptr)
					continue;

				if (PrimitiveSceneInfo->StaticMeshes.Num() > 0)
				{
					SetupStaticMeshDrawInfos(*PrimitiveSceneInfo, VisibleMultiPassFlags[Index]);
				}
				else
				{
					SetupDynamicMeshDrawInfos(*PrimitiveSceneInfo, VisibleMultiPassFlags[Index]);
				}
			}
		}

		{
			SCOPED_NAMED_EVENT(SetupNaniteFallback, FColor::Emerald);

			// Render nanite fallback or nanite screen quad.
			if (PassContext->IsUseNaniteFallback())
			{
				for (auto& Proxy : *NaniteFallbackProxies)
				{
					for (int32 Index = 0; Index < PassContext->NaniteFallbackRenderInfos.Num(); Index++)
					{
						FPassContext::FNaniteFallbackRenderInfo& RenderInfo = *VisibleNaniteFallbackRenderInfos[Index];

						if (RenderInfo.PrimitiveIndex == -1)
							continue;

						RenderInfo.SourceProxy = Scene->Primitives[RenderInfo.PrimitiveIndex]->Proxy;

						if (RenderInfo.RenderObjectID == Proxy->GetPrimitiveComponentId())
						{
							SetupNaniteFallbackMeshDrawInfos(RenderInfo, *Proxy);
						}
					}
				}
			}
		}
	}

public:

	void SetupMeshDrawInfos(bool bAsync = true)
	{
		if (bAsync)
		{
			bAsyncFlag = true;
			MeshDrawInfoSetupAsyncTask->StartBackgroundTask();
		}
		else
		{
			AsyncSetupMeshDrawInfos();
		}
	}

	void SubmitDraw(FRHICommandList& RHICmdList)
	{
		if (bAsyncFlag)
		{
			SCOPED_NAMED_EVENT(WaitForMeshDrawInfoSetupAsyncTask, FColor::Emerald);
			MeshDrawInfoSetupAsyncTask->EnsureCompletion();
		}

		for (int32 Index = 0; Index < AllocatedMeshDrawInfoCount; Index++)
		{
			FMeshDrawInfo& MeshDrawInfo = DrawList[Index];
			if (!MeshDrawInfo.bHasValidShader)
				continue;

			RHICmdList.ApplyCachedRenderTargets(MeshDrawInfo.GraphicsPSOInit);

			SetGraphicsPipelineState(RHICmdList, MeshDrawInfo.GraphicsPSOInit, 0);

			// TODO: FMeshDrawCommandStateCache is roughly used here, make something better...
			FMeshDrawCommandStateCache StateCache;
			MeshDrawInfo.ShaderBindings.SetOnCommandList(RHICmdList, MeshDrawInfo.GraphicsPSOInit.BoundShaderState, StateCache.ShaderBindings);

			for (auto& Stream : MeshDrawInfo.VertexInputStreamArray)
			{
				RHICmdList.SetStreamSource(Stream.StreamIndex, Stream.VertexBuffer, Stream.Offset);
			}

			RHICmdList.SetStencilRef(MeshDrawInfo.PassContext->StencilReference > 0 ? MeshDrawInfo.PassContext->StencilReference : MeshDrawInfo.Proxy->GetCustomDepthStencilValue());
			
			// TODO: Wireframe crash...
			RHICmdList.DrawIndexedPrimitive(
				MeshDrawInfo.IndexBuffer,
				MeshDrawInfo.BaseVertexIndex,
				0,
				MeshDrawInfo.NumVertices,
				MeshDrawInfo.FirstIndex,
				MeshDrawInfo.NumPrimitives,
				MeshDrawInfo.NumInstances);
		}
	}
};

FUnrealURPRenderer::FUnrealURPRenderer(const FAutoRegister& AutoReg, UWorld* InWorld)
	: FWorldSceneViewExtension(AutoReg, InWorld)
	, WorldType(InWorld->WorldType)
{
	PrivatePreRenderingHandle = GEngine->GetPreRenderDelegateEx().AddRaw(this, &FUnrealURPRenderer::PreRendering_RenderThread);

	PrivatePreTranslucencyPassRenderHandle = GetRendererModule().RegisterPostOpaqueRenderDelegate(
		FPostOpaqueRenderDelegate::CreateRaw(this, &FUnrealURPRenderer::PreTranslucencyPass_RenderThread));
}

void FUnrealURPRenderer::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	ViewsForPreRendering.Add(&InView);
}

void FUnrealURPRenderer::PreRendering_RenderThread(FRDGBuilder& GraphBuilder)
{
	if (ViewsForPreRendering.IsEmpty())
		return;

	for (auto& View : ViewsForPreRendering)
	{
		RedirectRenderPassByInjectionPoint(GraphBuilder, *View, (uint8)EPassInjectionPoint::BeforeRendering);
	}

	ViewsForPreRendering.Empty();
}

#if ENGINE_MODIFY
void FUnrealURPRenderer::PostPrePass_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& View)
{
	RedirectRenderPassByInjectionPoint(GraphBuilder, View, (uint8)EPassInjectionPoint::AfterPrePass);
}
#endif

void FUnrealURPRenderer::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& View, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	RedirectRenderPassByInjectionPoint(GraphBuilder, View, (uint8)EPassInjectionPoint::AfterOpaqueBasePass);
}

void FUnrealURPRenderer::PreTranslucencyPass_RenderThread(FPostOpaqueRenderParameters& InParameters)
{
	FRDGBuilder& GraphBuilder = *InParameters.GraphBuilder;
	const FViewInfo& View = *InParameters.View;

	RedirectRenderPassByInjectionPoint(GraphBuilder, View, (uint8)EPassInjectionPoint::BeforeTransparent);
}

void FUnrealURPRenderer::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
	RedirectRenderPassByInjectionPoint(GraphBuilder, View, (uint8)EPassInjectionPoint::BeforePostProcess);
}

void FUnrealURPRenderer::SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (Pass == EPostProcessingPass::SSRInput)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FUnrealURPRenderer::PostProcessPassAfterSSRInput_RenderThread));
	}

	if (Pass == EPostProcessingPass::MotionBlur)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FUnrealURPRenderer::PostProcessPassAfterMotionBlur_RenderThread));
	}

	if (Pass == EPostProcessingPass::Tonemap)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FUnrealURPRenderer::PostProcessPassAfterTonemap_RenderThread));
	}

	if (Pass == EPostProcessingPass::FXAA)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FUnrealURPRenderer::PostProcessPassAfterFXAA_RenderThread));
	}
}

FScreenPassTexture FUnrealURPRenderer::ReturnUntouchedSceneColorForPostProcessing(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& InOutInputs)
{
#if UE_VERSION >= UE5_4_3
	return InOutInputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
#else
	const FViewInfo& View = static_cast<const FViewInfo&>(InView);
	const FScreenPassTexture SceneColor = InOutInputs.GetInput(EPostProcessMaterialInput::SceneColor);
	FScreenPassRenderTarget Output = InOutInputs.OverrideOutput;

	// When skipping the pass, we still need to output a valid FScreenPassRenderTarget
	Output = FScreenPassRenderTarget(SceneColor, ERenderTargetLoadAction::ENoAction);

	// If there is override output, we need to output to that
	if (InOutInputs.OverrideOutput.IsValid())
	{
		const FScreenPassTextureViewport OutputViewport(Output);
		const FIntPoint SrcPoint = View.ViewRect.Min;
		const FIntPoint DstPoint = OutputViewport.Rect.Min;
		const FIntPoint Size = OutputViewport.Rect.Max - DstPoint;
		AddDrawTexturePass(GraphBuilder, View, Output.Texture, InOutInputs.OverrideOutput.Texture, SrcPoint, DstPoint, Size);
		Output = InOutInputs.OverrideOutput;
	}

	return MoveTemp(Output);
#endif
}

FScreenPassTexture FUnrealURPRenderer::PostProcessPassAfterSSRInput_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
{
	RedirectRenderPassByInjectionPoint(GraphBuilder, View, (uint8)EPassInjectionPoint::AfterSSRInput);

	return ReturnUntouchedSceneColorForPostProcessing(GraphBuilder, View, InOutInputs);
}

FScreenPassTexture FUnrealURPRenderer::PostProcessPassAfterMotionBlur_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
{
	RedirectRenderPassByInjectionPoint(GraphBuilder, View, (uint8)EPassInjectionPoint::AfterMotionBlur);

	return ReturnUntouchedSceneColorForPostProcessing(GraphBuilder, View, InOutInputs);
}

FScreenPassTexture FUnrealURPRenderer::PostProcessPassAfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
{
	RedirectRenderPassByInjectionPoint(GraphBuilder, View, (uint8)EPassInjectionPoint::AfterTonemap);

	return ReturnUntouchedSceneColorForPostProcessing(GraphBuilder, View, InOutInputs);
}

FScreenPassTexture FUnrealURPRenderer::PostProcessPassAfterFXAA_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
{
	RedirectRenderPassByInjectionPoint(GraphBuilder, View, (uint8)EPassInjectionPoint::AfterFXAA);

	return ReturnUntouchedSceneColorForPostProcessing(GraphBuilder, View, InOutInputs);
}

void FUnrealURPRenderer::AddPassContext_GameThread(FPassContext* InPassContext)
{
	check(IsInGameThread());

	if (InPassContext == nullptr)
		return;

	FNaniteFallbackPerPass NaniteFallbackPerPass;
	NaniteFallbackPerPass.PassName = InPassContext->PassName;

	NaniteFallbackPerPassArray.AddUnique(NaniteFallbackPerPass);

	InPassContext->NaniteFallbackStaticMeshComps.Empty();

	ENQUEUE_RENDER_COMMAND(AddPassContextCmd)(
		[this, InPassContext](FRHICommandListImmediate& RHICmdList)
		{
			FPassContext* PendingDelete = nullptr;

			for (FPassContext* Context : PassContexts)
			{
				if (*Context == *InPassContext)
				{
					PendingDelete = Context;
					break;
				}
			}

			if (PendingDelete != nullptr)
			{
				PassContexts.Remove(PendingDelete);
				delete PendingDelete;
			}

			PassContexts.Add(InPassContext);

			SortPass();
		});
}

void FUnrealURPRenderer::RemovePassContext_GameThread(const FName& InPassName)
{
	check(IsInGameThread());

	FPrimitiveComponentId PendingDeleteNaniteFallbackProxyCompID;

	int32 FoundIndex = -1;
	if (NaniteFallbackPerPassArray.Find(InPassName, FoundIndex))
	{
		for (auto& CompID : NaniteFallbackPerPassArray[FoundIndex].NaniteFallbackRenderObjectIDs)
		{
			if (NaniteFallbacks.Find(CompID, FoundIndex))
			{
				NaniteFallbacks[FoundIndex].ReferenceCount--;

				if (NaniteFallbacks[FoundIndex].ReferenceCount == 0)
				{
					PendingDeleteNaniteFallbackProxyCompID = CompID;
				}			
			}
		}

		NaniteFallbackPerPassArray.Remove(InPassName);
	}

	if (PendingDeleteNaniteFallbackProxyCompID.IsValid())
	{
		NaniteFallbacks.Remove(PendingDeleteNaniteFallbackProxyCompID);
	}

	ENQUEUE_RENDER_COMMAND(RemovePassContextCmd)(
		[this, InPassName, PendingDeleteNaniteFallbackProxyCompID](FRHICommandListImmediate& RHICmdList)
		{
			FStaticMeshSceneProxy* PendingDeleteNaniteFallbackProxy = nullptr;
			if (PendingDeleteNaniteFallbackProxyCompID.IsValid())
			{
				for (auto& Proxy : NaniteFallbackProxies)
				{
					if (Proxy->GetPrimitiveComponentId() == PendingDeleteNaniteFallbackProxyCompID)
					{
						PendingDeleteNaniteFallbackProxy = Proxy;
					}
				}
			}

			if (PendingDeleteNaniteFallbackProxy != nullptr)
			{
				NaniteFallbackProxies.Remove(PendingDeleteNaniteFallbackProxy);
				delete PendingDeleteNaniteFallbackProxy;
			}

			FPassContext* PendingDelete = nullptr;

			for (FPassContext* Context : PassContexts)
			{
				if (Context->PassName == InPassName)
				{
					PendingDelete = Context;
					break;
				}
			}

			if (PendingDelete != nullptr)
			{
				PassContexts.Remove(PendingDelete);
				delete PendingDelete;

				SortPass();
			}			
		});
}

void FUnrealURPRenderer::DeletePassContexts_GameThread()
{
	check(IsInGameThread());

	ENQUEUE_RENDER_COMMAND(DeletePassContextsCmd)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			for (FPassContext* Context : PassContexts)
			{
				if (Context != nullptr)
					delete Context;
			}

			PassContexts.Empty();
		});
}

void FUnrealURPRenderer::Deinitialize()
{
	GetRendererModule().RemovePostOpaqueRenderDelegate(PrivatePreTranslucencyPassRenderHandle);
	GEngine->GetPreRenderDelegateEx().Remove(PrivatePreRenderingHandle);
}

void FUnrealURPRenderer::AddRenderObjectToPass_GameThread(const UPrimitiveComponent* InPrimitiveComp, const FMultiPassFlags& InMultiPassFlags, const FName& InPassName)
{
	check(IsInGameThread());

	FPassContext::FRenderableObject RenderObject;
	RenderObject.RenderObjectID = Get_PrimCompId(InPrimitiveComp);
	RenderObject.MultiPassFlags = InMultiPassFlags;

	if (const UStaticMeshComponent* StaticComp = Cast<const UStaticMeshComponent>(InPrimitiveComp))
	{
		if (StaticComp->GetStaticMesh())
		{
			RenderObject.MaxDrawInfo = StaticComp->GetStaticMesh()->GetNumSections(StaticComp->GetStaticMesh()->GetNumLODs() - 1);
		}
	}

	ENQUEUE_RENDER_COMMAND(AddRenderObejectToPassCmd)(
		[this, InPassName, RenderObject](FRHICommandListImmediate& RHICmdList)
		{
			for (FPassContext* Context : PassContexts)
			{
				if (Context->PassName == InPassName)
				{
					Context->RenderObjects.Add(RenderObject);
					break;
				}
			}
		});
}

void FUnrealURPRenderer::AddNaniteFallbackToPass_GameThread(UStaticMeshComponent* InNaniteFallbackComp, const FMultiPassFlags& InMultiPassFlags, const FName& InPassName)
{
	check(IsInGameThread());

	if (InNaniteFallbackComp->GetStaticMesh() == nullptr)
		return;

	FStaticMeshSceneProxy* NewNaniteFallbackProxy = nullptr;
	int32 FoundIndex = -1;

	if (NaniteFallbacks.Find(Get_PrimCompId(InNaniteFallbackComp), FoundIndex))
	{
		NaniteFallbacks[FoundIndex].ReferenceCount++;
	}
	else
	{
		NewNaniteFallbackProxy = new FStaticMeshSceneProxy(InNaniteFallbackComp, false);
		NaniteFallbacks.Add(Get_PrimCompId(InNaniteFallbackComp));
	}

	// Assume NaniteFallbackPerPassArray is not empty.
	for (auto& NaniteFallbackPerPass : NaniteFallbackPerPassArray)
	{
		if (NaniteFallbackPerPass.PassName == InPassName)
		{
			NaniteFallbackPerPass.NaniteFallbackRenderObjectIDs.Add(Get_PrimCompId(InNaniteFallbackComp));
		}
	}

	FPassContext::FNaniteFallbackRenderInfo NaniteFallbackRenderInfo;
	NaniteFallbackRenderInfo.LODIndex = InNaniteFallbackComp->GetStaticMesh()->GetNumLODs() - 1;
	NaniteFallbackRenderInfo.NumSections = InNaniteFallbackComp->GetStaticMesh()->GetNumSections(NaniteFallbackRenderInfo.LODIndex);
	NaniteFallbackRenderInfo.RenderObjectID = Get_PrimCompId(InNaniteFallbackComp);
	NaniteFallbackRenderInfo.MultiPassFlags = InMultiPassFlags;

	ENQUEUE_RENDER_COMMAND(AddNaniteFallbackToPassCmd)(
		[this, InPassName, NewNaniteFallbackProxy, NaniteFallbackRenderInfo](FRHICommandListImmediate& RHICmdList)
		{
			if (NewNaniteFallbackProxy != nullptr)
			{
				NaniteFallbackProxies.Add(NewNaniteFallbackProxy);
			}

			for (FPassContext* Context : PassContexts)
			{
				if (Context->PassName == InPassName)
				{
					Context->NaniteFallbackRenderInfos.Add(NaniteFallbackRenderInfo);
					break;
				}
			}
		});
}

void FUnrealURPRenderer::RemoveRenderObjectFromPass_GameThread(const FPrimitiveComponentId& InRenderObjectID, const FName& InPassName)
{
	check(IsInGameThread());

	bool bIsNaniteFallback = false;
	bool bNeedDeleteNaniteFallbackProxy = false;

	int32 FoundIndex = -1;
	if (NaniteFallbackPerPassArray.Find(InPassName, FoundIndex))
	{
		NaniteFallbackPerPassArray[FoundIndex].NaniteFallbackRenderObjectIDs.Remove(InRenderObjectID);
	}

	if (NaniteFallbacks.Find(InRenderObjectID, FoundIndex))
	{
		NaniteFallbacks[FoundIndex].ReferenceCount--;

		if (NaniteFallbacks[FoundIndex].ReferenceCount == 0)
		{
			bNeedDeleteNaniteFallbackProxy = true;
		}

		bIsNaniteFallback = true;
	}

	if (bNeedDeleteNaniteFallbackProxy)
	{
		NaniteFallbacks.Remove(InRenderObjectID);
	}

	ENQUEUE_RENDER_COMMAND(RemoveRenderObjectFromPassCmd)(
		[this, InPassName, InRenderObjectID, bIsNaniteFallback, bNeedDeleteNaniteFallbackProxy](FRHICommandListImmediate& RHICmdList)
		{
			if (bIsNaniteFallback)
			{
				if (bNeedDeleteNaniteFallbackProxy)
				{
					FStaticMeshSceneProxy* PendingDeleteNaniteFallbackProxy = nullptr;
					for (auto& Proxy : NaniteFallbackProxies)
					{
						if (Proxy->GetPrimitiveComponentId() == InRenderObjectID)
						{
							PendingDeleteNaniteFallbackProxy = Proxy;
						}
					}

					if (PendingDeleteNaniteFallbackProxy != nullptr)
					{
						NaniteFallbackProxies.Remove(PendingDeleteNaniteFallbackProxy);
						delete PendingDeleteNaniteFallbackProxy;
					}
				}
				
				for (FPassContext* Context : PassContexts)
				{
					if (Context->PassName == InPassName)
					{
						Context->NaniteFallbackRenderInfos.Remove(InRenderObjectID);
						break;
					}
				}
			}
			else
			{
				for (FPassContext* Context : PassContexts)
				{
					if (Context->PassName == InPassName)
					{
						Context->RenderObjects.Remove(InRenderObjectID);
						break;
					}
				}
			}
		});
}

void FUnrealURPRenderer::SuspendPass_GameThread(const FName& InPassName)
{
	check(IsInGameThread());

	ENQUEUE_RENDER_COMMAND(SuspendPassCmd)(
		[this, InPassName](FRHICommandListImmediate& RHICmdList)
		{
			for (FPassContext* Context : PassContexts)
			{
				if (Context->PassName == InPassName)
				{
					Context->bSuspend = true;
					break;
				}
			}
		});
}

void FUnrealURPRenderer::ResumePass_GameThread(const FName& InPassName)
{
	check(IsInGameThread());

	ENQUEUE_RENDER_COMMAND(ResumePassCmd)(
		[this, InPassName](FRHICommandListImmediate& RHICmdList)
		{
			for (FPassContext* Context : PassContexts)
			{
				if (Context->PassName == InPassName)
				{
					Context->bSuspend = false;
					break;
				}
			}
		});
}

void FUnrealURPRenderer::UpdatePassPriority_GameThread(const FName& InPassName, int32 InPassPriority)
{
	check(IsInGameThread());

	ENQUEUE_RENDER_COMMAND(UpdatePassPriorityCmd)(
		[this, InPassName, InPassPriority](FRHICommandListImmediate& RHICmdList)
		{
			for (FPassContext* Context : PassContexts)
			{
				if (Context->PassName == InPassName)
				{
					Context->PassPriority = InPassPriority;
					break;
				}
			}
		});
}

void FUnrealURPRenderer::UpdatePassCamera_GameThread(FName InPassName, const FMatrix& InCustomViewProj)
{
	check(IsInGameThread());

	ENQUEUE_RENDER_COMMAND(UpdatePassCameraCmd)(
		[this, InPassName, InCustomViewProj](FRHICommandListImmediate& RHICmdList)
		{
			for (FPassContext* Context : PassContexts)
			{
				if (Context->PassName == InPassName)
				{
					Context->CustomViewProj = InCustomViewProj;
					Context->bUseCustomViewProj = true;
					break;
				}
			}
		});
}

void FUnrealURPRenderer::RedirectRenderPassByInjectionPoint(FRDGBuilder& GraphBuilder, const FSceneView& InView, const uint8& InjectionPoint)
{
	const FViewInfo& View = static_cast<const FViewInfo&>(InView);

	FScene* Scene = nullptr;
	FSceneTextures* RDGSceneTextures = nullptr;

	if (View.Family->Scene)
	{
		Scene = View.Family->Scene->GetRenderScene();
		RDGSceneTextures = ((FViewFamilyInfo*)View.Family)->GetSceneTexturesChecked();
	}

	if (Scene == nullptr || RDGSceneTextures == nullptr)
		return;

	if (Scene->GetWorld()->WorldType != WorldType)
		return;

	for (auto& PassContext : PassContexts)
	{
		PassContext->AdvanceFrame((EPassInjectionPoint)InjectionPoint, [&]()
			{
				RenderInternal(GraphBuilder, Scene, View, *RDGSceneTextures, *PassContext);
			});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Dummy Nanite Parameters.

//FNaniteUniformParameters* GetNaniteUniformParameters(FRDGBuilder& GraphBuilder, const Nanite::FRasterResults* RasterResults)
//{
//	const FRDGSystemTextures& SystemTextures = FRDGSystemTextures::Get(GraphBuilder);
//
//	FNaniteUniformParameters* UniformParameters = GraphBuilder.AllocParameters<FNaniteUniformParameters>();
//	UniformParameters->PageConstants = FIntVector4(ForceInit);
//	UniformParameters->MaterialConfig = FIntVector4(ForceInit);
//	UniformParameters->MaxNodes = 0;
//	UniformParameters->MaxVisibleClusters = 0;
//	UniformParameters->RenderFlags = 0;
//	UniformParameters->RayTracingCutError = 0.0f;
//	UniformParameters->RectScaleOffset - FVector4f(ForceInit);
//
//	UniformParameters->ClusterPageData = Nanite::GStreamingManager.GetClusterPageDataSRV(GraphBuilder);
//	UniformParameters->VisibleClustersSWHW = GraphBuilder.CreateSRV(GSystemTextures.GetDefaultStructuredBuffer<uint32>(GraphBuilder));
//	UniformParameters->HierarchyBuffer = Nanite::GStreamingManager.GetHierarchySRV(GraphBuilder);
//	UniformParameters->MaterialTileRemap = GraphBuilder.CreateSRV(GSystemTextures.GetDefaultStructuredBuffer<uint32>(GraphBuilder), PF_R32_UINT);
//	UniformParameters->VisBuffer64 = SystemTextures.Black;
//	UniformParameters->DbgBuffer64 = SystemTextures.Black;
//	UniformParameters->DbgBuffer32 = SystemTextures.Black;
//	
//	UniformParameters->RayTracingDataBuffer = GraphBuilder.CreateSRV(GSystemTextures.GetDefaultStructuredBuffer<uint32>(GraphBuilder));
//
//	UniformParameters->MultiViewEnabled = 0;
//	UniformParameters->MultiViewIndices = GraphBuilder.CreateSRV(GSystemTextures.GetDefaultStructuredBuffer<uint32>(GraphBuilder));
//	UniformParameters->MultiViewRectScaleOffsets = GraphBuilder.CreateSRV(GSystemTextures.GetDefaultStructuredBuffer<FVector4>(GraphBuilder));
//	UniformParameters->InViews = GraphBuilder.CreateSRV(GSystemTextures.GetDefaultStructuredBuffer<FPackedNaniteView>(GraphBuilder));
//
//	if (RasterResults != nullptr)
//	{
//		UniformParameters->VisibleClustersSWHW = GraphBuilder.CreateSRV(RasterResults->VisibleClustersSWHW);
//	}
//
//	return UniformParameters;
//}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FUnrealURPRenderer::RenderInternal(FRDGBuilder& GraphBuilder, FScene* Scene, const FViewInfo& View, FSceneTextures& SceneTextures, FPassContext& PassContext)
{
	SCOPED_NAMED_EVENT_F(TEXT("CustomMeshPass_%s_RenderInternal"), FColor::Emerald, *PassContext.PassName.ToString());
	SCOPE_CYCLE_COUNTER(STAT_CustomMeshPass);
	RDG_EVENT_SCOPE(GraphBuilder, "CustomMeshPass");
	RDG_GPU_STAT_SCOPE(GraphBuilder, CustomMeshPass);

	TArray<FRDGTextureRef, TInlineAllocator<MAX_RT>> RenderTargets;
	FRDGTextureRef DepthTarget = nullptr;
	FRDGTextureRef CustomDepthTarget = nullptr;

	const FSceneTexturesConfig& SceneTexturesConfig = FSceneTexturesConfig::Get();

	// Depth stencil target.
	if (PassContext.BuiltInDepthStencilTarget != EBuiltInDepthStencilTarget::None)
	{
		if (PassContext.BuiltInDepthStencilTarget == EBuiltInDepthStencilTarget::SceneDepthStencil)
		{
			DepthTarget = SceneTextures.Depth.Target;
		}
		else if (PassContext.BuiltInDepthStencilTarget == EBuiltInDepthStencilTarget::CustomDepthStencil)
		{
			CustomDepthTarget = SceneTextures.CustomDepth.Depth;
		}
	}
	else if (PassContext.bUseDepthStencilTargetAsset && PassContext.DepthStencilTarget.IsValid())
	{
		FString RTDebugName = FString::Printf(TEXT("CustomMeshPass (%s) RT DepthStencil"), *PassContext.PassName.ToString());
		DepthTarget = RegisterExternalTexture(GraphBuilder, PassContext.DepthStencilTarget.RenderTarget->GetTexture2DRHI(), *RTDebugName);

		AddClearDepthStencilPass(GraphBuilder, DepthTarget, true, PassContext.DepthStencilTarget.GetDepthClearValue(), true, PassContext.DepthStencilTarget.GetStencilClearValue());
	}

	// RT in PassContext is Guaranteed to be unique and valid.
	if (PassContext.bUseRenderTargetAssets && !PassContext.RenderTargets.IsEmpty())
	{
		for (int32 Index = 0; Index < PassContext.RenderTargets.Num(); Index++)
		{
			if (Index >= MAX_RT_INDEX)
			{
				break;
			}

			if (!PassContext.RenderTargets[Index].IsValid())
				return;

			FString RTDebugName = FString::Printf(TEXT("CustomMeshPass (%s) RT (%d)"), *PassContext.PassName.ToString(), Index);
			FRDGTextureRef RenderTarget = RegisterExternalTexture(GraphBuilder, PassContext.RenderTargets[Index].RenderTarget->GetTexture2DRHI(), *RTDebugName);

			// Clear color should match texture fast clear color to improve performance.
			AddClearRenderTargetPass(GraphBuilder, RenderTarget, PassContext.RenderTargets[Index].ClearColor);
			RenderTargets.Add(RenderTarget);
		}
	}
	else if (!PassContext.BuiltInRenderTargetTypes.IsEmpty())
	{
		for (int32 Index = 0; Index < PassContext.BuiltInRenderTargetTypes.Num(); Index++)
		{
			if (Index >= MAX_RT_INDEX)
			{
				break;
			}

			if (PassContext.BuiltInRenderTargetTypes[Index] == EBuiltInRenderTarget::SceneColor)
			{
				RenderTargets.Add(SceneTextures.Color.Target);
			}
			else if (PassContext.BuiltInRenderTargetTypes[Index] == EBuiltInRenderTarget::GBufferA)
			{
				RenderTargets.Add(SceneTextures.GBufferA);
			}
			else if (PassContext.BuiltInRenderTargetTypes[Index] == EBuiltInRenderTarget::GBufferB)
			{
				RenderTargets.Add(SceneTextures.GBufferB);
			}
			else if (PassContext.BuiltInRenderTargetTypes[Index] == EBuiltInRenderTarget::GBufferC)
			{
				RenderTargets.Add(SceneTextures.GBufferC);
			}
			else if (PassContext.BuiltInRenderTargetTypes[Index] == EBuiltInRenderTarget::GBufferD)
			{
				RenderTargets.Add(SceneTextures.GBufferD);
			}
			else if (PassContext.BuiltInRenderTargetTypes[Index] == EBuiltInRenderTarget::GBufferE)
			{
				RenderTargets.Add(SceneTextures.GBufferE);
			}
			else if (PassContext.BuiltInRenderTargetTypes[Index] == EBuiltInRenderTarget::GBufferF)
			{
				RenderTargets.Add(SceneTextures.GBufferF);
			}
		}		
	}

	if (RenderTargets.IsEmpty() && DepthTarget == nullptr && CustomDepthTarget == nullptr)
		return;

	FCustomMeshPassDrawContext& DrawContext = *GraphBuilder.AllocObject<FCustomMeshPassDrawContext>(GraphBuilder, Scene, &View, &PassContext, &NaniteFallbackProxies);
	DrawContext.SetupMeshDrawInfos(false);

	FExclusiveDepthStencil ExclusiveDepthStencil;
	const FDepthStencilState& DepthStencilState = PassContext.PipelineState.DepthStencilState;

	if (DepthStencilState.FrontFaceStencilTest.bEnableStencil ||
		DepthStencilState.BackFaceStencilTest.bEnableStencil)
	{
		if (DepthStencilState.FrontFaceStencilTest.StencilTestFailOp == EStencilTestOp::Keep &&
			DepthStencilState.FrontFaceStencilTest.StencilTestPassDepthTestFailOp == EStencilTestOp::Keep &&
			DepthStencilState.FrontFaceStencilTest.StencilTestPassDepthTestPassOp == EStencilTestOp::Keep &&
			DepthStencilState.BackFaceStencilTest.StencilTestFailOp == EStencilTestOp::Keep &&
			DepthStencilState.BackFaceStencilTest.StencilTestPassDepthTestFailOp == EStencilTestOp::Keep &&
			DepthStencilState.BackFaceStencilTest.StencilTestPassDepthTestPassOp == EStencilTestOp::Keep)
		{
			if (DepthStencilState.bEnableDepthWrite)
			{
				ExclusiveDepthStencil = FExclusiveDepthStencil::DepthWrite_StencilRead;
			}
			else
			{
				if (DepthStencilState.DepthTest == EDepthStencilCompareFunction::Always)
				{
					ExclusiveDepthStencil = FExclusiveDepthStencil::DepthNop_StencilRead;
				}
				else
				{
					ExclusiveDepthStencil = FExclusiveDepthStencil::DepthRead_StencilRead;
				}		
			}
		}
		else
		{
			if (DepthStencilState.bEnableDepthWrite)
			{
				ExclusiveDepthStencil = FExclusiveDepthStencil::DepthWrite_StencilWrite;
			}
			else
			{
				if (DepthStencilState.DepthTest == EDepthStencilCompareFunction::Always)
				{
					ExclusiveDepthStencil = FExclusiveDepthStencil::DepthNop_StencilWrite;
				}
				else
				{
					ExclusiveDepthStencil = FExclusiveDepthStencil::DepthRead_StencilWrite;
				}
			}
		}
	}
	else
	{
		if (DepthStencilState.bEnableDepthWrite)
		{
			ExclusiveDepthStencil = FExclusiveDepthStencil::DepthWrite_StencilNop;
		}
		else
		{
			if (DepthStencilState.DepthTest == EDepthStencilCompareFunction::Always)
			{
				ExclusiveDepthStencil = FExclusiveDepthStencil::DepthNop_StencilNop;
			}
			else
			{
				ExclusiveDepthStencil = FExclusiveDepthStencil::DepthRead_StencilNop;
			}
		}
	}

	ERenderTargetLoadAction DefaultDepthLoadAction = ERenderTargetLoadAction::ELoad;
	FDepthStencilBinding DepthStencilBinding;
	FRDGTextureRef FinalDepthTarget = DepthTarget != nullptr ? DepthTarget : CustomDepthTarget;

	if (FinalDepthTarget != nullptr)
	{
		if (!HasBeenProduced(FinalDepthTarget))
		{
			ExclusiveDepthStencil = FExclusiveDepthStencil::DepthWrite_StencilWrite;
			DefaultDepthLoadAction = ERenderTargetLoadAction::EClear;
		}

		DepthStencilBinding = FDepthStencilBinding(
			ExclusiveDepthStencil == FExclusiveDepthStencil::DepthNop_StencilNop ? nullptr : FinalDepthTarget,
			!ExclusiveDepthStencil.IsUsingDepth() ? ERenderTargetLoadAction::ENoAction : DefaultDepthLoadAction,
			!ExclusiveDepthStencil.IsUsingStencil() ? ERenderTargetLoadAction::ENoAction : DefaultDepthLoadAction,
			ExclusiveDepthStencil);
	}

	const FRDGSystemTextures& SystemTextures = FRDGSystemTextures::Get(GraphBuilder);

	FCustomMeshPassUniformParameters* PassUniformParameters = GraphBuilder.AllocParameters<FCustomMeshPassUniformParameters>();
	PassUniformParameters->CustomViewProj = FMatrix44f(PassContext.CustomViewProj);
	PassUniformParameters->PageConstants = FIntVector4(ForceInit);
	PassUniformParameters->VisibleClustersSWHW = GraphBuilder.CreateSRV(GSystemTextures.GetDefaultStructuredBuffer<uint32>(GraphBuilder));
	PassUniformParameters->VisBuffer64 = SystemTextures.Black;

	bool bSupportNanite = false;

#if ENGINE_MODIFY
	const auto PrimaryRasterResults = GraphBuilder.Blackboard.Get<Nanite::FRasterResults>();
	if (PrimaryRasterResults != nullptr)
	{
		PassUniformParameters->PageConstants = PrimaryRasterResults->PageConstants;
		PassUniformParameters->VisibleClustersSWHW = GraphBuilder.CreateSRV(PrimaryRasterResults->VisibleClustersSWHW);
		PassUniformParameters->VisBuffer64 = PrimaryRasterResults->VisBuffer64;

		bSupportNanite = true;
	}
#endif

	FCustomMeshPassParameters* PassParameters = GraphBuilder.AllocParameters<FCustomMeshPassParameters>();
	PassParameters->View = View.ViewUniformBuffer; // Skeletal Mesh Need Binding ViewUBO.
	PassParameters->SceneTextures = SceneTextures.GetSceneTextureShaderParameters(Scene->GetFeatureLevel());
#if UE_VERSION > UE5_2
	PassParameters->Scene = GetSceneUniformBufferRef(GraphBuilder, View);
#endif
	PassParameters->PassUniformBuffer = GraphBuilder.CreateUniformBuffer(PassUniformParameters);
	PassParameters->InstanceCulling = FInstanceCullingContext::CreateDummyInstanceCullingUniformBuffer(GraphBuilder); // This is always needed, but not use, bind dummy instead.
	PassParameters->RenderTargets.DepthStencil = DepthStencilBinding;

	for (int32 Index = 0; Index < RenderTargets.Num(); Index++)
	{
		PassParameters->RenderTargets[Index] = FRenderTargetBinding(RenderTargets[Index], ERenderTargetLoadAction::ELoad);
	}

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("CustomMeshPass (%s)", *PassContext.PassName.ToString()),
		PassParameters,
		ERDGPassFlags::Raster,
		[this, PassParameters, &DrawContext](FRHICommandList& RHICmdList)
		{
			if (DrawContext.PassContext->IsSceneViewRelevanceRenderTarget())
			{
				RHICmdList.SetViewport(
					DrawContext.View->ViewRect.Min.X, 
					DrawContext.View->ViewRect.Min.Y, 
					0.0f, 
					DrawContext.View->ViewRect.Max.X,
					DrawContext.View->ViewRect.Max.Y,
					1.0f);
			}
			DrawContext.SubmitDraw(RHICmdList);
		});

	if (!PassContext.IsUseNaniteFallback() && bSupportNanite)
	{
		// Do not support multipass in nanite screen quad, use fallback instead.
		RenderNaniteScreenQuadPass(GraphBuilder, Scene, View, SceneTextures, PassContext, PassParameters->RenderTargets);
	}

	if (PassContext.bResetStencilAfterUse)
	{
		ResetStencilAfterUse(GraphBuilder, View, FinalDepthTarget, PassContext);
	}

	if (CustomDepthTarget != nullptr)
	{
		const FSceneTexturesConfig& Config = FSceneTexturesConfig::Get();
		FRDGTextureRef CustomDepth = CustomDepthTarget;

		// TextureView is not supported in GLES, so we can't lookup CustomDepth and CustomStencil from a single texture
		// Do a copy of the CustomDepthStencil texture if both CustomDepth and CustomStencil are sampled in a shader.
		if (IsOpenGLPlatform(Scene->GetShaderPlatform()) && 
#if UE_VERSION > UE5_1
			Config.bSamplesCustomStencil)
#else
			Config.bSamplesCustomDepthAndStencil)
#endif
		{
			CustomDepth = GraphBuilder.CreateTexture(CustomDepthTarget->Desc, TEXT("CustomDepthCopy"));
			AddCopyTexturePass(GraphBuilder, CustomDepthTarget, CustomDepth);
		}

		SceneTextures.CustomDepth.Stencil = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateWithPixelFormat(CustomDepth, PF_X24_G8));

		SceneTextures.SetupMode |= ESceneTextureSetupMode::CustomDepth;
		SceneTextures.UniformBuffer = CreateSceneTextureUniformBuffer(GraphBuilder, &SceneTextures, Scene->GetFeatureLevel(), SceneTextures.SetupMode);
	}
}

void FUnrealURPRenderer::RenderNaniteScreenQuadPass(FRDGBuilder& GraphBuilder, FScene* Scene, const FViewInfo& View, const FSceneTextures& SceneTextures, FPassContext& PassContext, const FRenderTargetBindingSlots& RenderTargets)
{
#if ENGINE_MODIFY
	const auto& PrimaryRasterResults = GraphBuilder.Blackboard.GetChecked<Nanite::FRasterResults>();

	TShaderMapRef<FFullScreenQuadVS> VertexShader(View.ShaderMap);
	TShaderRef<FNaniteScreenQuadPS> PixelShader;

	const FMaterialRenderProxy* MaterialRenderProxy = PassContext.MaterialProxy;
	const FMaterial* Material = nullptr;

	FNaniteScreenQuadPS::FPermutationDomain PermutationVectorPS;
	PermutationVectorPS.Set<FNaniteScreenQuadPS::FOutputRenderTargetCount>(PassContext.bUseDepthStencilOnly ? 0 : PassContext.NumOutRenderTarget);
	PermutationVectorPS.Set<FNaniteScreenQuadPS::FOutputDepthTarget>(PassContext.IsOutputDepthTarget());
	PermutationVectorPS.Set<FNaniteScreenQuadPS::FUseDepthStencilOnly>(PassContext.bUseDepthStencilOnly);

	auto TryGetShader = [&](FMaterialShaders& OutShaders) -> bool
	{
		while (MaterialRenderProxy)
		{
			Material = MaterialRenderProxy->GetMaterialNoFallback(Scene->GetFeatureLevel());
			if (Material && Material->GetRenderingThreadShaderMap())
			{
				FMaterialShaderTypes ShaderTypes;
				ShaderTypes.AddShaderType<FNaniteScreenQuadPS>(PermutationVectorPS.ToDimensionValueId());

				if (Material->TryGetShaders(ShaderTypes, nullptr, OutShaders))
				{
					return true;
				}
			}

			MaterialRenderProxy = MaterialRenderProxy->GetFallback(Scene->GetFeatureLevel());
		}
		return false;
	};

	FMaterialShaders MaterialShaders;
	if (!TryGetShader(MaterialShaders))
		return;

	MaterialShaders.TryGetPixelShader(PixelShader);

	auto* PassParameters = GraphBuilder.AllocParameters<FNaniteScreenQuadPS::FParameters>();

	PassParameters->View = View.ViewUniformBuffer;
	PassParameters->VisibleClustersSWHW = GraphBuilder.CreateSRV(PrimaryRasterResults.VisibleClustersSWHW);
	PassParameters->PageConstants = PrimaryRasterResults.PageConstants;
	PassParameters->ClusterPageData = Nanite::GStreamingManager.GetClusterPageDataSRV(GraphBuilder);
	PassParameters->VisBuffer64 = PrimaryRasterResults.VisBuffer64;
	PassParameters->InViews = GraphBuilder.CreateSRV(PrimaryRasterResults.ViewsBuffer);
	PassParameters->RenderTargets = RenderTargets;

	uint32 NaniteCPDMarkIndex = PassContext.NaniteCPDMarkIndex > -1 ? FMath::Min((uint32)PassContext.NaniteCPDMarkIndex, (uint32)FCustomPrimitiveData::NumCustomPrimitiveDataFloats) : 0u;
	uint32 NaniteCPDMarkValue = PassContext.NaniteCPDMarkValue > -1 ? (uint32)PassContext.NaniteCPDMarkValue : 0u;

	PassParameters->NaniteCPDMark = FUintVector4(
		NaniteCPDMarkIndex,
		NaniteCPDMarkValue,
		NaniteCPDMarkIndex / 4,
		NaniteCPDMarkIndex % 4);

	FIntRect ViewRect = View.ViewRect;

	if (!PassContext.IsSceneViewRelevanceRenderTarget() && RenderTargets.Output[0].GetTexture() != nullptr)
	{
		ViewRect = FIntRect(FIntPoint::ZeroValue, RenderTargets.Output[0].GetTexture()->Desc.Extent);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	//FPixelShaderUtils::AddFullscreenPass(
	//	GraphBuilder,
	//	View.ShaderMap,
	//	RDG_EVENT_NAME("NaniteScreenQuadPass"),
	//	PixelShader,
	//	PassParameters,
	//	ViewRect,
	//	PassContext.GetBlendStateRHI(),
	//	TStaticRasterizerState<>::GetRHI(),
	//	PassContext.GetDepthStencilStateRHI(),
	//	PassContext.StencilReference);
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("NaniteScreenQuadPass"),
		PassParameters,
		ERDGPassFlags::Raster,
		[VertexShader, PixelShader, PassParameters, ViewRect, MaterialRenderProxy, Material, &View, &PassContext](FRHICommandList& RHICmdList)
		{
			RHICmdList.SetViewport(ViewRect.Min.X, ViewRect.Min.Y, 0.0f, ViewRect.Max.X, ViewRect.Max.Y, 1.0f);

			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			GraphicsPSOInit.DepthStencilState = PassContext.GetDepthStencilStateRHI();
			GraphicsPSOInit.BlendState = PassContext.GetBlendStateRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;
			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI; // check nullptr.
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, PassContext.StencilReference);

			SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);
			PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), MaterialRenderProxy, *Material, View);

			RHICmdList.SetStreamSource(0, nullptr, 0);
			RHICmdList.DrawPrimitive(0, 2, 1);
		});
#endif
}

void FUnrealURPRenderer::ResetStencilAfterUse(FRDGBuilder& GraphBuilder, const FViewInfo& View, FRDGTextureRef StencilClearTarget, FPassContext& PassContext)
{
	FRenderTargetParameters* ClearParameters = GraphBuilder.AllocParameters<FRenderTargetParameters>();
	ClearParameters->RenderTargets.DepthStencil = FDepthStencilBinding(
		StencilClearTarget, 
		ERenderTargetLoadAction::ENoAction, 
		ERenderTargetLoadAction::ELoad, 
		FExclusiveDepthStencil::DepthNop_StencilWrite);

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("ResetStencilAfterUse"),
		ClearParameters,
		ERDGPassFlags::Raster,
		[this, ClearParameters, &View, &PassContext](FRHICommandListImmediate& RHICmdList)
		{
			FClearQuadCallbacks Callbacks;
			Callbacks.PSOModifier = [&](FGraphicsPipelineStateInitializer& PSOInitializer)
			{
				PSOInitializer.DepthStencilState = PassContext.GetResetStencilStateRHI();
			};

			RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

			DrawClearQuadMRT(RHICmdList, false, 0, nullptr, false, 0, false, PassContext.StencilReference, Callbacks);
		});
}

void FUnrealURPRenderer::SortPass()
{
	auto PrioritySortRule = [&](const FPassContext& A, const FPassContext& B) -> bool
	{
		return A.PassPriority > B.PassPriority;
	};

	PassContexts.Sort(PrioritySortRule);
}

#pragma optimize("", on)
// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnrealURPStruct.generated.h"

class UUnrealURPRenderTarget;
class AActor;
class UMaterialInterface;
class ACameraActor;

DECLARE_LOG_CATEGORY_EXTERN(LogGBBP, Display, All);

UENUM(BlueprintType)
enum class EPassType : uint8
{
	Graphics,
	Compute,
	AsyncCompute
};

UENUM(BlueprintType)
enum class EDepthStencilCompareFunction : uint8
{
	Less,
	LessEqual,
	Greater,
	GreaterEqual,
	Equal,
	NotEqual,
	Never,
	Always
};

UENUM(BlueprintType)
enum class EStencilReadWriteMask : uint8
{
	None = 0,
	Bit_1 = 1,
	Bit_2 = 2,
	Bit_3 = 4,
	Bit_4 = 8,
	Bit_5 = 16,
	Bit_6 = 32,
	Bit_7 = 64,
	Bit_8 = 128,

	Default = 255,
	Bit_All = 255
};

UENUM(BlueprintType)
enum class EStencilTestOp : uint8
{
	Keep,
	Zero,
	Replace,
	SaturatedIncrement,
	SaturatedDecrement,
	Invert,
	Increment,
	Decrement
};

USTRUCT(BlueprintType)
struct FStencilState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StencilTest)
	bool bEnableStencil;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StencilTest)
	EDepthStencilCompareFunction StencilTest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StencilTest)
	EStencilTestOp StencilTestFailOp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StencilTest)
	EStencilTestOp StencilTestPassDepthTestFailOp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StencilTest)
	EStencilTestOp StencilTestPassDepthTestPassOp;

	FStencilState()
		: bEnableStencil(false)
		, StencilTest(EDepthStencilCompareFunction::Always)
		, StencilTestFailOp(EStencilTestOp::Keep)
		, StencilTestPassDepthTestFailOp(EStencilTestOp::Keep)
		, StencilTestPassDepthTestPassOp(EStencilTestOp::Keep)
	{}
};

USTRUCT(BlueprintType)
struct FDepthStencilState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DepthTest)
	bool bEnableDepthWrite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DepthTest)
	EDepthStencilCompareFunction DepthTest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StencilTest, meta = (ClampMax = "255"))
	int32 StencilReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StencilTest)
	FStencilState FrontFaceStencilTest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StencilTest)
	FStencilState BackFaceStencilTest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StencilTest)
	EStencilReadWriteMask StencilReadMask;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StencilTest)
	EStencilReadWriteMask StencilWriteMask;

	FDepthStencilState()
		: bEnableDepthWrite(false)
		, DepthTest(EDepthStencilCompareFunction::Always)
		, StencilReference(-1)
		, StencilReadMask(EStencilReadWriteMask::Bit_All)
		, StencilWriteMask(EStencilReadWriteMask::Bit_All)
	{}
};

UENUM(BlueprintType)
enum class ERasterFillMode : uint8
{
	Point,
	Wireframe,
	Solid
};

UENUM(BlueprintType)
enum class ERasterCullMode : uint8
{
	TwoSide,	
	Back,
	Front
};

UENUM(BlueprintType)
enum class EBlendColorWriteMask : uint8
{
	None = 0,

	R = 0x01,
	G = 0x02,
	B = 0x04,
	A = 0x08,

	RG = R | G,
	RB = R | B,
	RA = R | A,
	GB = G | B,
	GA = G | A,
	BA = B | A,

	RGB = R | G | B,
	RGA = R | G | A,
	RBA = R | B | A,
	GBA = G | B | A,

	RGBA = R | G | B | A
};

UENUM(BlueprintType)
enum class EBlendOp : uint8
{
	Add,
	Subtract,
	Min,
	Max,
	ReverseSubtract
};

UENUM(BlueprintType)
enum class EBlendOpFactor : uint8
{
	Zero,
	One,
	SourceColor,
	InverseSourceColor,
	SourceAlpha,
	InverseSourceAlpha,
	DestAlpha,
	InverseDestAlpha,
	DestColor,
	InverseDestColor,
	ConstantBlendFactor,
	InverseConstantBlendFactor,
	Source1Color,
	InverseSource1Color,
	Source1Alpha,
	InverseSource1Alpha,
};

USTRUCT(BlueprintType)
struct FBlendState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BlendState)
	EBlendColorWriteMask ColorWriteMask;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BlendState)
	EBlendOp ColorBlendOp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BlendState)
	EBlendOpFactor ColorBlendOpSrcFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BlendState)
	EBlendOpFactor ColorBlendOpDstFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BlendState)
	EBlendOp AlphaBlendOp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BlendState)
	EBlendOpFactor AlphaBlendOpSrcFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BlendState)
	EBlendOpFactor AlphaBlendOpDstFactor;

	FBlendState()
		: ColorWriteMask(EBlendColorWriteMask::RGBA)
		, ColorBlendOp(EBlendOp::Add)
		, ColorBlendOpSrcFactor(EBlendOpFactor::One)
		, ColorBlendOpDstFactor(EBlendOpFactor::Zero)
		, AlphaBlendOp(EBlendOp::Add)
		, AlphaBlendOpSrcFactor(EBlendOpFactor::One)
		, AlphaBlendOpDstFactor(EBlendOpFactor::Zero)
	{}
};

UENUM(BlueprintType)
enum class EDrawPrimitiveType : uint8
{
	// Topology that defines a triangle N with 3 vertex extremities : 3 * N + 0, 3 * N + 1, 3 * N + 2.
	TriangleList,

	// Topology that defines a triangle N with 3 vertex extremities: N+0, N+1, N+2.
	TriangleStrip,

	// Topology that defines a line with 2 vertex extremities: 2*N+0, 2*N+1.
	LineList,

	// Topology that defines a quad N with 4 vertex extremities: 4*N+0, 4*N+1, 4*N+2, 4*N+3.
	// Supported only if GRHISupportsQuadTopology == true.
	QuadList,

	// Topology that defines a point N with a single vertex N.
	PointList,

	// Topology that defines a screen aligned rectangle N with only 3 vertex corners:
	//    3*N + 0 is upper-left corner,
	//    3*N + 1 is upper-right corner,
	//    3*N + 2 is the lower-left corner.
	// Supported only if GRHISupportsRectTopology == true.
	RectList,
};

USTRUCT(BlueprintType)
struct FPipelineState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DepthStencilState)
	FDepthStencilState DepthStencilState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BlendState)
	TArray<FBlendState> BlendState;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BlendState)
	bool bUseAlphaToCoverage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RasterizerState)
	ERasterFillMode FillMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RasterizerState)
	ERasterCullMode CullMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Misc)
	EDrawPrimitiveType PrimitiveType;

	FPipelineState()
		: bUseAlphaToCoverage(false)
		, FillMode(ERasterFillMode::Solid)
		, CullMode(ERasterCullMode::Back)
		, PrimitiveType(EDrawPrimitiveType::TriangleList)
	{}
};

UENUM(BlueprintType)
enum class EBuiltInRenderTarget : uint8
{
	None,
	SceneColor,

	// WorldNormal in RGB, PerObjectGBufferData in A.
	GBufferA,

	// Metallic in R, Specular in G, Roughness in B, ShadingModelID in low 4-bit of A, SelectiveOutputMask in high 4-bit of A.
	GBufferB,

	// BaseColor in RGB, GBufferAO in A.
	GBufferC,

	// CustomData in RGBA.
	GBufferD,

	// PrecomputedShadowFactors in RGBA.
	GBufferE,

	// WorldTangent in RGB, Anisotropy in A.
	GBufferF
};

UENUM(BlueprintType)
enum class EBuiltInDepthStencilTarget : uint8
{
	None,
	SceneDepthStencil,
	CustomDepthStencil
};

UENUM(BlueprintType)
enum class EPassInjectionPoint : uint8
{
	None,
	BeforeRendering,
	AfterPrePass,
	AfterOpaqueBasePass,
	BeforeTransparent,
	BeforePostProcess,
	AfterSSRInput,
	AfterMotionBlur,
	AfterTonemap,
	AfterFXAA
};

USTRUCT(BlueprintType)
struct FRenderTargetSetup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RenderTargetSetup)
	TObjectPtr<UUnrealURPRenderTarget> RenderTargetAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RenderTargetSetup)
	FLinearColor ClearColor;

	FRenderTargetSetup()
		: ClearColor(FLinearColor::Black)
	{}
};

USTRUCT(BlueprintType)
struct FMultiPassFlags
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiPassFlags)
	uint8 bEnablePass0 : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiPassFlags)
	uint8 bEnablePass1 : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiPassFlags)
	uint8 bEnablePass2 : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiPassFlags)
	uint8 bEnablePass3 : 1;

	FMultiPassFlags()
		: bEnablePass0(true)
		, bEnablePass1(false)
		, bEnablePass2(false)
		, bEnablePass3(false)
	{}

	explicit FMultiPassFlags(EForceInit)
		: bEnablePass0(false)
		, bEnablePass1(false)
		, bEnablePass2(false)
		, bEnablePass3(false)
	{}
};

USTRUCT(BlueprintType)
struct FRenderableGameObject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RenderableGameObject)
	TObjectPtr<AActor> Actor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RenderableGameObject)
	FMultiPassFlags MultiPassFlags;
};

USTRUCT(BlueprintType)
struct FRenderableComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RenderableComponent)
	TObjectPtr<UPrimitiveComponent> Component;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RenderableComponent)
	FMultiPassFlags MultiPassFlags;
};

UENUM(BlueprintType)
enum class EFrameRate : uint8
{
	Full,
	Half,
	Quarter
};

USTRUCT(BlueprintType)
struct FGraphicPassParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	int32 PassPriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	FName PassName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	EFrameRate FrameRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	bool bUseNaniteFallback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup, meta = (EditCondition = "!bUseNaniteFallback", EditConditionHides))
	int32 NaniteCPDMarkIndex;

	// TODO: @BUG BP instance can not edit property.
	//UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = CustomMeshPassSetup, meta = (EditCondition = "!bUseNaniteFallback", EditConditionHides))
	int32 NaniteCPDMarkValue;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = CustomMeshPassSetup, meta = (EditCondition = "!bUseNaniteFallback", EditConditionHides, DisplayName = "Nanite CPDMark Value"))
	int32 NaniteCPDMarkValue_DebugOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	bool bUseManualCulling;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup, meta = (EditCondition = bUseManualCulling, EditConditionHides, UIMin = "0.0", UIMax = "1.0"))
	float MaxDrawDistanceScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	TArray<FRenderableGameObject> GameObjects;

	UPROPERTY()
	bool bEnableMultiPassSupport;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup, meta = (EditCondition = "!bEnableMultiPassSupport", EditConditionHides))
	TObjectPtr<UMaterialInterface> ShaderScript;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	TObjectPtr<ACameraActor> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	FPipelineState PipelineState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	bool bUseDepthStencilOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup, meta = (EditCondition = "!bUseDepthStencilOnly", EditConditionHides))
	bool bUseRenderTargetAssets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup, meta = (EditCondition = "!bUseDepthStencilOnly && bUseRenderTargetAssets", EditConditionHides))
	bool bUseScreenViewportSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup, meta = (EditCondition = "!bUseDepthStencilOnly && bUseRenderTargetAssets", EditConditionHides))
	TArray<FRenderTargetSetup> RenderTargetAssets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup, meta = (EditCondition = "!bUseDepthStencilOnly && !bUseRenderTargetAssets", EditConditionHides))
	TArray<EBuiltInRenderTarget> RenderTargets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	bool bUseDepthStencilTargetAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup, meta = (EditCondition = "!bUseDepthStencilTargetAsset", EditConditionHides))
	EBuiltInDepthStencilTarget DepthStencilTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup, meta = (EditCondition = "bUseDepthStencilTargetAsset", EditConditionHides))
	FRenderTargetSetup DepthStencilTargetAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	bool bResetStencilAfterUse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMeshPassSetup)
	EPassInjectionPoint InjectionPoint;

	FGraphicPassParameters()
		: PassPriority(-1)
		, FrameRate(EFrameRate::Full)
		, bUseNaniteFallback(true)
		, NaniteCPDMarkIndex(-1)
		, NaniteCPDMarkValue(-1)
		, NaniteCPDMarkValue_DebugOnly(-1)
		, bUseManualCulling(false)
		, MaxDrawDistanceScale(1.0f)
		, bEnableMultiPassSupport(false)
		, bUseDepthStencilOnly(false)
		, bUseRenderTargetAssets(true)
		, bUseScreenViewportSize(false)
		, bUseDepthStencilTargetAsset(false)
		, DepthStencilTarget(EBuiltInDepthStencilTarget::None)
		, bResetStencilAfterUse(false)
		, InjectionPoint(EPassInjectionPoint::None)
	{}
};

USTRUCT(BlueprintType)
struct FComputePassParameters
{
	GENERATED_BODY()
};

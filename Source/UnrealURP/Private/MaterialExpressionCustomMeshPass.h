// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "UObject/ObjectMacros.h"
#include "MaterialExpressionCustomMeshPass.generated.h"

////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassVSOutput
////////////////////////////////////////////////////////////////////////////////////////

// Note: Use derived BP class have problems, fallback to cpp.
UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionCustomMeshPassVSOutput : public UMaterialExpressionCustomOutput
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FExpressionInput HomogeneousPosition;

	UPROPERTY()
	FExpressionInput VertexColor;

	UPROPERTY()
	FExpressionInput WorldPosition;

	UPROPERTY()
	FExpressionInput WorldNormal;

	UPROPERTY()
	FExpressionInput WorldTangent;

	UPROPERTY()
	FExpressionInput TextureCoordinate0;

	UPROPERTY()
	FExpressionInput TextureCoordinate1;

	UPROPERTY()
	FExpressionInput CustomOutput0;

	UPROPERTY()
	FExpressionInput CustomOutput1;

	UPROPERTY()
	FExpressionInput WorldPositionOffset;

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	//~ End UMaterialExpression Interface

	virtual EShaderFrequency GetShaderFrequency() override { return SF_Vertex; }
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual int32 GetNumOutputs() const override;
	virtual FString GetFunctionName() const override;
	virtual FString GetDisplayName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};

////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassVSOutput
////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassVSOutputExt1
////////////////////////////////////////////////////////////////////////////////////////

UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionCustomMeshPassVSOutputExt1 : public UMaterialExpressionCustomMeshPassVSOutput
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	//~ End UMaterialExpression Interface
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual FString GetFunctionName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};

////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassVSOutputExt1
////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassVSOutputExt2
////////////////////////////////////////////////////////////////////////////////////////

UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionCustomMeshPassVSOutputExt2 : public UMaterialExpressionCustomMeshPassVSOutput
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	//~ End UMaterialExpression Interface
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual FString GetFunctionName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};

////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassVSOutputExt2
////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassVSOutputExt3
////////////////////////////////////////////////////////////////////////////////////////

UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionCustomMeshPassVSOutputExt3 : public UMaterialExpressionCustomMeshPassVSOutput
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	//~ End UMaterialExpression Interface
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual FString GetFunctionName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};

////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassVSOutputExt3
////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassPSOutput
////////////////////////////////////////////////////////////////////////////////////////

UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionCustomMeshPassPSOutput : public UMaterialExpressionCustomOutput
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FExpressionInput OutRenderTarget0;

	UPROPERTY()
	FExpressionInput OutRenderTarget1;

	UPROPERTY()
	FExpressionInput OutRenderTarget2;

	UPROPERTY()
	FExpressionInput OutRenderTarget3;

	UPROPERTY()
	FExpressionInput OutRenderTarget4;

	UPROPERTY()
	FExpressionInput OutRenderTarget5;

	UPROPERTY()
	FExpressionInput OutRenderTarget6;

	UPROPERTY()
	FExpressionInput OutRenderTarget7;

	UPROPERTY()
	FExpressionInput OutDepthTarget;

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	//~ End UMaterialExpression Interface
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual int32 GetNumOutputs() const override;
	virtual FString GetFunctionName() const override;
	virtual FString GetDisplayName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};

////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassPSOutput
////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassPSOutputExt1
////////////////////////////////////////////////////////////////////////////////////////

UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionCustomMeshPassPSOutputExt1 : public UMaterialExpressionCustomMeshPassPSOutput
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	//~ End UMaterialExpression Interface
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual FString GetFunctionName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};

////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassPSOutputExt1
////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassPSOutputExt2
////////////////////////////////////////////////////////////////////////////////////////

UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionCustomMeshPassPSOutputExt2 : public UMaterialExpressionCustomMeshPassPSOutput
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	//~ End UMaterialExpression Interface
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual FString GetFunctionName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};

////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassPSOutputExt2
////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassPSOutputExt3
////////////////////////////////////////////////////////////////////////////////////////

UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionCustomMeshPassPSOutputExt3 : public UMaterialExpressionCustomMeshPassPSOutput
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	//~ End UMaterialExpression Interface
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual FString GetFunctionName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};

////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassPSOutputExt3
////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------//
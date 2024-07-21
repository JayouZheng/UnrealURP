// Copyright Jayou, Inc. All Rights Reserved.

#include "MaterialExpressionCustomMeshPass.h"
#include "MaterialCompiler.h"

#define LOCTEXT_NAMESPACE "MaterialExpressionCustomMeshPass"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassVSOutput
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UMaterialExpressionCustomMeshPassVSOutput::UMaterialExpressionCustomMeshPassVSOutput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FText NAME_MaterialExpressionCustomMeshPass;
		FConstructorStatics()
			: NAME_MaterialExpressionCustomMeshPass(LOCTEXT("CustomMeshPass", "CustomMeshPass"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

#if WITH_EDITORONLY_DATA
	MenuCategories.Add(ConstructorStatics.NAME_MaterialExpressionCustomMeshPass);
#endif

#if WITH_EDITOR
	Outputs.Reset();
#endif
}

#if WITH_EDITOR

int32 UMaterialExpressionCustomMeshPassVSOutput::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	int32 CodeInput = INDEX_NONE;

	if (OutputIndex == 0)
	{
		CodeInput = HomogeneousPosition.IsConnected() ? HomogeneousPosition.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 1)
	{
		CodeInput = VertexColor.IsConnected() ? VertexColor.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 2)
	{
		CodeInput = WorldPosition.IsConnected() ? WorldPosition.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 3)
	{
		CodeInput = WorldNormal.IsConnected() ? WorldNormal.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 4)
	{
		CodeInput = WorldTangent.IsConnected() ? WorldTangent.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 5)
	{
		CodeInput = TextureCoordinate0.IsConnected() ? TextureCoordinate0.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 6)
	{
		CodeInput = TextureCoordinate1.IsConnected() ? TextureCoordinate1.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 7)
	{
		CodeInput = CustomOutput0.IsConnected() ? CustomOutput0.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 8)
	{
		CodeInput = CustomOutput1.IsConnected() ? CustomOutput1.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 9)
	{
		CodeInput = WorldPositionOffset.IsConnected() ? WorldPositionOffset.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	return Compiler->CustomOutput(this, OutputIndex, CodeInput);
}

void UMaterialExpressionCustomMeshPassVSOutput::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("Custom mesh pass vertex shader output")));
}

#endif // WITH_EDITOR

int32 UMaterialExpressionCustomMeshPassVSOutput::GetNumOutputs() const
{
	return 10;
}

FString UMaterialExpressionCustomMeshPassVSOutput::GetFunctionName() const
{
	return TEXT("GetCustomMeshPassVSOutput");
}

FString UMaterialExpressionCustomMeshPassVSOutput::GetDisplayName() const
{
	return TEXT("Custom mesh pass vertex shader output");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassVSOutput
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------------------------------------//

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassVSOutputExt1
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UMaterialExpressionCustomMeshPassVSOutputExt1::UMaterialExpressionCustomMeshPassVSOutputExt1(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void UMaterialExpressionCustomMeshPassVSOutputExt1::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("Custom mesh pass vertex shader output ext1")));
}
#endif // WITH_EDITOR

FString UMaterialExpressionCustomMeshPassVSOutputExt1::GetFunctionName() const
{
	return TEXT("GetCustomMeshPassVSOutputExt1");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassVSOutputExt1
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------------------------------------//

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassVSOutputExt2
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UMaterialExpressionCustomMeshPassVSOutputExt2::UMaterialExpressionCustomMeshPassVSOutputExt2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void UMaterialExpressionCustomMeshPassVSOutputExt2::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("Custom mesh pass vertex shader output ext2")));
}
#endif // WITH_EDITOR

FString UMaterialExpressionCustomMeshPassVSOutputExt2::GetFunctionName() const
{
	return TEXT("GetCustomMeshPassVSOutputExt2");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassVSOutputExt2
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------------------------------------//

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassVSOutputExt3
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UMaterialExpressionCustomMeshPassVSOutputExt3::UMaterialExpressionCustomMeshPassVSOutputExt3(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void UMaterialExpressionCustomMeshPassVSOutputExt3::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("Custom mesh pass vertex shader output ext3")));
}
#endif // WITH_EDITOR

FString UMaterialExpressionCustomMeshPassVSOutputExt3::GetFunctionName() const
{
	return TEXT("GetCustomMeshPassVSOutputExt3");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassVSOutputExt3
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------------------------------------//

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassPSOutput
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UMaterialExpressionCustomMeshPassPSOutput::UMaterialExpressionCustomMeshPassPSOutput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FText NAME_MaterialExpressionCustomMeshPass;
		FConstructorStatics()
			: NAME_MaterialExpressionCustomMeshPass(LOCTEXT("CustomMeshPass", "CustomMeshPass"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

#if WITH_EDITORONLY_DATA
	MenuCategories.Add(ConstructorStatics.NAME_MaterialExpressionCustomMeshPass);
#endif

#if WITH_EDITOR
	Outputs.Reset();
#endif
}

#if WITH_EDITOR

int32 UMaterialExpressionCustomMeshPassPSOutput::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	int32 CodeInput = INDEX_NONE;

	if (OutputIndex == 0)
	{
		CodeInput = OutRenderTarget0.IsConnected() ? OutRenderTarget0.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 1)
	{
		CodeInput = OutRenderTarget1.IsConnected() ? OutRenderTarget1.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 2)
	{
		CodeInput = OutRenderTarget2.IsConnected() ? OutRenderTarget2.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 3)
	{
		CodeInput = OutRenderTarget3.IsConnected() ? OutRenderTarget3.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 4)
	{
		CodeInput = OutRenderTarget4.IsConnected() ? OutRenderTarget4.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 5)
	{
		CodeInput = OutRenderTarget5.IsConnected() ? OutRenderTarget5.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 6)
	{
		CodeInput = OutRenderTarget6.IsConnected() ? OutRenderTarget6.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 7)
	{
		CodeInput = OutRenderTarget7.IsConnected() ? OutRenderTarget7.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (OutputIndex == 8)
	{
		CodeInput = OutDepthTarget.IsConnected() ? OutDepthTarget.Compile(Compiler) : Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	return Compiler->CustomOutput(this, OutputIndex, CodeInput);
}

void UMaterialExpressionCustomMeshPassPSOutput::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("Custom mesh pass pixel shader output")));
}

#endif // WITH_EDITOR

int32 UMaterialExpressionCustomMeshPassPSOutput::GetNumOutputs() const
{
	return 9;
}

FString UMaterialExpressionCustomMeshPassPSOutput::GetFunctionName() const
{
	return TEXT("GetCustomMeshPassPSOutput");
}

FString UMaterialExpressionCustomMeshPassPSOutput::GetDisplayName() const
{
	return TEXT("Custom mesh pass pixel shader output");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassPSOutput
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------------------------------------//

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassPSOutputExt1
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UMaterialExpressionCustomMeshPassPSOutputExt1::UMaterialExpressionCustomMeshPassPSOutputExt1(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void UMaterialExpressionCustomMeshPassPSOutputExt1::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("Custom mesh pass pixel shader output ext1")));
}
#endif // WITH_EDITOR

FString UMaterialExpressionCustomMeshPassPSOutputExt1::GetFunctionName() const
{
	return TEXT("GetCustomMeshPassPSOutputExt1");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassPSOutputExt1
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------------------------------------//

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassPSOutputExt2
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UMaterialExpressionCustomMeshPassPSOutputExt2::UMaterialExpressionCustomMeshPassPSOutputExt2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void UMaterialExpressionCustomMeshPassPSOutputExt2::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("Custom mesh pass pixel shader output ext2")));
}
#endif // WITH_EDITOR

FString UMaterialExpressionCustomMeshPassPSOutputExt2::GetFunctionName() const
{
	return TEXT("GetCustomMeshPassPSOutputExt2");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassPSOutputExt2
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------------------------------------//

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Begin: UMaterialExpressionCustomMeshPassPSOutputExt3
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UMaterialExpressionCustomMeshPassPSOutputExt3::UMaterialExpressionCustomMeshPassPSOutputExt3(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void UMaterialExpressionCustomMeshPassPSOutputExt3::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("Custom mesh pass pixel shader output ext3")));
}
#endif // WITH_EDITOR

FString UMaterialExpressionCustomMeshPassPSOutputExt3::GetFunctionName() const
{
	return TEXT("GetCustomMeshPassPSOutputExt3");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End: UMaterialExpressionCustomMeshPassPSOutputExt3
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------------------------------------//

#undef LOCTEXT_NAMESPACE
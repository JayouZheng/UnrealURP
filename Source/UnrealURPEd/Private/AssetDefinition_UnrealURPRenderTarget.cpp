// Copyright Jayou, Inc. All Rights Reserved.

#include "AssetDefinition_UnrealURPRenderTarget.h"
#include "Interfaces/ITextureEditorModule.h"

#define LOCTEXT_NAMESPACE "UAssetDefinition_UnrealURPRenderTarget"

EAssetCommandResult UAssetDefinition_UnrealURPRenderTarget::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UTexture* Texture : OpenArgs.LoadObjects<UTexture>())
	{
		ITextureEditorModule* TextureEditorModule = &FModuleManager::LoadModuleChecked<ITextureEditorModule>("TextureEditor");
		TextureEditorModule->CreateTextureEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Texture);
	}

	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE

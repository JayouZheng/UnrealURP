// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "UnrealURPRenderTarget.h"
#include "AssetDefinitionDefault.h"
#include "AssetDefinition_UnrealURPRenderTarget.generated.h"

UCLASS()
class UNREALURPED_API UAssetDefinition_UnrealURPRenderTarget : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetDefinition_UnrealURPRenderTarget", "UnrealURP RT"); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UUnrealURPRenderTarget::StaticClass(); }

	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(64, 192, 64)); }

	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { EAssetCategoryPaths::Texture };
		return Categories;
	}
	virtual bool CanImport() const override { return false; }

	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	// UAssetDefinition End
};

// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnrealURPConfig.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UnrealURPRenderTarget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UNREALURP_API UUnrealURPRenderTarget : public UTextureRenderTarget2D
{
	GENERATED_UCLASS_BODY()

public:

	virtual FTextureResource* CreateResource() override;
	
	void ResizeTarget(uint32 InSizeX, uint32 InSizeY);

	virtual uint32 CalcTextureMemorySizeEnum(ETextureMipCount Enum) const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	bool CanBeUseAsRenderTarget() const { return !bUseAsDepthStencilTarget && !bUseAsUAV; }
	bool CanBeUseAsDepthTarget() const { return bUseAsDepthStencilTarget; }
	bool CanBeUseAsUAV() const { return bUseAsUAV; }

private:

#if UE_VERSION >= UE5_4_3
	//~ Begin UTextureRenderTarget Interface
	virtual bool CanConvertToTexture(ETextureSourceFormat& OutTextureSourceFormat, EPixelFormat& OutPixelFormat, FText* OutErrorMessage) const override;
	virtual TSubclassOf<UTexture> GetTextureUClass() const override;
	virtual EPixelFormat GetFormat() const override;
	virtual bool IsSRGB() const override;
	virtual float GetDisplayGamma() const override;
	virtual ETextureClass GetRenderTargetTextureClass() const override { return ETextureClass::TwoD; }
	//~ End UTextureRenderTarget Interface

	ETextureSourceFormat ValidateTextureFormatForConversionToTextureInternal(EPixelFormat InFormat, const TArrayView<const EPixelFormat>& InCompatibleFormats, FText* OutErrorMessage) const;

	virtual ETextureRenderTargetSampleCount GetSampleCount() const override;
#endif

private:

	UPROPERTY(EditAnywhere, Category = TextureRenderTarget2D, AssetRegistrySearchable)
	uint8 bUseAsDepthStencilTarget : 1;

	UPROPERTY(EditAnywhere, Category = TextureRenderTarget2D, AssetRegistrySearchable)
	uint8 bUseAsUAV : 1;
};
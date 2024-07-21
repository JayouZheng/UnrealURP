// Copyright Jayou, Inc. All Rights Reserved.

#include "UnrealURPRenderTarget.h"
#include "SingleCustomMeshPass.h"
#include "TextureResource.h"
#include "UnrealURPConfig.h"

#if UE_VERSION >= UE5_4_3
#include "ImageCoreUtils.h"
#endif

class FDepthStencilTarget2DResource : public FTextureResource, public FRenderTarget, public FDeferredUpdateResource
{
public:

	FDepthStencilTarget2DResource(const class UTextureRenderTarget2D* InOwner)
		: Owner(InOwner)
		, ClearColor(InOwner->ClearColor)
		, Format(InOwner->GetFormat())
		, TargetSizeX(Owner->SizeX)
		, TargetSizeY(Owner->SizeY)
	{}

#if UE_VERSION > UE5_2
	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
#else
	virtual void InitDynamicRHI() override
#endif
	{
		LLM_SCOPED_TAG_WITH_OBJECT_IN_SET(Owner->GetOutermost(), ELLMTagSet::Assets);

		if (Owner->SizeX > 0 && Owner->SizeY > 0)
		{
			FString ResourceName = Owner->GetName();
			ETextureCreateFlags TexCreateFlags = ETextureCreateFlags::DepthStencilTargetable | ETextureCreateFlags::ShaderResource;

			FRHITextureCreateDesc Desc =
				FRHITextureCreateDesc::Create2D(*ResourceName)
				.SetExtent(Owner->SizeX, Owner->SizeY)
				.SetFormat(PF_DepthStencil)
				.SetNumMips(1)
				.SetFlags(TexCreateFlags)
				.SetInitialState(ERHIAccess::DSVWrite)
				.SetClearValue(FClearValueBinding::DepthZero);

			TextureRHI = RenderTargetTextureRHI = RHICreateTexture(Desc);

			// No UAV

			RHIUpdateTextureReference(Owner->TextureReference.TextureReferenceRHI, TextureRHI);

			AddToDeferredUpdateList(true);
		}

		// Create the sampler state RHI resource.
		FSamplerStateInitializerRHI SamplerStateInitializer
		(
			SF_Point,
			Owner->AddressX == TA_Wrap ? AM_Wrap : (Owner->AddressX == TA_Clamp ? AM_Clamp : AM_Mirror),
			Owner->AddressY == TA_Wrap ? AM_Wrap : (Owner->AddressY == TA_Clamp ? AM_Clamp : AM_Mirror),
			AM_Wrap
		);
		SamplerStateRHI = GetOrCreateSamplerState(SamplerStateInitializer);
	}

#if UE_VERSION > UE5_2
	virtual void ReleaseRHI() override
#else
	virtual void ReleaseDynamicRHI() override
#endif
	{
		// release the FTexture RHI resources here as well
		FTexture::ReleaseRHI();

		RHIUpdateTextureReference(Owner->TextureReference.TextureReferenceRHI, nullptr);
		RenderTargetTextureRHI.SafeRelease();

		// remove grom global list of deferred clears
		RemoveFromDeferredUpdateList();
	}

	virtual FIntPoint GetSizeXY() const override
	{
		return FIntPoint(TargetSizeX, TargetSizeY);
	}

protected:

	friend class UUnrealURPRenderTarget;
	virtual void UpdateDeferredResource(FRHICommandListImmediate& RHICmdList, bool bClearRenderTarget = true) override
	{
		LLM_SCOPED_TAG_WITH_OBJECT_IN_SET(Owner->GetOutermost(), ELLMTagSet::Assets);

		SCOPED_DRAW_EVENT(RHICmdList, GPUResourceUpdate)
		RemoveFromDeferredUpdateList();
	}

	void Resize(int32 NewSizeX, int32 NewSizeY)
	{
		if (TargetSizeX != NewSizeX || TargetSizeY != NewSizeY)
		{
			TargetSizeX = NewSizeX;
			TargetSizeY = NewSizeY;
			UpdateRHI();
		}
	}

private:

	const class UTextureRenderTarget2D* Owner;
	FLinearColor ClearColor;
	EPixelFormat Format;
	int32 TargetSizeX, TargetSizeY;
};

UUnrealURPRenderTarget::UUnrealURPRenderTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bUseAsDepthStencilTarget(false)
{
}

FTextureResource* UUnrealURPRenderTarget::CreateResource()
{
	if (bUseAsDepthStencilTarget)
	{
		//OverrideFormat = PF_DepthStencil;
		return new FDepthStencilTarget2DResource(this);
	}

	return Super::CreateResource();
}

void UUnrealURPRenderTarget::ResizeTarget(uint32 InSizeX, uint32 InSizeY)
{
	if (bUseAsDepthStencilTarget)
	{
		if (SizeX != InSizeX || SizeY != InSizeY)
		{
			SizeX = InSizeX;
			SizeY = InSizeY;

			if (GetResource())
			{
				FDepthStencilTarget2DResource* InResource = static_cast<FDepthStencilTarget2DResource*>(GetResource());
				int32 NewSizeX = SizeX;
				int32 NewSizeY = SizeY;
				ENQUEUE_RENDER_COMMAND(ResizeRenderTarget)(
					[InResource, NewSizeX, NewSizeY](FRHICommandListImmediate& RHICmdList)
					{
						InResource->Resize(NewSizeX, NewSizeY);
						InResource->UpdateDeferredResource(RHICmdList, true);
					}
				);
			}
			else
			{
				UpdateResource();
			}
		}
	}
	else
	{
		Super::ResizeTarget(InSizeX, InSizeY);
	}
}

uint32 UUnrealURPRenderTarget::CalcTextureMemorySizeEnum(ETextureMipCount Enum) const
{
	// Calculate size based on format.  All mips are resident on render targets so we always return the same value.
	EPixelFormat Format = GetFormat();
	int32 BlockSizeX = GPixelFormats[Format].BlockSizeX;
	int32 BlockSizeY = GPixelFormats[Format].BlockSizeY;
	int32 BlockBytes = GPixelFormats[Format].BlockBytes;
	int32 NumBlocksX = (SizeX + BlockSizeX - 1) / BlockSizeX;
	int32 NumBlocksY = (SizeY + BlockSizeY - 1) / BlockSizeY;
	int32 NumBytes = NumBlocksX * NumBlocksY * BlockBytes;
	return NumBytes;
}

#if WITH_EDITOR
void UUnrealURPRenderTarget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UUnrealURPRenderTarget, bUseAsDepthStencilTarget))
	{
		if (bUseAsDepthStencilTarget == true)
		{
			bUseAsUAV = false;
		}
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UUnrealURPRenderTarget, bUseAsUAV))
	{
		if (bUseAsUAV == true)
		{
			bUseAsDepthStencilTarget = false;
		}
	}

	for (TObjectIterator<UCustomMeshPassBaseComponent> It; It; ++It)
	{
		UCustomMeshPassBaseComponent* CustomMeshPassComponent = *It;

		for (auto& RT : CustomMeshPassComponent->GraphicPassParameters.RenderTargetAssets)
		{
			if (RT.RenderTargetAsset == this)
			{
				CustomMeshPassComponent->MarkRenderStateDirty();
			}
		}

		if (CustomMeshPassComponent->GraphicPassParameters.DepthStencilTargetAsset.RenderTargetAsset == this)
		{
			CustomMeshPassComponent->MarkRenderStateDirty();
		}
	}
}
#endif

#if UE_VERSION >= UE5_4_3
bool UUnrealURPRenderTarget::CanConvertToTexture(ETextureSourceFormat& OutTextureSourceFormat, EPixelFormat& OutPixelFormat, FText* OutErrorMessage) const
{
	const EPixelFormat LocalFormat = GetFormat();

	// empty array means all formats supported
	const ETextureSourceFormat TextureSourceFormat = ValidateTextureFormatForConversionToTextureInternal(LocalFormat, { }, OutErrorMessage);
	if (TextureSourceFormat == TSF_Invalid)
	{
		return false;
	}

	if ((SizeX <= 0) || (SizeY <= 0))
	{
		if (OutErrorMessage != nullptr)
		{
			*OutErrorMessage = FText::Format(NSLOCTEXT("TextureRenderTarget2D", "InvalidSizeForConversionToTexture", "Invalid size ({0},{1}) for converting {2} to {3}"),
				FText::AsNumber(SizeX),
				FText::AsNumber(SizeY),
				FText::FromString(GetClass()->GetName()),
				FText::FromString(GetTextureUClass()->GetName()));
		}
		return false;
	}

	OutPixelFormat = LocalFormat;
	OutTextureSourceFormat = TextureSourceFormat;
	return true;
}

TSubclassOf<UTexture> UUnrealURPRenderTarget::GetTextureUClass() const
{
	return UTexture2D::StaticClass();
}

EPixelFormat UUnrealURPRenderTarget::GetFormat() const
{
	if (OverrideFormat == PF_Unknown)
	{
		return GetPixelFormatFromRenderTargetFormat(RenderTargetFormat);
	}
	else
	{
		return OverrideFormat;
	}
}

bool UUnrealURPRenderTarget::IsSRGB() const
{
	// in theory you'd like the "bool SRGB" variable to == this, but it does not

	// ?? note: UTextureRenderTarget::TargetGamma is ignored here
	// ?? note: GetDisplayGamma forces linear for some float formats, but this doesn't

	if (OverrideFormat == PF_Unknown)
	{
		return RenderTargetFormat == RTF_RGBA8_SRGB;
	}
	else
	{
		return !bForceLinearGamma;
	}
}

float UUnrealURPRenderTarget::GetDisplayGamma() const
{
	// if TargetGamma is set (not zero), it overrides everything else
	if (TargetGamma > UE_KINDA_SMALL_NUMBER * 10.0f)
	{
		return TargetGamma;
	}

	// ?? special casing just two of the float PixelFormats to force 1.0 gamma here is inconsistent
	//		(there are lots of other float formats)
	// ignores Owner->IsSRGB() ? it's similar but not quite the same
	EPixelFormat Format = GetFormat();
	if (Format == PF_FloatRGB || Format == PF_FloatRGBA || bForceLinearGamma)
	{
		return 1.0f;
	}

	// TextureRenderTarget default gamma does not respond to Engine->DisplayGamma setting
	//	?? was that intentional or a bug ?
	//return FRenderTarget::GetEngineDisplayGamma();

	// return hard-coded gamma 2.2 which corresponds to SRGB
	return 2.2f;
}

ETextureSourceFormat UUnrealURPRenderTarget::ValidateTextureFormatForConversionToTextureInternal(EPixelFormat InFormat, const TArrayView<const EPixelFormat>& InCompatibleFormats, FText* OutErrorMessage) const
{
	// InCompatibleFormats can be empty, meaning anything works
	if (InCompatibleFormats.Num() != 0 && !InCompatibleFormats.Contains(InFormat))
	{
		if (OutErrorMessage != nullptr)
		{
			TArray<const TCHAR*> CompatibleFormatStrings;
			Algo::Transform(InCompatibleFormats, CompatibleFormatStrings, [](EPixelFormat InCompatiblePixelFormat) { return GetPixelFormatString(InCompatiblePixelFormat); });

			*OutErrorMessage = FText::Format(NSLOCTEXT("TextureRenderTarget", "UnsupportedFormatForConversionToTexture", "Unsupported format ({0}) for converting {1} to {2}. Supported formats are: {3}"),
				FText::FromString(FString(GetPixelFormatString(InFormat))),
				FText::FromString(GetClass()->GetName()),
				FText::FromString(GetTextureUClass()->GetName()),
				FText::FromString(FString::Join(CompatibleFormatStrings, TEXT(","))));
		}
		return TSF_Invalid;
	}

	// Return what ETextureSourceFormat corresponds to this EPixelFormat (must match the conversion capabilities of UTextureRenderTarget::UpdateTexture) : 
	ERawImageFormat::Type RawFormat = FImageCoreUtils::GetRawImageFormatForPixelFormat(InFormat);
	ETextureSourceFormat TextureFormat = FImageCoreUtils::ConvertToTextureSourceFormat(RawFormat);

	return TextureFormat;
}

ETextureRenderTargetSampleCount UUnrealURPRenderTarget::GetSampleCount() const
{
	// Note: MSAA is currently only supported in UCanvasRenderTarget2D
	return ETextureRenderTargetSampleCount::RTSC_1;
}
#endif

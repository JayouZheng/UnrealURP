// Copyright Jayou, Inc. All Rights Reserved.

#include "UnrealURPManager.h"
#include "UnrealURPRenderer.h"
#include "UnrealURPPassContext.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "SingleCustomMeshPass.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "UnrealURPRenderTarget.h"
#include "MaterialShared.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"

bool ShouldCreateNaniteProxy(UStaticMeshComponent* InComp)
{
	if (InComp->bDisallowNanite || InComp->GetScene() == nullptr)
	{
		// Regardless of the static mesh asset supporting Nanite, this component does not want Nanite to be used
		return false;
	}

	// Whether or not to allow Nanite for this component
#if WITH_EDITORONLY_DATA
	const bool bAllowNanite = !InComp->bDisplayNaniteFallbackMesh;
#else
	const bool bAllowNanite = true;
#endif

	EShaderPlatform ShaderPlatform = InComp->GetScene() ? InComp->GetScene()->GetShaderPlatform() : GMaxRHIShaderPlatform;
	return bAllowNanite && UseNanite(ShaderPlatform) && InComp->HasValidNaniteData();
}

UUnrealURPManager::UUnrealURPManager()
{
}

void UUnrealURPManager::Tick(float DeltaTime)
{
	for (auto& Comp : CustomMeshPassComponents)
	{
		if (Comp.IsValid() && !Comp->bDeferredRegistered)
		{
			DeferredRegisterCustomMeshPass(Comp.Get());
		}
	}

#if WITH_EDITOR
	for (auto& Comp : CustomMeshPassComponents)
	{
		if (Comp.IsValid())
		{
			UpdatePassCamera(Comp->GraphicPassParameters.PassName, Comp->GraphicPassParameters.Camera);
		}
	}
#endif
}

TStatId UUnrealURPManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUnrealURPManager, STATGROUP_Tickables);
}

bool UUnrealURPManager::DoesSupportWorldType(EWorldType::Type WorldType) const
{
#if WITH_EDITOR
	if (WorldType == EWorldType::EditorPreview)
	{
		return false;
	}
#endif // WITH_EDITOR

	return WorldType == EWorldType::Game || WorldType == EWorldType::Editor || WorldType == EWorldType::PIE;
}

void UUnrealURPManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UWorld* World = GetWorld();
	check(World != nullptr);

	UnrealURPRenderer = FSceneViewExtensions::NewExtension<FUnrealURPRenderer>(World);
}

void UUnrealURPManager::Deinitialize()
{
	Super::Deinitialize();

	UnrealURPRenderer->DeletePassContexts_GameThread();
	UnrealURPRenderer->Deinitialize();
}

void UUnrealURPManager::AddGameObjectToPass(const FRenderableGameObject& InGameObject, FName InPassName)
{
	TArray<USceneComponent*> OutComps;
	TArray<USceneComponent*> RenderComps;

	if (InGameObject.Actor && InGameObject.Actor->GetRootComponent())
	{
		InGameObject.Actor->GetRootComponent()->GetChildrenComponents(true, OutComps);
		OutComps.Add(InGameObject.Actor->GetRootComponent());
		RenderComps.Append(OutComps);
	}

	for (auto& RenderComp : RenderComps)
	{
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(RenderComp))
		{
			FRenderableComponent RenderableComponent;
			RenderableComponent.Component = PrimComp;
			RenderableComponent.MultiPassFlags = InGameObject.MultiPassFlags;
			AddRenderableComponentToPass(RenderableComponent, InPassName);
		}
	}
}

void UUnrealURPManager::AddGameObjectsToPass(const TArray<FRenderableGameObject>& InGameObjects, FName InPassName)
{
	for (auto& GameObject : InGameObjects)
	{
		AddGameObjectToPass(GameObject, InPassName);
	}
}

void UUnrealURPManager::RemoveGameObjectFromPass(const FRenderableGameObject& InGameObject, FName InPassName)
{
	TArray<USceneComponent*> OutComps;
	TArray<USceneComponent*> RenderComps;

	if (InGameObject.Actor && InGameObject.Actor->GetRootComponent())
	{
		InGameObject.Actor->GetRootComponent()->GetChildrenComponents(true, OutComps);
		OutComps.Add(InGameObject.Actor->GetRootComponent());
		RenderComps.Append(OutComps);
	}

	for (auto& RenderComp : RenderComps)
	{
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(RenderComp))
		{
			FRenderableComponent RenderableComponent;
			RenderableComponent.Component = PrimComp;
			RenderableComponent.MultiPassFlags = InGameObject.MultiPassFlags;		
			RemoveRenderableComponentFromPass(RenderableComponent, InPassName);
		}
	}
}

void UUnrealURPManager::RemoveGameObjectsFromPass(const TArray<FRenderableGameObject>& InGameObjects, FName InPassName)
{
	for (auto& GameObject : InGameObjects)
	{
		RemoveGameObjectFromPass(GameObject, InPassName);
	}
}

void UUnrealURPManager::AddRenderableComponentToPass(const FRenderableComponent& InComp, FName InPassName)
{
	if (UStaticMeshComponent* StaticComp = Cast<UStaticMeshComponent>(InComp.Component))
	{
		if (ShouldCreateNaniteProxy(StaticComp))
		{
			UnrealURPRenderer->AddNaniteFallbackToPass_GameThread(StaticComp, InComp.MultiPassFlags, InPassName);

			for (auto& Comp : CustomMeshPassComponents)
			{
				if (Comp.IsValid() && Comp->GraphicPassParameters.PassName == InPassName)
				{
					if (Comp->GraphicPassParameters.NaniteCPDMarkIndex > -1 && Comp->GraphicPassParameters.NaniteCPDMarkValue > -1)
					{
						if (Comp->GraphicPassParameters.NaniteCPDMarkIndex >= 0 && Comp->GraphicPassParameters.NaniteCPDMarkIndex < FCustomPrimitiveData::NumCustomPrimitiveDataFloats)
						{
							float OldCPD = 0.0f;
							if (InComp.Component->GetCustomPrimitiveData().Data.IsValidIndex(Comp->GraphicPassParameters.NaniteCPDMarkIndex))
							{
								OldCPD = InComp.Component->GetCustomPrimitiveData().Data[Comp->GraphicPassParameters.NaniteCPDMarkIndex];
							}
							float TargetCPD = float((uint32)OldCPD | (uint32)Comp->GraphicPassParameters.NaniteCPDMarkValue);
							InComp.Component->SetCustomPrimitiveDataFloat(Comp->GraphicPassParameters.NaniteCPDMarkIndex, Comp->GraphicPassParameters.NaniteCPDMarkValue);
						}
					}
				}
			}
		}
		else
		{
			UnrealURPRenderer->AddRenderObjectToPass_GameThread(StaticComp, InComp.MultiPassFlags, InPassName);
		}
	}

	if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(InComp.Component))
	{
		UnrealURPRenderer->AddRenderObjectToPass_GameThread(SkeletalComp, InComp.MultiPassFlags, InPassName);
	}

	if (UNiagaraComponent* NiagaraComp = Cast<UNiagaraComponent>(InComp.Component))
	{
		UnrealURPRenderer->AddRenderObjectToPass_GameThread(NiagaraComp, InComp.MultiPassFlags, InPassName);
	}

	if (UWidgetComponent* WidgetComp = Cast<UWidgetComponent>(InComp.Component))
	{
		UnrealURPRenderer->AddRenderObjectToPass_GameThread(WidgetComp, InComp.MultiPassFlags, InPassName);
	}
}

void UUnrealURPManager::AddRenderableComponentsToPass(const TArray<FRenderableComponent>& InComps, FName InPassName)
{
	for (auto& Comp : InComps)
	{
		AddRenderableComponentToPass(Comp, InPassName);
	}
}

void UUnrealURPManager::RemoveRenderableComponentFromPass(const FRenderableComponent& InComp, FName InPassName)
{
	UnrealURPRenderer->RemoveRenderObjectFromPass_GameThread(Get_PrimCompId(InComp.Component), InPassName);

	if (UStaticMeshComponent* PrimComp = Cast<UStaticMeshComponent>(InComp.Component))
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
		if (PrimComp->ShouldCreateNaniteProxy())
#else //ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1
		if (ShouldCreateNaniteProxy(PrimComp))
#endif
		{
			for (auto& Comp : CustomMeshPassComponents)
			{
				if (Comp.IsValid() && Comp->GraphicPassParameters.PassName == InPassName)
				{
					if (Comp->GraphicPassParameters.NaniteCPDMarkIndex > -1 && Comp->GraphicPassParameters.NaniteCPDMarkValue > -1)
					{
						if (InComp.Component->GetCustomPrimitiveData().Data.IsValidIndex(Comp->GraphicPassParameters.NaniteCPDMarkIndex))
						{
							InComp.Component->SetCustomPrimitiveDataFloat(Comp->GraphicPassParameters.NaniteCPDMarkIndex, 0.0f);
						}
					}
				}
			}
		}
	}
}

void UUnrealURPManager::RemoveRenderableComponentsFromPass(const TArray<FRenderableComponent>& InComps, FName InPassName)
{
	for (auto& Comp : InComps)
	{
		RemoveRenderableComponentFromPass(Comp, InPassName);
	}
}

void UUnrealURPManager::AddPrimitiveComponentToPass(UPrimitiveComponent* InPrimComps, FName InPassName)
{
	FRenderableComponent RenderableComponent;
	RenderableComponent.Component = InPrimComps;
	AddRenderableComponentToPass(RenderableComponent, InPassName);
}

void UUnrealURPManager::AddPrimitiveComponentsToPass(const TArray<UPrimitiveComponent*>& InPrimComps, FName InPassName)
{
	for (auto& PrimComp : InPrimComps)
	{
		AddPrimitiveComponentToPass(PrimComp, InPassName);
	}
}

void UUnrealURPManager::RemovePrimitiveComponentFromPass(UPrimitiveComponent* InPrimComp, FName InPassName)
{
	FRenderableComponent RenderableComponent;
	RenderableComponent.Component = InPrimComp;
	RemoveRenderableComponentFromPass(RenderableComponent, InPassName);
}

void UUnrealURPManager::RemovePrimitiveComponentsFromPass(const TArray<UPrimitiveComponent*>& InPrimComps, FName InPassName)
{
	for (auto& PrimComp : InPrimComps)
	{
		RemovePrimitiveComponentFromPass(PrimComp, InPassName);
	}
}

void UUnrealURPManager::SuspendPass(FName InPassName)
{
	UnrealURPRenderer->SuspendPass_GameThread(InPassName);
}

void UUnrealURPManager::ResumePass(FName InPassName)
{
	UnrealURPRenderer->ResumePass_GameThread(InPassName);
}

void UUnrealURPManager::UpdatePassPriority(FName InPassName, int32 InPassPriority)
{
	UnrealURPRenderer->UpdatePassPriority_GameThread(InPassName, InPassPriority);
}

void UUnrealURPManager::UpdatePassCamera(FName InPassName, ACameraActor* InCamera)
{
	if (InCamera != nullptr && IsValid(InCamera))
	{
		FMinimalViewInfo ViewInfo;
		InCamera->GetCameraComponent()->GetCameraView(0.0f, ViewInfo);

		FMatrix View;
		FMatrix Proj;
		FMatrix CustomViewProj;
		UGameplayStatics::GetViewProjectionMatrix(ViewInfo, View, Proj, CustomViewProj);

		UnrealURPRenderer->UpdatePassCamera_GameThread(InPassName, CustomViewProj);
	}
	else
	{
		bool bSet = false;
		FMatrix CustomViewProj;
		for (auto& Comp : CustomMeshPassComponents)
		{
			if (Comp.IsValid() && Comp->GraphicPassParameters.PassName == InPassName)
			{
				if (Comp->GraphicPassParameters.Camera)
				{
					FMinimalViewInfo ViewInfo;
					Comp->GraphicPassParameters.Camera->GetCameraComponent()->GetCameraView(0.0f, ViewInfo);

					FMatrix View;
					FMatrix Proj;			
					UGameplayStatics::GetViewProjectionMatrix(ViewInfo, View, Proj, CustomViewProj);
					bSet = true;
				}
			}
		}

		if (bSet)
		{
			UnrealURPRenderer->UpdatePassCamera_GameThread(InPassName, CustomViewProj);
		}
	}
}

void UUnrealURPManager::ResizeRenderTarget2D(FName InPassName, int32 InSlotIndex, int32 InSizeX, int32 InSizeY)
{
	for (auto& Comp : CustomMeshPassComponents)
	{
		if (!Comp.IsValid())
			continue;

		if (Comp->GraphicPassParameters.PassName != InPassName)
			continue;

		if (!Comp->GraphicPassParameters.bUseRenderTargetAssets)
			continue;

		if (!Comp->GraphicPassParameters.RenderTargetAssets.IsValidIndex(InSlotIndex))
			continue;

		if (!Comp->GraphicPassParameters.RenderTargetAssets[InSlotIndex].RenderTargetAsset)
			continue;

		auto& RT = Comp->GraphicPassParameters.RenderTargetAssets[InSlotIndex].RenderTargetAsset;
		RT->ResizeTarget(InSizeX, InSizeY);
	}
}

void UUnrealURPManager::RegisterCustomMeshPass(UCustomMeshPassBaseComponent* InPassComp)
{
	if (InPassComp == nullptr)
		return;

	for (auto& Comp : CustomMeshPassComponents)
	{
		if (Comp.IsValid() && Comp->GraphicPassParameters.PassName == InPassComp->GraphicPassParameters.PassName)
		{
			return;
		}
	}

	if (InPassComp->GraphicPassParameters.bUseNaniteFallback == false && InPassComp->GraphicPassParameters.NaniteCPDMarkIndex != -1)
	{
		for (int32 Index = 0; Index < CPD_MARK_VALUE_BIT_NUM; Index++)
		{
			if (CPDMarkValueTable[Index].PassName == InPassComp->GraphicPassParameters.PassName)
			{
				CPDMarkValueTable[Index].ReferenceCount++;
				InPassComp->GraphicPassParameters.NaniteCPDMarkValue = 1u << Index;
				break;
			}
		}
		
		if (InPassComp->GraphicPassParameters.NaniteCPDMarkValue == -1)
		{
			for (int32 Index = 0; Index < CPD_MARK_VALUE_BIT_NUM; Index++)
			{
				if (CPDMarkValueTable[Index].ReferenceCount == 0)
				{
					CPDMarkValueTable[Index].PassName = InPassComp->GraphicPassParameters.PassName;
					CPDMarkValueTable[Index].ReferenceCount++;
					InPassComp->GraphicPassParameters.NaniteCPDMarkValue = 1u << Index;
					break;
				}
			}
		}
	}
	
	CustomMeshPassComponents.Add(InPassComp);
	AddPassContext(FPassContext::Create(InPassComp->GraphicPassParameters));
}

void UUnrealURPManager::UnRegisterCustomMeshPass(UCustomMeshPassBaseComponent* InPassComp)
{
	if (InPassComp == nullptr)
		return;

	int32 RemoveIndex = -1;
	for (int32 Index = 0; Index < CustomMeshPassComponents.Num(); Index++)
	{
		if (CustomMeshPassComponents[Index].IsValid() && CustomMeshPassComponents[Index]->GraphicPassParameters.PassName == InPassComp->GraphicPassParameters.PassName)
		{
			RemoveIndex = Index;
			break;
		}
	}

	if (RemoveIndex != -1)
	{
		if (InPassComp->GraphicPassParameters.bUseNaniteFallback == false && InPassComp->GraphicPassParameters.NaniteCPDMarkIndex != -1)
		{
			for (int32 Index = 0; Index < CPD_MARK_VALUE_BIT_NUM; Index++)
			{
				if (CPDMarkValueTable[Index].PassName == InPassComp->GraphicPassParameters.PassName)
				{
					CPDMarkValueTable[Index].ReferenceCount--;
					InPassComp->GraphicPassParameters.NaniteCPDMarkValue = -1;
					break;
				}
			}
		}

		RemovePassContext(InPassComp->GraphicPassParameters.PassName);
		CustomMeshPassComponents.RemoveAt(RemoveIndex);
	}	
}

UUnrealURPManager* UUnrealURPManager::GetUnrealURPManager(const UWorld* InWorld)
{
	if (InWorld)
	{
		return InWorld->GetSubsystem<UUnrealURPManager>();
	}

	return nullptr;
}

void UUnrealURPManager::AddPassContext(FPassContext* InPassContext)
{
	UnrealURPRenderer->AddPassContext_GameThread(InPassContext);
}

void UUnrealURPManager::RemovePassContext(const FName& InPassName)
{
	auto RemoveComp = CustomMeshPassComponents.FindByPredicate(
		[&](const auto& InComp) { return InComp->GraphicPassParameters.PassName == InPassName; });

	if (RemoveComp->IsValid())
	{
		RemoveGameObjectsFromPass(RemoveComp->Get()->GraphicPassParameters.GameObjects, InPassName);
	}

	UnrealURPRenderer->RemovePassContext_GameThread(InPassName);
}

void UUnrealURPManager::DeferredRegisterCustomMeshPass(UCustomMeshPassBaseComponent* InPassComp)
{
	AddGameObjectsToPass(InPassComp->GraphicPassParameters.GameObjects, InPassComp->GraphicPassParameters.PassName);

	InPassComp->bDeferredRegistered = true;
	InPassComp->GraphicPassParameters.NaniteCPDMarkValue_DebugOnly = InPassComp->GraphicPassParameters.NaniteCPDMarkValue;
}

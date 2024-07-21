// Copyright Jayou, Inc. All Rights Reserved.


#include "MultiCustomMeshPass.h"
#include "UnrealURPPassContext.h"
#include "UnrealURP.h"
#include "UnrealURPManager.h"

UMultiCustomMeshPassComponent::UMultiCustomMeshPassComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GraphicPassParameters.bEnableMultiPassSupport = true;
}

void UMultiCustomMeshPassComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UMultiCustomMeshPassComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UMultiCustomMeshPassComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	Super::CreateRenderState_Concurrent(Context);

	UUnrealURPManager* Manager = UUnrealURPManager::GetUnrealURPManager(GetWorld());

	if (Manager != nullptr)
	{
		Manager->RegisterCustomMeshPass(this);
	}

	bDeferredRegistered = false;
}

void UMultiCustomMeshPassComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();

	UUnrealURPManager* Manager = UUnrealURPManager::GetUnrealURPManager(GetWorld());

	if (Manager != nullptr)
	{
		Manager->UnRegisterCustomMeshPass(this);
	}
}

void UMultiCustomMeshPassComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UMultiCustomMeshPassComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

// Sets default values
AMultiCustomMeshPass::AMultiCustomMeshPass()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	MultiCustomMeshPassComponent = CreateDefaultSubobject<UMultiCustomMeshPassComponent>(TEXT("MultiCustomMeshPassComponent"));
}

// Called when the game starts or when spawned
void AMultiCustomMeshPass::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AMultiCustomMeshPass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

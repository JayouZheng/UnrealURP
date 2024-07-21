// Copyright Jayou, Inc. All Rights Reserved.


#include "SingleCustomMeshPass.h"
#include "UnrealURPPassContext.h"
#include "UnrealURP.h"
#include "UnrealURPManager.h"

USingleCustomMeshPassComponent::USingleCustomMeshPassComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GraphicPassParameters.bEnableMultiPassSupport = false;
}

void USingleCustomMeshPassComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USingleCustomMeshPassComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void USingleCustomMeshPassComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	Super::CreateRenderState_Concurrent(Context);

	UUnrealURPManager* Manager = UUnrealURPManager::GetUnrealURPManager(GetWorld());

	if (Manager != nullptr)
	{
		Manager->RegisterCustomMeshPass(this);
	}

	bDeferredRegistered = false;
}

void USingleCustomMeshPassComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();

	UUnrealURPManager* Manager = UUnrealURPManager::GetUnrealURPManager(GetWorld());

	if (Manager != nullptr)
	{
		Manager->UnRegisterCustomMeshPass(this);
	}
}

void USingleCustomMeshPassComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USingleCustomMeshPassComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

// Sets default values
ASingleCustomMeshPass::ASingleCustomMeshPass()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SingleCustomMeshPassComponent = CreateDefaultSubobject<USingleCustomMeshPassComponent>(TEXT("SingleCustomMeshPassComponent"));
}

// Called when the game starts or when spawned
void ASingleCustomMeshPass::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ASingleCustomMeshPass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

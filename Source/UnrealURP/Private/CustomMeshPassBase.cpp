// Copyright Jayou, Inc. All Rights Reserved.

#include "CustomMeshPassBase.h"
#include "UnrealURPPassContext.h"
#include "UnrealURP.h"
#include "UnrealURPManager.h"

UCustomMeshPassBaseComponent::UCustomMeshPassBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	bDeferredRegistered = false;
}

void UCustomMeshPassBaseComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCustomMeshPassBaseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UCustomMeshPassBaseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCustomMeshPassBaseComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

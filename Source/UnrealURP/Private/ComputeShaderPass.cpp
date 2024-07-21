// Copyright Jayou, Inc. All Rights Reserved.

#include "ComputeShaderPass.h"
#include "UnrealURPPassContext.h"
#include "UnrealURP.h"
#include "UnrealURPManager.h"

UComputeShaderPassComponent::UComputeShaderPassComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

void UComputeShaderPassComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UComputeShaderPassComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UComputeShaderPassComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UComputeShaderPassComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

// Sets default values
AComputeShaderPass::AComputeShaderPass()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	ComputeShaderPassComponent = CreateDefaultSubobject<UComputeShaderPassComponent>(TEXT("ComputeShaderPassComponent"));
}

// Called when the game starts or when spawned
void AComputeShaderPass::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AComputeShaderPass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

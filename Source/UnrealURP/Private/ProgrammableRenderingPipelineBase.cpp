// Copyright Jayou, Inc. All Rights Reserved.


#include "ProgrammableRenderingPipelineBase.h"
#include "UnrealURPPassContext.h"
#include "UnrealURP.h"
#include "UnrealURPManager.h"

// Sets default values
AProgrammableRenderingPipelineBase::AProgrammableRenderingPipelineBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AProgrammableRenderingPipelineBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AProgrammableRenderingPipelineBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	Render(DeltaTime);
}

void AProgrammableRenderingPipelineBase::AddPass()
{

}


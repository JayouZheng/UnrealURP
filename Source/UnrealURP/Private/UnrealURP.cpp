// Copyright Jayou, Inc. All Rights Reserved.

#include "UnrealURP.h"
#include "Interfaces/IPluginManager.h"
#include "UnrealURPManager.h"

#define LOCTEXT_NAMESPACE "FUnrealURPModule"

DEFINE_LOG_CATEGORY(LogGBBP);

void FUnrealURPModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("UnrealURP"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/UnrealURP"), PluginShaderDir);
}

void FUnrealURPModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealURPModule, UnrealURP)
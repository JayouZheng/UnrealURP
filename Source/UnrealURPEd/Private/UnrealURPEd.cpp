// Copyright Jayou, Inc. All Rights Reserved.

#include "UnrealURPEd.h"
#include "MultiPassFlagsCustomization.h"

#define LOCTEXT_NAMESPACE "FUnrealURPEdModule"

void FUnrealURPEdModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyTypeLayout("MultiPassFlags", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMultiPassFlagsCustomization::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FUnrealURPEdModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout("MultiPassFlags");
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealURPEdModule, UnrealURPEd)
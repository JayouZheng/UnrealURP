// Copyright Jayou, Inc. All Rights Reserved.

#include "UnrealURPRenderTargetFactoryNew.h"
#include "UnrealURPRenderTarget.h"

UUnrealURPRenderTargetFactoryNew::UUnrealURPRenderTargetFactoryNew(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UUnrealURPRenderTarget::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;

	Width = 256;
	Height = 256;
	Format = 0;
}


UObject* UUnrealURPRenderTargetFactoryNew::FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn)
{
	// create the new object
	UUnrealURPRenderTarget* Result = NewObject<UUnrealURPRenderTarget>(InParent, Class, Name, Flags);
	check(Result);
	// initialize the resource
	Result->InitAutoFormat( Width, Height );
	return( Result );
}



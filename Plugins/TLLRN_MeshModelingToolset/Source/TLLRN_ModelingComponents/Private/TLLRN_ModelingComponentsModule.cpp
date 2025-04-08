// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModelingComponentsModule.h"
#include "Components/BaseDynamicMeshComponent.h"
#include "Materials/Material.h"
#include "Misc/CoreDelegates.h"

#define LOCTEXT_NAMESPACE "FTLLRN_ModelingComponentsModule"

void FTLLRN_ModelingComponentsModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTLLRN_ModelingComponentsModule::OnPostEngineInit);

	// Ensure that the GeometryFramework module is loaded so that UBaseDynamicMeshComponent materials are configured
	FModuleManager::Get().LoadModule(TEXT("GeometryFramework"));
}

void FTLLRN_ModelingComponentsModule::OnPostEngineInit()
{
	// Replace the standard UBaseDynamicMeshComponent vertex color material with something higher quality.
	// This is done in TLLRN_ModelingComponents module (ie part of TLLRN_MeshModelingToolset plugin) to avoid having to
	// make this Material a special "engine material", which has various undesirable implication
	UMaterial* VertexColorMaterial = LoadObject<UMaterial>(nullptr, TEXT("/TLLRN_MeshModelingToolset/Materials/M_DynamicMeshComponentVtxColor"));
	if (ensure(VertexColorMaterial))
	{
		UBaseDynamicMeshComponent::SetDefaultVertexColorMaterial(VertexColorMaterial);
	}
}

void FTLLRN_ModelingComponentsModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTLLRN_ModelingComponentsModule, TLLRN_ModelingComponents)
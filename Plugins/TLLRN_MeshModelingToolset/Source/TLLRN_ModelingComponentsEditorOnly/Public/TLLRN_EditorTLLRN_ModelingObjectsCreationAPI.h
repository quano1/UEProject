// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_ModelingObjectsCreationAPI.h"

#include "TLLRN_EditorTLLRN_ModelingObjectsCreationAPI.generated.h"

class UInteractiveToolsContext;

/**
 * Implementation of UTLLRN_ModelingObjectsCreationAPI suitable for use in UE Editor.
 * - CreateMeshObject() currently creates a StaticMesh Asset/Actor, a Volume Actor or a DynamicMesh Actor
 * - CreateTextureObject() currently creates a UTexture2D Asset
 * - CreateMaterialObject() currently creates a UMaterial Asset
 * 
 * This is intended to be registered in the ToolsContext ContextObjectStore.
 * Static utility functions ::Register() / ::Find() / ::Deregister() can be used to do this in a consistent way.
 * 
 * Several client-provided callbacks can be used to customize functionality (eg in Modeling Mode) 
 *  - GetNewAssetPathNameCallback is called to determine an asset path. This can be used to do
 *    things like pop up an interactive path-selection dialog, use project-defined paths, etc
 *  - OnModelingMeshCreated is broadcast for each new created mesh object
 *  - OnModelingTextureCreated is broadcast for each new created texture object
 *  - OnModelingMaterialCreated is broadcast for each new created material object
 */
UCLASS()
class TLLRN_MODELINGCOMPONENTSEDITORONLY_API UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI : public UTLLRN_ModelingObjectsCreationAPI
{
	GENERATED_BODY()
public:

	// UFUNCTION(BlueprintCallable, Category = "Modeling Objects")
	virtual FTLLRN_CreateMeshObjectResult CreateMeshObject(const FTLLRN_CreateMeshObjectParams& CreateMeshParams) override;

	// UFUNCTION(BlueprintCallable, Category = "Modeling Objects")
	virtual FTLLRN_CreateTextureObjectResult CreateTextureObject(const FTLLRN_CreateTextureObjectParams& CreateTexParams) override;

	// UFUNCTION(BlueprintCallable, Category = "Modeling Objects")
	virtual FTLLRN_CreateMaterialObjectResult CreateMaterialObject(const FTLLRN_CreateMaterialObjectParams& CreateMaterialParams) override;

	// UFUNCTION(BlueprintCallable, Category = "Modeling Objects")
	virtual FTLLRN_CreateActorResult CreateNewActor(const FTLLRN_CreateActorParams& TLLRN_CreateActorParams) override;

	//
	// Non-UFunction variants that support std::move operators
	//

	virtual bool HasMoveVariants() const { return true; }

	virtual FTLLRN_CreateMeshObjectResult CreateMeshObject(FTLLRN_CreateMeshObjectParams&& CreateMeshParams) override;

	virtual FTLLRN_CreateTextureObjectResult CreateTextureObject(FTLLRN_CreateTextureObjectParams&& CreateTexParams) override;

	virtual FTLLRN_CreateMaterialObjectResult CreateMaterialObject(FTLLRN_CreateMaterialObjectParams&& CreateMaterialParams) override;

	virtual FTLLRN_CreateActorResult CreateNewActor(FTLLRN_CreateActorParams&& TLLRN_CreateActorParams) override;


	//
	// Callbacks that editor can hook into to handle asset creation
	//

	DECLARE_DELEGATE_RetVal_ThreeParams(FString, FGetAssetPathNameCallbackSignature, const FString& BaseName, const UWorld* TargetWorld, FString SuggestedFolder);
	FGetAssetPathNameCallbackSignature GetNewAssetPathNameCallback;

	DECLARE_MULTICAST_DELEGATE_OneParam(FModelingMeshCreatedSignature, const FTLLRN_CreateMeshObjectResult& CreatedInfo);
	FModelingMeshCreatedSignature OnModelingMeshCreated;

	DECLARE_MULTICAST_DELEGATE_OneParam(FModelingTextureCreatedSignature, const FTLLRN_CreateTextureObjectResult& CreatedInfo);
	FModelingTextureCreatedSignature OnModelingTextureCreated;

	DECLARE_MULTICAST_DELEGATE_OneParam(FModelingMaterialCreatedSignature, const FTLLRN_CreateMaterialObjectResult& CreatedInfo);
	FModelingMaterialCreatedSignature OnModelingMaterialCreated;

	DECLARE_MULTICAST_DELEGATE_OneParam(FModelingActorCreatedSignature, const FTLLRN_CreateActorResult& CreatedInfo);
	FModelingActorCreatedSignature OnModelingActorCreated;

	//
	// Utility functions to handle registration/unregistration
	//

	static UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI* Register(UInteractiveToolsContext* ToolsContext);
	static UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI* Find(UInteractiveToolsContext* ToolsContext);
	static bool Deregister(UInteractiveToolsContext* ToolsContext);



	//
	// internal implementations called by public functions
	//
	FTLLRN_CreateMeshObjectResult CreateStaticMeshAsset(FTLLRN_CreateMeshObjectParams&& CreateMeshParams);
	FTLLRN_CreateMeshObjectResult CreateVolume(FTLLRN_CreateMeshObjectParams&& CreateMeshParams);
	FTLLRN_CreateMeshObjectResult CreateDynamicMeshActor(FTLLRN_CreateMeshObjectParams&& CreateMeshParams);
	ETLLRN_CreateModelingObjectResult GetNewAssetPath(FString& OutNewAssetPath, const FString& BaseName, const UObject* StoreRelativeToObject, const UWorld* World);

	TArray<UMaterialInterface*> FilterMaterials(const TArray<UMaterialInterface*>& MaterialsIn);
};

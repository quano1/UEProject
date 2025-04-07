// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InteractiveToolBuilder.h"
#include "Templates/SubclassOf.h"
#include "UObject/Interface.h"

#include "TLLRN_UVEditorToolBase.generated.h"

class UTLLRN_UVEditorToolMeshInput;

/** UObject for ITLLRN_UVEditorGenericBuildableTool */
UINTERFACE(MinimalAPI)
class UTLLRN_UVEditorGenericBuildableTool : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface that allows a component to receive various gizmo-specific callbacks while
 * still inheriting from some class other than UGizmoBaseComponent.
 */
class ITLLRN_UVEditorGenericBuildableTool
{
	GENERATED_BODY()
public:
	virtual void SetTargets(const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn) = 0;
};

//~ TODO: We can use this builder for pretty much all our UV tools and remove the other builder classes
/**
 * Simple builder that just instantiates the given class and passes in the targets. Can be used
 * for any UV tools that don't need special handling, as long as they implement ITLLRN_UVEditorGenericBuildableTool.
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UGenericTLLRN_UVEditorToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:

	/**
	 * @param ToolClassIn Must implement ITLLRN_UVEditorGenericBuildableTool.
	 */
	virtual void Initialize(TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn, TSubclassOf<UInteractiveTool> ToolClassIn);

	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

private:
	// This is a pointer so that it can be updated under the builder without
	// having to set it in the mode after initializing targets.
	const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>* Targets = nullptr;

	TSubclassOf<UInteractiveTool> ToolClass = nullptr;
};
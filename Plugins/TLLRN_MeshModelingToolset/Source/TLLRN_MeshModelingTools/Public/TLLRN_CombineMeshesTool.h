// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/TLLRN_MultiSelectionMeshEditingTool.h"
#include "InteractiveToolBuilder.h"
#include "InteractiveToolQueryInterfaces.h" // IInteractiveToolExclusiveToolAPI
#include "TLLRN_MeshOpPreviewHelpers.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "BaseTools/TLLRN_BaseCreateFromSelectedTool.h"
#include "PropertySets/TLLRN_OnAcceptProperties.h"
#include "PropertySets/TLLRN_CreateMeshObjectTypeProperties.h"
#include "TLLRN_CombineMeshesTool.generated.h"

// Forward declarations
struct FMeshDescription;

/**
 *
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_CombineMeshesToolBuilder : public UTLLRN_MultiSelectionMeshEditingToolBuilder
{
	GENERATED_BODY()

public:
	bool bIsDuplicateTool = false;

	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UTLLRN_MultiSelectionMeshEditingTool* CreateNewTool(const FToolBuilderState& SceneState) const override;
};


/**
 * Common properties
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_CombineMeshesToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (TransientToolProperty))
	bool bIsDuplicateMode = false;

	/** Defines the object the tool output is written to. */
	UPROPERTY(EditAnywhere, Category = OutputObject, meta = (DisplayName = "Write To",
		EditCondition = "bIsDuplicateMode == false", EditConditionHides, HideEditConditionToggle))
	ETLLRN_BaseCreateFromSelectedTargetType OutputWriteTo = ETLLRN_BaseCreateFromSelectedTargetType::NewObject;

	/** Base name of the newly generated object to which the output is written to. */
	UPROPERTY(EditAnywhere, Category = OutputObject, meta = (TransientToolProperty, DisplayName = "Name",
		EditCondition = "bIsDuplicateMode || OutputWriteTo == ETLLRN_BaseCreateFromSelectedTargetType::NewObject", EditConditionHides, NoResetToDefault))
	FString OutputNewName;

	/** Name of the existing object to which the output is written to. */
	UPROPERTY(VisibleAnywhere, Category = OutputObject, meta = (TransientToolProperty, DisplayName = "Name",
		EditCondition = "bIsDuplicateMode == false && OutputWriteTo != ETLLRN_BaseCreateFromSelectedTargetType::NewObject", EditConditionHides, NoResetToDefault))
	FString OutputExistingName;
};


/**
 * Simple tool to combine multiple meshes into a single mesh asset
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_CombineMeshesTool : public UTLLRN_MultiSelectionMeshEditingTool, 
	// Disallow auto-accept switch-away because it's easy to accidentally make an extra asset in duplicate mode,
	// and it's not great in combine mode either.
	public IInteractiveToolExclusiveToolAPI
{
	GENERATED_BODY()

public:
	virtual void SetDuplicateMode(bool bDuplicateMode);

	virtual void Setup() override;
	virtual void OnShutdown(EToolShutdownType ShutdownType) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }

protected:

	UPROPERTY()
	TObjectPtr<UTLLRN_CombineMeshesToolProperties> BasicProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_CreateMeshObjectTypeProperties> OutputTypeProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_OnAcceptHandleSourcesPropertiesBase> HandleSourceProperties;

	bool bDuplicateMode;

	void CreateNewAsset();
	void UpdateExistingAsset();

	void BuildCombinedMaterialSet(TArray<UMaterialInterface*>& NewMaterialsOut, TArray<TArray<int32>>& MaterialIDRemapsOut);
};

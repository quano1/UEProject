// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/TLLRN_MultiSelectionMeshEditingTool.h"
#include "InteractiveToolBuilder.h"
#include "InteractiveToolManager.h"
#include "TLLRN_MeshOpPreviewHelpers.h"
#include "PropertySets/TLLRN_OnAcceptProperties.h"
#include "PropertySets/TLLRN_CreateMeshObjectTypeProperties.h"
#include "TLLRN_BaseCreateFromSelectedTool.generated.h"

class UCombinedTransformGizmo;
class UTransformProxy;
class UTLLRN_BaseCreateFromSelectedTool;

/**
 * ToolBuilder for UTLLRN_BaseCreateFromSelectedTool
 */
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_BaseCreateFromSelectedToolBuilder : public UTLLRN_MultiSelectionMeshEditingToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;

	virtual UTLLRN_MultiSelectionMeshEditingTool* CreateNewTool(const FToolBuilderState& SceneState) const override;

public:
	virtual TOptional<int32> MaxComponentsSupported() const { return TOptional<int32>(); }
	virtual int32 MinComponentsSupported() const { return 1; }
};


UENUM()
enum class ETLLRN_BaseCreateFromSelectedTargetType
{
	/** Create and write to a new object with a given name. */
	NewObject,

	/** Write to the first object in the input selection. */
	FirstInputObject,

	/** Write to the last object in the input selection. */
	LastInputObject
};


UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_BaseCreateFromSelectedHandleSourceProperties : public UTLLRN_OnAcceptHandleSourcesProperties
{
	GENERATED_BODY()
public:
	/** Defines the object the tool output is written to. */
	UPROPERTY(EditAnywhere, Category = OutputObject, meta = (DisplayName = "Write To", NoResetToDefault))
	ETLLRN_BaseCreateFromSelectedTargetType OutputWriteTo = ETLLRN_BaseCreateFromSelectedTargetType::NewObject;

	/** Base name of the newly generated object to which the output is written to. */
	UPROPERTY(EditAnywhere, Category = OutputObject, meta = (TransientToolProperty, DisplayName = "Name",
		EditCondition = "OutputWriteTo == ETLLRN_BaseCreateFromSelectedTargetType::NewObject", EditConditionHides, NoResetToDefault))
	FString OutputNewName;

	/** Name of the existing object to which the output is written to. */
	UPROPERTY(VisibleAnywhere, Category = OutputObject, meta = (TransientToolProperty, DisplayName = "Name",
		EditCondition = "OutputWriteTo != ETLLRN_BaseCreateFromSelectedTargetType::NewObject", EditConditionHides, NoResetToDefault))
	FString OutputExistingName;
};

UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_BaseCreateFromSelectedCollisionProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Whether to transfer collision settings and any simple collision shapes from the source object(s) to the new object. */
	UPROPERTY(EditAnywhere, Category = OutputObject)
	bool bTransferCollision = true;
};

/**
 * Properties of UI to adjust input meshes
 */
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_TransformInputsToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** Show transform gizmo in the viewport to allow changing translation, rotation and scale of input meshes. */
	UPROPERTY(EditAnywhere, Category = Transform, meta = (DisplayName = "Show Gizmo"))
	bool bShowTransformGizmo = true;
};


/**
 * UTLLRN_BaseCreateFromSelectedTool is a base Tool (must be subclassed) 
 * that provides support for common functionality in tools that create a new mesh from a selection of one or more existing meshes
 */
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_BaseCreateFromSelectedTool : public UTLLRN_MultiSelectionMeshEditingTool, public UE::Geometry::IDynamicMeshOperatorFactory
{
	GENERATED_BODY()
protected:
	using FFrame3d = UE::Geometry::FFrame3d;
public:
	UTLLRN_BaseCreateFromSelectedTool() = default;

	//
	// InteractiveTool API - generally does not need to be modified by subclasses
	//

	virtual void Setup() override;
	virtual void OnShutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	

protected:

	//
	// UTLLRN_BaseCreateFromSelectedTool API - subclasses typically implement these functions
	//

	/**
	 * After preview is created, this is called to convert inputs and set preview materials
	 * (grouped together because materials may come from inputs)
	 * Subclasses should always implement this.
	 * @param bSetTLLRN_PreviewMesh If true, function may try to set an initial "early" preview mesh to have some initial surface on tool start.  (Not all tools will actually create this.)
	 *						  This boolean is here in case a subclass needs to call this setup function again later (e.g. to change the materials used), when it won't need or want the preview surface to be created
	 */
	virtual void ConvertInputsAndSetPreviewMaterials(bool bSetTLLRN_PreviewMesh = true) { check(false);  }

	/** overload to initialize any added properties in subclasses; called during setup */
	virtual void SetupProperties() {}

	/** overload to save any added properties in the subclasses; called on shutdown */
	virtual void SaveProperties() {}

	/** optional overload to set callbacks on preview, e.g. to visualize results; called after preview is created. */
	virtual void SetPreviewCallbacks() {}

	/** Return the name to be used for generated assets.  Note: Asset name will be prefixed by source actor name if only actor was selected. */
	virtual FString GetCreatedAssetName() const { return TEXT("Generated"); }

	/** Return the name of the action to be used in the Undo stack */
	virtual FText GetActionName() const;

	/** Return the materials to be used on the output mesh on tool accept; defaults to the materials set on the preview */
	virtual TArray<UMaterialInterface*> GetOutputMaterials() const;

	// Override this to control whether the Transfer Collision setting is available
	virtual bool SupportsCollisionTransfer() const
	{
		return true;
	}

	// Override this to control which inputs should transfer collision to the output (if collision transfer is enabled)
	virtual bool KeepCollisionFrom(int32 TargetIndex) const
	{
		return true;
	}


	/**
	 * IDynamicMeshOperatorFactory implementation that subclass must override and implement
	 */
	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override
	{
		check(false);
		return TUniquePtr<UE::Geometry::FDynamicMeshOperator>();
	}

protected:

	/** Helper to build asset names */
	FString PrefixWithSourceNameIfSingleSelection(const FString& AssetName) const;

	// Helpers for managing transform gizoms; typically do not need to be overloaded
	virtual void UpdateGizmoVisibility();
	virtual void SetTransformGizmos();
	virtual void TransformChanged(UTransformProxy* Proxy, FTransform Transform);

	// Helper to generate assets when a result is accepted; typically does not need to be overloaded
	virtual void GenerateAsset(const FDynamicMeshOpResult& Result);

	// Helper to generate assets when a result is accepted; typically does not need to be overloaded
	virtual void UpdateAsset(const FDynamicMeshOpResult& Result, UToolTarget* Target);

	// Which of the transform gizmos to hide, or -1 if all gizmos can be shown
	virtual int32 GetHiddenGizmoIndex() const;

protected:

	UPROPERTY()
	TObjectPtr<UTLLRN_TransformInputsToolProperties> TransformProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_CreateMeshObjectTypeProperties> OutputTypeProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_BaseCreateFromSelectedHandleSourceProperties> HandleSourcesProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_BaseCreateFromSelectedCollisionProperties> CollisionProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_MeshOpPreviewWithBackgroundCompute> Preview;

	UPROPERTY()
	TArray<TObjectPtr<UTransformProxy>> TransformProxies;

	UPROPERTY()
	TArray<TObjectPtr<UCombinedTransformGizmo>> TransformGizmos;
};


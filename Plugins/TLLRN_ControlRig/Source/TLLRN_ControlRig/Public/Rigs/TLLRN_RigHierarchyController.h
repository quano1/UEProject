// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Rigs/TLLRN_RigHierarchy.h"
#include "ReferenceSkeleton.h"
#include "TLLRN_RigHierarchyContainer.h"
#include "Animation/Skeleton.h"
#include "TLLRN_RigHierarchyContainer.h"
#include "TLLRN_RigHierarchyController.generated.h"

UCLASS(BlueprintType)
class TLLRN_CONTROLRIG_API UTLLRN_RigHierarchyController : public UObject
{
	GENERATED_BODY()

public:

	UTLLRN_RigHierarchyController()
	: bReportWarningsAndErrors(true)
	, bSuspendAllNotifications(false)
	, bSuspendSelectionNotifications(false)
	, bSuspendPythonPrinting(false)
	, CurrentInstructionIndex(INDEX_NONE)
	{}

	virtual ~UTLLRN_RigHierarchyController();

	// UObject interface
	virtual void Serialize(FArchive& Ar) override;

	// Returns the hierarchy currently linked to this controller
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	UTLLRN_RigHierarchy* GetHierarchy() const;

	// Sets the hierarchy currently linked to this controller
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	void SetHierarchy(UTLLRN_RigHierarchy* InHierarchy);

	/**
	 * Selects or deselects an element in the hierarchy
	 * @param InKey The key of the element to select
	 * @param bSelect If set to false the element will be deselected
	 * @return Returns true if the selection was applied
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool SelectElement(FTLLRN_RigElementKey InKey, bool bSelect = true, bool bClearSelection = false);

	/**
	 * Deselects or deselects an element in the hierarchy
	 * @param InKey The key of the element to deselect
	 * @return Returns true if the selection was applied
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    bool DeselectElement(FTLLRN_RigElementKey InKey)
	{
		return SelectElement(InKey, false);
	}

	/**
	 * Sets the selection based on a list of keys
	 * @param InKeys The array of keys of the elements to select
	 * @return Returns true if the selection was applied
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    bool SetSelection(const TArray<FTLLRN_RigElementKey>& InKeys, bool bPrintPythonCommand = false);

	/**
	 * Clears the selection
	 * @return Returns true if the selection was applied
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    bool ClearSelection()
	{
		return SetSelection(TArray<FTLLRN_RigElementKey>());
	}
	
	/**
	 * Adds a bone to the hierarchy
	 * @param InName The suggested name of the new bone - will eventually be corrected by the namespace
	 * @param InParent The (optional) parent of the new bone. If you don't need a parent, pass FTLLRN_RigElementKey()
	 * @param InTransform The transform for the new bone - either in local or global space, based on bTransformInGlobal
	 * @param bTransformInGlobal Set this to true if the Transform passed is expressed in global space, false for local space.
	 * @param InBoneType The type of bone to add. This can be used to differentiate between imported bones and user defined bones.
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The key for the newly created bone.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	FTLLRN_RigElementKey AddBone(FName InName, FTLLRN_RigElementKey InParent, FTransform InTransform, bool bTransformInGlobal = true, ETLLRN_RigBoneType InBoneType = ETLLRN_RigBoneType::User, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Adds a null to the hierarchy
	 * @param InName The suggested name of the new null - will eventually be corrected by the namespace
	 * @param InParent The (optional) parent of the new null. If you don't need a parent, pass FTLLRN_RigElementKey()
	 * @param InTransform The transform for the new null - either in local or global null, based on bTransformInGlobal
	 * @param bTransformInGlobal Set this to true if the Transform passed is expressed in global null, false for local null.
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The key for the newly created null.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    FTLLRN_RigElementKey AddNull(FName InName, FTLLRN_RigElementKey InParent, FTransform InTransform, bool bTransformInGlobal = true, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Adds a control to the hierarchy
	 * @param InName The suggested name of the new control - will eventually be corrected by the namespace
	 * @param InParent The (optional) parent of the new control. If you don't need a parent, pass FTLLRN_RigElementKey()
	 * @param InSettings All of the control's settings
	 * @param InValue The value to use for the control
	 * @param InOffsetTransform The transform to use for the offset
	 * @param InShapeTransform The transform to use for the shape
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The key for the newly created control.
	 */
    FTLLRN_RigElementKey AddControl(
    	FName InName,
    	FTLLRN_RigElementKey InParent,
    	FTLLRN_RigControlSettings InSettings,
    	FTLLRN_RigControlValue InValue,
    	FTransform InOffsetTransform = FTransform::Identity,
        FTransform InShapeTransform = FTransform::Identity,
        bool bSetupUndo = true,
        bool bPrintPythonCommand = false
        );

	/**
	 * Adds a control to the hierarchy
	 * @param InName The suggested name of the new control - will eventually be corrected by the namespace
	 * @param InParent The (optional) parent of the new control. If you don't need a parent, pass FTLLRN_RigElementKey()
	 * @param InSettings All of the control's settings
	 * @param InValue The value to use for the control
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @return The key for the newly created control.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController, meta = (DisplayName = "Add Control", ScriptName = "AddControl"))
    FTLLRN_RigElementKey AddControl_ForBlueprint(
        FName InName,
        FTLLRN_RigElementKey InParent,
        FTLLRN_RigControlSettings InSettings,
        FTLLRN_RigControlValue InValue,
        bool bSetupUndo = true,
        bool bPrintPythonCommand = false
    )
	{
		return AddControl(InName, InParent, InSettings, InValue, FTransform::Identity, FTransform::Identity, bSetupUndo, bPrintPythonCommand);
	}

		/**
	 * Adds a control to the hierarchy
	 * @param InName The suggested name of the new animation channel - will eventually be corrected by the namespace
	 * @param InParentControl The parent of the new animation channel.
	 * @param InSettings All of the animation channel's settings
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The key for the newly created animation channel.
	 */
    FTLLRN_RigElementKey AddAnimationChannel(
    	FName InName,
    	FTLLRN_RigElementKey InParentControl,
    	FTLLRN_RigControlSettings InSettings,
        bool bSetupUndo = true,
        bool bPrintPythonCommand = false
    );

	/**
	 * Adds a control to the hierarchy
	 * @param InName The suggested name of the new animation channel - will eventually be corrected by the namespace
	 * @param InParentControl The parent of the new animation channel.
	 * @param InSettings All of the animation channel's settings
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @return The key for the newly created animation channel.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController, meta = (DisplayName = "Add Control", ScriptName = "AddAnimationChannel"))
    FTLLRN_RigElementKey AddAnimationChannel_ForBlueprint(
        FName InName,
        FTLLRN_RigElementKey InParentControl,
        FTLLRN_RigControlSettings InSettings,
        bool bSetupUndo = true,
        bool bPrintPythonCommand = false
    )
	{
		return AddAnimationChannel(InName, InParentControl, InSettings, bSetupUndo, bPrintPythonCommand);
	}

	/**
	 * Adds a curve to the hierarchy
	 * @param InName The suggested name of the new curve - will eventually be corrected by the namespace
	 * @param InValue The value to use for the curve
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The key for the newly created curve.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    FTLLRN_RigElementKey AddCurve(
        FName InName,
        float InValue = 0.f,
        bool bSetupUndo = true,
		bool bPrintPythonCommand = false
        );

	/**
	* Adds a physics element to the hierarchy
	* @param InName The suggested name of the new physics element - will eventually be corrected by the namespace
	* @param InParent The (optional) parent of the new physics element. If you don't need a parent, pass FTLLRN_RigElementKey()
	* @param InSolver The guid identifying the solver to use
	* @param InSettings All of the physics element's settings
	* @param InLocalTransform The transform for the new physics element - in the space of the provided parent
	* @param bSetupUndo If set to true the stack will record the change for undo / redo
	* @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	* @return The key for the newly created physics element.
	*/
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    FTLLRN_RigElementKey AddPhysicsElement(
    	FName InName,
    	FTLLRN_RigElementKey InParent,
    	FTLLRN_RigPhysicsSolverID InSolver,
        FTLLRN_RigPhysicsSettings InSettings,
    	FTransform InLocalTransform,
    	bool bSetupUndo = false,
		bool bPrintPythonCommand = false);

	/**
	* Adds an reference to the hierarchy
	* @param InName The suggested name of the new reference - will eventually be corrected by the namespace
	* @param InParent The (optional) parent of the new reference. If you don't need a parent, pass FTLLRN_RigElementKey()
	* @param InDelegate The delegate to use to pull the local transform
	* @param bSetupUndo If set to true the stack will record the change for undo / redo
	* @return The key for the newly created reference.
	*/
    FTLLRN_RigElementKey AddReference(
        FName InName,
        FTLLRN_RigElementKey InParent,
        FRigReferenceGetWorldTransformDelegate InDelegate,
        bool bSetupUndo = false);

	/**
	 * Adds a connector to the hierarchy
	 * @param InName The suggested name of the new connector - will eventually be corrected by the namespace
	 * @param InSettings All of the connector's settings
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The key for the newly created bone.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	FTLLRN_RigElementKey AddConnector(FName InName, FTLLRN_RigConnectorSettings InSettings = FTLLRN_RigConnectorSettings(), bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Adds a socket to the hierarchy
	 * @param InName The suggested name of the new socket - will eventually be corrected by the namespace
	 * @param InParent The (optional) parent of the new null. If you don't need a parent, pass FTLLRN_RigElementKey()
	 * @param InTransform The transform for the new socket - either in local or global space, based on bTransformInGlobal
	 * @param bTransformInGlobal Set this to true if the Transform passed is expressed in global space, false for local space.
	 * @param InColor The color of the socket
	 * @param InDescription The description of the socket
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The key for the newly created bone.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	FTLLRN_RigElementKey AddSocket(FName InName, FTLLRN_RigElementKey InParent, FTransform InTransform, bool bTransformInGlobal = true, const FLinearColor& InColor = FLinearColor::White, const FString& InDescription = TEXT(""), bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Adds a socket to the first determined root bone the hierarchy
	 * @return The key for the newly created bone (or an invalid key).
	 */
	FTLLRN_RigElementKey AddDefaultRootSocket();
	/**
	 * Returns the control settings of a given control
	 * @param InKey The key of the control to receive the settings for
	 * @return The settings of the given control
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	FTLLRN_RigControlSettings GetControlSettings(FTLLRN_RigElementKey InKey) const;

	/**
	 * Sets a control's settings given a control key
	 * @param InKey The key of the control to set the settings for
	 * @param The settings to set
	 * @return Returns true if the settings have been set correctly
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    bool SetControlSettings(FTLLRN_RigElementKey InKey, FTLLRN_RigControlSettings InSettings, bool bSetupUndo = false) const;

	/**
	 * Imports an existing skeleton to the hierarchy
	 * @param InSkeleton The skeleton to import
	 * @param InNameSpace The namespace to prefix the bone names with
	 * @param bReplaceExistingBones If true existing bones will be removed
	 * @param bRemoveObsoleteBones If true bones non-existent in the skeleton will be removed from the hierarchy
	 * @param bSelectBones If true the bones will be selected upon import
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @return The keys of the imported elements
	 */
	TArray<FTLLRN_RigElementKey> ImportBones(
		const FReferenceSkeleton& InSkeleton,
		const FName& InNameSpace,
		bool bReplaceExistingBones = true,
		bool bRemoveObsoleteBones = true,
		bool bSelectBones = false,
		bool bSetupUndo = false);

	/**
	 * Imports an existing skeleton to the hierarchy
	 * @param InSkeleton The skeleton to import
	 * @param InNameSpace The namespace to prefix the bone names with
	 * @param bReplaceExistingBones If true existing bones will be removed
	 * @param bRemoveObsoleteBones If true bones non-existent in the skeleton will be removed from the hierarchy
	 * @param bSelectBones If true the bones will be selected upon import
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The keys of the imported elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	TArray<FTLLRN_RigElementKey> ImportBones(
		USkeleton* InSkeleton,
		FName InNameSpace = NAME_None,
		bool bReplaceExistingBones = true,
		bool bRemoveObsoleteBones = true,
		bool bSelectBones = false,
		bool bSetupUndo = false,
		bool bPrintPythonCommand = false);

#if WITH_EDITOR
	/**
	 * Imports an existing skeleton to the hierarchy
	 * @param InAssetPath The path to the uasset to import from
	 * @param InNameSpace The namespace to prefix the bone names with
	 * @param bReplaceExistingBones If true existing bones will be removed
	 * @param bRemoveObsoleteBones If true bones non-existent in the skeleton will be removed from the hierarchy
	 * @param bSelectBones If true the bones will be selected upon import
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @return The keys of the imported elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    TArray<FTLLRN_RigElementKey> ImportBonesFromAsset(
        FString InAssetPath,
        FName InNameSpace = NAME_None,
        bool bReplaceExistingBones = true,
        bool bRemoveObsoleteBones = true,
        bool bSelectBones = false,
        bool bSetupUndo = false);
#endif

	/**
	 * Imports all curves from an anim curve metadata object to the hierarchy
	 * @param InAnimCurvesMetadata The anim curve metadata object to import the curves from
	 * @param InNameSpace The namespace to prefix the bone names with
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @return The keys of the imported elements
	 */
	TArray<FTLLRN_RigElementKey> ImportCurves(
		UAnimCurveMetaData* InAnimCurvesMetadata, 
		FName InNameSpace = NAME_None,
		bool bSetupUndo = false);

	/**
	 * Imports all curves from a skeleton to the hierarchy
	 * @param InSkeleton The skeleton to import the curves from
	 * @param InNameSpace The namespace to prefix the bone names with
	 * @param bSelectCurves If true the curves will be selected upon import
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @return The keys of the imported elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	TArray<FTLLRN_RigElementKey> ImportCurves(
		USkeleton* InSkeleton, 
		FName InNameSpace = NAME_None,  
		bool bSelectCurves = false,
		bool bSetupUndo = false,
		bool bPrintPythonCommand = false);

	/**
	 * Imports all curves from a skeletalmesh to the hierarchy
	 * @param InSkeletalMesh The skeletalmesh to import the curves from
	 * @param InNameSpace The namespace to prefix the bone names with
	 * @param bSelectCurves If true the curves will be selected upon import
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @return The keys of the imported elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	TArray<FTLLRN_RigElementKey> ImportCurvesFromSkeletalMesh(
		USkeletalMesh* InSkeletalMesh, 
		FName InNameSpace = NAME_None,  
		bool bSelectCurves = false,
		bool bSetupUndo = false,
		bool bPrintPythonCommand = false);

#if WITH_EDITOR
	/**
	 * Imports all curves from a skeleton to the hierarchy
	 * @param InAssetPath The path to the uasset to import from
	 * @param InNameSpace The namespace to prefix the bone names with
	 * @param bSelectCurves If true the curves will be selected upon import
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @return The keys of the imported elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    TArray<FTLLRN_RigElementKey> ImportCurvesFromAsset(
        FString InAssetPath,
        FName InNameSpace = NAME_None, 
        bool bSelectCurves = false,
        bool bSetupUndo = false);
#endif

	/**
	 * Exports the selected items to text
	 * @return The text representation of the selected items
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	FString ExportSelectionToText() const;

	/**
	 * Exports a list of items to text
	 * @param InKeys The keys to export to text
	 * @return The text representation of the requested elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	FString ExportToText(TArray<FTLLRN_RigElementKey> InKeys) const;

	/**
	 * Imports the content of a text buffer to the hierarchy
	 * @param InContent The string buffer representing the content to import
	 * @param bReplaceExistingElements If set to true existing items will be replaced / updated with the content in the buffer
	 * @param bSelectNewElements If set to true the new elements will be selected
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	TArray<FTLLRN_RigElementKey> ImportFromText(
		FString InContent,
		bool bReplaceExistingElements = false,
		bool bSelectNewElements = true,
		bool bSetupUndo = false,
		bool bPrintPythonCommands = false);

	TArray<FTLLRN_RigElementKey> ImportFromText(
		FString InContent,
		ETLLRN_RigElementType InAllowedTypes,
		bool bReplaceExistingElements = false,
		bool bSelectNewElements = true,
		bool bSetupUndo = false,
		bool bPrintPythonCommands = false);

	/**
	* Imports the content of a RigHierachyContainer (the hierarchy v1 pre 5.0)
	* This is used for backwards compatbility only during load and does not support undo.
	* @param InContainer The input hierarchy container
	*/
    TArray<FTLLRN_RigElementKey> ImportFromHierarchyContainer(const FTLLRN_RigHierarchyContainer& InContainer, bool bIsCopyAndPaste);

	/**
	 * Removes an existing element from the hierarchy
	 * @param InElement The key of the element to remove
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns true if successful.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool RemoveElement(FTLLRN_RigElementKey InElement, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Renames an existing element in the hierarchy
	 * @param InElement The key of the element to rename
	 * @param InName The new name to set for the element
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @param bClearSelection True if the selection should be cleared after a rename
	 * @return Returns the new element key used for the element
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    FTLLRN_RigElementKey RenameElement(FTLLRN_RigElementKey InElement, FName InName, bool bSetupUndo = false, bool bPrintPythonCommand = false, bool bClearSelection = true);

	/**
	 * Changes the element's index within its default parent (or the top level)
	 * @param InElement The key of the element to rename
	 * @param InIndex The new index of the element to take within its default parent (or the top level)
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns true if the element has been reordered accordingly
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool ReorderElement(FTLLRN_RigElementKey InElement, int32 InIndex, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
 	 * Sets the display name on a control
 	 * @param InControl The key of the control to change the display name for
 	 * @param InDisplayName The new display name to set for the control
 	 * @param bRenameElement True if the control should also be renamed
 	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
 	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns the new display name used for the control
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	FName SetDisplayName(FTLLRN_RigElementKey InControl, FName InDisplayName, bool bRenameElement = false, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Adds a new parent to an element. For elements that allow only one parent the parent will be replaced (Same as ::SetParent).
	 * @param InChild The key of the element to add the parent for
	 * @param InParent The key of the new parent to add
	 * @param InWeight The initial weight to give to the parent
	 * @param bMaintainGlobalTransform If set to true the child will stay in the same place spatially, otherwise it will maintain it's local transform (and potential move).
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @return Returns true if successful.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool AddParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, float InWeight = 0.f, bool bMaintainGlobalTransform = true, bool bSetupUndo = false);

	/**
	* Adds a new parent to an element. For elements that allow only one parent the parent will be replaced (Same as ::SetParent).
	* @param InChild The element to add the parent for
	* @param InParent The new parent to add
	* @param InWeight The initial weight to give to the parent
	* @param bMaintainGlobalTransform If set to true the child will stay in the same place spatially, otherwise it will maintain it's local transform (and potential move).
	* @param bRemoveAllParents If set to true all parents of the child will be removed first.
	* @return Returns true if successful.
	*/
	bool AddParent(FTLLRN_RigBaseElement* InChild, FTLLRN_RigBaseElement* InParent, float InWeight = 0.f, bool bMaintainGlobalTransform = true, bool bRemoveAllParents = false);

	/**
	 * Removes an existing parent from an element in the hierarchy. For elements that allow only one parent the element will be unparented (same as ::RemoveAllParents)
	 * @param InChild The key of the element to remove the parent for
	 * @param InParent The key of the parent to remove
	 * @param bMaintainGlobalTransform If set to true the child will stay in the same place spatially, otherwise it will maintain it's local transform (and potential move).
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns true if successful.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool RemoveParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, bool bMaintainGlobalTransform = true, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
 	 * Removes all parents from an element in the hierarchy.
 	 * @param InChild The key of the element to remove all parents for
 	 * @param bMaintainGlobalTransform If set to true the child will stay in the same place spatially, otherwise it will maintain it's local transform (and potential move).
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns true if successful.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool RemoveAllParents(FTLLRN_RigElementKey InChild, bool bMaintainGlobalTransform = true, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Sets a new parent to an element. For elements that allow more than one parent the parent list will be replaced.
	 * @param InChild The key of the element to set the parent for
	 * @param InParent The key of the new parent to set
	 * @param bMaintainGlobalTransform If set to true the child will stay in the same place spatially, otherwise it will maintain it's local transform (and potential move).
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns true if successful.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool SetParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, bool bMaintainGlobalTransform = true, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Adds a new available space to the given control
	 * @param InControl The control to add the available space for
	 * @param InSpace The space to add to the available spaces list
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns true if successful.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool AddAvailableSpace(FTLLRN_RigElementKey InControl, FTLLRN_RigElementKey InSpace, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Removes an available space from the given control
	 * @param InControl The control to remove the available space from
	 * @param InSpace The space to remove from the available spaces list
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns true if successful.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool RemoveAvailableSpace(FTLLRN_RigElementKey InControl, FTLLRN_RigElementKey InSpace, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Reorders an available space for the given control
	 * @param InControl The control to reorder the host for
	 * @param InSpace The space to set the new index for
	 * @param InIndex The new index of the available space
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns true if successful.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool SetAvailableSpaceIndex(FTLLRN_RigElementKey InControl, FTLLRN_RigElementKey InSpace, int32 InIndex, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Adds a new channel host to the animation channel
	 * @note This is just an overload of AddAvailableSpace for readability
	 * @param InChannel The animation channel to add the channel host for
	 * @param InHost The host to add to the channel to
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns true if successful.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool AddChannelHost(FTLLRN_RigElementKey InChannel, FTLLRN_RigElementKey InHost, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Removes an channel host from the animation channel
	 * @note This is just an overload of RemoveAvailableSpace for readability
	 * @param InChannel The animation channel to remove the channel host from
	 * @param InHost The host to remove from the channel from
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return Returns true if successful.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	bool RemoveChannelHost(FTLLRN_RigElementKey InChannel, FTLLRN_RigElementKey InHost, bool bSetupUndo = false, bool bPrintPythonCommand = false);

	/**
	 * Duplicate the given elements
	 * @param InKeys The keys of the elements to duplicate
	 * @param bSelectNewElements If set to true the new elements will be selected
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The keys of the 4d items
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    TArray<FTLLRN_RigElementKey> DuplicateElements(TArray<FTLLRN_RigElementKey> InKeys, bool bSelectNewElements = true, bool bSetupUndo = false, bool bPrintPythonCommands = false);

	/**
	 * Mirrors the given elements
	 * @param InKeys The keys of the elements to mirror
	 * @param InSettings The settings to use for the mirror operation
	 * @param bSelectNewElements If set to true the new elements will be selected
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The keys of the mirrored items
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
    TArray<FTLLRN_RigElementKey> MirrorElements(TArray<FTLLRN_RigElementKey> InKeys, FRigVMMirrorSettings InSettings, bool bSelectNewElements = true, bool bSetupUndo = false, bool bPrintPythonCommands = false);

	/**
	 * Returns the modified event, which can be used to 
	 * subscribe to topological changes happening within the hierarchy.
	 * @return The event used for subscription.
	 */
	FTLLRN_RigHierarchyModifiedEvent& OnModified() { return ModifiedEvent; }

	/**
	 * Reports a warning to the console. This does nothing if bReportWarningsAndErrors is false.
	 * @param InMessage The warning message to report.
	 */
	void ReportWarning(const FString& InMessage) const;

	/**
	 * Reports an error to the console. This does nothing if bReportWarningsAndErrors is false.
	 * @param InMessage The error message to report.
	 */
	void ReportError(const FString& InMessage) const;

	/**
	 * Reports an error to the console and logs a notification to the UI. This does nothing if bReportWarningsAndErrors is false.
	 * @param InMessage The error message to report / notify.
	 */
	void ReportAndNotifyError(const FString& InMessage) const;

	template <typename FmtType, typename... Types>
    void ReportWarningf(const FmtType& Fmt, Types... Args) const
	{
		ReportWarning(FString::Printf(Fmt, Args...));
	}

	template <typename FmtType, typename... Types>
    void ReportErrorf(const FmtType& Fmt, Types... Args) const
	{
		ReportError(FString::Printf(Fmt, Args...));
	}

	template <typename FmtType, typename... Types>
    void ReportAndNotifyErrorf(const FmtType& Fmt, Types... Args) const
	{
		ReportAndNotifyError(FString::Printf(Fmt, Args...));
	}

	UPROPERTY(transient)
	bool bReportWarningsAndErrors;

	/**
	 * Returns a reference to the suspend notifications flag
	 */
	bool& GetSuspendNotificationsFlag() { return bSuspendAllNotifications; }

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchyController)
	TArray<FString> GeneratePythonCommands();

	TArray<FString> GetAddElementPythonCommands(FTLLRN_RigBaseElement* Element) const;

	TArray<FString> GetAddBonePythonCommands(FTLLRN_RigBoneElement* Bone) const;

	TArray<FString> GetAddNullPythonCommands(FTLLRN_RigNullElement* Null) const;

	TArray<FString> GetAddControlPythonCommands(FTLLRN_RigControlElement* Control) const;

	TArray<FString> GetAddCurvePythonCommands(FTLLRN_RigCurveElement* Curve) const;

	TArray<FString> GetAddPhysicsElementPythonCommands(FTLLRN_RigPhysicsElement* PhysicsElement) const;

	TArray<FString> GetAddConnectorPythonCommands(FTLLRN_RigConnectorElement* Connector) const;

	TArray<FString> GetAddSocketPythonCommands(FTLLRN_RigSocketElement* Socket) const;

	TArray<FString> GetSetControlValuePythonCommands(const FTLLRN_RigControlElement* Control, const FTLLRN_RigControlValue& Value, const ETLLRN_RigControlValueType& Type) const;
	
	TArray<FString> GetSetControlOffsetTransformPythonCommands(const FTLLRN_RigControlElement* Control, const FTransform& Offset, bool bInitial = false, bool bAffectChildren = true) const;
	
	TArray<FString> GetSetControlShapeTransformPythonCommands(const FTLLRN_RigControlElement* Control, const FTransform& Transform, bool bInitial = false) const;
#endif
	
private:

	FTLLRN_RigHierarchyModifiedEvent ModifiedEvent;
	void Notify(ETLLRN_RigHierarchyNotification InNotifType, const FTLLRN_RigBaseElement* InElement);
	void HandleHierarchyModified(ETLLRN_RigHierarchyNotification InNotifType, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement) const;

	/**
	 * Returns true if this controller is valid / linked to a valid hierarchy.
	 * @return Returns true if this controller is valid / linked to a valid hierarchy.
	 */
	bool IsValid() const;

	/**
	 * Determine a safe new name for an element. If a name is passed which contains a namespace
	 * (for example "MyNameSpace:Control") we'll remove the namespace and just use the short name
	 * prefixed with the current namespace (for example: "MyOtherNameSpace:Control"). Name clashes
	 * will be resolved for the full name. So two modules with two different namespaces can have
	 * elements with the same short name (for example "MyModuleA:Control" and "MyModuleB:Control").
	 * @param InDesiredName The name provided by the user
	 * @param InElementType The kind of element we are about to create
	 * @param bAllowNameSpace If true the name won't be changed for namespaces
	 * @return The safe name of the element to create.
	 */
	FName GetSafeNewName(const FName& InDesiredName, ETLLRN_RigElementType InElementType, bool bAllowNameSpace = true) const;
	
	/**
	 * Adds a new element to the hierarchy
	 * @param InElementToAdd The new element to add to the hierarchy 
	 * @param InFirstParent The (optional) parent of the new bone. If you don't need a parent, pass nullptr
	 * @param bMaintainGlobalTransform If set to true the child will stay in the same place spatially, otherwise it will maintain it's local transform (and potential move).
	 * @param InDesiredName The original desired name
	 * @return The index of the newly added element
	 */
	int32 AddElement(FTLLRN_RigBaseElement* InElementToAdd, FTLLRN_RigBaseElement* InFirstParent, bool bMaintainGlobalTransform, const FName& InDesiredName = NAME_None);

	/**
	 * Removes an existing element from the hierarchy.
	 * @param InElement The element to remove
	 * @return Returns true if successful.
	 */
	bool RemoveElement(FTLLRN_RigBaseElement* InElement);

	/**
	 * Renames an existing element in the hierarchy
	 * @param InElement The element to rename
	 * @param InName The new name to set for the element
	 * @param bClearSelection True if the selection should be cleared after a rename
	 * @return Returns true if successful.
	 */
    bool RenameElement(FTLLRN_RigBaseElement* InElement, const FName &InName, bool bClearSelection = true);

	/**
	 * Changes the element's index within its default parent (or the top level)
	 * @param InElement The key of the element to rename
	 * @param InIndex The new index of the element to take within its default parent (or the top level)
	 * @return Returns true if the element has been reordered accordingly
	 */
	bool ReorderElement(FTLLRN_RigBaseElement* InElement, int32 InIndex);

	/**
 	 * Sets the display name on a control
 	 * @param InControlElement The element to change the display name for
 	 * @param InDisplayName The new display name to set for the control
 	 * @param bRenameElement True if the control should also be renamed
 	 * @return Returns true if successful.
 	 */
	FName SetDisplayName(FTLLRN_RigControlElement* InControlElement, const FName &InDisplayName, bool bRenameElement = false);

	/**
	 * Removes an existing parent from an element in the hierarchy. For elements that allow only one parent the element will be unparented (same as ::RemoveAllParents)
	 * @param InChild The element to remove the parent for
	 * @param InParent The parent to remove
	 * @param bMaintainGlobalTransform If set to true the child will stay in the same place spatially, otherwise it will maintain it's local transform (and potential move).
	 * @return Returns true if successful.
	 */
	bool RemoveParent(FTLLRN_RigBaseElement* InChild, FTLLRN_RigBaseElement* InParent, bool bMaintainGlobalTransform = true);

	/**
	 * Removes all parents from an element in the hierarchy.
	 * @param InChild The element to remove all parents for
	 * @param bMaintainGlobalTransform If set to true the child will stay in the same place spatially, otherwise it will maintain it's local transform (and potential move).
	 * @return Returns true if successful.
	 */
	bool RemoveAllParents(FTLLRN_RigBaseElement* InChild, bool bMaintainGlobalTransform = true);

	/**
	 * Sets a new parent to an element. For elements that allow more than one parent the parent list will be replaced.
	 * @param InChild The element to set the parent for
	 * @param InParent The new parent to set
	 * @param bMaintainGlobalTransform If set to true the child will stay in the same place spatially, otherwise it will maintain it's local transform (and potential move).
	 * @return Returns true if successful.
 	 */
	bool SetParent(FTLLRN_RigBaseElement* InChild, FTLLRN_RigBaseElement* InParent, bool bMaintainGlobalTransform = true);

	/**
	 * Adds a new available space to the given control
	 * @param InControlElement The control element to add the available space for
	 * @param InSpaceElement The space element to add to the available spaces list
	 * @return Returns true if successful.
	 */
	bool AddAvailableSpace(FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigTransformElement* InSpaceElement);

	/**
	 * Removes an available space from the given control
	 * @param InControlElement The control element to remove the available space from
	 * @param InSpaceElement The space element to remove from the available spaces list
	 * @return Returns true if successful.
	 */
	bool RemoveAvailableSpace(FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigTransformElement* InSpaceElement);

	/**
	 * Reorders an available space for the given control
	 * @param InControlElement The control element to remove the available space from
	 * @param InSpaceElement The space element to remove from the available spaces list
	 * @param InIndex The new index of the available space
	 * @return Returns true if successful.
	 */
	bool SetAvailableSpaceIndex(FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigTransformElement* InSpaceElement, int32 InIndex);

	/**
	 * Adds a new element to the dirty list of the given parent.
	 * This function is recursive and will affect all parents in the tree.
	 * @param InParent The parent element to change the dirty list for
	 * @param InElementToAdd The child element to add to the dirty list
	 * @param InHierarchyDistance The distance of number of elements in the hierarchy
	 */
	void AddElementToDirty(FTLLRN_RigBaseElement* InParent, FTLLRN_RigBaseElement* InElementToAdd, int32 InHierarchyDistance = 1) const;

	/**
	 * Remove an existing element to the dirty list of the given parent.
	 * This function is recursive and will affect all parents in the tree.
	 * @param InParent The parent element to change the dirty list for
	 * @param InElementToRemove The child element to remove from the dirty list
	 */
	void RemoveElementToDirty(FTLLRN_RigBaseElement* InParent, FTLLRN_RigBaseElement* InElementToRemove) const;

#if WITH_EDITOR
	static USkeletalMesh* GetSkeletalMeshFromAssetPath(const FString& InAssetPath);
	static USkeleton* GetSkeletonFromAssetPath(const FString& InAssetPath);
#endif

	/** 
	 * If set to true all notifications coming from this hierarchy will be suspended
	 */
	bool bSuspendAllNotifications;

	/** 
	 * If set to true selection related notifications coming from this hierarchy will be suspended
	 */
	bool bSuspendSelectionNotifications;

	/** 
	* If set to true all python printing can be disabled.  
	*/
	bool bSuspendPythonPrinting;

	/**
	 * If set the controller will mark new items as procedural and created at the current instruction
	 */
	int32 CurrentInstructionIndex;

	/**
	 * This function can be used to override the controller's logging mechanism
	 */
	TFunction<void(EMessageSeverity::Type,const FString&)> LogFunction = nullptr;

	template<typename T>
	T* MakeElement(bool bAllocateStorage = false)
	{
		T* Element = GetHierarchy()->NewElement<T>(1, bAllocateStorage);
		Element->CreatedAtInstructionIndex = CurrentInstructionIndex;
		return Element;
	}
	
	friend class UTLLRN_ControlRig;
	friend class UTLLRN_ControlRig;
	friend class UTLLRN_RigHierarchy;
	friend class FTLLRN_RigHierarchyControllerInstructionBracket;
	friend class UTLLRN_ControlRigBlueprint;
};

class TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyControllerInstructionBracket : TGuardValue<int32>
{
public:
	
	FTLLRN_RigHierarchyControllerInstructionBracket(UTLLRN_RigHierarchyController* InController, int32 InInstructionIndex)
		: TGuardValue<int32>(InController->CurrentInstructionIndex, InInstructionIndex)
	{
	}
};

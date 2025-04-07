// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "TLLRN_IKRetargetDetails.h"
#include "TLLRN_IKRetargeterPoseGenerator.h"
#include "TLLRN_IKRetargetPoseExporter.h"
#include "IPersonaToolkit.h"
#include "TLLRN_SIKRetargetAssetBrowser.h"
#include "UObject/ObjectPtr.h"
#include "Input/Reply.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SWindow.h"

class TLLRN_SRetargetOpStack;
enum class ETLLRN_RetargetSourceOrTarget : uint8;
class TLLRN_SIKRetargetHierarchy;
class TLLRN_SIKRigOutputLog;
class UTLLRN_IKRetargetProcessor;
class TLLRN_SIKRetargetChainMapList;
class UTLLRN_IKRetargetAnimInstance;
class FTLLRN_IKRetargetEditor;
class FPrimitiveDrawInterface;
class UDebugSkelMeshComponent;
class UTLLRN_IKRigDefinition;
class UTLLRN_IKRetargeterController;
class UTLLRN_IKRetargetBoneDetails;
struct FTLLRN_RetargetSkeleton;


// retarget editor modes
enum class ETLLRN_RetargeterOutputMode : uint8
{
	RunRetarget,		// output the retargeted target pose
	EditRetargetPose,	// allow editing the retarget pose
};

enum class ETLLRN_SelectionEdit : uint8
{
	Add,	// add to selection set
	Remove,	// remove from selection
	Replace	// replace selection entirely
};

enum class ETLLRN_RetargetSelectionType : uint8
{
	BONE,
	CHAIN,
	MESH,
	ROOT,
	NONE
};

struct FTLLRN_BoundIKRig
{
	FTLLRN_BoundIKRig(UTLLRN_IKRigDefinition* InIKRig, const FTLLRN_IKRetargetEditorController& InController);
	void UnBind() const;
	
	TWeakObjectPtr<UTLLRN_IKRigDefinition> IKRig;
	FDelegateHandle ReInitIKDelegateHandle;
	FDelegateHandle AddedChainDelegateHandle;
	FDelegateHandle RenameChainDelegateHandle;
	FDelegateHandle RemoveChainDelegateHandle;
};

struct FTLLRN_RetargetPlaybackManager : public TSharedFromThis<FTLLRN_RetargetPlaybackManager>
{
	FTLLRN_RetargetPlaybackManager(const TWeakPtr<FTLLRN_IKRetargetEditorController>& InEditorController);
	void PlayAnimationAsset(UAnimationAsset* AssetToPlay);
	void StopPlayback();
	void PausePlayback();
	void ResumePlayback() const;
	bool IsStopped() const;
	
private:

	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;
	UAnimationAsset* AnimThatWasPlaying = nullptr;
	float TimeWhenPaused = 0.0f;
	bool bWasPlayingAnim = false;
};

// a home for cross-widget communication to synchronize state across all tabs and viewport
class FTLLRN_IKRetargetEditorController : public TSharedFromThis<FTLLRN_IKRetargetEditorController>, FGCObject
{
public:

	virtual ~FTLLRN_IKRetargetEditorController() override {};

	// Initialize the editor
	void Initialize(TSharedPtr<FTLLRN_IKRetargetEditor> InEditor, UTLLRN_IKRetargeter* InAsset);
	// Close the editor
	void Close();
	
	// Bind callbacks to this IK Rig
	void BindToIKRigAssets();
	// callback when IK Rig asset requires reinitialization
	void HandleIKRigNeedsInitialized(UTLLRN_IKRigDefinition* ModifiedIKRig) const;
	// callback when IK Rig asset's retarget chain's have been added or removed
	void HandleRetargetChainAdded(UTLLRN_IKRigDefinition* ModifiedIKRig) const;
	// callback when IK Rig asset's retarget chain has been renamed
	void HandleRetargetChainRenamed(UTLLRN_IKRigDefinition* ModifiedIKRig, FName OldName, FName NewName) const;
	// callback when IK Rig asset's retarget chain has been removed
	void HandleRetargetChainRemoved(UTLLRN_IKRigDefinition* ModifiedIKRig, const FName InChainRemoved) const;
	// callback when IK Retargeter asset requires reinitialization
	void HandleRetargeterNeedsInitialized() const;
	// reinitialize retargeter without refreshing UI
	void ReinitializeRetargeterNoUIRefresh() const;
	FDelegateHandle RetargeterReInitDelegateHandle;
	// callback when IK Rig asset has been swapped out
	void HandleIKRigReplaced(ETLLRN_RetargetSourceOrTarget SourceOrTarget);
	FDelegateHandle IKRigReplacedDelegateHandle;
	// callback when Preview Mesh asset has been swapped out
	void HandlePreviewMeshReplaced(ETLLRN_RetargetSourceOrTarget SourceOrTarget);
	FDelegateHandle PreviewMeshReplacedDelegateHandle;
	FDelegateHandle RetargeterInitializedDelegateHandle;
	
	// all modifications to the data model should go through this controller
	TObjectPtr<UTLLRN_IKRetargeterController> AssetController;

	// the persona toolkit
	TWeakPtr<FTLLRN_IKRetargetEditor> Editor;

	// import / export retarget poses
	TSharedPtr<FTLLRN_IKRetargetPoseExporter> PoseExporter;

	// manage playback of animation in the editor
	TUniquePtr<FTLLRN_RetargetPlaybackManager> PlaybackManager;

	// viewport skeletal mesh
	UDebugSkelMeshComponent* GetSkeletalMeshComponent(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	UDebugSkelMeshComponent* SourceSkelMeshComponent;
	UDebugSkelMeshComponent* TargetSkelMeshComponent;
	// this root component is used as a parent of the source skeletal mesh to allow us to translate the source.
	// we can't offset the source mesh component itself because that conflicts with root motion
	USceneComponent* SourceRootComponent;

	// viewport anim instance
	UTLLRN_IKRetargetAnimInstance* GetAnimInstance(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	UPROPERTY(transient, NonTransactional)
	TObjectPtr<UTLLRN_IKRetargetAnimInstance> SourceAnimInstance;
	UPROPERTY(transient, NonTransactional)
	TObjectPtr<UTLLRN_IKRetargetAnimInstance> TargetAnimInstance;
	
	// store pointers to various tabs of UI,
	// have to manage access to these because they can be null if the tabs are closed
	void SetDetailsView(const TSharedPtr<IDetailsView>& InDetailsView) { DetailsView = InDetailsView; };
	void SetChainsView(const TSharedPtr<TLLRN_SIKRetargetChainMapList>& InChainsView) { ChainsView = InChainsView; };
	void SetAssetBrowserView(const TSharedPtr<TLLRN_SIKRetargetAssetBrowser>& InAssetBrowserView) { AssetBrowserView = InAssetBrowserView; };
	void SetOutputLogView(const TSharedPtr<TLLRN_SIKRigOutputLog>& InOutputLogView) { OutputLogView = InOutputLogView; };
	void SetHierarchyView(const TSharedPtr<TLLRN_SIKRetargetHierarchy>& InHierarchyView) { HierarchyView = InHierarchyView; };
	void SetOpStackView(const TSharedPtr<TLLRN_SRetargetOpStack>& InOpStackView) { OpStackView = InOpStackView; };
	bool IsObjectInDetailsView(const UObject* Object);
	
	// force refresh all views in the editor
	void RefreshAllViews() const;
	void RefreshDetailsView() const;
	void RefreshChainsView() const;
	void RefreshAssetBrowserView() const;
	void RefreshHierarchyView() const;
	void RefreshOpStackView() const;
	void RefreshPoseList() const;
	void SetDetailsObject(UObject* DetailsObject) const;
	void SetDetailsObjects(const TArray<UObject*>& DetailsObjects) const;

	// retargeter state
	bool IsReadyToRetarget() const;
	bool IsCurrentMeshLoaded() const;
	bool IsEditingPose() const;

	// display settings in the details panel
	void ShowGlobalSettings();
	void ShowRootSettings();
	void ShowPostPhaseSettings();
	FTLLRN_RetargetGlobalSettings& GetGlobalSettings() const;

	// clear the output log
	void ClearOutputLog() const;

	// get the USkeletalMesh we are transferring animation between (either source or target)
	USkeletalMesh* GetSkeletalMesh(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	// get the USkeleton we are transferring animation between (either source or target)
	const USkeleton* GetSkeleton(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	// get currently edited debug skeletal mesh
	UDebugSkelMeshComponent* GetEditedSkeletalMesh() const;
	// get the currently edited retarget skeleton
	const FTLLRN_RetargetSkeleton& GetCurrentlyEditedSkeleton(const UTLLRN_IKRetargetProcessor& Processor) const;
	
	// get world space pose of a bone (with component scale / offset applied)
	FTransform GetGlobalRetargetPoseOfBone(
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
		const int32& BoneIndex,
		const float& Scale,
		const FVector& Offset) const;
	
	// get world space positions of all immediate children of bone (with component scale / offset applied)
	static void GetGlobalRetargetPoseOfImmediateChildren(
		const FTLLRN_RetargetSkeleton& RetargetSkeleton,
		const int32& BoneIndex,
		const float& Scale,
		const FVector& Offset,
		TArray<int32>& OutChildrenIndices,
		TArray<FVector>& OutChildrenPositions);

	// get the retargeter that is running in the viewport (which is a duplicate of the source asset)
	UTLLRN_IKRetargetProcessor* GetRetargetProcessor() const;
	// Reset the planting state of the IK (when scrubbing or animation loops over)
	void ResetIKPlantingState() const;

	// Set viewport / editor tool mode
	void SetRetargeterMode(ETLLRN_RetargeterOutputMode Mode);
	void SetRetargetModeToPreviousMode() { SetRetargeterMode(PreviousMode); };
	ETLLRN_RetargeterOutputMode GetRetargeterMode() const { return OutputMode; }
	FText GetRetargeterModeLabel();
	FSlateIcon GetCurrentRetargetModeIcon() const;
	FSlateIcon GetRetargeterModeIcon(ETLLRN_RetargeterOutputMode Mode) const;
	float GetRetargetPoseAmount() const;
	void SetRetargetPoseAmount(float InValue);
	// END viewport / editor tool mode
	
	// general editor mode can be either viewing/editing source or target
	ETLLRN_RetargetSourceOrTarget GetSourceOrTarget() const { return CurrentlyEditingSourceOrTarget; };
	void SetSourceOrTargetMode(ETLLRN_RetargetSourceOrTarget SourceOrTarget);

	// ------------------------- SELECTION -----------------------------

	// SELECTION - BONES (viewport or hierarchy view)
	void EditBoneSelection(
		const TArray<FName>& InBoneNames,
		ETLLRN_SelectionEdit EditMode,
		const bool bFromHierarchyView = false);
	const TArray<FName>& GetSelectedBones() const {return SelectedBoneNames[CurrentlyEditingSourceOrTarget]; };
	// END bone selection

	// SELECTION - CHAINS (viewport or chains view)
	void EditChainSelection(
		const TArray<FName>& InChainNames,
		ETLLRN_SelectionEdit EditMode,
		const bool bFromChainsView);
	const TArray<FName>& GetSelectedChains() const {return SelectedChains; };
	// END chain selection

	void SetRootSelected(const bool bIsSelected);
	bool GetRootSelected() const { return bIsRootSelected; };

	void CleanSelection(ETLLRN_RetargetSourceOrTarget SourceOrTarget);
	void ClearSelection(const bool bKeepBoneSelection=false);
	ETLLRN_RetargetSelectionType GetLastSelectedItemType() const { return LastSelectedItem; };

	// op stack selection
	UTLLRN_RetargetOpBase* GetSelectedOp() const;
	int32 LastSelectedOpIndex = 0;

	// to frame selection when pressing "f" in viewport
	bool GetCameraTargetForSelection(FSphere& OutTarget) const;

	// check if any bone is selected
	bool IsEditingPoseWithAnyBoneSelected() const;

	// ------------------------- END SELECTION -----------------------------

	// determine if bone in the specified skeleton is part of the retarget (in a mapped chain)
	bool IsBoneRetargeted(const FName& BoneName, ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	// get the name of the chain that contains this bone
	FName GetChainNameFromBone(const FName& BoneName, ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;

	// factory to get/create bone details UObject
	TObjectPtr<UTLLRN_IKRetargetBoneDetails> GetOrCreateBoneDetailsObject(const FName& BoneName);

	// ------------------------- RETARGET POSES -----------------------------
	
	// toggle current retarget pose
	TArray<TSharedPtr<FName>> PoseNames;
	FText GetCurrentPoseName() const;
	void OnPoseSelected(TSharedPtr<FName> InPoseName, ESelectInfo::Type SelectInfo) const;

	// reset retarget pose
	void HandleResetAllBones() const;
	void HandleResetSelectedBones() const;
	void HandleResetSelectedAndChildrenBones() const;
	
	// auto generate retarget pose
	void HandleAlignAllBones() const;
	void HandleAlignSelectedBones(const ETLLRN_RetargetAutoAlignMethod Method, const bool bIncludeChildren) const;
	void HandleSnapToGround() const;

	// create new retarget pose
	void HandleNewPose();
	bool CanCreatePose() const;
	FReply CreateNewPose() const;
	TSharedPtr<SWindow> NewPoseWindow;
	TSharedPtr<SEditableTextBox> NewPoseEditableText;

	// duplicate current retarget pose
	void HandleDuplicatePose();
	FReply CreateDuplicatePose() const;

	// delete retarget pose
	void HandleDeletePose();
	bool CanDeletePose() const;

	// rename retarget pose
	void HandleRenamePose();
	FReply RenamePose() const;
	bool CanRenamePose() const;
	TSharedPtr<SWindow> RenamePoseWindow;
	TSharedPtr<SEditableTextBox> NewNameEditableText;

	// detect and auto-fix retarget pose that causes root height to be on the ground
	void FixZeroHeightRetargetRoot(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	
	// ------------------------- END RETARGET POSES -----------------------------

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("Retarget Editor"); };
	// END FGCObject interface

	// render the skeleton's in the viewport (either source or target)
	void RenderSkeleton(FPrimitiveDrawInterface* PDI, ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;

	void UpdateMeshOffset(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;

private:

	TArray<FName> GetSelectedBonesAndChildren() const;

	// modal dialog to ask user if they want to fix root bones that are "on the ground"
	bool PromptToFixRootHeight(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	bool bAskedToFixRoot = false;

	// asset properties tab
	TSharedPtr<IDetailsView> DetailsView;
	// chain list view
	TSharedPtr<TLLRN_SIKRetargetChainMapList> ChainsView;
	// asset browser view
	TSharedPtr<TLLRN_SIKRetargetAssetBrowser> AssetBrowserView;
	// output log view
	TSharedPtr<TLLRN_SIKRigOutputLog> OutputLogView;
	// hierarchy view
	TSharedPtr<TLLRN_SIKRetargetHierarchy> HierarchyView;
	// op stack widget
	TSharedPtr<TLLRN_SRetargetOpStack> OpStackView;

	// when prompting user to assign an IK Rig
	TSharedPtr<SWindow> IKRigPickerWindow;

	// the current output mode of the retargeter
	ETLLRN_RetargeterOutputMode OutputMode = ETLLRN_RetargeterOutputMode::RunRetarget;
	ETLLRN_RetargeterOutputMode PreviousMode;
	// slider value to blend between reference pose and retarget pose
	float RetargetPosePreviewBlend = 1.0f;
	
	// which skeleton are we editing / viewing?
	ETLLRN_RetargetSourceOrTarget CurrentlyEditingSourceOrTarget = ETLLRN_RetargetSourceOrTarget::Target;

	// current selection set
	bool bIsRootSelected = false;
	UPrimitiveComponent* SelectedMesh = nullptr;
	TArray<FName> SelectedChains;
	TMap<ETLLRN_RetargetSourceOrTarget, TArray<FName>> SelectedBoneNames;
	ETLLRN_RetargetSelectionType LastSelectedItem = ETLLRN_RetargetSelectionType::NONE;
	UPROPERTY()
	TMap<FName,TObjectPtr<UTLLRN_IKRetargetBoneDetails>> AllBoneDetails;

	// ik rigs bound to this editor (will receive callbacks when requiring reinitialization
	TArray<FTLLRN_BoundIKRig> BoundIKRigs;
};


// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Rig/Solvers/TLLRN_IKRigSolver.h"
#include "TLLRN_SIKRigRetargetChainList.h"
#include "Templates/SubclassOf.h"
#include "Animation/AnimationAsset.h"
#include "SAdvancedTransformInputBox.h"

#include "TLLRN_IKRigEditorController.generated.h"

class TLLRN_SIKRigAssetBrowser;
class TLLRN_SIKRigOutputLog;
class UTLLRN_IKRigAnimInstance;
class FTLLRN_IKRigEditorToolkit;
class TLLRN_SIKRigSolverStack;
class TLLRN_SIKRigHierarchy;
class UTLLRN_IKRigController;
class FTLLRN_SolverStackElement;
class FIKRigTreeElement;
class UAnimInstance;
class UDebugSkelMeshComponent;

enum EIKRigSelectionType : int8
{
	Hierarchy,
	SolverStack,
	RetargetChains,
};

UCLASS(config = Engine, hidecategories = UObject)
class TLLRN_IKRIGEDITOR_API UTLLRN_IKRigBoneDetails : public UObject
{
	GENERATED_BODY()

public:

	// todo update bone info automatically using something else
	void SetBone(const FName& BoneName)
	{
		SelectedBone = BoneName;
	};

	UPROPERTY(VisibleAnywhere, Category = "Selection")
	FName SelectedBone;
	
	UPROPERTY(VisibleAnywhere, Category = "Bone Transforms")
	FTransform CurrentTransform;

	UPROPERTY(VisibleAnywhere, Category = "Bone Transforms")
	FTransform ReferenceTransform;

	UPROPERTY()
	TWeakObjectPtr<UAnimInstance> AnimInstancePtr;

	UPROPERTY()
	TWeakObjectPtr<UTLLRN_IKRigDefinition> AssetPtr;

#if WITH_EDITOR

	TOptional<FTransform> GetTransform(ETLLRN_IKRigTransformType::Type TransformType) const;
	bool IsComponentRelative(ESlateTransformComponent::Type Component, ETLLRN_IKRigTransformType::Type TransformType) const;
	void OnComponentRelativeChanged(ESlateTransformComponent::Type Component, bool bIsRelative, ETLLRN_IKRigTransformType::Type TransformType);
	void OnCopyToClipboard(ESlateTransformComponent::Type Component, ETLLRN_IKRigTransformType::Type TransformType) const;
	void OnPasteFromClipboard(ESlateTransformComponent::Type Component, ETLLRN_IKRigTransformType::Type TransformType);

	template <typename DataType>
	void GetContentFromData(const DataType& InData, FString& Content) const
	{
		TBaseStructure<DataType>::Get()->ExportText(Content, &InData, &InData, nullptr, PPF_None, nullptr);
	}

#endif
	
private:
	
	static bool CurrentTransformRelative[3];
	static bool ReferenceTransformRelative[3];
};

enum class EChainSide
{
	Left,
	Right,
	Center
};

// use string matching and skeletal analysis to suggest a reasonable default name for retarget chains 
struct FTLLRN_RetargetChainAnalyzer
{
	TSharedPtr<FTextFilterExpressionEvaluator> TextFilter;

	FTLLRN_RetargetChainAnalyzer()
	{
		TextFilter = MakeShared<FTextFilterExpressionEvaluator>(ETextFilterExpressionEvaluatorMode::BasicString);
	}
	
	void AssignBestGuessName(FTLLRN_BoneChain& Chain, const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton);
	
	static FName GetDefaultChainName();

	EChainSide GetSideOfChain(const TArray<int32>& BoneIndices, const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton) const;
};

// a home for cross-widget communication to synchronize state across all tabs and viewport 
class FTLLRN_IKRigEditorController : public TSharedFromThis<FTLLRN_IKRigEditorController>, FGCObject
{
public:

	// initialize the editor controller to an instance of the IK Rig editor 
	void Initialize(TSharedPtr<FTLLRN_IKRigEditorToolkit> Toolkit, const UTLLRN_IKRigDefinition* Asset);
	// cleanup when editor closed 
	void Close() const;

	// get the currently active processor running the IK Rig in the editor 
	UTLLRN_IKRigProcessor* GetTLLRN_IKRigProcessor() const;
	// get the currently running IKRig skeleton (if there is a running processor) 
	const FTLLRN_IKRigSkeleton* GetCurrentTLLRN_IKRigSkeleton() const;

	// callback when IK Rig requires re-initialization 
	void HandleIKRigNeedsInitialized(UTLLRN_IKRigDefinition* ModifiedIKRig) const;

	// delete key pressed (in either viewport or tree view)
	void HandleDeleteSelectedElements();

	// create goals 
	void AddNewGoals(const TArray<FName>& GoalNames, const TArray<FName>& BoneNames);
	// clear all selected objects 
	void ClearSelection() const;
	// callback when goal is selected in the viewport 
	void HandleGoalSelectedInViewport(const FName& GoalName, bool bReplace) const;
	// callback when bone is selected in the viewport 
	void HandleBoneSelectedInViewport(const FName& BoneName, bool bReplace) const;
	// reset all goals to initial transforms 
	void Reset() const;
	// refresh just the skeleton tree view 
	void RefreshTreeView() const;
	// clear the output log 
	void ClearOutputLog() const;
	// automatically generates retarget chains 
	void AutoGenerateRetargetChains() const;
	// automatically generates IK setup 
	void AutoGenerateFBIK() const;

	// return list of those solvers in the stack that are selected by user 
	void GetSelectedSolvers(TArray<TSharedPtr<FTLLRN_SolverStackElement> >& OutSelectedSolvers) const;
	// get index of the first selected solver, return INDEX_None if nothing selected 
	int32 GetSelectedSolverIndex();
	// get names of all goals that are selected 
	void GetSelectedGoalNames(TArray<FName>& OutGoalNames) const;
	// return the number of selected goals 
	int32 GetNumSelectedGoals() const;
	// get names of all bones that are selected 
	void GetSelectedBoneNames(TArray<FName>& OutBoneNames) const;
	// get all bones that are selected 
	void GetSelectedBones(TArray<TSharedPtr<FIKRigTreeElement>>& OutBoneItems) const;
	// returns true if Goal is currently selected 
	bool IsGoalSelected(const FName& GoalName) const;
	// is anything selected in the skeleton view? 
	bool DoesSkeletonHaveSelectedItems() const;
	// get name of the selected retargeting chain 
	TArray<FName> GetSelectedChains() const;
	// get chains selected in skeleton view 
	void GetChainsSelectedInSkeletonView(TArray<FTLLRN_BoneChain>& InOutChains);
	
	// create new retarget chains from selected bones (or single empty chain if no selection) 
	void CreateNewRetargetChains();
	
	// show single transform of bone in details view 
	void ShowDetailsForBone(const FName BoneName) const;
	// show single BONE settings in details view 
	void ShowDetailsForBoneSettings(const FName& BoneName, int32 SolverIndex) const;
	// show single GOAL settings in details view 
	void ShowDetailsForGoal(const FName& GoalName) const;
	// show single EFFECTOR settings in details view 
	void ShowDetailsForGoalSettings(const FName GoalName, const int32 SolverIndex) const;
	// show single SOLVER settings in details view 
	void ShowDetailsForSolver(const int32 SolverIndex) const;
	// show nothing in details view 
	void ShowAssetDetails() const;
	// show selected items in details view 
	void ShowDetailsForElements(const TArray<TSharedPtr<FIKRigTreeElement>>& InItems) const;
	// callback when detail is edited 
	void OnFinishedChangingDetails(const FPropertyChangedEvent& PropertyChangedEvent) const;
	// returns true if the supplied UObject is being shown in the details panel
	bool IsObjectInDetailsView(const UObject* Object) const;
	
	// set details tab view 
	void SetDetailsView(const TSharedPtr<class IDetailsView>& InDetailsView);
	// set skeleton tab view 
	void SetHierarchyView(const TSharedPtr<TLLRN_SIKRigHierarchy>& InSkeletonView){ SkeletonView = InSkeletonView; };
	// set solver stack tab view 
	void SetSolverStackView(const TSharedPtr<TLLRN_SIKRigSolverStack>& InSolverStackView){ SolverStackView = InSolverStackView; };
	// set retargeting tab view 
	void SetRetargetingView(const TSharedPtr<TLLRN_SIKRigRetargetChainList>& InRetargetingView){ RetargetingView = InRetargetingView; };
	// set output log view 
	void SetOutputLogView(const TSharedPtr<TLLRN_SIKRigOutputLog>& InOutputLogView){ OutputLogView = InOutputLogView; };

	// right after importing a skeleton, we ask user what solver they want to use 
	bool PromptToAddDefaultSolver() const;
	// show user the new retarget chain they are about to create (provides option to edit name) 
	FName PromptToAddNewRetargetChain(FTLLRN_BoneChain& BoneChain) const;
	// right after creating a goal, we ask user if they want it assigned to a retarget chain 
	void PromptToAssignGoalToChain(FName NewGoalName) const;

	// play preview animation on running anim instance in editor (before IK) 
	void PlayAnimationAsset(UAnimationAsset* AssetToPlay);
	
	// all modifications to the data model should go through this controller
	UPROPERTY(transient)
	TObjectPtr<UTLLRN_IKRigController> AssetController;

	// viewport skeletal mesh 
	UDebugSkelMeshComponent* SkelMeshComponent;

	// viewport anim instance 
	UPROPERTY(transient, NonTransactional)
	TObjectPtr<UTLLRN_IKRigAnimInstance> AnimInstance;

	// the persona toolkit 
	TWeakPtr<FTLLRN_IKRigEditorToolkit> EditorToolkit;

	// used to analyze a retarget chain 
	FTLLRN_RetargetChainAnalyzer ChainAnalyzer;

	// UI and viewport selection state 
	bool bManipulatingGoals = false;

	// record which part of the UI was last selected 
	EIKRigSelectionType GetLastSelectedType() const;
	void SetLastSelectedType(EIKRigSelectionType SelectionType);
	// END selection type 

	// FGCObject interface 
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(BoneDetails);
		Collector.AddReferencedObject(AnimInstance);
		Collector.AddReferencedObject(AssetController);
	};
	virtual FString GetReferencerName() const override { return "TLLRN_IKRigEditorController"; };
	// END FGCObject interface 

	// UTLLRN_IKRigBoneDetails factory *
	TObjectPtr<UTLLRN_IKRigBoneDetails> CreateBoneDetails(const TSharedPtr<FIKRigTreeElement const>& InItem) const;
	
private:

	// asset properties tab 
	TSharedPtr<IDetailsView> DetailsView;

	// the skeleton tree view 
	TSharedPtr<TLLRN_SIKRigHierarchy> SkeletonView;
	
	// the solver stack view 
	TSharedPtr<TLLRN_SIKRigSolverStack> SolverStackView;

	// the solver stack view 
	TSharedPtr<TLLRN_SIKRigRetargetChainList> RetargetingView;

	// asset browser view 
	TSharedPtr<TLLRN_SIKRigAssetBrowser> AssetBrowserView;

	// output log view 
	TSharedPtr<TLLRN_SIKRigOutputLog> OutputLogView;

	// when prompting user to assign a skeletal mesh 
	TSharedPtr<SWindow> MeshPickerWindow;

	EIKRigSelectionType LastSelectedType;

	UPROPERTY()
	TObjectPtr<UTLLRN_IKRigBoneDetails> BoneDetails;

	// remove delegate when closing editor 
	FDelegateHandle ReinitializeDelegateHandle;

	friend struct FTLLRN_IKRigOutputLogTabSummoner;
	friend class TLLRN_SIKRigAssetBrowser;
};

struct FTLLRN_IKRigSolverTypeAndName
{
	FText NiceName;
	TSubclassOf<UTLLRN_IKRigSolver> SolverType;
};
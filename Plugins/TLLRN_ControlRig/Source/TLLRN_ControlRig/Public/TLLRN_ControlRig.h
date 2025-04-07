// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/SparseDelegate.h"
#include "Engine/EngineBaseTypes.h"
#include "Templates/SubclassOf.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_ControlRigGizmoLibrary.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Animation/NodeMappingProviderInterface.h"
#include "Units/TLLRN_RigUnit.h"
#include "Units/Control/TLLRN_RigUnit_Control.h"
#include "RigVMCore/RigVM.h"
#include "RigVMHost.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AttributesRuntime.h"
#include "Rigs/TLLRN_TLLRN_RigModuleDefines.h"

#if WITH_EDITOR
#include "RigVMModel/RigVMPin.h"
#include "RigVMModel/Nodes/RigVMUnitNode.h"
#include "RigVMTypeUtils.h"
#endif

#if WITH_EDITOR
#include "AnimPreviewInstance.h"
#endif 

#include "TLLRN_ControlRig.generated.h"

class ITLLRN_ControlRigObjectBinding;
class UScriptStruct;
class USkeletalMesh;
class USkeletalMeshComponent;
class AActor;
class UTLLRN_TransformableControlHandle;

struct FReferenceSkeleton;
struct FTLLRN_RigUnit;
struct FTLLRN_RigControl;

TLLRN_CONTROLRIG_API DECLARE_LOG_CATEGORY_EXTERN(LogTLLRN_ControlRig, Log, All);

/** Runs logic for mapping input data to transforms (the "Rig") */
UCLASS(Blueprintable, Abstract, editinlinenew)
class TLLRN_CONTROLRIG_API UTLLRN_ControlRig : public URigVMHost, public INodeMappingProviderInterface, public ITLLRN_RigHierarchyProvider
{
	GENERATED_UCLASS_BODY()

	friend class UTLLRN_ControlRigComponent;
	friend class STLLRN_ControlRigStackView;

public:

	/** Bindable event for external objects to contribute to / filter a control value */
	DECLARE_EVENT_ThreeParams(UTLLRN_ControlRig, FFilterControlEvent, UTLLRN_ControlRig*, FTLLRN_RigControlElement*, FTLLRN_RigControlValue&);

	/** Bindable event for external objects to be notified of Control changes */
	DECLARE_EVENT_ThreeParams(UTLLRN_ControlRig, FControlModifiedEvent, UTLLRN_ControlRig*, FTLLRN_RigControlElement*, const FTLLRN_RigControlModifiedContext&);

	/** Bindable event for external objects to be notified that a Control is Selected */
	DECLARE_EVENT_ThreeParams(UTLLRN_ControlRig, FControlSelectedEvent, UTLLRN_ControlRig*, FTLLRN_RigControlElement*, bool);

	/** Bindable event to manage undo / redo brackets in the client */
	DECLARE_EVENT_TwoParams(UTLLRN_ControlRig, FControlUndoBracketEvent, UTLLRN_ControlRig*, bool /* bOpen */);

	// To support Blueprints/scripting, we need a different delegate type (a 'Dynamic' delegate) which supports looser style UFunction binding (using names).
	DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_ThreeParams(FOnControlSelectedBP, UTLLRN_ControlRig, OnControlSelected_BP, UTLLRN_ControlRig*, Rig, const FTLLRN_RigControlElement&, Control, bool, bSelected);

	/** Bindable event to notify object binding change. */
	DECLARE_EVENT_OneParam(UTLLRN_ControlRig, FTLLRN_ControlRigBoundEvent, UTLLRN_ControlRig*);

	static const FName OwnerComponent;

	UFUNCTION(BlueprintCallable, Category = TLLRN_ControlRig)
	static TArray<UTLLRN_ControlRig*> FindTLLRN_ControlRigs(UObject* Outer, TSubclassOf<UTLLRN_ControlRig> OptionalClass);

public:
	virtual UWorld* GetWorld() const override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual UScriptStruct* GetPublicContextStruct() const override { return FTLLRN_ControlRigExecuteContext::StaticStruct(); }

	// Returns the settings of the module this instance belongs to
	const FTLLRN_RigModuleSettings& GetTLLRN_RigModuleSettings() const;

	// Returns true if the rig is defined as a rig module
	bool IsTLLRN_RigModule() const;

	// Returns true if this rig is an instance module. Rigs may be a module but not instance
	// when being interacted with the asset editor
	bool IsTLLRN_RigModuleInstance() const;

	// Returns true if this rig is a modular rig (of class UTLLRN_ModularRig)
	bool IsTLLRN_ModularRig() const;

	// Returns true if this is a standalone rig (of class UTLLRN_ControlRig and not modular)
	bool IsStandaloneRig() const;

	// Returns true if this is a native rig (implemented in C++)
	bool IsNativeRig() const;

	// Returns the parent rig hosting this module instance
	UTLLRN_ControlRig* GetParentRig() const;

	// Returns the namespace of this module (for example ArmModule::)
	const FString& GetTLLRN_RigModuleNameSpace() const;

	// Returns the redirector from key to key for this rig
	virtual FTLLRN_RigElementKeyRedirector& GetElementKeyRedirector();
	virtual FTLLRN_RigElementKeyRedirector GetElementKeyRedirector() const { return ElementKeyRedirector; }
	
	// Returns the redirector from key to key for this rig
	virtual void SetElementKeyRedirector(const FTLLRN_RigElementKeyRedirector InElementRedirector);

	/** Creates a transformable control handle for the specified control to be used by the constraints system. Should use the UObject from 
	ConstraintsScriptingLibrary::GetManager(UWorld* InWorld)*/
	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig | Constraints")
	UTLLRN_TransformableControlHandle* CreateTLLRN_TransformableControlHandle(const FName& ControlName) const;


#if WITH_EDITOR
	/** Get the category of this TLLRN_ControlRig (for display in menus) */
	virtual FText GetCategory() const;

	/** Get the tooltip text to display for this node (displayed in graphs and from context menus) */
	virtual FText GetToolTipText() const;
#endif

	/** Initialize things for the TLLRN_ControlRig */
	virtual void Initialize(bool bInitTLLRN_RigUnits = true) override;

	/** Initialize the VM */
	virtual bool InitializeVM(const FName& InEventName) override;

	virtual void InitializeVMs(bool bInitTLLRN_RigUnits = true) { Super::Initialize(bInitTLLRN_RigUnits); }
	virtual bool InitializeVMs(const FName& InEventName) { return Super::InitializeVM(InEventName); }

#if WITH_EDITOR
protected:
	bool bIsRunningInPIE;
#endif
	
public:

	/** Evaluates the TLLRN_ControlRig */
	virtual void Evaluate_AnyThread() override;

	/** Ticks animation of the skeletal mesh component bound to this control rig */
	bool EvaluateSkeletalMeshComponent(double InDeltaTime);

	/** Removes any stored additive control values */
	void ResetControlValues();

	/** Resets the stored pose coming from the anim sequence.
	 * This usually indicates a new pose should be stored. */
	void ClearPoseBeforeBackwardsSolve();

	/* For additive rigs, will set control values by inverting the pose found after the backwards solve */
	/* Returns the array of control elements that were modified*/
	TArray<FTLLRN_RigControlElement*> InvertInputPose(const TArray<FTLLRN_RigElementKey>& InElements = TArray<FTLLRN_RigElementKey>(), ETLLRN_ControlRigSetKey InSetKey = ETLLRN_ControlRigSetKey::Never);

	/** Setup bindings to a runtime object (or clear by passing in nullptr). */
	void SetObjectBinding(TSharedPtr<ITLLRN_ControlRigObjectBinding> InObjectBinding)
	{
		ObjectBinding = InObjectBinding;
		OnTLLRN_ControlRigBound.Broadcast(this);
	}

	TSharedPtr<ITLLRN_ControlRigObjectBinding> GetObjectBinding() const
	{
		return ObjectBinding;
	}

	/** Find the actor the rig is bound to, if any */
	UFUNCTION(BlueprintPure, Category = "TLLRN Control Rig")
	AActor* GetHostingActor() const;

	UFUNCTION(BlueprintPure, Category = "TLLRN Control Rig")
	UTLLRN_RigHierarchy* GetHierarchy()
	{
		return DynamicHierarchy;
	}
	
	virtual UTLLRN_RigHierarchy* GetHierarchy() const override
	{
		return DynamicHierarchy;
	}

#if WITH_EDITOR

	// called after post reinstance when compilng blueprint by Sequencer
	void PostReinstanceCallback(const UTLLRN_ControlRig* Old);

	// resets the recorded transform changes
	void ResetRecordedTransforms(const FName& InEventName);

#endif // WITH_EDITOR
	
	// BEGIN UObject interface
	virtual void BeginDestroy() override;
	// END UObject interface

	DECLARE_EVENT_OneParam(UTLLRN_ControlRig, FTLLRN_ControlRigBeginDestroyEvent, class UTLLRN_ControlRig*);
	FTLLRN_ControlRigBeginDestroyEvent& OnBeginDestroy() {return BeginDestroyEvent;};
private:
	/** Broadcasts a notification just before the controlrig is destroyed. */
	FTLLRN_ControlRigBeginDestroyEvent BeginDestroyEvent;
	
public:	
	
	UPROPERTY(transient)
	ETLLRN_RigExecutionType ExecutionType;

	UPROPERTY()
	FTLLRN_RigHierarchySettings HierarchySettings;

	virtual bool Execute(const FName& InEventName) override;
	virtual bool Execute_Internal(const FName& InEventName) override;
	virtual void RequestInit() override;
	virtual void RequestInitVMs()  { Super::RequestInit(); }
	virtual bool SupportsEvent(const FName& InEventName) const override { return Super::SupportsEvent(InEventName); }
	virtual const TArray<FName>& GetSupportedEvents() const override{ return Super::GetSupportedEvents(); }

	template<class T>
	bool SupportsEvent() const
	{
		return SupportsEvent(T::EventName);
	}

	bool AllConnectorsAreResolved(FString* OutFailureReason = nullptr, FTLLRN_RigElementKey* OutConnector = nullptr) const;

	/** Requests to perform construction during the next execution */
	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig")
	void RequestConstruction();

	bool IsConstructionRequired() const;

	/** Contains a backwards solve event */
	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig")
	bool SupportsBackwardsSolve() const;

	virtual void AdaptEventQueueForEvaluate(TArray<FName>& InOutEventQueueToRun) override;

	/** INodeMappingInterface implementation */
	virtual void GetMappableNodeData(TArray<FName>& OutNames, TArray<FNodeItem>& OutNodeItems) const override;

	/** Data Source Registry Getter */
	UAnimationDataSourceRegistry* GetDataSourceRegistry();

	virtual TArray<FTLLRN_RigControlElement*> AvailableControls() const;
	virtual FTLLRN_RigControlElement* FindControl(const FName& InControlName) const;
	virtual bool ShouldApplyLimits() const { return !IsConstructionModeEnabled(); }
	virtual bool IsConstructionModeEnabled() const;
	virtual FTransform SetupControlFromGlobalTransform(const FName& InControlName, const FTransform& InGlobalTransform);
	virtual FTransform GetControlGlobalTransform(const FName& InControlName) const;

	// Sets the relative value of a Control
	template<class T>
	void SetControlValue(const FName& InControlName, T InValue, bool bNotify = true,
		const FTLLRN_RigControlModifiedContext& Context = FTLLRN_RigControlModifiedContext(), bool bSetupUndo = true, bool bPrintPythonCommnds = false, bool bFixEulerFlips = false)
	{
		SetControlValueImpl(InControlName, FTLLRN_RigControlValue::Make<T>(InValue), bNotify, Context, bSetupUndo, bPrintPythonCommnds, bFixEulerFlips);
	}

	// Returns the value of a Control
	FTLLRN_RigControlValue GetControlValue(const FName& InControlName) const
	{
		const FTLLRN_RigElementKey Key(InControlName, ETLLRN_RigElementType::Control);
		if (FTLLRN_RigBaseElement* Element = DynamicHierarchy->Find(Key))
		{
			if (FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
			{
				return GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
			}
		}
		return DynamicHierarchy->GetControlValue(Key);
	}

	FTLLRN_RigControlValue GetControlValue(FTLLRN_RigControlElement* InControl, const ETLLRN_RigControlValueType& InValueType) const;

	// Sets the relative value of a Control
	virtual void SetControlValueImpl(const FName& InControlName, const FTLLRN_RigControlValue& InValue, bool bNotify = true,
		const FTLLRN_RigControlModifiedContext& Context = FTLLRN_RigControlModifiedContext(), bool bSetupUndo = true, bool bPrintPythonCommnds = false, bool bFixEulerFlips = false);

	void SwitchToParent(const FTLLRN_RigElementKey& InElementKey, const FTLLRN_RigElementKey& InNewParentKey, bool bInitial, bool bAffectChildren);

	FTransform GetInitialLocalTransform(const FTLLRN_RigElementKey &InKey)
	{
		if (bIsAdditive)
		{
			// The initial value of all additive controls is always Identity
			return FTransform::Identity;
		}
		return GetHierarchy()->GetInitialLocalTransform(InKey);
	}

	bool SetControlGlobalTransform(const FName& InControlName, const FTransform& InGlobalTransform, bool bNotify = true, const FTLLRN_RigControlModifiedContext& Context = FTLLRN_RigControlModifiedContext(), bool bSetupUndo = true, bool bPrintPythonCommands = false, bool bFixEulerFlips = false);

	virtual FTLLRN_RigControlValue GetControlValueFromGlobalTransform(const FName& InControlName, const FTransform& InGlobalTransform, ETLLRN_RigTransformType::Type InTransformType);

	virtual void SetControlLocalTransform(const FName& InControlName, const FTransform& InLocalTransform, bool bNotify = true, const FTLLRN_RigControlModifiedContext& Context = FTLLRN_RigControlModifiedContext(), bool bSetupUndo = true, bool bFixEulerFlips = false);
	virtual FTransform GetControlLocalTransform(const FName& InControlName) ;

	FVector GetControlSpecifiedEulerAngle(const FTLLRN_RigControlElement* InControlElement, bool bIsInitial = false) const;

	virtual const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>>& GetShapeLibraries() const;
	virtual void CreateTLLRN_RigControlsForCurveContainer();
	virtual void GetControlsInOrder(TArray<FTLLRN_RigControlElement*>& SortedControls) const;

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig")
	virtual void SelectControl(const FName& InControlName, bool bSelect = true);
	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig")
	virtual bool ClearControlSelection();
	UFUNCTION(BlueprintPure, Category = "TLLRN Control Rig")
	virtual TArray<FName> CurrentControlSelection() const;
	UFUNCTION(BlueprintPure, Category = "TLLRN Control Rig")
	virtual bool IsControlSelected(const FName& InControlName)const;

	// Returns true if this manipulatable subject is currently
	// available for manipulation / is enabled.
	virtual bool ManipulationEnabled() const
	{
		return bManipulationEnabled;
	}

	// Sets the manipulatable subject to enabled or disabled
	virtual bool SetManipulationEnabled(bool Enabled = true)
	{
		if (bManipulationEnabled == Enabled)
		{
			return false;
		}
		bManipulationEnabled = Enabled;
		return true;
	}

	// Returns a event that can be used to subscribe to
	// filtering control data when needed
	FFilterControlEvent& ControlFilter() { return OnFilterControl; }

	// Returns a event that can be used to subscribe to
	// change notifications coming from the manipulated subject.
	FControlModifiedEvent& ControlModified() { return OnControlModified; }

	// Returns a event that can be used to subscribe to
	// selection changes coming from the manipulated subject.
	FControlSelectedEvent& ControlSelected() { return OnControlSelected; }

	// Returns an event that can be used to subscribe to
	// Undo Bracket requests such as Open and Close.
	FControlUndoBracketEvent & ControlUndoBracket() { return OnControlUndoBracket; }
	
	FTLLRN_ControlRigBoundEvent& TLLRN_ControlRigBound() { return OnTLLRN_ControlRigBound; };

	bool IsCurveControl(const FTLLRN_RigControlElement* InControlElement) const;

	DECLARE_EVENT_TwoParams(UTLLRN_ControlRig, FTLLRN_ControlRigExecuteEvent, class UTLLRN_ControlRig*, const FName&);
#if WITH_EDITOR
	FTLLRN_ControlRigExecuteEvent& OnPreConstructionForUI_AnyThread() { return PreConstructionForUIEvent; }
#endif
	FTLLRN_ControlRigExecuteEvent& OnPreConstruction_AnyThread() { return PreConstructionEvent; }
	FTLLRN_ControlRigExecuteEvent& OnPostConstruction_AnyThread() { return PostConstructionEvent; }
	FTLLRN_ControlRigExecuteEvent& OnPreForwardsSolve_AnyThread() { return PreForwardsSolveEvent; }
	FTLLRN_ControlRigExecuteEvent& OnPostForwardsSolve_AnyThread() { return PostForwardsSolveEvent; }
	FTLLRN_ControlRigExecuteEvent& OnPreAdditiveValuesApplication_AnyThread() { return PreAdditiveValuesApplicationEvent; }
	FTLLRN_RigEventDelegate& OnTLLRN_RigEvent_AnyThread() { return TLLRN_RigEventDelegate; }

	// Setup the initial transform / ref pose of the bones based upon an anim instance
	// This uses the current refpose instead of the RefSkeleton pose.
	virtual void SetBoneInitialTransformsFromAnimInstance(UAnimInstance* InAnimInstance);

	// Setup the initial transform / ref pose of the bones based upon an anim instance proxy
	// This uses the current refpose instead of the RefSkeleton pose.
	virtual void SetBoneInitialTransformsFromAnimInstanceProxy(const FAnimInstanceProxy* InAnimInstanceProxy);

	// Setup the initial transform / ref pose of the bones based upon skeletal mesh component (ref skeleton)
	// This uses the RefSkeleton pose instead of the current refpose (or vice versae is bUseAnimInstance == true)
	virtual void SetBoneInitialTransformsFromSkeletalMeshComponent(USkeletalMeshComponent* InSkelMeshComp, bool bUseAnimInstance = false);

	// Setup the initial transforms / ref pose of the bones based on a skeletal mesh
	// This uses the RefSkeleton pose instead of the current refpose.
	virtual void SetBoneInitialTransformsFromSkeletalMesh(USkeletalMesh* InSkeletalMesh);

	// Setup the initial transforms / ref pose of the bones based on a reference skeleton
	// This uses the RefSkeleton pose instead of the current refpose.
	virtual void SetBoneInitialTransformsFromRefSkeleton(const FReferenceSkeleton& InReferenceSkeleton);

private:

	void SetBoneInitialTransformsFromCompactPose(FCompactPose* InCompactPose);

public:
	
	const FTLLRN_RigControlElementCustomization* GetControlCustomization(const FTLLRN_RigElementKey& InControl) const;
	void SetControlCustomization(const FTLLRN_RigElementKey& InControl, const FTLLRN_RigControlElementCustomization& InCustomization);

	virtual void PostInitInstanceIfRequired() override;
#if WITH_EDITORONLY_DATA
	static void DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass);
#endif

	virtual USceneComponent* GetOwningSceneComponent() override;

	void SetDynamicHierarchy(TObjectPtr<UTLLRN_RigHierarchy> InHierarchy);

protected:

	virtual void PostInitInstance(URigVMHost* InCDO) override;

	UPROPERTY()
	TMap<FTLLRN_RigElementKey, FTLLRN_RigControlElementCustomization> ControlCustomizations;

	UPROPERTY()
	TObjectPtr<UTLLRN_RigHierarchy> DynamicHierarchy;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary> GizmoLibrary_DEPRECATED;
#endif

	UPROPERTY()
	TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>> ShapeLibraries;

	UPROPERTY(transient)
	TMap<FString, FString> ShapeLibraryNameMap;

	/** Runtime object binding */
	TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding;

#if WITH_EDITORONLY_DATA
	// you either go Input or Output, currently if you put it in both place, Output will override
	UPROPERTY()
	TMap<FName, FCachedPropertyPath> InputProperties_DEPRECATED;

	UPROPERTY()
	TMap<FName, FCachedPropertyPath> OutputProperties_DEPRECATED;
#endif

private:
	
	void HandleOnControlModified(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* Control, const FTLLRN_RigControlModifiedContext& Context);

public:
	
	class TLLRN_CONTROLRIG_API FAnimAttributeContainerPtrScope
	{
	public:
		FAnimAttributeContainerPtrScope(UTLLRN_ControlRig* InTLLRN_ControlRig, UE::Anim::FStackAttributeContainer& InExternalContainer);
		~FAnimAttributeContainerPtrScope();

		UTLLRN_ControlRig* TLLRN_ControlRig;
	};
	
private:
	UPROPERTY(Transient)
	FRigVMExtendedExecuteContext RigVMExtendedExecuteContext;

	UE::Anim::FStackAttributeContainer* ExternalAnimAttributeContainer;

#if WITH_EDITOR
	void SetEnableAnimAttributeTrace(bool bInEnable)
	{
		bEnableAnimAttributeTrace = bInEnable;
	};
	
	bool bEnableAnimAttributeTrace;
	
	UE::Anim::FHeapAttributeContainer InputAnimAttributeSnapshot;
	UE::Anim::FHeapAttributeContainer OutputAnimAttributeSnapshot;
#endif
	
	/** The registry to access data source */
	UPROPERTY(Transient)
	TObjectPtr<UAnimationDataSourceRegistry> DataSourceRegistry;

	/** Broadcasts a notification when launching the construction event */
	FTLLRN_ControlRigExecuteEvent PreConstructionForUIEvent;

	/** Broadcasts a notification just before the controlrig is setup. */
	FTLLRN_ControlRigExecuteEvent PreConstructionEvent;

	/** Broadcasts a notification whenever the controlrig has been setup. */
	FTLLRN_ControlRigExecuteEvent PostConstructionEvent;

	/** Broadcasts a notification before a forward solve has been initiated */
	FTLLRN_ControlRigExecuteEvent PreForwardsSolveEvent;
	
	/** Broadcasts a notification after a forward solve has been initiated */
	FTLLRN_ControlRigExecuteEvent PostForwardsSolveEvent;

	/** Broadcasts a notification before additive controls have been applied */
	FTLLRN_ControlRigExecuteEvent PreAdditiveValuesApplicationEvent;

	/** Handle changes within the hierarchy */
	void HandleHierarchyModified(ETLLRN_RigHierarchyNotification InNotification, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);

protected:
	
	virtual void RunPostConstructionEvent();

private:
#if WITH_EDITOR
	/** Add a transient / temporary control used to interact with a node */
	FName AddTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget);

	/** Sets the value of a transient control based on a node */
	bool SetTransientControlValue(const URigVMUnitNode* InNode, TSharedPtr<FRigDirectManipulationInfo> InInfo);

	/** Remove a transient / temporary control used to interact with a node */
	FName RemoveTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget);

	FName AddTransientControl(const FTLLRN_RigElementKey& InElement);

	/** Sets the value of a transient control based on a bone */
	bool SetTransientControlValue(const FTLLRN_RigElementKey& InElement);

	/** Remove a transient / temporary control used to interact with a bone */
	FName RemoveTransientControl(const FTLLRN_RigElementKey& InElement);

	static FName GetNameForTransientControl(const FTLLRN_RigElementKey& InElement);
	FName GetNameForTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget) const;
	static FString GetNodeNameFromTransientControl(const FTLLRN_RigElementKey& InKey);
	static FString GetTargetFromTransientControl(const FTLLRN_RigElementKey& InKey);
	TSharedPtr<FRigDirectManipulationInfo> GetTLLRN_RigUnitManipulationInfoForTransientControl(const FTLLRN_RigElementKey& InKey);
	
	static FTLLRN_RigElementKey GetElementKeyFromTransientControl(const FTLLRN_RigElementKey& InKey);
	bool CanAddTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget, FString* OutFailureReason);

	/** Removes all  transient / temporary control used to interact with pins */
	void ClearTransientControls();

	UAnimPreviewInstance* PreviewInstance;

	// this is needed because PreviewInstance->ModifyBone(...) cannot modify user created bones,
	TMap<FName, FTransform> TransformOverrideForUserCreatedBones;
	
public:
	
	void ApplyTransformOverrideForUserCreatedBones();
	void ApplySelectionPoseForConstructionMode(const FName& InEventName);
	
#endif

protected:

	void HandleHierarchyEvent(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigEventContext& InEvent);
	FTLLRN_RigEventDelegate TLLRN_RigEventDelegate;

	void RestoreShapeLibrariesFromCDO();
	void OnAddShapeLibrary(const FTLLRN_ControlRigExecuteContext* InContext, const FString& InLibraryName, UTLLRN_ControlRigShapeLibrary* InShapeLibrary, bool bLogResults);
	bool OnShapeExists(const FName& InShapeName) const;
	virtual void InitializeVMsFromCDO() { Super::InitializeFromCDO(); }
	virtual void InitializeFromCDO() override;


	UPROPERTY()
	FTLLRN_RigInfluenceMapPerEvent Influences;

	const FTLLRN_RigInfluenceMap* FindInfluenceMap(const FName& InEventName);


	FTLLRN_RigElementKeyRedirector ElementKeyRedirector;

public:

	// UObject interface
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	float GetDebugBoneRadiusMultiplier() const { return DebugBoneRadiusMultiplier; }
	static FTLLRN_RigUnit* GetTLLRN_RigUnitInstanceFromScope(TSharedPtr<FStructOnScope> InScope);

public:
	//~ Begin IInterface_AssetUserData Interface
	virtual const TArray<UAssetUserData*>* GetAssetUserDataArray() const override;
	//~ End IInterface_AssetUserData Interface
protected:
	mutable TArray<TObjectPtr<UAssetUserData>> CombinedAssetUserData;

	UPROPERTY(Transient, DuplicateTransient)
	mutable TMap<FName, TObjectPtr<UDataAssetLink>> ExternalVariableDataAssetLinks;

	DECLARE_DELEGATE_RetVal(TArray<TObjectPtr<UAssetUserData>>, FGetExternalAssetUserData);
	FGetExternalAssetUserData GetExternalAssetUserDataDelegate;

private:

	void CopyPoseFromOtherRig(UTLLRN_ControlRig* Subject);

protected:
	bool bCopyHierarchyBeforeConstruction;
	bool bResetInitialTransformsBeforeConstruction;
	bool bResetCurrentTransformsAfterConstruction;
	bool bManipulationEnabled;

	int32 PreConstructionBracket;
	int32 PostConstructionBracket;
	int32 PreForwardsSolveBracket;
	int32 PostForwardsSolveBracket;
	int32 PreAdditiveValuesApplicationBracket;
	int32 InteractionBracket;
	int32 InterRigSyncBracket;
	int32 ControlUndoBracketIndex;
	uint8 InteractionType;
	TArray<FTLLRN_RigElementKey> ElementsBeingInteracted;
#if WITH_EDITOR
	TArray<TSharedPtr<FRigDirectManipulationInfo>> TLLRN_RigUnitManipulationInfos;
#endif
	bool bInteractionJustBegan;

	TWeakObjectPtr<USceneComponent> OuterSceneComponent;

	bool IsRunningPreConstruction() const
	{
		return PreConstructionBracket > 0;
	}

	bool IsRunningPostConstruction() const
	{
		return PostConstructionBracket > 0;
	}

	bool IsInteracting() const
	{
		return InteractionBracket > 0;
	}

	uint8 GetInteractionType() const
	{
		return InteractionType;
	}

	bool IsSyncingWithOtherRig() const
	{
		return InterRigSyncBracket > 0;
	}


#if WITH_EDITOR
	static void OnHierarchyTransformUndoRedoWeak(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InKey, ETLLRN_RigTransformType::Type InTransformType, const FTransform& InTransform, bool bIsUndo, TWeakObjectPtr<UTLLRN_ControlRig> WeakThis)
	{
		if(WeakThis.IsValid() && InHierarchy != nullptr)
		{
			WeakThis->OnHierarchyTransformUndoRedo(InHierarchy, InKey, InTransformType, InTransform, bIsUndo);
		}
	}
#endif
	
	void OnHierarchyTransformUndoRedo(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InKey, ETLLRN_RigTransformType::Type InTransformType, const FTransform& InTransform, bool bIsUndo);

	FFilterControlEvent OnFilterControl;
	FControlModifiedEvent OnControlModified;
	FControlSelectedEvent OnControlSelected;
	FControlUndoBracketEvent OnControlUndoBracket;
	FTLLRN_ControlRigBoundEvent OnTLLRN_ControlRigBound;

	UPROPERTY(BlueprintAssignable, Category = TLLRN_ControlRig, meta=(DisplayName="OnControlSelected"))
	FOnControlSelectedBP OnControlSelected_BP;

	TArray<FTLLRN_RigElementKey> QueuedModifiedControls;

private:

#if WITH_EDITORONLY_DATA

	/** Whether controls are visible */
	UPROPERTY(transient)
	bool bControlsVisible;

#endif


protected:
	/** An additive control rig runs a backwards solve before applying additive control values
	 * and running the forward solve
	 */
	UPROPERTY()
	bool bIsAdditive;

	struct FRigSetControlValueInfo
	{
		FTLLRN_RigControlValue Value;
		bool bNotify;
		FTLLRN_RigControlModifiedContext Context;
		bool bSetupUndo;
		bool bPrintPythonCommnds;
		bool bFixEulerFlips;
	};
	struct FRigSwitchParentInfo
	{
		FTLLRN_RigElementKey NewParent;
		bool bInitial;
		bool bAffectChildren;
	};
	FTLLRN_RigPose PoseBeforeBackwardsSolve;
	FTLLRN_RigPose ControlsAfterBackwardsSolve;
	TMap<FTLLRN_RigElementKey, FRigSetControlValueInfo> ControlValues; // Layered Rigs: Additive values in local space (to add after backwards solve)
	TMap<FTLLRN_RigElementKey, FRigSwitchParentInfo> SwitchParentValues; // Layered Rigs: Parent switching values to perform after backwards solve


private:
	float DebugBoneRadiusMultiplier;

	// Physics Solvers
	TArray<FTLLRN_RigPhysicsSolverDescription> PhysicsSolvers;

public:

	/**
	 * Returns the number of physics solvers
	 * @return The number of physics solvers
	 */
	int32 NumPhysicsSolvers() const; 

	/**
	 * Returns a physics solver by index (or nullptr)
	 * @param InIndex The index of the physics solver to return
	 * @return The physics solver
	 */
	const FTLLRN_RigPhysicsSolverDescription* GetPhysicsSolver(int32 InIndex) const; 

	/**
	 * Finds a new physics solver given its guid
	 * @param InID The id identifying the physics solver
	 * @return The physics solver
	 */
	const FTLLRN_RigPhysicsSolverDescription* FindPhysicsSolver(const FTLLRN_RigPhysicsSolverID& InID) const; 

	/**
	 * Finds a new physics solver given its name
	 * @param InName The name identifying the physics solver in the scope of this control rig
	 * @return The physics solver
	 */
	const FTLLRN_RigPhysicsSolverDescription* FindPhysicsSolverByName(const FName& InName) const; 

	/**
	 * Adds a physics solver to the hierarchy
	 * @param InName The suggested name of the new physics solver - will eventually be corrected by the namespace
	 * @param bSetupUndo If set to true the stack will record the change for undo / redo
	 * @param bPrintPythonCommand If set to true a python command equivalent to this call will be printed out
	 * @return The ID for the newly created physics solver.
	 */
	UFUNCTION(BlueprintCallable, Category = TLLRN_ControlRig)
	FTLLRN_RigPhysicsSolverID AddPhysicsSolver(
		FName InName,
		bool bSetupUndo = false,
		bool bPrintPythonCommand = false);

#if WITH_EDITOR	

	void ToggleControlsVisible() { bControlsVisible = !bControlsVisible; }
	void SetControlsVisible(const bool bIsVisible) { bControlsVisible = bIsVisible; }
	bool GetControlsVisible()const { return bControlsVisible;}

#endif
	
	virtual bool IsAdditive() const { return bIsAdditive; }
	void SetIsAdditive(const bool bInIsAdditive)
	{
		bIsAdditive = bInIsAdditive;
		if (UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
		{
			Hierarchy->bUsePreferredEulerAngles = !bIsAdditive;
		}
	}

private:

	// Class used to temporarily cache current pose of the hierarchy
	// restore it on destruction, similar to UTLLRN_ControlRigBlueprint::FControlValueScope
	class FPoseScope
	{
	public:
		FPoseScope(UTLLRN_ControlRig* InTLLRN_ControlRig, ETLLRN_RigElementType InFilter = ETLLRN_RigElementType::All,
			const TArray<FTLLRN_RigElementKey>& InElements = TArray<FTLLRN_RigElementKey>(), const ETLLRN_RigTransformType::Type InTransformType = ETLLRN_RigTransformType::CurrentLocal);
		~FPoseScope();

	private:

		UTLLRN_ControlRig* TLLRN_ControlRig;
		ETLLRN_RigElementType Filter;
		FTLLRN_RigPose CachedPose;
		ETLLRN_RigTransformType::Type TransformType;
	};

	UPROPERTY()
	FTLLRN_RigModuleSettings TLLRN_RigModuleSettings;

	UPROPERTY(transient)
	mutable FString TLLRN_RigModuleNameSpace;


public:

#if WITH_EDITOR

	// Class used to temporarily cache current transient controls
	// restore them after a CopyHierarchy call
	class FTransientControlScope
	{
	public:
		FTransientControlScope(TObjectPtr<UTLLRN_RigHierarchy> InHierarchy);
		~FTransientControlScope();
	
	private:
		// used to match UTLLRN_RigHierarchyController::AddControl(...)
		struct FTransientControlInfo
		{
			FName Name;
			// transient control should only have 1 parent, with weight = 1.0
			FTLLRN_RigElementKey Parent;
			FTLLRN_RigControlSettings Settings;
			FTLLRN_RigControlValue Value;
			FTransform OffsetTransform;
			FTransform ShapeTransform;
		};
		
		TArray<FTransientControlInfo> SavedTransientControls;
		TObjectPtr<UTLLRN_RigHierarchy> Hierarchy;
	};

	// Class used to temporarily cache current pose of transient controls
	// restore them after a ResetPoseToInitial call,
	// which allows user to move bones in construction mode
	class FTransientControlPoseScope
	{
	public:
		FTransientControlPoseScope(TObjectPtr<UTLLRN_ControlRig> InTLLRN_ControlRig)
		{
			TLLRN_ControlRig = InTLLRN_ControlRig;

			TArray<FTLLRN_RigControlElement*> TransientControls = TLLRN_ControlRig->GetHierarchy()->GetTransientControls();
			TArray<FTLLRN_RigElementKey> Keys;
			for(FTLLRN_RigControlElement* TransientControl : TransientControls)
			{
				Keys.Add(TransientControl->GetKey());
			}

			if(Keys.Num() > 0)
			{
				CachedPose = TLLRN_ControlRig->GetHierarchy()->GetPose(false, ETLLRN_RigElementType::Control, TArrayView<FTLLRN_RigElementKey>(Keys));
			}
		}
		~FTransientControlPoseScope()
		{
			check(TLLRN_ControlRig);

			if(CachedPose.Num() > 0)
			{
				TLLRN_ControlRig->GetHierarchy()->SetPose(CachedPose);
			}
		}
	
	private:
		
		UTLLRN_ControlRig* TLLRN_ControlRig;
		FTLLRN_RigPose CachedPose;	
	};	

	bool bRecordSelectionPoseForConstructionMode;
	TMap<FTLLRN_RigElementKey, FTransform> SelectionPoseForConstructionMode;
	bool bIsClearingTransientControls;

	FTLLRN_RigPose InputPoseOnDebuggedRig;
	
#endif

public:
	UE_DEPRECATED(5.4, "InteractionRig is no longer used") UFUNCTION(BlueprintGetter, meta = (DeprecatedFunction, DeprecationMessage = "InteractionRig is no longer used"))
	UTLLRN_ControlRig* GetInteractionRig() const
	{
#if WITH_EDITORONLY_DATA
		return InteractionRig_DEPRECATED;
#else
		return nullptr;
#endif
	}

	UE_DEPRECATED(5.4, "InteractionRig is no longer used")
	UFUNCTION(BlueprintSetter, meta = (DeprecatedFunction, DeprecationMessage = "InteractionRig is no longer used"))
	void SetInteractionRig(UTLLRN_ControlRig* InInteractionRig) {}

	UE_DEPRECATED(5.4, "InteractionRig is no longer used")
	UFUNCTION(BlueprintGetter, meta = (DeprecatedFunction, DeprecationMessage = "InteractionRig is no longer used"))
	TSubclassOf<UTLLRN_ControlRig> GetInteractionRigClass() const
	{
#if WITH_EDITORONLY_DATA
		return InteractionRigClass_DEPRECATED;
#else
		return nullptr;
#endif
	}

	UE_DEPRECATED(5.4, "InteractionRig is no longer used")
	UFUNCTION(BlueprintSetter, meta = (DeprecatedFunction, DeprecationMessage = "InteractionRig is no longer used"))
	void SetInteractionRigClass(TSubclassOf<UTLLRN_ControlRig> InInteractionRigClass) {}

	uint32 GetShapeLibraryHash() const;
	
private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UTLLRN_ControlRig> InteractionRig_DEPRECATED;

	UPROPERTY()
	TSubclassOf<UTLLRN_ControlRig> InteractionRigClass_DEPRECATED;
#endif

	friend class FTLLRN_ControlRigBlueprintCompilerContext;
	friend struct FTLLRN_RigHierarchyRef;
	friend class FTLLRN_ControlRigEditor;
	friend class STLLRN_TLLRN_RigCurveContainer;
	friend class STLLRN_RigHierarchy;
	friend class STLLRN_ControlRigAnimAttributeView;
	friend class UEngineTestTLLRN_ControlRig;
 	friend class FTLLRN_ControlRigEditMode;
	friend class UTLLRN_ControlRigBlueprint;
	friend class UTLLRN_ControlRigComponent;
	friend class UTLLRN_ControlRigBlueprintGeneratedClass;
	friend class FTLLRN_ControlRigInteractionScope;
	friend class UTLLRN_ControlRigValidator;
	friend struct FAnimNode_TLLRN_ControlRig;
	friend struct FAnimNode_TLLRN_ControlRigBase;
	friend class UTLLRN_RigHierarchy;
	friend class UFKTLLRN_ControlRig;
	friend class UTLLRN_ControlRigGraph;
	friend class ATLLRN_ControlTLLRN_RigControlActor;
	friend class ATLLRN_ControlRigShapeActor;
	friend class FTLLRN_RigTransformElementDetails;
	friend class FTLLRN_ControlRigEditorModule;
	friend class UTLLRN_ModularRig;
	friend class UTLLRN_ModularTLLRN_RigController;
};

class TLLRN_CONTROLRIG_API FTLLRN_ControlRigBracketScope
{
public:

	FTLLRN_ControlRigBracketScope(int32& InBracket)
		: Bracket(InBracket)
	{
		Bracket++;
	}

	~FTLLRN_ControlRigBracketScope()
	{
		Bracket--;
	}

private:

	int32& Bracket;
};

class TLLRN_CONTROLRIG_API FTLLRN_ControlRigInteractionScope
{
public:

	FTLLRN_ControlRigInteractionScope(UTLLRN_ControlRig* InTLLRN_ControlRig)
		: TLLRN_ControlRig(InTLLRN_ControlRig)
		, InteractionBracketScope(InTLLRN_ControlRig->InteractionBracket)
		, SyncBracketScope(InTLLRN_ControlRig->InterRigSyncBracket)
	{
		InTLLRN_ControlRig->GetHierarchy()->StartInteraction();
	}

	FTLLRN_ControlRigInteractionScope(
		UTLLRN_ControlRig* InTLLRN_ControlRig,
		const FTLLRN_RigElementKey& InKey,
		ETLLRN_ControlRigInteractionType InInteractionType = ETLLRN_ControlRigInteractionType::All
	)
		: TLLRN_ControlRig(InTLLRN_ControlRig)
		, InteractionBracketScope(InTLLRN_ControlRig->InteractionBracket)
		, SyncBracketScope(InTLLRN_ControlRig->InterRigSyncBracket)
	{
		InTLLRN_ControlRig->ElementsBeingInteracted = { InKey };
		InTLLRN_ControlRig->InteractionType = (uint8)InInteractionType;
		InTLLRN_ControlRig->bInteractionJustBegan = true;
		InTLLRN_ControlRig->GetHierarchy()->StartInteraction();
	}

	FTLLRN_ControlRigInteractionScope(
		UTLLRN_ControlRig* InTLLRN_ControlRig,
		const TArray<FTLLRN_RigElementKey>& InKeys,
		ETLLRN_ControlRigInteractionType InInteractionType = ETLLRN_ControlRigInteractionType::All
	)
		: TLLRN_ControlRig(InTLLRN_ControlRig)
		, InteractionBracketScope(InTLLRN_ControlRig->InteractionBracket)
	, SyncBracketScope(InTLLRN_ControlRig->InterRigSyncBracket)
	{
		InTLLRN_ControlRig->ElementsBeingInteracted = InKeys;
		InTLLRN_ControlRig->InteractionType = (uint8)InInteractionType;
		InTLLRN_ControlRig->bInteractionJustBegan = true;
		InTLLRN_ControlRig->GetHierarchy()->StartInteraction();
	}

	~FTLLRN_ControlRigInteractionScope()
	{
		if(ensure(TLLRN_ControlRig.IsValid()))
		{
			TLLRN_ControlRig->GetHierarchy()->EndInteraction();
			TLLRN_ControlRig->InteractionType = (uint8)ETLLRN_ControlRigInteractionType::None;
			TLLRN_ControlRig->bInteractionJustBegan = false;
			TLLRN_ControlRig->ElementsBeingInteracted.Reset();
		}
	}

private:

	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;
	FTLLRN_ControlRigBracketScope InteractionBracketScope;
	FTLLRN_ControlRigBracketScope SyncBracketScope;
};

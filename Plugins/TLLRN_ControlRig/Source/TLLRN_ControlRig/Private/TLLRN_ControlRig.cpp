// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRig.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "Misc/RuntimeErrors.h"
#include "ITLLRN_ControlRigObjectBinding.h"
#include "HelperUtil.h"
#include "ObjectTrace.h"
#include "RigVMBlueprintGeneratedClass.h"
#include "TLLRN_ControlRigObjectVersion.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InteractionExecution.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimationPoseData.h"
#include "Internationalization/TextKey.h"
#include "UObject/Package.h"
#if WITH_EDITOR
#include "TLLRN_ControlTLLRN_RigModule.h"
#include "Modules/ModuleManager.h"
#include "RigVMModel/RigVMNode.h"
#include "Engine/Blueprint.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "RigVMDeveloperTypeUtils.h"
#include "Editor.h"
#include "RigVMBlueprint.h"
#include "RigVMCompiler/RigVMCompiler.h"
#endif// WITH_EDITOR
#include "TLLRN_ControlRigComponent.h"
#include "Constraints/TLLRN_ControlTLLRN_RigTransformableHandle.h"
#include "RigVMCore/RigVMNativized.h"
#include "UObject/UObjectIterator.h"
#include "RigVMCore/RigVMAssetUserData.h"
#include "TLLRN_ModularRig.h"
#include "Units/Modules/TLLRN_RigUnit_ConnectorExecution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRig)

#define LOCTEXT_NAMESPACE "TLLRN_ControlRig"

DEFINE_LOG_CATEGORY(LogTLLRN_ControlRig);

DECLARE_STATS_GROUP(TEXT("TLLRN_ControlRig"), STATGROUP_TLLRN_ControlRig, STATCAT_Advanced);
DECLARE_CYCLE_STAT_EXTERN(TEXT("TLLRN Control Rig Execution"), STAT_RigExecution, STATGROUP_TLLRN_ControlRig, );
DEFINE_STAT(STAT_RigExecution);

const FName UTLLRN_ControlRig::OwnerComponent("OwnerComponent");

//CVar to specify if we should create a float control for each curve in the curve container
//By default we don't but it may be useful to do so for debugging
static TAutoConsoleVariable<int32> CVarTLLRN_ControlRigCreateFloatControlsForCurves(
	TEXT("TLLRN_ControlRig.CreateFloatControlsForCurves"),
	0,
	TEXT("If nonzero we create a float control for each curve in the curve container, useful for debugging low level controls."),
	ECVF_Default);

//CVar to specify if we should allow debug drawing in game (during shipped game or PIE)
//By default we don't for performance 
static TAutoConsoleVariable<float> CVarTLLRN_ControlRigEnableDrawInterfaceInGame(
	TEXT("TLLRN_ControlRig.EnableDrawInterfaceInGame"),
	0,
	TEXT("If nonzero debug drawing will be enabled during play."),
	ECVF_Default);


UTLLRN_ControlRig::UTLLRN_ControlRig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
#if WITH_EDITOR
	, bIsRunningInPIE(false)
	, bEnableAnimAttributeTrace(false)
#endif 
	, DataSourceRegistry(nullptr)
#if WITH_EDITOR
	, PreviewInstance(nullptr)
#endif
	, bCopyHierarchyBeforeConstruction(true)
	, bResetInitialTransformsBeforeConstruction(true)
	, bResetCurrentTransformsAfterConstruction(false)
	, bManipulationEnabled(false)
	, PreConstructionBracket(0)
	, PostConstructionBracket(0)
	, InteractionBracket(0)
	, InterRigSyncBracket(0)
	, ControlUndoBracketIndex(0)
	, InteractionType((uint8)ETLLRN_ControlRigInteractionType::None)
	, bInteractionJustBegan(false)
	, bIsAdditive(false)
	, DebugBoneRadiusMultiplier(1.f)
#if WITH_EDITOR
	, bRecordSelectionPoseForConstructionMode(true)
	, bIsClearingTransientControls(false)
#endif
{
	EventQueue.Add(FTLLRN_RigUnit_BeginExecution::EventName);

	SetRigVMExtendedExecuteContext(&RigVMExtendedExecuteContext);
}

#if WITH_EDITOR

void UTLLRN_ControlRig::ResetRecordedTransforms(const FName& InEventName)
{
	if(const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		if(Hierarchy->bRecordTransformsAtRuntime)
		{
			bool bResetRecordedTransforms = SupportsEvent(InEventName);
			if(InEventName == FTLLRN_RigUnit_PostBeginExecution::EventName ||
				InEventName == FTLLRN_RigUnit_PreBeginExecution::EventName)
			{
				bResetRecordedTransforms = false;
			}
			
			if(bResetRecordedTransforms)
			{
				Hierarchy->ReadTransformsAtRuntime.Reset();
				Hierarchy->WrittenTransformsAtRuntime.Reset();
			}
		}
	}
}

#endif

void UTLLRN_ControlRig::BeginDestroy()
{
	BeginDestroyEvent.Broadcast(this);
	BeginDestroyEvent.Clear();
	
	Super::BeginDestroy();
	SetRigVMExtendedExecuteContext(nullptr);

	PreConstructionEvent.Clear();
	PostConstructionEvent.Clear();
	PreForwardsSolveEvent.Clear();
	PostForwardsSolveEvent.Clear();
	PreAdditiveValuesApplicationEvent.Clear();

#if WITH_EDITOR
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if(UTLLRN_ControlRig* CDO = GetClass()->GetDefaultObject<UTLLRN_ControlRig>())
		{
			if (!IsGarbageOrDestroyed(CDO))
			{
				if (CDO->GetHierarchy())
				{
					CDO->GetHierarchy()->UnregisterListeningHierarchy(GetHierarchy());
				}
			}
		}
	}
#endif

	TRACE_OBJECT_LIFETIME_END(this);
}

UWorld* UTLLRN_ControlRig::GetWorld() const
{
	if (ObjectBinding.IsValid())
	{
		AActor* HostingActor = ObjectBinding->GetHostingActor();
		if (HostingActor)
		{
			return HostingActor->GetWorld();
		}

		UObject* Owner = ObjectBinding->GetBoundObject();
		if (Owner)
		{
			return Owner->GetWorld();
		}
	}
	return Super::GetWorld();
}

void UTLLRN_ControlRig::Initialize(bool bRequestInit)
{
	TRACE_OBJECT_LIFETIME_BEGIN(this);

	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()
	QUICK_SCOPE_CYCLE_COUNTER(STAT_TLLRN_ControlRig_Initialize);

	InitializeVMs(bRequestInit);

	// Create the data source registry here to avoid UObject creation from Non-Game Threads
	GetDataSourceRegistry();

	// Create the Hierarchy Controller here to avoid UObject creation from Non-Game Threads
	if(!IsTLLRN_RigModuleInstance())
	{
		GetHierarchy()->GetController(true);
	}

#if WITH_EDITOR
	bIsRunningInPIE = GEditor && GEditor->IsPlaySessionInProgress();
#endif
	
	// should refresh mapping 
	RequestConstruction();

	if(!IsTLLRN_RigModuleInstance())
	{
		GetHierarchy()->OnModified().RemoveAll(this);
		GetHierarchy()->OnModified().AddUObject(this, &UTLLRN_ControlRig::HandleHierarchyModified);
		GetHierarchy()->OnEventReceived().RemoveAll(this);
		GetHierarchy()->OnEventReceived().AddUObject(this, &UTLLRN_ControlRig::HandleHierarchyEvent);
		GetHierarchy()->UpdateVisibilityOnProxyControls();
	}
}

void UTLLRN_ControlRig::RestoreShapeLibrariesFromCDO()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if(UTLLRN_ControlRig* CDO = GetClass()->GetDefaultObject<UTLLRN_ControlRig>())
		{
			ShapeLibraryNameMap.Reset();
			ShapeLibraries = CDO->ShapeLibraries;
		}
	}
}

void UTLLRN_ControlRig::OnAddShapeLibrary(const FTLLRN_ControlRigExecuteContext* InContext, const FString& InLibraryName, UTLLRN_ControlRigShapeLibrary* InShapeLibrary, bool bLogResults)
{
	// don't ever change the CDO
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	if(InShapeLibrary == nullptr)
	{
		if(InContext)
		{
			static constexpr TCHAR Message[] = TEXT("No shape library provided.");
			InContext->Log(EMessageSeverity::Error, Message);
		}
		return;
	}

	// remove the shape library in case it was there before
	ShapeLibraryNameMap.Remove(InShapeLibrary->GetName());
	ShapeLibraries.Remove(InShapeLibrary);

	// if we've removed all shape libraries - let's add the ones from the CDO back
	if (ShapeLibraries.IsEmpty())
	{
		RestoreShapeLibrariesFromCDO();
	}

	// if we are supposed to replace the library and the library name is empty
	FString LibraryName = InLibraryName;
	if(LibraryName.IsEmpty())
	{
		LibraryName = InShapeLibrary->GetName();
	}

	if(LibraryName != InShapeLibrary->GetName())
	{
		ShapeLibraryNameMap.FindOrAdd(InShapeLibrary->GetName()) = LibraryName;
	}
	ShapeLibraries.AddUnique(InShapeLibrary);

#if WITH_EDITOR
	if(bLogResults)
	{
		static constexpr TCHAR MapFormat[] = TEXT("TLLRN Control Rig '%s': Shape Library Name Map: '%s' -> '%s'");
		static constexpr TCHAR LibraryFormat[] = TEXT("TLLRN Control Rig '%s': Shape Library '%s' uses asset '%s'");
		static constexpr TCHAR DefaultShapeFormat[] = TEXT("TLLRN Control Rig '%s': Shape Library '%s' has default shape '%s'");
		static constexpr TCHAR ShapeFormat[] = TEXT("TLLRN Control Rig '%s': Shape Library '%s' contains shape %03d: '%s'");
		static constexpr TCHAR ResolvedShapeFormat[] = TEXT("TLLRN Control Rig '%s': ShapeName '%s' resolved to '%s.%s'");

		const FString PathName = GetPathName();

		for(const TPair<FString, FString>& Pair: ShapeLibraryNameMap)
		{
			UE_LOG(LogTLLRN_ControlRig, Display, MapFormat, *PathName, *Pair.Key, *Pair.Value);
		}

		const int32 NumShapeLibraries = ShapeLibraries.Num();

		TMap<FString, const UTLLRN_ControlRigShapeLibrary*> ShapeNameToShapeLibrary;
		for(int32 Index = 0; Index <NumShapeLibraries; Index++)
		{
			const TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>& ShapeLibrary = ShapeLibraries[Index];
			if(ShapeLibrary.IsNull())
			{
				continue;
			}

			UE_LOG(LogTLLRN_ControlRig, Display, LibraryFormat, *PathName, *ShapeLibrary->GetName(), *ShapeLibrary->GetPathName());

			const bool bUseNameSpace = ShapeLibraries.Num() > 1;
			const FString DefaultShapeName = UTLLRN_ControlRigShapeLibrary::GetShapeName(ShapeLibrary.Get(), bUseNameSpace, ShapeLibraryNameMap, ShapeLibrary->DefaultShape);
			UE_LOG(LogTLLRN_ControlRig, Display, DefaultShapeFormat, *PathName, *ShapeLibrary->GetName(), *DefaultShapeName);

			for(int32 ShapeIndex = 0; ShapeIndex < ShapeLibrary->Shapes.Num(); ShapeIndex++)
			{
				const FString ShapeName = UTLLRN_ControlRigShapeLibrary::GetShapeName(ShapeLibrary.Get(), bUseNameSpace, ShapeLibraryNameMap, ShapeLibrary->Shapes[ShapeIndex]);
				UE_LOG(LogTLLRN_ControlRig, Display, ShapeFormat, *PathName, *ShapeLibrary->GetName(), ShapeIndex, *ShapeName);
				ShapeNameToShapeLibrary.FindOrAdd(ShapeName, nullptr) = ShapeLibrary.Get();
			}
		}

		for(const TPair<FString, const UTLLRN_ControlRigShapeLibrary*>& Pair : ShapeNameToShapeLibrary)
		{
			FString Left, Right = Pair.Key;
			(void)Pair.Key.Split(TEXT("."), &Left, &Right, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			UE_LOG(LogTLLRN_ControlRig, Display, ResolvedShapeFormat, *PathName, *Pair.Key, *Pair.Value->GetName(), *Right);
		}
	}
#endif
}

bool UTLLRN_ControlRig::OnShapeExists(const FName& InShapeName) const
{
	const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>>& Libraries = GetShapeLibraries();
	if (UTLLRN_ControlRigShapeLibrary::GetShapeByName(InShapeName, GetShapeLibraries(), ShapeLibraryNameMap, false))
	{
		return true;
	}
	
	return false;
}

bool UTLLRN_ControlRig::InitializeVM(const FName& InEventName)
{
	if(!InitializeVMs(InEventName))
	{
		return false;
	}

	RequestConstruction();

#if WITH_EDITOR
	// setup the hierarchy's controller log function
	if (UTLLRN_RigHierarchyController* HierarchyController = GetHierarchy()->GetController(true))
	{
		HierarchyController->LogFunction = [this](EMessageSeverity::Type InSeverity, const FString& Message)
		{
			const FRigVMExecuteContext& PublicContext = GetRigVMExtendedExecuteContext().GetPublicData<>();
			if(RigVMLog)
			{
				RigVMLog->Report(InSeverity,PublicContext.GetFunctionName(),PublicContext.GetInstructionIndex(), Message);
			}
			else
			{
				LogOnce(InSeverity,PublicContext.GetInstructionIndex(), Message);
			}
		};
	}
#endif

	return true;
}

void UTLLRN_ControlRig::Evaluate_AnyThread()
{
	if (bIsAdditive)
	{
		if (bRequiresInitExecution)
		{
			Super::Evaluate_AnyThread();
		}
		
		// we can have other systems trying to poke into running instances of Control Rigs
		// on the anim thread and query data, such as
		// URigVMHostSkeletalMeshComponent::RebuildDebugDrawSkeleton,
		// using a lock here to prevent them from having an inconsistent view of the rig at some
		// intermediate stage of evaluation, for example, during evaluate, we can have a call
		// to copy hierarchy, which empties the hierarchy for a short period of time
		// and we don't want other systems to see that.
		FScopeLock EvaluateLock(&GetEvaluateMutex());

		UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

		// Reset all control local transforms to initial, and switch to default parents
		Hierarchy->ForEach([Hierarchy](FTLLRN_RigBaseElement* Element) -> bool
		{
			if (FTLLRN_RigControlElement* Control = Cast<FTLLRN_RigControlElement>(Element))
			{
				if (Control->CanTreatAsAdditive())
				{
					if (Hierarchy->GetActiveParent(Control->GetKey()) != UTLLRN_RigHierarchy::GetDefaultParentKey())
					{
						Hierarchy->SwitchToDefaultParent(Control);
					}
					Hierarchy->SetTransform(Control, Hierarchy->GetTransform(Control, ETLLRN_RigTransformType::InitialLocal), ETLLRN_RigTransformType::CurrentLocal, true);
				}
			}
			return true;
		});

		if (PoseBeforeBackwardsSolve.Num() == 0)
		{
			// If the pose is empty, this is an indication that a new pose is coming in
			uint8 Types = (uint8)ETLLRN_RigElementType::Bone | (uint8)ETLLRN_RigElementType::Curve;
			PoseBeforeBackwardsSolve = Hierarchy->GetPose(false, (ETLLRN_RigElementType)Types, TArrayView<const FTLLRN_RigElementKey>());
		}
		else
		{
			// Restore the pose from the anim sequence
			Hierarchy->SetPose(PoseBeforeBackwardsSolve);
		}
		
		// Backwards solve
		{
			Execute(FTLLRN_RigUnit_InverseExecution::EventName);
		}

		// Switch parents
		for (TPair<FTLLRN_RigElementKey, FRigSwitchParentInfo>& Pair : SwitchParentValues)
		{
			if (FTLLRN_RigBaseElement* Element = Hierarchy->Find(Pair.Key))
			{
				if (FTLLRN_RigBaseElement* NewParent = Hierarchy->Find(Pair.Value.NewParent))
				{
#if WITH_EDITOR
					UTLLRN_RigHierarchy::TElementDependencyMap DependencyMap = Hierarchy->GetDependenciesForVM(GetVM());
					Hierarchy->SwitchToParent(Element, NewParent, Pair.Value.bInitial, Pair.Value.bAffectChildren, DependencyMap, nullptr);
#else
					Hierarchy->SwitchToParent(Element, NewParent, Pair.Value.bInitial, Pair.Value.bAffectChildren);
#endif
				}
			}
		}

		// Store control pose after backwards solve to figure out additive local transforms based on animation
		// This needs to happen after the switch parents
		ControlsAfterBackwardsSolve = Hierarchy->GetPose(false, ETLLRN_RigElementType::Control, TArrayView<const FTLLRN_RigElementKey>());

		if (PreAdditiveValuesApplicationEvent.IsBound())
		{
			PreAdditiveValuesApplicationEvent.Broadcast(this, TEXT("Additive"));
		}
		
		// Apply additive controls
		for (TPair<FTLLRN_RigElementKey, FRigSetControlValueInfo>& Value : ControlValues)
		{
			if (FTLLRN_RigBaseElement* Element = Hierarchy->Find(Value.Key))
			{
				if (FTLLRN_RigControlElement* Control = Cast<FTLLRN_RigControlElement>(Element))
				{
					FRigSetControlValueInfo& Info = Value.Value;

					// A bool/enum value is not an additive property. We just overwrite the value.
					if (!Control->CanTreatAsAdditive())
					{
						const bool bSetupUndo = false; // Rely on the sequencer track to handle undo/redo
						Hierarchy->SetControlValue(Control, Info.Value, ETLLRN_RigControlValueType::Current, bSetupUndo, false, Info.bPrintPythonCommnds, false);
					}
					else
					{
						// Transform from animation
						const FTLLRN_RigControlValue PreviousValue = Hierarchy->GetControlValue(Control, ETLLRN_RigControlValueType::Current, false);
						const FTransform PreviousTransform = PreviousValue.GetAsTransform(Control->Settings.ControlType, Control->Settings.PrimaryAxis);

						// Additive transform from controls
						const FTLLRN_RigControlValue& AdditiveValue = Info.Value;
						const FTransform AdditiveTransform = AdditiveValue.GetAsTransform(Control->Settings.ControlType, Control->Settings.PrimaryAxis);

						// Add them to find the final value
						FTransform FinalTransform = AdditiveTransform * PreviousTransform;
						FinalTransform.SetRotation(FinalTransform.GetRotation().GetNormalized());

						FTLLRN_RigControlValue FinalValue;
						FinalValue.SetFromTransform(FinalTransform, Control->Settings.ControlType, Control->Settings.PrimaryAxis);

						const bool bSetupUndo = false; // Rely on the sequencer track to handle undo/redo
						Hierarchy->SetControlValue(Control, FinalValue, ETLLRN_RigControlValueType::Current, bSetupUndo, false, Info.bPrintPythonCommnds, false);
					}

					if (Info.bNotify && OnControlModified.IsBound())
					{
						OnControlModified.Broadcast(this, Control, Info.Context);
						Info.bNotify = false;
					}
				}
			}
		}
		
		// Forward solve
		{
			Execute(FTLLRN_RigUnit_BeginExecution::EventName);
		}
	}
	else
	{
		Super::Evaluate_AnyThread();
	}
}

bool UTLLRN_ControlRig::EvaluateSkeletalMeshComponent(double InDeltaTime)
{
	if (USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(GetObjectBinding()->GetBoundObject()))
	{
		SkelMeshComp->TickAnimation(InDeltaTime, false);

		SkelMeshComp->RefreshBoneTransforms();
		SkelMeshComp->RefreshFollowerComponents();
		SkelMeshComp->UpdateComponentToWorld();
		SkelMeshComp->FinalizeBoneTransform();
		SkelMeshComp->MarkRenderTransformDirty();
		SkelMeshComp->MarkRenderDynamicDataDirty();
		return true;
	}
	return false;
}

void UTLLRN_ControlRig::ResetControlValues()
{
	ControlValues.Reset();
}

void UTLLRN_ControlRig::ClearPoseBeforeBackwardsSolve()
{
	PoseBeforeBackwardsSolve.Reset();
}

TArray<FTLLRN_RigControlElement*> UTLLRN_ControlRig::InvertInputPose(const TArray<FTLLRN_RigElementKey>& InElements, ETLLRN_ControlRigSetKey InSetKey)
{
	TArray<FTLLRN_RigControlElement*> ModifiedElements;
	ModifiedElements.Reserve(ControlsAfterBackwardsSolve.Num());
	for (const FTLLRN_RigPoseElement& PoseElement : ControlsAfterBackwardsSolve)
	{
		if (FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(DynamicHierarchy->Get(PoseElement.Index)))
		{
			if (IsAdditive() && !ControlElement->CanTreatAsAdditive())
			{
				continue;
			}
			
			if (InElements.IsEmpty() || InElements.Contains(ControlElement->GetKey()))
			{
				FTLLRN_RigControlValue Value;
				Value.SetFromTransform(PoseElement.LocalTransform.Inverse(), ControlElement->Settings.ControlType, ControlElement->Settings.PrimaryAxis);
				FRigSetControlValueInfo Info = {Value, true, FTLLRN_RigControlModifiedContext(InSetKey), false, false, false};
				ControlValues.Add(ControlElement->GetKey(), Info);
				ModifiedElements.Add(ControlElement);
			}
		}
	}
	return ModifiedElements;
}

void UTLLRN_ControlRig::InitializeFromCDO()
{
	InitializeVMsFromCDO();
	
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	// copy CDO property you need to here
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (!IsTLLRN_RigModuleInstance())
		{
			// similar to FTLLRN_ControlRigBlueprintCompilerContext::CopyTermDefaultsToDefaultObject,
			// where CDO is initialized from BP there,
			// we initialize all other instances of Control Rig from the CDO here
			UTLLRN_ControlRig* CDO = GetClass()->GetDefaultObject<UTLLRN_ControlRig>();
			UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

			// copy physics solvers
			PhysicsSolvers = CDO->PhysicsSolvers;

			// copy hierarchy
			{
				FTLLRN_RigHierarchyValidityBracket ValidityBracketA(Hierarchy);
				FTLLRN_RigHierarchyValidityBracket ValidityBracketB(CDO->GetHierarchy());
			
				TGuardValue<bool> Guard(Hierarchy->GetSuspendNotificationsFlag(), true);
				Hierarchy->CopyHierarchy(CDO->GetHierarchy());
				Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::All);
			}

			// update the physics solvers with new unique IDs and remap the physics solver IDs within hierarchy
			{
				TMap<FTLLRN_RigPhysicsSolverID, FTLLRN_RigPhysicsSolverID> PhysicsSolverMap;
				for(FTLLRN_RigPhysicsSolverDescription& Solver : PhysicsSolvers)
				{
					const FTLLRN_RigPhysicsSolverID OldID = Solver.ID;
					Solver.ID = FTLLRN_RigPhysicsSolverDescription::MakeID(GetPathName(), Solver.Name);
					PhysicsSolverMap.Add(OldID, Solver.ID);
				}
				const TArray<FTLLRN_RigPhysicsElement*> PhysicsElements = Hierarchy->GetPhysicsElements();
				for(FTLLRN_RigPhysicsElement* PhysicsElement : PhysicsElements)
				{
					if(const FTLLRN_RigPhysicsSolverID* NewID = PhysicsSolverMap.Find(PhysicsElement->Solver))
					{
						PhysicsElement->Solver = *NewID;
					}
				}
			}

#if WITH_EDITOR
			// current hierarchy should always mirror CDO's hierarchy whenever a change of interest happens
			CDO->GetHierarchy()->RegisterListeningHierarchy(Hierarchy);
#endif

			// notify clients that the hierarchy has changed
			Hierarchy->Notify(ETLLRN_RigHierarchyNotification::HierarchyReset, nullptr);

			// copy hierarchy settings
			HierarchySettings = CDO->HierarchySettings;
			ElementKeyRedirector = FTLLRN_RigElementKeyRedirector(CDO->ElementKeyRedirector, Hierarchy); 
		
			// increment the procedural limit based on the number of elements in the CDO
			if(const UTLLRN_RigHierarchy* CDOHierarchy = CDO->GetHierarchy())
			{
				HierarchySettings.ProceduralElementLimit += CDOHierarchy->Num();
			}
		}

		ExternalVariableDataAssetLinks.Reset();
	}
}

UTLLRN_ControlRig::FAnimAttributeContainerPtrScope::FAnimAttributeContainerPtrScope(UTLLRN_ControlRig* InTLLRN_ControlRig,
	UE::Anim::FStackAttributeContainer& InExternalContainer)
{
	TLLRN_ControlRig = InTLLRN_ControlRig;
	TLLRN_ControlRig->ExternalAnimAttributeContainer = &InExternalContainer;
}

UTLLRN_ControlRig::FAnimAttributeContainerPtrScope::~FAnimAttributeContainerPtrScope()
{
	// control rig should not hold on to this container since it is stack allocated
	// and should not be used outside of stack, see FPoseContext
	TLLRN_ControlRig->ExternalAnimAttributeContainer = nullptr;
}

AActor* UTLLRN_ControlRig::GetHostingActor() const
{
	return ObjectBinding ? ObjectBinding->GetHostingActor() : nullptr;
}

#if WITH_EDITOR
FText UTLLRN_ControlRig::GetCategory() const
{
	return LOCTEXT("DefaultTLLRN_ControlRigCategory", "Animation|TLLRN_ControlRigs");
}

FText UTLLRN_ControlRig::GetToolTipText() const
{
	return LOCTEXT("DefaultTLLRN_ControlRigTooltip", "TLLRN_ControlRig");
}
#endif

bool UTLLRN_ControlRig::AllConnectorsAreResolved(FString* OutFailureReason, FTLLRN_RigElementKey* OutConnector) const
{
	if(const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		// todo: introduce a cache here based on ElementKeyDirector hash and
		// topology hash of the hierarchy
		
		const TArray<FTLLRN_RigModuleConnector> Connectors = GetTLLRN_RigModuleSettings().ExposedConnectors;

		// collect the connection map
		TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> ConnectionMap;
		for(const FTLLRN_RigModuleConnector& Connector : Connectors)
		{
			if (Connector.Settings.bOptional)
			{
				continue;
			}
			
			const FTLLRN_RigElementKey ConnectorKey(*Connector.Name, ETLLRN_RigElementType::Connector);
			if(const FTLLRN_CachedTLLRN_RigElement* Cache = ElementKeyRedirector.Find(ConnectorKey))
			{
				if(const_cast<FTLLRN_CachedTLLRN_RigElement*>(Cache)->UpdateCache(Hierarchy))
				{
					ConnectionMap.Add(ConnectorKey, Cache->GetKey());
				}
				else
				{
					if(OutFailureReason)
					{
						static constexpr TCHAR Format[] = TEXT("Connector '%s%s' has invalid target '%s'.");
						*OutFailureReason = FString::Printf(Format, *GetTLLRN_RigModuleNameSpace(), *Connector.Name, *Cache->GetKey().ToString());
					}
					if(OutConnector)
					{
						*OutConnector = ConnectorKey;
					}
					return false;
				}
			}
			else
			{
				if(OutFailureReason)
				{
					static constexpr TCHAR Format[] = TEXT("Connector '%s' is not resolved.");
					*OutFailureReason = FString::Printf(Format, *Connector.Name);
				}
				if(OutConnector)
				{
					*OutConnector = ConnectorKey;
				}
				return false;
			}
		}
	}
	return true;
}

bool UTLLRN_ControlRig::Execute(const FName& InEventName)
{
	if(!CanExecute())
	{
		return false;
	}

	// Only top-level rigs should execute this function.
	// Rig modules/nested rigs should run through Execute_Internal
	if (InEventName != FTLLRN_RigUnit_PrepareForExecution::EventName)
	{
		ensureMsgf(GetTypedOuter<UTLLRN_ControlRig>() == nullptr, TEXT("UTLLRN_ControlRig::Execute running from a nested rig in %s"), *GetPackage()->GetPathName());
	}
	// ensureMsgf(InEventName != FTLLRN_RigUnit_PreBeginExecution::EventName &&
	// 					InEventName != FTLLRN_RigUnit_PostBeginExecution::EventName, TEXT("Requested execution of invalid event %s on top level rig in %s"), *InEventName.ToString(), *GetPackage()->GetPathName());

	bool bJustRanInit = false;
	if(bRequiresInitExecution)
	{
		const TGuardValue<float> AbsoluteTimeGuard(AbsoluteTime, AbsoluteTime);
		const TGuardValue<float> DeltaTimeGuard(DeltaTime, DeltaTime);
		if(!InitializeVM(InEventName))
		{
			return false;
		}
		bJustRanInit = true;
	}

	// The EventQueueToRun should only be modified in URigVMHost::Evaluate_AnyThread
	// We create a temporary queue for the execution of only this event
	TArray<FName> LocalEventQueueToRun = EventQueueToRun;
	if(LocalEventQueueToRun.IsEmpty())
	{
		LocalEventQueueToRun = EventQueue;
	}

	const bool bIsEventInQueue = LocalEventQueueToRun.Contains(InEventName);
	const bool bIsEventFirstInQueue = !LocalEventQueueToRun.IsEmpty() && LocalEventQueueToRun[0] == InEventName; 
	const bool bIsEventLastInQueue = !LocalEventQueueToRun.IsEmpty() && LocalEventQueueToRun.Last() == InEventName;
	const bool bIsConstructionEvent = InEventName == FTLLRN_RigUnit_PrepareForExecution::EventName;
	const bool bIsPostConstructionEvent = InEventName == FTLLRN_RigUnit_PostPrepareForExecution::EventName;
	const bool bPostConstructionEventInQueue = LocalEventQueueToRun.Contains(FTLLRN_RigUnit_PostPrepareForExecution::EventName) && SupportsEvent(FTLLRN_RigUnit_PostPrepareForExecution::EventName);
	const bool bPreForwardSolveInQueue = LocalEventQueueToRun.Contains(FTLLRN_RigUnit_PreBeginExecution::EventName);
	const bool bPostForwardSolveInQueue = LocalEventQueueToRun.Contains(FTLLRN_RigUnit_PostBeginExecution::EventName);
	const bool bIsPreForwardSolve = InEventName == FTLLRN_RigUnit_PreBeginExecution::EventName;
	const bool bIsForwardSolve = InEventName == FTLLRN_RigUnit_BeginExecution::EventName;
	const bool bIsPostForwardSolve = InEventName == FTLLRN_RigUnit_PostBeginExecution::EventName;
	const bool bIsInteractionEvent = InEventName == FTLLRN_RigUnit_InteractionExecution::EventName;

	ensure(!HasAnyFlags(RF_ClassDefaultObject));
	
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()
	QUICK_SCOPE_CYCLE_COUNTER(STAT_TLLRN_ControlRig_Execute);
	
	FRigVMExtendedExecuteContext& ExtendedExecuteContext = GetRigVMExtendedExecuteContext();

	FTLLRN_ControlRigExecuteContext& PublicContext = ExtendedExecuteContext.GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();

#if WITH_EDITOR
	PublicContext.SetLog(RigVMLog); // may be nullptr
#endif

	PublicContext.SetDeltaTime(DeltaTime);
	PublicContext.SetAbsoluteTime(AbsoluteTime);
	PublicContext.SetFramesPerSecond(GetCurrentFramesPerSecond());
#if WITH_EDITOR
	PublicContext.SetHostBeingDebugged(bIsBeingDebugged);
#endif

#if UE_RIGVM_DEBUG_EXECUTION
	PublicContext.bDebugExecution = bDebugExecutionEnabled;
#endif

#if WITH_EDITOR
	ExtendedExecuteContext.SetInstructionVisitInfo(&InstructionVisitInfo);

	if (IsInDebugMode())
	{
		if (UTLLRN_ControlRig* CDO = GetClass()->GetDefaultObject<UTLLRN_ControlRig>())
		{
			// Copy the breakpoints. This will not override the state of the breakpoints
			DebugInfo.SetBreakpoints(CDO->DebugInfo.GetBreakpoints());

			// If there are any breakpoints, create the Snapshot VM if it hasn't been created yet
			if (DebugInfo.GetBreakpoints().Num() > 0)
			{
				GetSnapshotVM();
			}
		}

		ExtendedExecuteContext.SetDebugInfo(&DebugInfo);
	}
	else
	{
		ExtendedExecuteContext.SetDebugInfo(nullptr);
		GetSnapshotContext().Reset();
	}
	
	if (IsProfilingEnabled())
    {
    	ExtendedExecuteContext.SetProfilingInfo(&ProfilingInfo);
    }
    else
    {
    	ExtendedExecuteContext.SetProfilingInfo(nullptr);
    }
#endif

	FTLLRN_RigUnitContext& Context = PublicContext.UnitContext;

	bool bEnableDrawInterface = false;
#if WITH_EDITOR
	const bool bEnabledDuringGame = CVarTLLRN_ControlRigEnableDrawInterfaceInGame->GetInt() != 0;
	if (bEnabledDuringGame || !bIsRunningInPIE)
	{
		bEnableDrawInterface = true;
	}	
#endif

	// setup the draw interface for debug drawing
	if(!bIsEventInQueue || bIsEventFirstInQueue)
	{
		DrawInterface.Reset();
	}

	if (bEnableDrawInterface)
	{
		PublicContext.SetDrawInterface(&DrawInterface);
	}
	else
	{
		PublicContext.SetDrawInterface(nullptr);
	}

	// setup the animation attribute container
	Context.AnimAttributeContainer = ExternalAnimAttributeContainer;

	// draw container contains persistent draw instructions, 
	// so we cannot call Reset(), which will clear them,
	// instead, we re-initialize them from the CDO
	if (UTLLRN_ControlRig* CDO = GetClass()->GetDefaultObject<UTLLRN_ControlRig>())
	{
		DrawContainer = CDO->DrawContainer;
	}
	PublicContext.SetDrawContainer(&DrawContainer);

	// setup the data source registry
	Context.DataSourceRegistry = GetDataSourceRegistry();

	// setup the context with further fields
	Context.InteractionType = InteractionType;
	Context.ElementsBeingInteracted = ElementsBeingInteracted;
	PublicContext.Hierarchy = GetHierarchy();
	PublicContext.TLLRN_ControlRig = this;

	// allow access to the hierarchy
	Context.HierarchySettings = HierarchySettings;
	check(PublicContext.Hierarchy);

	// allow access to the shape libraries
	PublicContext.OnAddShapeLibraryDelegate.BindUObject(this, &UTLLRN_ControlRig::OnAddShapeLibrary);
	PublicContext.OnShapeExistsDelegate.BindUObject(this, &UTLLRN_ControlRig::OnShapeExists);

	// allow access to the default hierarchy to allow to reset
	if(!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(GetClass()->GetDefaultObject()))
		{
			if(UTLLRN_RigHierarchy* DefaultHierarchy = CDO->GetHierarchy())
			{
				PublicContext.Hierarchy->DefaultHierarchyPtr = DefaultHierarchy;
			}
		}
	}

	// disable any controller access outside of the construction event
	FTLLRN_RigHierarchyEnableControllerBracket DisableHierarchyController(PublicContext.Hierarchy, bIsConstructionEvent || bIsPostConstructionEvent);

	// given the outer scene component configure
	// the transform lookups to map transforms from rig space to world space
	if (USceneComponent* SceneComponent = GetOwningSceneComponent())
	{
		PublicContext.SetOwningComponent(SceneComponent);
	}
	else
	{
		PublicContext.SetOwningComponent(nullptr);
		
		if (ObjectBinding.IsValid())
		{
			AActor* HostingActor = ObjectBinding->GetHostingActor();
			if (HostingActor)
			{
				PublicContext.SetOwningActor(HostingActor);
			}
			else if (UObject* Owner = ObjectBinding->GetBoundObject())
			{
				PublicContext.SetWorld(Owner->GetWorld());
			}
		}

		if (PublicContext.GetWorld() == nullptr)
		{
			if (UObject* Outer = GetOuter())
			{
				PublicContext.SetWorld(Outer->GetWorld());
			}
		}
	}
	
	// setup the user data
	// first using the data from the rig,
	// followed by the data coming from the skeleton
	// then the data coming from the skeletal mesh,
	// and in the future we'll also add the data from the control rig component
	PublicContext.AssetUserData.Reset();
	if(const TArray<UAssetUserData*>* TLLRN_ControlRigUserDataArray = GetAssetUserDataArray())
	{
		for(const UAssetUserData* TLLRN_ControlRigUserData : *TLLRN_ControlRigUserDataArray)
		{
			PublicContext.AssetUserData.Add(TLLRN_ControlRigUserData);
		}
	}
	PublicContext.AssetUserData.Remove(nullptr);

	// if we have any referenced elements dirty them
	// also reset the recorded read / written transforms as needed
#if WITH_EDITOR
	TSharedPtr<TGuardValue<bool>> RecordTransformsPerInstructionGuard;
#endif

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if(Hierarchy)
	{
		Hierarchy->UpdateReferences(&PublicContext);

#if WITH_EDITOR
		bool bRecordTransformsAtRuntime = true;
		if(const UObject* Outer = GetOuter())
		{
			if(Outer->IsA<UTLLRN_ControlRigComponent>())
			{
				bRecordTransformsAtRuntime = false;
			}
		}
		RecordTransformsPerInstructionGuard = MakeShared<TGuardValue<bool>>(Hierarchy->bRecordTransformsAtRuntime, bRecordTransformsAtRuntime);
		ResetRecordedTransforms(InEventName);
#endif
	}

	// guard against recursion
	if(IsExecuting())
	{
		UE_LOG(LogTLLRN_ControlRig, Warning, TEXT("%s: Execute is being called recursively."), *GetPathName());
		return false;
	}
	if(bIsConstructionEvent || bIsPostConstructionEvent)
	{
		if(IsRunningPreConstruction() || IsRunningPostConstruction())
		{
			UE_LOG(LogTLLRN_ControlRig, Warning, TEXT("%s: Construction is being called recursively."), *GetPathName());
			return false;
		}
	}

	bool bSuccess = true;
	bool bPostConstructionEventWasRun = false;

	// we'll special case the construction event here
	if (bIsConstructionEvent)
	{
		check(Hierarchy);
		
		// remember the previous selection
		const TArray<FTLLRN_RigElementKey> PreviousSelection = Hierarchy->GetSelectedKeys();

		// construction mode means that we are running the construction event
		// constantly for testing purposes.
		const bool bConstructionModeEnabled = IsConstructionModeEnabled();
		{
			if (PreConstructionForUIEvent.IsBound())
			{
				FTLLRN_ControlRigBracketScope BracketScope(PreConstructionBracket);
				PreConstructionForUIEvent.Broadcast(this, FTLLRN_RigUnit_PrepareForExecution::EventName);
			}

#if WITH_EDITOR
			// apply the selection pose for the construction mode
			// we are doing this here to make sure the brackets below have the right data
			if(bConstructionModeEnabled)
			{
				ApplySelectionPoseForConstructionMode(InEventName);
			}
#endif

			// disable selection notifications from the hierarchy
			TGuardValue<bool> DisableSelectionNotifications(Hierarchy->GetController(true)->bSuspendSelectionNotifications, true);
			{
				FTLLRN_RigPose CurrentPose;
				// We might want to reset the input pose after construction
				if (bResetCurrentTransformsAfterConstruction)
				{
					CurrentPose = Hierarchy->GetPose(false, ETLLRN_RigElementType::ToResetAfterConstructionEvent, FTLLRN_RigElementKeyCollection());
				}
				
				{
					// Copy the hierarchy from the default object onto this one
#if WITH_EDITOR
					FTransientControlScope TransientControlScope(Hierarchy);
	#endif
					{
						// maintain the initial pose if it ever was set by the client
						FTLLRN_RigPose InitialPose;
						if(!bResetInitialTransformsBeforeConstruction)
						{
							InitialPose = Hierarchy->GetPose(true, ETLLRN_RigElementType::ToResetAfterConstructionEvent, FTLLRN_RigElementKeyCollection());
						}

						if(bCopyHierarchyBeforeConstruction)
						{
							Hierarchy->ResetToDefault();
						}

						if(InitialPose.Num() > 0)
						{
							const TGuardValue<bool> DisableRecordingCurveChanges(Hierarchy->GetRecordCurveChangesFlag(), false);
							Hierarchy->SetPose(InitialPose, ETLLRN_RigTransformType::InitialLocal);
						}
					}

					{
	#if WITH_EDITOR
						TUniquePtr<FTransientControlPoseScope> TransientControlPoseScope;
						if (bConstructionModeEnabled)
						{
							// save the transient control value, it should not be constantly reset in construction mode
							TransientControlPoseScope = MakeUnique<FTransientControlPoseScope>(this);
						}
	#endif
						// reset the pose to initial such that construction event can run from a deterministic initial state
						Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::All);
					}

					RestoreShapeLibrariesFromCDO();

					if (PreConstructionEvent.IsBound())
					{
						FTLLRN_ControlRigBracketScope BracketScope(PreConstructionBracket);
						PreConstructionEvent.Broadcast(this, FTLLRN_RigUnit_PrepareForExecution::EventName);
					}

					bSuccess = Execute_Internal(FTLLRN_RigUnit_PrepareForExecution::EventName);
					
				} // destroy FTransientControlScope

				if(!bPostConstructionEventInQueue && !bPostConstructionEventWasRun)
				{
					RunPostConstructionEvent();
					bPostConstructionEventWasRun = true;
				}

				// Reset the input pose after construction
				if (CurrentPose.Num() > 0)
				{
					const TGuardValue<bool> DisableRecordingCurveChanges(Hierarchy->GetRecordCurveChangesFlag(), false);
					Hierarchy->SetPose(CurrentPose, ETLLRN_RigTransformType::CurrentLocal);
				}
			}
			
			// set it here to reestablish the selection. the notifications
			// will be eaten since we still have the bSuspend flag on in the controller.
			Hierarchy->GetController()->SetSelection(PreviousSelection);
			
		} // destroy DisableSelectionNotifications

		if ((bIsPostConstructionEvent || (bIsConstructionEvent && !bPostConstructionEventInQueue)) && !bPostConstructionEventWasRun)
		{
			RunPostConstructionEvent();
			bPostConstructionEventWasRun = true;
		}

		if (bConstructionModeEnabled)
		{
#if WITH_EDITOR
			TUniquePtr<FTransientControlPoseScope> TransientControlPoseScope;
			if (bConstructionModeEnabled)
			{
				// save the transient control value, it should not be constantly reset in construction mode
				TransientControlPoseScope = MakeUnique<FTransientControlPoseScope>(this);
			}
#endif
			Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
		}

		// synchronize the selection now with the new hierarchy after running construction
		const TArray<const FTLLRN_RigBaseElement*> CurrentSelection = Hierarchy->GetSelectedElements();
		for(const FTLLRN_RigBaseElement* SelectedElement : CurrentSelection)
		{
			if(!PreviousSelection.Contains(SelectedElement->GetKey()))
			{
				Hierarchy->Notify(ETLLRN_RigHierarchyNotification::ElementSelected, SelectedElement);
			}
		}
		for(const FTLLRN_RigElementKey& PreviouslySelectedKey : PreviousSelection)
		{
			if(const FTLLRN_RigBaseElement* PreviouslySelectedElement = Hierarchy->Find(PreviouslySelectedKey))
			{
				if(!CurrentSelection.Contains(PreviouslySelectedElement))
				{
					Hierarchy->Notify(ETLLRN_RigHierarchyNotification::ElementDeselected, PreviouslySelectedElement);
				}
			}
		}
	}
	else
	{
#if WITH_EDITOR
		// only set a valid first entry event when none has been set
		if (InstructionVisitInfo.GetFirstEntryEventInEventQueue() == NAME_None && !LocalEventQueueToRun.IsEmpty() && VM)
		{
			InstructionVisitInfo.SetFirstEntryEventInEventQueue(LocalEventQueueToRun[0]);
		}

		// Transform Overrride is generated using a Transient Control 
		ApplyTransformOverrideForUserCreatedBones();

		if (bEnableAnimAttributeTrace && ExternalAnimAttributeContainer != nullptr)
		{
			InputAnimAttributeSnapshot.CopyFrom(*ExternalAnimAttributeContainer);
		}
#endif
		
		if (bIsPreForwardSolve || (bIsForwardSolve && !bPreForwardSolveInQueue))
		{
			if (PreForwardsSolveEvent.IsBound())
			{
				FTLLRN_ControlRigBracketScope BracketScope(PreForwardsSolveBracket);
				PreForwardsSolveEvent.Broadcast(this, FTLLRN_RigUnit_BeginExecution::EventName);
			}

		}

		bSuccess = Execute_Internal(InEventName);

#if WITH_EDITOR
		if (bEnableAnimAttributeTrace && ExternalAnimAttributeContainer != nullptr)
		{
			OutputAnimAttributeSnapshot.CopyFrom(*ExternalAnimAttributeContainer);
		}
#endif

		if (bIsPostForwardSolve || (bIsForwardSolve && !bPostForwardSolveInQueue))
		{
			if (PostForwardsSolveEvent.IsBound())
			{
				FTLLRN_ControlRigBracketScope BracketScope(PostForwardsSolveBracket);
				PostForwardsSolveEvent.Broadcast(this, FTLLRN_RigUnit_BeginExecution::EventName);
			}
		}
	}

#if WITH_EDITOR

	// for the last event in the queue - clear the log message queue
	if (RigVMLog != nullptr && bEnableLogging)
	{
		if (bJustRanInit)
		{
			RigVMLog->KnownMessages.Reset();
			LoggedMessages.Reset();
		}
		else if(bIsEventLastInQueue)
		{
			for (const FRigVMLog::FLogEntry& Entry : RigVMLog->Entries)
			{
				if (Entry.FunctionName == NAME_None || Entry.InstructionIndex == INDEX_NONE || Entry.Message.IsEmpty())
				{
					continue;
				}

				FString PerInstructionMessage = 
					FString::Printf(
						TEXT("Instruction[%d] '%s': '%s'"),
						Entry.InstructionIndex,
						*Entry.FunctionName.ToString(),
						*Entry.Message
					);

				LogOnce(Entry.Severity, Entry.InstructionIndex, PerInstructionMessage);
			}
		}
	}
#endif

	if(!bIsEventInQueue || bIsEventLastInQueue) 
	{
		DeltaTime = 0.f;
	}

	if (ExecutedEvent.IsBound())
	{
		if (!IsAdditive() || InEventName != FTLLRN_RigUnit_InverseExecution::EventName)
		{
			FTLLRN_ControlRigBracketScope BracketScope(ExecuteBracket);
			ExecutedEvent.Broadcast(this, InEventName);
		}
	}

	// close remaining undo brackets from hierarchy
	while(ControlUndoBracketIndex > 0)
	{
		FTLLRN_RigEventContext EventContext;
		EventContext.Event = ETLLRN_RigEvent::CloseUndoBracket;
		EventContext.SourceEventName = InEventName;
		EventContext.LocalTime = PublicContext.GetAbsoluteTime();
		HandleHierarchyEvent(Hierarchy, EventContext);
	}

	if (PublicContext.GetDrawInterface() && PublicContext.GetDrawContainer() && bIsEventLastInQueue) 
	{
		PublicContext.GetDrawInterface()->Instructions.Append(PublicContext.GetDrawContainer()->Instructions);

		FTLLRN_RigHierarchyValidityBracket ValidityBracket(Hierarchy);
		
		Hierarchy->ForEach<FTLLRN_RigControlElement>([this, Hierarchy](FTLLRN_RigControlElement* ControlElement) -> bool
		{
			const FTLLRN_RigControlSettings& Settings = ControlElement->Settings;

			if (Settings.IsVisible() &&
				!Settings.bIsTransientControl &&
				Settings.bDrawLimits &&
				Settings.LimitEnabled.Contains(FTLLRN_RigControlLimitEnabled(true, true)))
			{
				FTransform Transform = Hierarchy->GetGlobalControlOffsetTransformByIndex(ControlElement->GetIndex());
				FRigVMDrawInstruction Instruction(ERigVMDrawSettings::Lines, Settings.ShapeColor, 0.f, Transform);

				switch (Settings.ControlType)
				{
					case ETLLRN_RigControlType::Float:
					case ETLLRN_RigControlType::ScaleFloat:
					{
						if(Settings.LimitEnabled[0].IsOff())
						{
							break;
						}

						FVector MinPos = FVector::ZeroVector;
						FVector MaxPos = FVector::ZeroVector;

						switch (Settings.PrimaryAxis)
						{
							case ETLLRN_RigControlAxis::X:
							{
								MinPos.X = Settings.MinimumValue.Get<float>();
								MaxPos.X = Settings.MaximumValue.Get<float>();
								break;
							}
							case ETLLRN_RigControlAxis::Y:
							{
								MinPos.Y = Settings.MinimumValue.Get<float>();
								MaxPos.Y = Settings.MaximumValue.Get<float>();
								break;
							}
							case ETLLRN_RigControlAxis::Z:
							{
								MinPos.Z = Settings.MinimumValue.Get<float>();
								MaxPos.Z = Settings.MaximumValue.Get<float>();
								break;
							}
						}

						Instruction.Positions.Add(MinPos);
						Instruction.Positions.Add(MaxPos);
						break;
					}
					case ETLLRN_RigControlType::Integer:
					{
						if(Settings.LimitEnabled[0].IsOff())
						{
							break;
						}

						FVector MinPos = FVector::ZeroVector;
						FVector MaxPos = FVector::ZeroVector;

						switch (Settings.PrimaryAxis)
						{
							case ETLLRN_RigControlAxis::X:
							{
								MinPos.X = (float)Settings.MinimumValue.Get<int32>();
								MaxPos.X = (float)Settings.MaximumValue.Get<int32>();
								break;
							}
							case ETLLRN_RigControlAxis::Y:
							{
								MinPos.Y = (float)Settings.MinimumValue.Get<int32>();
								MaxPos.Y = (float)Settings.MaximumValue.Get<int32>();
								break;
							}
							case ETLLRN_RigControlAxis::Z:
							{
								MinPos.Z = (float)Settings.MinimumValue.Get<int32>();
								MaxPos.Z = (float)Settings.MaximumValue.Get<int32>();
								break;
							}
						}

						Instruction.Positions.Add(MinPos);
						Instruction.Positions.Add(MaxPos);
						break;
					}
					case ETLLRN_RigControlType::Vector2D:
					{
						if(Settings.LimitEnabled.Num() < 2)
						{
							break;
						}
						if(Settings.LimitEnabled[0].IsOff() && Settings.LimitEnabled[1].IsOff())
						{
							break;
						}

						Instruction.PrimitiveType = ERigVMDrawSettings::LineStrip;
						FVector3f MinPos = Settings.MinimumValue.Get<FVector3f>();
						FVector3f MaxPos = Settings.MaximumValue.Get<FVector3f>();

						switch (Settings.PrimaryAxis)
						{
							case ETLLRN_RigControlAxis::X:
							{
								Instruction.Positions.Add(FVector(0.f, MinPos.X, MinPos.Y));
								Instruction.Positions.Add(FVector(0.f, MaxPos.X, MinPos.Y));
								Instruction.Positions.Add(FVector(0.f, MaxPos.X, MaxPos.Y));
								Instruction.Positions.Add(FVector(0.f, MinPos.X, MaxPos.Y));
								Instruction.Positions.Add(FVector(0.f, MinPos.X, MinPos.Y));
								break;
							}
							case ETLLRN_RigControlAxis::Y:
							{
								Instruction.Positions.Add(FVector(MinPos.X, 0.f, MinPos.Y));
								Instruction.Positions.Add(FVector(MaxPos.X, 0.f, MinPos.Y));
								Instruction.Positions.Add(FVector(MaxPos.X, 0.f, MaxPos.Y));
								Instruction.Positions.Add(FVector(MinPos.X, 0.f, MaxPos.Y));
								Instruction.Positions.Add(FVector(MinPos.X, 0.f, MinPos.Y));
								break;
							}
							case ETLLRN_RigControlAxis::Z:
							{
								Instruction.Positions.Add(FVector(MinPos.X, MinPos.Y, 0.f));
								Instruction.Positions.Add(FVector(MaxPos.X, MinPos.Y, 0.f));
								Instruction.Positions.Add(FVector(MaxPos.X, MaxPos.Y, 0.f));
								Instruction.Positions.Add(FVector(MinPos.X, MaxPos.Y, 0.f));
								Instruction.Positions.Add(FVector(MinPos.X, MinPos.Y, 0.f));
								break;
							}
						}
						break;
					}
					case ETLLRN_RigControlType::Position:
					case ETLLRN_RigControlType::Scale:
					case ETLLRN_RigControlType::Transform:
					case ETLLRN_RigControlType::TransformNoScale:
					case ETLLRN_RigControlType::EulerTransform:
					{
						FVector3f MinPos = FVector3f::ZeroVector;
						FVector3f MaxPos = FVector3f::ZeroVector;

						// we only check the first three here
						// since we only consider translation anyway
						// for scale it's also the first three
						if(Settings.LimitEnabled.Num() < 3)
						{
							break;
						}
						if(!Settings.LimitEnabled[0].IsOn() && !Settings.LimitEnabled[1].IsOn() && !Settings.LimitEnabled[2].IsOn())
						{
							break;
						}

						switch (Settings.ControlType)
						{
							case ETLLRN_RigControlType::Position:
							case ETLLRN_RigControlType::Scale:
							{
								MinPos = Settings.MinimumValue.Get<FVector3f>();
								MaxPos = Settings.MaximumValue.Get<FVector3f>();
								break;
							}
							case ETLLRN_RigControlType::Transform:
							{
								MinPos = Settings.MinimumValue.Get<FTLLRN_RigControlValue::FTransform_Float>().GetTranslation();
								MaxPos = Settings.MaximumValue.Get<FTLLRN_RigControlValue::FTransform_Float>().GetTranslation();
								break;
							}
							case ETLLRN_RigControlType::TransformNoScale:
							{
								MinPos = Settings.MinimumValue.Get<FTLLRN_RigControlValue::FTransformNoScale_Float>().GetTranslation();
								MaxPos = Settings.MaximumValue.Get<FTLLRN_RigControlValue::FTransformNoScale_Float>().GetTranslation();
								break;
							}
							case ETLLRN_RigControlType::EulerTransform:
							{
								MinPos = Settings.MinimumValue.Get<FTLLRN_RigControlValue::FEulerTransform_Float>().GetTranslation();
								MaxPos = Settings.MaximumValue.Get<FTLLRN_RigControlValue::FEulerTransform_Float>().GetTranslation();
								break;
							}
						}

						Instruction.Positions.Add(FVector(MinPos.X, MinPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MinPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MinPos.X, MaxPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MaxPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MinPos.X, MinPos.Y, MaxPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MinPos.Y, MaxPos.Z));
						Instruction.Positions.Add(FVector(MinPos.X, MaxPos.Y, MaxPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MaxPos.Y, MaxPos.Z));

						Instruction.Positions.Add(FVector(MinPos.X, MinPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MinPos.X, MaxPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MinPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MaxPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MinPos.X, MinPos.Y, MaxPos.Z));
						Instruction.Positions.Add(FVector(MinPos.X, MaxPos.Y, MaxPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MinPos.Y, MaxPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MaxPos.Y, MaxPos.Z));

						Instruction.Positions.Add(FVector(MinPos.X, MinPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MinPos.X, MinPos.Y, MaxPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MinPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MinPos.Y, MaxPos.Z));
						Instruction.Positions.Add(FVector(MinPos.X, MaxPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MinPos.X, MaxPos.Y, MaxPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MaxPos.Y, MinPos.Z));
						Instruction.Positions.Add(FVector(MaxPos.X, MaxPos.Y, MaxPos.Z));
						break;
					}
				}

				if (Instruction.Positions.Num() > 0)
				{
					DrawInterface.DrawInstruction(Instruction);
				}
			}

			return true;
		});
	}

	if(bIsInteractionEvent)
	{
		bInteractionJustBegan = false;
	}

	if(bIsConstructionEvent)
	{
		RemoveRunOnceEvent(FTLLRN_RigUnit_PrepareForExecution::EventName);
	}
	if(bIsPostConstructionEvent)
	{
		RemoveRunOnceEvent(FTLLRN_RigUnit_PostPrepareForExecution::EventName);
	}

	return bSuccess;
}

bool UTLLRN_ControlRig::Execute_Internal(const FName& InEventName)
{
	if(!SupportsEvent(InEventName))
	{
		return false;
	}

	// The EventQueueToRun should only be modified in URigVMHost::Evaluate_AnyThread
	// We create a temporary queue for the execution of only this event
	TArray<FName> LocalEventQueueToRun = EventQueueToRun;
	if(LocalEventQueueToRun.IsEmpty())
	{
		LocalEventQueueToRun = EventQueue;
	}

	// make sure to initialize here as well - just in case this gets
	// called without a call to ::Execute. This is already tackled by
	// the UTLLRN_ModularRig::ExecuteQueue, but added it here as well nevertheless
	// to avoid crashes in the future.
	if(bRequiresInitExecution)
	{
		const TGuardValue<float> AbsoluteTimeGuard(AbsoluteTime, AbsoluteTime);
		const TGuardValue<float> DeltaTimeGuard(DeltaTime, DeltaTime);
		if(!InitializeVM(InEventName))
		{
			return false;
		}
	}
	
	if(IsTLLRN_RigModule() && InEventName != FTLLRN_RigUnit_ConnectorExecution::EventName)
	{
		FString ConnectorWarning;
		if(!AllConnectorsAreResolved(&ConnectorWarning))
		{
#if WITH_EDITOR
			LogOnce(EMessageSeverity::Warning, INDEX_NONE, ConnectorWarning);
#endif
			return false;
		}
	}
	
	if (VM)
	{
		FRigVMExtendedExecuteContext& Context = GetRigVMExtendedExecuteContext();

		static constexpr TCHAR InvalidatedVMFormat[] = TEXT("%s: Invalidated VM - aborting execution.");
		if(VM->IsNativized())
		{
			if(!IsValidLowLevel() ||
				!VM->IsValidLowLevel())
			{
				UE_LOG(LogTLLRN_ControlRig, Warning, InvalidatedVMFormat, *GetClass()->GetName());
				return false;
			}
		}
		else
		{
			// sanity check the validity of the VM to ensure stability.
			if(!VM->IsContextValidForExecution(Context)
				|| !IsValidLowLevel()
				|| !VM->IsValidLowLevel()
			)
			{
				UE_LOG(LogTLLRN_ControlRig, Warning, InvalidatedVMFormat, *GetClass()->GetName());
				return false;
			}
		}
		
#if UE_RIGVM_PROFILE_EXECUTE_UNITS_NUM
		const uint64 StartCycles = FPlatformTime::Cycles64();
		if(ProfilingRunsLeft <= 0)
		{
			ProfilingRunsLeft = UE_RIGVM_PROFILE_EXECUTE_UNITS_NUM;
			AccumulatedCycles = 0;
		}
#endif
		
		const bool bUseDebuggingSnapshots = !VM->IsNativized();
		
#if WITH_EDITOR
		if(bUseDebuggingSnapshots)
		{
			if(URigVM* SnapShotVM = GetSnapshotVM(false)) // don't create it for normal runs
			{
				const bool bIsEventFirstInQueue = !LocalEventQueueToRun.IsEmpty() && LocalEventQueueToRun[0] == InEventName; 
				const bool bIsEventLastInQueue = !LocalEventQueueToRun.IsEmpty() && LocalEventQueueToRun.Last() == InEventName;

				if (GetHaltedAtBreakpoint().IsValid())
				{
					if(bIsEventFirstInQueue)
					{
						CopyVMMemory(GetRigVMExtendedExecuteContext(), GetSnapshotContext());
					}
				}
				else if(bIsEventLastInQueue)
				{
					CopyVMMemory(GetSnapshotContext(), GetRigVMExtendedExecuteContext());
				}
			}
		}
#endif

		UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
		FTLLRN_RigHierarchyExecuteContextBracket HierarchyContextGuard(Hierarchy, &Context);

		// setup the module information
		FTLLRN_ControlRigExecuteContext& PublicContext = Context.GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
		FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard TLLRN_RigModuleGuard(PublicContext, this);
		FTLLRN_RigHierarchyRedirectorGuard ElementRedirectorGuard(this);

		const bool bSuccess = VM->ExecuteVM(Context, InEventName) != ERigVMExecuteResult::Failed;

#if UE_RIGVM_PROFILE_EXECUTE_UNITS_NUM
		const uint64 EndCycles = FPlatformTime::Cycles64();
		const uint64 Cycles = EndCycles - StartCycles;
		AccumulatedCycles += Cycles;
		ProfilingRunsLeft--;
		if(ProfilingRunsLeft == 0)
		{
			const double Milliseconds = FPlatformTime::ToMilliseconds64(AccumulatedCycles);
			UE_LOG(LogTLLRN_ControlRig, Display, TEXT("%s: %d runs took %.03lfms."), *GetClass()->GetName(), UE_RIGVM_PROFILE_EXECUTE_UNITS_NUM, Milliseconds);
		}
#endif

		return bSuccess;
	}
	return false;
}

void UTLLRN_ControlRig::RequestInit()
{
	RequestInitVMs();
	RequestConstruction();
}

void UTLLRN_ControlRig::RequestConstruction()
{
	RequestRunOnceEvent(FTLLRN_RigUnit_PrepareForExecution::EventName, 0);
	RequestRunOnceEvent(FTLLRN_RigUnit_PostPrepareForExecution::EventName, 1);
}

bool UTLLRN_ControlRig::IsConstructionRequired() const
{
	return IsRunOnceEvent(FTLLRN_RigUnit_PrepareForExecution::EventName);
}

bool UTLLRN_ControlRig::SupportsBackwardsSolve() const
{
	return SupportsEvent(FTLLRN_RigUnit_InverseExecution::EventName);
}

void UTLLRN_ControlRig::AdaptEventQueueForEvaluate(TArray<FName>& InOutEventQueueToRun)
{
	Super::AdaptEventQueueForEvaluate(InOutEventQueueToRun);

	for (int32 i=0; i<InOutEventQueueToRun.Num(); ++i)
	{
		if (InOutEventQueueToRun[i] == FTLLRN_RigUnit_PrepareForExecution::EventName)
		{
			if (SupportsEvent(FTLLRN_RigUnit_PostPrepareForExecution::EventName))
			{
				i++; // skip construction
				InOutEventQueueToRun.Insert(FTLLRN_RigUnit_PostPrepareForExecution::EventName, i);
			}
		}
		if (InOutEventQueueToRun[i] == FTLLRN_RigUnit_BeginExecution::EventName)
		{
			if (SupportsEvent(FTLLRN_RigUnit_PreBeginExecution::EventName))
			{
				InOutEventQueueToRun.Insert(FTLLRN_RigUnit_PreBeginExecution::EventName, i);
				i++; // skip preforward 
			}
			if (SupportsEvent(FTLLRN_RigUnit_PostBeginExecution::EventName))
			{
				i++; // skip forward
				InOutEventQueueToRun.Insert(FTLLRN_RigUnit_PostBeginExecution::EventName, i);
			}
		}
	}

	if(InteractionType != (uint8)ETLLRN_ControlRigInteractionType::None)
	{
		static const FName& InteractionEvent = FTLLRN_RigUnit_InteractionExecution::EventName;
		if(SupportsEvent(InteractionEvent))
		{
			if(InOutEventQueueToRun.IsEmpty())
			{
				InOutEventQueueToRun.Add(InteractionEvent);
			}
			else if(!InOutEventQueueToRun.Contains(FTLLRN_RigUnit_PrepareForExecution::EventName))
			{
				// insert just before the last event so the interaction runs prior to
				// forward solve or backwards solve.
				InOutEventQueueToRun.Insert(InteractionEvent, InOutEventQueueToRun.Num() - 1);
			}
		}
	}
}

void UTLLRN_ControlRig::GetMappableNodeData(TArray<FName>& OutNames, TArray<FNodeItem>& OutNodeItems) const
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	OutNames.Reset();
	OutNodeItems.Reset();

	check(DynamicHierarchy);

	// now add all nodes
	DynamicHierarchy->ForEach<FTLLRN_RigBoneElement>([&OutNames, &OutNodeItems, this](FTLLRN_RigBoneElement* BoneElement) -> bool
    {
		OutNames.Add(BoneElement->GetFName());
		FTLLRN_RigElementKey ParentKey = DynamicHierarchy->GetFirstParent(BoneElement->GetKey());
		if(ParentKey.Type != ETLLRN_RigElementType::Bone)
		{
			ParentKey.Name = NAME_None;
		}

		const FTransform GlobalInitial = DynamicHierarchy->GetGlobalTransformByIndex(BoneElement->GetIndex(), true);
		OutNodeItems.Add(FNodeItem(ParentKey.Name, GlobalInitial));
		return true;
	});
}

UAnimationDataSourceRegistry* UTLLRN_ControlRig::GetDataSourceRegistry()
{
	if (DataSourceRegistry)
	{
		if (DataSourceRegistry->GetOuter() != this)
		{
			DataSourceRegistry = nullptr;
		}
	}
	if (DataSourceRegistry == nullptr)
	{
		DataSourceRegistry = NewObject<UAnimationDataSourceRegistry>(this, NAME_None, RF_Transient);

		if (!IsInGameThread())
		{
			// If the object was created on a non-game thread, clear the async flag immediately, so that it can be
			// garbage collected in the future. 
			(void)DataSourceRegistry->AtomicallyClearInternalFlags(EInternalObjectFlags::Async);
		}

		if (HasAnyFlags(RF_ClassDefaultObject) && GetClass()->IsNative())
		{
			DataSourceRegistry->AddToRoot();
		}
	}

	return DataSourceRegistry;
}

#if WITH_EDITORONLY_DATA

void UTLLRN_ControlRig::PostReinstanceCallback(const UTLLRN_ControlRig* Old)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	ObjectBinding = Old->ObjectBinding;
	Initialize();
}

#endif // WITH_EDITORONLY_DATA

TArray<UTLLRN_ControlRig*> UTLLRN_ControlRig::FindTLLRN_ControlRigs(UObject* Outer, TSubclassOf<UTLLRN_ControlRig> OptionalClass)
{
	TArray<UTLLRN_ControlRig*> Result;
	
	if(Outer == nullptr)
	{
		return Result; 
	}
	
	AActor* OuterActor = Cast<AActor>(Outer);
	if(OuterActor == nullptr)
	{
		OuterActor = Outer->GetTypedOuter<AActor>();
	}
	
	for (TObjectIterator<UTLLRN_ControlRig> Itr; Itr; ++Itr)
	{
		UTLLRN_ControlRig* RigInstance = *Itr;
		if (OptionalClass == nullptr || RigInstance->GetClass()->IsChildOf(OptionalClass))
		{
			if(RigInstance->IsInOuter(Outer))
			{
				Result.Add(RigInstance);
				continue;
			}

			if(OuterActor)
			{
				if(RigInstance->IsInOuter(OuterActor))
				{
					Result.Add(RigInstance);
					continue;
				}

				if (TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = RigInstance->GetObjectBinding())
				{
					if (AActor* Actor = Binding->GetHostingActor())
					{
						if (Actor == OuterActor)
						{
							Result.Add(RigInstance);
							continue;
						}
					}
				}
			}
		}
	}

	return Result;
}

void UTLLRN_ControlRig::Serialize(FArchive& Ar)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::TLLRN_ControlRigStoresPhysicsSolvers)
	{
		Ar << PhysicsSolvers;
	}
	else
	{
		PhysicsSolvers.Reset();
	}
}

void UTLLRN_ControlRig::PostLoad()
{
	Super::PostLoad();

	if(HasAnyFlags(RF_ClassDefaultObject))
	{
		if(DynamicHierarchy)
		{
			// Some dynamic hierarchy objects have been created using NewObject<> instead of CreateDefaultSubObjects.
			// Assets from that version require the dynamic hierarchy to be flagged as below.
			DynamicHierarchy->SetFlags(DynamicHierarchy->GetFlags() | RF_Public | RF_DefaultSubObject);
		}
	}

	FTLLRN_RigInfluenceMapPerEvent NewInfluences;
	for(int32 MapIndex = 0; MapIndex < Influences.Num(); MapIndex++)
	{
		FTLLRN_RigInfluenceMap Map = Influences[MapIndex];
		FName EventName = Map.GetEventName();

		if(EventName == TEXT("Update"))
		{
			EventName = FTLLRN_RigUnit_BeginExecution::EventName;
		}
		else if(EventName == TEXT("Inverse"))
		{
			EventName = FTLLRN_RigUnit_InverseExecution::EventName;
		}
		
		NewInfluences.FindOrAdd(EventName).Merge(Map, true);
	}
	Influences = NewInfluences;

	if(DynamicHierarchy)
	{
		DynamicHierarchy->bUsePreferredEulerAngles = !bIsAdditive;
	}
}

const FTLLRN_RigModuleSettings& UTLLRN_ControlRig::GetTLLRN_RigModuleSettings() const
{
	if(HasAnyFlags(RF_ClassDefaultObject))
	{
		return TLLRN_RigModuleSettings;
	}
	if (const UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(GetClass()->GetDefaultObject()))
	{
		return CDO->GetTLLRN_RigModuleSettings();
	}
	return TLLRN_RigModuleSettings;
}

bool UTLLRN_ControlRig::IsTLLRN_RigModule() const
{
	return GetTLLRN_RigModuleSettings().IsValidModule(false);
}

bool UTLLRN_ControlRig::IsTLLRN_RigModuleInstance() const
{
	if(IsTLLRN_RigModule())
	{
		return GetParentRig() != nullptr;
	}
	return false;
}

bool UTLLRN_ControlRig::IsTLLRN_ModularRig() const
{
	if(const UClass* Class = GetClass())
	{
		return Class->IsChildOf(UTLLRN_ModularRig::StaticClass());
	}
	return false;
}

bool UTLLRN_ControlRig::IsStandaloneRig() const
{
	return !IsTLLRN_ModularRig();
}

bool UTLLRN_ControlRig::IsNativeRig() const
{
	if(const UClass* Class = GetClass())
	{
		return Class->IsNative();
	}
	return false;
}

UTLLRN_ControlRig* UTLLRN_ControlRig::GetParentRig() const
{
	return GetTypedOuter<UTLLRN_ControlRig>();
}

const FString& UTLLRN_ControlRig::GetTLLRN_RigModuleNameSpace() const
{
	if(IsTLLRN_RigModule())
	{
		if(const UTLLRN_ControlRig* ParentRig = GetParentRig())
		{
			const FString& ParentNameSpace = ParentRig->GetTLLRN_RigModuleNameSpace();
			static constexpr TCHAR JoinFormat[] = TEXT("%s%s:");
			TLLRN_RigModuleNameSpace = FString::Printf(JoinFormat, *ParentNameSpace, *GetFName().ToString());
			return TLLRN_RigModuleNameSpace;
		}

		// this means we are not an instance - so we are authoring the rig module right now.
		// for visualization purposes we'll use the name of the rig module chosen in the settings.
		static constexpr TCHAR NameSpaceFormat[] = TEXT("%s:"); 
		TLLRN_RigModuleNameSpace = FString::Printf(NameSpaceFormat, *GetTLLRN_RigModuleSettings().Identifier.Name);
		return TLLRN_RigModuleNameSpace;
	}

	static const FString EmptyNameSpace;
	return EmptyNameSpace;
}

FTLLRN_RigElementKeyRedirector& UTLLRN_ControlRig::GetElementKeyRedirector()
{
	// if we are an instance on a modular rig, use our local info
	if(IsTLLRN_RigModuleInstance())
	{
		return ElementKeyRedirector;
	}

	// if we are a rig module ( but not an instance on a rig)
	if(IsTLLRN_RigModule())
	{
		return ElementKeyRedirector;
	}

	static FTLLRN_RigElementKeyRedirector EmptyRedirector = FTLLRN_RigElementKeyRedirector();
	return EmptyRedirector;
}

void UTLLRN_ControlRig::SetElementKeyRedirector(const FTLLRN_RigElementKeyRedirector InElementRedirector)
{
	ElementKeyRedirector = InElementRedirector;
}

TArray<FTLLRN_RigControlElement*> UTLLRN_ControlRig::AvailableControls() const
{
	if(DynamicHierarchy)
	{
		return DynamicHierarchy->GetElementsOfType<FTLLRN_RigControlElement>();
	}
	return TArray<FTLLRN_RigControlElement*>();
}

FTLLRN_RigControlElement* UTLLRN_ControlRig::FindControl(const FName& InControlName) const
{
	if(DynamicHierarchy == nullptr)
	{
		return nullptr;
	}
	return DynamicHierarchy->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(InControlName, ETLLRN_RigElementType::Control));
}

bool UTLLRN_ControlRig::IsConstructionModeEnabled() const
{
	return EventQueueToRun.Num() == 1 && EventQueueToRun.Contains(FTLLRN_RigUnit_PrepareForExecution::EventName);
}

FTransform UTLLRN_ControlRig::SetupControlFromGlobalTransform(const FName& InControlName, const FTransform& InGlobalTransform)
{
	if (IsConstructionModeEnabled())
	{
		FTLLRN_RigControlElement* ControlElement = FindControl(InControlName);
		if (ControlElement && !ControlElement->Settings.bIsTransientControl)
		{
			const FTransform ParentTransform = GetHierarchy()->GetParentTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal);
			const FTransform OffsetTransform = InGlobalTransform.GetRelativeTransform(ParentTransform);
			GetHierarchy()->SetControlOffsetTransform(ControlElement, OffsetTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
			GetHierarchy()->SetControlOffsetTransform(ControlElement, OffsetTransform, ETLLRN_RigTransformType::CurrentLocal, true, false);

			if(UTLLRN_RigHierarchy* DefaultHierarchy = GetHierarchy()->GetDefaultHierarchy())
			{
				if(FTLLRN_RigControlElement* DefaultControlElement = DefaultHierarchy->Find<FTLLRN_RigControlElement>(ControlElement->GetKey()))
				{
					DefaultHierarchy->SetControlOffsetTransform(DefaultControlElement, OffsetTransform, ETLLRN_RigTransformType::InitialLocal, true, true);
					DefaultHierarchy->SetControlOffsetTransform(DefaultControlElement, OffsetTransform, ETLLRN_RigTransformType::CurrentLocal, true, true);
				}
			}
		}
	}
	return InGlobalTransform;
}

void UTLLRN_ControlRig::CreateTLLRN_RigControlsForCurveContainer()
{
	const bool bCreateFloatControls = CVarTLLRN_ControlRigCreateFloatControlsForCurves->GetInt() == 0 ? false : true;
	if(bCreateFloatControls && DynamicHierarchy)
	{
		UTLLRN_RigHierarchyController* Controller = DynamicHierarchy->GetController(true);
		if(Controller == nullptr)
		{
			return;
		}
		static const FString CtrlPrefix(TEXT("CTRL_"));

		DynamicHierarchy->ForEach<FTLLRN_RigCurveElement>([this, Controller](FTLLRN_RigCurveElement* CurveElement) -> bool
        {
			const FString Name = CurveElement->GetFName().ToString();
			
			if (Name.Contains(CtrlPrefix) && !DynamicHierarchy->Contains(FTLLRN_RigElementKey(*Name, ETLLRN_RigElementType::Curve))) //-V1051
			{
				FTLLRN_RigControlSettings Settings;
				Settings.AnimationType = ETLLRN_RigControlAnimationType::AnimationChannel;
				Settings.ControlType = ETLLRN_RigControlType::Float;
				Settings.bIsCurve = true;
				Settings.bDrawLimits = false;

				FTLLRN_RigControlValue Value;
				Value.Set<float>(CurveElement->Get());

				Controller->AddControl(CurveElement->GetFName(), FTLLRN_RigElementKey(), Settings, Value, FTransform::Identity, FTransform::Identity); 
			}

			return true;
		});

		ControlModified().AddUObject(this, &UTLLRN_ControlRig::HandleOnControlModified);
	}
}

void UTLLRN_ControlRig::HandleOnControlModified(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* Control, const FTLLRN_RigControlModifiedContext& Context)
{
	if (Control->Settings.bIsCurve && DynamicHierarchy)
	{
		const FTLLRN_RigControlValue Value = DynamicHierarchy->GetControlValue(Control, IsConstructionModeEnabled() ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
		DynamicHierarchy->SetCurveValue(FTLLRN_RigElementKey(Control->GetFName(), ETLLRN_RigElementType::Curve), Value.Get<float>());
	}	
}

bool UTLLRN_ControlRig::IsCurveControl(const FTLLRN_RigControlElement* InControlElement) const
{
	return InControlElement->Settings.bIsCurve;
}

FTransform UTLLRN_ControlRig::GetControlGlobalTransform(const FName& InControlName) const
{
	if(DynamicHierarchy == nullptr)
	{
		return FTransform::Identity;
	}
	return DynamicHierarchy->GetGlobalTransform(FTLLRN_RigElementKey(InControlName, ETLLRN_RigElementType::Control), false);
}

FTLLRN_RigControlValue UTLLRN_ControlRig::GetControlValue(FTLLRN_RigControlElement* InControl, const ETLLRN_RigControlValueType& InValueType) const
{
	if (bIsAdditive && InValueType == ETLLRN_RigControlValueType::Current)
	{
		const int32 ControlIndex = ControlsAfterBackwardsSolve.GetIndex(InControl->GetKey());
		if (ControlIndex != INDEX_NONE)
		{
			// Booleans/Enums are not additive properties, just return the current value
			if (!InControl->CanTreatAsAdditive())
			{
				return GetHierarchy()->GetControlValue(InControl, InValueType);
			}
			
			// return local space control value (the one to be added after backwards solve)
			const FTLLRN_RigPoseElement& AnimPose = ControlsAfterBackwardsSolve[ControlIndex];
			const FTLLRN_RigControlValue& CurrentValue = GetHierarchy()->GetControlValue(InControl, InValueType, false);
			const FTransform FinalTransform = CurrentValue.GetAsTransform(InControl->Settings.ControlType, InControl->Settings.PrimaryAxis);
			FTransform AdditiveTransform = FinalTransform * AnimPose.LocalTransform.Inverse();
			AdditiveTransform.SetRotation(AdditiveTransform.GetRotation().GetNormalized());
			FTLLRN_RigControlValue AdditiveValue;
			AdditiveValue.SetFromTransform(AdditiveTransform, InControl->Settings.ControlType, InControl->Settings.PrimaryAxis);
			return AdditiveValue;
		}
	}
	return GetHierarchy()->GetControlValue(InControl, InValueType);
}

void UTLLRN_ControlRig::SetControlValueImpl(const FName& InControlName, const FTLLRN_RigControlValue& InValue, bool bNotify,
	const FTLLRN_RigControlModifiedContext& Context, bool bSetupUndo, bool bPrintPythonCommnds, bool bFixEulerFlips)
{
	const FTLLRN_RigElementKey Key(InControlName, ETLLRN_RigElementType::Control);

	FTLLRN_RigControlElement* ControlElement = DynamicHierarchy->Find<FTLLRN_RigControlElement>(Key);
	if(ControlElement == nullptr)
	{
		return;
	}
	if (bIsAdditive)
	{
		// Store the value to apply it after the backwards solve
		FRigSetControlValueInfo Info = {InValue, bNotify, Context, bSetupUndo, bPrintPythonCommnds, bFixEulerFlips};
		ControlValues.Add(ControlElement->GetKey(), Info);
	}
	else
	{
		DynamicHierarchy->SetControlValue(ControlElement, InValue, ETLLRN_RigControlValueType::Current, bSetupUndo, false, bPrintPythonCommnds, bFixEulerFlips);

		if (bNotify && OnControlModified.IsBound())
		{
			OnControlModified.Broadcast(this, ControlElement, Context);
		}
	}
}

void UTLLRN_ControlRig::SwitchToParent(const FTLLRN_RigElementKey& InElementKey, const FTLLRN_RigElementKey& InNewParentKey, bool bInitial, bool bAffectChildren)
{
	FTLLRN_RigBaseElement* Element = DynamicHierarchy->Find<FTLLRN_RigBaseElement>(InElementKey);
	if(Element == nullptr)
	{
		return;
	}
	if (InNewParentKey.Type != ETLLRN_RigElementType::Reference)
	{
		FTLLRN_RigBaseElement* Parent = DynamicHierarchy->Find<FTLLRN_RigBaseElement>(InNewParentKey);
		if (Parent == nullptr)
		{
			return;
		}
	}
	if (bIsAdditive)
	{
		FRigSwitchParentInfo Info;
		Info.NewParent = InNewParentKey;
		Info.bInitial = bInitial;
		Info.bAffectChildren = bAffectChildren;
		SwitchParentValues.Add(InElementKey, Info);
	}
	else
	{
#if WITH_EDITOR
		UTLLRN_RigHierarchy::TElementDependencyMap Dependencies = DynamicHierarchy->GetDependenciesForVM(GetVM());
		DynamicHierarchy->SwitchToParent(InElementKey, InNewParentKey, bInitial, bAffectChildren, Dependencies, nullptr);
#else
		DynamicHierarchy->SwitchToParent(InElementKey, InNewParentKey, bInitial, bAffectChildren);
#endif
	}
}

bool UTLLRN_ControlRig::SetControlGlobalTransform(const FName& InControlName, const FTransform& InGlobalTransform, bool bNotify, const FTLLRN_RigControlModifiedContext& Context, bool bSetupUndo, bool bPrintPythonCommands, bool bFixEulerFlips)
{
	FTransform GlobalTransform = InGlobalTransform;
	ETLLRN_RigTransformType::Type TransformType = ETLLRN_RigTransformType::CurrentGlobal;
	if (IsConstructionModeEnabled())
	{
#if WITH_EDITOR
		if(bRecordSelectionPoseForConstructionMode)
		{
			SelectionPoseForConstructionMode.FindOrAdd(FTLLRN_RigElementKey(InControlName, ETLLRN_RigElementType::Control)) = GlobalTransform;
		}
#endif
		TransformType = ETLLRN_RigTransformType::InitialGlobal;
		GlobalTransform = SetupControlFromGlobalTransform(InControlName, GlobalTransform);
	}

	FTLLRN_RigControlElement* Control = FindControl(InControlName);
	if (IsAdditive())
	{
		if (Control)
		{
			// For additive control rigs, proxy controls should not be stored and applied after the backwards solve like other controls
			// Instead, apply the transform as global, run the forward solve and let the driven controls request keying
			if (Control->Settings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl)
			{
				DynamicHierarchy->SetTransform(Control, GlobalTransform, ETLLRN_RigTransformType::CurrentGlobal, true);
				Execute(FTLLRN_RigUnit_BeginExecution::EventName);
				return true;
			}
		}
	}

	FTLLRN_RigControlValue Value = GetControlValueFromGlobalTransform(InControlName, GlobalTransform, TransformType);
	if (OnFilterControl.IsBound())
	{
		if (Control)
		{
			OnFilterControl.Broadcast(this, Control, Value);
		}
	}

	SetControlValue(InControlName, Value, bNotify, Context, bSetupUndo, bPrintPythonCommands, bFixEulerFlips);
	return true;
}

FTLLRN_RigControlValue UTLLRN_ControlRig::GetControlValueFromGlobalTransform(const FName& InControlName, const FTransform& InGlobalTransform, ETLLRN_RigTransformType::Type InTransformType)
{
	FTLLRN_RigControlValue Value;

	if (FTLLRN_RigControlElement* ControlElement = FindControl(InControlName))
	{
		if(DynamicHierarchy)
		{
			if (bIsAdditive)
			{
				const int32 ControlIndex = ControlsAfterBackwardsSolve.GetIndex(ControlElement->GetKey());
				if (ControlIndex != INDEX_NONE)
				{
					// ParentGlobalTransform = ParentAnimationGlobalTransform + ParentAdditiveTransform
					const FTransform& ParentGlobalTransform = DynamicHierarchy->GetParentTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal);

					// LocalTransform = InGlobal - ParentGlobalTransform - OffsetLocal
					const FTransform OffsetLocalTransform = InGlobalTransform.GetRelativeTransform(ParentGlobalTransform);
					const FTransform LocalTransform = OffsetLocalTransform.GetRelativeTransform(DynamicHierarchy->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal));

					// Additive = LocalTransform - AnimLocalTransform
					const FTransform& AnimLocalTransform = ControlsAfterBackwardsSolve[ControlIndex].LocalTransform;
					const FTransform AdditiveTransform = LocalTransform.GetRelativeTransform(AnimLocalTransform);

					Value.SetFromTransform(AdditiveTransform, ControlElement->Settings.ControlType, ControlElement->Settings.PrimaryAxis);
				}
			}
			else
			{
				FTransform Transform = DynamicHierarchy->ComputeLocalControlValue(ControlElement, InGlobalTransform, InTransformType);
				Value.SetFromTransform(Transform, ControlElement->Settings.ControlType, ControlElement->Settings.PrimaryAxis);
			}

			if (ShouldApplyLimits())
			{
				ControlElement->Settings.ApplyLimits(Value);
			}
		}
	}

	return Value;
}

void UTLLRN_ControlRig::SetControlLocalTransform(const FName& InControlName, const FTransform& InLocalTransform, bool bNotify, const FTLLRN_RigControlModifiedContext& Context, bool bSetupUndo, bool bFixEulerFlips)
{
	if (FTLLRN_RigControlElement* ControlElement = FindControl(InControlName))
	{
		FTLLRN_RigControlValue Value;
		Value.SetFromTransform(InLocalTransform, ControlElement->Settings.ControlType, ControlElement->Settings.PrimaryAxis);

		if (OnFilterControl.IsBound())
		{
			OnFilterControl.Broadcast(this, ControlElement, Value);
			
		}
		SetControlValue(InControlName, Value, bNotify, Context, bSetupUndo, false /*bPrintPython not defined!*/, bFixEulerFlips);
	}
}

FTransform UTLLRN_ControlRig::GetControlLocalTransform(const FName& InControlName)
{
	if(DynamicHierarchy == nullptr)
	{
		return FTransform::Identity;
	}
	if (bIsAdditive)
	{
		FTLLRN_RigElementKey ControlKey (InControlName, ETLLRN_RigElementType::Control);
		if (FTLLRN_RigBaseElement* Element = DynamicHierarchy->Find(ControlKey))
		{
			if (FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
			{
				return GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).GetAsTransform(ControlElement->Settings.ControlType, ControlElement->Settings.PrimaryAxis);
			}
		}
	}
	return DynamicHierarchy->GetLocalTransform(FTLLRN_RigElementKey(InControlName, ETLLRN_RigElementType::Control));
}

FVector UTLLRN_ControlRig::GetControlSpecifiedEulerAngle(const FTLLRN_RigControlElement* InControlElement, bool bIsInitial) const
{
	if (bIsAdditive)
	{
		const ETLLRN_RigControlValueType Type = bIsInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current;
		const FTLLRN_RigControlValue Value = GetControlValue(InControlElement->GetKey().Name);
		FRotator Rotator = Value.GetAsTransform(InControlElement->Settings.ControlType, InControlElement->Settings.PrimaryAxis).Rotator();
		return FVector(Rotator.Roll, Rotator.Pitch, Rotator.Yaw);
	}
	else
	{
		return DynamicHierarchy->GetControlSpecifiedEulerAngle(InControlElement, bIsInitial);
	}
}

const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>>& UTLLRN_ControlRig::GetShapeLibraries() const
{
	const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>>* LibrariesPtr = &ShapeLibraries;

	if(!GetClass()->IsNative() && ShapeLibraries.IsEmpty())
	{
		if (UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(GetClass()->GetDefaultObject()))
		{
			LibrariesPtr = &CDO->ShapeLibraries;
		}
	}

	const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>>& Libraries = *LibrariesPtr;
	for(const TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>& ShapeLibrary : Libraries)
	{
		if (!ShapeLibrary.IsValid())
		{
			ShapeLibrary.LoadSynchronous();
		}
	}

	return Libraries;
}

void UTLLRN_ControlRig::SelectControl(const FName& InControlName, bool bSelect)
{
	if(DynamicHierarchy)
	{
		if(UTLLRN_RigHierarchyController* Controller = DynamicHierarchy->GetController(true))
		{
			Controller->SelectElement(FTLLRN_RigElementKey(InControlName, ETLLRN_RigElementType::Control), bSelect);
		}
	}
}

bool UTLLRN_ControlRig::ClearControlSelection()
{
	if(DynamicHierarchy)
	{
		if(UTLLRN_RigHierarchyController* Controller = DynamicHierarchy->GetController(true))
		{
			return Controller->ClearSelection();
		}
	}
	return false;
}

TArray<FName> UTLLRN_ControlRig::CurrentControlSelection() const
{
	TArray<FName> SelectedControlNames;

	if(DynamicHierarchy)
	{
		TArray<const FTLLRN_RigBaseElement*> SelectedControls = DynamicHierarchy->GetSelectedElements(ETLLRN_RigElementType::Control);
		for (const FTLLRN_RigBaseElement* SelectedControl : SelectedControls)
		{
			SelectedControlNames.Add(SelectedControl->GetFName());
		}
#if WITH_EDITOR
		for(const TSharedPtr<FRigDirectManipulationInfo>& ManipulationInfo : TLLRN_RigUnitManipulationInfos)
		{
			SelectedControlNames.Add(ManipulationInfo->ControlKey.Name);
		}
#endif
	}
	return SelectedControlNames;
}

bool UTLLRN_ControlRig::IsControlSelected(const FName& InControlName)const
{
	if(DynamicHierarchy)
	{
		if(FTLLRN_RigControlElement* ControlElement = FindControl(InControlName))
		{
			return DynamicHierarchy->IsSelected(ControlElement);
		}
	}
	return false;
}

void UTLLRN_ControlRig::RunPostConstructionEvent()
{
	if (PostConstructionEvent.IsBound())
	{
		FTLLRN_ControlRigBracketScope BracketScope(PostConstructionBracket);
		PostConstructionEvent.Broadcast(this, FTLLRN_RigUnit_PrepareForExecution::EventName);
	}
}

void UTLLRN_ControlRig::HandleHierarchyModified(ETLLRN_RigHierarchyNotification InNotification, UTLLRN_RigHierarchy* InHierarchy,
                                          const FTLLRN_RigBaseElement* InElement)
{
	switch(InNotification)
	{
		case ETLLRN_RigHierarchyNotification::ElementSelected:
		case ETLLRN_RigHierarchyNotification::ElementDeselected:
		{
			bool bClearTransientControls = true;
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>((FTLLRN_RigBaseElement*)InElement))
			{
				const bool bSelected = InNotification == ETLLRN_RigHierarchyNotification::ElementSelected;
				ControlSelected().Broadcast(this, ControlElement, bSelected);

				OnControlSelected_BP.Broadcast(this, *ControlElement, bSelected);
				bClearTransientControls = !ControlElement->Settings.bIsTransientControl;
			}

#if WITH_EDITOR
			if(IsConstructionModeEnabled())
			{
				if(InElement->GetType() == ETLLRN_RigElementType::Control)
				{
					if(InNotification == ETLLRN_RigHierarchyNotification::ElementSelected)
					{
						SelectionPoseForConstructionMode.FindOrAdd(InElement->GetKey()) = GetHierarchy()->GetGlobalTransform(InElement->GetKey());
					}
					else
					{
						SelectionPoseForConstructionMode.Remove(InElement->GetKey());
					}
				}
			}
			if(bClearTransientControls)
			{
				ClearTransientControls();
			}
#endif
			break;
		}
		case ETLLRN_RigHierarchyNotification::ControlSettingChanged:
		case ETLLRN_RigHierarchyNotification::ControlShapeTransformChanged:
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>((FTLLRN_RigBaseElement*)InElement))
			{
				ControlModified().Broadcast(this, ControlElement, FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey::Never));
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

#if WITH_EDITOR

bool UTLLRN_ControlRig::CanAddTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget, FString* OutFailureReason)
{
	if (InNode == nullptr)
	{
		if(OutFailureReason)
		{
			static const FString Reason = TEXT("Provided node is nullptr.");
			*OutFailureReason = Reason;
		}
		return false;
	}
	if (DynamicHierarchy == nullptr)
	{
		if(OutFailureReason)
		{
			static const FString Reason = TEXT("The rig does not contain a hierarchy.");
			*OutFailureReason = Reason;
		}
		return false;
	}

	const UScriptStruct* UnitStruct = InNode->GetScriptStruct();
	if(UnitStruct == nullptr)
	{
		if(OutFailureReason)
		{
			static const FString Reason = TEXT("The node is not resolved.");
			*OutFailureReason = Reason;
		}
		return false;
	}

	const TSharedPtr<FStructOnScope> NodeInstance = InNode->ConstructLiveStructInstance(this);
	if(!NodeInstance.IsValid() || !NodeInstance->IsValid())
	{
		if(OutFailureReason)
		{
			static const FString Reason = TEXT("Unexpected error: Node instance could not be constructed.");
			*OutFailureReason = Reason;
		}
		return false;
	}

	const FTLLRN_RigUnit* UnitInstance = GetTLLRN_RigUnitInstanceFromScope(NodeInstance);
	check(UnitInstance);

	TArray<FRigDirectManipulationTarget> Targets;
	if(UnitInstance->GetDirectManipulationTargets(InNode, NodeInstance, DynamicHierarchy, Targets, OutFailureReason))
	{
		return Targets.Contains(InTarget);
	}
	return false;
}

FName UTLLRN_ControlRig::AddTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget)
{
	if(!CanAddTransientControl(InNode, InTarget, nullptr))
	{
		return NAME_None;
	}
	RemoveTransientControl(InNode, InTarget);

	UTLLRN_RigHierarchyController* Controller = DynamicHierarchy->GetController(true);
	if(Controller == nullptr)
	{
		return NAME_None;
	}

	const UScriptStruct* UnitStruct = InNode->GetScriptStruct();
	check(UnitStruct);

	const TSharedPtr<FStructOnScope> NodeInstance = InNode->ConstructLiveStructInstance(this);
	check(NodeInstance.IsValid() && NodeInstance->IsValid());

	const FTLLRN_RigUnit* UnitInstance = GetTLLRN_RigUnitInstanceFromScope(NodeInstance);
	check(UnitInstance);

	const FName ControlName = GetNameForTransientControl(InNode, InTarget);
	FTransform ShapeTransform = FTransform::Identity;
	ShapeTransform.SetScale3D(FVector::ZeroVector);

	FTLLRN_RigControlSettings Settings;
	Settings.ControlType = ETLLRN_RigControlType::Transform;
	Settings.bIsTransientControl = true;
	Settings.DisplayName = TEXT("Temporary Control");

	TSharedPtr<FRigDirectManipulationInfo> Info = MakeShareable(new FRigDirectManipulationInfo());
	Info->Target = InTarget;
	Info->Node = TWeakObjectPtr<const URigVMUnitNode>(InNode);

	FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make(FTransform::Identity);
	UnitInstance->ConfigureDirectManipulationControl(InNode, Info, Settings, Value);

	Controller->ClearSelection();

    const FTLLRN_RigElementKey ControlKey = Controller->AddControl(
    	ControlName,
    	FTLLRN_RigElementKey(),
    	Settings,
    	Value,
    	FTransform::Identity,
    	ShapeTransform, false);

	Info->ControlKey = ControlKey;
	TLLRN_RigUnitManipulationInfos.Add(Info);
	SetTransientControlValue(InNode, Info);

	return Info->ControlKey.Name;
}

bool UTLLRN_ControlRig::SetTransientControlValue(const URigVMUnitNode* InNode, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	check(InNode);
	check(DynamicHierarchy);
	
	const TSharedPtr<FStructOnScope> NodeInstance = InNode->ConstructLiveStructInstance(this);
	check(NodeInstance.IsValid());
	check(NodeInstance->IsValid());

	const UScriptStruct* UnitStruct = InNode->GetScriptStruct();
	check(UnitStruct);

	FTLLRN_RigUnit* UnitInstance = GetTLLRN_RigUnitInstanceFromScope(NodeInstance);
	check(UnitInstance);

	const FName ControlName = GetNameForTransientControl(InNode, InInfo->Target);
	const FTLLRN_RigControlElement* ControlElement = DynamicHierarchy->Find<FTLLRN_RigControlElement>({ ControlName, ETLLRN_RigElementType::Control });
	if(ControlElement == nullptr)
	{
		return false;
	}

	FTLLRN_ControlRigExecuteContext& PublicContext = GetRigVMExtendedExecuteContext().GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
	const bool bResult = UnitInstance->UpdateHierarchyForDirectManipulation(InNode, NodeInstance, PublicContext, InInfo);
	InInfo->bInitialized = true;
	return bResult;
}

FName UTLLRN_ControlRig::RemoveTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget)
{
	if ((InNode == nullptr) || (DynamicHierarchy == nullptr))
	{
		return NAME_None;
	}

	TLLRN_RigUnitManipulationInfos.RemoveAll([InTarget](const TSharedPtr<FRigDirectManipulationInfo>& Info) -> bool
	{
		return Info->Target == InTarget;
	});

	UTLLRN_RigHierarchyController* Controller = DynamicHierarchy->GetController(true);
	if(Controller == nullptr)
	{
		return NAME_None;
	}

	const FName ControlName = GetNameForTransientControl(InNode, InTarget);
	if(FTLLRN_RigControlElement* ControlElement = FindControl(ControlName))
	{
		DynamicHierarchy->Notify(ETLLRN_RigHierarchyNotification::ElementDeselected, ControlElement);
		if(Controller->RemoveElement(ControlElement))
		{
#if WITH_EDITOR
			SelectionPoseForConstructionMode.Remove(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control));
#endif
			return ControlName;
		}
	}

	return NAME_None;
}

FName UTLLRN_ControlRig::AddTransientControl(const FTLLRN_RigElementKey& InElement)
{
	if (!InElement.IsValid())
	{
		return NAME_None;
	}

	if(DynamicHierarchy == nullptr)
	{
		return NAME_None;
	}

	UTLLRN_RigHierarchyController* Controller = DynamicHierarchy->GetController(true);
	if(Controller == nullptr)
	{
		return NAME_None;
	}

	const FName ControlName = GetNameForTransientControl(InElement);
	if(DynamicHierarchy->Contains(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
	{
		SetTransientControlValue(InElement);
		return ControlName;
	}

	const int32 ElementIndex = DynamicHierarchy->GetIndex(InElement);
	if (ElementIndex == INDEX_NONE)
	{
		return NAME_None;
	}

	FTransform ShapeTransform = FTransform::Identity;
	ShapeTransform.SetScale3D(FVector::ZeroVector);

	FTLLRN_RigControlSettings Settings;
	Settings.ControlType = ETLLRN_RigControlType::Transform;
	Settings.bIsTransientControl = true;
	Settings.DisplayName = TEXT("Temporary Control");

	FTLLRN_RigElementKey Parent;
	switch (InElement.Type)
	{
		case ETLLRN_RigElementType::Bone:
		{
			Parent = DynamicHierarchy->GetFirstParent(InElement);
			break;
		}
		case ETLLRN_RigElementType::Null:
		{
			Parent = InElement;
			break;
		}
		default:
		{
			break;
		}
	}

	const FTLLRN_RigElementKey ControlKey = Controller->AddControl(
        ControlName,
        Parent,
        Settings,
        FTLLRN_RigControlValue::Make(FTransform::Identity),
        FTransform::Identity,
        ShapeTransform, false);

	if (InElement.Type == ETLLRN_RigElementType::Bone)
	{
		// don't allow transient control to modify forward mode poses when we
		// already switched to the construction mode
		if (!IsConstructionModeEnabled())
		{
			if(FTLLRN_RigBoneElement* BoneElement = DynamicHierarchy->Find<FTLLRN_RigBoneElement>(InElement))
			{
				// add a modify bone AnimNode internally that the transient control controls for imported bones only
				// for user created bones, refer to UTLLRN_ControlRig::TransformOverrideForUserCreatedBones 
				if (BoneElement->BoneType == ETLLRN_RigBoneType::Imported)
				{ 
					if (PreviewInstance)
					{
						PreviewInstance->ModifyBone(InElement.Name);
					}
				}
				else if (BoneElement->BoneType == ETLLRN_RigBoneType::User)
				{
					// add an empty entry, which will be given the correct value in
					// SetTransientControlValue(InElement);
					TransformOverrideForUserCreatedBones.FindOrAdd(InElement.Name);
				}
			}
		}
	}

	SetTransientControlValue(InElement);

	return ControlKey.Name;
}

bool UTLLRN_ControlRig::SetTransientControlValue(const FTLLRN_RigElementKey& InElement)
{
	if (!InElement.IsValid())
	{
		return false;
	}

	if(DynamicHierarchy == nullptr)
	{
		return false;
	}

	const FName ControlName = GetNameForTransientControl(InElement);
	if(FTLLRN_RigControlElement* ControlElement = FindControl(ControlName))
	{
		if (InElement.Type == ETLLRN_RigElementType::Bone)
		{
			if (IsConstructionModeEnabled())
			{
				// need to get initial because that is what construction mode uses
				// specifically, when user change the initial from the details panel
				// this will allow the transient control to react to that change
				const FTransform InitialLocalTransform = DynamicHierarchy->GetInitialLocalTransform(InElement);
				DynamicHierarchy->SetTransform(ControlElement, InitialLocalTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
				DynamicHierarchy->SetTransform(ControlElement, InitialLocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, false);
			}
			else
			{
				const FTransform LocalTransform = DynamicHierarchy->GetLocalTransform(InElement);
				DynamicHierarchy->SetTransform(ControlElement, LocalTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
				DynamicHierarchy->SetTransform(ControlElement, LocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, false);

				if (FTLLRN_RigBoneElement* BoneElement = DynamicHierarchy->Find<FTLLRN_RigBoneElement>(InElement))
				{
					if (BoneElement->BoneType == ETLLRN_RigBoneType::Imported)
					{
						if (PreviewInstance)
						{
							if (FAnimNode_ModifyBone* Modify = PreviewInstance->FindModifiedBone(InElement.Name))
							{
								Modify->Translation = LocalTransform.GetTranslation();
								Modify->Rotation = LocalTransform.GetRotation().Rotator();
								Modify->TranslationSpace = EBoneControlSpace::BCS_ParentBoneSpace;
								Modify->RotationSpace = EBoneControlSpace::BCS_ParentBoneSpace;
							}
						}	
					}
					else if (BoneElement->BoneType == ETLLRN_RigBoneType::User)
					{
						if (FTransform* TransformOverride = TransformOverrideForUserCreatedBones.Find(InElement.Name))
						{
							*TransformOverride = LocalTransform;
						}	
					}
				}
			}
		}
		else if (InElement.Type == ETLLRN_RigElementType::Null)
		{
			const FTransform GlobalTransform = DynamicHierarchy->GetGlobalTransform(InElement);
			DynamicHierarchy->SetTransform(ControlElement, GlobalTransform, ETLLRN_RigTransformType::InitialGlobal, true, false);
			DynamicHierarchy->SetTransform(ControlElement, GlobalTransform, ETLLRN_RigTransformType::CurrentGlobal, true, false);
		}

		return true;
	}
	return false;
}

FName UTLLRN_ControlRig::RemoveTransientControl(const FTLLRN_RigElementKey& InElement)
{
	if (!InElement.IsValid())
	{
		return NAME_None;
	}

	if(DynamicHierarchy == nullptr)
	{
		return NAME_None;
	}

	UTLLRN_RigHierarchyController* Controller = DynamicHierarchy->GetController(true);
	if(Controller == nullptr)
	{
		return NAME_None;
	}

	const FName ControlName = GetNameForTransientControl(InElement);
	if(FTLLRN_RigControlElement* ControlElement = FindControl(ControlName))
	{
		DynamicHierarchy->Notify(ETLLRN_RigHierarchyNotification::ElementDeselected, ControlElement);
		if(Controller->RemoveElement(ControlElement))
		{
#if WITH_EDITOR
			SelectionPoseForConstructionMode.Remove(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control));
#endif
			return ControlName;
		}
	}

	return NAME_None;
}

FName UTLLRN_ControlRig::GetNameForTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget) const
{
	check(InNode);
	check(DynamicHierarchy);
	
	const FString NodeName = InNode->GetName();
	return DynamicHierarchy->GetSanitizedName(FTLLRN_RigName(FString::Printf(TEXT("ControlForNode|%s|%s"), *NodeName, *InTarget.Name)));
}

FString UTLLRN_ControlRig::GetNodeNameFromTransientControl(const FTLLRN_RigElementKey& InKey)
{
	FString Name = InKey.Name.ToString();
	if(Name.StartsWith(TEXT("ControlForNode|")))
	{
		Name.RightChopInline(15);
		Name.LeftInline(Name.Find(TEXT("|")));
	}
	else
	{
		return FString();
	}
	return Name;
}

FString UTLLRN_ControlRig::GetTargetFromTransientControl(const FTLLRN_RigElementKey& InKey)
{
	FString Name = InKey.Name.ToString();
	if(Name.StartsWith(TEXT("ControlForNode|")))
	{
		Name.RightChopInline(15);
		Name.RightChopInline(Name.Find(TEXT("|")) + 1);
	}
	else
	{
		return FString();
	}
	return Name;
}

TSharedPtr<FRigDirectManipulationInfo> UTLLRN_ControlRig::GetTLLRN_RigUnitManipulationInfoForTransientControl(
	const FTLLRN_RigElementKey& InKey)
{
	const TSharedPtr<FRigDirectManipulationInfo>* InfoPtr = TLLRN_RigUnitManipulationInfos.FindByPredicate(
		[InKey](const TSharedPtr<FRigDirectManipulationInfo>& Info) -> bool
		{
			return Info->ControlKey == InKey;
		}) ;

	if(InfoPtr)
	{
		return *InfoPtr;
	}

	return TSharedPtr<FRigDirectManipulationInfo>();
}

FName UTLLRN_ControlRig::GetNameForTransientControl(const FTLLRN_RigElementKey& InElement)
{
	if (InElement.Type == ETLLRN_RigElementType::Control)
	{
		return InElement.Name;
	}

	const FName EnumName = *StaticEnum<ETLLRN_RigElementType>()->GetDisplayNameTextByValue((int64)InElement.Type).ToString();
	return *FString::Printf(TEXT("ControlForTLLRN_RigElement_%s_%s"), *EnumName.ToString(), *InElement.Name.ToString());
}

FTLLRN_RigElementKey UTLLRN_ControlRig::GetElementKeyFromTransientControl(const FTLLRN_RigElementKey& InKey)
{
	if(InKey.Type != ETLLRN_RigElementType::Control)
	{
		return FTLLRN_RigElementKey();
	}
	
	static FString TLLRN_ControlRigForElementBoneName;
	static FString TLLRN_ControlRigForElementNullName;

	if (TLLRN_ControlRigForElementBoneName.IsEmpty())
	{
		TLLRN_ControlRigForElementBoneName = FString::Printf(TEXT("ControlForTLLRN_RigElement_%s_"),
            *StaticEnum<ETLLRN_RigElementType>()->GetDisplayNameTextByValue((int64)ETLLRN_RigElementType::Bone).ToString());
		TLLRN_ControlRigForElementNullName = FString::Printf(TEXT("ControlForTLLRN_RigElement_%s_"),
            *StaticEnum<ETLLRN_RigElementType>()->GetDisplayNameTextByValue((int64)ETLLRN_RigElementType::Null).ToString());
	}
	
	FString Name = InKey.Name.ToString();
	if(Name.StartsWith(TLLRN_ControlRigForElementBoneName))
	{
		Name.RightChopInline(TLLRN_ControlRigForElementBoneName.Len());
		return FTLLRN_RigElementKey(*Name, ETLLRN_RigElementType::Bone);
	}
	if(Name.StartsWith(TLLRN_ControlRigForElementNullName))
	{
		Name.RightChopInline(TLLRN_ControlRigForElementNullName.Len());
		return FTLLRN_RigElementKey(*Name, ETLLRN_RigElementType::Null);
	}
	
	return FTLLRN_RigElementKey();;
}

void UTLLRN_ControlRig::ClearTransientControls()
{
	if(DynamicHierarchy == nullptr)
	{
		return;
	}

	UTLLRN_RigHierarchyController* Controller = DynamicHierarchy->GetController(true);
	if(Controller == nullptr)
	{
		return;
	}

	if(bIsClearingTransientControls)
	{
		return;
	}
	TGuardValue<bool> ReEntryGuard(bIsClearingTransientControls, true);

	TLLRN_RigUnitManipulationInfos.Reset();

	const TArray<FTLLRN_RigControlElement*> ControlsToRemove = DynamicHierarchy->GetTransientControls();
	for (FTLLRN_RigControlElement* ControlToRemove : ControlsToRemove)
	{
		const FTLLRN_RigElementKey KeyToRemove = ControlToRemove->GetKey();
		if(Controller->RemoveElement(ControlToRemove))
		{
			SelectionPoseForConstructionMode.Remove(KeyToRemove);
		}
	}
}

void UTLLRN_ControlRig::ApplyTransformOverrideForUserCreatedBones()
{
	if(DynamicHierarchy == nullptr)
	{
		return;
	}
	
	for (const auto& Entry : TransformOverrideForUserCreatedBones)
	{
		DynamicHierarchy->SetLocalTransform(FTLLRN_RigElementKey(Entry.Key, ETLLRN_RigElementType::Bone), Entry.Value, false);
	}
}

void UTLLRN_ControlRig::ApplySelectionPoseForConstructionMode(const FName& InEventName)
{
	FTLLRN_RigControlModifiedContext ControlValueContext;
	ControlValueContext.EventName = InEventName;

	TGuardValue<bool> DisableRecording(bRecordSelectionPoseForConstructionMode, false);
	for(const TPair<FTLLRN_RigElementKey, FTransform>& Pair : SelectionPoseForConstructionMode)
	{
		const FName ControlName = Pair.Key.Name;
		const FTLLRN_RigControlValue Value = GetControlValueFromGlobalTransform(ControlName, Pair.Value, ETLLRN_RigTransformType::InitialGlobal);
		SetControlValue(ControlName, Value, true, ControlValueContext, false, false, false);
	}
}

#endif

void UTLLRN_ControlRig::HandleHierarchyEvent(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigEventContext& InEvent)
{
	if (TLLRN_RigEventDelegate.IsBound())
	{
		TLLRN_RigEventDelegate.Broadcast(InHierarchy, InEvent);
	}

	switch (InEvent.Event)
	{
		case ETLLRN_RigEvent::RequestAutoKey:
		{
			int32 Index = InHierarchy->GetIndex(InEvent.Key);
			if (Index != INDEX_NONE && InEvent.Key.Type == ETLLRN_RigElementType::Control)
			{
				if(FTLLRN_RigControlElement* ControlElement = InHierarchy->GetChecked<FTLLRN_RigControlElement>(Index))
				{
					FTLLRN_RigControlModifiedContext Context;
					Context.SetKey = ETLLRN_ControlRigSetKey::Always;
					Context.LocalTime = InEvent.LocalTime;
					Context.EventName = InEvent.SourceEventName;
					ControlModified().Broadcast(this, ControlElement, Context);
				}
			}
			break;
		}
		case ETLLRN_RigEvent::OpenUndoBracket:
		case ETLLRN_RigEvent::CloseUndoBracket:
		{
			const bool bOpenUndoBracket = InEvent.Event == ETLLRN_RigEvent::OpenUndoBracket;
			ControlUndoBracketIndex = FMath::Max<int32>(0, ControlUndoBracketIndex + (bOpenUndoBracket ? 1 : -1));
			ControlUndoBracket().Broadcast(this, bOpenUndoBracket);
			break;
		}
		default:
		{
			break;
		}
	}
}

void UTLLRN_ControlRig::GetControlsInOrder(TArray<FTLLRN_RigControlElement*>& SortedControls) const
{
	SortedControls.Reset();

	if(DynamicHierarchy == nullptr)
	{
		return;
	}

	SortedControls = DynamicHierarchy->GetControls(true);
}

const FTLLRN_RigInfluenceMap* UTLLRN_ControlRig::FindInfluenceMap(const FName& InEventName)
{
	if (UTLLRN_ControlRig* CDO = GetClass()->GetDefaultObject<UTLLRN_ControlRig>())
	{
		return CDO->Influences.Find(InEventName);
	}
	return nullptr;
}

#if WITH_EDITOR

void UTLLRN_ControlRig::PreEditChange(FProperty* PropertyAboutToChange)
{
	// for BP user authored properties let's ignore changes since they
	// will be distributed from the BP anyway to all archetype instances.
	if(PropertyAboutToChange && !PropertyAboutToChange->IsNative())
	{
		if(const UBlueprint* Blueprint = Cast<UBlueprint>(GetClass()->ClassGeneratedBy))
		{
			const struct FBPVariableDescription* FoundVariable = Blueprint->NewVariables.FindByPredicate(
				[PropertyAboutToChange](const struct FBPVariableDescription& NewVariable)
				{
					return NewVariable.VarName == PropertyAboutToChange->GetFName();
				}
			);

			if(FoundVariable)
			{
				return;
			}
		}
	}
	
	Super::PreEditChange(PropertyAboutToChange);
}

void UTLLRN_ControlRig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

FTLLRN_RigUnit* UTLLRN_ControlRig::GetTLLRN_RigUnitInstanceFromScope(TSharedPtr<FStructOnScope> InScope)
{
	if(InScope.IsValid())
	{
		if(InScope->IsValid())
		{
			if(InScope->GetStruct()->IsChildOf(FTLLRN_RigUnit::StaticStruct()))
			{
				return (FTLLRN_RigUnit*)InScope->GetStructMemory(); 
			}
		}
	}
	static FStructOnScope DefaultTLLRN_RigUnitInstance(FTLLRN_RigUnit::StaticStruct());
	return (FTLLRN_RigUnit*)DefaultTLLRN_RigUnitInstance.GetStructMemory();
}

const TArray<UAssetUserData*>* UTLLRN_ControlRig::GetAssetUserDataArray() const
{
	if(HasAnyFlags(RF_ClassDefaultObject))
	{
#if WITH_EDITOR
		if (IsRunningCookCommandlet())
		{
			return &ToRawPtrTArrayUnsafe(AssetUserData);
		}
		else
		{
			static thread_local TArray<TObjectPtr<UAssetUserData>> CachedAssetUserData;
			CachedAssetUserData.Reset();
			CachedAssetUserData.Append(AssetUserData);
			CachedAssetUserData.Append(AssetUserDataEditorOnly);
			return &ToRawPtrTArrayUnsafe(CachedAssetUserData);
		}
#else
		return &ToRawPtrTArrayUnsafe(AssetUserData);
#endif
	}

	CombinedAssetUserData.Reset();

	if (UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(GetClass()->GetDefaultObject(false)))
	{
		CombinedAssetUserData.Append(*CDO->GetAssetUserDataArray());
	}
	else
	{
		CombinedAssetUserData.Append(AssetUserData);
#if WITH_EDITOR
		if (!IsRunningCookCommandlet())
		{
			CombinedAssetUserData.Append(AssetUserDataEditorOnly);
		}
#endif
	}
	
	if(GetExternalAssetUserDataDelegate.IsBound())
	{
		CombinedAssetUserData.Append(GetExternalAssetUserDataDelegate.Execute());
	}

	// find the data assets on the external variable list
	TArray<FRigVMExternalVariable> ExternalVariables = GetExternalVariablesImpl(false);
	for(const FRigVMExternalVariable& ExternalVariable : ExternalVariables)
	{
		if(ExternalVariable.Memory != nullptr)
		{
			if(const UClass* Class = Cast<UClass>(ExternalVariable.TypeObject))
			{
				if(Class->IsChildOf(UDataAsset::StaticClass()))
				{
					TObjectPtr<UDataAsset>& DataAsset = *(TObjectPtr<UDataAsset>*)ExternalVariable.Memory;
					if(IsValid(DataAsset) && !DataAsset->GetFName().IsNone())
					{
						if(const TObjectPtr<UDataAssetLink>* ExistingDataAssetLink =
							ExternalVariableDataAssetLinks.Find(ExternalVariable.Name))
						{
							if(!IsValid(*ExistingDataAssetLink) ||
								(*ExistingDataAssetLink)->HasAnyFlags(EObjectFlags::RF_BeginDestroyed))
							{
								ExternalVariableDataAssetLinks.Remove(ExternalVariable.Name);
							}
						}
						if(!ExternalVariableDataAssetLinks.Contains(ExternalVariable.Name))
						{
							ExternalVariableDataAssetLinks.Add(
								ExternalVariable.Name,
								NewObject<UDataAssetLink>(GetTransientPackage(), UDataAssetLink::StaticClass(), NAME_None, RF_Transient));
						}

						TObjectPtr<UDataAssetLink>& DataAssetLink = ExternalVariableDataAssetLinks.FindChecked(ExternalVariable.Name);
						DataAssetLink->NameSpace = ExternalVariable.Name.ToString();
						DataAssetLink->SetDataAsset(DataAsset);
						CombinedAssetUserData.Add(DataAssetLink);
					}
				}
			}
		}
	}

	// Propagate the outer rig user data into the child modules
	if (UTLLRN_ControlRig* OuterCR = GetTypedOuter<UTLLRN_ControlRig>())
	{
		if(const TArray<UAssetUserData*>* OuterUserDataArray = OuterCR->GetAssetUserDataArray())
		{
			for(UAssetUserData* OuterUserData : *OuterUserDataArray)
			{
				CombinedAssetUserData.Add(OuterUserData);
			}
		}
	}
	
	if(OuterSceneComponent.IsValid())
	{
		if(const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(OuterSceneComponent))
		{
			if(USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset())
			{
				if(const USkeleton* Skeleton = SkeletalMesh->GetSkeleton())
				{
					if(const TArray<UAssetUserData*>* SkeletonUserDataArray = Skeleton->GetAssetUserDataArray())
					{
						for(UAssetUserData* SkeletonUserData : *SkeletonUserDataArray)
						{
							CombinedAssetUserData.Add(SkeletonUserData);
						}
					}
				}
				if(const TArray<UAssetUserData*>* SkeletalMeshUserDataArray = SkeletalMesh->GetAssetUserDataArray())
				{
					for(UAssetUserData* SkeletalMeshUserData : *SkeletalMeshUserDataArray)
					{
						CombinedAssetUserData.Add(SkeletalMeshUserData);
					}
				}
			}
			if (const TArray<UAssetUserData*>* ActorComponentUserDataArray = SkeletalMeshComponent->GetAssetUserDataArray())
			{
				for(UAssetUserData* ActorComponentUserData : *ActorComponentUserDataArray)
				{
					CombinedAssetUserData.Add(ActorComponentUserData);
				}
			}
		}
	}

	return &ToRawPtrTArrayUnsafe(CombinedAssetUserData);
}

void UTLLRN_ControlRig::CopyPoseFromOtherRig(UTLLRN_ControlRig* Subject)
{
	check(DynamicHierarchy);
	check(Subject);
	UTLLRN_RigHierarchy* OtherHierarchy = Subject->GetHierarchy();
	check(OtherHierarchy);

	for (FTLLRN_RigBaseElement* Element : *DynamicHierarchy)
	{
		FTLLRN_RigBaseElement* OtherElement = OtherHierarchy->Find(Element->GetKey());
		if(OtherElement == nullptr)
		{
			continue;
		}

		if(OtherElement->GetType() != Element->GetType())
		{
			continue;
		}

		if(FTLLRN_RigBoneElement* BoneElement = Cast<FTLLRN_RigBoneElement>(Element))
		{
			FTLLRN_RigBoneElement* OtherBoneElement = CastChecked<FTLLRN_RigBoneElement>(OtherElement);
			const FTransform Transform = OtherHierarchy->GetTransform(OtherBoneElement, ETLLRN_RigTransformType::CurrentLocal);
			DynamicHierarchy->SetTransform(BoneElement, Transform, ETLLRN_RigTransformType::CurrentLocal, true, false);
		}
		else if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(Element))
		{
			FTLLRN_RigCurveElement* OtherCurveElement = CastChecked<FTLLRN_RigCurveElement>(OtherElement);
			const float Value = OtherHierarchy->GetCurveValue(OtherCurveElement);
			DynamicHierarchy->SetCurveValue(CurveElement, Value, false);
		}
	}
}

void UTLLRN_ControlRig::SetBoneInitialTransformsFromAnimInstance(UAnimInstance* InAnimInstance)
{
	FMemMark Mark(FMemStack::Get());
	FCompactPose OutPose;
	OutPose.ResetToRefPose(InAnimInstance->GetRequiredBones());
	SetBoneInitialTransformsFromCompactPose(&OutPose);
}

void UTLLRN_ControlRig::SetBoneInitialTransformsFromAnimInstanceProxy(const FAnimInstanceProxy* InAnimInstanceProxy)
{
	FMemMark Mark(FMemStack::Get());
	FCompactPose OutPose;
	OutPose.ResetToRefPose(InAnimInstanceProxy->GetRequiredBones());
	SetBoneInitialTransformsFromCompactPose(&OutPose);
}

void UTLLRN_ControlRig::SetBoneInitialTransformsFromSkeletalMeshComponent(USkeletalMeshComponent* InSkelMeshComp, bool bUseAnimInstance)
{
	check(InSkelMeshComp);
	check(DynamicHierarchy);
	
	if (!bUseAnimInstance && (InSkelMeshComp->GetAnimInstance() != nullptr))
	{
		SetBoneInitialTransformsFromAnimInstance(InSkelMeshComp->GetAnimInstance());
	}
	else
	{
		SetBoneInitialTransformsFromSkeletalMesh(InSkelMeshComp->GetSkeletalMeshAsset());
	}
}


void UTLLRN_ControlRig::SetBoneInitialTransformsFromSkeletalMesh(USkeletalMesh* InSkeletalMesh)
{
	if (ensure(InSkeletalMesh))
	{ 
		SetBoneInitialTransformsFromRefSkeleton(InSkeletalMesh->GetRefSkeleton());
	}
}

void UTLLRN_ControlRig::SetBoneInitialTransformsFromRefSkeleton(const FReferenceSkeleton& InReferenceSkeleton)
{
	check(DynamicHierarchy);

	DynamicHierarchy->ForEach<FTLLRN_RigBoneElement>([this, InReferenceSkeleton](FTLLRN_RigBoneElement* BoneElement) -> bool
	{
		if(BoneElement->BoneType == ETLLRN_RigBoneType::Imported)
		{
			const int32 BoneIndex = InReferenceSkeleton.FindBoneIndex(BoneElement->GetFName());
			if (BoneIndex != INDEX_NONE)
			{
				const FTransform LocalInitialTransform = InReferenceSkeleton.GetRefBonePose()[BoneIndex];
				DynamicHierarchy->SetTransform(BoneElement, LocalInitialTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
				DynamicHierarchy->SetTransform(BoneElement, LocalInitialTransform, ETLLRN_RigTransformType::CurrentLocal, true, false);
			}
		}
		return true;
	});
	bResetInitialTransformsBeforeConstruction = false;
	RequestConstruction();
}

void UTLLRN_ControlRig::SetBoneInitialTransformsFromCompactPose(FCompactPose* InCompactPose)
{
	check(InCompactPose);

	if(!InCompactPose->IsValid())
	{
		return;
	}
	if(!InCompactPose->GetBoneContainer().IsValid())
	{
		return;
	}
	
	FMemMark Mark(FMemStack::Get());

	DynamicHierarchy->ForEach<FTLLRN_RigBoneElement>([this, InCompactPose](FTLLRN_RigBoneElement* BoneElement) -> bool
		{
			if (BoneElement->BoneType == ETLLRN_RigBoneType::Imported)
			{
				int32 MeshIndex = InCompactPose->GetBoneContainer().GetPoseBoneIndexForBoneName(BoneElement->GetFName());
				if (MeshIndex != INDEX_NONE)
				{
					FCompactPoseBoneIndex CPIndex = InCompactPose->GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
					if (CPIndex != INDEX_NONE)
					{
						FTransform LocalInitialTransform = InCompactPose->GetRefPose(CPIndex);
						DynamicHierarchy->SetTransform(BoneElement, LocalInitialTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
					}
				}
			}
			return true;
		});

	bResetInitialTransformsBeforeConstruction = false;
	RequestConstruction();
}

const FTLLRN_RigControlElementCustomization* UTLLRN_ControlRig::GetControlCustomization(const FTLLRN_RigElementKey& InControl) const
{
	check(InControl.Type == ETLLRN_RigElementType::Control);

	if(const FTLLRN_RigControlElementCustomization* Customization = ControlCustomizations.Find(InControl))
	{
		return Customization;
	}

	if(DynamicHierarchy)
	{
		if(const FTLLRN_RigControlElement* ControlElement = DynamicHierarchy->Find<FTLLRN_RigControlElement>(InControl))
		{
			return &ControlElement->Settings.Customization;
		}
	}

	return nullptr;
}

void UTLLRN_ControlRig::SetControlCustomization(const FTLLRN_RigElementKey& InControl, const FTLLRN_RigControlElementCustomization& InCustomization)
{
	check(InControl.Type == ETLLRN_RigElementType::Control);
	
	ControlCustomizations.FindOrAdd(InControl) = InCustomization;
}

void UTLLRN_ControlRig::PostInitInstanceIfRequired()
{
	if(GetHierarchy() == nullptr || VM == nullptr)
	{
		if(HasAnyFlags(RF_ClassDefaultObject))
		{
			PostInitInstance(nullptr);
		}
		else
		{
			UTLLRN_ControlRig* CDO = GetClass()->GetDefaultObject<UTLLRN_ControlRig>();
			ensure(VM == nullptr || VM == CDO->VM);
			PostInitInstance(CDO);
		}
	}
}

#if WITH_EDITORONLY_DATA
void UTLLRN_ControlRig::DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass)
{
	Super::DeclareConstructClasses(OutConstructClasses, SpecificSubclass);
	OutConstructClasses.Add(FTopLevelAssetPath(UTLLRN_RigHierarchy::StaticClass()));
	OutConstructClasses.Add(FTopLevelAssetPath(UAnimationDataSourceRegistry::StaticClass()));
}

#endif

USceneComponent* UTLLRN_ControlRig::GetOwningSceneComponent()
{
	if(OuterSceneComponent == nullptr)
	{
		const FTLLRN_ControlRigExecuteContext& PublicContext = GetRigVMExtendedExecuteContext().GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
		const FTLLRN_RigUnitContext& Context = PublicContext.UnitContext;

		USceneComponent* SceneComponentFromRegistry = Context.DataSourceRegistry->RequestSource<USceneComponent>(UTLLRN_ControlRig::OwnerComponent);
		if (SceneComponentFromRegistry)
		{
			OuterSceneComponent = SceneComponentFromRegistry;
		}

		if(OuterSceneComponent == nullptr)
		{
			OuterSceneComponent = Super::GetOwningSceneComponent();
		}
	}
	return OuterSceneComponent.Get();
}

void UTLLRN_ControlRig::PostInitInstance(URigVMHost* InCDO)
{
	const EObjectFlags SubObjectFlags =
	HasAnyFlags(RF_ClassDefaultObject) ?
		RF_Public | RF_DefaultSubObject :
		RF_Transient | RF_Transactional;

	FRigVMExtendedExecuteContext& Context = GetRigVMExtendedExecuteContext();
	
	Context.SetContextPublicDataStruct(FTLLRN_ControlRigExecuteContext::StaticStruct());

	Context.ExecutionReachedExit().RemoveAll(this);
	Context.ExecutionReachedExit().AddUObject(this, &UTLLRN_ControlRig::HandleExecutionReachedExit);
	UpdateVMSettings();

	// set up the hierarchy
	{
		// If this is not a CDO, it should have never saved its hieararchy. However, we have found that some rigs in the past
		// did save their hiearchies. If that's the case, let's mark them as garbage and rename them before creating our own hierarchy.
		if (!HasAnyFlags(RF_ClassDefaultObject))
		{
			FGCScopeGuard Guard;
			UObject* ObjectFound = StaticFindObjectFast(UTLLRN_RigHierarchy::StaticClass(), this, TEXT("DynamicHierarchy"), true, RF_DefaultSubObject);
			if (ObjectFound)
			{
				FName NewName = MakeUniqueObjectName(GetTransientPackage(), UTLLRN_RigHierarchy::StaticClass(), TEXT("DynamicHierarchy_Deleted"));
				ObjectFound->Rename(*NewName.ToString(), GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional | REN_AllowPackageLinkerMismatch);
				ObjectFound->MarkAsGarbage();
			}
		}

		if(!IsTLLRN_RigModuleInstance())
		{
			DynamicHierarchy = NewObject<UTLLRN_RigHierarchy>(this, TEXT("DynamicHierarchy"), SubObjectFlags);

			if (!IsInGameThread())
			{
				// If the object was created on a non-game thread, clear the async flag immediately, so that it can be
				// garbage collected in the future. 
				(void)DynamicHierarchy->AtomicallyClearInternalFlags(EInternalObjectFlags::Async);
			}
		}
	}

#if WITH_EDITOR
	if(!IsTLLRN_RigModuleInstance())
	{
		const TWeakObjectPtr<UTLLRN_ControlRig> WeakThis = this;
		DynamicHierarchy->OnUndoRedo().AddStatic(&UTLLRN_ControlRig::OnHierarchyTransformUndoRedoWeak, WeakThis);
	}
#endif

	if(!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (ensure(InCDO))
		{
			ensure(VM == nullptr || VM == InCDO->GetVM());
			if (VM == nullptr) // this is needed for some EngineTests, on a normal setup, the VM is set to the CDO VM already
			{
				VM = InCDO->GetVM();
			}

			if(!IsTLLRN_RigModuleInstance())
			{
				DynamicHierarchy->CopyHierarchy(CastChecked<UTLLRN_ControlRig>(InCDO)->GetHierarchy());
			}
		}
	}
	else // we are the CDO
	{
		check(InCDO == nullptr);

		if (VM == nullptr)
		{
			VM = NewObject<URigVM>(this, TEXT("TLLRN_ControlRig_VM"), SubObjectFlags);

			if (!IsInGameThread())
			{
				// If the object was created on a non-game thread, clear the async flag immediately, so that it can be
				// garbage collected in the future. 
				(void)VM->AtomicallyClearInternalFlags(EInternalObjectFlags::Async);
			}
		}

		// for default objects we need to check if the CDO is rooted. specialized Control Rigs
		// such as the FK control rig may not have a root since they are part of a C++ package.

		// since the sub objects are created after the constructor
		// GC won't consider them part of the CDO, even if they have the sub object flags
		// so even if CDO is rooted and references these sub objects, 
		// it is not enough to keep them alive.
		// Hence, we have to add them to root here.
		if(GetClass()->IsNative())
		{
			VM->AddToRoot();

			if(!IsTLLRN_RigModuleInstance())
			{
				DynamicHierarchy->AddToRoot();
			}
		}
	}

	if(UTLLRN_ControlRig* CDOTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(InCDO))
	{
		ElementKeyRedirector = FTLLRN_RigElementKeyRedirector(CDOTLLRN_ControlRig->ElementKeyRedirector, DynamicHierarchy);
	}

	RequestInit();
}

void UTLLRN_ControlRig::SetDynamicHierarchy(TObjectPtr<UTLLRN_RigHierarchy> InHierarchy)
{
	// Delete any existing hierarchy
	if (DynamicHierarchy->GetOuter() == this)
	{
		DynamicHierarchy->OnUndoRedo().RemoveAll(this);
		DynamicHierarchy->Rename(nullptr, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
		DynamicHierarchy->MarkAsGarbage();
	}
	DynamicHierarchy = InHierarchy;
}

UTLLRN_TransformableControlHandle* UTLLRN_ControlRig::CreateTLLRN_TransformableControlHandle(
	const FName& InControlName) const
{
	auto IsConstrainable = [this](const FName& InControlName)
	{
		const FTLLRN_RigControlElement* ControlElement = FindControl(InControlName);
		if (!ControlElement)
		{
			return false;
		}
		
		const FTLLRN_RigControlSettings& ControlSettings = ControlElement->Settings;
		if (ControlSettings.ControlType == ETLLRN_RigControlType::Bool ||
			ControlSettings.ControlType == ETLLRN_RigControlType::Float ||
			ControlSettings.ControlType == ETLLRN_RigControlType::ScaleFloat ||
			ControlSettings.ControlType == ETLLRN_RigControlType::Integer)
		{
			return false;
		}

		return true;
	};

	if (!IsConstrainable(InControlName))
	{
		return nullptr;
	}
	
	UTLLRN_TransformableControlHandle* CtrlHandle = NewObject<UTLLRN_TransformableControlHandle>(GetTransientPackage(), NAME_None, RF_Transactional);
	check(CtrlHandle);
	CtrlHandle->TLLRN_ControlRig = const_cast<UTLLRN_ControlRig*>(this);
	CtrlHandle->ControlName = InControlName;
	CtrlHandle->RegisterDelegates();
	return CtrlHandle;
}

void UTLLRN_ControlRig::OnHierarchyTransformUndoRedo(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InKey, ETLLRN_RigTransformType::Type InTransformType, const FTransform& InTransform, bool bIsUndo)
{
	if(InKey.Type == ETLLRN_RigElementType::Control)
	{
		if(FTLLRN_RigControlElement* ControlElement = InHierarchy->Find<FTLLRN_RigControlElement>(InKey))
		{
			ControlModified().Broadcast(this, ControlElement, FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey::Never));
		}
	}
}

int32 UTLLRN_ControlRig::NumPhysicsSolvers() const
{
	return PhysicsSolvers.Num();
}

const FTLLRN_RigPhysicsSolverDescription* UTLLRN_ControlRig::GetPhysicsSolver(int32 InIndex) const
{
	if(PhysicsSolvers.IsValidIndex(InIndex))
	{
		return &PhysicsSolvers[InIndex];
	}
	return nullptr;
}

const FTLLRN_RigPhysicsSolverDescription* UTLLRN_ControlRig::FindPhysicsSolver(const FTLLRN_RigPhysicsSolverID& InID) const
{
	return PhysicsSolvers.FindByPredicate([InID](const FTLLRN_RigPhysicsSolverDescription& Solver) -> bool
	{
		return Solver.ID == InID;
	});
}

const FTLLRN_RigPhysicsSolverDescription* UTLLRN_ControlRig::FindPhysicsSolverByName(const FName& InName) const
{
	return PhysicsSolvers.FindByPredicate([InName](const FTLLRN_RigPhysicsSolverDescription& Solver) -> bool
	{
		return Solver.Name == InName;
	});
}

FTLLRN_RigPhysicsSolverID UTLLRN_ControlRig::AddPhysicsSolver(FName InName, bool bSetupUndo, bool bPrintPythonCommand)
{
#if WITH_EDITOR
	if(CVarTLLRN_ControlTLLRN_RigHierarchyEnablePhysics.GetValueOnAnyThread() == false)
	{
		return FTLLRN_RigPhysicsSolverID();
	}
#endif

	if(InName.IsNone())
	{
		return FTLLRN_RigPhysicsSolverID();
	}

	const FName Name = UtilityHelpers::CreateUniqueName(InName, [this](const FName& InName) -> bool
	{
		return FindPhysicsSolverByName(InName) == nullptr;
	});

	FTLLRN_RigPhysicsSolverDescription Solver;
	Solver.ID = FTLLRN_RigPhysicsSolverDescription::MakeID(GetPathName(), Name);
	Solver.Name = Name;
	PhysicsSolvers.Add(Solver);
	return Solver.ID;
}

UTLLRN_ControlRig::FPoseScope::FPoseScope(UTLLRN_ControlRig* InTLLRN_ControlRig, ETLLRN_RigElementType InFilter, const TArray<FTLLRN_RigElementKey>& InElements, const ETLLRN_RigTransformType::Type InTransformType)
: TLLRN_ControlRig(InTLLRN_ControlRig)
, Filter(InFilter)
, TransformType(InTransformType)
{
	check(InTLLRN_ControlRig);
	const TArrayView<const FTLLRN_RigElementKey> ElementView(InElements.GetData(), InElements.Num());
	CachedPose = InTLLRN_ControlRig->GetHierarchy()->GetPose(IsInitial(InTransformType), InFilter, ElementView);
}

UTLLRN_ControlRig::FPoseScope::~FPoseScope()
{
	check(TLLRN_ControlRig);

	TLLRN_ControlRig->GetHierarchy()->SetPose(CachedPose, TransformType);
}

#if WITH_EDITOR

UTLLRN_ControlRig::FTransientControlScope::FTransientControlScope(TObjectPtr<UTLLRN_RigHierarchy> InHierarchy)
	:Hierarchy(InHierarchy)
{
	for (FTLLRN_RigControlElement* Control : Hierarchy->GetTransientControls())
	{
		FTransientControlInfo Info;
		Info.Name = Control->GetFName();
		Info.Parent = Hierarchy->GetFirstParent(Control->GetKey());
		Info.Settings = Control->Settings;
		// preserve whatever value that was produced by this transient control at the moment
		Info.Value = Hierarchy->GetControlValue(Control->GetKey(),ETLLRN_RigControlValueType::Current);
		Info.OffsetTransform = Hierarchy->GetControlOffsetTransform(Control, ETLLRN_RigTransformType::CurrentLocal);
		Info.ShapeTransform = Hierarchy->GetControlShapeTransform(Control, ETLLRN_RigTransformType::CurrentLocal);
				
		SavedTransientControls.Add(Info);
	}
}

UTLLRN_ControlRig::FTransientControlScope::~FTransientControlScope()
{
	if (UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController())
	{
		for (const FTransientControlInfo& Info : SavedTransientControls)
		{
			Controller->AddControl(
				Info.Name,
				Info.Parent,
				Info.Settings,
				Info.Value,
				Info.OffsetTransform,
				Info.ShapeTransform,
				false,
				false
			);
		}
	}
}

uint32 UTLLRN_ControlRig::GetShapeLibraryHash() const
{
	uint32 Hash = 0;
	for(const TPair<FString, FString>& Pair : ShapeLibraryNameMap)
	{
		Hash = HashCombine(Hash, HashCombine(GetTypeHash(Pair.Key), GetTypeHash(Pair.Value)));
	}
	for(const TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>& ShapeLibrary : ShapeLibraries)
	{
		Hash = HashCombine(Hash, GetTypeHash(ShapeLibrary.GetUniqueID()));
	}
	return Hash;
}

#endif
 
#undef LOCTEXT_NAMESPACE


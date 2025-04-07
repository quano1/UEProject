// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_RigHierarchy.h"
#include "TLLRN_ControlRig.h"

#include "Rigs/TLLRN_RigHierarchyElements.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "TLLRN_ModularRigRuleManager.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "UObject/AnimObjectVersion.h"
#include "TLLRN_ControlRigObjectVersion.h"
#include "TLLRN_ModularRig.h"
#include "Algo/Count.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "HAL/LowLevelMemTracker.h"

LLM_DEFINE_TAG(Animation_TLLRN_ControlRig);

#if WITH_EDITOR
#include "RigVMPythonUtils.h"
#include "ScopedTransaction.h"
#include "HAL/PlatformStackWalk.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/TransactionObjectEvent.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Algo/Transform.h"
#include "Algo/Sort.h"

static FCriticalSection GTLLRN_RigHierarchyStackTraceMutex;
static char GTLLRN_RigHierarchyStackTrace[65536];
static void TLLRN_RigHierarchyCaptureCallStack(FString& OutCallstack, uint32 NumCallsToIgnore)
{
	FScopeLock ScopeLock(&GTLLRN_RigHierarchyStackTraceMutex);
	GTLLRN_RigHierarchyStackTrace[0] = 0;
	FPlatformStackWalk::StackWalkAndDump(GTLLRN_RigHierarchyStackTrace, 65535, 1 + NumCallsToIgnore);
	OutCallstack = ANSI_TO_TCHAR(GTLLRN_RigHierarchyStackTrace);
}

// CVar to record all transform changes 
static TAutoConsoleVariable<int32> CVarTLLRN_ControlTLLRN_RigHierarchyTraceAlways(TEXT("TLLRN_ControlRig.Hierarchy.TraceAlways"), 0, TEXT("if nonzero we will record all transform changes."));
static TAutoConsoleVariable<int32> CVarTLLRN_ControlTLLRN_RigHierarchyTraceCallstack(TEXT("TLLRN_ControlRig.Hierarchy.TraceCallstack"), 0, TEXT("if nonzero we will record the callstack for any trace entry.\nOnly works if(TLLRN_ControlRig.Hierarchy.TraceEnabled != 0)"));
static TAutoConsoleVariable<int32> CVarTLLRN_ControlTLLRN_RigHierarchyTracePrecision(TEXT("TLLRN_ControlRig.Hierarchy.TracePrecision"), 3, TEXT("sets the number digits in a float when tracing hierarchies."));
static TAutoConsoleVariable<int32> CVarTLLRN_ControlTLLRN_RigHierarchyTraceOnSpawn(TEXT("TLLRN_ControlRig.Hierarchy.TraceOnSpawn"), 0, TEXT("sets the number of frames to trace when a new hierarchy is spawned"));
TAutoConsoleVariable<bool> CVarTLLRN_ControlTLLRN_RigHierarchyEnableRotationOrder(TEXT("TLLRN_ControlRig.Hierarchy.EnableRotationOrder"), true, TEXT("enables the rotation order for controls"));
TAutoConsoleVariable<bool> CVarTLLRN_ControlTLLRN_RigHierarchyEnableModules(TEXT("TLLRN_ControlRig.Hierarchy.Modules"), true, TEXT("enables the modular rigging functionality"));
TAutoConsoleVariable<bool> CVarTLLRN_ControlTLLRN_RigHierarchyEnablePhysics(TEXT("TLLRN_ControlRig.Hierarchy.Physics"), false, TEXT("enables physics support for the rig hierarchy"));
static int32 sTLLRN_RigHierarchyLastTrace = INDEX_NONE;
static TCHAR sTLLRN_RigHierarchyTraceFormat[16];

// A console command to trace a single frame / single execution for a control rig anim node / control rig component
FAutoConsoleCommandWithWorldAndArgs FCmdTLLRN_ControlTLLRN_RigHierarchyTraceFrames
(
	TEXT("TLLRN_ControlRig.Hierarchy.Trace"),
	TEXT("Traces changes in a hierarchy for a provided number of executions (defaults to 1).\nYou can use TLLRN_ControlRig.Hierarchy.TraceCallstack to enable callstack tracing as part of this."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InParams, UWorld* InWorld)
	{
		int32 NumFrames = 1;
		if(InParams.Num() > 0)
		{
			NumFrames = FCString::Atoi(*InParams[0]);
		}
		
		TArray<UObject*> Instances;
		UTLLRN_RigHierarchy::StaticClass()->GetDefaultObject()->GetArchetypeInstances(Instances);

		for(UObject* Instance : Instances)
		{
			if (Instance->HasAnyFlags(RF_ClassDefaultObject))
			{
				continue;
			}
			
			// we'll just trace all of them for now
			//if(Instance->GetWorld() == InWorld)
			if(Instance->GetTypedOuter<UTLLRN_ControlRig>() != nullptr)
			{
				CastChecked<UTLLRN_RigHierarchy>(Instance)->TraceFrames(NumFrames);
			}
		}
	})
);

#endif

////////////////////////////////////////////////////////////////////////////////
// UTLLRN_RigHierarchy
////////////////////////////////////////////////////////////////////////////////

#if URIGHIERARCHY_ENSURE_CACHE_VALIDITY
bool UTLLRN_RigHierarchy::bEnableValidityCheckbyDefault = true;
#else
bool UTLLRN_RigHierarchy::bEnableValidityCheckbyDefault = false;
#endif

UTLLRN_RigHierarchy::UTLLRN_RigHierarchy()
: TopologyVersion(0)
, MetadataVersion(0)
, MetadataTagVersion(0)
, bEnableDirtyPropagation(true)
, Elements()
, bRecordCurveChanges(true)
, IndexLookup()
, TransformStackIndex(0)
, bTransactingForTransformChange(false)
, bIsInteracting(false)
, LastInteractedKey()
, bSuspendNotifications(false)
, bSuspendMetadataNotifications(false)
, HierarchyController(nullptr)
, bIsControllerAvailable(true)
, ResetPoseHash(INDEX_NONE)
, bIsCopyingHierarchy(false)
#if WITH_EDITOR
, bPropagatingChange(false)
, bForcePropagation(false)
, TraceFramesLeft(0)
, TraceFramesCaptured(0)
#endif
, bEnableCacheValidityCheck(bEnableValidityCheckbyDefault)
, HierarchyForCacheValidation()
, bUsePreferredEulerAngles(true)
, bAllowNameSpaceWhenSanitizingName(false)
, ExecuteContext(nullptr)
#if WITH_EDITOR
, bRecordTransformsAtRuntime(true)
#endif
, ElementKeyRedirector(nullptr)
, ElementBeingDestroyed(nullptr)
{
	Reset();
#if WITH_EDITOR
	TraceFrames(CVarTLLRN_ControlTLLRN_RigHierarchyTraceOnSpawn->GetInt());
#endif
}

void UTLLRN_RigHierarchy::BeginDestroy()
{
	// reset needs to be in begin destroy since
	// reset touches a UObject member of this hierarchy,
	// which will be GCed by the time this hierarchy reaches destructor
	Reset();
	Super::BeginDestroy();
}

void UTLLRN_RigHierarchy::Serialize(FArchive& Ar)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	
	Ar.UsingCustomVersion(FAnimObjectVersion::GUID);
	Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);

	if (Ar.IsSaving() || Ar.IsObjectReferenceCollector() || Ar.IsCountingMemory())
	{
		Save(Ar);
	}
	else if (Ar.IsLoading())
	{
		Load(Ar);
	}
	else
	{
		// remove due to FPIEFixupSerializer hitting this checkNoEntry();
	}
}

void UTLLRN_RigHierarchy::AddReferencedObjects(UObject* InpThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InpThis, Collector);

	UTLLRN_RigHierarchy* pThis = static_cast<UTLLRN_RigHierarchy*>(InpThis);
	FScopeLock Lock(&pThis->ElementsLock);
	for (FTLLRN_RigBaseElement* Element : pThis->Elements)
	{
		Collector.AddPropertyReferencesWithStructARO(Element->GetElementStruct(), Element, pThis);
	}
}

void UTLLRN_RigHierarchy::Save(FArchive& Ar)
{
	FScopeLock Lock(&ElementsLock);
	
	if(Ar.IsTransacting())
	{
		Ar << TransformStackIndex;
		Ar << bTransactingForTransformChange;
		
		if(bTransactingForTransformChange)
		{
			return;
		}

		TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();
		Ar << SelectedKeys;
	}

	// make sure all parts of pose are valid.
	// this ensures cache validity.
	EnsureCacheValidity();

	ComputeAllTransforms();

	int32 ElementCount = Elements.Num();
	Ar << ElementCount;

	for(int32 ElementIndex = 0; ElementIndex < ElementCount; ElementIndex++)
	{
		FTLLRN_RigBaseElement* Element = Elements[ElementIndex];

		// store the key
		FTLLRN_RigElementKey Key = Element->GetKey();
		Ar << Key;
	}

	for(int32 ElementIndex = 0; ElementIndex < ElementCount; ElementIndex++)
	{
		FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
		Element->Serialize(Ar, FTLLRN_RigBaseElement::StaticData);
	}

	for(int32 ElementIndex = 0; ElementIndex < ElementCount; ElementIndex++)
	{
		FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
		Element->Serialize(Ar, FTLLRN_RigBaseElement::InterElementData);
	}

	Ar << PreviousNameMap;
	Ar << PreviousParentMap;

	{
		TMap<FTLLRN_RigElementKey, FMetadataStorage> ElementMetadataToSave;
		
		for (const FTLLRN_RigBaseElement* Element: Elements)
		{
			if (ElementMetadata.IsValidIndex(Element->MetadataStorageIndex))
			{
				ElementMetadataToSave.Add(Element->Key, ElementMetadata[Element->MetadataStorageIndex]);
			}
		}
		
		Ar << ElementMetadataToSave;
	}
}

void UTLLRN_RigHierarchy::Load(FArchive& Ar)
{
	FScopeLock Lock(&ElementsLock);
	
	// unlink any existing pose adapter
	UnlinkPoseAdapter();

	TArray<FTLLRN_RigElementKey> SelectedKeys;
	if(Ar.IsTransacting())
	{
		bool bOnlySerializedTransformStackIndex = false;
		Ar << TransformStackIndex;
		Ar << bOnlySerializedTransformStackIndex;
		
		if(bOnlySerializedTransformStackIndex)
		{
			return;
		}

		Ar << SelectedKeys;
	}

	// If a controller is found where the outer is this hierarchy, make sure it is configured correctly
	{
		TArray<UObject*> ChildObjects;
		GetObjectsWithOuter(this, ChildObjects, false);
		ChildObjects = ChildObjects.FilterByPredicate([](UObject* Object)
			{ return Object->IsA<UTLLRN_RigHierarchyController>();});
		if (!ChildObjects.IsEmpty())
		{
			ensure(ChildObjects.Num() == 1); // there should only be one controller
			bIsControllerAvailable = true;
			HierarchyController = Cast<UTLLRN_RigHierarchyController>(ChildObjects[0]);
			HierarchyController->SetHierarchy(this);
		}
	}
	
	Reset();

	int32 ElementCount = 0;
	Ar << ElementCount;

	PoseVersionPerElement.Reset();

	int32 NumTransforms = 0;
	int32 NumDirtyStates = 0;
	int32 NumCurves = 0;

	const bool bAllocateStoragePerElement = Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::TLLRN_RigHierarchyIndirectElementStorage;
	for(int32 ElementIndex = 0; ElementIndex < ElementCount; ElementIndex++)
	{
		FTLLRN_RigElementKey Key;
		Ar << Key;

		FTLLRN_RigBaseElement* Element = MakeElement(Key.Type);
		check(Element);

		Element->SubIndex = Num(Key.Type);
		Element->Index = Elements.Add(Element);
		ElementsPerType[TLLRN_RigElementTypeToFlatIndex(Key.Type)].Add(Element);
		IndexLookup.Add(Key, Element->Index);

		if(bAllocateStoragePerElement)
		{
			AllocateDefaultElementStorage(Element, false);
			Element->Load(Ar, FTLLRN_RigBaseElement::StaticData);
		}
		else
		{
			NumTransforms += Element->GetNumTransforms();
			NumDirtyStates += Element->GetNumTransforms();
			NumCurves += Element->GetNumCurves();
		}
	}

	if(bAllocateStoragePerElement)
	{
		// update all storage pointers now that we've created
		// all elements.
		UpdateElementStorage();
	}
	else // if we can allocate the storage as one big buffer...
	{
		const TArray<int32, TInlineAllocator<4>> TransformIndices = ElementTransforms.Allocate(NumTransforms, FTransform::Identity);
		const TArray<int32, TInlineAllocator<4>> DirtyStateIndices = ElementDirtyStates.Allocate(NumDirtyStates, false);
		const TArray<int32, TInlineAllocator<4>> CurveIndices = ElementCurves.Allocate(NumCurves, 0.f);
		int32 UsedTransformIndex = 0;
		int32 UsedDirtyStateIndex = 0;
		int32 UsedCurveIndex = 0;

		ElementTransforms.Shrink();
		ElementDirtyStates.Shrink();
		ElementCurves.Shrink();

		for(int32 ElementIndex = 0; ElementIndex < ElementCount; ElementIndex++)
		{
			FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
			
			if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Element))
			{
				TransformElement->PoseStorage.Initial.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
				TransformElement->PoseStorage.Current.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
				TransformElement->PoseStorage.Initial.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
				TransformElement->PoseStorage.Current.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
				TransformElement->PoseDirtyState.Initial.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
				TransformElement->PoseDirtyState.Current.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
				TransformElement->PoseDirtyState.Initial.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
				TransformElement->PoseDirtyState.Current.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];

				if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(TransformElement))
				{
					ControlElement->OffsetStorage.Initial.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->OffsetStorage.Current.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->OffsetStorage.Initial.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->OffsetStorage.Current.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->OffsetDirtyState.Initial.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->OffsetDirtyState.Current.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->OffsetDirtyState.Initial.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->OffsetDirtyState.Current.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->ShapeStorage.Initial.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->ShapeStorage.Current.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->ShapeStorage.Initial.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->ShapeStorage.Current.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->ShapeDirtyState.Initial.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->ShapeDirtyState.Current.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->ShapeDirtyState.Initial.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->ShapeDirtyState.Current.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
				}
			}
			else if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(Element))
			{
				CurveElement->StorageIndex = CurveIndices[UsedCurveIndex++];
			}

			Element->LinkStorage(ElementTransforms.Storage, ElementDirtyStates.Storage, ElementCurves.Storage);
			Element->Load(Ar, FTLLRN_RigBaseElement::StaticData);
		}
	}
	IncrementTopologyVersion();

	for(int32 ElementIndex = 0; ElementIndex < ElementCount; ElementIndex++)
	{
		FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
		Element->Load(Ar, FTLLRN_RigBaseElement::InterElementData);
	}

	IncrementTopologyVersion();

	for(int32 ElementIndex = 0; ElementIndex < ElementCount; ElementIndex++)
	{
		if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Elements[ElementIndex]))
		{
			FTLLRN_RigBaseElementParentArray CurrentParents = GetParents(TransformElement, false);
			for (FTLLRN_RigBaseElement* CurrentParent : CurrentParents)
			{
				if(FTLLRN_RigTransformElement* TransformParent = Cast<FTLLRN_RigTransformElement>(CurrentParent))
				{
					TransformParent->ElementsToDirty.AddUnique(TransformElement);
				}
			}
		}
	}

	if(Ar.IsTransacting())
	{
		for(const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
		{
			if(FTLLRN_RigBaseElement* Element = Find<FTLLRN_RigBaseElement>(SelectedKey))
			{
				Element->bSelected = true;
				OrderedSelection.Add(SelectedKey);
			}
		}
	}

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::TLLRN_RigHierarchyStoringPreviousNames)
	{
		Ar << PreviousNameMap;
		Ar << PreviousParentMap;
	}
	else
	{
		PreviousNameMap.Reset();
		PreviousParentMap.Reset();
	}

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::TLLRN_RigHierarchyStoresElementMetadata)
	{
		ElementMetadata.Reset();
		TMap<FTLLRN_RigElementKey, FMetadataStorage> LoadedElementMetadata;
		
		Ar << LoadedElementMetadata;
		for (TPair<FTLLRN_RigElementKey, FMetadataStorage>& Entry: LoadedElementMetadata)
		{
			FTLLRN_RigBaseElement* Element = Find(Entry.Key);
			Element->MetadataStorageIndex = ElementMetadata.Num();
			ElementMetadata.Storage.Add(MoveTemp(Entry.Value));
		}
	}

	(void)SortElementStorage();
}

void UTLLRN_RigHierarchy::PostLoad()
{
	UObject::PostLoad();

	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	CleanupInvalidCaches();

	Notify(ETLLRN_RigHierarchyNotification::HierarchyReset, nullptr);
}

#if WITH_EDITORONLY_DATA
void UTLLRN_RigHierarchy::DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass)
{
	Super::DeclareConstructClasses(OutConstructClasses, SpecificSubclass);
	OutConstructClasses.Add(FTopLevelAssetPath(UTLLRN_RigHierarchyController::StaticClass()));
}
#endif

void UTLLRN_RigHierarchy::Reset()
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	Reset_Impl(true);
}

void UTLLRN_RigHierarchy::Reset_Impl(bool bResetElements)
{
	TopologyVersion = 0;
	MetadataVersion = 0;
	bEnableDirtyPropagation = true;

	if(bResetElements)
	{
		FScopeLock Lock(&ElementsLock);
		
		// walk in reverse since certain elements might not have been allocated themselves
		for(int32 ElementIndex = Elements.Num() - 1; ElementIndex >= 0; ElementIndex--)
		{
			DestroyElement(Elements[ElementIndex]);
		}
		Elements.Reset();
		ElementsPerType.Reset();
		const UEnum* ElementTypeEnum = StaticEnum<ETLLRN_RigElementType>();
		for(int32 TypeIndex=0; ;TypeIndex++)
		{
			if (static_cast<ETLLRN_RigElementType>(ElementTypeEnum->GetValueByIndex(TypeIndex)) == ETLLRN_RigElementType::All)
			{
				break;
			}
			ElementsPerType.Add(TArray<FTLLRN_RigBaseElement*>());
		}
		IndexLookup.Reset();

		ElementTransforms.Reset();
		ElementDirtyStates.Reset();
		ElementCurves.Reset();

		ElementMetadata.Reset([](int32 InIndex, FMetadataStorage& MetadataStorage)
		{
			for (TTuple<FName, FTLLRN_RigBaseMetadata*>& Item: MetadataStorage.MetadataMap)
			{
				FTLLRN_RigBaseMetadata::DestroyMetadata(&Item.Value);
			} 
		});
	}

	ResetPoseHash = INDEX_NONE;
	ResetPoseIsFilteredOut.Reset();
	ElementsToRetainLocalTransform.Reset();
	DefaultParentPerElement.Reset();
	OrderedSelection.Reset();
	PoseVersionPerElement.Reset();
	ElementDependencyCache.Reset();
	ResetChangedCurveIndices();

	ChildElementOffsetAndCountCache.Reset();
	ChildElementCache.Reset();
	ChildElementCacheTopologyVersion = std::numeric_limits<uint32>::max();


	{
		FGCScopeGuard Guard;
		Notify(ETLLRN_RigHierarchyNotification::HierarchyReset, nullptr);
	}

	if(HierarchyForCacheValidation)
	{
		HierarchyForCacheValidation->Reset();
	}
}

#if WITH_EDITOR

void UTLLRN_RigHierarchy::ForEachListeningHierarchy(TFunctionRef<void(const FTLLRN_RigHierarchyListener&)> PerListeningHierarchyFunction)
{
	for(int32 Index = 0; Index < ListeningHierarchies.Num(); Index++)
	{
		PerListeningHierarchyFunction(ListeningHierarchies[Index]);
	}
}

#endif

void UTLLRN_RigHierarchy::ResetToDefault()
{
	FScopeLock Lock(&ElementsLock);
	
	if(DefaultHierarchyPtr.IsValid())
	{
		if(UTLLRN_RigHierarchy* DefaultHierarchy = DefaultHierarchyPtr.Get())
		{
			CopyHierarchy(DefaultHierarchy);
			return;
		}
	}
	Reset();
}

void UTLLRN_RigHierarchy::CopyHierarchy(UTLLRN_RigHierarchy* InHierarchy)
{
	check(InHierarchy);

	const TGuardValue<bool> MarkCopyingHierarchy(bIsCopyingHierarchy, true);
	
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	FScopeLock Lock(&ElementsLock);
	if(Elements.Num() == 0 && InHierarchy->Elements.Num () == 0)
	{
		return;
	}

	// unlink any existing pose adapter
	UnlinkPoseAdapter();

	// check if we really need to do a deep copy all over again.
	// for rigs which contain more elements (likely procedural elements)
	// we'll assume we can just remove superfluous elements (from the end of the lists).
	bool bReallocateElements = Elements.Num() < InHierarchy->Elements.Num();
	if(!bReallocateElements)
	{
		const UEnum* ElementTypeEnum = StaticEnum<ETLLRN_RigElementType>();
		for(int32 ElementTypeIndex = 0; ; ElementTypeIndex++)
		{
			if (static_cast<ETLLRN_RigElementType>(ElementTypeEnum->GetValueByIndex(ElementTypeIndex)) == ETLLRN_RigElementType::All)
			{
				break;
			}
			check(ElementsPerType.IsValidIndex(ElementTypeIndex));
			check(InHierarchy->ElementsPerType.IsValidIndex(ElementTypeIndex));
			if(ElementsPerType[ElementTypeIndex].Num() != InHierarchy->ElementsPerType[ElementTypeIndex].Num())
			{
				bReallocateElements = true;
				break;
			}
		}

		// make sure that we have the elements in the right order / type
		if(!bReallocateElements)
		{
			for(int32 Index = 0; Index < InHierarchy->Elements.Num(); Index++)
			{
				if((Elements[Index]->GetKey().Type != InHierarchy->Elements[Index]->GetKey().Type) ||
					(Elements[Index]->SubIndex != InHierarchy->Elements[Index]->SubIndex))
				{
					bReallocateElements = true;
					break;
				}
			}
		}
	}

	{
		TGuardValue<bool> SuspendMetadataNotifications(bSuspendMetadataNotifications, true);
		Reset_Impl(bReallocateElements);

		static const TArray<int32> StructureSizePerType = {
			sizeof(FTLLRN_RigBoneElement),
			sizeof(FTLLRN_RigNullElement),
			sizeof(FTLLRN_RigControlElement),
			sizeof(FTLLRN_RigCurveElement),
			sizeof(FTLLRN_RigPhysicsElement),
			sizeof(FTLLRN_RigReferenceElement),
			sizeof(FTLLRN_RigConnectorElement),
			sizeof(FTLLRN_RigSocketElement),
		};

		int32 NumTransforms = 0;
		int32 NumDirtyStates = 0;
		int32 NumCurves = 0;

		if(bReallocateElements)
		{
			// Allocate the elements in batches to improve performance
			TArray<uint8*> NewElementsPerType;
			for(int32 ElementTypeIndex = 0; ElementTypeIndex < InHierarchy->ElementsPerType.Num(); ElementTypeIndex++)
			{
				const ETLLRN_RigElementType ElementType = FlatIndexToTLLRN_RigElementType(ElementTypeIndex);
				int32 StructureSize = 0;

				const int32 Count = InHierarchy->ElementsPerType[ElementTypeIndex].Num();
				if(Count)
				{
					FTLLRN_RigBaseElement* ElementMemory = MakeElement(ElementType, Count, &StructureSize);
					verify(StructureSize == StructureSizePerType[ElementTypeIndex]);
					NewElementsPerType.Add(reinterpret_cast<uint8*>(ElementMemory));
				}
				else
				{
					NewElementsPerType.Add(nullptr);
				}
			
				ElementsPerType[ElementTypeIndex].Reserve(Count);
			}

			Elements.Reserve(InHierarchy->Elements.Num());
			IndexLookup.Reserve(InHierarchy->IndexLookup.Num());

			for(int32 Index = 0; Index < InHierarchy->Num(); Index++)
			{
				const FTLLRN_RigBaseElement* Source = InHierarchy->Get(Index);
				const FTLLRN_RigElementKey& Key = Source->Key;

				const int32 ElementTypeIndex = TLLRN_RigElementTypeToFlatIndex(Key.Type);
		
				const int32 SubIndex = Num(Key.Type);

				const int32 StructureSize = StructureSizePerType[ElementTypeIndex];
				check(NewElementsPerType[ElementTypeIndex] != nullptr);
				FTLLRN_RigBaseElement* Target = reinterpret_cast<FTLLRN_RigBaseElement*>(&NewElementsPerType[ElementTypeIndex][StructureSize * SubIndex]);

				Target->InitializeFrom(Source);

				NumTransforms += Target->GetNumTransforms();
				NumDirtyStates += Target->GetNumTransforms();
				NumCurves += Target->GetNumCurves();

				Target->SubIndex = SubIndex;
				Target->Index = Elements.Add(Target);

				ElementsPerType[ElementTypeIndex].Add(Target);
				IndexLookup.Add(Key, Target->Index);
			
				IncrementPoseVersion(Index);

				check(Source->Index == Index);
				check(Target->Index == Index);
			}
		}
		else
		{
			// remove the superfluous elements
			for(int32 ElementIndex = Elements.Num() - 1; ElementIndex >= InHierarchy->Elements.Num(); ElementIndex--)
			{
				DestroyElement(Elements[ElementIndex]);
			}

			// shrink the containers accordingly
			Elements.SetNum(InHierarchy->Elements.Num());
			const UEnum* ElementTypeEnum = StaticEnum<ETLLRN_RigElementType>();
			for(int32 ElementTypeIndex = 0; ; ElementTypeIndex++)
			{
				if ((ETLLRN_RigElementType)ElementTypeEnum->GetValueByIndex(ElementTypeIndex) == ETLLRN_RigElementType::All)
				{
					break;
				}
				ElementsPerType[ElementTypeIndex].SetNum(InHierarchy->ElementsPerType[ElementTypeIndex].Num());
			}

			for(int32 Index = 0; Index < InHierarchy->Num(); Index++)
			{
				const FTLLRN_RigBaseElement* Source = InHierarchy->Get(Index);
				FTLLRN_RigBaseElement* Target = Elements[Index];

				check(Target->Key.Type == Source->Key.Type);
				Target->InitializeFrom(Source);

				NumTransforms += Target->GetNumTransforms();
				NumDirtyStates += Target->GetNumTransforms();
				NumCurves += Target->GetNumCurves();

				IncrementPoseVersion(Index);
			}

			IndexLookup = InHierarchy->IndexLookup;
		}

		ElementTransforms.Reset();
		ElementDirtyStates.Reset();
		ElementCurves.Reset();
		
		const TArray<int32, TInlineAllocator<4>> TransformIndices = ElementTransforms.Allocate(NumTransforms, FTransform::Identity);
		const TArray<int32, TInlineAllocator<4>> DirtyStateIndices = ElementDirtyStates.Allocate(NumDirtyStates, false);
		const TArray<int32, TInlineAllocator<4>> CurveIndices = ElementCurves.Allocate(NumCurves, 0.f);
		int32 UsedTransformIndex = 0;
		int32 UsedDirtyStateIndex = 0;
		int32 UsedCurveIndex = 0;

		ElementTransforms.Shrink();
		ElementDirtyStates.Shrink();
		ElementCurves.Shrink();

		// Copy all the element subclass data and all elements' metadata over.
		for(int32 Index = 0; Index < InHierarchy->Num(); Index++)
		{
			const FTLLRN_RigBaseElement* Source = InHierarchy->Get(Index);
			FTLLRN_RigBaseElement* Target = Elements[Index];

			if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Target))
			{
				TransformElement->PoseStorage.Initial.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
				TransformElement->PoseStorage.Current.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
				TransformElement->PoseStorage.Initial.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
				TransformElement->PoseStorage.Current.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
				TransformElement->PoseDirtyState.Initial.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
				TransformElement->PoseDirtyState.Current.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
				TransformElement->PoseDirtyState.Initial.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
				TransformElement->PoseDirtyState.Current.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];

				if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(TransformElement))
				{
					ControlElement->OffsetStorage.Initial.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->OffsetStorage.Current.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->OffsetStorage.Initial.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->OffsetStorage.Current.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->OffsetDirtyState.Initial.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->OffsetDirtyState.Current.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->OffsetDirtyState.Initial.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->OffsetDirtyState.Current.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->ShapeStorage.Initial.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->ShapeStorage.Current.Local.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->ShapeStorage.Initial.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->ShapeStorage.Current.Global.StorageIndex = TransformIndices[UsedTransformIndex++];
					ControlElement->ShapeDirtyState.Initial.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->ShapeDirtyState.Current.Local.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->ShapeDirtyState.Initial.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
					ControlElement->ShapeDirtyState.Current.Global.StorageIndex = DirtyStateIndices[UsedDirtyStateIndex++];
				}
			}
			else if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(Target))
			{
				CurveElement->StorageIndex = CurveIndices[UsedCurveIndex++];
			}
			Target->LinkStorage(ElementTransforms.Storage, ElementDirtyStates.Storage, ElementCurves.Storage);

			Target->CopyFrom(Source);

			CopyAllMetadataFromElement(Target, Source);
		}

		(void)SortElementStorage();

		PreviousNameMap.Append(InHierarchy->PreviousNameMap);

		// make sure the curves are marked as unset after copying
		UnsetCurveValues();

		// copy the topology version from the hierarchy.
		// for this we'll include the hash of the hierarchy to make sure we
		// are deterministic for different hierarchies.
		TopologyVersion = HashCombine(InHierarchy->GetTopologyVersion(), InHierarchy->GetTopologyHash());

		// Increment the topology version to invalidate our cached children.
		IncrementTopologyVersion();

		// Keep incrementing the metadata version so that the UI can refresh.
		MetadataVersion += InHierarchy->GetMetadataVersion();
		MetadataTagVersion += InHierarchy->GetMetadataTagVersion();
	}

	if (MetadataChangedDelegate.IsBound())
	{
		MetadataChangedDelegate.Broadcast(FTLLRN_RigElementKey(ETLLRN_RigElementType::All), NAME_None);
	}

	EnsureCacheValidity();

	bIsCopyingHierarchy = false;
	Notify(ETLLRN_RigHierarchyNotification::HierarchyCopied, nullptr);
}

uint32 UTLLRN_RigHierarchy::GetNameHash() const
{
	FScopeLock Lock(&ElementsLock);
	
	uint32 Hash = GetTypeHash(GetTopologyVersion());
	for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
	{
		const FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
		Hash = HashCombine(Hash, GetTypeHash(Element->GetFName()));
	}
	return Hash;	
}

uint32 UTLLRN_RigHierarchy::GetTopologyHash(bool bIncludeTopologyVersion, bool bIncludeTransientControls) const
{
	FScopeLock Lock(&ElementsLock);
	
	uint32 Hash = bIncludeTopologyVersion ? TopologyVersion : 0;

	for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
	{
		const FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
		
		// skip transient controls
		if(!bIncludeTransientControls)
		{
			if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
			{
				if(ControlElement->Settings.bIsTransientControl)
				{
					continue;
				}
			}
		}
		
		Hash = HashCombine(Hash, GetTypeHash(Element->GetKey()));

		if(const FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(Element))
		{
			if(SingleParentElement->ParentElement)
			{
				Hash = HashCombine(Hash, GetTypeHash(SingleParentElement->ParentElement->GetKey()));
			}
		}
		if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(Element))
		{
			for(const FTLLRN_RigElementParentConstraint& ParentConstraint : MultiParentElement->ParentConstraints)
			{
				Hash = HashCombine(Hash, GetTypeHash(ParentConstraint.ParentElement->GetKey()));
			}
		}
		if(const FTLLRN_RigBoneElement* BoneElement = Cast<FTLLRN_RigBoneElement>(Element))
		{
			Hash = HashCombine(Hash, GetTypeHash(BoneElement->BoneType));
		}
		if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
		{
			Hash = HashCombine(Hash, GetTypeHash(ControlElement->Settings));
		}
		if(const FTLLRN_RigConnectorElement* ConnectorElement = Cast<FTLLRN_RigConnectorElement>(Element))
		{
			Hash = HashCombine(Hash, GetTypeHash(ConnectorElement->Settings));
		}
	}

	return Hash;
}

#if WITH_EDITOR
void UTLLRN_RigHierarchy::RegisterListeningHierarchy(UTLLRN_RigHierarchy* InHierarchy)
{
	if (InHierarchy)
	{
		bool bFoundListener = false;
		for(int32 ListenerIndex = ListeningHierarchies.Num() - 1; ListenerIndex >= 0; ListenerIndex--)
		{
			const UTLLRN_RigHierarchy::FTLLRN_RigHierarchyListener& Listener = ListeningHierarchies[ListenerIndex];
			if(Listener.Hierarchy.IsValid())
			{
				if(Listener.Hierarchy.Get() == InHierarchy)
				{
					bFoundListener = true;
					break;
				}
			}
		}

		if(!bFoundListener)
		{
			UTLLRN_RigHierarchy::FTLLRN_RigHierarchyListener Listener;
			Listener.Hierarchy = InHierarchy; 
			ListeningHierarchies.Add(Listener);
		}
	}
}

void UTLLRN_RigHierarchy::UnregisterListeningHierarchy(UTLLRN_RigHierarchy* InHierarchy)
{
	if (InHierarchy)
	{
		for(int32 ListenerIndex = ListeningHierarchies.Num() - 1; ListenerIndex >= 0; ListenerIndex--)
		{
			const UTLLRN_RigHierarchy::FTLLRN_RigHierarchyListener& Listener = ListeningHierarchies[ListenerIndex];
			if(Listener.Hierarchy.IsValid())
			{
				if(Listener.Hierarchy.Get() == InHierarchy)
				{
					ListeningHierarchies.RemoveAt(ListenerIndex);
				}
			}
		}
	}
}

void UTLLRN_RigHierarchy::ClearListeningHierarchy()
{
	ListeningHierarchies.Reset();
}
#endif

void UTLLRN_RigHierarchy::CopyPose(UTLLRN_RigHierarchy* InHierarchy, bool bCurrent, bool bInitial, bool bWeights, bool bMatchPoseInGlobalIfNeeded)
{
	check(InHierarchy);
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	// if we need to copy the weights but the hierarchies are topologically
	// different we need to reset the topology. this is expensive and should
	// only happen during construction of the hierarchy itself.
	if(bWeights && (GetTopologyVersion() != InHierarchy->GetTopologyVersion()))
	{
		CopyHierarchy(InHierarchy);
	}

	const bool bPerformTopologyCheck = GetTopologyVersion() != InHierarchy->GetTopologyVersion();
	for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
	{
		FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
		if(FTLLRN_RigBaseElement* OtherElement = InHierarchy->Find(Element->GetKey()))
		{
			Element->CopyPose(OtherElement, bCurrent, bInitial, bWeights);
			IncrementPoseVersion(Element->Index);

			// if the topologies don't match and we are supposed to match
			// elements in global space...
			if(bMatchPoseInGlobalIfNeeded && bPerformTopologyCheck)
			{
				FTLLRN_RigMultiParentElement* MultiParentElementA = Cast<FTLLRN_RigMultiParentElement>(Element);
				FTLLRN_RigMultiParentElement* MultiParentElementB = Cast<FTLLRN_RigMultiParentElement>(OtherElement);
				if(MultiParentElementA && MultiParentElementB)
				{
					if(MultiParentElementA->ParentConstraints.Num() != MultiParentElementB->ParentConstraints.Num())
					{
						FTLLRN_RigControlElement* ControlElementA = Cast<FTLLRN_RigControlElement>(Element);
						FTLLRN_RigControlElement* ControlElementB = Cast<FTLLRN_RigControlElement>(OtherElement);
						if(ControlElementA && ControlElementB)
						{
							if(bCurrent)
							{
								ControlElementA->GetOffsetTransform().Set(ETLLRN_RigTransformType::CurrentGlobal, InHierarchy->GetControlOffsetTransform(ControlElementB, ETLLRN_RigTransformType::CurrentGlobal));
								ControlElementA->GetOffsetDirtyState().MarkClean(ETLLRN_RigTransformType::CurrentGlobal);
								ControlElementA->GetOffsetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentLocal);
								ControlElementA->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);
								ControlElementA->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);
								IncrementPoseVersion(ControlElementA->Index);
							}
							if(bInitial)
							{
								ControlElementA->GetOffsetTransform().Set(ETLLRN_RigTransformType::InitialGlobal, InHierarchy->GetControlOffsetTransform(ControlElementB, ETLLRN_RigTransformType::InitialGlobal));
								ControlElementA->GetOffsetDirtyState().MarkClean(ETLLRN_RigTransformType::InitialGlobal);
								ControlElementA->GetOffsetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialLocal);
								ControlElementA->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
								ControlElementA->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
								IncrementPoseVersion(ControlElementA->Index);
							}
						}
						else
						{
							if(bCurrent)
							{
								MultiParentElementA->GetTransform().Set(ETLLRN_RigTransformType::CurrentGlobal, InHierarchy->GetTransform(MultiParentElementB, ETLLRN_RigTransformType::CurrentGlobal));
								MultiParentElementA->GetDirtyState().MarkClean(ETLLRN_RigTransformType::CurrentGlobal);
								MultiParentElementA->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentLocal);
								IncrementPoseVersion(MultiParentElementA->Index);
							}
							if(bInitial)
							{
								MultiParentElementA->GetTransform().Set(ETLLRN_RigTransformType::InitialGlobal, InHierarchy->GetTransform(MultiParentElementB, ETLLRN_RigTransformType::InitialGlobal));
								MultiParentElementA->GetDirtyState().MarkClean(ETLLRN_RigTransformType::InitialGlobal);
								MultiParentElementA->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialLocal);
								IncrementPoseVersion(MultiParentElementA->Index);
							}
						}
					}
				}
			}
		}
	}

	EnsureCacheValidity();
}

void UTLLRN_RigHierarchy::UpdateReferences(const FRigVMExecuteContext* InContext)
{
	check(InContext);
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	
	for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
	{
		if(FTLLRN_RigReferenceElement* Reference = Cast<FTLLRN_RigReferenceElement>(Elements[ElementIndex]))
		{
			const FTransform InitialWorldTransform = Reference->GetReferenceWorldTransform(InContext, true);
			const FTransform CurrentWorldTransform = Reference->GetReferenceWorldTransform(InContext, false);

			const FTransform InitialGlobalTransform = InitialWorldTransform.GetRelativeTransform(InContext->GetToWorldSpaceTransform());
			const FTransform CurrentGlobalTransform = CurrentWorldTransform.GetRelativeTransform(InContext->GetToWorldSpaceTransform());

			const FTransform InitialParentTransform = GetParentTransform(Reference, ETLLRN_RigTransformType::InitialGlobal); 
			const FTransform CurrentParentTransform = GetParentTransform(Reference, ETLLRN_RigTransformType::CurrentGlobal);

			const FTransform InitialLocalTransform = InitialGlobalTransform.GetRelativeTransform(InitialParentTransform);
			const FTransform CurrentLocalTransform = CurrentGlobalTransform.GetRelativeTransform(CurrentParentTransform);

			SetTransform(Reference, InitialLocalTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
			SetTransform(Reference, CurrentLocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, false);
		}
	}
}

void UTLLRN_RigHierarchy::ResetPoseToInitial(ETLLRN_RigElementType InTypeFilter)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	bool bPerformFiltering = InTypeFilter != ETLLRN_RigElementType::All;
	
	FScopeLock Lock(&ElementsLock);
	const TGuardValue<bool> DisableRecordingOfCurveChanges(bRecordCurveChanges, false);
	
	// if we are resetting the pose on some elements, we need to check if
	// any of affected elements has any children that would not be affected
	// by resetting the pose. if all children are affected we can use the
	// fast path.
	if(bPerformFiltering)
	{
		const int32 Hash = HashCombine(GetTopologyVersion(), (int32)InTypeFilter);
		if(Hash != ResetPoseHash)
		{
			ResetPoseIsFilteredOut.Reset();
			ElementsToRetainLocalTransform.Reset();
			ResetPoseHash = Hash;

			// let's look at all elements and mark all parent of unaffected children
			ResetPoseIsFilteredOut.AddZeroed(Elements.Num());

			Traverse([this, InTypeFilter](FTLLRN_RigBaseElement* InElement, bool& bContinue)
			{
				bContinue = true;
				ResetPoseIsFilteredOut[InElement->GetIndex()] = !InElement->IsTypeOf(InTypeFilter);

				// make sure to distribute the filtering options from
				// the parent to the children of the part of the tree
				const FTLLRN_RigBaseElementParentArray Parents = GetParents(InElement);
				for(const FTLLRN_RigBaseElement* Parent : Parents)
				{
					if(!ResetPoseIsFilteredOut[Parent->GetIndex()])
					{
						if(InElement->IsA<FTLLRN_RigNullElement>() || InElement->IsA<FTLLRN_RigControlElement>())
						{
							ElementsToRetainLocalTransform.Add(InElement->GetIndex());
						}
						else
						{
							ResetPoseIsFilteredOut[InElement->GetIndex()] = false;
						}
					}
				}
			});
		}

		// if the per element state is empty
		// it means that the filter doesn't affect 
		if(ResetPoseIsFilteredOut.IsEmpty())
		{
			bPerformFiltering = false;
		}
	}

	if(bPerformFiltering)
	{
		for(const int32 ElementIndex : ElementsToRetainLocalTransform)
		{
			if(FTLLRN_RigTransformElement* TransformElement = Get<FTLLRN_RigTransformElement>(ElementIndex))
			{
				// compute the local value if necessary
				GetTransform(TransformElement, ETLLRN_RigTransformType::CurrentLocal);

				if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(TransformElement))
				{
					// compute the local offset if necessary
					GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
					GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
				}

				PropagateDirtyFlags(TransformElement, false, true, true, false);
			}
		}
		
		for(const int32 ElementIndex : ElementsToRetainLocalTransform)
		{
			if(FTLLRN_RigTransformElement* TransformElement = Get<FTLLRN_RigTransformElement>(ElementIndex))
			{
				if(TransformElement->GetDirtyState().IsDirty(ETLLRN_RigTransformType::CurrentGlobal))
				{
					continue;
				}
				
				TransformElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);

				if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(TransformElement))
				{
					ControlElement->GetOffsetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);
					ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);
				}

				PropagateDirtyFlags(TransformElement, false, true, false, true);
			}
		}
	}

	for(int32 ElementIndex=0; ElementIndex<Elements.Num(); ElementIndex++)
	{
		if(!ResetPoseIsFilteredOut.IsEmpty() && bPerformFiltering)
		{
			if(ResetPoseIsFilteredOut[ElementIndex])
			{
				continue;
			}
		}

		// reset the weights to the initial values as well
		if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(Elements[ElementIndex]))
		{
			for(FTLLRN_RigElementParentConstraint& ParentConstraint : MultiParentElement->ParentConstraints)
			{
				ParentConstraint.Weight = ParentConstraint.InitialWeight;
			}
		}

		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[ElementIndex]))
		{
			ControlElement->GetOffsetTransform().Current = ControlElement->GetOffsetTransform().Initial;
			ControlElement->GetOffsetDirtyState().Current = ControlElement->GetOffsetDirtyState().Initial;
			ControlElement->GetShapeTransform().Current = ControlElement->GetShapeTransform().Initial;
			ControlElement->GetShapeDirtyState().Current = ControlElement->GetShapeDirtyState().Initial;
			ControlElement->PreferredEulerAngles.Current = ControlElement->PreferredEulerAngles.Initial;
		}

		if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Elements[ElementIndex]))
		{
			TransformElement->GetTransform().Current = TransformElement->GetTransform().Initial;
			TransformElement->GetDirtyState().Current = TransformElement->GetDirtyState().Initial;
		}
	}
	
	EnsureCacheValidity();
}

void UTLLRN_RigHierarchy::ResetCurveValues()
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	const TGuardValue<bool> DisableRecordingOfCurveChanges(bRecordCurveChanges, false);

	TArray<FTLLRN_RigBaseElement*> Curves = GetCurvesFast();
	for(FTLLRN_RigBaseElement* Element : Curves)
	{
		if(FTLLRN_RigCurveElement* CurveElement = CastChecked<FTLLRN_RigCurveElement>(Element))
		{
			SetCurveValue(CurveElement, 0.f);
		}
	}
}

void UTLLRN_RigHierarchy::UnsetCurveValues(bool bSetupUndo)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	TArray<FTLLRN_RigBaseElement*> Curves = GetCurvesFast();
	for(FTLLRN_RigBaseElement* Element : Curves)
	{
		if(FTLLRN_RigCurveElement* CurveElement = CastChecked<FTLLRN_RigCurveElement>(Element))
		{
			UnsetCurveValue(CurveElement, bSetupUndo);
		}
	}
	ResetChangedCurveIndices();
}

const TArray<int32>& UTLLRN_RigHierarchy::GetChangedCurveIndices() const
{
	return ChangedCurveIndices;
}

void UTLLRN_RigHierarchy::ResetChangedCurveIndices()
{
	ChangedCurveIndices.Reset();
}

int32 UTLLRN_RigHierarchy::Num(ETLLRN_RigElementType InElementType) const
{
	return ElementsPerType[TLLRN_RigElementTypeToFlatIndex(InElementType)].Num();
}

bool UTLLRN_RigHierarchy::IsProcedural(const FTLLRN_RigElementKey& InKey) const
{
	return IsProcedural(Find(InKey));
}

bool UTLLRN_RigHierarchy::IsProcedural(const FTLLRN_RigBaseElement* InElement) const
{
	if(InElement == nullptr)
	{
		return false;
	}
	return InElement->IsProcedural();
}

TArray<FTLLRN_RigSocketState> UTLLRN_RigHierarchy::GetSocketStates() const
{
	const TArray<FTLLRN_RigElementKey> Keys = GetSocketKeys(true);
	TArray<FTLLRN_RigSocketState> States;
	States.Reserve(Keys.Num());
	for(const FTLLRN_RigElementKey& Key : Keys)
	{
		const FTLLRN_RigSocketElement* Socket = FindChecked<FTLLRN_RigSocketElement>(Key);
		if(!Socket->IsProcedural())
		{
			States.Add(Socket->GetSocketState(this));
		}
	}
	return States;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchy::RestoreSocketsFromStates(TArray<FTLLRN_RigSocketState> InStates, bool bSetupUndoRedo)
{
	TArray<FTLLRN_RigElementKey> Keys;
	for(const FTLLRN_RigSocketState& State : InStates)
	{
		FTLLRN_RigElementKey Key(State.Name, ETLLRN_RigElementType::Socket);

		if(FTLLRN_RigSocketElement* Socket = Find<FTLLRN_RigSocketElement>(Key))
		{
			(void)GetController()->SetParent(Key, State.Parent);
			Socket->SetColor(State.Color, this);
			Socket->SetDescription(State.Description, this);
			SetInitialLocalTransform(Key, State.InitialLocalTransform);
			SetLocalTransform(Key, State.InitialLocalTransform);
		}
		else
		{
			Key = GetController()->AddSocket(State.Name, State.Parent, State.InitialLocalTransform, false, State.Color, State.Description, bSetupUndoRedo, false);
		}

		Keys.Add(Key);
	}
	return Keys;
}

TArray<FTLLRN_RigConnectorState> UTLLRN_RigHierarchy::GetConnectorStates() const
{
	const TArray<FTLLRN_RigElementKey> Keys = GetConnectorKeys(true);
	TArray<FTLLRN_RigConnectorState> States;
	States.Reserve(Keys.Num());
	for(const FTLLRN_RigElementKey& Key : Keys)
	{
		const FTLLRN_RigConnectorElement* Connector = FindChecked<FTLLRN_RigConnectorElement>(Key);
		if(!Connector->IsProcedural())
		{
			States.Add(Connector->GetConnectorState(this));
		}
	}
	return States;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchy::RestoreConnectorsFromStates(TArray<FTLLRN_RigConnectorState> InStates, bool bSetupUndoRedo)
{
	TArray<FTLLRN_RigElementKey> Keys;
	for(const FTLLRN_RigConnectorState& State : InStates)
	{
		FTLLRN_RigElementKey Key(State.Name, ETLLRN_RigElementType::Connector);

		if(const FTLLRN_RigConnectorElement* Connector = Find<FTLLRN_RigConnectorElement>(Key))
		{
			SetConnectorSettings(Key, State.Settings, bSetupUndoRedo, false, false);
		}
		else
		{
			Key = GetController()->AddConnector(State.Name, State.Settings, bSetupUndoRedo, false);
		}

		Keys.Add(Key);
	}
	return Keys;
}

const FTLLRN_RigPhysicsSolverDescription* UTLLRN_RigHierarchy::FindPhysicsSolver(const FTLLRN_RigPhysicsSolverID& InID) const
{
	if(const UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetOuter()))
	{
		return TLLRN_ControlRig->FindPhysicsSolver(InID);
	}
	return nullptr;
}

const FTLLRN_RigPhysicsSolverDescription* UTLLRN_RigHierarchy::FindPhysicsSolverByName(const FName& InName) const
{
	if(const UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetOuter()))
	{
		return TLLRN_ControlRig->FindPhysicsSolverByName(InName);
	}
	return nullptr;
}

TArray<FName> UTLLRN_RigHierarchy::GetMetadataNames(FTLLRN_RigElementKey InItem) const
{
	TArray<FName> Names;
	if (const FTLLRN_RigBaseElement* Element = Find(InItem))
	{
		if (Element->MetadataStorageIndex != INDEX_NONE)
		{
			ElementMetadata[Element->MetadataStorageIndex].MetadataMap.GetKeys(Names);
		}
	}
	return Names;
}

ETLLRN_RigMetadataType UTLLRN_RigHierarchy::GetMetadataType(FTLLRN_RigElementKey InItem, FName InMetadataName) const
{
	if (const FTLLRN_RigBaseElement* Element = Find(InItem))
	{
		if (Element->MetadataStorageIndex != INDEX_NONE)
		{
			if (const FTLLRN_RigBaseMetadata* const* MetadataPtrPtr = ElementMetadata[Element->MetadataStorageIndex].MetadataMap.Find(InMetadataName))
			{
				return (*MetadataPtrPtr)->GetType();
			}
		}
	}
	
	return ETLLRN_RigMetadataType::Invalid;
}

bool UTLLRN_RigHierarchy::RemoveMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName)
{
	return RemoveMetadataForElement(Find(InItem), InMetadataName);
}

bool UTLLRN_RigHierarchy::RemoveAllMetadata(FTLLRN_RigElementKey InItem)
{
	return RemoveAllMetadataForElement(Find(InItem));
}

FName UTLLRN_RigHierarchy::GetModulePathFName(FTLLRN_RigElementKey InItem) const
{
	if(!InItem.IsValid())
	{
		return NAME_None;
	}
	
	const FName Result = GetNameMetadata(InItem, ModuleMetadataName, NAME_None);
	if(!Result.IsNone())
	{
		return Result;
	}

	// fall back on the name of the item
	const FString NameString = InItem.Name.ToString();
	FString ModulePathFromName;
	if(SplitNameSpace(NameString, &ModulePathFromName, nullptr))
	{
		if(!ModulePathFromName.IsEmpty())
		{
			return *ModulePathFromName;
		}
	}

	return NAME_None;
}

FString UTLLRN_RigHierarchy::GetModulePath(FTLLRN_RigElementKey InItem) const
{
	const FName ModulePathName = GetModulePathFName(InItem);
	if(!ModulePathName.IsNone())
	{
		return ModulePathName.ToString();
	}
	return FString();
}

FName UTLLRN_RigHierarchy::GetNameSpaceFName(FTLLRN_RigElementKey InItem) const
{
	if(!InItem.IsValid())
	{
		return NAME_None;
	}
	
	const FName Result = GetNameMetadata(InItem, NameSpaceMetadataName, NAME_None);
	if(!Result.IsNone())
	{
		return Result;
	}

	// fall back on the name of the item
	const FString NameString = InItem.Name.ToString();
	FString NameSpaceFromName;
	if(SplitNameSpace(NameString, &NameSpaceFromName, nullptr))
	{
		if(!NameSpaceFromName.IsEmpty())
		{
			return *(NameSpaceFromName + UTLLRN_ModularRig::NamespaceSeparator);
		}
	}
	
	return NAME_None;
}

FString UTLLRN_RigHierarchy::GetNameSpace(FTLLRN_RigElementKey InItem) const
{
	const FName NameSpaceName = GetNameSpaceFName(InItem);
	if(!NameSpaceName.IsNone())
	{
		return NameSpaceName.ToString();
	}
	return FString();
}

TArray<const FTLLRN_RigBaseElement*> UTLLRN_RigHierarchy::GetSelectedElements(ETLLRN_RigElementType InTypeFilter) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	TArray<const FTLLRN_RigBaseElement*> Selection;

	if(UTLLRN_RigHierarchy* HierarchyForSelection = HierarchyForSelectionPtr.Get())
	{
		TArray<FTLLRN_RigElementKey> SelectedKeys = HierarchyForSelection->GetSelectedKeys(InTypeFilter);
		for(const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
		{
			if(const FTLLRN_RigBaseElement* Element = Find(SelectedKey))
			{
				Selection.Add((FTLLRN_RigBaseElement*)Element);
			}
		}
		return Selection;
	}

	for (const FTLLRN_RigElementKey& SelectedKey : OrderedSelection)
	{
		if(SelectedKey.IsTypeOf(InTypeFilter))
		{
			if(const FTLLRN_RigBaseElement* Element = FindChecked(SelectedKey))
			{
				ensure(Element->IsSelected());
				Selection.Add(Element);
			}
		}
	}
	return Selection;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchy::GetSelectedKeys(ETLLRN_RigElementType InTypeFilter) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	
	if(UTLLRN_RigHierarchy* HierarchyForSelection = HierarchyForSelectionPtr.Get())
	{
		return HierarchyForSelection->GetSelectedKeys(InTypeFilter);
	}

	TArray<FTLLRN_RigElementKey> Selection;
	for (const FTLLRN_RigElementKey& SelectedKey : OrderedSelection)
	{
		if(SelectedKey.IsTypeOf(InTypeFilter))
		{
			Selection.Add(SelectedKey);
		}
	}
	
	return Selection;
}

FTLLRN_RigName UTLLRN_RigHierarchy::JoinNameSpace(const FTLLRN_RigName& InLeft, const FTLLRN_RigName& InRight)
{
	return FTLLRN_RigName(JoinNameSpace(InLeft.ToString(), InRight.ToString()));
}

FString UTLLRN_RigHierarchy::JoinNameSpace(const FString& InLeft, const FString& InRight)
{
	if(InLeft.EndsWith(UTLLRN_ModularRig::NamespaceSeparator))
	{
		return InLeft + InRight;
	}
	return InLeft + UTLLRN_ModularRig::NamespaceSeparator + InRight;
}

TPair<FString, FString> UTLLRN_RigHierarchy::SplitNameSpace(const FString& InNameSpacedPath, bool bFromEnd)
{
	TPair<FString, FString> Result;
	(void)SplitNameSpace(InNameSpacedPath, &Result.Key, &Result.Value, bFromEnd);
	return Result;
}

TPair<FTLLRN_RigName, FTLLRN_RigName> UTLLRN_RigHierarchy::SplitNameSpace(const FTLLRN_RigName& InNameSpacedPath, bool bFromEnd)
{
	const TPair<FString, FString> Result = SplitNameSpace(InNameSpacedPath.GetName(), bFromEnd);
	return {FTLLRN_RigName(Result.Key), FTLLRN_RigName(Result.Value)};
}

bool UTLLRN_RigHierarchy::SplitNameSpace(const FString& InNameSpacedPath, FString* OutNameSpace, FString* OutName, bool bFromEnd)
{
	return InNameSpacedPath.Split(UTLLRN_ModularRig::NamespaceSeparator, OutNameSpace, OutName, ESearchCase::CaseSensitive, bFromEnd ? ESearchDir::FromEnd : ESearchDir::FromStart);
}

bool UTLLRN_RigHierarchy::SplitNameSpace(const FTLLRN_RigName& InNameSpacedPath, FTLLRN_RigName* OutNameSpace, FTLLRN_RigName* OutName, bool bFromEnd)
{
	FString NameSpace, Name;
	if(SplitNameSpace(InNameSpacedPath.GetName(), &NameSpace, &Name, bFromEnd))
	{
		if(OutNameSpace)
		{
			OutNameSpace->SetName(NameSpace);
		}
		if(OutName)
		{
			OutName->SetName(Name);
		}
		return true;
	}
	return false;
}

void UTLLRN_RigHierarchy::SanitizeName(FTLLRN_RigName& InOutName, bool bAllowNameSpaces)
{
	// Sanitize the name
	FString SanitizedNameString = InOutName.GetName();
	bool bChangedSomething = false;
	for (int32 i = 0; i < SanitizedNameString.Len(); ++i)
	{
		TCHAR& C = SanitizedNameString[i];

		const bool bGoodChar = FChar::IsAlpha(C) ||					 // Any letter
			(C == '_') || (C == '-') || (C == '.') || (C == '|') ||	 // _  - .  | anytime
			(FChar::IsDigit(C)) ||									 // 0-9 anytime
			((i > 0) && (C== ' '));									 // Space after the first character to support virtual bones

		if (!bGoodChar)
		{
			if(bAllowNameSpaces && C == ':')
			{
				continue;
			}
			
			C = '_';
			bChangedSomething = true;
		}
	}

	if (SanitizedNameString.Len() > GetMaxNameLength())
	{
		SanitizedNameString.LeftChopInline(SanitizedNameString.Len() - GetMaxNameLength());
		bChangedSomething = true;
	}

	if(bChangedSomething)
	{
		InOutName.SetName(SanitizedNameString);
	}
}

FTLLRN_RigName UTLLRN_RigHierarchy::GetSanitizedName(const FTLLRN_RigName& InName, bool bAllowNameSpaces)
{
	FTLLRN_RigName Name = InName;
	SanitizeName(Name, bAllowNameSpaces);
	return Name;
}

bool UTLLRN_RigHierarchy::IsNameAvailable(const FTLLRN_RigName& InPotentialNewName, ETLLRN_RigElementType InType, FString* OutErrorMessage) const
{
	// check for fixed keywords
	const FTLLRN_RigElementKey PotentialKey(InPotentialNewName.GetFName(), InType);
	if(PotentialKey == UTLLRN_RigHierarchy::GetDefaultParentKey())
	{
		return false;
	}

	if (GetIndex(PotentialKey) != INDEX_NONE)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = TEXT("Name already used.");
		}
		return false;
	}

	const FTLLRN_RigName UnsanitizedName = InPotentialNewName;
	if (UnsanitizedName.Len() > GetMaxNameLength())
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = TEXT("Name too long.");
		}
		return false;
	}

	if (UnsanitizedName.IsNone())
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = TEXT("None is not a valid name.");
		}
		return false;
	}

	bool bAllowNameSpaces = bAllowNameSpaceWhenSanitizingName;

	// try to find a control rig this belongs to
	const UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetOuter());
	if(TLLRN_ControlRig == nullptr)
	{
		if(const UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			if(const UClass* Class = Blueprint->GeneratedClass)
			{
				TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(Class->GetDefaultObject());
			}
		}
	}

	// allow namespaces on default control rigs (non-module and non-modular)
	if(TLLRN_ControlRig)
	{
		if(!TLLRN_ControlRig->IsTLLRN_RigModule() &&
			!TLLRN_ControlRig->GetClass()->IsChildOf(UTLLRN_ModularRig::StaticClass()))
		{
			bAllowNameSpaces = true;
		}
	}
	else
	{
		bAllowNameSpaces = true;
	}

	FTLLRN_RigName SanitizedName = UnsanitizedName;
	SanitizeName(SanitizedName, bAllowNameSpaces);

	if (SanitizedName != UnsanitizedName)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = TEXT("Name contains invalid characters.");
		}
		return false;
	}

	return true;
}

bool UTLLRN_RigHierarchy::IsDisplayNameAvailable(const FTLLRN_RigElementKey& InParentElement,
	const FTLLRN_RigName& InPotentialNewDisplayName, FString* OutErrorMessage) const
{
	if(InParentElement.IsValid())
	{
		const TArray<FTLLRN_RigElementKey> ChildKeys = GetChildren(InParentElement);
		if(ChildKeys.ContainsByPredicate([&InPotentialNewDisplayName, this](const FTLLRN_RigElementKey& InChildKey) -> bool
		{
			if(const FTLLRN_RigBaseElement* BaseElement = Find(InChildKey))
			{
				if(BaseElement->GetDisplayName() == InPotentialNewDisplayName.GetFName())
				{
					return true;
				}
			}
			return false;
		}))
		{
			if (OutErrorMessage)
			{
				*OutErrorMessage = TEXT("Name already used.");
			}
			return false;
		}
	}

	const FTLLRN_RigName UnsanitizedName = InPotentialNewDisplayName;
	if (UnsanitizedName.Len() > GetMaxNameLength())
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = TEXT("Name too long.");
		}
		return false;
	}

	if (UnsanitizedName.IsNone())
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = TEXT("None is not a valid name.");
		}
		return false;
	}

	FTLLRN_RigName SanitizedName = UnsanitizedName;
	SanitizeName(SanitizedName, true);

	if (SanitizedName != UnsanitizedName)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = TEXT("Name contains invalid characters.");
		}
		return false;
	}

	return true;
}

FTLLRN_RigName UTLLRN_RigHierarchy::GetSafeNewName(const FTLLRN_RigName& InPotentialNewName, ETLLRN_RigElementType InType, bool bAllowNameSpace) const
{
	FTLLRN_RigName SanitizedName = InPotentialNewName;
	SanitizeName(SanitizedName, bAllowNameSpace);

	bAllowNameSpaceWhenSanitizingName = bAllowNameSpace;
	if(ExecuteContext)
	{
		const FTLLRN_ControlRigExecuteContext& CRContext = ExecuteContext->GetPublicData<FTLLRN_ControlRigExecuteContext>();
		if(CRContext.IsTLLRN_RigModule())
		{
			FTLLRN_RigName LastSegmentName;
			if(SplitNameSpace(SanitizedName, nullptr, &LastSegmentName, true))
			{
				SanitizedName = LastSegmentName;
			}
			SanitizedName = CRContext.GetTLLRN_RigModuleNameSpace() + SanitizedName.GetName();
			bAllowNameSpaceWhenSanitizingName = true;
		}
	}

	FTLLRN_RigName Name = SanitizedName;

	int32 Suffix = 1;
	while (!IsNameAvailable(Name, InType))
	{
		FString BaseString = SanitizedName.GetName();
		if (BaseString.Len() > GetMaxNameLength() - 4)
		{
			BaseString.LeftChopInline(BaseString.Len() - (GetMaxNameLength() - 4));
		}
		Name.SetName(FString::Printf(TEXT("%s_%d"), *BaseString, ++Suffix));
	}

	bAllowNameSpaceWhenSanitizingName = false;
	return Name;
}

FTLLRN_RigName UTLLRN_RigHierarchy::GetSafeNewDisplayName(const FTLLRN_RigElementKey& InParentElement, const FTLLRN_RigName& InPotentialNewDisplayName) const
{
	if(InPotentialNewDisplayName.IsNone())
	{
		return FTLLRN_RigName();
	}

	TArray<FTLLRN_RigElementKey> KeysToCheck;
	if(InParentElement.IsValid())
	{
		KeysToCheck = GetChildren(InParentElement);
	}
	else
	{
		// get all of the root elements
		for(const FTLLRN_RigBaseElement* Element : Elements)
		{
			if(!Element->IsA<FTLLRN_RigTransformElement>())
			{
				continue;
			}
			
			if(GetNumberOfParents(Element) == 0)
			{
				KeysToCheck.Add(Element->GetKey());
			}
		}
	}

	FTLLRN_RigName SanitizedName = InPotentialNewDisplayName;
	SanitizeName(SanitizedName);
	FTLLRN_RigName Name = SanitizedName;

	TArray<FString> DisplayNames;
	Algo::Transform(KeysToCheck, DisplayNames, [this](const FTLLRN_RigElementKey& InKey) -> FString
	{
		if(const FTLLRN_RigBaseElement* BaseElement = Find(InKey))
		{
			return BaseElement->GetDisplayName().ToString();
		}
		return FString();
	});

	int32 Suffix = 1;
	while (DisplayNames.Contains(Name.GetName()))
	{
		FString BaseString = SanitizedName.GetName();
		if (BaseString.Len() > GetMaxNameLength() - 4)
		{
			BaseString.LeftChopInline(BaseString.Len() - (GetMaxNameLength() - 4));
		}
		Name.SetName(FString::Printf(TEXT("%s_%d"), *BaseString, ++Suffix));
	}

	return Name;
}

FText UTLLRN_RigHierarchy::GetDisplayNameForUI(const FTLLRN_RigBaseElement* InElement, bool bIncludeNameSpace) const
{
	check(InElement);

	if(const UTLLRN_ModularRig* TLLRN_ModularRig = GetTypedOuter<UTLLRN_ModularRig>())
	{
		const FString ShortestPath = TLLRN_ModularRig->GetShortestDisplayPathForElement(InElement->GetKey(), false);
		if(!ShortestPath.IsEmpty())
		{
			return FText::FromString(ShortestPath);
		}
	}

	const FName& DisplayName = InElement->GetDisplayName();
	FString DisplayNameString = DisplayName.ToString();
	(void)SplitNameSpace(DisplayNameString, nullptr, &DisplayNameString);
	
	if(bIncludeNameSpace)
	{
		const FName ModuleShortName = GetNameMetadata(InElement->Key, ShortModuleNameMetadataName, NAME_None);
		if(!ModuleShortName.IsNone())
		{
			const FString ModuleShortNameString = ModuleShortName.ToString();
			const FString ModuleDisplayName = JoinNameSpace(ModuleShortNameString, DisplayNameString);
			return FText::FromString(ModuleDisplayName);
		}
	}

	return FText::FromString(*DisplayNameString);
}

FText UTLLRN_RigHierarchy::GetDisplayNameForUI(const FTLLRN_RigElementKey& InKey, bool bIncludeNameSpace) const
{
	if(const FTLLRN_RigBaseElement* Element = Find(InKey))
	{
		return GetDisplayNameForUI(Element, bIncludeNameSpace);
	}
	return FText();
}

int32 UTLLRN_RigHierarchy::GetPoseVersion(const FTLLRN_RigElementKey& InKey) const
{
	if(const FTLLRN_RigTransformElement* TransformElement = Find<FTLLRN_RigTransformElement>(InKey))
	{
		return GetPoseVersion(TransformElement->Index);
	}
	return INDEX_NONE;
}

FEdGraphPinType UTLLRN_RigHierarchy::GetControlPinType(FTLLRN_RigControlElement* InControlElement) const
{
	check(InControlElement);
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	// local copy of UEdGraphSchema_K2::PC_ ... static members
	static const FName PC_Boolean(TEXT("bool"));
	static const FName PC_Float(TEXT("float"));
	static const FName PC_Int(TEXT("int"));
	static const FName PC_Struct(TEXT("struct"));
	static const FName PC_Real(TEXT("real"));

	FEdGraphPinType PinType;

	switch(InControlElement->Settings.ControlType)
	{
		case ETLLRN_RigControlType::Bool:
		{
			PinType.PinCategory = PC_Boolean;
			break;
		}
		case ETLLRN_RigControlType::Float:
		case ETLLRN_RigControlType::ScaleFloat:
		{
			PinType.PinCategory = PC_Real;
			PinType.PinSubCategory = PC_Float;
			break;
		}
		case ETLLRN_RigControlType::Integer:
		{
			PinType.PinCategory = PC_Int;
			break;
		}
		case ETLLRN_RigControlType::Vector2D:
		{
			PinType.PinCategory = PC_Struct;
			PinType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get();
			break;
		}
		case ETLLRN_RigControlType::Position:
		case ETLLRN_RigControlType::Scale:
		{
			PinType.PinCategory = PC_Struct;
			PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
			break;
		}
		case ETLLRN_RigControlType::Rotator:
		{
			PinType.PinCategory = PC_Struct;
			PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
			break;
		}
		case ETLLRN_RigControlType::Transform:
		case ETLLRN_RigControlType::TransformNoScale:
		case ETLLRN_RigControlType::EulerTransform:
		{
			PinType.PinCategory = PC_Struct;
			PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
			break;
		}
	}

	return PinType;
}

FString UTLLRN_RigHierarchy::GetControlPinDefaultValue(FTLLRN_RigControlElement* InControlElement, bool bForEdGraph, ETLLRN_RigControlValueType InValueType) const
{
	check(InControlElement);
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	FTLLRN_RigControlValue Value = GetControlValue(InControlElement, InValueType);
	switch(InControlElement->Settings.ControlType)
	{
		case ETLLRN_RigControlType::Bool:
		{
			return Value.ToString<bool>();
		}
		case ETLLRN_RigControlType::Float:
		case ETLLRN_RigControlType::ScaleFloat:
		{
			return Value.ToString<float>();
		}
		case ETLLRN_RigControlType::Integer:
		{
			return Value.ToString<int32>();
		}
		case ETLLRN_RigControlType::Vector2D:
		{
			const FVector3f Vector = Value.Get<FVector3f>();
			const FVector2D Vector2D(Vector.X, Vector.Y);

			if(bForEdGraph)
			{
				return Vector2D.ToString();
			}

			FString Result;
			TBaseStructure<FVector2D>::Get()->ExportText(Result, &Vector2D, nullptr, nullptr, PPF_None, nullptr);
			return Result;
		}
		case ETLLRN_RigControlType::Position:
		case ETLLRN_RigControlType::Scale:
		{
			if(bForEdGraph)
			{
				// NOTE: We can not use ToString() here since the FDefaultValueHelper::IsStringValidVector used in
				// EdGraphSchema_K2 expects a string with format '#,#,#', while FVector:ToString is returning the value
				// with format 'X=# Y=# Z=#'				
				const FVector Vector(Value.Get<FVector3f>());
				return FString::Printf(TEXT("%3.3f,%3.3f,%3.3f"), Vector.X, Vector.Y, Vector.Z);
			}
			return Value.ToString<FVector>();
		}
		case ETLLRN_RigControlType::Rotator:
		{
				if(bForEdGraph)
				{
					// NOTE: se explanations above for Position/Scale
					const FRotator Rotator = FRotator::MakeFromEuler((FVector)Value.GetRef<FVector3f>());
					return FString::Printf(TEXT("%3.3f,%3.3f,%3.3f"), Rotator.Pitch, Rotator.Yaw, Rotator.Roll);
				}
				return Value.ToString<FRotator>();
		}
		case ETLLRN_RigControlType::Transform:
		case ETLLRN_RigControlType::TransformNoScale:
		case ETLLRN_RigControlType::EulerTransform:
		{
			const FTransform Transform = Value.GetAsTransform(
				InControlElement->Settings.ControlType,
				InControlElement->Settings.PrimaryAxis);
				
			if(bForEdGraph)
			{
				return Transform.ToString();
			}

			FString Result;
			TBaseStructure<FTransform>::Get()->ExportText(Result, &Transform, nullptr, nullptr, PPF_None, nullptr);
			return Result;
		}
	}
	return FString();
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchy::GetChildren(FTLLRN_RigElementKey InKey, bool bRecursive) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	if (bRecursive)
	{
		return ConvertElementsToKeys(GetChildren(Find(InKey), true));
	}
	else
	{
		return ConvertElementsToKeys(GetChildren(Find(InKey)));
	}
}

FTLLRN_RigBaseElementChildrenArray UTLLRN_RigHierarchy::GetActiveChildren(const FTLLRN_RigBaseElement* InElement, bool bRecursive) const
{
	TArray<FTLLRN_RigBaseElement*> Children;

	TArray<const FTLLRN_RigBaseElement*> ToProcess;
	ToProcess.Push(InElement);

	while (!ToProcess.IsEmpty())
	{
		const FTLLRN_RigBaseElement* CurrentParent = ToProcess.Pop();
		TArray<FTLLRN_RigBaseElement*> CurrentChildren;
		if (CurrentParent)
		{
			CurrentChildren = GetChildren(CurrentParent);
			if (CurrentParent->GetKey() == UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey())
			{
				CurrentChildren.Append(GetFilteredElements<FTLLRN_RigBaseElement>([this](const FTLLRN_RigBaseElement* Element)
				{
					return !Element->GetKey().IsTypeOf(ETLLRN_RigElementType::Reference) && GetActiveParent(Element) == nullptr;
				}));
			}
		}
		else
		{
			CurrentChildren = GetFilteredElements<FTLLRN_RigBaseElement>([this](const FTLLRN_RigBaseElement* Element)
			{
				return !Element->GetKey().IsTypeOf(ETLLRN_RigElementType::Reference) && GetActiveParent(Element) == nullptr;
			});
		}
		const FTLLRN_RigElementKey ParentKey = CurrentParent ? CurrentParent->GetKey() : UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey();
		for (const FTLLRN_RigBaseElement* Child : CurrentChildren)
		{
			const FTLLRN_RigBaseElement* ThisParent = GetActiveParent(Child);
			const FTLLRN_RigElementKey ThisParentKey = ThisParent ? ThisParent->GetKey() : UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey();
			if (ThisParentKey == ParentKey)
			{
				Children.Add(const_cast<FTLLRN_RigBaseElement*>(Child));
				if (bRecursive)
				{
					ToProcess.Push(Child);
				}
			}
		}
	}
	
	return FTLLRN_RigBaseElementChildrenArray(Children);
}

TArray<int32> UTLLRN_RigHierarchy::GetChildren(int32 InIndex, bool bRecursive) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	if (!ensure(Elements.IsValidIndex(InIndex)))
	{
		return {};
	}
	
	TArray<int32> ChildIndices;
	ConvertElementsToIndices(GetChildren(Elements[InIndex]), ChildIndices);

	if (bRecursive)
	{
		// Go along the children array and add all children. Once we stop adding children, the traversal index
		// will reach the end and we're done.
		for(int32 TraversalIndex = 0; TraversalIndex != ChildIndices.Num(); TraversalIndex++)
		{
			ConvertElementsToIndices(GetChildren(Elements[ChildIndices[TraversalIndex]]), ChildIndices);
		}
	}
	return ChildIndices;
}

TConstArrayView<FTLLRN_RigBaseElement*> UTLLRN_RigHierarchy::GetChildren(const FTLLRN_RigBaseElement* InElement) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InElement)
	{
		EnsureCachedChildrenAreCurrent();

		if (InElement->ChildCacheIndex != INDEX_NONE)
		{
			const FChildElementOffsetAndCount& OffsetAndCount = ChildElementOffsetAndCountCache[InElement->ChildCacheIndex];
			return TConstArrayView<FTLLRN_RigBaseElement*>(&ChildElementCache[OffsetAndCount.Offset], OffsetAndCount.Count);
		}
	}
	return {};
}

TArrayView<FTLLRN_RigBaseElement*> UTLLRN_RigHierarchy::GetChildren(const FTLLRN_RigBaseElement* InElement)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InElement)
	{
		EnsureCachedChildrenAreCurrent();

		if (InElement->ChildCacheIndex != INDEX_NONE)
		{
			const FChildElementOffsetAndCount& OffsetAndCount = ChildElementOffsetAndCountCache[InElement->ChildCacheIndex];
			return TArrayView<FTLLRN_RigBaseElement*>(&ChildElementCache[OffsetAndCount.Offset], OffsetAndCount.Count);
		}
	}
	return {};
}



FTLLRN_RigBaseElementChildrenArray UTLLRN_RigHierarchy::GetChildren(const FTLLRN_RigBaseElement* InElement, bool bRecursive) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	FTLLRN_RigBaseElementChildrenArray Children;

	Children.Append(GetChildren(InElement));

	if (bRecursive)
	{
		// Go along the children array and add all children. Once we stop adding children, the traversal index
		// will reach the end and we're done.
		for(int32 TraversalIndex = 0; TraversalIndex != Children.Num(); TraversalIndex++)
		{
			Children.Append(GetChildren(Children[TraversalIndex]));
		}
	}
	
	return Children;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchy::GetParents(FTLLRN_RigElementKey InKey, bool bRecursive) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	const FTLLRN_RigBaseElementParentArray& Parents = GetParents(Find(InKey), bRecursive);
	TArray<FTLLRN_RigElementKey> Keys;
	for(const FTLLRN_RigBaseElement* Parent : Parents)
	{
		Keys.Add(Parent->Key);
	}
	return Keys;
}

TArray<int32> UTLLRN_RigHierarchy::GetParents(int32 InIndex, bool bRecursive) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	const FTLLRN_RigBaseElementParentArray& Parents = GetParents(Get(InIndex), bRecursive);
	TArray<int32> Indices;
	for(const FTLLRN_RigBaseElement* Parent : Parents)
	{
		Indices.Add(Parent->Index);
	}
	return Indices;
}

FTLLRN_RigBaseElementParentArray UTLLRN_RigHierarchy::GetParents(const FTLLRN_RigBaseElement* InElement, bool bRecursive) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	FTLLRN_RigBaseElementParentArray Parents;

	if(const FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(InElement))
	{
		if(SingleParentElement->ParentElement)
		{
			Parents.Add(SingleParentElement->ParentElement);
		}
	}
	else if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InElement))
	{
		Parents.Reserve(MultiParentElement->ParentConstraints.Num());
		for(const FTLLRN_RigElementParentConstraint& ParentConstraint : MultiParentElement->ParentConstraints)
		{
			Parents.Add(ParentConstraint.ParentElement);
		}
	}

	if(bRecursive)
	{
		const int32 CurrentNumberParents = Parents.Num();
		for(int32 ParentIndex = 0;ParentIndex < CurrentNumberParents; ParentIndex++)
		{
			const FTLLRN_RigBaseElementParentArray GrandParents = GetParents(Parents[ParentIndex], bRecursive);
			for (FTLLRN_RigBaseElement* GrandParent : GrandParents)
			{
				Parents.AddUnique(GrandParent);
			}
		}
	}

	return Parents;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchy::GetDefaultParent(FTLLRN_RigElementKey InKey) const
{
	if (DefaultParentCacheTopologyVersion != GetTopologyVersion())
	{
		DefaultParentPerElement.Reset();
		DefaultParentCacheTopologyVersion = GetTopologyVersion();
	}
	
	FTLLRN_RigElementKey DefaultParent;
	if(const FTLLRN_RigElementKey* DefaultParentPtr = DefaultParentPerElement.Find(InKey))
	{
		DefaultParent = *DefaultParentPtr;
	}
	else
	{
		DefaultParent = GetFirstParent(InKey);
		DefaultParentPerElement.Add(InKey, DefaultParent);
	}
	return DefaultParent;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchy::GetFirstParent(FTLLRN_RigElementKey InKey) const
{
	if(FTLLRN_RigBaseElement* FirstParent = GetFirstParent(Find(InKey)))
	{
		return FirstParent->Key;
	}
	return FTLLRN_RigElementKey();
}

int32 UTLLRN_RigHierarchy::GetFirstParent(int32 InIndex) const
{
	if(FTLLRN_RigBaseElement* FirstParent = GetFirstParent(Get(InIndex)))
	{
		return FirstParent->Index;
	}
	return INDEX_NONE;
}

FTLLRN_RigBaseElement* UTLLRN_RigHierarchy::GetFirstParent(const FTLLRN_RigBaseElement* InElement) const
{
	if(const FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(InElement))
	{
		return SingleParentElement->ParentElement;
	}
	else if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InElement))
	{
		if(MultiParentElement->ParentConstraints.Num() > 0)
		{
			return MultiParentElement->ParentConstraints[0].ParentElement;
		}
	}
	
	return nullptr;
}

int32 UTLLRN_RigHierarchy::GetNumberOfParents(FTLLRN_RigElementKey InKey) const
{
	return GetNumberOfParents(Find(InKey));
}

int32 UTLLRN_RigHierarchy::GetNumberOfParents(int32 InIndex) const
{
	return GetNumberOfParents(Get(InIndex));
}

int32 UTLLRN_RigHierarchy::GetNumberOfParents(const FTLLRN_RigBaseElement* InElement) const
{
	if(InElement == nullptr)
	{
		return 0;
	}

	if(const FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(InElement))
	{
		return SingleParentElement->ParentElement == nullptr ? 0 : 1;
	}
	else if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InElement))
	{
		return MultiParentElement->ParentConstraints.Num();
	}

	return 0;
}

FTLLRN_RigElementWeight UTLLRN_RigHierarchy::GetParentWeight(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, bool bInitial) const
{
	return GetParentWeight(Find(InChild), Find(InParent), bInitial);
}

FTLLRN_RigElementWeight UTLLRN_RigHierarchy::GetParentWeight(const FTLLRN_RigBaseElement* InChild, const FTLLRN_RigBaseElement* InParent, bool bInitial) const
{
	if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		if(const int32* ParentIndexPtr = MultiParentElement->IndexLookup.Find(InParent->GetKey()))
		{
			return GetParentWeight(InChild, *ParentIndexPtr, bInitial);
		}
	}
	return FTLLRN_RigElementWeight(FLT_MAX);
}

FTLLRN_RigElementWeight UTLLRN_RigHierarchy::GetParentWeight(const FTLLRN_RigBaseElement* InChild, int32 InParentIndex, bool bInitial) const
{
	if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		if(MultiParentElement->ParentConstraints.IsValidIndex(InParentIndex))
		{
			if(bInitial)
			{
				return MultiParentElement->ParentConstraints[InParentIndex].InitialWeight;
			}
			else
			{
				return MultiParentElement->ParentConstraints[InParentIndex].Weight;
			}
		}
	}
	return FTLLRN_RigElementWeight(FLT_MAX);
}

TArray<FTLLRN_RigElementWeight> UTLLRN_RigHierarchy::GetParentWeightArray(FTLLRN_RigElementKey InChild, bool bInitial) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	return GetParentWeightArray(Find(InChild), bInitial);
}

TArray<FTLLRN_RigElementWeight> UTLLRN_RigHierarchy::GetParentWeightArray(const FTLLRN_RigBaseElement* InChild, bool bInitial) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	
	TArray<FTLLRN_RigElementWeight> Weights;
	if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		for(int32 ParentIndex = 0; ParentIndex < MultiParentElement->ParentConstraints.Num(); ParentIndex++)
		{
			if(bInitial)
			{
				Weights.Add(MultiParentElement->ParentConstraints[ParentIndex].InitialWeight);
			}
			else
			{
				Weights.Add(MultiParentElement->ParentConstraints[ParentIndex].Weight);
			}
		}
	}
	return Weights;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchy::GetActiveParent(const FTLLRN_RigElementKey& InKey, bool bReferenceKey) const
{
	if(FTLLRN_RigBaseElement* Parent = GetActiveParent(Find(InKey)))
	{
		if (bReferenceKey && Parent->GetKey() == GetDefaultParent(InKey))
		{
			return UTLLRN_RigHierarchy::GetDefaultParentKey();
		}
		return Parent->Key;
	}

	if (bReferenceKey)
	{
		return UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey();
	}

	return FTLLRN_RigElementKey();
}

int32 UTLLRN_RigHierarchy::GetActiveParent(int32 InIndex) const
{
	if(FTLLRN_RigBaseElement* Parent = GetActiveParent(Get(InIndex)))
	{
		return Parent->Index;
	}
	return INDEX_NONE;
}

FTLLRN_RigBaseElement* UTLLRN_RigHierarchy::GetActiveParent(const FTLLRN_RigBaseElement* InElement) const
{
	const TArray<FTLLRN_RigElementWeight> ParentWeights = GetParentWeightArray(InElement);
	if (ParentWeights.Num() > 0)
	{
		const FTLLRN_RigBaseElementParentArray ParentKeys = GetParents(InElement);
		check(ParentKeys.Num() == ParentWeights.Num());
		for (int32 ParentIndex = 0; ParentIndex < ParentKeys.Num(); ParentIndex++)
		{
			if (ParentWeights[ParentIndex].IsAlmostZero())
			{
				continue;
			}
			if (Elements.IsValidIndex(ParentKeys[ParentIndex]->GetIndex()))
			{
				return Elements[ParentKeys[ParentIndex]->GetIndex()];
			}
		}
	}

	return nullptr;
}


bool UTLLRN_RigHierarchy::SetParentWeight(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, FTLLRN_RigElementWeight InWeight, bool bInitial, bool bAffectChildren)
{
	return SetParentWeight(Find(InChild), Find(InParent), InWeight, bInitial, bAffectChildren);
}

bool UTLLRN_RigHierarchy::SetParentWeight(FTLLRN_RigBaseElement* InChild, const FTLLRN_RigBaseElement* InParent, FTLLRN_RigElementWeight InWeight, bool bInitial, bool bAffectChildren)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		if(const int32* ParentIndexPtr = MultiParentElement->IndexLookup.Find(InParent->GetKey()))
		{
			return SetParentWeight(InChild, *ParentIndexPtr, InWeight, bInitial, bAffectChildren);
		}
	}
	return false;
}

bool UTLLRN_RigHierarchy::SetParentWeight(FTLLRN_RigBaseElement* InChild, int32 InParentIndex, FTLLRN_RigElementWeight InWeight, bool bInitial, bool bAffectChildren)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	using namespace ETLLRN_RigTransformType;

	if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		if(MultiParentElement->ParentConstraints.IsValidIndex(InParentIndex))
		{
			if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(MultiParentElement))
			{
				// animation channels cannot change their parent weights, 
				// they are not 3d things - so multi parenting doesn't matter for transforms.
				if(ControlElement->IsAnimationChannel())
				{
					return false;
				}
			}
			
			InWeight.Location = FMath::Max(InWeight.Location, 0.f);
			InWeight.Rotation = FMath::Max(InWeight.Rotation, 0.f);
			InWeight.Scale = FMath::Max(InWeight.Scale, 0.f);

			FTLLRN_RigElementWeight& TargetWeight = bInitial?
				MultiParentElement->ParentConstraints[InParentIndex].InitialWeight :
				MultiParentElement->ParentConstraints[InParentIndex].Weight;

			if(FMath::IsNearlyZero(InWeight.Location - TargetWeight.Location) &&
				FMath::IsNearlyZero(InWeight.Rotation - TargetWeight.Rotation) &&
				FMath::IsNearlyZero(InWeight.Scale - TargetWeight.Scale))
			{
				return false;
			}
			
			const ETLLRN_RigTransformType::Type LocalType = bInitial ? InitialLocal : CurrentLocal;
			const ETLLRN_RigTransformType::Type GlobalType = SwapLocalAndGlobal(LocalType);

			if(bAffectChildren)
			{
				GetParentTransform(MultiParentElement, LocalType);
				if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(MultiParentElement))
				{
					GetControlOffsetTransform(ControlElement, LocalType);
				}
				GetTransform(MultiParentElement, LocalType);
				MultiParentElement->GetDirtyState().MarkDirty(GlobalType);
			}
			else
			{
				GetParentTransform(MultiParentElement, GlobalType);
				if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(MultiParentElement))
				{
					GetControlOffsetTransform(ControlElement, GlobalType);
				}
				GetTransform(MultiParentElement, GlobalType);
				MultiParentElement->GetDirtyState().MarkDirty(LocalType);
			}

			TargetWeight = InWeight;

			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(MultiParentElement))
			{
				ControlElement->GetOffsetDirtyState().MarkDirty(GlobalType);
			}

			PropagateDirtyFlags(MultiParentElement, ETLLRN_RigTransformType::IsInitial(LocalType), bAffectChildren);
			EnsureCacheValidity();
			
#if WITH_EDITOR
			if (!bPropagatingChange)
			{
				TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);

				ForEachListeningHierarchy([this, LocalType, InChild, InParentIndex, InWeight, bInitial, bAffectChildren](const FTLLRN_RigHierarchyListener& Listener)
				{
					if(!bForcePropagation && !Listener.ShouldReactToChange(LocalType))
					{
						return;
					}

					if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
					{
						if(FTLLRN_RigBaseElement* ListeningElement = ListeningHierarchy->Find(InChild->GetKey()))
						{
							ListeningHierarchy->SetParentWeight(ListeningElement, InParentIndex, InWeight, bInitial, bAffectChildren);
						}
					}
				});
			}
#endif

			Notify(ETLLRN_RigHierarchyNotification::ParentWeightsChanged, MultiParentElement);
			return true;
		}
	}
	return false;
}

bool UTLLRN_RigHierarchy::SetParentWeightArray(FTLLRN_RigElementKey InChild, TArray<FTLLRN_RigElementWeight> InWeights, bool bInitial,
	bool bAffectChildren)
{
	return SetParentWeightArray(Find(InChild), InWeights, bInitial, bAffectChildren);
}

bool UTLLRN_RigHierarchy::SetParentWeightArray(FTLLRN_RigBaseElement* InChild, const TArray<FTLLRN_RigElementWeight>& InWeights,
	bool bInitial, bool bAffectChildren)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InWeights.Num() == 0)
	{
		return false;
	}
	
	TArrayView<const FTLLRN_RigElementWeight> View(InWeights.GetData(), InWeights.Num());
	return SetParentWeightArray(InChild, View, bInitial, bAffectChildren);
}

bool UTLLRN_RigHierarchy::SetParentWeightArray(FTLLRN_RigBaseElement* InChild,  const TArrayView<const FTLLRN_RigElementWeight>& InWeights,
	bool bInitial, bool bAffectChildren)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	using namespace ETLLRN_RigTransformType;

	if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(MultiParentElement))
		{
			// animation channels cannot change their parent weights, 
			// they are not 3d things - so multi parenting doesn't matter for transforms.
			if(ControlElement->IsAnimationChannel())
			{
				return false;
			}
		}

		if(MultiParentElement->ParentConstraints.Num() == InWeights.Num())
		{
			TArray<FTLLRN_RigElementWeight> InputWeights;
			InputWeights.Reserve(InWeights.Num());

			bool bFoundDifference = false;
			for(int32 WeightIndex=0; WeightIndex < InWeights.Num(); WeightIndex++)
			{
				FTLLRN_RigElementWeight InputWeight = InWeights[WeightIndex];
				InputWeight.Location = FMath::Max(InputWeight.Location, 0.f);
				InputWeight.Rotation = FMath::Max(InputWeight.Rotation, 0.f);
				InputWeight.Scale = FMath::Max(InputWeight.Scale, 0.f);
				InputWeights.Add(InputWeight);

				FTLLRN_RigElementWeight& TargetWeight = bInitial?
					MultiParentElement->ParentConstraints[WeightIndex].InitialWeight :
					MultiParentElement->ParentConstraints[WeightIndex].Weight;

				if(!FMath::IsNearlyZero(InputWeight.Location - TargetWeight.Location) ||
					!FMath::IsNearlyZero(InputWeight.Rotation - TargetWeight.Rotation) ||
					!FMath::IsNearlyZero(InputWeight.Scale - TargetWeight.Scale))
				{
					bFoundDifference = true;
				}
			}

			if(!bFoundDifference)
			{
				return false;
			}
			
			const ETLLRN_RigTransformType::Type LocalType = bInitial ? InitialLocal : CurrentLocal;
			const ETLLRN_RigTransformType::Type GlobalType = SwapLocalAndGlobal(LocalType);

			if(bAffectChildren)
			{
				GetTransform(MultiParentElement, LocalType);
				MultiParentElement->GetDirtyState().MarkDirty(GlobalType);
			}
			else
			{
				GetTransform(MultiParentElement, GlobalType);
				MultiParentElement->GetDirtyState().MarkDirty(LocalType);
			}

			for(int32 WeightIndex=0; WeightIndex < InWeights.Num(); WeightIndex++)
			{
				if(bInitial)
				{
					MultiParentElement->ParentConstraints[WeightIndex].InitialWeight = InputWeights[WeightIndex];
				}
				else
				{
					MultiParentElement->ParentConstraints[WeightIndex].Weight = InputWeights[WeightIndex];
				}
			}

			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(MultiParentElement))
			{
				ControlElement->GetOffsetDirtyState().MarkDirty(GlobalType);
				ControlElement->GetShapeDirtyState().MarkDirty(GlobalType);
			}

			PropagateDirtyFlags(MultiParentElement, ETLLRN_RigTransformType::IsInitial(LocalType), bAffectChildren);
			EnsureCacheValidity();
			
#if WITH_EDITOR
			if (!bPropagatingChange)
			{
				TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);
			
				ForEachListeningHierarchy([this, LocalType, InChild, InWeights, bInitial, bAffectChildren](const FTLLRN_RigHierarchyListener& Listener)
     			{
					if(!bForcePropagation && !Listener.ShouldReactToChange(LocalType))
					{
						return;
					}

					if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
					{
						if(FTLLRN_RigBaseElement* ListeningElement = ListeningHierarchy->Find(InChild->GetKey()))
						{
							ListeningHierarchy->SetParentWeightArray(ListeningElement, InWeights, bInitial, bAffectChildren);
						}
					}
				});
			}
#endif

			Notify(ETLLRN_RigHierarchyNotification::ParentWeightsChanged, MultiParentElement);

			return true;
		}
	}
	return false;
}

bool UTLLRN_RigHierarchy::CanSwitchToParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, const TElementDependencyMap& InDependencyMap, FString* OutFailureReason)
{
	InParent = PreprocessParentElementKeyForSpaceSwitching(InChild, InParent);

	FTLLRN_RigBaseElement* Child = Find(InChild);
	if(Child == nullptr)
	{
		if(OutFailureReason)
		{
			OutFailureReason->Appendf(TEXT("Child Element %s cannot be found."), *InChild.ToString());
		}
		return false;
	}

	FTLLRN_RigBaseElement* Parent = Find(InParent);
	if(Parent == nullptr)
	{
		// if we don't specify anything and the element is parented directly to the world,
		// perfomring this switch means unparenting it from world (since there is no default parent)
		if(!InParent.IsValid() && GetFirstParent(InChild) == GetWorldSpaceReferenceKey())
		{
			return true;
		}
		
		if(OutFailureReason)
		{
			OutFailureReason->Appendf(TEXT("Parent Element %s cannot be found."), *InParent.ToString());
		}
		return false;
	}

	// see if this is already parented to the target parent
	if(GetFirstParent(Child) == Parent)
	{
		return true;
	}

	const FTLLRN_RigMultiParentElement* MultiParentChild = Cast<FTLLRN_RigMultiParentElement>(Child);
	if(MultiParentChild == nullptr)
	{
		if(OutFailureReason)
		{
			OutFailureReason->Appendf(TEXT("Child Element %s does not allow space switching (it's not a multi parent element)."), *InChild.ToString());
		}
	}

	const FTLLRN_RigTransformElement* TransformParent = Cast<FTLLRN_RigMultiParentElement>(Parent);
	if(TransformParent == nullptr)
	{
		if(OutFailureReason)
		{
			OutFailureReason->Appendf(TEXT("Parent Element %s is not a transform element"), *InParent.ToString());
		}
	}

	if(IsParentedTo(Parent, Child, InDependencyMap))
	{
		if(OutFailureReason)
		{
			OutFailureReason->Appendf(TEXT("Cannot switch '%s' to '%s' - would cause a cycle."), *InChild.ToString(), *InParent.ToString());
		}
		return false;
	}

	return true;
}

bool UTLLRN_RigHierarchy::SwitchToParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, bool bInitial, bool bAffectChildren, const TElementDependencyMap& InDependencyMap, FString* OutFailureReason)
{
	InParent = PreprocessParentElementKeyForSpaceSwitching(InChild, InParent);
	return SwitchToParent(Find(InChild), Find(InParent), bInitial, bAffectChildren, InDependencyMap, OutFailureReason);
}

bool UTLLRN_RigHierarchy::SwitchToParent(FTLLRN_RigBaseElement* InChild, FTLLRN_RigBaseElement* InParent, bool bInitial,
	bool bAffectChildren, const TElementDependencyMap& InDependencyMap, FString* OutFailureReason)
{
	FTLLRN_RigHierarchyEnableControllerBracket EnableController(this, true);

	// Exit early if switching to the same parent 
	if (InChild)
	{
		const FTLLRN_RigElementKey ChildKey = InChild->GetKey();
		const FTLLRN_RigElementKey ParentKey = InParent ? InParent->GetKey() : GetDefaultParent(ChildKey);
		const FTLLRN_RigElementKey ActiveParentKey = GetActiveParent(ChildKey);
		if(ActiveParentKey == ParentKey ||
			(ActiveParentKey == UTLLRN_RigHierarchy::GetDefaultParentKey() && GetDefaultParent(ChildKey) == ParentKey))
		{
			return true;
		}
	}

	// rely on the VM's dependency map if there's currently an available context.
	const TElementDependencyMap* DependencyMapPtr = &InDependencyMap;

#if WITH_EDITOR
	// Keep this in function scope, since we might be taking a pointer to it.
	TElementDependencyMap DependencyMapFromVM;
	{
		FScopeLock Lock(&ExecuteContextLock);
		if(ExecuteContext != nullptr && DependencyMapPtr->IsEmpty())
		{
			if(ExecuteContext->VM)
			{
				DependencyMapFromVM = GetDependenciesForVM(ExecuteContext->VM);
				DependencyMapPtr = &DependencyMapFromVM;
			}
		}
	}
	
#endif
	
	if(InChild && InParent)
	{
		if(!CanSwitchToParent(InChild->GetKey(), InParent->GetKey(), *DependencyMapPtr, OutFailureReason))
		{
			return false;
		}
	}

	if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		int32 ParentIndex = INDEX_NONE;
		if(InParent)
		{
			if(const int32* ParentIndexPtr = MultiParentElement->IndexLookup.Find(InParent->GetKey()))
			{
				ParentIndex = *ParentIndexPtr;
			}
			else
			{
				if(UTLLRN_RigHierarchyController* Controller = GetController(true))
				{
					if(Controller->AddParent(InChild, InParent, 0.f, true, false))
					{
						ParentIndex = MultiParentElement->IndexLookup.FindChecked(InParent->GetKey());
					}
				}
			}
		}
		return SwitchToParent(InChild, ParentIndex, bInitial, bAffectChildren);
	}
	return false;
}

bool UTLLRN_RigHierarchy::SwitchToParent(FTLLRN_RigBaseElement* InChild, int32 InParentIndex, bool bInitial, bool bAffectChildren)
{
	TArray<FTLLRN_RigElementWeight> Weights = GetParentWeightArray(InChild, bInitial);
	FMemory::Memzero(Weights.GetData(), Weights.GetAllocatedSize());
	if(Weights.IsValidIndex(InParentIndex))
	{
		Weights[InParentIndex] = 1.f;
	}
	return SetParentWeightArray(InChild, Weights, bInitial, bAffectChildren);
}

bool UTLLRN_RigHierarchy::SwitchToDefaultParent(FTLLRN_RigElementKey InChild, bool bInitial, bool bAffectChildren)
{
	return SwitchToParent(InChild, GetDefaultParentKey(), bInitial, bAffectChildren);
}

bool UTLLRN_RigHierarchy::SwitchToDefaultParent(FTLLRN_RigBaseElement* InChild, bool bInitial, bool bAffectChildren)
{
	// we assume that the first stored parent is the default parent
	check(InChild);
	return SwitchToParent(InChild->GetKey(), GetDefaultParentKey(), bInitial, bAffectChildren);
}

bool UTLLRN_RigHierarchy::SwitchToWorldSpace(FTLLRN_RigElementKey InChild, bool bInitial, bool bAffectChildren)
{
	return SwitchToParent(InChild, GetWorldSpaceReferenceKey(), bInitial, bAffectChildren);
}

bool UTLLRN_RigHierarchy::SwitchToWorldSpace(FTLLRN_RigBaseElement* InChild, bool bInitial, bool bAffectChildren)
{
	check(InChild);
	return SwitchToParent(InChild->GetKey(), GetWorldSpaceReferenceKey(), bInitial, bAffectChildren);
}

FTLLRN_RigElementKey UTLLRN_RigHierarchy::GetOrAddWorldSpaceReference()
{
	FTLLRN_RigHierarchyEnableControllerBracket EnableController(this, true);

	const FTLLRN_RigElementKey WorldSpaceReferenceKey = GetWorldSpaceReferenceKey();

	FTLLRN_RigBaseElement* Parent = Find(WorldSpaceReferenceKey);
	if(Parent)
	{
		return Parent->GetKey();
	}

	if(UTLLRN_RigHierarchyController* Controller = GetController(true))
	{
		return Controller->AddReference(
			WorldSpaceReferenceKey.Name,
			FTLLRN_RigElementKey(),
			FRigReferenceGetWorldTransformDelegate::CreateUObject(this, &UTLLRN_RigHierarchy::GetWorldTransformForReference),
			false);
	}

	return FTLLRN_RigElementKey();
}

FTLLRN_RigElementKey UTLLRN_RigHierarchy::GetDefaultParentKey()
{
	static const FName DefaultParentName = TEXT("DefaultParent");
	return FTLLRN_RigElementKey(DefaultParentName, ETLLRN_RigElementType::Reference); 
}

FTLLRN_RigElementKey UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey()
{
	static const FName WorldSpaceReferenceName = TEXT("WorldSpace");
	return FTLLRN_RigElementKey(WorldSpaceReferenceName, ETLLRN_RigElementType::Reference); 
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchy::GetAnimationChannels(FTLLRN_RigElementKey InKey, bool bOnlyDirectChildren) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	return ConvertElementsToKeys(GetAnimationChannels(Find<FTLLRN_RigControlElement>(InKey), bOnlyDirectChildren));
}

TArray<int32> UTLLRN_RigHierarchy::GetAnimationChannels(int32 InIndex, bool bOnlyDirectChildren) const
{
	return ConvertElementsToIndices(GetAnimationChannels(Get<FTLLRN_RigControlElement>(InIndex), bOnlyDirectChildren));
}

TArray<FTLLRN_RigControlElement*> UTLLRN_RigHierarchy::GetAnimationChannels(const FTLLRN_RigControlElement* InElement, bool bOnlyDirectChildren) const
{
	if(InElement == nullptr)
	{
		return {};
	}

	const TConstArrayView<FTLLRN_RigBaseElement*> AllChildren = GetChildren(InElement);
	const TArray<FTLLRN_RigBaseElement*> FilteredChildren = AllChildren.FilterByPredicate([](const FTLLRN_RigBaseElement* Element) -> bool
	{
		if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
		{
			return ControlElement->IsAnimationChannel();
		}
		return false;
	});

	TArray<FTLLRN_RigControlElement*> AnimationChannels = ConvertElements<FTLLRN_RigControlElement>(FilteredChildren);
	if(!bOnlyDirectChildren)
	{
		AnimationChannels.Append(GetFilteredElements<FTLLRN_RigControlElement>(
			[InElement](const FTLLRN_RigControlElement* PotentialAnimationChannel) -> bool
			{
				if(PotentialAnimationChannel->IsAnimationChannel())
				{
					if(PotentialAnimationChannel->Settings.Customization.AvailableSpaces.Contains(InElement->Key))
					{
						return true;
					}
				}
				return false;
			},
		true /* traverse */
		));
	}
	
	return AnimationChannels;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchy::GetAllKeys(bool bTraverse, ETLLRN_RigElementType InElementType) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"))

	return GetKeysByPredicate([InElementType](const FTLLRN_RigBaseElement& InElement)
	{
		return InElement.IsTypeOf(InElementType);
	}, bTraverse);
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchy::GetKeysByPredicate(
	TFunctionRef<bool(const FTLLRN_RigBaseElement&)> InPredicateFunc,
	bool bTraverse
	) const
{
	auto ElementTraverser = [&](TFunctionRef<void(const FTLLRN_RigBaseElement&)> InProcessFunc)
	{
		if(bTraverse)
		{
			// TBitArray reserves 4, we'll do 16 so we can remember at least 512 elements before
			// we need to hit the heap.
			TBitArray<TInlineAllocator<16>> ElementVisited(false, Elements.Num());

			const TArray<FTLLRN_RigBaseElement*> RootElements = GetRootElements();
			for (FTLLRN_RigBaseElement* Element : RootElements)
			{
				const int32 ElementIndex = Element->GetIndex();
				Traverse(Element, true, [&ElementVisited, InProcessFunc, InPredicateFunc](FTLLRN_RigBaseElement* InElement, bool& bContinue)
				{
					bContinue = !ElementVisited[InElement->GetIndex()];

					if(bContinue)
					{
						if(InPredicateFunc(*InElement))
						{
							InProcessFunc(*InElement);
						}
						ElementVisited[InElement->GetIndex()] = true;
					}
				});
			}
		}
		else
		{
			for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
			{
				const FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
				if(InPredicateFunc(*Element))
				{
					InProcessFunc(*Element);
				}
			}
		}
	};
	
	// First count up how many elements we matched and only reserve that amount. There's very little overhead
	// since we're just running over the same data, so it should still be hot when we do the second pass.
	int32 NbElements = 0;
	ElementTraverser([&NbElements](const FTLLRN_RigBaseElement&) { NbElements++; });
	
	TArray<FTLLRN_RigElementKey> Keys;
	Keys.Reserve(NbElements);
	ElementTraverser([&Keys](const FTLLRN_RigBaseElement& InElement) { Keys.Add(InElement.GetKey()); });

	return Keys;
}

void UTLLRN_RigHierarchy::Traverse(FTLLRN_RigBaseElement* InElement, bool bTowardsChildren,
                             TFunction<void(FTLLRN_RigBaseElement*, bool&)> PerElementFunction) const
{
	bool bContinue = true;
	PerElementFunction(InElement, bContinue);

	if(bContinue)
	{
		if(bTowardsChildren)
		{
			for (FTLLRN_RigBaseElement* Child : GetChildren(InElement))
			{
				Traverse(Child, true, PerElementFunction);
			}
		}
		else
		{
			FTLLRN_RigBaseElementParentArray Parents = GetParents(InElement);
			for (FTLLRN_RigBaseElement* Parent : Parents)
			{
				Traverse(Parent, false, PerElementFunction);
			}
		}
	}
}

void UTLLRN_RigHierarchy::Traverse(TFunction<void(FTLLRN_RigBaseElement*, bool& /* continue */)> PerElementFunction, bool bTowardsChildren) const
{
	if(bTowardsChildren)
	{
		for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
		{
			FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
			if(GetNumberOfParents(Element) == 0)
			{
				Traverse(Element, bTowardsChildren, PerElementFunction);
			}
        }
	}
	else
	{
		for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
		{
			FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
			if(GetChildren(Element).Num() == 0)
			{
				Traverse(Element, bTowardsChildren, PerElementFunction);
			}
		}
	}
}

const FTLLRN_RigElementKey& UTLLRN_RigHierarchy::GetResolvedTarget(const FTLLRN_RigElementKey& InConnectorKey) const
{
	if (InConnectorKey.Type == ETLLRN_RigElementType::Connector)
	{
		if(ElementKeyRedirector)
		{
			if(const FTLLRN_CachedTLLRN_RigElement* Target = ElementKeyRedirector->Find(InConnectorKey))
			{
				return Target->GetKey();
			}
		}
	}
	return InConnectorKey;
}

bool UTLLRN_RigHierarchy::Undo()
{
#if WITH_EDITOR
	
	if(TransformUndoStack.IsEmpty())
	{
		return false;
	}

	const FTLLRN_RigTransformStackEntry Entry = TransformUndoStack.Pop();
	ApplyTransformFromStack(Entry, true);
	UndoRedoEvent.Broadcast(this, Entry.Key, Entry.TransformType, Entry.OldTransform, true);
	TransformRedoStack.Push(Entry);
	TransformStackIndex = TransformUndoStack.Num();
	return true;
	
#else
	
	return false;
	
#endif
}

bool UTLLRN_RigHierarchy::Redo()
{
#if WITH_EDITOR

	if(TransformRedoStack.IsEmpty())
	{
		return false;
	}

	const FTLLRN_RigTransformStackEntry Entry = TransformRedoStack.Pop();
	ApplyTransformFromStack(Entry, false);
	UndoRedoEvent.Broadcast(this, Entry.Key, Entry.TransformType, Entry.NewTransform, false);
	TransformUndoStack.Push(Entry);
	TransformStackIndex = TransformUndoStack.Num();
	return true;
	
#else
	
	return false;
	
#endif
}

bool UTLLRN_RigHierarchy::SetTransformStackIndex(int32 InTransformStackIndex)
{
#if WITH_EDITOR

	while(TransformUndoStack.Num() > InTransformStackIndex)
	{
		if(TransformUndoStack.Num() == 0)
		{
			return false;
		}

		if(!Undo())
		{
			return false;
		}
	}
	
	while(TransformUndoStack.Num() < InTransformStackIndex)
	{
		if(TransformRedoStack.Num() == 0)
		{
			return false;
		}

		if(!Redo())
		{
			return false;
		}
	}

	return InTransformStackIndex == TransformStackIndex;

#else
	
	return false;
	
#endif
}

#if WITH_EDITOR

void UTLLRN_RigHierarchy::PostEditUndo()
{
	Super::PostEditUndo();

	const int32 DesiredStackIndex = TransformStackIndex;
	TransformStackIndex = TransformUndoStack.Num();
	if (DesiredStackIndex != TransformStackIndex)
	{
		SetTransformStackIndex(DesiredStackIndex);
	}

	if(UTLLRN_RigHierarchyController* Controller = GetController(false))
	{
		Controller->SetHierarchy(this);
	}
}

#endif

void UTLLRN_RigHierarchy::SendEvent(const FTLLRN_RigEventContext& InEvent, bool bAsynchronous)
{
	if(EventDelegate.IsBound())
	{
		TWeakObjectPtr<UTLLRN_RigHierarchy> WeakThis = this;
		FTLLRN_RigEventDelegate& Delegate = EventDelegate;

		if (bAsynchronous)
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady([WeakThis, Delegate, InEvent]()
            {
                Delegate.Broadcast(WeakThis.Get(), InEvent);
            }, TStatId(), NULL, ENamedThreads::GameThread);
		}
		else
		{
			Delegate.Broadcast(this, InEvent);
		}
	}

}

void UTLLRN_RigHierarchy::SendAutoKeyEvent(FTLLRN_RigElementKey InElement, float InOffsetInSeconds, bool bAsynchronous)
{
	FTLLRN_RigEventContext Context;
	Context.Event = ETLLRN_RigEvent::RequestAutoKey;
	Context.Key = InElement;
	Context.LocalTime = InOffsetInSeconds;
	if(UTLLRN_ControlRig* Rig = Cast<UTLLRN_ControlRig>(GetOuter()))
	{
		Context.LocalTime += Rig->AbsoluteTime;
	}
	SendEvent(Context, bAsynchronous);
}

bool UTLLRN_RigHierarchy::IsControllerAvailable() const
{
	return bIsControllerAvailable;
}

UTLLRN_RigHierarchyController* UTLLRN_RigHierarchy::GetController(bool bCreateIfNeeded)
{
	if(!IsControllerAvailable())
	{
		return nullptr;
	}
	if(HierarchyController)
	{
		return HierarchyController;
	}
	else if(bCreateIfNeeded)
	{
		 {
			 FGCScopeGuard Guard;
			 HierarchyController = NewObject<UTLLRN_RigHierarchyController>(this, TEXT("HierarchyController"), RF_Transient);
			 // In case we create this object from async loading thread
			 HierarchyController->ClearInternalFlags(EInternalObjectFlags::Async);

			 HierarchyController->SetHierarchy(this);
			 return HierarchyController;
		 }
	}
	return nullptr;
}

UTLLRN_ModularRigRuleManager* UTLLRN_RigHierarchy::GetRuleManager(bool bCreateIfNeeded)
{
	if(RuleManager)
	{
		return RuleManager;
	}
	else if(bCreateIfNeeded)
	{
		{
			FGCScopeGuard Guard;
			RuleManager = NewObject<UTLLRN_ModularRigRuleManager>(this, TEXT("RuleManager"), RF_Transient);
			// In case we create this object from async loading thread
			RuleManager->ClearInternalFlags(EInternalObjectFlags::Async);

			RuleManager->SetHierarchy(this);
			return RuleManager;
		}
	}
	return nullptr;
}

void UTLLRN_RigHierarchy::IncrementTopologyVersion()
{
	TopologyVersion++;
	KeyCollectionCache.Reset();
}

FTLLRN_RigPose UTLLRN_RigHierarchy::GetPose(
	bool bInitial,
	ETLLRN_RigElementType InElementType,
	const FTLLRN_RigElementKeyCollection& InItems,
	bool bIncludeTransientControls
) const
{
	return GetPose(bInitial, InElementType, TArrayView<const FTLLRN_RigElementKey>(InItems.Keys.GetData(), InItems.Num()), bIncludeTransientControls);
}

FTLLRN_RigPose UTLLRN_RigHierarchy::GetPose(bool bInitial, ETLLRN_RigElementType InElementType,
	const TArrayView<const FTLLRN_RigElementKey>& InItems, bool bIncludeTransientControls) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	FTLLRN_RigPose Pose;
	Pose.HierarchyTopologyVersion = GetTopologyVersion();
	Pose.PoseHash = Pose.HierarchyTopologyVersion;

	for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
	{
		FTLLRN_RigBaseElement* Element = Elements[ElementIndex];

		// filter by type
		if (((uint8)InElementType & (uint8)Element->GetType()) == 0)
		{
			continue;
		}

		// filter by optional collection
		if(InItems.Num() > 0)
		{
			if(!InItems.Contains(Element->GetKey()))
			{
				continue;
			}
		}
		
		FTLLRN_RigPoseElement PoseElement;
		PoseElement.Index.UpdateCache(Element->GetKey(), this);
		
		if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Element))
		{
			PoseElement.LocalTransform = GetTransform(TransformElement, bInitial ? ETLLRN_RigTransformType::InitialLocal : ETLLRN_RigTransformType::CurrentLocal);
			PoseElement.GlobalTransform = GetTransform(TransformElement, bInitial ? ETLLRN_RigTransformType::InitialGlobal : ETLLRN_RigTransformType::CurrentGlobal);
			PoseElement.ActiveParent = GetActiveParent(Element->GetKey());

			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
			{
				if (bUsePreferredEulerAngles)
				{
					PoseElement.PreferredEulerAngle = GetControlPreferredEulerAngles(ControlElement,
					   GetControlPreferredEulerRotationOrder(ControlElement), bInitial);
				}

				if(!bIncludeTransientControls && ControlElement->Settings.bIsTransientControl)
				{
					continue;
				}
			}
		}
		else if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(Element))
		{
			PoseElement.CurveValue = GetCurveValue(CurveElement);
		}
		else
		{
			continue;
		}
		Pose.Elements.Add(PoseElement);
		Pose.PoseHash = HashCombine(Pose.PoseHash, GetTypeHash(PoseElement.Index.GetKey()));
	}
	return Pose;
}

void UTLLRN_RigHierarchy::SetPose(
	const FTLLRN_RigPose& InPose,
	ETLLRN_RigTransformType::Type InTransformType,
	ETLLRN_RigElementType InElementType,
	const FTLLRN_RigElementKeyCollection& InItems,
	float InWeight
)
{
	SetPose(InPose, InTransformType, InElementType, TArrayView<const FTLLRN_RigElementKey>(InItems.Keys.GetData(), InItems.Num()), InWeight);
}

void UTLLRN_RigHierarchy::SetPose(const FTLLRN_RigPose& InPose, ETLLRN_RigTransformType::Type InTransformType,
	ETLLRN_RigElementType InElementType, const TArrayView<const FTLLRN_RigElementKey>& InItems, float InWeight)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	const float U = FMath::Clamp(InWeight, 0.f, 1.f);
	if(U < SMALL_NUMBER)
	{
		return;
	}

	const bool bBlend = U < 1.f - SMALL_NUMBER;
	const bool bLocal = IsLocal(InTransformType);
	static constexpr bool bAffectChildren = true;

	for(const FTLLRN_RigPoseElement& PoseElement : InPose)
	{
		FTLLRN_CachedTLLRN_RigElement Index = PoseElement.Index;

		// filter by type
		if (((uint8)InElementType & (uint8)Index.GetKey().Type) == 0)
		{
			continue;
		}

		// filter by optional collection
		if(InItems.Num() > 0)
		{
			if(!InItems.Contains(Index.GetKey()))
			{
				continue;
			}
		}

		if(Index.UpdateCache(this))
		{
			FTLLRN_RigBaseElement* Element = Get(Index.GetIndex());
			if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Element))
			{
				// only nulls and controls can switch parent (cf. FTLLRN_RigUnit_SwitchParent)
				const bool bCanSwitch = TransformElement->IsA<FTLLRN_RigMultiParentElement>() && PoseElement.ActiveParent.IsValid();
					
				const FTransform& PoseTransform = bLocal ? PoseElement.LocalTransform : PoseElement.GlobalTransform;
				if (bBlend)
				{
					const FTransform PreviousTransform = GetTransform(TransformElement, InTransformType);
					const FTransform TransformToSet = FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, PoseTransform, U);
					if (bCanSwitch)
					{
						SwitchToParent(Element->GetKey(), PoseElement.ActiveParent);
					}
					SetTransform(TransformElement, TransformToSet, InTransformType, bAffectChildren);
				}
				else
				{
					if (bCanSwitch)
					{
						SwitchToParent(Element->GetKey(), PoseElement.ActiveParent);
					}
					SetTransform(TransformElement, PoseTransform, InTransformType, bAffectChildren);
				}
			}
			else if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(Element))
			{
				SetCurveValue(CurveElement, PoseElement.CurveValue);
			}
		}
	}
}

void UTLLRN_RigHierarchy::LinkPoseAdapter(TSharedPtr<FTLLRN_RigHierarchyPoseAdapter> InPoseAdapter)
{
	if(PoseAdapter)
	{
		PoseAdapter->PreUnlinked(this);
		PoseAdapter.Reset();
	}

	if(InPoseAdapter)
	{
		PoseAdapter = InPoseAdapter;
		PoseAdapter->PostLinked(this);
	}
}

void UTLLRN_RigHierarchy::Notify(ETLLRN_RigHierarchyNotification InNotifType, const FTLLRN_RigBaseElement* InElement)
{
	if(bSuspendNotifications)
	{
		return;
	}

	if (!IsValid(this))
	{
		return;
	}

	// if we are running a VM right now
	{
		FScopeLock Lock(&ExecuteContextLock);
		if(ExecuteContext != nullptr)
		{
			QueueNotification(InNotifType, InElement);
			return;
		}
	}
	

	if(QueuedNotifications.IsEmpty())
	{
		ModifiedEvent.Broadcast(InNotifType, this, InElement);
		if(ModifiedEventDynamic.IsBound())
		{
			FTLLRN_RigElementKey Key;
			if(InElement)
			{
				Key = InElement->GetKey();
			}
			ModifiedEventDynamic.Broadcast(InNotifType, this, Key);
		}
	}
	else
	{
		QueueNotification(InNotifType, InElement);
		SendQueuedNotifications();
	}

#if WITH_EDITOR

	// certain events needs to be forwarded to the listening hierarchies.
	// this mainly has to do with topological change within the hierarchy.
	switch (InNotifType)
	{
		case ETLLRN_RigHierarchyNotification::ElementAdded:
		case ETLLRN_RigHierarchyNotification::ElementRemoved:
		case ETLLRN_RigHierarchyNotification::ElementRenamed:
		case ETLLRN_RigHierarchyNotification::ParentChanged:
		case ETLLRN_RigHierarchyNotification::ParentWeightsChanged:
		{
			if (ensure(InElement != nullptr))
			{
				ForEachListeningHierarchy([this, InNotifType, InElement](const FTLLRN_RigHierarchyListener& Listener)
				{
					if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
					{			
						if(const FTLLRN_RigBaseElement* ListeningElement = ListeningHierarchy->Find( InElement->GetKey()))
						{
							ListeningHierarchy->Notify(InNotifType, ListeningElement);
						}
					}
				});
			}
			break;
		}
		default:
		{
			break;
		}
	}

#endif
}

void UTLLRN_RigHierarchy::QueueNotification(ETLLRN_RigHierarchyNotification InNotification, const FTLLRN_RigBaseElement* InElement)
{
	FQueuedNotification Entry;
	Entry.Type = InNotification;
	Entry.Key = InElement ? InElement->GetKey() : FTLLRN_RigElementKey();
	QueuedNotifications.Enqueue(Entry);
}

void UTLLRN_RigHierarchy::SendQueuedNotifications()
{
	if(bSuspendNotifications)
	{
		QueuedNotifications.Empty();
		return;
	}

	if(QueuedNotifications.IsEmpty())
	{
		return;
	}

	{
		FScopeLock Lock(&ExecuteContextLock);
    	if(ExecuteContext != nullptr)
    	{
    		return;
    	}
	}
	
	// enable access to the controller during this method
	FTLLRN_RigHierarchyEnableControllerBracket EnableController(this, true);

	// we'll collect all notifications and will clean them up
	// to guard against notification storms.
	TArray<FQueuedNotification> AllNotifications;
	FQueuedNotification EntryFromQueue;
	while(QueuedNotifications.Dequeue(EntryFromQueue))
	{
		AllNotifications.Add(EntryFromQueue);
	}
	QueuedNotifications.Empty();

	// now we'll filter the notifications. we'll go through them in
	// reverse and will skip any aggregates (like change color multiple times on the same thing)
	// as well as collapse pairs such as select and deselect
	TArray<FQueuedNotification> FilteredNotifications;
	TArray<FQueuedNotification> UniqueNotifications;
	for(int32 Index = AllNotifications.Num() - 1; Index >= 0; Index--)
	{
		const FQueuedNotification& Entry = AllNotifications[Index];

		bool bSkipNotification = false;
		switch(Entry.Type)
		{
			case ETLLRN_RigHierarchyNotification::HierarchyReset:
			case ETLLRN_RigHierarchyNotification::ElementRemoved:
			case ETLLRN_RigHierarchyNotification::ElementRenamed:
			{
				// we don't allow these to happen during the run of a VM
				if(const UTLLRN_RigHierarchyController* Controller = GetController())
				{
					static constexpr TCHAR InvalidNotificationReceivedMessage[] =
						TEXT("Found invalid queued notification %s - %s. Skipping notification.");
					const FString NotificationText = StaticEnum<ETLLRN_RigHierarchyNotification>()->GetNameStringByValue((int64)Entry.Type);
					
					Controller->ReportErrorf(InvalidNotificationReceivedMessage, *NotificationText, *Entry.Key.ToString());	
				}
				bSkipNotification = true;
				break;
			}
			case ETLLRN_RigHierarchyNotification::ControlSettingChanged:
			case ETLLRN_RigHierarchyNotification::ControlVisibilityChanged:
			case ETLLRN_RigHierarchyNotification::ControlDrivenListChanged:
			case ETLLRN_RigHierarchyNotification::ControlShapeTransformChanged:
			case ETLLRN_RigHierarchyNotification::ParentChanged:
			case ETLLRN_RigHierarchyNotification::ParentWeightsChanged:
			{
				// these notifications are aggregates - they don't need to happen
				// more than once during an update
				bSkipNotification = UniqueNotifications.Contains(Entry);
				break;
			}
			case ETLLRN_RigHierarchyNotification::ElementSelected:
			case ETLLRN_RigHierarchyNotification::ElementDeselected:
			{
				FQueuedNotification OppositeEntry;
				OppositeEntry.Type = (Entry.Type == ETLLRN_RigHierarchyNotification::ElementSelected) ?
					ETLLRN_RigHierarchyNotification::ElementDeselected :
					ETLLRN_RigHierarchyNotification::ElementSelected;
				OppositeEntry.Key = Entry.Key;

				// we don't need to add this if we already performed the selection or
				// deselection of the same item before
				bSkipNotification = UniqueNotifications.Contains(Entry) ||
					UniqueNotifications.Contains(OppositeEntry);
				break;
			}
			case ETLLRN_RigHierarchyNotification::Max:
			{
				bSkipNotification = true;
				break;
			}
			case ETLLRN_RigHierarchyNotification::ElementAdded:
			case ETLLRN_RigHierarchyNotification::InteractionBracketOpened:
			case ETLLRN_RigHierarchyNotification::InteractionBracketClosed:
			default:
			{
				break;
			}
		}

		UniqueNotifications.AddUnique(Entry);
		if(!bSkipNotification)
		{
			FilteredNotifications.Add(Entry);
		}

		// if we ever hit a reset then we don't need to deal with
		// any previous notifications.
		if(Entry.Type == ETLLRN_RigHierarchyNotification::HierarchyReset)
		{
			break;
		}
	}

	if(FilteredNotifications.IsEmpty())
	{
		return;
	}

	ModifiedEvent.Broadcast(ETLLRN_RigHierarchyNotification::InteractionBracketOpened, this, nullptr);
	if(ModifiedEventDynamic.IsBound())
	{
		ModifiedEventDynamic.Broadcast(ETLLRN_RigHierarchyNotification::InteractionBracketOpened, this, FTLLRN_RigElementKey());
	}

	// finally send all of the notifications
	// (they have been added in the reverse order to the array)
	for(int32 Index = FilteredNotifications.Num() - 1; Index >= 0; Index--)
	{
		const FQueuedNotification& Entry = FilteredNotifications[Index];
		const FTLLRN_RigBaseElement* Element = Find(Entry.Key);
		if(Element)
		{
			ModifiedEvent.Broadcast(Entry.Type, this, Element);
			if(ModifiedEventDynamic.IsBound())
			{
				ModifiedEventDynamic.Broadcast(Entry.Type, this, Element->GetKey());
			}
		}
	}

	ModifiedEvent.Broadcast(ETLLRN_RigHierarchyNotification::InteractionBracketClosed, this, nullptr);
	if(ModifiedEventDynamic.IsBound())
	{
		ModifiedEventDynamic.Broadcast(ETLLRN_RigHierarchyNotification::InteractionBracketClosed, this, FTLLRN_RigElementKey());
	}
}

FTransform UTLLRN_RigHierarchy::GetTransform(FTLLRN_RigTransformElement* InTransformElement,
	const ETLLRN_RigTransformType::Type InTransformType) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InTransformElement == nullptr)
	{
		return FTransform::Identity;
	}

#if WITH_EDITOR
	{
		FScopeLock Lock(&ExecuteContextLock);
		if(bRecordTransformsAtRuntime && ExecuteContext != nullptr)
		{
			ReadTransformsAtRuntime.Emplace(
				ExecuteContext->GetPublicData<>().GetInstructionIndex(),
				ExecuteContext->GetSlice().GetIndex(),
				InTransformElement->GetIndex(),
				InTransformType
			);
		}
	}
	
	TGuardValue<bool> RecordTransformsPerInstructionGuard(bRecordTransformsAtRuntime, false);
	
#endif
	
	if(InTransformElement->GetDirtyState().IsDirty(InTransformType))
	{
		const ETLLRN_RigTransformType::Type OpposedType = SwapLocalAndGlobal(InTransformType);
		const ETLLRN_RigTransformType::Type GlobalType = MakeGlobal(InTransformType);
		ensure(!InTransformElement->GetDirtyState().IsDirty(OpposedType));

		FTransform ParentTransform;
		if(IsLocal(InTransformType))
		{
			// if we have a zero scale provided - and the parent also contains a zero scale,
			// we'll keep the local translation and scale since otherwise we'll loose the values.
			// we cannot compute the local from the global if the scale is 0 - since the local scale
			// may be anything - any translation or scale multiplied with the parent's zero scale is zero. 
			auto CompensateZeroScale = [this, InTransformElement, InTransformType](FTransform& Transform)
			{
				const FVector Scale = Transform.GetScale3D();
				if(FMath::IsNearlyZero(Scale.X) || FMath::IsNearlyZero(Scale.Y) || FMath::IsNearlyZero(Scale.Z))
				{
					const FTransform ParentTransform =
						GetParentTransform(InTransformElement, ETLLRN_RigTransformType::SwapLocalAndGlobal(InTransformType));
					const FVector ParentScale = ParentTransform.GetScale3D();
					if(FMath::IsNearlyZero(ParentScale.X) || FMath::IsNearlyZero(ParentScale.Y) || FMath::IsNearlyZero(ParentScale.Z))
					{
						const FTransform& InputTransform = InTransformElement->GetTransform().Get(InTransformType); 
						Transform.SetTranslation(InputTransform.GetTranslation());
						Transform.SetScale3D(InputTransform.GetScale3D());
					}
				}
			};
			
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InTransformElement))
			{
				FTransform NewTransform = ComputeLocalControlValue(ControlElement, ControlElement->GetTransform().Get(OpposedType), GlobalType);
				CompensateZeroScale(NewTransform);
				ControlElement->GetTransform().Set(InTransformType, NewTransform);
				ControlElement->GetDirtyState().MarkClean(InTransformType);
				/** from mikez we do not want geting a pose to set these preferred angles
				switch(ControlElement->Settings.ControlType)
				{
					case ETLLRN_RigControlType::Rotator:
					case ETLLRN_RigControlType::EulerTransform:
					case ETLLRN_RigControlType::Transform:
					case ETLLRN_RigControlType::TransformNoScale:
					{
						ControlElement->PreferredEulerAngles.SetRotator(NewTransform.Rotator(), IsInitial(InTransformType), true);
						break;
					}
					default:
					{
						break;
					}
					
				}*/
			}
			else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InTransformElement))
			{
				// this is done for nulls and any element that can have more than one parent which 
				// is not a control
				const FTransform& GlobalTransform = MultiParentElement->GetTransform().Get(GlobalType);
				FTransform LocalTransform = InverseSolveParentConstraints(
					GlobalTransform, 
					MultiParentElement->ParentConstraints, GlobalType, FTransform::Identity);
				CompensateZeroScale(LocalTransform);
				MultiParentElement->GetTransform().Set(InTransformType, LocalTransform);
				MultiParentElement->GetDirtyState().MarkClean(InTransformType);
			}
			else
			{
				ParentTransform = GetParentTransform(InTransformElement, GlobalType);

				FTransform NewTransform = InTransformElement->GetTransform().Get(OpposedType).GetRelativeTransform(ParentTransform);
				NewTransform.NormalizeRotation();
				CompensateZeroScale(NewTransform);
				InTransformElement->GetTransform().Set(InTransformType, NewTransform);
				InTransformElement->GetDirtyState().MarkClean(InTransformType);
			}
		}
		else
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InTransformElement))
			{
				// using GetControlOffsetTransform to check dirty flag before accessing the transform
				// note: no need to do the same for Pose.Local because there is already an ensure:
				// "ensure(!InTransformElement->Pose.IsDirty(OpposedType));" above
				const FTransform NewTransform = SolveParentConstraints(
					ControlElement->ParentConstraints, InTransformType,
					GetControlOffsetTransform(ControlElement, OpposedType), true,
					ControlElement->GetTransform().Get(OpposedType), true);
				ControlElement->GetTransform().Set(InTransformType, NewTransform);
				ControlElement->GetDirtyState().MarkClean(InTransformType);
			}
			else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InTransformElement))
			{
				// this is done for nulls and any element that can have more than one parent which 
				// is not a control
				const FTransform NewTransform = SolveParentConstraints(
					MultiParentElement->ParentConstraints, InTransformType,
					FTransform::Identity, false,
					MultiParentElement->GetTransform().Get(OpposedType), true);
				MultiParentElement->GetTransform().Set(InTransformType, NewTransform);
				MultiParentElement->GetDirtyState().MarkClean(InTransformType);
			}
			else
			{
				ParentTransform = GetParentTransform(InTransformElement, GlobalType);

				FTransform NewTransform = InTransformElement->GetTransform().Get(OpposedType) * ParentTransform;
				NewTransform.NormalizeRotation();
				InTransformElement->GetTransform().Set(InTransformType, NewTransform);
				InTransformElement->GetDirtyState().MarkClean(InTransformType);
			}
		}

		EnsureCacheValidity();
	}
	return InTransformElement->GetTransform().Get(InTransformType);
}

void UTLLRN_RigHierarchy::SetTransform(FTLLRN_RigTransformElement* InTransformElement, const FTransform& InTransform, const ETLLRN_RigTransformType::Type InTransformType, bool bAffectChildren, bool bSetupUndo, bool bForce, bool bPrintPythonCommands)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InTransformElement == nullptr)
	{
		return;
	}

	if(IsGlobal(InTransformType))
	{
		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InTransformElement))
		{
			FTransform LocalTransform = ComputeLocalControlValue(ControlElement, InTransform, InTransformType);
			ControlElement->Settings.ApplyLimits(LocalTransform);
			SetTransform(ControlElement, LocalTransform, MakeLocal(InTransformType), bAffectChildren, false, false, bPrintPythonCommands);
			return;
		}
	}

#if WITH_EDITOR

	// lock execute context scope
	{
		FScopeLock Lock(&ExecuteContextLock);
	
		if(bRecordTransformsAtRuntime && ExecuteContext)
		{
			const FRigVMExecuteContext& PublicData = ExecuteContext->GetPublicData<>();
			const FRigVMSlice& Slice = ExecuteContext->GetSlice();
			WrittenTransformsAtRuntime.Emplace(
				PublicData.GetInstructionIndex(),
				Slice.GetIndex(),
				InTransformElement->GetIndex(),
				InTransformType
			);

			// if we are setting a control / null parent after a child here - let's let the user know
			if(InTransformElement->IsA<FTLLRN_RigControlElement>() || InTransformElement->IsA<FTLLRN_RigNullElement>())
			{
				if(const UWorld* World = GetWorld())
				{
					// only fire these notes if we are inside the asset editor
					if(World->WorldType == EWorldType::EditorPreview)
					{
						for (FTLLRN_RigBaseElement* Child : GetChildren(InTransformElement))
						{
							const bool bChildFound = WrittenTransformsAtRuntime.ContainsByPredicate([Child](const TInstructionSliceElement& Entry) -> bool
							{
								return Entry.Get<2>() == Child->GetIndex();
							});

							if(bChildFound)
							{
								const FTLLRN_ControlRigExecuteContext& CRContext = ExecuteContext->GetPublicData<FTLLRN_ControlRigExecuteContext>();
								if(CRContext.GetLog())
								{
									static constexpr TCHAR MessageFormat[] = TEXT("Setting transform of parent (%s) after setting child (%s).\nThis may lead to unexpected results.");
									const FString& Message = FString::Printf(
										MessageFormat,
										*InTransformElement->GetName(),
										*Child->GetName());
									CRContext.Report(
										EMessageSeverity::Info,
										ExecuteContext->GetPublicData<>().GetFunctionName(),
										ExecuteContext->GetPublicData<>().GetInstructionIndex(),
										Message);
								}
							}
						}
					}
				}
			}
		}
	}
	
	TGuardValue<bool> RecordTransformsPerInstructionGuard(bRecordTransformsAtRuntime, false);
	
#endif

	if(!InTransformElement->GetDirtyState().IsDirty(InTransformType))
	{
		const FTransform PreviousTransform = InTransformElement->GetTransform().Get(InTransformType);
		if(!bForce && FTLLRN_RigComputedTransform::Equals(PreviousTransform, InTransform))
		{
			return;
		}
	}

	const FTransform PreviousTransform = GetTransform(InTransformElement, InTransformType);
	PropagateDirtyFlags(InTransformElement, ETLLRN_RigTransformType::IsInitial(InTransformType), bAffectChildren);

	const ETLLRN_RigTransformType::Type OpposedType = SwapLocalAndGlobal(InTransformType);
	InTransformElement->GetTransform().Set(InTransformType, InTransform);
	InTransformElement->GetDirtyState().MarkClean(InTransformType);
	InTransformElement->GetDirtyState().MarkDirty(OpposedType);
	IncrementPoseVersion(InTransformElement->Index);

	if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InTransformElement))
	{
		ControlElement->GetShapeDirtyState().MarkDirty(MakeGlobal(InTransformType));

		if(bUsePreferredEulerAngles && ETLLRN_RigTransformType::IsLocal(InTransformType))
		{
			const bool bInitial = ETLLRN_RigTransformType::IsInitial(InTransformType);
			const FVector Angle = GetControlAnglesFromQuat(ControlElement, InTransform.GetRotation(), true);
			ControlElement->PreferredEulerAngles.SetAngles(Angle, bInitial, ControlElement->PreferredEulerAngles.RotationOrder, true);
		}
	}

	EnsureCacheValidity();
	
#if WITH_EDITOR
	if(bSetupUndo || IsTracingChanges())
	{
		PushTransformToStack(
			InTransformElement->GetKey(),
			ETLLRN_RigTransformStackEntryType::TransformPose,
			InTransformType,
			PreviousTransform,
			InTransformElement->GetTransform().Get(InTransformType),
			bAffectChildren,
			bSetupUndo);
	}

	if (!bPropagatingChange)
	{
		TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);

		ForEachListeningHierarchy([this, InTransformElement, InTransform, InTransformType, bAffectChildren, bForce](const FTLLRN_RigHierarchyListener& Listener)
		{
			if(!bForcePropagation && !Listener.ShouldReactToChange(InTransformType))
			{
				return;
			}

			if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
			{			
				if(FTLLRN_RigTransformElement* ListeningElement = Cast<FTLLRN_RigTransformElement>(ListeningHierarchy->Find(InTransformElement->GetKey())))
				{
					// bSetupUndo = false such that all listening hierarchies performs undo at the same time the root hierachy undos
					ListeningHierarchy->SetTransform(ListeningElement, InTransform, InTransformType, bAffectChildren, false, bForce);
				}
			}
		});
	}

	if (bPrintPythonCommands)
	{
		FString BlueprintName;
		if (UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			BlueprintName = Blueprint->GetFName().ToString();
		}
		else if (UTLLRN_ControlRig* Rig = Cast<UTLLRN_ControlRig>(GetOuter()))
		{
			if (UBlueprint* BlueprintCR = Cast<UBlueprint>(Rig->GetClass()->ClassGeneratedBy))
			{
				BlueprintName = BlueprintCR->GetFName().ToString();
			}
		}
		if (!BlueprintName.IsEmpty())
		{
			FString MethodName;
			switch (InTransformType)
			{
				case ETLLRN_RigTransformType::InitialLocal: 
				case ETLLRN_RigTransformType::CurrentLocal:
				{
					MethodName = TEXT("set_local_transform");
					break;
				}
				case ETLLRN_RigTransformType::InitialGlobal: 
				case ETLLRN_RigTransformType::CurrentGlobal:
				{
					MethodName = TEXT("set_global_transform");
					break;
				}
			}

			RigVMPythonUtils::Print(BlueprintName,
				FString::Printf(TEXT("hierarchy.%s(%s, %s, %s, %s)"),
				*MethodName,
				*InTransformElement->GetKey().ToPythonString(),
				*RigVMPythonUtils::TransformToPythonString(InTransform),
				(InTransformType == ETLLRN_RigTransformType::InitialGlobal || InTransformType == ETLLRN_RigTransformType::InitialLocal) ? TEXT("True") : TEXT("False"),
				(bAffectChildren) ? TEXT("True") : TEXT("False")));
		}
	}
#endif
}

FTransform UTLLRN_RigHierarchy::GetControlOffsetTransform(FTLLRN_RigControlElement* InControlElement,
	const ETLLRN_RigTransformType::Type InTransformType) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InControlElement == nullptr)
	{
		return FTransform::Identity;
	}

#if WITH_EDITOR

	// lock execute context scope
	{
		FScopeLock Lock(&ExecuteContextLock);
		if(bRecordTransformsAtRuntime && ExecuteContext)
		{
			ReadTransformsAtRuntime.Emplace(
				ExecuteContext->GetPublicData<>().GetInstructionIndex(),
				ExecuteContext->GetSlice().GetIndex(),
				InControlElement->GetIndex(),
				InTransformType
			);
		}
	}
	
	TGuardValue<bool> RecordTransformsPerInstructionGuard(bRecordTransformsAtRuntime, false);
	
#endif

	if(InControlElement->GetOffsetDirtyState().IsDirty(InTransformType))
	{
		const ETLLRN_RigTransformType::Type OpposedType = SwapLocalAndGlobal(InTransformType);
		const ETLLRN_RigTransformType::Type GlobalType = MakeGlobal(InTransformType);
		ensure(!InControlElement->GetOffsetDirtyState().IsDirty(OpposedType));

		if(IsLocal(InTransformType))
		{
			const FTransform& GlobalTransform = InControlElement->GetOffsetTransform().Get(GlobalType);
			const FTransform LocalTransform = InverseSolveParentConstraints(
				GlobalTransform, 
				InControlElement->ParentConstraints, GlobalType, FTransform::Identity);
			InControlElement->GetOffsetTransform().Set(InTransformType, LocalTransform);
			InControlElement->GetOffsetDirtyState().MarkClean(InTransformType);

			if(bEnableCacheValidityCheck)
			{
				const FTransform ComputedTransform = SolveParentConstraints(
					InControlElement->ParentConstraints, MakeGlobal(InTransformType),
					LocalTransform, true,
					FTransform::Identity, false);

				const TArray<FString>& TransformTypeStrings = GetTransformTypeStrings();

				checkf(FTLLRN_RigComputedTransform::Equals(GlobalTransform, ComputedTransform),
					TEXT("Element '%s' Offset %s Cached vs Computed doesn't match. ('%s' <-> '%s')"),
					*InControlElement->GetName(),
					*TransformTypeStrings[(int32)InTransformType],
					*GlobalTransform.ToString(), *ComputedTransform.ToString());
			}
		}
		else
		{
			const FTransform& LocalTransform = InControlElement->GetOffsetTransform().Get(OpposedType); 
			const FTransform GlobalTransform = SolveParentConstraints(
				InControlElement->ParentConstraints, InTransformType,
				LocalTransform, true,
				FTransform::Identity, false);
			InControlElement->GetOffsetTransform().Set(InTransformType, GlobalTransform);
			InControlElement->GetOffsetDirtyState().MarkClean(InTransformType);

			if(bEnableCacheValidityCheck)
			{
				const FTransform ComputedTransform = InverseSolveParentConstraints(
					GlobalTransform, 
					InControlElement->ParentConstraints, GlobalType, FTransform::Identity);

				const TArray<FString>& TransformTypeStrings = GetTransformTypeStrings();

				checkf(FTLLRN_RigComputedTransform::Equals(LocalTransform, ComputedTransform),
					TEXT("Element '%s' Offset %s Cached vs Computed doesn't match. ('%s' <-> '%s')"),
					*InControlElement->GetName(),
					*TransformTypeStrings[(int32)InTransformType],
					*LocalTransform.ToString(), *ComputedTransform.ToString());
			}
		}

		EnsureCacheValidity();
	}
	return InControlElement->GetOffsetTransform().Get(InTransformType);
}

void UTLLRN_RigHierarchy::SetControlOffsetTransform(FTLLRN_RigControlElement* InControlElement, const FTransform& InTransform,
                                              const ETLLRN_RigTransformType::Type InTransformType, bool bAffectChildren, bool bSetupUndo, bool bForce, bool bPrintPythonCommands)
{
	if(InControlElement == nullptr)
	{
		return;
	}

#if WITH_EDITOR

	// lock execute context scope
	{
		FScopeLock Lock(&ExecuteContextLock);
		if(bRecordTransformsAtRuntime && ExecuteContext)
		{
			WrittenTransformsAtRuntime.Emplace(
				ExecuteContext->GetPublicData<>().GetInstructionIndex(),
				ExecuteContext->GetSlice().GetIndex(),
				InControlElement->GetIndex(),
				InTransformType
			);
		}
	}
	
	TGuardValue<bool> RecordTransformsPerInstructionGuard(bRecordTransformsAtRuntime, false);
	
#endif

	if(!InControlElement->GetOffsetDirtyState().IsDirty(InTransformType))
	{
		const FTransform PreviousTransform = InControlElement->GetOffsetTransform().Get(InTransformType);
		if(!bForce && FTLLRN_RigComputedTransform::Equals(PreviousTransform, InTransform))
		{
			return;
		}
	}
	
	const FTransform PreviousTransform = GetControlOffsetTransform(InControlElement, InTransformType);
	PropagateDirtyFlags(InControlElement, ETLLRN_RigTransformType::IsInitial(InTransformType), bAffectChildren);

	GetTransform(InControlElement, MakeLocal(InTransformType));
	InControlElement->GetDirtyState().MarkDirty(MakeGlobal(InTransformType));

	const ETLLRN_RigTransformType::Type OpposedType = SwapLocalAndGlobal(InTransformType);
	InControlElement->GetOffsetTransform().Set(InTransformType, InTransform);
	InControlElement->GetOffsetDirtyState().MarkClean(InTransformType);
	InControlElement->GetOffsetDirtyState().MarkDirty(OpposedType);
	InControlElement->GetShapeDirtyState().MarkDirty(MakeGlobal(InTransformType));

	EnsureCacheValidity();

	if (ETLLRN_RigTransformType::IsInitial(InTransformType))
	{
		// control's offset transform is considered a special type of transform
		// whenever its initial value is changed, we want to make sure the current is kept in sync
		// such that the viewport can reflect this change
		SetControlOffsetTransform(InControlElement, InTransform, ETLLRN_RigTransformType::MakeCurrent(InTransformType), bAffectChildren, false, bForce);
	}
	

#if WITH_EDITOR
	if(bSetupUndo || IsTracingChanges())
	{
		PushTransformToStack(
            InControlElement->GetKey(),
            ETLLRN_RigTransformStackEntryType::ControlOffset,
            InTransformType,
            PreviousTransform,
            InControlElement->GetOffsetTransform().Get(InTransformType),
            bAffectChildren,
            bSetupUndo);
	}

	if (!bPropagatingChange)
	{
		TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);
			
		ForEachListeningHierarchy([this, InControlElement, InTransform, InTransformType, bAffectChildren, bForce](const FTLLRN_RigHierarchyListener& Listener)
		{
			if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
			{	
				if(FTLLRN_RigControlElement* ListeningElement = Cast<FTLLRN_RigControlElement>(ListeningHierarchy->Find(InControlElement->GetKey())))
				{
					// bSetupUndo = false such that all listening hierarchies performs undo at the same time the root hierachy undos
					ListeningHierarchy->SetControlOffsetTransform(ListeningElement, InTransform, InTransformType, bAffectChildren, false, bForce);
				}
			}
		});
	}

	if (bPrintPythonCommands)
	{
		FString BlueprintName;
		if (UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			BlueprintName = Blueprint->GetFName().ToString();
		}
		else if (UTLLRN_ControlRig* Rig = Cast<UTLLRN_ControlRig>(GetOuter()))
		{
			if (UBlueprint* BlueprintCR = Cast<UBlueprint>(Rig->GetClass()->ClassGeneratedBy))
			{
				BlueprintName = BlueprintCR->GetFName().ToString();
			}
		}
		if (!BlueprintName.IsEmpty())
		{
			RigVMPythonUtils::Print(BlueprintName,
				FString::Printf(TEXT("hierarchy.set_control_offset_transform(%s, %s, %s, %s)"),
				*InControlElement->GetKey().ToPythonString(),
				*RigVMPythonUtils::TransformToPythonString(InTransform),
				(ETLLRN_RigTransformType::IsInitial(InTransformType)) ? TEXT("True") : TEXT("False"),
				(bAffectChildren) ? TEXT("True") : TEXT("False")));
		}
	}
#endif
}

FTransform UTLLRN_RigHierarchy::GetControlShapeTransform(FTLLRN_RigControlElement* InControlElement,
	const ETLLRN_RigTransformType::Type InTransformType) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InControlElement == nullptr)
	{
		return FTransform::Identity;
	}
	
	if(InControlElement->GetShapeDirtyState().IsDirty(InTransformType))
	{
		const ETLLRN_RigTransformType::Type OpposedType = SwapLocalAndGlobal(InTransformType);
		const ETLLRN_RigTransformType::Type GlobalType = MakeGlobal(InTransformType);
		ensure(!InControlElement->GetShapeDirtyState().IsDirty(OpposedType));

		const FTransform ParentTransform = GetTransform(InControlElement, GlobalType);
		if(IsLocal(InTransformType))
		{
			FTransform LocalTransform = InControlElement->GetShapeTransform().Get(OpposedType).GetRelativeTransform(ParentTransform);
			LocalTransform.NormalizeRotation();
			InControlElement->GetShapeTransform().Set(InTransformType, LocalTransform);
			InControlElement->GetShapeDirtyState().MarkClean(InTransformType);
		}
		else
		{
			FTransform GlobalTransform = InControlElement->GetShapeTransform().Get(OpposedType) * ParentTransform;
			GlobalTransform.NormalizeRotation();
			InControlElement->GetShapeTransform().Set(InTransformType, GlobalTransform);
			InControlElement->GetShapeDirtyState().MarkClean(InTransformType);
		}

		EnsureCacheValidity();
	}
	return InControlElement->GetShapeTransform().Get(InTransformType);
}

void UTLLRN_RigHierarchy::SetControlShapeTransform(FTLLRN_RigControlElement* InControlElement, const FTransform& InTransform,
	const ETLLRN_RigTransformType::Type InTransformType, bool bSetupUndo, bool bForce, bool bPrintPythonCommands)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InControlElement == nullptr)
	{
		return;
	}

	if(!InControlElement->GetShapeDirtyState().IsDirty(InTransformType))
	{
		const FTransform PreviousTransform = InControlElement->GetShapeTransform().Get(InTransformType);
		if(!bForce && FTLLRN_RigComputedTransform::Equals(PreviousTransform, InTransform))
		{
			return;
		}
	}

	const FTransform PreviousTransform = GetControlShapeTransform(InControlElement, InTransformType);
	const ETLLRN_RigTransformType::Type OpposedType = SwapLocalAndGlobal(InTransformType);
	InControlElement->GetShapeTransform().Set(InTransformType, InTransform);
	InControlElement->GetShapeDirtyState().MarkClean(InTransformType);
	InControlElement->GetShapeDirtyState().MarkDirty(OpposedType);

	if (IsInitial(InTransformType))
	{
		// control's shape transform, similar to offset transform, is considered a special type of transform
		// whenever its initial value is changed, we want to make sure the current is kept in sync
		// such that the viewport can reflect this change
		SetControlShapeTransform(InControlElement, InTransform, ETLLRN_RigTransformType::MakeCurrent(InTransformType), false, bForce);
	}
	
	EnsureCacheValidity();
	
#if WITH_EDITOR
	if(bSetupUndo || IsTracingChanges())
	{
		PushTransformToStack(
            InControlElement->GetKey(),
            ETLLRN_RigTransformStackEntryType::ControlShape,
            InTransformType,
            PreviousTransform,
            InControlElement->GetShapeTransform().Get(InTransformType),
            false,
            bSetupUndo);
	}
#endif

	if(IsLocal(InTransformType))
	{
		Notify(ETLLRN_RigHierarchyNotification::ControlShapeTransformChanged, InControlElement);
	}

#if WITH_EDITOR
	if (!bPropagatingChange)
	{
		TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);
			
		ForEachListeningHierarchy([this, InControlElement, InTransform, InTransformType, bForce](const FTLLRN_RigHierarchyListener& Listener)
		{
			if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
			{	
				if(FTLLRN_RigControlElement* ListeningElement = Cast<FTLLRN_RigControlElement>(ListeningHierarchy->Find(InControlElement->GetKey())))
				{
					// bSetupUndo = false such that all listening hierarchies performs undo at the same time the root hierachy undos
					ListeningHierarchy->SetControlShapeTransform(ListeningElement, InTransform, InTransformType, false, bForce);
				}
			}
		});
	}

	if (bPrintPythonCommands)
	{
		FString BlueprintName;
		if (UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			BlueprintName = Blueprint->GetFName().ToString();
		}
		else if (UTLLRN_ControlRig* Rig = Cast<UTLLRN_ControlRig>(GetOuter()))
		{
			if (UBlueprint* BlueprintCR = Cast<UBlueprint>(Rig->GetClass()->ClassGeneratedBy))
			{
				BlueprintName = BlueprintCR->GetFName().ToString();
			}
		}
		if (!BlueprintName.IsEmpty())
		{
			RigVMPythonUtils::Print(BlueprintName,
				FString::Printf(TEXT("hierarchy.set_control_shape_transform(%s, %s, %s)"),
				*InControlElement->GetKey().ToPythonString(),
				*RigVMPythonUtils::TransformToPythonString(InTransform),
				ETLLRN_RigTransformType::IsInitial(InTransformType) ? TEXT("True") : TEXT("False")));
		}
	
	}
#endif
}

void UTLLRN_RigHierarchy::SetControlSettings(FTLLRN_RigControlElement* InControlElement, FTLLRN_RigControlSettings InSettings, bool bSetupUndo, bool bForce, bool bPrintPythonCommands)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InControlElement == nullptr)
	{
		return;
	}

	const FTLLRN_RigControlSettings PreviousSettings = InControlElement->Settings;
	if(!bForce && PreviousSettings == InSettings)
	{
		return;
	}

	if(bSetupUndo && !HasAnyFlags(RF_Transient))
	{
		Modify();
	}

	InControlElement->Settings = InSettings;
	Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, InControlElement);
	
#if WITH_EDITOR
	if (!bPropagatingChange)
	{
		TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);
			
		ForEachListeningHierarchy([this, InControlElement, InSettings, bForce](const FTLLRN_RigHierarchyListener& Listener)
		{
			if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
			{	
				if(FTLLRN_RigControlElement* ListeningElement = Cast<FTLLRN_RigControlElement>(ListeningHierarchy->Find(InControlElement->GetKey())))
				{
					// bSetupUndo = false such that all listening hierarchies performs undo at the same time the root hierachy undos
					ListeningHierarchy->SetControlSettings(ListeningElement, InSettings, false, bForce);
				}
			}
		});
	}

	if (bPrintPythonCommands)
	{
		FString BlueprintName;
		if (UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			BlueprintName = Blueprint->GetFName().ToString();
		}
		else if (UTLLRN_ControlRig* Rig = Cast<UTLLRN_ControlRig>(GetOuter()))
		{
			if (UBlueprint* BlueprintCR = Cast<UBlueprint>(Rig->GetClass()->ClassGeneratedBy))
			{
				BlueprintName = BlueprintCR->GetFName().ToString();
			}
		}
		if (!BlueprintName.IsEmpty())
		{
			FString ControlNamePythonized = RigVMPythonUtils::PythonizeName(InControlElement->GetName());
			FString SettingsName = FString::Printf(TEXT("control_settings_%s"),
				*ControlNamePythonized);
			TArray<FString> Commands = ControlSettingsToPythonCommands(InControlElement->Settings, SettingsName);

			for (const FString& Command : Commands)
			{
				RigVMPythonUtils::Print(BlueprintName, Command);
			}
			
			RigVMPythonUtils::Print(BlueprintName,
				FString::Printf(TEXT("hierarchy.set_control_settings(%s, %s)"),
				*InControlElement->GetKey().ToPythonString(),
				*SettingsName));
		}
	}
#endif
}

FTransform UTLLRN_RigHierarchy::GetParentTransform(FTLLRN_RigBaseElement* InElement, const ETLLRN_RigTransformType::Type InTransformType) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(InElement))
	{
		return GetTransform(SingleParentElement->ParentElement, InTransformType);
	}
	else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InElement))
	{
		const FTransform OutputTransform = SolveParentConstraints(
			MultiParentElement->ParentConstraints,
			InTransformType,
			FTransform::Identity,
			false,
			FTransform::Identity,
			false
		);
		EnsureCacheValidity();
		return OutputTransform;
	}
	return FTransform::Identity;
}

FTLLRN_RigControlValue UTLLRN_RigHierarchy::GetControlValue(FTLLRN_RigControlElement* InControlElement, ETLLRN_RigControlValueType InValueType, bool bUsePreferredAngles) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	using namespace ETLLRN_RigTransformType;

	FTLLRN_RigControlValue Value;

	if(InControlElement != nullptr)
	{
		auto GetValueFromPreferredEulerAngles = [this, InControlElement, &Value, InValueType, bUsePreferredAngles]() -> bool
		{
			if (!bUsePreferredAngles)
			{
				return false;
			}
			
			const bool bInitial = InValueType == ETLLRN_RigControlValueType::Initial;
			switch(InControlElement->Settings.ControlType)
			{
				case ETLLRN_RigControlType::Rotator:
				{
					Value = MakeControlValueFromRotator(InControlElement->PreferredEulerAngles.GetRotator(bInitial)); 
					return true;
				}
				case ETLLRN_RigControlType::EulerTransform:
				{
					FEulerTransform EulerTransform(GetTransform(InControlElement, CurrentLocal));
					EulerTransform.Rotation = InControlElement->PreferredEulerAngles.GetRotator(bInitial);
					Value = MakeControlValueFromEulerTransform(EulerTransform); 
					return true;
				}
				default:
				{
					break;
				}
			}
			return false;
		};
		
		switch(InValueType)
		{
			case ETLLRN_RigControlValueType::Current:
			{
				if(GetValueFromPreferredEulerAngles())
				{
					break;
				}
					
				Value.SetFromTransform(
                    GetTransform(InControlElement, CurrentLocal),
                    InControlElement->Settings.ControlType,
                    InControlElement->Settings.PrimaryAxis
                );
				break;
			}
			case ETLLRN_RigControlValueType::Initial:
			{
				if(GetValueFromPreferredEulerAngles())
				{
					break;
				}

				Value.SetFromTransform(
                    GetTransform(InControlElement, InitialLocal),
                    InControlElement->Settings.ControlType,
                    InControlElement->Settings.PrimaryAxis
                );
				break;
			}
			case ETLLRN_RigControlValueType::Minimum:
			{
				return InControlElement->Settings.MinimumValue;
			}
			case ETLLRN_RigControlValueType::Maximum:
			{
				return InControlElement->Settings.MaximumValue;
			}
		}
	}
	return Value;
}

void UTLLRN_RigHierarchy::SetPreferredEulerAnglesFromValue(FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlValue& InValue, const ETLLRN_RigControlValueType& InValueType, const bool bFixEulerFlips)
{
	const bool bInitial = InValueType == ETLLRN_RigControlValueType::Initial;
	switch(InControlElement->Settings.ControlType)
	{
	case ETLLRN_RigControlType::Rotator:
		{
			InControlElement->PreferredEulerAngles.SetRotator(GetRotatorFromControlValue(InValue), bInitial, bFixEulerFlips);
			break;
		}
	case ETLLRN_RigControlType::EulerTransform:
		{
			FEulerTransform EulerTransform = GetEulerTransformFromControlValue(InValue);
			FQuat Quat = EulerTransform.GetRotation();
			const FVector Angle = GetControlAnglesFromQuat(InControlElement, Quat, bFixEulerFlips);
			InControlElement->PreferredEulerAngles.SetAngles(Angle, bInitial, InControlElement->PreferredEulerAngles.RotationOrder, bFixEulerFlips);
			break;
		}
	case ETLLRN_RigControlType::Transform:
		{
			FTransform Transform = GetTransformFromControlValue(InValue);
			FQuat Quat = Transform.GetRotation();
			const FVector Angle = GetControlAnglesFromQuat(InControlElement, Quat, bFixEulerFlips);
			InControlElement->PreferredEulerAngles.SetAngles(Angle, bInitial, InControlElement->PreferredEulerAngles.RotationOrder, bFixEulerFlips);
			break;
		}
	case ETLLRN_RigControlType::TransformNoScale:
		{
			FTransform Transform = GetTransformNoScaleFromControlValue(InValue);
			FQuat Quat = Transform.GetRotation();
			const FVector Angle = GetControlAnglesFromQuat(InControlElement, Quat, bFixEulerFlips);
			InControlElement->PreferredEulerAngles.SetAngles(Angle, bInitial, InControlElement->PreferredEulerAngles.RotationOrder, bFixEulerFlips);
			break;
		}
	default:
		{
			break;
		}
	}
};

void UTLLRN_RigHierarchy::SetControlValue(FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlValue& InValue, ETLLRN_RigControlValueType InValueType, bool bSetupUndo, bool bForce, bool bPrintPythonCommands, bool bFixEulerFlips)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	
	using namespace ETLLRN_RigTransformType;

	if(InControlElement != nullptr)
	{
		switch(InValueType)
		{
			case ETLLRN_RigControlValueType::Current:
			{
				FTLLRN_RigControlValue Value = InValue;
				InControlElement->Settings.ApplyLimits(Value);

				TGuardValue<bool> DontSetPreferredEulerAngle(bUsePreferredEulerAngles, false);
				SetTransform(
					InControlElement,
					Value.GetAsTransform(
						InControlElement->Settings.ControlType,
						InControlElement->Settings.PrimaryAxis
					),
					CurrentLocal,
					true,
					bSetupUndo,
					bForce,
					bPrintPythonCommands
				);
				if (bFixEulerFlips)
				{
					SetPreferredEulerAnglesFromValue(InControlElement, Value, InValueType, bFixEulerFlips);
				}
				break;
			}
			case ETLLRN_RigControlValueType::Initial:
			{
				FTLLRN_RigControlValue Value = InValue;
				InControlElement->Settings.ApplyLimits(Value);

				TGuardValue<bool> DontSetPreferredEulerAngle(bUsePreferredEulerAngles, false);
				SetTransform(
					InControlElement,
					Value.GetAsTransform(
						InControlElement->Settings.ControlType,
						InControlElement->Settings.PrimaryAxis
					),
					InitialLocal,
					true,
					bSetupUndo,
					bForce,
					bPrintPythonCommands
				);

				if (bFixEulerFlips)
				{
					SetPreferredEulerAnglesFromValue(InControlElement, Value, InValueType, bFixEulerFlips);
				}
				break;
			}
			case ETLLRN_RigControlValueType::Minimum:
			case ETLLRN_RigControlValueType::Maximum:
			{
				if(bSetupUndo)
				{
					Modify();
				}

				if(InValueType == ETLLRN_RigControlValueType::Minimum)
				{
					FTLLRN_RigControlSettings& Settings = InControlElement->Settings;
					Settings.MinimumValue = InValue;

					// Make sure the maximum value respects the new minimum value
					TArray<FTLLRN_RigControlLimitEnabled> NoMaxLimits = Settings.LimitEnabled;
					for (FTLLRN_RigControlLimitEnabled& NoMaxLimit : NoMaxLimits)
					{
						NoMaxLimit.bMaximum = false;
					}
					Settings.MaximumValue.ApplyLimits(NoMaxLimits, Settings.ControlType, Settings.MinimumValue, Settings.MaximumValue);
				}
				else
				{
					FTLLRN_RigControlSettings& Settings = InControlElement->Settings;
					Settings.MaximumValue = InValue;
					
					// Make sure the minimum value respects the new maximum value
					TArray<FTLLRN_RigControlLimitEnabled> NoMinLimits = Settings.LimitEnabled;
					for (FTLLRN_RigControlLimitEnabled& NoMinLimit : NoMinLimits)
					{
						NoMinLimit.bMinimum = false;
					}
					Settings.MinimumValue.ApplyLimits(NoMinLimits, Settings.ControlType, Settings.MinimumValue, Settings.MaximumValue);
				}
				
				Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, InControlElement);

#if WITH_EDITOR
				if (!bPropagatingChange)
				{
					TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);
			
					ForEachListeningHierarchy([this, InControlElement, InValue, InValueType, bForce](const FTLLRN_RigHierarchyListener& Listener)
					{
						if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
						{
							if(FTLLRN_RigControlElement* ListeningElement = Cast<FTLLRN_RigControlElement>(ListeningHierarchy->Find(InControlElement->GetKey())))
							{
								ListeningHierarchy->SetControlValue(ListeningElement, InValue, InValueType, false, bForce);
							}
						}
					});
				}

				if (bPrintPythonCommands)
				{
					FString BlueprintName;
					if (UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
					{
						BlueprintName = Blueprint->GetFName().ToString();
					}
					else if (UTLLRN_ControlRig* Rig = Cast<UTLLRN_ControlRig>(GetOuter()))
					{
						if (UBlueprint* BlueprintCR = Cast<UBlueprint>(Rig->GetClass()->ClassGeneratedBy))
						{
							BlueprintName = BlueprintCR->GetFName().ToString();
						}
					}
					if (!BlueprintName.IsEmpty())
					{
						RigVMPythonUtils::Print(BlueprintName,
							FString::Printf(TEXT("hierarchy.set_control_value(%s, %s, %s)"),
							*InControlElement->GetKey().ToPythonString(),
							*InValue.ToPythonString(InControlElement->Settings.ControlType),
							*RigVMPythonUtils::EnumValueToPythonString<ETLLRN_RigControlValueType>((int64)InValueType)));
					}
				}
#endif
				break;
			}
		}	
	}
}

void UTLLRN_RigHierarchy::SetControlVisibility(FTLLRN_RigControlElement* InControlElement, bool bVisibility)
{
	if(InControlElement == nullptr)
	{
		return;
	}

	if(InControlElement->Settings.SetVisible(bVisibility))
	{
		Notify(ETLLRN_RigHierarchyNotification::ControlVisibilityChanged, InControlElement);
	}

#if WITH_EDITOR
	if (!bPropagatingChange)
	{
		TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);
			
		ForEachListeningHierarchy([this, InControlElement, bVisibility](const FTLLRN_RigHierarchyListener& Listener)
		{
			if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
			{
				if(FTLLRN_RigControlElement* ListeningElement = Cast<FTLLRN_RigControlElement>(ListeningHierarchy->Find(InControlElement->GetKey())))
				{
					ListeningHierarchy->SetControlVisibility(ListeningElement, bVisibility);
				}
			}
		});
	}
#endif
}

void UTLLRN_RigHierarchy::SetConnectorSettings(FTLLRN_RigConnectorElement* InConnectorElement, FTLLRN_RigConnectorSettings InSettings,
	bool bSetupUndo, bool bForce, bool bPrintPythonCommands)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InConnectorElement == nullptr)
	{
		return;
	}

	const FTLLRN_RigConnectorSettings PreviousSettings = InConnectorElement->Settings;
	if(!bForce && PreviousSettings == InSettings)
	{
		return;
	}

	if (InSettings.Type == ETLLRN_ConnectorType::Primary)
	{
		if (InSettings.bOptional)
		{
			return;
		}
	}

	if(bSetupUndo && !HasAnyFlags(RF_Transient))
	{
		Modify();
	}

	InConnectorElement->Settings = InSettings;
	Notify(ETLLRN_RigHierarchyNotification::ConnectorSettingChanged, InConnectorElement);
	
#if WITH_EDITOR
	if (!bPropagatingChange)
	{
		TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);
			
		ForEachListeningHierarchy([this, InConnectorElement, InSettings, bForce](const FTLLRN_RigHierarchyListener& Listener)
		{
			if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
			{	
				if(FTLLRN_RigConnectorElement* ListeningElement = Cast<FTLLRN_RigConnectorElement>(ListeningHierarchy->Find(InConnectorElement->GetKey())))
				{
					// bSetupUndo = false such that all listening hierarchies performs undo at the same time the root hierachy undos
					ListeningHierarchy->SetConnectorSettings(ListeningElement, InSettings, false, bForce);
				}
			}
		});
	}

	if (bPrintPythonCommands)
	{
		FString BlueprintName;
		if (UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			BlueprintName = Blueprint->GetFName().ToString();
		}
		else if (UTLLRN_ControlRig* Rig = Cast<UTLLRN_ControlRig>(GetOuter()))
		{
			if (UBlueprint* BlueprintCR = Cast<UBlueprint>(Rig->GetClass()->ClassGeneratedBy))
			{
				BlueprintName = BlueprintCR->GetFName().ToString();
			}
		}
		if (!BlueprintName.IsEmpty())
		{
			FString ControlNamePythonized = RigVMPythonUtils::PythonizeName(InConnectorElement->GetName());
			FString SettingsName = FString::Printf(TEXT("connector_settings_%s"),
				*ControlNamePythonized);
			TArray<FString> Commands = ConnectorSettingsToPythonCommands(InConnectorElement->Settings, SettingsName);

			for (const FString& Command : Commands)
			{
				RigVMPythonUtils::Print(BlueprintName, Command);
			}
			
			RigVMPythonUtils::Print(BlueprintName,
				FString::Printf(TEXT("hierarchy.set_connector_settings(%s, %s)"),
				*InConnectorElement->GetKey().ToPythonString(),
				*SettingsName));
		}
	}
#endif
}


float UTLLRN_RigHierarchy::GetCurveValue(FTLLRN_RigCurveElement* InCurveElement) const
{
	if(InCurveElement == nullptr)
	{
		return 0.f;
	}
	return InCurveElement->bIsValueSet ? InCurveElement->Get() : 0.f;
}


bool UTLLRN_RigHierarchy::IsCurveValueSet(FTLLRN_RigCurveElement* InCurveElement) const
{
	return InCurveElement && InCurveElement->bIsValueSet;
}


void UTLLRN_RigHierarchy::SetCurveValue(FTLLRN_RigCurveElement* InCurveElement, float InValue, bool bSetupUndo, bool bForce)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InCurveElement == nullptr)
	{
		return;
	}

	// we need to record the no matter what since this may be in consecutive frames.
	// if the client code desires to set the value we need to remember it as changed,
	// even if the value matches with a previous set. the ChangedCurveIndices array
	// represents a list of values that were changed by the client.
	if(bRecordCurveChanges)
	{
		ChangedCurveIndices.Add(InCurveElement->GetIndex());
	}

	const bool bPreviousIsValueSet = InCurveElement->bIsValueSet; 
	const float PreviousValue = InCurveElement->Get();
	if(!bForce && InCurveElement->bIsValueSet && FMath::IsNearlyZero(PreviousValue - InValue))
	{
		return;
	}

	InCurveElement->Set(InValue, bRecordCurveChanges);

#if WITH_EDITOR
	if(bSetupUndo || IsTracingChanges())
	{
		PushCurveToStack(InCurveElement->GetKey(), PreviousValue, InCurveElement->Get(), bPreviousIsValueSet, true, bSetupUndo);
	}

	if (!bPropagatingChange)
	{
		TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);
			
		ForEachListeningHierarchy([this, InCurveElement, InValue, bForce](const FTLLRN_RigHierarchyListener& Listener)
		{
			if(!Listener.Hierarchy.IsValid())
			{
				return;
			}

			if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
			{
				if(FTLLRN_RigCurveElement* ListeningElement = Cast<FTLLRN_RigCurveElement>(ListeningHierarchy->Find(InCurveElement->GetKey())))
				{
					// bSetupUndo = false such that all listening hierarchies performs undo at the same time the root hierarchy undoes
					ListeningHierarchy->SetCurveValue(ListeningElement, InValue, false, bForce);
				}
			}
		});
	}
#endif
}


void UTLLRN_RigHierarchy::UnsetCurveValue(FTLLRN_RigCurveElement* InCurveElement, bool bSetupUndo, bool bForce)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(InCurveElement == nullptr)
	{
		return;
	}

	const bool bPreviousIsValueSet = InCurveElement->bIsValueSet; 
	if(!bForce && !InCurveElement->bIsValueSet)
	{
		return;
	}

	InCurveElement->bIsValueSet = false;
	ChangedCurveIndices.Remove(InCurveElement->GetIndex());

#if WITH_EDITOR
	if(bSetupUndo || IsTracingChanges())
	{
		PushCurveToStack(InCurveElement->GetKey(), InCurveElement->Get(), InCurveElement->Get(), bPreviousIsValueSet, false, bSetupUndo);
	}

	if (!bPropagatingChange)
	{
		TGuardValue<bool> bPropagatingChangeGuardValue(bPropagatingChange, true);
			
		ForEachListeningHierarchy([this, InCurveElement, bForce](const FTLLRN_RigHierarchyListener& Listener)
		{
			if(!Listener.Hierarchy.IsValid())
			{
				return;
			}

			if (UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
			{
				if(FTLLRN_RigCurveElement* ListeningElement = Cast<FTLLRN_RigCurveElement>(ListeningHierarchy->Find(InCurveElement->GetKey())))
				{
					// bSetupUndo = false such that all listening hierarchies performs undo at the same time the root hierarchy undoes
					ListeningHierarchy->UnsetCurveValue(ListeningElement, false, bForce);
				}
			}
		});
	}
#endif
}


FName UTLLRN_RigHierarchy::GetPreviousName(const FTLLRN_RigElementKey& InKey) const
{
	if(const FTLLRN_RigElementKey* OldKeyPtr = PreviousNameMap.Find(InKey))
	{
		return OldKeyPtr->Name;
	}
	return NAME_None;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchy::GetPreviousParent(const FTLLRN_RigElementKey& InKey) const
{
	if(const FTLLRN_RigElementKey* OldParentPtr = PreviousParentMap.Find(InKey))
	{
		return *OldParentPtr;
	}
	return FTLLRN_RigElementKey();
}

bool UTLLRN_RigHierarchy::IsParentedTo(FTLLRN_RigBaseElement* InChild, FTLLRN_RigBaseElement* InParent, const TElementDependencyMap& InDependencyMap) const
{
	ElementDependencyVisited.Reset();
	if(!InDependencyMap.IsEmpty())
	{
		ElementDependencyVisited.SetNumZeroed(Elements.Num());
	}
	return IsDependentOn(InChild, InParent, InDependencyMap, true);
}

bool UTLLRN_RigHierarchy::IsDependentOn(FTLLRN_RigBaseElement* InDependent, FTLLRN_RigBaseElement* InDependency, const TElementDependencyMap& InDependencyMap, bool bIsOnActualTopology) const
{
	if((InDependent == nullptr) || (InDependency == nullptr))
	{
		return false;
	}

	if(InDependent == InDependency)
	{
		return true;
	}

	const int32 DependentElementIndex = InDependent->GetIndex();
	const int32 DependencyElementIndex = InDependency->GetIndex();
	const TTuple<int32,int32> CacheKey(DependentElementIndex, DependencyElementIndex);

	if(!ElementDependencyCache.IsValid(GetTopologyVersion()))
	{
		ElementDependencyCache.Set(TMap<TTuple<int32, int32>, bool>(), GetTopologyVersion());
	}

	// we'll only update the caches if we are following edges on the actual topology
	if(const bool* bCachedResult = ElementDependencyCache.Get().Find(CacheKey))
	{
		return *bCachedResult;
	}

	// check if the reverse dependency check has been stored before - if the dependency is dependent
	// then we don't need to recurse any further.
	const TTuple<int32,int32> ReverseCacheKey(DependencyElementIndex, DependentElementIndex);
	if(const bool* bReverseCachedResult = ElementDependencyCache.Get().Find(ReverseCacheKey))
	{
		if(*bReverseCachedResult)
		{
			return false;
		}
	}

	if(!ElementDependencyVisited.IsEmpty())
	{
		// when running this with a provided dependency map
		// we may run into a cycle / infinite recursion. this array
		// keeps track of all elements being visited before.
		if(!ElementDependencyVisited.IsValidIndex(DependentElementIndex))
		{
			return false;
		}
		if (ElementDependencyVisited[DependentElementIndex])
		{
			return false;
		}
		ElementDependencyVisited[DependentElementIndex] = true;
	}

	// collect all possible parents of the dependent
	if(const FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(InDependent))
	{
		if(IsDependentOn(SingleParentElement->ParentElement, InDependency, InDependencyMap, true))
		{
			// we'll only update the caches if we are following edges on the actual topology
			if(bIsOnActualTopology)
			{
				ElementDependencyCache.Get().FindOrAdd(CacheKey, true);
			}
			return true;
		}
	}
	else if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InDependent))
	{
		for(const FTLLRN_RigElementParentConstraint& ParentConstraint : MultiParentElement->ParentConstraints)
		{
			if(IsDependentOn(ParentConstraint.ParentElement, InDependency, InDependencyMap, true))
			{
				// we'll only update the caches if we are following edges on the actual topology
				if(bIsOnActualTopology)
				{
					ElementDependencyCache.Get().FindOrAdd(CacheKey, true);
				}
				return true;
			}
		}
	}

	// check the optional dependency map
	if(const TArray<int32>* DependentIndicesPtr = InDependencyMap.Find(InDependent->GetIndex()))
	{
		const TArray<int32>& DependentIndices = *DependentIndicesPtr;
		for(const int32 DependentIndex : DependentIndices)
		{
			ensure(Elements.IsValidIndex(DependentIndex));
			if(IsDependentOn(Elements[DependentIndex], InDependency, InDependencyMap, false))
			{
				// we'll only update the caches if we are following edges on the actual topology
				if(bIsOnActualTopology)
				{
					ElementDependencyCache.Get().FindOrAdd(CacheKey, true);
				}
				return true;
			}
		}
	}

	// we'll only update the caches if we are following edges on the actual topology
	if(bIsOnActualTopology)
	{
		ElementDependencyCache.Get().FindOrAdd(CacheKey, false);
	}
	return false;
}

int32 UTLLRN_RigHierarchy::GetLocalIndex(const FTLLRN_RigBaseElement* InElement) const
{
	if(InElement == nullptr)
	{
		return INDEX_NONE;
	}
	
	if(const FTLLRN_RigBaseElement* ParentElement = GetFirstParent(InElement))
	{
		TConstArrayView<FTLLRN_RigBaseElement*> Children = GetChildren(ParentElement);
		return Children.Find(const_cast<FTLLRN_RigBaseElement*>(InElement));
	}

	return GetRootElements().Find(const_cast<FTLLRN_RigBaseElement*>(InElement));
}

bool UTLLRN_RigHierarchy::IsTracingChanges() const
{
#if WITH_EDITOR
	return (CVarTLLRN_ControlTLLRN_RigHierarchyTraceAlways->GetInt() != 0) || (TraceFramesLeft > 0);
#else
	return false;
#endif
}

#if WITH_EDITOR

void UTLLRN_RigHierarchy::ResetTransformStack()
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	TransformUndoStack.Reset();
	TransformRedoStack.Reset();
	TransformStackIndex = TransformUndoStack.Num();

	if(IsTracingChanges())
	{
		TracePoses.Reset();
		StorePoseForTrace(TEXT("BeginOfFrame"));
	}
}

void UTLLRN_RigHierarchy::StorePoseForTrace(const FString& InPrefix)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	check(!InPrefix.IsEmpty());
	
	FName InitialKey = *FString::Printf(TEXT("%s_Initial"), *InPrefix);
	FName CurrentKey = *FString::Printf(TEXT("%s_Current"), *InPrefix);
	TracePoses.FindOrAdd(InitialKey) = GetPose(true);
	TracePoses.FindOrAdd(CurrentKey) = GetPose(false);
}

void UTLLRN_RigHierarchy::CheckTraceFormatIfRequired()
{
	if(sTLLRN_RigHierarchyLastTrace != CVarTLLRN_ControlTLLRN_RigHierarchyTracePrecision->GetInt())
	{
		sTLLRN_RigHierarchyLastTrace = CVarTLLRN_ControlTLLRN_RigHierarchyTracePrecision->GetInt();
		const FString Format = FString::Printf(TEXT("%%.%df"), sTLLRN_RigHierarchyLastTrace);
		check(Format.Len() < 16);
		sTLLRN_RigHierarchyTraceFormat[Format.Len()] = '\0';
		FMemory::Memcpy(sTLLRN_RigHierarchyTraceFormat, *Format, Format.Len() * sizeof(TCHAR));
	}
}

template <class CharType>
struct TTLLRN_RigHierarchyJsonPrintPolicy
	: public TPrettyJsonPrintPolicy<CharType>
{
	static inline void WriteDouble(  FArchive* Stream, double Value )
	{
		UTLLRN_RigHierarchy::CheckTraceFormatIfRequired();
		TJsonPrintPolicy<CharType>::WriteString(Stream, FString::Printf(sTLLRN_RigHierarchyTraceFormat, Value));
	}
};

void UTLLRN_RigHierarchy::DumpTransformStackToFile(FString* OutFilePath)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(IsTracingChanges())
	{
		StorePoseForTrace(TEXT("EndOfFrame"));
	}

	FString PathName = GetPathName();
	PathName.Split(TEXT(":"), nullptr, &PathName);
	PathName.ReplaceCharInline('.', '/');

	FString Suffix;
	if(TraceFramesLeft > 0)
	{
		Suffix = FString::Printf(TEXT("_Trace_%03d"), TraceFramesCaptured);
	}

	FString FileName = FString::Printf(TEXT("%sTLLRN_ControlRig/%s%s.json"), *FPaths::ProjectLogDir(), *PathName, *Suffix);
	FString FullFilename = FPlatformFileManager::Get().GetPlatformFile().ConvertToAbsolutePathForExternalAppForWrite(*FileName);

	TSharedPtr<FJsonObject> JsonData = MakeShareable(new FJsonObject);
	JsonData->SetStringField(TEXT("PathName"), GetPathName());

	TSharedRef<FJsonObject> JsonTracedPoses = MakeShareable(new FJsonObject);
	for(const TPair<FName, FTLLRN_RigPose>& Pair : TracePoses)
	{
		TSharedRef<FJsonObject> JsonTracedPose = MakeShareable(new FJsonObject);
		if (FJsonObjectConverter::UStructToJsonObject(FTLLRN_RigPose::StaticStruct(), &Pair.Value, JsonTracedPose, 0, 0))
		{
			JsonTracedPoses->SetObjectField(Pair.Key.ToString(), JsonTracedPose);
		}
	}
	JsonData->SetObjectField(TEXT("TracedPoses"), JsonTracedPoses);

	TArray<TSharedPtr<FJsonValue>> JsonTransformStack;
	for (const FTLLRN_RigTransformStackEntry& TransformStackEntry : TransformUndoStack)
	{
		TSharedRef<FJsonObject> JsonTransformStackEntry = MakeShareable(new FJsonObject);
		if (FJsonObjectConverter::UStructToJsonObject(FTLLRN_RigTransformStackEntry::StaticStruct(), &TransformStackEntry, JsonTransformStackEntry, 0, 0))
		{
			JsonTransformStack.Add(MakeShareable(new FJsonValueObject(JsonTransformStackEntry)));
		}
	}
	JsonData->SetArrayField(TEXT("TransformStack"), JsonTransformStack);

	FString JsonText;
	const TSharedRef< TJsonWriter< TCHAR, TTLLRN_RigHierarchyJsonPrintPolicy<TCHAR> > > JsonWriter = TJsonWriterFactory< TCHAR, TTLLRN_RigHierarchyJsonPrintPolicy<TCHAR> >::Create(&JsonText);
	if (FJsonSerializer::Serialize(JsonData.ToSharedRef(), JsonWriter))
	{
		if ( FFileHelper::SaveStringToFile(JsonText, *FullFilename) )
		{
			UE_LOG(LogTLLRN_ControlRig, Display, TEXT("Saved hierarchy trace to %s"), *FullFilename);

			if(OutFilePath)
			{
				*OutFilePath = FullFilename;
			}
		}
	}

	TraceFramesLeft = FMath::Max(0, TraceFramesLeft - 1);
	TraceFramesCaptured++;
}

void UTLLRN_RigHierarchy::TraceFrames(int32 InNumFramesToTrace)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	
	TraceFramesLeft = InNumFramesToTrace;
	TraceFramesCaptured = 0;
	ResetTransformStack();
}

#endif

bool UTLLRN_RigHierarchy::IsSelected(const FTLLRN_RigBaseElement* InElement) const
{
	if(InElement == nullptr)
	{
		return false;
	}
	if(const UTLLRN_RigHierarchy* HierarchyForSelection = HierarchyForSelectionPtr.Get())
	{
		return HierarchyForSelection->IsSelected(InElement->GetKey());
	}

	const bool bIsSelected = OrderedSelection.Contains(InElement->GetKey());
	ensure(bIsSelected == InElement->IsSelected());
	return bIsSelected;
}


void UTLLRN_RigHierarchy::EnsureCachedChildrenAreCurrent() const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	if(ChildElementCacheTopologyVersion != TopologyVersion)
	{
		const_cast<UTLLRN_RigHierarchy*>(this)->UpdateCachedChildren();
	}
}
	
void UTLLRN_RigHierarchy::UpdateCachedChildren()
{
	FScopeLock Lock(&ElementsLock);
	
	// First we tally up how many children each element has, then we allocate for the total
	// count and do the same loop again.
	TArray<int32> ChildrenCount;
	ChildrenCount.SetNumZeroed(Elements.Num());

	// Bit array that denotes elements that have parents. We'll use this to quickly iterate
	// for the second pass.
	TBitArray<> ElementHasParent(false, Elements.Num());

	for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
	{
		const FTLLRN_RigBaseElement* Element = Elements[ElementIndex];
		
		if(const FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(Element))
		{
			if(const FTLLRN_RigTransformElement* ParentElement = SingleParentElement->ParentElement)
			{
				ChildrenCount[ParentElement->Index]++;
				ElementHasParent[ElementIndex] = true;
			}
		}
		else if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(Element))
		{
			for(const FTLLRN_RigElementParentConstraint& ParentConstraint : MultiParentElement->ParentConstraints)
			{
				if(const FTLLRN_RigTransformElement* ParentElement = ParentConstraint.ParentElement)
				{
					ChildrenCount[ParentElement->Index]++;
					ElementHasParent[ElementIndex] = true;
				}
			}
		}
	}

	// Tally up how many elements have children.
	const int32 NumElementsWithChildren = Algo::CountIf(ChildrenCount, [](int32 InCount) { return InCount > 0; });

	ChildElementOffsetAndCountCache.Reset(NumElementsWithChildren);

	// Tally up how many children there are in total and set the index on each of the elements as we go.
	int32 TotalChildren = 0;
	for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
	{
		if (ChildrenCount[ElementIndex])
		{
			Elements[ElementIndex]->ChildCacheIndex = ChildElementOffsetAndCountCache.Num();
			ChildElementOffsetAndCountCache.Add({TotalChildren, ChildrenCount[ElementIndex]});
			TotalChildren += ChildrenCount[ElementIndex];
		}
		else
		{
			// This element has no children, mark it as having no entry in the child table.
			Elements[ElementIndex]->ChildCacheIndex = INDEX_NONE;
		}
	}

	// Now run through all elements that are known to have parents and start filling up the children array.
	ChildElementCache.Reset();
	ChildElementCache.SetNumZeroed(TotalChildren);

	// Recycle this array to indicate where we are with each set of children, as a local offset into each
	// element's children sub-array.
	ChildrenCount.Reset();
	ChildrenCount.SetNumZeroed(Elements.Num());	

	auto SetChildElement = [this, &ChildrenCount](
		const FTLLRN_RigTransformElement* InParentElement,
		FTLLRN_RigBaseElement* InChildElement)
	{
		const int32 ParentElementIndex = InParentElement->Index;
		const int32 CacheOffset = ChildElementOffsetAndCountCache[InParentElement->ChildCacheIndex].Offset;

		ChildElementCache[CacheOffset + ChildrenCount[ParentElementIndex]] = InChildElement;
		ChildrenCount[ParentElementIndex]++;
	};
	
	for (TConstSetBitIterator<> ElementIt(ElementHasParent); ElementIt; ++ElementIt)
	{
		FTLLRN_RigBaseElement* Element = Elements[ElementIt.GetIndex()];
		
		if(const FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(Element))
		{
			if(const FTLLRN_RigTransformElement* ParentElement = SingleParentElement->ParentElement)
			{
				SetChildElement(ParentElement, Element);
			}
		}
		else if(const FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(Element))
		{
			for(const FTLLRN_RigElementParentConstraint& ParentConstraint : MultiParentElement->ParentConstraints)
			{
				if(const FTLLRN_RigTransformElement* ParentElement = ParentConstraint.ParentElement)
				{
					SetChildElement(ParentElement, Element);
				}
			}
		}
	}

	// Mark the cache up-to-date.
	ChildElementCacheTopologyVersion = TopologyVersion;
}

	
FTLLRN_RigElementKey UTLLRN_RigHierarchy::PreprocessParentElementKeyForSpaceSwitching(const FTLLRN_RigElementKey& InChildKey, const FTLLRN_RigElementKey& InParentKey)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	
	if(InParentKey == GetWorldSpaceReferenceKey())
	{
		return GetOrAddWorldSpaceReference();
	}
	else if(InParentKey == GetDefaultParentKey())
	{
		const FTLLRN_RigElementKey DefaultParent = GetDefaultParent(InChildKey);
		if(DefaultParent == GetWorldSpaceReferenceKey())
		{
			return FTLLRN_RigElementKey();
		}
		else
		{
			return DefaultParent;
		}
	}

	return InParentKey;
}

FTLLRN_RigBaseElement* UTLLRN_RigHierarchy::MakeElement(ETLLRN_RigElementType InElementType, int32 InCount, int32* OutStructureSize)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	check(InCount > 0);
	
	FTLLRN_RigBaseElement* Element = nullptr;
	switch(InElementType)
	{
		case ETLLRN_RigElementType::Bone:
		{
			if(OutStructureSize)
			{
				*OutStructureSize = sizeof(FTLLRN_RigBoneElement);
			}
			Element = NewElement<FTLLRN_RigBoneElement>(InCount);
			break;
		}
		case ETLLRN_RigElementType::Null:
		{
			if(OutStructureSize)
			{
				*OutStructureSize = sizeof(FTLLRN_RigNullElement);
			}
			Element = NewElement<FTLLRN_RigNullElement>(InCount);
			break;
		}
		case ETLLRN_RigElementType::Control:
		{
			if(OutStructureSize)
			{
				*OutStructureSize = sizeof(FTLLRN_RigControlElement);
			}
			Element = NewElement<FTLLRN_RigControlElement>(InCount);
			break;
		}
		case ETLLRN_RigElementType::Curve:
		{
			if(OutStructureSize)
			{
				*OutStructureSize = sizeof(FTLLRN_RigCurveElement);
			}
			Element = NewElement<FTLLRN_RigCurveElement>(InCount);
			break;
		}
		case ETLLRN_RigElementType::Physics:
		{
			if(OutStructureSize)
			{
				*OutStructureSize = sizeof(FTLLRN_RigPhysicsElement);
			}
			Element = NewElement<FTLLRN_RigPhysicsElement>(InCount);
			break;
		}
		case ETLLRN_RigElementType::Reference:
		{
			if(OutStructureSize)
			{
				*OutStructureSize = sizeof(FTLLRN_RigReferenceElement);
			}
			Element = NewElement<FTLLRN_RigReferenceElement>(InCount);
			break;
		}
		case ETLLRN_RigElementType::Connector:
		{
			if(OutStructureSize)
			{
				*OutStructureSize = sizeof(FTLLRN_RigConnectorElement);
			}
			Element = NewElement<FTLLRN_RigConnectorElement>(InCount);
			break;
		}
		case ETLLRN_RigElementType::Socket:
		{
			if(OutStructureSize)
			{
				*OutStructureSize = sizeof(FTLLRN_RigSocketElement);
			}
			Element = NewElement<FTLLRN_RigSocketElement>(InCount);
			break;
		}
		default:
		{
			ensure(false);
		}
	}

	return Element;
}

void UTLLRN_RigHierarchy::DestroyElement(FTLLRN_RigBaseElement*& InElement)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	check(InElement != nullptr);

	if(InElement->OwnedInstances == 0)
	{
		return;
	}

	const int32 Count = InElement->OwnedInstances;
	switch(InElement->GetType())
	{
		case ETLLRN_RigElementType::Bone:
		{
			FTLLRN_RigBoneElement* ExistingElements = Cast<FTLLRN_RigBoneElement>(InElement);
			for(int32 Index=0;Index<Count;Index++)
			{
				TGuardValue<const FTLLRN_RigBaseElement*> DestroyGuard(ElementBeingDestroyed, &ExistingElements[Index]);
				DeallocateElementStorage(&ExistingElements[Index]);
				ExistingElements[Index].~FTLLRN_RigBoneElement(); 
			}
			break;
		}
		case ETLLRN_RigElementType::Null:
		{
			FTLLRN_RigNullElement* ExistingElements = Cast<FTLLRN_RigNullElement>(InElement);
			for(int32 Index=0;Index<Count;Index++)
			{
				TGuardValue<const FTLLRN_RigBaseElement*> DestroyGuard(ElementBeingDestroyed, &ExistingElements[Index]);
				DeallocateElementStorage(&ExistingElements[Index]);
				ExistingElements[Index].~FTLLRN_RigNullElement(); 
			}
			break;
		}
		case ETLLRN_RigElementType::Control:
		{
			FTLLRN_RigControlElement* ExistingElements = Cast<FTLLRN_RigControlElement>(InElement);
			for(int32 Index=0;Index<Count;Index++)
			{
				TGuardValue<const FTLLRN_RigBaseElement*> DestroyGuard(ElementBeingDestroyed, &ExistingElements[Index]);
				DeallocateElementStorage(&ExistingElements[Index]);
				ExistingElements[Index].~FTLLRN_RigControlElement(); 
			}
			break;
		}
		case ETLLRN_RigElementType::Curve:
		{
			FTLLRN_RigCurveElement* ExistingElements = Cast<FTLLRN_RigCurveElement>(InElement);
			for(int32 Index=0;Index<Count;Index++)
			{
				TGuardValue<const FTLLRN_RigBaseElement*> DestroyGuard(ElementBeingDestroyed, &ExistingElements[Index]);
				DeallocateElementStorage(&ExistingElements[Index]);
				ExistingElements[Index].~FTLLRN_RigCurveElement(); 
			}
			break;
		}
		case ETLLRN_RigElementType::Physics:
		{
			FTLLRN_RigPhysicsElement* ExistingElements = Cast<FTLLRN_RigPhysicsElement>(InElement);
			for(int32 Index=0;Index<Count;Index++)
			{
				TGuardValue<const FTLLRN_RigBaseElement*> DestroyGuard(ElementBeingDestroyed, &ExistingElements[Index]);
				DeallocateElementStorage(&ExistingElements[Index]);
				ExistingElements[Index].~FTLLRN_RigPhysicsElement(); 
			}
			break;
		}
		case ETLLRN_RigElementType::Reference:
		{
			FTLLRN_RigReferenceElement* ExistingElements = Cast<FTLLRN_RigReferenceElement>(InElement);
			for(int32 Index=0;Index<Count;Index++)
			{
				TGuardValue<const FTLLRN_RigBaseElement*> DestroyGuard(ElementBeingDestroyed, &ExistingElements[Index]);
				DeallocateElementStorage(&ExistingElements[Index]);
				ExistingElements[Index].~FTLLRN_RigReferenceElement(); 
			}
			break;
		}
		case ETLLRN_RigElementType::Connector:
		{
			FTLLRN_RigConnectorElement* ExistingElements = Cast<FTLLRN_RigConnectorElement>(InElement);
			for(int32 Index=0;Index<Count;Index++)
			{
				TGuardValue<const FTLLRN_RigBaseElement*> DestroyGuard(ElementBeingDestroyed, &ExistingElements[Index]);
				DeallocateElementStorage(&ExistingElements[Index]);
				ExistingElements[Index].~FTLLRN_RigConnectorElement(); 
			}
			break;
		}
		case ETLLRN_RigElementType::Socket:
		{
			FTLLRN_RigSocketElement* ExistingElements = Cast<FTLLRN_RigSocketElement>(InElement);
			for(int32 Index=0;Index<Count;Index++)
			{
				TGuardValue<const FTLLRN_RigBaseElement*> DestroyGuard(ElementBeingDestroyed, &ExistingElements[Index]);
				DeallocateElementStorage(&ExistingElements[Index]);
				ExistingElements[Index].~FTLLRN_RigSocketElement(); 
			}
			break;
		}
		default:
		{
			ensure(false);
			return;
		}
	}

	FMemory::Free(InElement);
	InElement = nullptr;
	
	ElementTransformRanges.Reset();
}

void UTLLRN_RigHierarchy::PropagateDirtyFlags(FTLLRN_RigTransformElement* InTransformElement, bool bInitial, bool bAffectChildren, bool bComputeOpposed, bool bMarkDirty) const
{
	if(!bEnableDirtyPropagation)
	{
		return;
	}
	
	check(InTransformElement);

	const ETLLRN_RigTransformType::Type LocalType = bInitial ? ETLLRN_RigTransformType::InitialLocal : ETLLRN_RigTransformType::CurrentLocal;
	const ETLLRN_RigTransformType::Type GlobalType = bInitial ? ETLLRN_RigTransformType::InitialGlobal : ETLLRN_RigTransformType::CurrentGlobal;

	if(bComputeOpposed)
	{
		for(const FTLLRN_RigTransformElement::FElementToDirty& ElementToDirty : InTransformElement->ElementsToDirty)
		{
			ETLLRN_RigTransformType::Type TypeToCompute = bAffectChildren ? LocalType : GlobalType;
			ETLLRN_RigTransformType::Type TypeToDirty = SwapLocalAndGlobal(TypeToCompute);
			
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(ElementToDirty.Element))
			{
				// animation channels never dirty their local value
				if(ControlElement->IsAnimationChannel())
				{
					if(ETLLRN_RigTransformType::IsLocal(TypeToDirty))
					{
						Swap(TypeToDirty, TypeToCompute);
					}
				}
				
				if(ETLLRN_RigTransformType::IsGlobal(TypeToDirty))
				{
					if(ControlElement->GetOffsetDirtyState().IsDirty(TypeToDirty) &&
						ControlElement->GetDirtyState().IsDirty(TypeToDirty) &&
						ControlElement->GetShapeDirtyState().IsDirty(TypeToDirty))
					{
						continue;
					}
				}
			}
			else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(ElementToDirty.Element))
			{
				if(ETLLRN_RigTransformType::IsGlobal(TypeToDirty))
				{
					if(MultiParentElement->GetDirtyState().IsDirty(TypeToDirty))
					{
						continue;
					}
				}
			}
			else
			{
				if(ElementToDirty.Element->GetDirtyState().IsDirty(TypeToDirty))
				{
					continue;
				}
			}

			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(ElementToDirty.Element))
			{
				GetControlOffsetTransform(ControlElement, LocalType);
			}
			GetTransform(ElementToDirty.Element, TypeToCompute); // make sure the local / global transform is up 2 date

			PropagateDirtyFlags(ElementToDirty.Element, bInitial, bAffectChildren, true, false);
		}
	}

	if(bMarkDirty)
	{
		for(const FTLLRN_RigTransformElement::FElementToDirty& ElementToDirty : InTransformElement->ElementsToDirty)
		{
			ETLLRN_RigTransformType::Type TypeToCompute = bAffectChildren ? LocalType : GlobalType;
			ETLLRN_RigTransformType::Type TypeToDirty = SwapLocalAndGlobal(TypeToCompute);

			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(ElementToDirty.Element))
			{
				// animation channels never dirty their local value
				if(ControlElement->IsAnimationChannel())
				{
					if(ETLLRN_RigTransformType::IsLocal(TypeToDirty))
					{
						Swap(TypeToDirty, TypeToCompute);
					}
				}

				if(ETLLRN_RigTransformType::IsGlobal(TypeToDirty))
				{
					if(ControlElement->GetOffsetDirtyState().IsDirty(TypeToDirty) &&
						ControlElement->GetDirtyState().IsDirty(TypeToDirty) &&
					    ControlElement->GetShapeDirtyState().IsDirty(TypeToDirty))
					{
						continue;
					}
				}
			}
			else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(ElementToDirty.Element))
			{
				if(ETLLRN_RigTransformType::IsGlobal(TypeToDirty))
				{
					if(MultiParentElement->GetDirtyState().IsDirty(TypeToDirty))
					{
						continue;
					}
				}
			}
			else
			{
				if(ElementToDirty.Element->GetDirtyState().IsDirty(TypeToDirty))
				{
					continue;
				}
			}
						
			ElementToDirty.Element->GetDirtyState().MarkDirty(TypeToDirty);
		
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(ElementToDirty.Element))
			{
				ControlElement->GetOffsetDirtyState().MarkDirty(GlobalType);
				ControlElement->GetShapeDirtyState().MarkDirty(GlobalType);
			}

			if(bAffectChildren)
			{
				PropagateDirtyFlags(ElementToDirty.Element, bInitial, bAffectChildren, false, true);
			}
		}
	}
}

void UTLLRN_RigHierarchy::CleanupInvalidCaches()
{
	// create a copy of this hierarchy and pre compute all transforms
	bool bHierarchyWasCreatedDuringCleanup = false;
	if(!IsValid(HierarchyForCacheValidation))
	{
		// Give the validation hierarchy a unique name for debug purposes and to avoid possible memory reuse
		static TAtomic<uint32> HierarchyNameIndex = 0;
		static constexpr TCHAR Format[] = TEXT("CacheValidationHierarchy_%u");
		const FString ValidationHierarchyName = FString::Printf(Format, uint32(++HierarchyNameIndex));
		
		HierarchyForCacheValidation = NewObject<UTLLRN_RigHierarchy>(this, *ValidationHierarchyName, RF_Transient);
		HierarchyForCacheValidation->bEnableCacheValidityCheck = false;
		bHierarchyWasCreatedDuringCleanup = true;
	}
	HierarchyForCacheValidation->CopyHierarchy(this);

	struct Local
	{
		static bool NeedsCheck(const FTLLRN_RigLocalAndGlobalDirtyState& InDirtyState)
		{
			return !InDirtyState.Local.Get() && !InDirtyState.Global.Get();
		}
	};

	// mark all elements' initial as dirty where needed
	for(int32 ElementIndex = 0; ElementIndex < HierarchyForCacheValidation->Elements.Num(); ElementIndex++)
	{
		FTLLRN_RigBaseElement* BaseElement = HierarchyForCacheValidation->Elements[ElementIndex];
		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(BaseElement))
		{
			if(Local::NeedsCheck(ControlElement->GetOffsetDirtyState().Initial))
			{
				ControlElement->GetOffsetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
			}

			if(Local::NeedsCheck(ControlElement->GetDirtyState().Initial))
			{
				ControlElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
			}

			if(Local::NeedsCheck(ControlElement->GetShapeDirtyState().Initial))
			{
				ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
			}
			continue;
		}

		if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(BaseElement))
		{
			if(Local::NeedsCheck(MultiParentElement->GetDirtyState().Initial))
			{
				MultiParentElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialLocal);
			}
			continue;
		}

		if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(BaseElement))
		{
			if(Local::NeedsCheck(TransformElement->GetDirtyState().Initial))
			{
				TransformElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
			}
		}
	}

	// recompute 
	HierarchyForCacheValidation->ComputeAllTransforms();

	// we need to check the elements for integrity (global vs local) to be correct.
	for(int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
	{
		FTLLRN_RigBaseElement* BaseElement = Elements[ElementIndex];

		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(BaseElement))
		{
			FTLLRN_RigControlElement* OtherControlElement = HierarchyForCacheValidation->FindChecked<FTLLRN_RigControlElement>(ControlElement->GetKey());
			
			if(Local::NeedsCheck(ControlElement->GetOffsetDirtyState().Initial))
			{
				const FTransform CachedGlobalTransform = OtherControlElement->GetOffsetTransform().Get(ETLLRN_RigTransformType::InitialGlobal);
				const FTransform ComputedGlobalTransform = HierarchyForCacheValidation->GetControlOffsetTransform(OtherControlElement, ETLLRN_RigTransformType::InitialGlobal);

				if(!FTLLRN_RigComputedTransform::Equals(ComputedGlobalTransform, CachedGlobalTransform, 0.01))
				{
					ControlElement->GetOffsetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
				}
			}

			if(Local::NeedsCheck(ControlElement->GetDirtyState().Initial))
			{
				const FTransform CachedGlobalTransform = ControlElement->GetTransform().Get(ETLLRN_RigTransformType::InitialGlobal);
				const FTransform ComputedGlobalTransform = HierarchyForCacheValidation->GetTransform(OtherControlElement, ETLLRN_RigTransformType::InitialGlobal);
				
				if(!FTLLRN_RigComputedTransform::Equals(ComputedGlobalTransform, CachedGlobalTransform, 0.01))
				{
					ControlElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
				}
			}

			if(Local::NeedsCheck(ControlElement->GetShapeDirtyState().Initial))
			{
				const FTransform CachedGlobalTransform = ControlElement->GetShapeTransform().Get(ETLLRN_RigTransformType::InitialGlobal);
				const FTransform ComputedGlobalTransform = HierarchyForCacheValidation->GetControlShapeTransform(OtherControlElement, ETLLRN_RigTransformType::InitialGlobal);
				
				if(!FTLLRN_RigComputedTransform::Equals(ComputedGlobalTransform, CachedGlobalTransform, 0.01))
				{
					ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
				}
			}
			continue;
		}

		if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(BaseElement))
		{
			FTLLRN_RigMultiParentElement* OtherMultiParentElement = HierarchyForCacheValidation->FindChecked<FTLLRN_RigMultiParentElement>(MultiParentElement->GetKey());

			if(Local::NeedsCheck(MultiParentElement->GetDirtyState().Initial))
			{
				const FTransform CachedGlobalTransform = MultiParentElement->GetTransform().Get(ETLLRN_RigTransformType::InitialGlobal);
				const FTransform ComputedGlobalTransform = HierarchyForCacheValidation->GetTransform(OtherMultiParentElement, ETLLRN_RigTransformType::InitialGlobal);
				
				if(!FTLLRN_RigComputedTransform::Equals(ComputedGlobalTransform, CachedGlobalTransform, 0.01))
				{
					// for nulls we perceive the local transform as less relevant
					MultiParentElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialLocal);
				}
			}
			continue;
		}

		if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(BaseElement))
		{
			FTLLRN_RigTransformElement* OtherTransformElement = HierarchyForCacheValidation->FindChecked<FTLLRN_RigTransformElement>(TransformElement->GetKey());

			if(Local::NeedsCheck(TransformElement->GetDirtyState().Initial))
			{
				const FTransform CachedGlobalTransform = TransformElement->GetTransform().Get(ETLLRN_RigTransformType::InitialGlobal);
				const FTransform ComputedGlobalTransform = HierarchyForCacheValidation->GetTransform(OtherTransformElement, ETLLRN_RigTransformType::InitialGlobal);
				
				if(!FTLLRN_RigComputedTransform::Equals(ComputedGlobalTransform, CachedGlobalTransform, 0.01))
				{
					TransformElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
				}
			}
		}
	}

	ResetPoseToInitial(ETLLRN_RigElementType::All);
	EnsureCacheValidity();

	if(bHierarchyWasCreatedDuringCleanup && HierarchyForCacheValidation)
	{
		HierarchyForCacheValidation->Rename(nullptr, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
		HierarchyForCacheValidation->RemoveFromRoot();
		HierarchyForCacheValidation->MarkAsGarbage();
		HierarchyForCacheValidation = nullptr;
	}
}

void UTLLRN_RigHierarchy::FMetadataStorage::Reset()
{
	for (TTuple<FName, FTLLRN_RigBaseMetadata*>& Item: MetadataMap)
	{
		FTLLRN_RigBaseMetadata::DestroyMetadata(&Item.Value);
	}
	MetadataMap.Reset();
	LastAccessName = NAME_None;
	LastAccessMetadata = nullptr;
}

void UTLLRN_RigHierarchy::FMetadataStorage::Serialize(FArchive& Ar)
{
	static const UEnum* MetadataTypeEnum = StaticEnum<ETLLRN_RigMetadataType>();
	
	if (Ar.IsLoading())
	{
		Reset();
		
		int32 NumEntries = 0;
		Ar << NumEntries;
		MetadataMap.Reserve(NumEntries);
		for (int32 Index = 0; Index < NumEntries; Index++)
		{
			FName ItemName, TypeName;
			Ar << ItemName;
			Ar << TypeName;

			const ETLLRN_RigMetadataType Type = static_cast<ETLLRN_RigMetadataType>(MetadataTypeEnum->GetValueByName(TypeName));
			FTLLRN_RigBaseMetadata* Metadata = FTLLRN_RigBaseMetadata::MakeMetadata(ItemName, Type);
			Metadata->Serialize(Ar);

			MetadataMap.Add(ItemName, Metadata);
		}
	}
	else
	{
		int32 NumEntries = MetadataMap.Num();
		Ar << NumEntries;
		for (const TTuple<FName, FTLLRN_RigBaseMetadata*>& Item: MetadataMap)
		{
			FName ItemName = Item.Key;
			FName TypeName = MetadataTypeEnum->GetNameByValue(static_cast<int64>(Item.Value->GetType()));
			Ar << ItemName;
			Ar << TypeName;
			Item.Value->Serialize(Ar);
		}
	}
}

void UTLLRN_RigHierarchy::AllocateDefaultElementStorage(FTLLRN_RigBaseElement* InElement, bool bUpdateAllElements)
{
	if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(InElement))
	{
		// only for the default allocation we want to catch double allocations.
		// so if we are already linked to the default storage, exit early.
		if(ElementTransforms.Contains(TransformElement->PoseStorage.Initial.Local.StorageIndex, TransformElement->PoseStorage.Initial.Local.Storage))
		{
			check(ElementTransforms.Contains(TransformElement->PoseStorage.Current.Local.StorageIndex, TransformElement->PoseStorage.Current.Local.Storage));
			check(ElementTransforms.Contains(TransformElement->PoseStorage.Initial.Global.StorageIndex, TransformElement->PoseStorage.Initial.Global.Storage));
			check(ElementTransforms.Contains(TransformElement->PoseStorage.Current.Global.StorageIndex, TransformElement->PoseStorage.Current.Global.Storage));
			check(ElementDirtyStates.Contains(TransformElement->PoseDirtyState.Initial.Local.StorageIndex, TransformElement->PoseDirtyState.Initial.Local.Storage));
			check(ElementDirtyStates.Contains(TransformElement->PoseDirtyState.Current.Local.StorageIndex, TransformElement->PoseDirtyState.Current.Local.Storage));
			check(ElementDirtyStates.Contains(TransformElement->PoseDirtyState.Initial.Global.StorageIndex, TransformElement->PoseDirtyState.Initial.Global.Storage));
			check(ElementDirtyStates.Contains(TransformElement->PoseDirtyState.Current.Global.StorageIndex, TransformElement->PoseDirtyState.Current.Global.Storage));

			if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(TransformElement))
			{
				check(ElementTransforms.Contains(ControlElement->OffsetStorage.Initial.Local.StorageIndex, ControlElement->OffsetStorage.Initial.Local.Storage));
				check(ElementTransforms.Contains(ControlElement->OffsetStorage.Current.Local.StorageIndex, ControlElement->OffsetStorage.Current.Local.Storage));
				check(ElementTransforms.Contains(ControlElement->OffsetStorage.Initial.Global.StorageIndex, ControlElement->OffsetStorage.Initial.Global.Storage));
				check(ElementTransforms.Contains(ControlElement->OffsetStorage.Current.Global.StorageIndex, ControlElement->OffsetStorage.Current.Global.Storage));
				check(ElementDirtyStates.Contains(ControlElement->OffsetDirtyState.Initial.Local.StorageIndex, ControlElement->OffsetDirtyState.Initial.Local.Storage));
				check(ElementDirtyStates.Contains(ControlElement->OffsetDirtyState.Current.Local.StorageIndex, ControlElement->OffsetDirtyState.Current.Local.Storage));
				check(ElementDirtyStates.Contains(ControlElement->OffsetDirtyState.Initial.Global.StorageIndex, ControlElement->OffsetDirtyState.Initial.Global.Storage));
				check(ElementDirtyStates.Contains(ControlElement->OffsetDirtyState.Current.Global.StorageIndex, ControlElement->OffsetDirtyState.Current.Global.Storage));
				check(ElementTransforms.Contains(ControlElement->ShapeStorage.Initial.Local.StorageIndex, ControlElement->ShapeStorage.Initial.Local.Storage));
				check(ElementTransforms.Contains(ControlElement->ShapeStorage.Current.Local.StorageIndex, ControlElement->ShapeStorage.Current.Local.Storage));
				check(ElementTransforms.Contains(ControlElement->ShapeStorage.Initial.Global.StorageIndex, ControlElement->ShapeStorage.Initial.Global.Storage));
				check(ElementTransforms.Contains(ControlElement->ShapeStorage.Current.Global.StorageIndex, ControlElement->ShapeStorage.Current.Global.Storage));
				check(ElementDirtyStates.Contains(ControlElement->ShapeDirtyState.Initial.Local.StorageIndex, ControlElement->ShapeDirtyState.Initial.Local.Storage));
				check(ElementDirtyStates.Contains(ControlElement->ShapeDirtyState.Current.Local.StorageIndex, ControlElement->ShapeDirtyState.Current.Local.Storage));
				check(ElementDirtyStates.Contains(ControlElement->ShapeDirtyState.Initial.Global.StorageIndex, ControlElement->ShapeDirtyState.Initial.Global.Storage));
				check(ElementDirtyStates.Contains(ControlElement->ShapeDirtyState.Current.Global.StorageIndex, ControlElement->ShapeDirtyState.Current.Global.Storage));
			}
			return;
		}
		const TArray<int32, TInlineAllocator<4>> TransformIndices = ElementTransforms.Allocate(TransformElement->GetNumTransforms(), FTransform::Identity);
		check(TransformIndices.Num() >= 4);
		const TArray<int32, TInlineAllocator<4>> DirtyStateIndices = ElementDirtyStates.Allocate(TransformElement->GetNumTransforms(), false);
		check(DirtyStateIndices.Num() >= 4);
		TransformElement->PoseStorage.Initial.Local.StorageIndex = TransformIndices[0];
		TransformElement->PoseStorage.Current.Local.StorageIndex = TransformIndices[1];
		TransformElement->PoseStorage.Initial.Global.StorageIndex = TransformIndices[2];
		TransformElement->PoseStorage.Current.Global.StorageIndex = TransformIndices[3];
		TransformElement->PoseDirtyState.Initial.Local.StorageIndex = DirtyStateIndices[0];
		TransformElement->PoseDirtyState.Current.Local.StorageIndex = DirtyStateIndices[1];
		TransformElement->PoseDirtyState.Initial.Global.StorageIndex = DirtyStateIndices[2];
		TransformElement->PoseDirtyState.Current.Global.StorageIndex = DirtyStateIndices[3];

		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(TransformElement))
		{
			check(TransformIndices.Num() >= 12);
			check(DirtyStateIndices.Num() >= 12);
			ControlElement->OffsetStorage.Initial.Local.StorageIndex = TransformIndices[4];
			ControlElement->OffsetStorage.Current.Local.StorageIndex = TransformIndices[5];
			ControlElement->OffsetStorage.Initial.Global.StorageIndex = TransformIndices[6];
			ControlElement->OffsetStorage.Current.Global.StorageIndex = TransformIndices[7];
			ControlElement->OffsetDirtyState.Initial.Local.StorageIndex = DirtyStateIndices[4];
			ControlElement->OffsetDirtyState.Current.Local.StorageIndex = DirtyStateIndices[5];
			ControlElement->OffsetDirtyState.Initial.Global.StorageIndex = DirtyStateIndices[6];
			ControlElement->OffsetDirtyState.Current.Global.StorageIndex = DirtyStateIndices[7];
			ControlElement->ShapeStorage.Initial.Local.StorageIndex = TransformIndices[8];
			ControlElement->ShapeStorage.Current.Local.StorageIndex = TransformIndices[9];
			ControlElement->ShapeStorage.Initial.Global.StorageIndex = TransformIndices[10];
			ControlElement->ShapeStorage.Current.Global.StorageIndex = TransformIndices[11];
			ControlElement->ShapeDirtyState.Initial.Local.StorageIndex = DirtyStateIndices[8];
			ControlElement->ShapeDirtyState.Current.Local.StorageIndex = DirtyStateIndices[9];
			ControlElement->ShapeDirtyState.Initial.Global.StorageIndex = DirtyStateIndices[10];
			ControlElement->ShapeDirtyState.Current.Global.StorageIndex = DirtyStateIndices[11];
		}
	}
	else if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(InElement))
	{
		if(ElementCurves.Contains(CurveElement->StorageIndex, CurveElement->Storage))
		{
			return;
		}
		
		const TArray<int32, TInlineAllocator<4>> CurveIndices = ElementCurves.Allocate(CurveElement->GetNumCurves(), 0.f);
		check(CurveIndices.Num() >= 1);
		CurveElement->StorageIndex = CurveIndices[0];
	}

	if(bUpdateAllElements)
	{
		UpdateElementStorage();
	}
	else
	{
		InElement->LinkStorage(ElementTransforms.Storage, ElementDirtyStates.Storage, ElementCurves.Storage);
	}
	ElementTransformRanges.Reset();
}

void UTLLRN_RigHierarchy::DeallocateElementStorage(FTLLRN_RigBaseElement* InElement)
{
	InElement->UnlinkStorage(ElementTransforms, ElementDirtyStates, ElementCurves);
}

void UTLLRN_RigHierarchy::UpdateElementStorage()
{
	for(FTLLRN_RigBaseElement* Element : Elements)
	{
		Element->LinkStorage(ElementTransforms.Storage, ElementDirtyStates.Storage, ElementCurves.Storage);
	}
	ElementTransformRanges.Reset();
}

bool UTLLRN_RigHierarchy::SortElementStorage()
{
	ElementTransformRanges.Reset();

	if(Elements.IsEmpty())
	{
		return false;
	}
	
	TMap<int32, FTLLRN_RigComputedTransform*> TransformIndexToComputedTransform;
	TMap<int32, FTLLRN_RigTransformDirtyState*> TransformIndexToDirtyState;

	struct FTypeInfo
	{
		FTypeInfo()
		: Index(INDEX_NONE)
		, Element(ETLLRN_RigElementType::Bone)
		, Transform(ETLLRN_RigTransformType::InitialLocal)
		, Storage(ETLLRN_RigTransformStorageType::Pose)
		{
		}

		FTypeInfo(int32 InIndex, ETLLRN_RigElementType InElement, ETLLRN_RigTransformType::Type InTransform, ETLLRN_RigTransformStorageType::Type InStorage)
		: Index(InIndex)
		, Element(InElement)
		, Transform(InTransform)
		, Storage(InStorage)
		{
		}

		uint32 GetSortKey(uint32 InNumElements) const
		{
			const uint32 StorageMultiplier = InNumElements; 
			const uint32 ElementMultiplier = StorageMultiplier * (uint32)ETLLRN_RigTransformStorageType::NumStorageTypes;
			const uint32 TransformMultiplier = ElementMultiplier * uint32(TLLRN_RigElementTypeToFlatIndex(ETLLRN_RigElementType::Last) + 1);
			// we sort elements by laying them out first by transform type, then element type, then storage type
			// and finally in the order of their original index in the hierarchy
			return Index +
				int32(Transform) * TransformMultiplier +
				int32(Element) * ElementMultiplier +
				int32(Storage) * StorageMultiplier;
		}

		int32 Index;
		ETLLRN_RigElementType Element;
		ETLLRN_RigTransformType::Type Transform;
		ETLLRN_RigTransformStorageType::Type Storage;
	};

	struct FTransformInfo
	{
		FTransformInfo()
		: ComputedTransform(nullptr)
		, DirtyState(nullptr)
		{
		}

		FTransformInfo(const FTypeInfo& InTypeInfo, FTLLRN_RigComputedTransform* InComputedTransform, FTLLRN_RigTransformDirtyState* InDirtyState)
		: TypeInfo(InTypeInfo)
		, ComputedTransform(InComputedTransform)
		, DirtyState(InDirtyState)
		{
		}
		
		FTypeInfo TypeInfo;
		FTLLRN_RigComputedTransform* ComputedTransform;
		FTLLRN_RigTransformDirtyState* DirtyState;
	};

	TArray<FTransformInfo> TransformInfos;
	TransformInfos.Reserve(Elements.Num() * 4);

	ForEachTransformElementStorage([&TransformInfos](
		FTLLRN_RigTransformElement* InTransformElement,
		ETLLRN_RigTransformType::Type InTransformType,
		ETLLRN_RigTransformStorageType::Type InStorageType,
		FTLLRN_RigComputedTransform* InComputedTransform,
		FTLLRN_RigTransformDirtyState* InDirtyState)
	{
		TransformInfos.Emplace(
			FTypeInfo(InTransformElement->GetIndex(), InTransformElement->GetType(), InTransformType, InStorageType),
			InComputedTransform,
			InDirtyState);
	});

	uint32 NumElements = (uint32)Elements.Num();
	Algo::SortBy(TransformInfos, [NumElements](const FTransformInfo& InInfo) -> uint32
	{
		return InInfo.TypeInfo.GetSortKey(NumElements);
	});

	bool bHasMatchingRanges = true;
	TMap<ETLLRN_RigTransformType::Type, TTuple<int32, int32>> NewRanges;
	
	// check if the sorting is at all required
	bool bRequiresSort = false;
	for(int32 Index = 0; Index < TransformInfos.Num(); Index++)
	{
		const int32 CurrentTransformIndex = TransformInfos[Index].ComputedTransform->StorageIndex;
		if(CurrentTransformIndex != INDEX_NONE && CurrentTransformIndex != Index)
		{
			bRequiresSort = true;
		}

		const int32 CurrentDirtyStateIndex = TransformInfos[Index].DirtyState->StorageIndex;
		if(CurrentDirtyStateIndex != INDEX_NONE && CurrentDirtyStateIndex != Index)
		{
			bRequiresSort = true;
		}

		if(CurrentTransformIndex == INDEX_NONE && CurrentDirtyStateIndex == INDEX_NONE)
		{
			// skip over entries with external storage
			continue;
		}

		if(CurrentTransformIndex != CurrentDirtyStateIndex)
		{
			bHasMatchingRanges = false;
		}
		if(bHasMatchingRanges)
		{
			TTuple<int32,int32>& Range = NewRanges.FindOrAdd(TransformInfos[Index].TypeInfo.Transform, {INT32_MAX, INDEX_NONE});
			Range.Get<0>() = FMath::Min(CurrentTransformIndex, Range.Get<0>());
			Range.Get<1>() = FMath::Max(CurrentTransformIndex, Range.Get<1>());
		}
	}

	if(!bRequiresSort)
	{
		if(bHasMatchingRanges)
		{
			ElementTransformRanges = NewRanges;
		}
		return false;
	}

	FRigReusableElementStorage<FTransform> NewElementTransforms;
	FRigReusableElementStorage<bool> NewElementDirtyStates;
	NewElementDirtyStates.Storage.Reserve(ElementDirtyStates.Storage.Num() - ElementDirtyStates.FreeList.Num());
	NewElementDirtyStates.Storage.Reserve(ElementDirtyStates.Storage.Num() - ElementDirtyStates.FreeList.Num());

	bHasMatchingRanges = true;
	NewRanges.Reset();
	for(int32 Index = 0; Index < TransformInfos.Num(); Index++)
	{
		const int32 CurrentTransformIndex = TransformInfos[Index].ComputedTransform->StorageIndex;
		const int32 CurrentDirtyStateIndex = TransformInfos[Index].DirtyState->StorageIndex;

		int32 NewTransformIndex = INDEX_NONE;
		int32 NewDirtyStateIndex = INDEX_NONE;

		if(CurrentTransformIndex != INDEX_NONE)
		{
			NewTransformIndex = NewElementTransforms.Storage.Add(ElementTransforms[CurrentTransformIndex]);
			TransformInfos[Index].ComputedTransform->StorageIndex = NewTransformIndex;
		}

		if(CurrentDirtyStateIndex != INDEX_NONE)
		{
			NewDirtyStateIndex = NewElementDirtyStates.Storage.Add(ElementDirtyStates[CurrentDirtyStateIndex]);
			TransformInfos[Index].DirtyState->StorageIndex = NewDirtyStateIndex;
		}

		if(CurrentTransformIndex == INDEX_NONE && CurrentDirtyStateIndex)
		{
			// ignore entries with external storage
			continue;
		}

		if(NewTransformIndex == NewDirtyStateIndex)
		{
			TTuple<int32,int32>& Range = NewRanges.FindOrAdd(TransformInfos[Index].TypeInfo.Transform, {INT32_MAX, INDEX_NONE});
			Range.Get<0>() = FMath::Min(NewTransformIndex, Range.Get<0>());
			Range.Get<1>() = FMath::Max(NewTransformIndex, Range.Get<1>());
		}
		else
		{
			bHasMatchingRanges = false;
		}
	}

	ElementTransforms = NewElementTransforms;
	ElementDirtyStates = NewElementDirtyStates;
	
	UpdateElementStorage();

	if(bHasMatchingRanges)
	{
		ElementTransformRanges = NewRanges;
	}
	else
	{
		ElementTransformRanges.Reset();
	}

	return true;
}

bool UTLLRN_RigHierarchy::ShrinkElementStorage()
{
	const TMap<int32, int32> TransformLookup = ElementTransforms.Shrink();
	const TMap<int32, int32> DirtyStateLookup = ElementDirtyStates.Shrink();
	const TMap<int32, int32> CurveLookup = ElementCurves.Shrink();

	if(!TransformLookup.IsEmpty() || !DirtyStateLookup.IsEmpty())
	{
		ForEachTransformElementStorage([&TransformLookup, &DirtyStateLookup](
			FTLLRN_RigTransformElement* InTransformElement,
			ETLLRN_RigTransformType::Type InTransformType,
			ETLLRN_RigTransformStorageType::Type InStorageType,
			FTLLRN_RigComputedTransform* InComputedTransform,
			FTLLRN_RigTransformDirtyState* InDirtyState)
		{
			if(const int32* NewTransformIndex = TransformLookup.Find(InComputedTransform->StorageIndex))
			{
				InComputedTransform->StorageIndex = *NewTransformIndex;
			}
			if(const int32* NewDirtyStateIndex = DirtyStateLookup.Find(InDirtyState->StorageIndex))
			{
				InDirtyState->StorageIndex = *NewDirtyStateIndex;
			}
		});
	}

	if(!CurveLookup.IsEmpty())
	{
		static const int32 CurveElementIndex = TLLRN_RigElementTypeToFlatIndex(ETLLRN_RigElementType::Curve); 
		for(FTLLRN_RigBaseElement* Element : ElementsPerType[CurveElementIndex])
		{
			FTLLRN_RigCurveElement* CurveElement = CastChecked<FTLLRN_RigCurveElement>(Element);
			if(const int32* NewIndex = CurveLookup.Find(CurveElement->StorageIndex))
			{
				CurveElement->StorageIndex = *NewIndex;
			}
		}
	}

	if(!TransformLookup.IsEmpty() || !DirtyStateLookup.IsEmpty() || !CurveLookup.IsEmpty())
	{
		UpdateElementStorage();
		(void)SortElementStorage();
		return true;
	}
	
	return false;
}

void UTLLRN_RigHierarchy::ForEachTransformElementStorage(
	TFunction<void(FTLLRN_RigTransformElement*, ETLLRN_RigTransformType::Type, ETLLRN_RigTransformStorageType::Type, FTLLRN_RigComputedTransform*, FTLLRN_RigTransformDirtyState*)>
	InCallback)
{
	check(InCallback);
	
	for(FTLLRN_RigBaseElement* Element : Elements)
	{
		if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Element))
		{
			InCallback(TransformElement, ETLLRN_RigTransformType::InitialLocal, ETLLRN_RigTransformStorageType::Pose, &TransformElement->PoseStorage.Initial.Local, &TransformElement->PoseDirtyState.Initial.Local);
			InCallback(TransformElement, ETLLRN_RigTransformType::InitialGlobal, ETLLRN_RigTransformStorageType::Pose, &TransformElement->PoseStorage.Initial.Global, &TransformElement->PoseDirtyState.Initial.Global);
			InCallback(TransformElement, ETLLRN_RigTransformType::CurrentLocal, ETLLRN_RigTransformStorageType::Pose, &TransformElement->PoseStorage.Current.Local, &TransformElement->PoseDirtyState.Current.Local);
			InCallback(TransformElement, ETLLRN_RigTransformType::CurrentGlobal, ETLLRN_RigTransformStorageType::Pose, &TransformElement->PoseStorage.Current.Global, &TransformElement->PoseDirtyState.Current.Global);

			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
			{
				InCallback(ControlElement, ETLLRN_RigTransformType::InitialLocal, ETLLRN_RigTransformStorageType::Offset, &ControlElement->OffsetStorage.Initial.Local, &ControlElement->OffsetDirtyState.Initial.Local);
				InCallback(ControlElement, ETLLRN_RigTransformType::InitialGlobal, ETLLRN_RigTransformStorageType::Offset, &ControlElement->OffsetStorage.Initial.Global, &ControlElement->OffsetDirtyState.Initial.Global);
				InCallback(ControlElement, ETLLRN_RigTransformType::CurrentLocal, ETLLRN_RigTransformStorageType::Offset, &ControlElement->OffsetStorage.Current.Local, &ControlElement->OffsetDirtyState.Current.Local);
				InCallback(ControlElement, ETLLRN_RigTransformType::CurrentGlobal, ETLLRN_RigTransformStorageType::Offset, &ControlElement->OffsetStorage.Current.Global, &ControlElement->OffsetDirtyState.Current.Global);
				InCallback(ControlElement, ETLLRN_RigTransformType::InitialLocal, ETLLRN_RigTransformStorageType::Shape, &ControlElement->ShapeStorage.Initial.Local, &ControlElement->ShapeDirtyState.Initial.Local);
				InCallback(ControlElement, ETLLRN_RigTransformType::InitialGlobal, ETLLRN_RigTransformStorageType::Shape, &ControlElement->ShapeStorage.Initial.Global, &ControlElement->ShapeDirtyState.Initial.Global);
				InCallback(ControlElement, ETLLRN_RigTransformType::CurrentLocal, ETLLRN_RigTransformStorageType::Shape, &ControlElement->ShapeStorage.Current.Local, &ControlElement->ShapeDirtyState.Current.Local);
				InCallback(ControlElement, ETLLRN_RigTransformType::CurrentGlobal, ETLLRN_RigTransformStorageType::Shape, &ControlElement->ShapeStorage.Current.Global, &ControlElement->ShapeDirtyState.Current.Global);
			}
		}
	}
}

TTuple<FTLLRN_RigComputedTransform*, FTLLRN_RigTransformDirtyState*> UTLLRN_RigHierarchy::GetElementTransformStorage(const FTLLRN_RigElementKeyAndIndex& InKey,
	ETLLRN_RigTransformType::Type InTransformType, ETLLRN_RigTransformStorageType::Type InStorageType)
{
	FTLLRN_RigTransformElement* TransformElement = Get<FTLLRN_RigTransformElement>(InKey);
	if(TransformElement && TransformElement->GetKey() == InKey.Key)
	{
		FTLLRN_RigCurrentAndInitialTransform& Transform = TransformElement->PoseStorage;
		FTLLRN_RigCurrentAndInitialDirtyState& DirtyState = TransformElement->PoseDirtyState;
		if(InStorageType == ETLLRN_RigTransformStorageType::Offset || InStorageType == ETLLRN_RigTransformStorageType::Shape)
		{
			FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(TransformElement);
			if(ControlElement == nullptr)
			{
				return {nullptr, nullptr};
			}
			if(InStorageType == ETLLRN_RigTransformStorageType::Offset)
			{
				Transform = ControlElement->OffsetStorage;
				DirtyState = ControlElement->OffsetDirtyState;
			}
			else
			{
				Transform = ControlElement->ShapeStorage;
				DirtyState = ControlElement->ShapeDirtyState;
			}
		}
		
		switch(InTransformType)
		{
			case ETLLRN_RigTransformType::InitialLocal:
			{
				return {&Transform.Initial.Local, &DirtyState.Initial.Local};
			}
			case ETLLRN_RigTransformType::CurrentLocal:
			{
				return {&Transform.Current.Local, &DirtyState.Current.Local};
			}
			case ETLLRN_RigTransformType::InitialGlobal:
			{
				return {&Transform.Initial.Global, &DirtyState.Initial.Global};
			}
			case ETLLRN_RigTransformType::CurrentGlobal:
			{
				return {&Transform.Current.Global, &DirtyState.Current.Global};
			}
			default:
			{
				break;
			}
		}
	}
	return {nullptr, nullptr};
}

TOptional<TTuple<int32, int32>> UTLLRN_RigHierarchy::GetElementStorageRange(ETLLRN_RigTransformType::Type InTransformType) const
{
	if(const TTuple<int32,int32>* Range = ElementTransformRanges.Find(InTransformType))
	{
		return *Range;
	}
	return TOptional<TTuple<int32, int32>>();
}

void UTLLRN_RigHierarchy::PropagateMetadata(const FTLLRN_RigElementKey& InKey, const FName& InName, bool bNotify)
{
	if(const FTLLRN_RigBaseElement* Element = Find(InKey))
	{
		PropagateMetadata(Element, InName, bNotify);
	}
}

void UTLLRN_RigHierarchy::PropagateMetadata(const FTLLRN_RigBaseElement* InElement, const FName& InName, bool bNotify)
{
#if WITH_EDITOR
	check(InElement);

	if (!ElementMetadata.IsValidIndex(InElement->MetadataStorageIndex))
	{
		return;
	}

	FMetadataStorage& Storage = ElementMetadata[InElement->MetadataStorageIndex];
	FTLLRN_RigBaseMetadata** MetadataPtrPtr = Storage.MetadataMap.Find(InName);
	if (!MetadataPtrPtr)
	{
		return;
	}

	FTLLRN_RigBaseMetadata* MetadataPtr = *MetadataPtrPtr;
	if(MetadataPtr == nullptr)
	{
		return;
	}

	ForEachListeningHierarchy([InElement, MetadataPtr, bNotify](const FTLLRN_RigHierarchyListener& Listener)
	{
		if(UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
		{
			if(FTLLRN_RigBaseElement* Element = ListeningHierarchy->Find(InElement->GetKey()))
			{
				if (FTLLRN_RigBaseMetadata* Metadata = ListeningHierarchy->GetMetadataForElement(Element, MetadataPtr->GetName(), MetadataPtr->GetType(), bNotify))
				{
					Metadata->SetValueData(MetadataPtr->GetValueData(), MetadataPtr->GetValueSize());
					ListeningHierarchy->PropagateMetadata(Element, Metadata->GetName(), bNotify);
				}
			}
		}
	});
#endif
}

void UTLLRN_RigHierarchy::OnMetadataChanged(const FTLLRN_RigElementKey& InKey, const FName& InName)
{
	MetadataVersion++;

	if (!bSuspendMetadataNotifications)
	{
		if(MetadataChangedDelegate.IsBound())
		{
			MetadataChangedDelegate.Broadcast(InKey, InName);
		}
	}
}

void UTLLRN_RigHierarchy::OnMetadataTagChanged(const FTLLRN_RigElementKey& InKey, const FName& InTag, bool bAdded)
{
	MetadataTagVersion++;

	if (!bSuspendMetadataNotifications)
	{
		if(MetadataTagChangedDelegate.IsBound())
		{
			MetadataTagChangedDelegate.Broadcast(InKey, InTag, bAdded);
		}
	}
}

FTLLRN_RigBaseMetadata* UTLLRN_RigHierarchy::GetMetadataForElement(FTLLRN_RigBaseElement* InElement, const FName& InName, ETLLRN_RigMetadataType InType, bool bInNotify)
{
	if (!ElementMetadata.IsValidIndex(InElement->MetadataStorageIndex))
	{
		InElement->MetadataStorageIndex = ElementMetadata.Allocate(1, FMetadataStorage())[0];
	}
	
	FMetadataStorage& Storage = ElementMetadata[InElement->MetadataStorageIndex];

	// If repeatedly accessing the same element, store it here for faster access to avoid map lookups.
	if (Storage.LastAccessMetadata && Storage.LastAccessName == InName && Storage.LastAccessMetadata->GetType() == InType)
	{
		return Storage.LastAccessMetadata;
	}

	FTLLRN_RigBaseMetadata* Metadata;
	if (FTLLRN_RigBaseMetadata** MetadataPtrPtr = Storage.MetadataMap.Find(InName))
	{
		if ((*MetadataPtrPtr)->GetType() == InType)
		{
			Metadata = *MetadataPtrPtr;
		}
		else
		{
			// The type changed, replace the existing metadata with a new one of the correct type.
			FTLLRN_RigBaseMetadata::DestroyMetadata(MetadataPtrPtr);
			Metadata = *MetadataPtrPtr = FTLLRN_RigBaseMetadata::MakeMetadata(InName, InType);
			
			if (bInNotify)
			{
				OnMetadataChanged(InElement->Key, InName);
			}
		}
	}
	else
	{
		// No metadata with that name existed on the element, create one from scratch.
		Metadata = FTLLRN_RigBaseMetadata::MakeMetadata(InName, InType);
		Storage.MetadataMap.Add(InName, Metadata);

		if (bInNotify)
		{
			OnMetadataChanged(InElement->Key, InName);
		}
	}
	
	Storage.LastAccessName = InName;
	Storage.LastAccessMetadata = Metadata;
	return Metadata;
}


FTLLRN_RigBaseMetadata* UTLLRN_RigHierarchy::FindMetadataForElement(const FTLLRN_RigBaseElement* InElement, const FName& InName, ETLLRN_RigMetadataType InType)
{
	if (!ElementMetadata.IsValidIndex(InElement->MetadataStorageIndex))
	{
		return nullptr;
	}

	FMetadataStorage& Storage = ElementMetadata[InElement->MetadataStorageIndex];
	if (InName == Storage.LastAccessName && (InType == ETLLRN_RigMetadataType::Invalid || (Storage.LastAccessMetadata && Storage.LastAccessMetadata->GetType() == InType)))
	{
		return Storage.LastAccessMetadata;
	}
	
	FTLLRN_RigBaseMetadata** MetadataPtrPtr = Storage.MetadataMap.Find(InName);
	if (!MetadataPtrPtr)
	{
		Storage.LastAccessName = NAME_None;
		Storage.LastAccessMetadata = nullptr;
		return nullptr;
	}

	if (InType != ETLLRN_RigMetadataType::Invalid && (*MetadataPtrPtr)->GetType() != InType)
	{
		Storage.LastAccessName = NAME_None;
		Storage.LastAccessMetadata = nullptr;
		return nullptr;
	}

	Storage.LastAccessName = InName;
	Storage.LastAccessMetadata = *MetadataPtrPtr;

	return *MetadataPtrPtr;
}

const FTLLRN_RigBaseMetadata* UTLLRN_RigHierarchy::FindMetadataForElement(const FTLLRN_RigBaseElement* InElement, const FName& InName, ETLLRN_RigMetadataType InType) const
{
	return const_cast<UTLLRN_RigHierarchy*>(this)->FindMetadataForElement(InElement, InName, InType);
}


bool UTLLRN_RigHierarchy::RemoveMetadataForElement(FTLLRN_RigBaseElement* InElement, const FName& InName)
{
	if (!ElementMetadata.IsValidIndex(InElement->MetadataStorageIndex))
	{
		return false;
	}

	FMetadataStorage& Storage = ElementMetadata[InElement->MetadataStorageIndex];
	FTLLRN_RigBaseMetadata** MetadataPtrPtr = Storage.MetadataMap.Find(InName);
	if (!MetadataPtrPtr)
	{
		return false;
	}
	
	FTLLRN_RigBaseMetadata::DestroyMetadata(MetadataPtrPtr);
	Storage.MetadataMap.Remove(InName);

	// If the storage is now empty, remove the element's storage, so we're not lugging it around
	// unnecessarily. Add the storage slot to the freelist so that the next element to add a new
	// metadata storage can just recycle that.
	if (Storage.MetadataMap.IsEmpty())
	{
		ElementMetadata.Deallocate(InElement->MetadataStorageIndex, nullptr);
		InElement->MetadataStorageIndex = INDEX_NONE;
	}
	else if (Storage.LastAccessName == InName)
	{
		Storage.LastAccessMetadata = nullptr;
	}

	if(ElementBeingDestroyed != InElement)
	{
		OnMetadataChanged(InElement->Key, InName);
	}
	return true;
}


bool UTLLRN_RigHierarchy::RemoveAllMetadataForElement(FTLLRN_RigBaseElement* InElement)
{
	if (!ElementMetadata.IsValidIndex(InElement->MetadataStorageIndex))
	{
		return false;
	}

	
	FMetadataStorage& Storage = ElementMetadata[InElement->MetadataStorageIndex];
	TArray<FName> Names;
	Storage.MetadataMap.GetKeys(Names);
	
	// Clear the storage for the next user.
	Storage.Reset();
	
	ElementMetadata.Deallocate(InElement->MetadataStorageIndex, nullptr);
	InElement->MetadataStorageIndex = INDEX_NONE;

	if(ElementBeingDestroyed != InElement)
	{
		for (FName Name: Names)
		{
			OnMetadataChanged(InElement->Key, Name);
		}
	}
	
	return true; 
}


void UTLLRN_RigHierarchy::CopyAllMetadataFromElement(FTLLRN_RigBaseElement* InTargetElement, const FTLLRN_RigBaseElement* InSourceElement)
{
	if (!ensure(InSourceElement->Owner))
	{
		return;
	}

	if (!InSourceElement->Owner->ElementMetadata.IsValidIndex(InSourceElement->MetadataStorageIndex))
	{
		return;
	}
	
	const FMetadataStorage& SourceStorage = InSourceElement->Owner->ElementMetadata[InSourceElement->MetadataStorageIndex];
	
	for (const TTuple<FName, FTLLRN_RigBaseMetadata*>& SourceItem: SourceStorage.MetadataMap)
	{
		const FTLLRN_RigBaseMetadata* SourceMetadata = SourceItem.Value; 	

		constexpr bool bNotify = false;
		FTLLRN_RigBaseMetadata* TargetMetadata = GetMetadataForElement(InTargetElement, SourceItem.Key, SourceMetadata->GetType(), bNotify);
		TargetMetadata->SetValueData(SourceMetadata->GetValueData(), SourceMetadata->GetValueSize());
	}
}


void UTLLRN_RigHierarchy::EnsureCacheValidityImpl()
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	if(!bEnableCacheValidityCheck)
	{
		return;
	}
	TGuardValue<bool> Guard(bEnableCacheValidityCheck, false);

	static const TArray<FString>& TransformTypeStrings = GetTransformTypeStrings();

	// make sure that elements which are marked as dirty don't have fully cached children
	ForEach<FTLLRN_RigTransformElement>([](FTLLRN_RigTransformElement* TransformElement)
    {
		for(int32 TransformTypeIndex = 0; TransformTypeIndex < (int32) ETLLRN_RigTransformType::NumTransformTypes; TransformTypeIndex++)
		{
			const ETLLRN_RigTransformType::Type GlobalType = (ETLLRN_RigTransformType::Type)TransformTypeIndex;
			const ETLLRN_RigTransformType::Type LocalType = ETLLRN_RigTransformType::SwapLocalAndGlobal(GlobalType);
			const FString& TransformTypeString = TransformTypeStrings[TransformTypeIndex];

			if(ETLLRN_RigTransformType::IsLocal(GlobalType))
			{
				continue;
			}

			if(!TransformElement->GetDirtyState().IsDirty(GlobalType))
			{
				continue;
			}

			for(const FTLLRN_RigTransformElement::FElementToDirty& ElementToDirty : TransformElement->ElementsToDirty)
			{
				if(FTLLRN_RigMultiParentElement* MultiParentElementToDirty = Cast<FTLLRN_RigMultiParentElement>(ElementToDirty.Element))
				{
                    if(FTLLRN_RigControlElement* ControlElementToDirty = Cast<FTLLRN_RigControlElement>(ElementToDirty.Element))
                    {
                        if(ControlElementToDirty->GetOffsetDirtyState().IsDirty(GlobalType))
                        {
                            checkf(ControlElementToDirty->GetDirtyState().IsDirty(GlobalType) ||
                                    ControlElementToDirty->GetDirtyState().IsDirty(LocalType),
                                    TEXT("Control '%s' %s Offset Cache is dirty, but the Pose is not."),
									*ControlElementToDirty->GetKey().ToString(),
									*TransformTypeString);
						}

                        if(ControlElementToDirty->GetDirtyState().IsDirty(GlobalType))
                        {
                            checkf(ControlElementToDirty->GetShapeDirtyState().IsDirty(GlobalType) ||
                                    ControlElementToDirty->GetShapeDirtyState().IsDirty(LocalType),
                                    TEXT("Control '%s' %s Pose Cache is dirty, but the Shape is not."),
									*ControlElementToDirty->GetKey().ToString(),
									*TransformTypeString);
						}
                    }
                    else
                    {
                        checkf(MultiParentElementToDirty->GetDirtyState().IsDirty(GlobalType) ||
                                MultiParentElementToDirty->GetDirtyState().IsDirty(LocalType),
                                TEXT("MultiParent '%s' %s Parent Cache is dirty, but the Pose is not."),
								*MultiParentElementToDirty->GetKey().ToString(),
								*TransformTypeString);
                    }
				}
				else
				{
					checkf(ElementToDirty.Element->GetDirtyState().IsDirty(GlobalType) ||
						ElementToDirty.Element->GetDirtyState().IsDirty(LocalType),
						TEXT("SingleParent '%s' %s Pose is not dirty in Local or Global"),
						*ElementToDirty.Element->GetKey().ToString(),
						*TransformTypeString);
				}
			}
		}
		
        return true;
    });

	// store our own pose in a transient hierarchy used for cache validation
	if(HierarchyForCacheValidation == nullptr)
	{
		HierarchyForCacheValidation = NewObject<UTLLRN_RigHierarchy>(this, NAME_None, RF_Transient);
		HierarchyForCacheValidation->bEnableCacheValidityCheck = false;
	}
	if(HierarchyForCacheValidation->GetTopologyVersion() != GetTopologyVersion())
	{
		HierarchyForCacheValidation->CopyHierarchy(this);
	}
	HierarchyForCacheValidation->CopyPose(this, true, true, true);

	// traverse the copied hierarchy and compare cached vs computed values
	UTLLRN_RigHierarchy* HierarchyForLambda = HierarchyForCacheValidation;
	HierarchyForLambda->Traverse([HierarchyForLambda](FTLLRN_RigBaseElement* Element, bool& bContinue)
	{
		bContinue = true;

		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
		{
			for(int32 TransformTypeIndex = 0; TransformTypeIndex < (int32) ETLLRN_RigTransformType::NumTransformTypes; TransformTypeIndex++)
			{
				const ETLLRN_RigTransformType::Type TransformType = (ETLLRN_RigTransformType::Type)TransformTypeIndex;
				const ETLLRN_RigTransformType::Type OpposedType = ETLLRN_RigTransformType::SwapLocalAndGlobal(TransformType);
				const FString& TransformTypeString = TransformTypeStrings[TransformTypeIndex];

				if(!ControlElement->GetOffsetDirtyState().IsDirty(TransformType) && !ControlElement->GetOffsetDirtyState().IsDirty(OpposedType))
				{
					const FTransform CachedTransform = HierarchyForLambda->GetControlOffsetTransform(ControlElement, TransformType);
					ControlElement->GetOffsetDirtyState().MarkDirty(TransformType);
					const FTransform ComputedTransform = HierarchyForLambda->GetControlOffsetTransform(ControlElement, TransformType);
					checkf(FTLLRN_RigComputedTransform::Equals(CachedTransform, ComputedTransform),
						TEXT("Element '%s' Offset %s Cached vs Computed doesn't match. ('%s' <-> '%s')"),
						*Element->GetName(),
						*TransformTypeString,
						*CachedTransform.ToString(),
						*ComputedTransform.ToString());
				}
			}
		}

		if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Element))
		{
			for(int32 TransformTypeIndex = 0; TransformTypeIndex < (int32) ETLLRN_RigTransformType::NumTransformTypes; TransformTypeIndex++)
			{
				const ETLLRN_RigTransformType::Type TransformType = (ETLLRN_RigTransformType::Type)TransformTypeIndex;
				const ETLLRN_RigTransformType::Type OpposedType = ETLLRN_RigTransformType::SwapLocalAndGlobal(TransformType);
				const FString& TransformTypeString = TransformTypeStrings[TransformTypeIndex];

				if(!TransformElement->GetDirtyState().IsDirty(TransformType) && !TransformElement->GetDirtyState().IsDirty(OpposedType))
				{
					const FTransform CachedTransform = HierarchyForLambda->GetTransform(TransformElement, TransformType);
					TransformElement->GetDirtyState().MarkDirty(TransformType);
					const FTransform ComputedTransform = HierarchyForLambda->GetTransform(TransformElement, TransformType);
					checkf(FTLLRN_RigComputedTransform::Equals(CachedTransform, ComputedTransform),
						TEXT("Element '%s' Pose %s Cached vs Computed doesn't match. ('%s' <-> '%s')"),
						*Element->GetName(),
						*TransformTypeString,
						*CachedTransform.ToString(), *ComputedTransform.ToString());
				}
			}
		}

		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
		{
			for(int32 TransformTypeIndex = 0; TransformTypeIndex < (int32) ETLLRN_RigTransformType::NumTransformTypes; TransformTypeIndex++)
			{
				const ETLLRN_RigTransformType::Type TransformType = (ETLLRN_RigTransformType::Type)TransformTypeIndex;
				const ETLLRN_RigTransformType::Type OpposedType = ETLLRN_RigTransformType::SwapLocalAndGlobal(TransformType);
				const FString& TransformTypeString = TransformTypeStrings[TransformTypeIndex];

				if(!ControlElement->GetShapeDirtyState().IsDirty(TransformType) && !ControlElement->GetShapeDirtyState().IsDirty(OpposedType))
				{
					const FTransform CachedTransform = HierarchyForLambda->GetControlShapeTransform(ControlElement, TransformType);
					ControlElement->GetShapeDirtyState().MarkDirty(TransformType);
					const FTransform ComputedTransform = HierarchyForLambda->GetControlShapeTransform(ControlElement, TransformType);
					checkf(FTLLRN_RigComputedTransform::Equals(CachedTransform, ComputedTransform),
						TEXT("Element '%s' Shape %s Cached vs Computed doesn't match. ('%s' <-> '%s')"),
						*Element->GetName(),
						*TransformTypeString,
						*CachedTransform.ToString(), *ComputedTransform.ToString());
				}
			}
		}
	});
}

#if WITH_EDITOR

UTLLRN_RigHierarchy::TElementDependencyMap UTLLRN_RigHierarchy::GetDependenciesForVM(const URigVM* InVM, FName InEventName) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	check(InVM);

	if(InEventName.IsNone())
	{
		InEventName = FTLLRN_RigUnit_BeginExecution().GetEventName();
	}

	UTLLRN_RigHierarchy::TElementDependencyMap Dependencies;
	const FRigVMInstructionArray Instructions = InVM->GetByteCode().GetInstructions();

	// if the VM doesn't implement the given event
	if(!InVM->ContainsEntry(InEventName))
	{
		return Dependencies;
	}

	// only represent instruction for a given event
	const int32 EntryIndex = InVM->GetByteCode().FindEntryIndex(InEventName);
	const int32 EntryInstructionIndex = InVM->GetByteCode().GetEntry(EntryIndex).InstructionIndex;

	TMap<FRigVMOperand, TArray<int32>> OperandToInstructions;

	for(int32 InstructionIndex = EntryInstructionIndex; InstructionIndex < Instructions.Num(); InstructionIndex++)
	{
		// early exit since following instructions belong to another event
		if(Instructions[InstructionIndex].OpCode == ERigVMOpCode::Exit)
		{
			break;
		}

		const FRigVMOperandArray InputOperands = InVM->GetByteCode().GetInputOperands(InstructionIndex);
		for(const FRigVMOperand InputOperand : InputOperands)
		{
			const FRigVMOperand InputOperandNoRegisterOffset(InputOperand.GetMemoryType(), InputOperand.GetRegisterIndex());
			OperandToInstructions.FindOrAdd(InputOperandNoRegisterOffset).Add(InstructionIndex);
		}
	}

	typedef TTuple<int32, int32> TInt32Tuple;
	TArray<TArray<TInt32Tuple>> ReadTransformPerInstruction, WrittenTransformsPerInstruction;
	
	// Find the max instruction index
	{
		/** TODO: UE-207320
		 * This function needs to be rewritten to account for modular rigs. Rig Hierarchies will have reads/writes coming from
		 * different VMs, so we cannot depend on the algorithm used in this function to find dependencies. */
		
		int32 MaxInstructionIndex = Instructions.Num();
		for(int32 RecordType = 0; RecordType < 2; RecordType++)
		{
			const TArray<TInstructionSliceElement>& Records = RecordType == 0 ? ReadTransformsAtRuntime : WrittenTransformsAtRuntime; 
			for(int32 RecordIndex = 0; RecordIndex < Records.Num(); RecordIndex++)
			{
				const TInstructionSliceElement& Record = Records[RecordIndex];
				const int32& InstructionIndex = Record.Get<0>();
				MaxInstructionIndex = FMath::Max(MaxInstructionIndex, InstructionIndex);
			}
		}
		ReadTransformPerInstruction.AddZeroed(MaxInstructionIndex+1);
		WrittenTransformsPerInstruction.AddZeroed(MaxInstructionIndex+1);
	}

	// fill lookup tables per instruction / element
	for(int32 RecordType = 0; RecordType < 2; RecordType++)
	{
		const TArray<TInstructionSliceElement>& Records = RecordType == 0 ? ReadTransformsAtRuntime : WrittenTransformsAtRuntime; 
		TArray<TArray<TInt32Tuple>>& PerInstruction = RecordType == 0 ? ReadTransformPerInstruction : WrittenTransformsPerInstruction;

		for(int32 RecordIndex = 0; RecordIndex < Records.Num(); RecordIndex++)
		{
			const TInstructionSliceElement& Record = Records[RecordIndex];
			const int32& InstructionIndex = Record.Get<0>();
			const int32& SliceIndex = Record.Get<1>();
			const int32& TransformIndex = Record.Get<2>();
			PerInstruction[InstructionIndex].Emplace(SliceIndex, TransformIndex);
		}
	}

	// for each read transform on an instruction
	// follow the operands to the next instruction affected by it
	TArray<TInt32Tuple> FilteredTransforms;
	TArray<int32> InstructionsToVisit;

	for(int32 InstructionIndex = EntryInstructionIndex; InstructionIndex < Instructions.Num(); InstructionIndex++)
	{
		// early exit since following instructions belong to another event
		if(Instructions[InstructionIndex].OpCode == ERigVMOpCode::Exit)
		{
			break;
		}

		const TArray<TInt32Tuple>& ReadTransforms = ReadTransformPerInstruction[InstructionIndex];
		if(ReadTransforms.IsEmpty())
		{
			continue;
		}

		FilteredTransforms.Reset();

		for(int32 ReadTransformIndex = 0; ReadTransformIndex < ReadTransforms.Num(); ReadTransformIndex++)
		{
			const TInt32Tuple& ReadTransform = ReadTransforms[ReadTransformIndex];
			
			InstructionsToVisit.Reset();
			InstructionsToVisit.Add(InstructionIndex);

			for(int32 InstructionToVisitIndex = 0; InstructionToVisitIndex < InstructionsToVisit.Num(); InstructionToVisitIndex++)
			{
				const int32 InstructionToVisit = InstructionsToVisit[InstructionToVisitIndex];

				const TArray<TInt32Tuple>& WrittenTransforms = WrittenTransformsPerInstruction[InstructionToVisit];
				for(int32 WrittenTransformIndex = 0; WrittenTransformIndex < WrittenTransforms.Num(); WrittenTransformIndex++)
				{
					const TInt32Tuple& WrittenTransform = WrittenTransforms[WrittenTransformIndex];

					// for the first instruction in this pass let's only care about
					// written transforms which have not been read before
					if(InstructionToVisitIndex == 0)
					{
						if(ReadTransforms.Contains(WrittenTransform))
						{
							continue;
						}
					}
					
					FilteredTransforms.AddUnique(WrittenTransform);
				}

				FRigVMOperandArray OutputOperands = InVM->GetByteCode().GetOutputOperands(InstructionToVisit);
				for(const FRigVMOperand OutputOperand : OutputOperands)
				{
					const FRigVMOperand OutputOperandNoRegisterOffset(OutputOperand.GetMemoryType(), OutputOperand.GetRegisterIndex());

					if(const TArray<int32>* InstructionsWithInputOperand = OperandToInstructions.Find(OutputOperandNoRegisterOffset))
					{
						for(int32 InstructionWithInputOperand : *InstructionsWithInputOperand)
						{
							InstructionsToVisit.AddUnique(InstructionWithInputOperand);
						}
					}
				}
			}
		}
		
		for(const TInt32Tuple& ReadTransform : ReadTransforms)
		{
			for(const TInt32Tuple& FilteredTransform : FilteredTransforms)
			{
				// only create dependencies for reads and writes that are on the same slice
				if(ReadTransform != FilteredTransform && ReadTransform.Get<0>() == FilteredTransform.Get<0>())
				{
					Dependencies.FindOrAdd(FilteredTransform.Get<1>()).AddUnique(ReadTransform.Get<1>());
				}
			}
		}
	}

	return Dependencies;
}

#endif

void UTLLRN_RigHierarchy::UpdateVisibilityOnProxyControls()
{
	UTLLRN_RigHierarchy* HierarchyForSelection = HierarchyForSelectionPtr.Get();
	if(HierarchyForSelection == nullptr)
	{
		HierarchyForSelection = this;
	}

	const UWorld* World = GetWorld();
	if(World == nullptr)
	{
		return;
	}
	if(World->IsPreviewWorld())
	{
		return;
	}

	// create a local map of visible things, starting with the selection
	TSet<FTLLRN_RigElementKey> VisibleElements;
	VisibleElements.Append(HierarchyForSelection->OrderedSelection);

	// if the control is visible - we should consider the
	// driven controls visible as well - so that other proxies
	// assigned to the same driven control also show up
	for(const FTLLRN_RigBaseElement* Element : Elements)
	{
		if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
		{
			if(ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl)
			{
				if(VisibleElements.Contains(ControlElement->GetKey()))
				{
					VisibleElements.Append(ControlElement->Settings.DrivenControls);
				}
			}
		}
	}

	for(FTLLRN_RigBaseElement* Element : Elements)
	{
		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
		{
			if(ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl)
			{
				if(ControlElement->Settings.ShapeVisibility == ETLLRN_RigControlVisibility::BasedOnSelection)
				{
					if(HierarchyForSelection->OrderedSelection.IsEmpty())
					{
						if(ControlElement->Settings.SetVisible(false, true))
						{
							Notify(ETLLRN_RigHierarchyNotification::ControlVisibilityChanged, ControlElement);
						}
					}
					else
					{
						// a proxy control should be visible if itself or a driven control is selected / visible
						bool bVisible = VisibleElements.Contains(ControlElement->GetKey());
						if(!bVisible)
						{
							if(ControlElement->Settings.DrivenControls.FindByPredicate([VisibleElements](const FTLLRN_RigElementKey& AffectedControl) -> bool
							{
								return VisibleElements.Contains(AffectedControl);
							}) != nullptr)
							{
								bVisible = true;
							}
						}

						if(bVisible)
						{
							VisibleElements.Add(ControlElement->GetKey());
						}

						if(ControlElement->Settings.SetVisible(bVisible, true))
						{
							Notify(ETLLRN_RigHierarchyNotification::ControlVisibilityChanged, ControlElement);
						}
					}
				}
			}
		}
	}
}

const TArray<FString>& UTLLRN_RigHierarchy::GetTransformTypeStrings()
{
	static TArray<FString> TransformTypeStrings;
	if(TransformTypeStrings.IsEmpty())
	{
		for(int32 TransformTypeIndex = 0; TransformTypeIndex < (int32) ETLLRN_RigTransformType::NumTransformTypes; TransformTypeIndex++)
		{
			TransformTypeStrings.Add(StaticEnum<ETLLRN_RigTransformType::Type>()->GetDisplayNameTextByValue((int64)TransformTypeIndex).ToString());
		}
	}
	return TransformTypeStrings;
}

void UTLLRN_RigHierarchy::PushTransformToStack(const FTLLRN_RigElementKey& InKey, ETLLRN_RigTransformStackEntryType InEntryType,
                                         ETLLRN_RigTransformType::Type InTransformType, const FTransform& InOldTransform, const FTransform& InNewTransform,
                                         bool bAffectChildren, bool bModify)
{
#if WITH_EDITOR

	if(GIsTransacting)
	{
		return;
	}

	static const FText TransformPoseTitle = NSLOCTEXT("TLLRN_RigHierarchy", "Set Pose Transform", "Set Pose Transform");
	static const FText ControlOffsetTitle = NSLOCTEXT("TLLRN_RigHierarchy", "Set Control Offset", "Set Control Offset");
	static const FText ControlShapeTitle = NSLOCTEXT("TLLRN_RigHierarchy", "Set Control Shape", "Set Control Shape");
	static const FText CurveValueTitle = NSLOCTEXT("TLLRN_RigHierarchy", "Set Curve Value", "Set Curve Value");
	
	FText Title;
	switch(InEntryType)
	{
		case ETLLRN_RigTransformStackEntryType::TransformPose:
		{
			Title = TransformPoseTitle;
			break;
		}
		case ETLLRN_RigTransformStackEntryType::ControlOffset:
		{
			Title = TransformPoseTitle;
			break;
		}
		case ETLLRN_RigTransformStackEntryType::ControlShape:
		{
			Title = TransformPoseTitle;
			break;
		}
		case ETLLRN_RigTransformStackEntryType::CurveValue:
		{
			Title = TransformPoseTitle;
			break;
		}
	}

	TGuardValue<bool> TransactingGuard(bTransactingForTransformChange, true);

	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bModify)
	{
		TransactionPtr = MakeShareable(new FScopedTransaction(Title));
	}

	if(bIsInteracting)
	{
		bool bCanMerge = LastInteractedKey == InKey;

		FTLLRN_RigTransformStackEntry LastEntry;
		if(!TransformUndoStack.IsEmpty())
		{
			LastEntry = TransformUndoStack.Last();
		}

		if(bCanMerge && LastEntry.Key == InKey && LastEntry.EntryType == InEntryType && LastEntry.bAffectChildren == bAffectChildren)
		{
			// merge the entries on the stack
			TransformUndoStack.Last() = 
                FTLLRN_RigTransformStackEntry(InKey, InEntryType, InTransformType, LastEntry.OldTransform, InNewTransform, bAffectChildren);
		}
		else
		{
			Modify();

			TransformUndoStack.Add(
                FTLLRN_RigTransformStackEntry(InKey, InEntryType, InTransformType, InOldTransform, InNewTransform, bAffectChildren));
			TransformStackIndex = TransformUndoStack.Num();
		}

		TransformRedoStack.Reset();
		LastInteractedKey = InKey;
		return;
	}

	if(bModify)
	{
		Modify();
	}

	TArray<FString> Callstack;
	if(IsTracingChanges() && (CVarTLLRN_ControlTLLRN_RigHierarchyTraceCallstack->GetInt() != 0))
	{
		FString JoinedCallStack;
		TLLRN_RigHierarchyCaptureCallStack(JoinedCallStack, 1);
		JoinedCallStack.ReplaceInline(TEXT("\r"), TEXT(""));

		FString Left, Right;
		do
		{
			if(!JoinedCallStack.Split(TEXT("\n"), &Left, &Right))
			{
				Left = JoinedCallStack;
				Right.Empty();
			}

			Left.TrimStartAndEndInline();
			if(Left.StartsWith(TEXT("0x")))
			{
				Left.Split(TEXT(" "), nullptr, &Left);
			}
			Callstack.Add(Left);
			JoinedCallStack = Right;
		}
		while(!JoinedCallStack.IsEmpty());
	}

	TransformUndoStack.Add(
		FTLLRN_RigTransformStackEntry(InKey, InEntryType, InTransformType, InOldTransform, InNewTransform, bAffectChildren, Callstack));
	TransformStackIndex = TransformUndoStack.Num();

	TransformRedoStack.Reset();
	
#endif
}

void UTLLRN_RigHierarchy::PushCurveToStack(const FTLLRN_RigElementKey& InKey, float InOldCurveValue, float InNewCurveValue, bool bInOldIsCurveValueSet, bool bInNewIsCurveValueSet, bool bModify)
{
#if WITH_EDITOR

	FTransform OldTransform = FTransform::Identity;
	FTransform NewTransform = FTransform::Identity;

	OldTransform.SetTranslation(FVector(InOldCurveValue, bInOldIsCurveValueSet ? 1.f : 0.f, 0.f));
	NewTransform.SetTranslation(FVector(InNewCurveValue, bInNewIsCurveValueSet ? 1.f : 0.f, 0.f));

	PushTransformToStack(InKey, ETLLRN_RigTransformStackEntryType::CurveValue, ETLLRN_RigTransformType::CurrentLocal, OldTransform, NewTransform, false, bModify);

#endif
}

bool UTLLRN_RigHierarchy::ApplyTransformFromStack(const FTLLRN_RigTransformStackEntry& InEntry, bool bUndo)
{
#if WITH_EDITOR

	bool bApplyInitialForCurrent = false;
	FTLLRN_RigBaseElement* Element = Find(InEntry.Key);
	if(Element == nullptr)
	{
		// this might be a transient control which had been removed.
		if(InEntry.Key.Type == ETLLRN_RigElementType::Control)
		{
			const FTLLRN_RigElementKey TargetKey = UTLLRN_ControlRig::GetElementKeyFromTransientControl(InEntry.Key);
			Element = Find(TargetKey);
			bApplyInitialForCurrent = Element != nullptr;
		}

		if(Element == nullptr)
		{
			return false;
		}
	}

	const FTransform& Transform = bUndo ? InEntry.OldTransform : InEntry.NewTransform;
	
	switch(InEntry.EntryType)
	{
		case ETLLRN_RigTransformStackEntryType::TransformPose:
		{
			SetTransform(Cast<FTLLRN_RigTransformElement>(Element), Transform, InEntry.TransformType, InEntry.bAffectChildren, false);

			if(ETLLRN_RigTransformType::IsCurrent(InEntry.TransformType) && bApplyInitialForCurrent)
			{
				SetTransform(Cast<FTLLRN_RigTransformElement>(Element), Transform, ETLLRN_RigTransformType::MakeInitial(InEntry.TransformType), InEntry.bAffectChildren, false);
			}
			break;
		}
		case ETLLRN_RigTransformStackEntryType::ControlOffset:
		{
			SetControlOffsetTransform(Cast<FTLLRN_RigControlElement>(Element), Transform, InEntry.TransformType, InEntry.bAffectChildren, false); 
			break;
		}
		case ETLLRN_RigTransformStackEntryType::ControlShape:
		{
			SetControlShapeTransform(Cast<FTLLRN_RigControlElement>(Element), Transform, InEntry.TransformType, false); 
			break;
		}
		case ETLLRN_RigTransformStackEntryType::CurveValue:
		{
			const float CurveValue = Transform.GetTranslation().X;
			SetCurveValue(Cast<FTLLRN_RigCurveElement>(Element), CurveValue, false);
			break;
		}
	}

	return true;
#else
	return false;
#endif
}

void UTLLRN_RigHierarchy::ComputeAllTransforms()
{
	for(int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
	{
		for(int32 TransformTypeIndex = 0; TransformTypeIndex < (int32) ETLLRN_RigTransformType::NumTransformTypes; TransformTypeIndex++)
		{
			const ETLLRN_RigTransformType::Type TransformType = (ETLLRN_RigTransformType::Type)TransformTypeIndex; 
			if(FTLLRN_RigControlElement* ControlElement = Get<FTLLRN_RigControlElement>(ElementIndex))
			{
				GetControlOffsetTransform(ControlElement, TransformType);
			}
			if(FTLLRN_RigTransformElement* TransformElement = Get<FTLLRN_RigTransformElement>(ElementIndex))
			{
				GetTransform(TransformElement, TransformType);
			}
			if(FTLLRN_RigControlElement* ControlElement = Get<FTLLRN_RigControlElement>(ElementIndex))
			{
				GetControlShapeTransform(ControlElement, TransformType);
			}
		}
	}
}

bool UTLLRN_RigHierarchy::IsAnimatable(const FTLLRN_RigElementKey& InKey) const
{
	if(const FTLLRN_RigControlElement* ControlElement = Find<FTLLRN_RigControlElement>(InKey))
	{
		return IsAnimatable(ControlElement);
	}
	return false;
}

bool UTLLRN_RigHierarchy::IsAnimatable(const FTLLRN_RigControlElement* InControlElement) const
{
	if(InControlElement)
	{
		if(!InControlElement->Settings.IsAnimatable())
		{
			return false;
		}

		// animation channels are dependent on the control they are under.
		if(InControlElement->IsAnimationChannel())
		{
			if(const FTLLRN_RigControlElement* ParentControlElement = Cast<FTLLRN_RigControlElement>(GetFirstParent(InControlElement)))
			{
				return IsAnimatable(ParentControlElement);
			}
		}
		
		return true;
	}
	return false;
}

bool UTLLRN_RigHierarchy::ShouldBeGrouped(const FTLLRN_RigElementKey& InKey) const
{
	if(const FTLLRN_RigControlElement* ControlElement = Find<FTLLRN_RigControlElement>(InKey))
	{
		return ShouldBeGrouped(ControlElement);
	}
	return false;
}

bool UTLLRN_RigHierarchy::ShouldBeGrouped(const FTLLRN_RigControlElement* InControlElement) const
{
	if(InControlElement)
	{
		if(!InControlElement->Settings.ShouldBeGrouped())
		{
			return false;
		}

		if(!GetChildren(InControlElement).IsEmpty())
		{
			return false;
		}

		if(const FTLLRN_RigControlElement* ParentControlElement = Cast<FTLLRN_RigControlElement>(GetFirstParent(InControlElement)))
		{
			return ParentControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::AnimationControl;
		}
	}
	return false;
}

FTransform UTLLRN_RigHierarchy::GetWorldTransformForReference(const FRigVMExecuteContext* InContext, const FTLLRN_RigElementKey& InKey, bool bInitial)
{
	if(const USceneComponent* OuterSceneComponent = GetTypedOuter<USceneComponent>())
	{
		return OuterSceneComponent->GetComponentToWorld().Inverse();
	}
	return FTransform::Identity;
}

FTransform UTLLRN_RigHierarchy::ComputeLocalControlValue(FTLLRN_RigControlElement* ControlElement,
	const FTransform& InGlobalTransform, ETLLRN_RigTransformType::Type InTransformType) const
{
	check(ETLLRN_RigTransformType::IsGlobal(InTransformType));

	const FTransform OffsetTransform =
		GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::MakeLocal(InTransformType));

	FTransform Result = InverseSolveParentConstraints(
		InGlobalTransform,
		ControlElement->ParentConstraints,
		InTransformType,
		OffsetTransform);

	return Result;
}

FTransform UTLLRN_RigHierarchy::SolveParentConstraints(
	const FTLLRN_RigElementParentConstraintArray& InConstraints,
	const ETLLRN_RigTransformType::Type InTransformType,
	const FTransform& InLocalOffsetTransform,
	bool bApplyLocalOffsetTransform,
	const FTransform& InLocalPoseTransform,
	bool bApplyLocalPoseTransform) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	FTransform Result = FTransform::Identity;
	const bool bInitial = IsInitial(InTransformType);

	// collect all of the weights
	FConstraintIndex FirstConstraint;
	FConstraintIndex SecondConstraint;
	FConstraintIndex NumConstraintsAffecting(0);
	FTLLRN_RigElementWeight TotalWeight(0.f);
	ComputeParentConstraintIndices(InConstraints, InTransformType, FirstConstraint, SecondConstraint, NumConstraintsAffecting, TotalWeight);

	// performance improvement for case of a single parent
	if(NumConstraintsAffecting.Location == 1 &&
		NumConstraintsAffecting.Rotation == 1 &&
		NumConstraintsAffecting.Scale == 1 &&
		FirstConstraint.Location == FirstConstraint.Rotation &&
		FirstConstraint.Location == FirstConstraint.Scale)
	{
		return LazilyComputeParentConstraint(InConstraints, FirstConstraint.Location, InTransformType,
			InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);
	}

	if(NumConstraintsAffecting.Location == 0 ||
		NumConstraintsAffecting.Rotation == 0 ||
		NumConstraintsAffecting.Scale == 0)
	{
		if(bApplyLocalOffsetTransform)
		{
			Result = InLocalOffsetTransform;
		}
		
		if(bApplyLocalPoseTransform)
		{
			Result = InLocalPoseTransform * Result;
		}

		if(NumConstraintsAffecting.Location == 0 &&
			NumConstraintsAffecting.Rotation == 0 &&
			NumConstraintsAffecting.Scale == 0)
		{
			Result.NormalizeRotation();
			return Result;
		}
	}

	if(NumConstraintsAffecting.Location == 1)
	{
		check(FirstConstraint.Location != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[FirstConstraint.Location];
		const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
		const FTransform Transform = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Location, InTransformType,
			InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);

		check(Weight.AffectsLocation());
		Result.SetLocation(Transform.GetLocation());
	}
	else if(NumConstraintsAffecting.Location == 2)
	{
		check(FirstConstraint.Location != INDEX_NONE);
		check(SecondConstraint.Location != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraintA = InConstraints[FirstConstraint.Location];
		const FTLLRN_RigElementParentConstraint& ParentConstraintB = InConstraints[SecondConstraint.Location];

		const FTLLRN_RigElementWeight& WeightA = ParentConstraintA.GetWeight(bInitial); 
		const FTLLRN_RigElementWeight& WeightB = ParentConstraintB.GetWeight(bInitial);
		check(WeightA.AffectsLocation());
		check(WeightB.AffectsLocation());
		const float Weight = GetWeightForLerp(WeightA.Location, WeightB.Location);

		const FTransform TransformA = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Location, InTransformType,
			InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);
		const FTransform TransformB = LazilyComputeParentConstraint(InConstraints, SecondConstraint.Location, InTransformType,
			InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);

		const FVector ParentLocationA = TransformA.GetLocation();
		const FVector ParentLocationB = TransformB.GetLocation();
		Result.SetLocation(FMath::Lerp<FVector>(ParentLocationA, ParentLocationB, Weight));
	}
	else if(NumConstraintsAffecting.Location > 2)
	{
		check(TotalWeight.Location > SMALL_NUMBER);
		
		FVector Location = FVector::ZeroVector;
		
		for(int32 ConstraintIndex = 0; ConstraintIndex < InConstraints.Num(); ConstraintIndex++)
		{
			const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[ConstraintIndex];
			const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
			if(!Weight.AffectsLocation())
			{
				continue;
			}

			const FTransform Transform = LazilyComputeParentConstraint(InConstraints, ConstraintIndex, InTransformType,
				InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);

			IntegrateParentConstraintVector(Location, Transform, Weight.Location / TotalWeight.Location, true);
		}

		Result.SetLocation(Location);
	}

	if(NumConstraintsAffecting.Rotation == 1)
	{
		check(FirstConstraint.Rotation != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[FirstConstraint.Rotation];
		const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
		const FTransform Transform = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Rotation, InTransformType,
			InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);
		check(Weight.AffectsRotation());
		Result.SetRotation(Transform.GetRotation());
	}
	else if(NumConstraintsAffecting.Rotation == 2)
	{
		check(FirstConstraint.Rotation != INDEX_NONE);
		check(SecondConstraint.Rotation != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraintA = InConstraints[FirstConstraint.Rotation];
		const FTLLRN_RigElementParentConstraint& ParentConstraintB = InConstraints[SecondConstraint.Rotation];

		const FTLLRN_RigElementWeight& WeightA = ParentConstraintA.GetWeight(bInitial); 
		const FTLLRN_RigElementWeight& WeightB = ParentConstraintB.GetWeight(bInitial);
		check(WeightA.AffectsRotation());
		check(WeightB.AffectsRotation());
		const float Weight = GetWeightForLerp(WeightA.Rotation, WeightB.Rotation);

		const FTransform TransformA = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Rotation, InTransformType,
			InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);
		const FTransform TransformB = LazilyComputeParentConstraint(InConstraints, SecondConstraint.Rotation, InTransformType,
			InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);

		const FQuat ParentRotationA = TransformA.GetRotation();
		const FQuat ParentRotationB = TransformB.GetRotation();
		Result.SetRotation(FQuat::Slerp(ParentRotationA, ParentRotationB, Weight));
	}
	else if(NumConstraintsAffecting.Rotation > 2)
	{
		check(TotalWeight.Rotation > SMALL_NUMBER);
		
		int NumMixedRotations = 0;
		FQuat FirstRotation = FQuat::Identity;
		FQuat MixedRotation = FQuat(0.f, 0.f, 0.f, 0.f);

		for(int32 ConstraintIndex = 0; ConstraintIndex < InConstraints.Num(); ConstraintIndex++)
		{
			const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[ConstraintIndex];
			const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
			if(!Weight.AffectsRotation())
			{
				continue;
			}

			const FTransform Transform = LazilyComputeParentConstraint(InConstraints, ConstraintIndex, InTransformType,
				InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);

			IntegrateParentConstraintQuat(
				NumMixedRotations,
				FirstRotation,
				MixedRotation,
				Transform,
				Weight.Rotation / TotalWeight.Rotation);
		}

		Result.SetRotation(MixedRotation.GetNormalized());
	}

	if(NumConstraintsAffecting.Scale == 1)
	{
		check(FirstConstraint.Scale != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[FirstConstraint.Scale];
		const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
		
		const FTransform Transform = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Scale, InTransformType,
			InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);

		check(Weight.AffectsScale());
		Result.SetScale3D(Transform.GetScale3D());
	}
	else if(NumConstraintsAffecting.Scale == 2)
	{
		check(FirstConstraint.Scale != INDEX_NONE);
		check(SecondConstraint.Scale != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraintA = InConstraints[FirstConstraint.Scale];
		const FTLLRN_RigElementParentConstraint& ParentConstraintB = InConstraints[SecondConstraint.Scale];

		const FTLLRN_RigElementWeight& WeightA = ParentConstraintA.GetWeight(bInitial); 
		const FTLLRN_RigElementWeight& WeightB = ParentConstraintB.GetWeight(bInitial);
		check(WeightA.AffectsScale());
		check(WeightB.AffectsScale());
		const float Weight = GetWeightForLerp(WeightA.Scale, WeightB.Scale);

		const FTransform TransformA = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Scale, InTransformType,
			InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);
		const FTransform TransformB = LazilyComputeParentConstraint(InConstraints, SecondConstraint.Scale, InTransformType,
			InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);

		const FVector ParentScaleA = TransformA.GetScale3D();
		const FVector ParentScaleB = TransformB.GetScale3D();
		Result.SetScale3D(FMath::Lerp<FVector>(ParentScaleA, ParentScaleB, Weight));
	}
	else if(NumConstraintsAffecting.Scale > 2)
	{
		check(TotalWeight.Scale > SMALL_NUMBER);
		
		FVector Scale = FVector::ZeroVector;
		
		for(int32 ConstraintIndex = 0; ConstraintIndex < InConstraints.Num(); ConstraintIndex++)
		{
			const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[ConstraintIndex];
			const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
			if(!Weight.AffectsScale())
			{
				continue;
			}

			const FTransform Transform = LazilyComputeParentConstraint(InConstraints, ConstraintIndex, InTransformType,
				InLocalOffsetTransform, bApplyLocalOffsetTransform, InLocalPoseTransform, bApplyLocalPoseTransform);

			IntegrateParentConstraintVector(Scale, Transform, Weight.Scale / TotalWeight.Scale, false);
		}

		Result.SetScale3D(Scale);
	}

	Result.NormalizeRotation();
	return Result;
}

FTransform UTLLRN_RigHierarchy::InverseSolveParentConstraints(
	const FTransform& InGlobalTransform,
	const FTLLRN_RigElementParentConstraintArray& InConstraints,
	const ETLLRN_RigTransformType::Type InTransformType,
	const FTransform& InLocalOffsetTransform) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));

	FTransform Result = FTransform::Identity;

	// this function is doing roughly the following 
	// ResultLocalTransform = InGlobalTransform.GetRelative(ParentGlobal)
	// InTransformType is only used to determine Initial vs Current
	const bool bInitial = IsInitial(InTransformType);
	check(ETLLRN_RigTransformType::IsGlobal(InTransformType));

	// collect all of the weights
	FConstraintIndex FirstConstraint;
	FConstraintIndex SecondConstraint;
	FConstraintIndex NumConstraintsAffecting(0);
	FTLLRN_RigElementWeight TotalWeight(0.f);
	ComputeParentConstraintIndices(InConstraints, InTransformType, FirstConstraint, SecondConstraint, NumConstraintsAffecting, TotalWeight);

	// performance improvement for case of a single parent
	if(NumConstraintsAffecting.Location == 1 &&
		NumConstraintsAffecting.Rotation == 1 &&
		NumConstraintsAffecting.Scale == 1 &&
		FirstConstraint.Location == FirstConstraint.Rotation &&
		FirstConstraint.Location == FirstConstraint.Scale)
	{
		const FTransform Transform = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Location, InTransformType,
			InLocalOffsetTransform, true, FTransform::Identity, false);

		return InGlobalTransform.GetRelativeTransform(Transform);
	}

	if(NumConstraintsAffecting.Location == 0 ||
		NumConstraintsAffecting.Rotation == 0 ||
		NumConstraintsAffecting.Scale == 0)
	{
		Result = InGlobalTransform.GetRelativeTransform(InLocalOffsetTransform);
		
		if(NumConstraintsAffecting.Location == 0 &&
			NumConstraintsAffecting.Rotation == 0 &&
			NumConstraintsAffecting.Scale == 0)
		{
			Result.NormalizeRotation();
			return Result;
		}
	}

	if(NumConstraintsAffecting.Location == 1)
	{
		check(FirstConstraint.Location != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[FirstConstraint.Location];
		const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
		const FTransform Transform = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Location, InTransformType,
			InLocalOffsetTransform, true, FTransform::Identity, false);

		check(Weight.AffectsLocation());
		Result.SetLocation(InGlobalTransform.GetRelativeTransform(Transform).GetLocation());
	}
	else if(NumConstraintsAffecting.Location == 2)
	{
		check(FirstConstraint.Location != INDEX_NONE);
		check(SecondConstraint.Location != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraintA = InConstraints[FirstConstraint.Location];
		const FTLLRN_RigElementParentConstraint& ParentConstraintB = InConstraints[SecondConstraint.Location];

		const FTLLRN_RigElementWeight& WeightA = ParentConstraintA.GetWeight(bInitial); 
		const FTLLRN_RigElementWeight& WeightB = ParentConstraintB.GetWeight(bInitial);
		check(WeightA.AffectsLocation());
		check(WeightB.AffectsLocation());
		const float Weight = GetWeightForLerp(WeightA.Location, WeightB.Location);

		const FTransform TransformA = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Location, InTransformType,
			InLocalOffsetTransform, true, FTransform::Identity, false);
		const FTransform TransformB = LazilyComputeParentConstraint(InConstraints, SecondConstraint.Location, InTransformType,
			InLocalOffsetTransform, true, FTransform::Identity, false);

		const FTransform MixedTransform = FTLLRN_ControlRigMathLibrary::LerpTransform(TransformA, TransformB, Weight);
		Result.SetLocation(InGlobalTransform.GetRelativeTransform(MixedTransform).GetLocation());
	}
	else if(NumConstraintsAffecting.Location > 2)
	{
		check(TotalWeight.Location > SMALL_NUMBER);
		
		FVector Location = FVector::ZeroVector;
		int NumMixedRotations = 0;
		FQuat FirstRotation = FQuat::Identity;
		FQuat MixedRotation = FQuat(0.f, 0.f, 0.f, 0.f);
		FVector Scale = FVector::ZeroVector;

		for(int32 ConstraintIndex = 0; ConstraintIndex < InConstraints.Num(); ConstraintIndex++)
		{
			const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[ConstraintIndex];
			const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
			if(!Weight.AffectsLocation())
			{
				continue;
			}

			const FTransform Transform = LazilyComputeParentConstraint(InConstraints, ConstraintIndex, InTransformType,
				InLocalOffsetTransform, true, FTransform::Identity, false);

			const float NormalizedWeight = Weight.Location / TotalWeight.Location;
			IntegrateParentConstraintVector(Location, Transform, NormalizedWeight, true);
			IntegrateParentConstraintQuat(NumMixedRotations, FirstRotation, MixedRotation, Transform, NormalizedWeight);
			IntegrateParentConstraintVector(Scale, Transform, NormalizedWeight, false);
		}

		FTransform ParentTransform(MixedRotation.GetNormalized(), Location, Scale);
		Result.SetLocation(InGlobalTransform.GetRelativeTransform(ParentTransform).GetLocation());
	}

	if(NumConstraintsAffecting.Rotation == 1)
	{
		check(FirstConstraint.Rotation != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[FirstConstraint.Rotation];
		const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
		const FTransform Transform = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Rotation, InTransformType,
			InLocalOffsetTransform, true, FTransform::Identity, false);
		check(Weight.AffectsRotation());
		Result.SetRotation(InGlobalTransform.GetRelativeTransform(Transform).GetRotation());
	}
	else if(NumConstraintsAffecting.Rotation == 2)
	{
		check(FirstConstraint.Rotation != INDEX_NONE);
		check(SecondConstraint.Rotation != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraintA = InConstraints[FirstConstraint.Rotation];
		const FTLLRN_RigElementParentConstraint& ParentConstraintB = InConstraints[SecondConstraint.Rotation];

		const FTLLRN_RigElementWeight& WeightA = ParentConstraintA.GetWeight(bInitial); 
		const FTLLRN_RigElementWeight& WeightB = ParentConstraintB.GetWeight(bInitial);
		check(WeightA.AffectsRotation());
		check(WeightB.AffectsRotation());
		const float Weight = GetWeightForLerp(WeightA.Rotation, WeightB.Rotation);

		const FTransform TransformA = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Rotation, InTransformType,
			InLocalOffsetTransform, true, FTransform::Identity, false);
		const FTransform TransformB = LazilyComputeParentConstraint(InConstraints, SecondConstraint.Rotation, InTransformType,
			InLocalOffsetTransform, true, FTransform::Identity, false);

		const FTransform MixedTransform = FTLLRN_ControlRigMathLibrary::LerpTransform(TransformA, TransformB, Weight);
		Result.SetRotation(InGlobalTransform.GetRelativeTransform(MixedTransform).GetRotation());
	}
	else if(NumConstraintsAffecting.Rotation > 2)
	{
		check(TotalWeight.Rotation > SMALL_NUMBER);
		
		FVector Location = FVector::ZeroVector;
		int NumMixedRotations = 0;
		FQuat FirstRotation = FQuat::Identity;
		FQuat MixedRotation = FQuat(0.f, 0.f, 0.f, 0.f);
		FVector Scale = FVector::ZeroVector;

		for(int32 ConstraintIndex = 0; ConstraintIndex < InConstraints.Num(); ConstraintIndex++)
		{
			const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[ConstraintIndex];
			const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
			if(!Weight.AffectsRotation())
			{
				continue;
			}

			const FTransform Transform = LazilyComputeParentConstraint(InConstraints, ConstraintIndex, InTransformType,
				InLocalOffsetTransform, true, FTransform::Identity, false);

			const float NormalizedWeight = Weight.Rotation / TotalWeight.Rotation;
			IntegrateParentConstraintVector(Location, Transform, NormalizedWeight, true);
			IntegrateParentConstraintQuat(NumMixedRotations, FirstRotation, MixedRotation, Transform, NormalizedWeight);
			IntegrateParentConstraintVector(Scale, Transform, NormalizedWeight, false);
		}

		FTransform ParentTransform(MixedRotation.GetNormalized(), Location, Scale);
		Result.SetRotation(InGlobalTransform.GetRelativeTransform(ParentTransform).GetRotation());
	}

	if(NumConstraintsAffecting.Scale == 1)
	{
		check(FirstConstraint.Scale != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[FirstConstraint.Scale];
		const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
		
		const FTransform Transform = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Scale, InTransformType,
			InLocalOffsetTransform, true, FTransform::Identity, false);

		check(Weight.AffectsScale());
		Result.SetScale3D(InGlobalTransform.GetRelativeTransform(Transform).GetScale3D());
	}
	else if(NumConstraintsAffecting.Scale == 2)
	{
		check(FirstConstraint.Scale != INDEX_NONE);
		check(SecondConstraint.Scale != INDEX_NONE);

		const FTLLRN_RigElementParentConstraint& ParentConstraintA = InConstraints[FirstConstraint.Scale];
		const FTLLRN_RigElementParentConstraint& ParentConstraintB = InConstraints[SecondConstraint.Scale];

		const FTLLRN_RigElementWeight& WeightA = ParentConstraintA.GetWeight(bInitial); 
		const FTLLRN_RigElementWeight& WeightB = ParentConstraintB.GetWeight(bInitial);
		check(WeightA.AffectsScale());
		check(WeightB.AffectsScale());
		const float Weight = GetWeightForLerp(WeightA.Scale, WeightB.Scale);

		const FTransform TransformA = LazilyComputeParentConstraint(InConstraints, FirstConstraint.Scale, InTransformType,
			InLocalOffsetTransform, true, FTransform::Identity, false);
		const FTransform TransformB = LazilyComputeParentConstraint(InConstraints, SecondConstraint.Scale, InTransformType,
			InLocalOffsetTransform, true, FTransform::Identity, false);

		const FTransform MixedTransform = FTLLRN_ControlRigMathLibrary::LerpTransform(TransformA, TransformB, Weight);
		Result.SetScale3D(InGlobalTransform.GetRelativeTransform(MixedTransform).GetScale3D());
	}
	else if(NumConstraintsAffecting.Scale > 2)
	{
		check(TotalWeight.Scale > SMALL_NUMBER);
		
		FVector Location = FVector::ZeroVector;
		int NumMixedRotations = 0;
		FQuat FirstRotation = FQuat::Identity;
		FQuat MixedRotation = FQuat(0.f, 0.f, 0.f, 0.f);
		FVector Scale = FVector::ZeroVector;
		
		for(int32 ConstraintIndex = 0; ConstraintIndex < InConstraints.Num(); ConstraintIndex++)
		{
			const FTLLRN_RigElementParentConstraint& ParentConstraint = InConstraints[ConstraintIndex];
			const FTLLRN_RigElementWeight& Weight = ParentConstraint.GetWeight(bInitial);
			if(!Weight.AffectsScale())
			{
				continue;
			}

			const FTransform Transform = LazilyComputeParentConstraint(InConstraints, ConstraintIndex, InTransformType,
				InLocalOffsetTransform, true, FTransform::Identity, false);

			const float NormalizedWeight = Weight.Scale / TotalWeight.Scale;
			IntegrateParentConstraintVector(Location, Transform, NormalizedWeight, true);
			IntegrateParentConstraintQuat(NumMixedRotations, FirstRotation, MixedRotation, Transform, NormalizedWeight);
			IntegrateParentConstraintVector(Scale, Transform, NormalizedWeight, false);
		}

		FTransform ParentTransform(MixedRotation.GetNormalized(), Location, Scale);
		Result.SetScale3D(InGlobalTransform.GetRelativeTransform(ParentTransform).GetScale3D());
	}

	Result.NormalizeRotation();
	return Result;
}

FTransform UTLLRN_RigHierarchy::LazilyComputeParentConstraint(
	const FTLLRN_RigElementParentConstraintArray& InConstraints,
	int32 InIndex,
	const ETLLRN_RigTransformType::Type InTransformType,
	const FTransform& InLocalOffsetTransform,
	bool bApplyLocalOffsetTransform,
	const FTransform& InLocalPoseTransform,
	bool bApplyLocalPoseTransform) const
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	const FTLLRN_RigElementParentConstraint& Constraint = InConstraints[InIndex];
	if(Constraint.bCacheIsDirty)
	{
		FTransform Transform = GetTransform(Constraint.ParentElement, InTransformType);
		if(bApplyLocalOffsetTransform)
		{
			Transform = InLocalOffsetTransform * Transform;
		}
		if(bApplyLocalPoseTransform)
		{
			Transform = InLocalPoseTransform * Transform;
		}

		Transform.NormalizeRotation();
		Constraint.Cache = Transform;
		Constraint.bCacheIsDirty = false;
	}
	return Constraint.Cache;
}

void UTLLRN_RigHierarchy::ComputeParentConstraintIndices(
	const FTLLRN_RigElementParentConstraintArray& InConstraints,
	ETLLRN_RigTransformType::Type InTransformType,
	FConstraintIndex& OutFirstConstraint,
	FConstraintIndex& OutSecondConstraint,
	FConstraintIndex& OutNumConstraintsAffecting,
	FTLLRN_RigElementWeight& OutTotalWeight)
{
	const bool bInitial = IsInitial(InTransformType);
	
	// find all of the weights affecting this output
	for(int32 ConstraintIndex = 0; ConstraintIndex < InConstraints.Num(); ConstraintIndex++)
	{
		// this is not relying on the cache whatsoever. we might as well remove it.
		InConstraints[ConstraintIndex].bCacheIsDirty = true;
		
		const FTLLRN_RigElementWeight& Weight = InConstraints[ConstraintIndex].GetWeight(bInitial);
		if(Weight.AffectsLocation())
		{
			OutNumConstraintsAffecting.Location++;
			OutTotalWeight.Location += Weight.Location;

			if(OutFirstConstraint.Location == INDEX_NONE)
			{
				OutFirstConstraint.Location = ConstraintIndex;
			}
			else if(OutSecondConstraint.Location == INDEX_NONE)
			{
				OutSecondConstraint.Location = ConstraintIndex;
			}
		}
		if(Weight.AffectsRotation())
		{
			OutNumConstraintsAffecting.Rotation++;
			OutTotalWeight.Rotation += Weight.Rotation;

			if(OutFirstConstraint.Rotation == INDEX_NONE)
			{
				OutFirstConstraint.Rotation = ConstraintIndex;
			}
			else if(OutSecondConstraint.Rotation == INDEX_NONE)
			{
				OutSecondConstraint.Rotation = ConstraintIndex;
			}
		}
		if(Weight.AffectsScale())
		{
			OutNumConstraintsAffecting.Scale++;
			OutTotalWeight.Scale += Weight.Scale;

			if(OutFirstConstraint.Scale == INDEX_NONE)
			{
				OutFirstConstraint.Scale = ConstraintIndex;
			}
			else if(OutSecondConstraint.Scale == INDEX_NONE)
			{
				OutSecondConstraint.Scale = ConstraintIndex;
			}
		}
	}
}
void UTLLRN_RigHierarchy::IntegrateParentConstraintVector(
	FVector& OutVector,
	const FTransform& InTransform,
	float InWeight,
	bool bIsLocation)
{
	if(bIsLocation)
	{
		OutVector += InTransform.GetLocation() * InWeight;
	}
	else
	{
		OutVector += InTransform.GetScale3D() * InWeight;
	}
}

void UTLLRN_RigHierarchy::IntegrateParentConstraintQuat(
	int32& OutNumMixedRotations,
	FQuat& OutFirstRotation,
	FQuat& OutMixedRotation,
	const FTransform& InTransform,
	float InWeight)
{
	LLM_SCOPE_BYNAME(TEXT("Animation/TLLRN_ControlRig"));
	FQuat ParentRotation = InTransform.GetRotation().GetNormalized();

	if(OutNumMixedRotations == 0)
	{
		OutFirstRotation = ParentRotation; 
	}
	else if ((ParentRotation | OutFirstRotation) <= 0.f)
	{
		InWeight = -InWeight;
	}

	OutMixedRotation.X += InWeight * ParentRotation.X;
	OutMixedRotation.Y += InWeight * ParentRotation.Y;
	OutMixedRotation.Z += InWeight * ParentRotation.Z;
	OutMixedRotation.W += InWeight * ParentRotation.W;
	OutNumMixedRotations++;
}

#if WITH_EDITOR
TArray<FString> UTLLRN_RigHierarchy::ControlSettingsToPythonCommands(const FTLLRN_RigControlSettings& Settings, const FString& NameSettings)
{
	TArray<FString> Commands;
	Commands.Add(FString::Printf(TEXT("%s = unreal.TLLRN_RigControlSettings()"),
			*NameSettings));

	ETLLRN_RigControlType ControlType = Settings.ControlType;
	switch(ControlType)
	{
		case ETLLRN_RigControlType::Transform:
		case ETLLRN_RigControlType::TransformNoScale:
		{
			ControlType = ETLLRN_RigControlType::EulerTransform;
			break;
		}
		default:
		{
			break;
		}
	}

	const FString AnimationTypeStr = RigVMPythonUtils::EnumValueToPythonString<ETLLRN_RigControlAnimationType>((int64)Settings.AnimationType);
	const FString ControlTypeStr = RigVMPythonUtils::EnumValueToPythonString<ETLLRN_RigControlType>((int64)ControlType);

	static const TCHAR* TrueText = TEXT("True");
	static const TCHAR* FalseText = TEXT("False");

	TArray<FString> LimitEnabledParts;
	for(const FTLLRN_RigControlLimitEnabled& LimitEnabled : Settings.LimitEnabled)
	{
		LimitEnabledParts.Add(FString::Printf(TEXT("unreal.TLLRN_RigControlLimitEnabled(%s, %s)"),
						   LimitEnabled.bMinimum ? TrueText : FalseText,
						   LimitEnabled.bMaximum ? TrueText : FalseText));
	}
	
	const FString LimitEnabledStr = FString::Join(LimitEnabledParts, TEXT(", "));
	
	Commands.Add(FString::Printf(TEXT("%s.animation_type = %s"),
									*NameSettings,
									*AnimationTypeStr));
	Commands.Add(FString::Printf(TEXT("%s.control_type = %s"),
									*NameSettings,
									*ControlTypeStr));
	Commands.Add(FString::Printf(TEXT("%s.display_name = '%s'"),
		*NameSettings,
		*Settings.DisplayName.ToString()));
	Commands.Add(FString::Printf(TEXT("%s.draw_limits = %s"),
		*NameSettings,
		Settings.bDrawLimits ? TrueText : FalseText));
	Commands.Add(FString::Printf(TEXT("%s.shape_color = %s"),
		*NameSettings,
		*RigVMPythonUtils::LinearColorToPythonString(Settings.ShapeColor)));
	Commands.Add(FString::Printf(TEXT("%s.shape_name = '%s'"),
		*NameSettings,
		*Settings.ShapeName.ToString()));
	Commands.Add(FString::Printf(TEXT("%s.shape_visible = %s"),
		*NameSettings,
		Settings.bShapeVisible ? TrueText : FalseText));
	Commands.Add(FString::Printf(TEXT("%s.is_transient_control = %s"),
		*NameSettings,
		Settings.bIsTransientControl ? TrueText : FalseText));
	Commands.Add(FString::Printf(TEXT("%s.limit_enabled = [%s]"),
		*NameSettings,
		*LimitEnabledStr));
	Commands.Add(FString::Printf(TEXT("%s.minimum_value = %s"),
		*NameSettings,
		*Settings.MinimumValue.ToPythonString(Settings.ControlType)));
	Commands.Add(FString::Printf(TEXT("%s.maximum_value = %s"),
		*NameSettings,
		*Settings.MaximumValue.ToPythonString(Settings.ControlType)));
	Commands.Add(FString::Printf(TEXT("%s.primary_axis = %s"),
		*NameSettings,
		*RigVMPythonUtils::EnumValueToPythonString<ETLLRN_RigControlAxis>((int64)Settings.PrimaryAxis)));

	return Commands;
}

TArray<FString> UTLLRN_RigHierarchy::ConnectorSettingsToPythonCommands(const FTLLRN_RigConnectorSettings& Settings, const FString& NameSettings)
{
	TArray<FString> Commands;
	Commands.Add(FString::Printf(TEXT("%s = unreal.TLLRN_RigConnectorSettings()"),
			*NameSettings));

	// no content values just yet - we are skipping the ResolvedItem here since
	// we don't assume it is going to be resolved initially.

	return Commands;
}

#endif

FTLLRN_RigHierarchyRedirectorGuard::FTLLRN_RigHierarchyRedirectorGuard(UTLLRN_ControlRig* InTLLRN_ControlRig)
: Guard(InTLLRN_ControlRig->GetHierarchy()->ElementKeyRedirector, &InTLLRN_ControlRig->GetElementKeyRedirector())
{
}
;

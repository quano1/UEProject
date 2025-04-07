// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigBlueprint.h"

#include "RigVMBlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "Engine/SkeletalMesh.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "TLLRN_ControlRig.h"
#include "Graph/TLLRN_ControlRigGraph.h"
#include "Graph/TLLRN_ControlRigGraphSchema.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectGlobals.h"
#include "TLLRN_ControlRigObjectVersion.h"
#include "BlueprintCompilationManager.h"
#include "TLLRN_ModularRig.h"
#include "TLLRN_ModularTLLRN_RigController.h"
#include "RigVMCompiler/RigVMCompiler.h"
#include "RigVMCore/RigVMRegistry.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetBoneTransform.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "RigVMPythonUtils.h"
#include "RigVMTypeUtils.h"
#include "RigVMModel/Nodes/RigVMAggregateNode.h"
#include "Rigs/TLLRN_TLLRN_RigControlHierarchy.h"
#include "Settings/TLLRN_ControlRigSettings.h"
#include "Units/TLLRN_ControlRigNodeWorkflow.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Units/Execution/TLLRN_RigUnit_DynamicHierarchy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigBlueprint)

#if WITH_EDITOR
#include "ITLLRN_ControlRigEditorModule.h"
#include "Kismet2/WatchedPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Editor/UnrealEdEngine.h"
#include "Editor/Transactor.h"
#include "CookOnTheSide/CookOnTheFlyServer.h"
#include "ScopedTransaction.h"
#include "Algo/Count.h"
#endif//WITH_EDITOR

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigBlueprint"

TArray<UTLLRN_ControlRigBlueprint*> UTLLRN_ControlRigBlueprint::sCurrentlyOpenedRigBlueprints;

UTLLRN_ControlRigBlueprint::UTLLRN_ControlRigBlueprint(const FObjectInitializer& ObjectInitializer)
	: URigVMBlueprint(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	GizmoLibrary_DEPRECATED = nullptr;
	ShapeLibraries.Add(UTLLRN_ControlRigSettings::Get()->DefaultShapeLibrary);
#endif

	Validator = ObjectInitializer.CreateDefaultSubobject<UTLLRN_ControlRigValidator>(this, TEXT("TLLRN_ControlRigValidator"));
	DebugBoneRadius = 1.f;

	bExposesAnimatableControls = false;

	Hierarchy = CreateDefaultSubobject<UTLLRN_RigHierarchy>(TEXT("Hierarchy"));
	UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
	// give BP a chance to propagate hierarchy changes to available control rig instances
	Controller->OnModified().AddUObject(this, &UTLLRN_ControlRigBlueprint::HandleHierarchyModified);

	if(GetClass() == UTLLRN_ControlRigBlueprint::StaticClass())
	{
		CommonInitialization(ObjectInitializer);
	}

	TLLRN_ModularRigModel.SetOuterClientHost(this);
	UTLLRN_ModularTLLRN_RigController* ModularController = TLLRN_ModularRigModel.GetController();
	ModularController->OnModified().AddUObject(this, &UTLLRN_ControlRigBlueprint::HandleTLLRN_RigModulesModified);
	
}

UTLLRN_ControlRigBlueprint::UTLLRN_ControlRigBlueprint()
{
	ModulesRecompilationBracket = 0;
}

UClass* UTLLRN_ControlRigBlueprint::RegenerateClass(UClass* ClassToRegenerate, UObject* PreviousCDO)
{
	UClass* Result = Super::RegenerateClass(ClassToRegenerate, PreviousCDO);
	Hierarchy->CleanupInvalidCaches();
	PropagateHierarchyFromBPToInstances();
	return Result;
}

bool UTLLRN_ControlRigBlueprint::RequiresForceLoadMembers(UObject* InObject) const
{
	// old assets don't support preload filtering
	if (GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::RemoveParameters)
	{
		return UBlueprint::RequiresForceLoadMembers(InObject);
	}
	
	return Super::RequiresForceLoadMembers(InObject);
}

void UTLLRN_ControlRigBlueprint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// if this is any of our external variables we need to request construction so that the rig rebuilds itself
	if(NewVariables.ContainsByPredicate([&PropertyChangedEvent](const FBPVariableDescription& Variable)
	{
		return Variable.VarName == PropertyChangedEvent.GetMemberPropertyName();
	}))
	{
		if(UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetObjectBeingDebugged()))
		{
			if(const FProperty* PropertyOnRig = DebuggedTLLRN_ControlRig->GetClass()->FindPropertyByName(PropertyChangedEvent.MemberProperty->GetFName()))
			{
				if(PropertyOnRig->SameType(PropertyChangedEvent.MemberProperty))
				{
					UTLLRN_ControlRig* CDO = DebuggedTLLRN_ControlRig->GetClass()->GetDefaultObject<UTLLRN_ControlRig>();
					const uint8* SourceMemory = PropertyOnRig->ContainerPtrToValuePtr<uint8>(CDO);
					uint8* TargetMemory = PropertyOnRig->ContainerPtrToValuePtr<uint8>(DebuggedTLLRN_ControlRig);
					PropertyOnRig->CopyCompleteValue(TargetMemory, SourceMemory);
				}
			}
			DebuggedTLLRN_ControlRig->RequestConstruction();
		}
	}
}

void UTLLRN_ControlRigBlueprint::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Propagate shape libraries
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, ShapeLibraries))
	{
		URigVMBlueprintGeneratedClass* RigClass = GetRigVMBlueprintGeneratedClass();
		UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(false /* create if needed */));

		TArray<UObject*> ArchetypeInstances;
		CDO->GetArchetypeInstances(ArchetypeInstances);
		ArchetypeInstances.Add(CDO);

		// Propagate libraries to archetypes
		for (UObject* Instance : ArchetypeInstances)
		{
			if (UTLLRN_ControlRig* InstanceRig = Cast<UTLLRN_ControlRig>(Instance))
			{
				InstanceRig->ShapeLibraries = ShapeLibraries;
			}
		}
	}
}

UClass* UTLLRN_ControlRigBlueprint::GetTLLRN_ControlRigClass() const
{
	return GetRigVMHostClass();
}

bool UTLLRN_ControlRigBlueprint::IsTLLRN_ModularRig() const
{
	if(const UClass* Class = GetTLLRN_ControlRigClass())
	{
		return Class->IsChildOf(UTLLRN_ModularRig::StaticClass());
	}
	return false;
}

USkeletalMesh* UTLLRN_ControlRigBlueprint::GetPreviewMesh() const
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

#if WITH_EDITORONLY_DATA
	if (!PreviewSkeletalMesh.IsValid())
	{
		(void)PreviewSkeletalMesh.LoadSynchronous();
	}

	return PreviewSkeletalMesh.Get();
#else
	return nullptr;
#endif
}

bool UTLLRN_ControlRigBlueprint::IsTLLRN_ControlTLLRN_RigModule() const
{
	return TLLRN_RigModuleSettings.Identifier.IsValid();
}

#if WITH_EDITORONLY_DATA

bool UTLLRN_ControlRigBlueprint::CanTurnIntoTLLRN_ControlTLLRN_RigModule(bool InAutoConvertHierarchy, FString* OutErrorMessage) const
{
	if(IsTLLRN_ControlTLLRN_RigModule())
	{
		if(OutErrorMessage)
		{
			static const FString Message = TEXT("This asset is already a Control Rig Module.");
			*OutErrorMessage = Message;
		}
		return false;
	}

	if (GetRigVMHostClass()->IsChildOf<UTLLRN_ModularRig>())
	{
		if(OutErrorMessage)
		{
			static const FString Message = TEXT("This asset is a Modular Rig.");
			*OutErrorMessage = Message;
		}
		return false;
	}

	if(Hierarchy == nullptr)
	{
		if(OutErrorMessage)
		{
			static const FString Message = TEXT("This asset contains no hierarchy.");
			*OutErrorMessage = Message;
		}
		return false;
	}

	const TArray<FTLLRN_RigElementKey> Keys = Hierarchy->GetAllKeys(true);
	for(const FTLLRN_RigElementKey& Key : Keys)
	{
		if(!InAutoConvertHierarchy)
		{
			if(Key.Type != ETLLRN_RigElementType::Bone &&
				Key.Type != ETLLRN_RigElementType::Curve &&
				Key.Type != ETLLRN_RigElementType::Connector)
			{
				if(OutErrorMessage)
				{
					static constexpr TCHAR Format[] = TEXT("The hierarchy contains elements other than bones (for example '%s'). Modules only allow imported bones and user authored connectors.");
					*OutErrorMessage = FString::Printf(Format, *Key.ToString());
				}
				return false;
			}

			if(Key.Type == ETLLRN_RigElementType::Bone)
			{
				if(Hierarchy->FindChecked<FTLLRN_RigBoneElement>(Key)->BoneType != ETLLRN_RigBoneType::Imported)
				{
					if(OutErrorMessage)
					{
						static constexpr TCHAR Format[] = TEXT("The hierarchy contains a user defined bone ('%s') - only imported bones are allowed.");
						*OutErrorMessage = FString::Printf(Format, *Key.ToString());
					}
					return false;
				}
			}
		}
	}
	
	return true;
}

bool UTLLRN_ControlRigBlueprint::TurnIntoTLLRN_ControlTLLRN_RigModule(bool InAutoConvertHierarchy, FString* OutErrorMessage)
{
	if(!CanTurnIntoTLLRN_ControlTLLRN_RigModule(InAutoConvertHierarchy, OutErrorMessage))
	{
		return false;
	}

	FScopedTransaction Transaction(LOCTEXT("TurnIntoTLLRN_ControlTLLRN_RigModule", "Turn Rig into Module"));

	Modify();
	TLLRN_RigModuleSettings.Identifier = FTLLRN_RigModuleIdentifier();
	TLLRN_RigModuleSettings.Identifier.Name = GetName();

	if(Hierarchy)
	{
		Hierarchy->Modify();
		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);

		// create a copy of this hierarchy
		UTLLRN_RigHierarchy* CopyOfHierarchy = NewObject<UTLLRN_RigHierarchy>(GetTransientPackage());
		CopyOfHierarchy->CopyHierarchy(Hierarchy);

		// also create a hierarchy based on the preview mesh
		UTLLRN_RigHierarchy* PreviewMeshHierarchy = NewObject<UTLLRN_RigHierarchy>(GetTransientPackage());
		if(PreviewSkeletalMesh)
		{
			PreviewMeshHierarchy->GetController(true)->ImportBones(PreviewSkeletalMesh->GetSkeleton());
		}

		// disable compilation
		{
			FRigVMBlueprintCompileScope CompileScope(this);

			// remove everything from the hierarchy
			Hierarchy->Reset();

			const TArray<FTLLRN_RigElementKey> AllKeys = CopyOfHierarchy->GetAllKeys(true);
			TArray<FTLLRN_RigElementKey> KeysToSpawn;

			for(const FTLLRN_RigElementKey& Key : AllKeys)
			{
				if(Key.Type == ETLLRN_RigElementType::Curve)
				{
					continue;
				}
				if(Key.Type == ETLLRN_RigElementType::Bone)
				{
					if(PreviewMeshHierarchy->Contains(Key))
					{
						continue;
					}
				}
				KeysToSpawn.Add(Key);
			}

			(void)ConvertHierarchyElementsToSpawnerNodes(CopyOfHierarchy, KeysToSpawn, false);

			if(Hierarchy->Num(ETLLRN_RigElementType::Connector) == 0)
			{
				static const FName RootName = TEXT("Root");
				static const FString RootDescription = TEXT("This is the default temporary socket used for the root connection.");
				const FTLLRN_RigElementKey ConnectorKey = Controller->AddConnector(RootName);
				const FTLLRN_RigElementKey SocketKey = Controller->AddSocket(RootName, FTLLRN_RigElementKey(), FTransform::Identity, false, FTLLRN_RigSocketElement::SocketDefaultColor, RootDescription, false);
				(void)ResolveConnector(ConnectorKey, SocketKey);
			}
		}
	}

	OnRigTypeChangedDelegate.Broadcast(this);
	return true;
}

bool UTLLRN_ControlRigBlueprint::CanTurnIntoStandaloneRig(FString* OutErrorMessage) const
{
	return IsTLLRN_ControlTLLRN_RigModule();
}

bool UTLLRN_ControlRigBlueprint::TurnIntoStandaloneRig(FString* OutErrorMessage)
{
	if(!CanTurnIntoStandaloneRig(OutErrorMessage))
	{
		return false;
	}

	FScopedTransaction Transaction(LOCTEXT("TurnIntoStandaloneRig", "Turn Module into Rig"));

	Modify();
	TLLRN_RigModuleSettings = FTLLRN_RigModuleSettings();

	if(Hierarchy)
	{
		Hierarchy->Modify();
		Hierarchy->Reset();
		if(PreviewSkeletalMesh)
		{
			Hierarchy->GetController(true)->ImportBones(PreviewSkeletalMesh->GetSkeleton());
		}
	}

	OnRigTypeChangedDelegate.Broadcast(this);
	return true;
}

TArray<URigVMNode*> UTLLRN_ControlRigBlueprint::ConvertHierarchyElementsToSpawnerNodes(UTLLRN_RigHierarchy* InHierarchy, TArray<FTLLRN_RigElementKey> InKeys, bool bRemoveElements)
{
	TArray<URigVMNode*> SpawnerNodes;

	// find the construction event 
	const URigVMNode* EventNode = nullptr;
	for(const URigVMGraph* Graph : GetRigVMClient()->GetAllModels(false, false))
	{
		for(const URigVMNode* Node : Graph->GetNodes())
		{
			if(Node->IsEvent() && Node->GetEventName() == FTLLRN_RigUnit_PrepareForExecution::EventName)
			{
				EventNode = Node;
				break;
			}
		}
		if(EventNode)
		{
			break;
		}
	}

	FVector2D NodePosition = FVector2D::ZeroVector;
	const FVector2D NodePositionIncrement = FVector2D(400, 0);
	
	// if we didn't find the construction event yet, create it
	if(EventNode == nullptr)
	{
		const URigVMGraph* ConstructionGraph = GetRigVMClient()->AddModel(TEXT("ConstructionGraph"), true);
		URigVMController* GraphController = GetRigVMClient()->GetOrCreateController(ConstructionGraph);
		EventNode = GraphController->AddUnitNode(FTLLRN_RigUnit_PrepareForExecution::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), NodePosition);
		NodePosition += NodePositionIncrement;
	}

	const URigVMPin* LastPin = EventNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_PrepareForExecution, ExecuteContext));
	if(LastPin)
	{
		// follow the node's execution links to find the last one
		bool bCarryOn = true;
		while(bCarryOn)
		{
			static const TArray<FString> ExecutePinPaths = {
				FRigVMStruct::ControlFlowCompletedName.ToString(),
				FRigVMStruct::ExecuteContextName.ToString()
			};

			for(const FString& ExecutePinPath : ExecutePinPaths)
			{
				if(const URigVMPin* ExecutePin = LastPin->GetNode()->FindPin(ExecutePinPath))
				{
					const TArray<URigVMPin*> TargetPins = ExecutePin->GetLinkedTargetPins();
					if(TargetPins.IsEmpty())
					{
						bCarryOn = false;
						break;
					}
					LastPin = TargetPins[0];
					NodePosition = LastPin->GetNode()->GetPosition() + NodePositionIncrement;
				}
			}
		}
	}

	const URigVMGraph* ConstructionGraph = EventNode->GetGraph();
	URigVMController* GraphController = GetRigVMClient()->GetOrCreateController(ConstructionGraph);

	auto GetParentAndTransformDefaults = [InHierarchy](const FTLLRN_RigElementKey& InKey, FString& OutParentDefault, FString& OutTransformDefault)
	{
		const FTLLRN_RigElementKey Parent = InHierarchy->GetFirstParent(InKey);
		OutParentDefault.Reset();
		FTLLRN_RigElementKey::StaticStruct()->ExportText(OutParentDefault, &Parent, nullptr, nullptr, PPF_None, nullptr);

		const FTransform Transform = InHierarchy->GetInitialLocalTransform(InKey);
		OutTransformDefault.Reset();
		TBaseStructure<FTransform>::Get()->ExportText(OutTransformDefault, &Transform, nullptr, nullptr, PPF_None, nullptr);
	};

	TMap<FTLLRN_RigElementKey, const URigVMPin*> ParentItemPinMap;
	auto AddParentItemLink = [GraphController, InHierarchy, &SpawnerNodes, &ParentItemPinMap]
		(const FTLLRN_RigElementKey& Key, URigVMNode* Node)
		{
			SpawnerNodes.Add(Node);
			ParentItemPinMap.Add(Key, Node->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddElement, Item)));

			if(const URigVMPin** SourcePin = ParentItemPinMap.Find(InHierarchy->GetFirstParent(Key)))
			{
				if(const URigVMPin* TargetPin = Node->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddElement, Parent)))
				{
					GraphController->AddLink((*SourcePin)->GetPinPath(), TargetPin->GetPinPath(), true);
				}
			}
		};
	
	for(const FTLLRN_RigElementKey& Key : InKeys)
	{
		if(Key.Type == ETLLRN_RigElementType::Bone)
		{
			FString ParentDefault, TransformDefault;
			GetParentAndTransformDefaults(Key, ParentDefault, TransformDefault);

			URigVMNode* AddBoneNode = GraphController->AddUnitNode(FTLLRN_RigUnit_HierarchyAddBone::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), NodePosition);
			NodePosition += NodePositionIncrement;
			AddParentItemLink(Key, AddBoneNode);

			if(LastPin)
			{
				if(const URigVMPin* NextPin = AddBoneNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddBone, ExecuteContext)))
				{
					GraphController->AddLink(LastPin->GetPinPath(), NextPin->GetPinPath(), true);
					LastPin = NextPin;
				}
			}

			GraphController->SetPinDefaultValue(AddBoneNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddElement, Name))->GetPinPath(), Key.Name.ToString(), true, true);
			GraphController->SetPinDefaultValue(AddBoneNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddElement, Parent))->GetPinPath(), ParentDefault, true, true);
			GraphController->SetPinDefaultValue(AddBoneNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddBone, Space))->GetPinPath(), TEXT("LocalSpace"), true, true);
			GraphController->SetPinDefaultValue(AddBoneNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddBone, Transform))->GetPinPath(), TransformDefault, true, true);
		}
		else if(Key.Type == ETLLRN_RigElementType::Null)
		{
			FString ParentDefault, TransformDefault;
			GetParentAndTransformDefaults(Key, ParentDefault, TransformDefault);

			URigVMNode* AddNullNode = GraphController->AddUnitNode(FTLLRN_RigUnit_HierarchyAddNull::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), NodePosition);
			NodePosition += NodePositionIncrement;
			AddParentItemLink(Key, AddNullNode);
			SpawnerNodes.Add(AddNullNode);

			if(LastPin)
			{
				if(const URigVMPin* NextPin = AddNullNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddBone, ExecuteContext)))
				{
					GraphController->AddLink(LastPin->GetPinPath(), NextPin->GetPinPath(), true);
					LastPin = NextPin;
				}
			}

			GraphController->SetPinDefaultValue(AddNullNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddElement, Name))->GetPinPath(), Key.Name.ToString(), true, true);
			GraphController->SetPinDefaultValue(AddNullNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddElement, Parent))->GetPinPath(), ParentDefault, true, true);
			GraphController->SetPinDefaultValue(AddNullNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddNull, Space))->GetPinPath(), TEXT("LocalSpace"), true, true);
			GraphController->SetPinDefaultValue(AddNullNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddNull, Transform))->GetPinPath(), TransformDefault, true, true);
		}
		else if(Key.Type == ETLLRN_RigElementType::Control)
		{
			FTLLRN_RigControlElement* ControlElement = InHierarchy->FindChecked<FTLLRN_RigControlElement>(Key);
			
			FString ParentDefault, TransformDefault;
			GetParentAndTransformDefaults(Key, ParentDefault, TransformDefault);

			const FTransform OffsetTransform = InHierarchy->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::InitialLocal);
			FString OffsetDefault;
			TBaseStructure<FTransform>::Get()->ExportText(OffsetDefault, &OffsetTransform, nullptr, nullptr, PPF_None, nullptr);
			
			if(ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::AnimationChannel)
			{
				UScriptStruct* UnitNodeStruct = nullptr;
				TRigVMTypeIndex TypeIndex = INDEX_NONE;
				FString InitialValue, MinimumValue, MaximumValue, SettingsValue;
				switch(ControlElement->Settings.ControlType)
				{
					case ETLLRN_RigControlType::Bool:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddAnimationChannelBool::StaticStruct();
						TypeIndex = RigVMTypeUtils::TypeIndex::Bool;
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<float>();
						MinimumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Minimum).ToString<float>();
						MaximumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Maximum).ToString<float>();
						break;
					}
					case ETLLRN_RigControlType::Float:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat::StaticStruct();
						TypeIndex = RigVMTypeUtils::TypeIndex::Float;
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<float>();
						MinimumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Minimum).ToString<float>();
						MaximumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Maximum).ToString<float>();

						if(ControlElement->Settings.LimitEnabled.Num() == 1)
						{
							FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings Settings;
							Settings.Enabled = ControlElement->Settings.LimitEnabled[0];
							FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings::StaticStruct()->ExportText(SettingsValue, &Settings, &Settings, nullptr, PPF_None, nullptr);
						}
						break;
					}
					case ETLLRN_RigControlType::ScaleFloat:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddAnimationChannelScaleFloat::StaticStruct();
						TypeIndex = RigVMTypeUtils::TypeIndex::Float;
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<float>();
						MinimumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Minimum).ToString<float>();
						MaximumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Maximum).ToString<float>();

						if(ControlElement->Settings.LimitEnabled.Num() == 1)
						{
							FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings Settings;
							Settings.Enabled = ControlElement->Settings.LimitEnabled[0];
							FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings::StaticStruct()->ExportText(SettingsValue, &Settings, &Settings, nullptr, PPF_None, nullptr);
						}
						break;
					}
					case ETLLRN_RigControlType::Integer:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddAnimationChannelInteger::StaticStruct();
						TypeIndex = RigVMTypeUtils::TypeIndex::Int32; 
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<int32>();
						MinimumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Minimum).ToString<int32>();
						MaximumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Maximum).ToString<int32>();

						if(ControlElement->Settings.LimitEnabled.Num() == 1)
						{
							FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings Settings;
							Settings.Enabled = ControlElement->Settings.LimitEnabled[0];
							FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings::StaticStruct()->ExportText(SettingsValue, &Settings, &Settings, nullptr, PPF_None, nullptr);
						}
						break;
					}
					case ETLLRN_RigControlType::Vector2D:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddAnimationChannelVector2D::StaticStruct();
						TypeIndex = FRigVMRegistry::Get().GetTypeIndex<FVector2D>(); 
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<FVector2D>();
						MinimumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Minimum).ToString<FVector2D>();
						MaximumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Maximum).ToString<FVector2D>();

						if(ControlElement->Settings.LimitEnabled.Num() == 2)
						{
							FTLLRN_RigUnit_HierarchyAddAnimationChannel2DLimitSettings Settings;
							Settings.X = ControlElement->Settings.LimitEnabled[0];
							Settings.Y = ControlElement->Settings.LimitEnabled[1];
							FTLLRN_RigUnit_HierarchyAddAnimationChannel2DLimitSettings::StaticStruct()->ExportText(SettingsValue, &Settings, &Settings, nullptr, PPF_None, nullptr);
						}
						break;
					}
					case ETLLRN_RigControlType::Position:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddAnimationChannelVector::StaticStruct();
						TypeIndex = FRigVMRegistry::Get().GetTypeIndex<FVector>(); 
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<FVector>();
						MinimumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Minimum).ToString<FVector>();
						MaximumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Maximum).ToString<FVector>();

						if(ControlElement->Settings.LimitEnabled.Num() == 3)
						{
							FTLLRN_RigUnit_HierarchyAddAnimationChannelVectorLimitSettings Settings;
							Settings.X = ControlElement->Settings.LimitEnabled[0];
							Settings.Y = ControlElement->Settings.LimitEnabled[1];
							Settings.Z = ControlElement->Settings.LimitEnabled[2];
							FTLLRN_RigUnit_HierarchyAddAnimationChannelVectorLimitSettings::StaticStruct()->ExportText(SettingsValue, &Settings, &Settings, nullptr, PPF_None, nullptr);
						}
						break;
					}
					case ETLLRN_RigControlType::Scale:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddAnimationChannelScaleVector::StaticStruct();
						TypeIndex = FRigVMRegistry::Get().GetTypeIndex<FVector>(); 
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<FVector>();
						MinimumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Minimum).ToString<FVector>();
						MaximumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Maximum).ToString<FVector>();

						if(ControlElement->Settings.LimitEnabled.Num() == 3)
						{
							FTLLRN_RigUnit_HierarchyAddAnimationChannelVectorLimitSettings Settings;
							Settings.X = ControlElement->Settings.LimitEnabled[0];
							Settings.Y = ControlElement->Settings.LimitEnabled[1];
							Settings.Z = ControlElement->Settings.LimitEnabled[2];
							FTLLRN_RigUnit_HierarchyAddAnimationChannelVectorLimitSettings::StaticStruct()->ExportText(SettingsValue, &Settings, &Settings, nullptr, PPF_None, nullptr);
						}
						break;
					}
					case ETLLRN_RigControlType::Rotator:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddAnimationChannelRotator::StaticStruct();
						TypeIndex = FRigVMRegistry::Get().GetTypeIndex<FRotator>(); 
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<FRotator>();
						MinimumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Minimum).ToString<FRotator>();
						MaximumValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Maximum).ToString<FRotator>();

						if(ControlElement->Settings.LimitEnabled.Num() == 3)
						{
							FTLLRN_RigUnit_HierarchyAddAnimationChannelRotatorLimitSettings Settings;
							Settings.Pitch = ControlElement->Settings.LimitEnabled[0];
							Settings.Yaw = ControlElement->Settings.LimitEnabled[1];
							Settings.Roll = ControlElement->Settings.LimitEnabled[2];
							FTLLRN_RigUnit_HierarchyAddAnimationChannelRotatorLimitSettings::StaticStruct()->ExportText(SettingsValue, &Settings, &Settings, nullptr, PPF_None, nullptr);
						}
						break;
					}
					default:
					{
						break;
					}
				}

				if(UnitNodeStruct == nullptr)
				{
					continue;
				}

				URigVMNode* AddControlNode = GraphController->AddUnitNode(UnitNodeStruct, FTLLRN_RigUnit::GetMethodName(), NodePosition);
				NodePosition += NodePositionIncrement;
				AddParentItemLink(Key, AddControlNode);

				if(LastPin)
				{
					if(const URigVMPin* NextPin = AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddBone, ExecuteContext)))
					{
						GraphController->AddLink(LastPin->GetPinPath(), NextPin->GetPinPath(), true);
						LastPin = NextPin;
					}
				}

				GraphController->ResolveWildCardPin(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat, InitialValue))->GetPinPath(), TypeIndex, true);
				GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat, Name))->GetPinPath(), Key.Name.ToString(), true, true);
				GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat, Parent))->GetPinPath(), ParentDefault, true, true);
				GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat, InitialValue))->GetPinPath(), InitialValue, true, true);
				GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat, MinimumValue))->GetPinPath(), MinimumValue, true, true);
				GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat, MaximumValue))->GetPinPath(), MaximumValue, true, true);

				if(!SettingsValue.IsEmpty())
				{
					GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat, LimitsEnabled))->GetPinPath(), SettingsValue, true, true);
				}
			}
			else
			{
				UScriptStruct* UnitNodeStruct = nullptr;
				TRigVMTypeIndex TypeIndex = INDEX_NONE;
				FString InitialValue;
				switch(ControlElement->Settings.ControlType)
				{
					case ETLLRN_RigControlType::Float:
					case ETLLRN_RigControlType::ScaleFloat:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddControlFloat::StaticStruct();
						TypeIndex = RigVMTypeUtils::TypeIndex::Float;
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<float>();
						break;
					}
					case ETLLRN_RigControlType::Integer:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddControlInteger::StaticStruct();
						TypeIndex = RigVMTypeUtils::TypeIndex::Int32; 
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<int32>();
						break;
					}
					case ETLLRN_RigControlType::Vector2D:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddControlVector2D::StaticStruct();
						TypeIndex = FRigVMRegistry::Get().GetTypeIndex<FVector2D>(); 
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<FVector2D>();
						break;
					}
					case ETLLRN_RigControlType::Position:
					case ETLLRN_RigControlType::Scale:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddControlVector::StaticStruct();
						TypeIndex = FRigVMRegistry::Get().GetTypeIndex<FVector>(); 
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<FVector>();
						break;
					}
					case ETLLRN_RigControlType::Rotator:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddControlRotator::StaticStruct();
						TypeIndex = FRigVMRegistry::Get().GetTypeIndex<FRotator>(); 
						InitialValue = InHierarchy->GetControlValue(Key, ETLLRN_RigControlValueType::Initial).ToString<FRotator>();
						break;
					}
					case ETLLRN_RigControlType::Transform:
					case ETLLRN_RigControlType::TransformNoScale:
					case ETLLRN_RigControlType::EulerTransform:
					{
						UnitNodeStruct = FTLLRN_RigUnit_HierarchyAddControlTransform::StaticStruct();
						TypeIndex = FRigVMRegistry::Get().GetTypeIndex<FTransform>(); 
						const FTransform InitialTransform = InHierarchy->GetInitialLocalTransform(Key);
						TBaseStructure<FTransform>::Get()->ExportText(InitialValue, &InitialTransform, nullptr, nullptr, PPF_None, nullptr);
						break;
					}
					default:
					{
						break;
					}
				}

				if(UnitNodeStruct == nullptr)
				{
					continue;
				}

				URigVMNode* AddControlNode = GraphController->AddUnitNode(UnitNodeStruct, FTLLRN_RigUnit::GetMethodName(), NodePosition);
				NodePosition += NodePositionIncrement;
				AddParentItemLink(Key, AddControlNode);

				if(LastPin)
				{
					if(const URigVMPin* NextPin = AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddBone, ExecuteContext)))
					{
						GraphController->AddLink(LastPin->GetPinPath(), NextPin->GetPinPath(), true);
						LastPin = NextPin;
					}
				}

				GraphController->ResolveWildCardPin(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddControlInteger, InitialValue))->GetPinPath(), TypeIndex, true);
				GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddElement, Name))->GetPinPath(), Key.Name.ToString(), true, true);
				GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddElement, Parent))->GetPinPath(), ParentDefault, true, true);
				GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddControlElement, OffsetSpace))->GetPinPath(), TEXT("LocalSpace"), true, true);
				GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddControlElement, OffsetTransform))->GetPinPath(), OffsetDefault, true, true);
				GraphController->SetPinDefaultValue(AddControlNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddControlInteger, InitialValue))->GetPinPath(), InitialValue, true, true);

				if(const FStructProperty* SettingsProperty = CastField<FStructProperty>(UnitNodeStruct->FindPropertyByName(TEXT("Settings"))))
				{
					UScriptStruct* SettingsStruct = CastChecked<UScriptStruct>(SettingsProperty->Struct);
					FStructOnScope SettingsScope(SettingsStruct);
					FTLLRN_RigUnit_HierarchyAddControl_Settings* Settings = (FTLLRN_RigUnit_HierarchyAddControl_Settings*)SettingsScope.GetStructMemory();
					Settings->ConfigureFrom(ControlElement, ControlElement->Settings);
					FString SettingsDefault;
					SettingsStruct->ExportText(SettingsDefault, Settings, nullptr, nullptr, PPF_None, nullptr);

					GraphController->SetPinDefaultValue(AddControlNode->FindPin(SettingsProperty->GetName())->GetPinPath(), SettingsDefault, true, true);
				}
			}
		}
		else if(Key.Type == ETLLRN_RigElementType::Socket)
		{
			FString ParentDefault, TransformDefault;
			GetParentAndTransformDefaults(Key, ParentDefault, TransformDefault);

			URigVMNode* AddSocketNode = GraphController->AddUnitNode(FTLLRN_RigUnit_HierarchyAddSocket::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), NodePosition);
			NodePosition += NodePositionIncrement;
			AddParentItemLink(Key, AddSocketNode);
			SpawnerNodes.Add(AddSocketNode);

			if(LastPin)
			{
				if(const URigVMPin* NextPin = AddSocketNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddBone, ExecuteContext)))
				{
					GraphController->AddLink(LastPin->GetPinPath(), NextPin->GetPinPath(), true);
					LastPin = NextPin;
				}
			}

			GraphController->SetPinDefaultValue(AddSocketNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddElement, Name))->GetPinPath(), Key.Name.ToString(), true, true);
			GraphController->SetPinDefaultValue(AddSocketNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddElement, Parent))->GetPinPath(), ParentDefault, true, true);
			GraphController->SetPinDefaultValue(AddSocketNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddNull, Space))->GetPinPath(), TEXT("LocalSpace"), true, true);
			GraphController->SetPinDefaultValue(AddSocketNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HierarchyAddNull, Transform))->GetPinPath(), TransformDefault, true, true);
		}
	}

	if(bRemoveElements && InHierarchy)
	{
		InHierarchy->Modify();
		for(const FTLLRN_RigElementKey& Key : InKeys)
		{
			InHierarchy->GetController(true)->RemoveElement(Key, true);
		}
	}

	return SpawnerNodes;
}

#endif // WITH_EDITORONLY_DATA

UTexture2D* UTLLRN_ControlRigBlueprint::GetTLLRN_RigModuleIcon() const
{
	if(IsTLLRN_ControlTLLRN_RigModule())
	{
		if(UTexture2D* Icon = Cast<UTexture2D>(TLLRN_RigModuleSettings.Icon.TryLoad()))
		{
			return Icon;
		}
	}
	return nullptr;
}

void UTLLRN_ControlRigBlueprint::SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty/*=true*/)
{
#if WITH_EDITORONLY_DATA
	if(bMarkAsDirty)
	{
		Modify();
	}

	PreviewSkeletalMesh = PreviewMesh;
#endif
}

void UTLLRN_ControlRigBlueprint::Serialize(FArchive& Ar)
{
	if(IsValid(this))
	{
		RigVMClient.SetOuterClientHost(this, GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, RigVMClient));
		TLLRN_ModularRigModel.SetOuterClientHost(this);
	}
	
	Super::Serialize(Ar);

	if(Ar.IsObjectReferenceCollector())
	{
		Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);


#if WITH_EDITORONLY_DATA
		if (Ar.IsCooking() && ReferencedObjectPathsStored)
		{
			for (FSoftObjectPath ObjectPath : ReferencedObjectPaths)
			{
				ObjectPath.Serialize(Ar);
			}
		}
		else
#endif
		{
			TArray<IRigVMGraphFunctionHost*> ReferencedFunctionHosts = GetReferencedFunctionHosts(false);

			for(IRigVMGraphFunctionHost* ReferencedFunctionHost : ReferencedFunctionHosts)
			{
				if (URigVMBlueprintGeneratedClass* BPGeneratedClass = Cast<URigVMBlueprintGeneratedClass>(ReferencedFunctionHost))
				{
					Ar << BPGeneratedClass;
				}
			}

			for(const TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>& ShapeLibraryPtr : ShapeLibraries)
			{
				if(ShapeLibraryPtr.IsValid())
				{
					UTLLRN_ControlRigShapeLibrary* ShapeLibrary = ShapeLibraryPtr.Get();
					Ar << ShapeLibrary;
				}
			}
		}
	}

	if(Ar.IsLoading())
	{
		if(Model_DEPRECATED || FunctionLibrary_DEPRECATED)
		{
			TGuardValue<bool> DisableClientNotifs(RigVMClient.bSuspendNotifications, true);
			RigVMClient.SetFromDeprecatedData(Model_DEPRECATED, FunctionLibrary_DEPRECATED);
		}

		TLLRN_ModularRigModel.UpdateCachedChildren();
		TLLRN_ModularRigModel.Connections.UpdateFromConnectionList();
	}
}

void UTLLRN_ControlRigBlueprint::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);

	// make sure to save the VM with high performance settings
	// so that during cooking we reach small footprints.
	// these settings may have changed during the user session.
	VMCompileSettings.ASTSettings.bFoldAssignments = true;
	VMCompileSettings.ASTSettings.bFoldLiterals = true;

	bExposesAnimatableControls = false;
	Hierarchy->ForEach<FTLLRN_RigControlElement>([this](FTLLRN_RigControlElement* ControlElement) -> bool
    {
		if (Hierarchy->IsAnimatable(ControlElement))
		{
			bExposesAnimatableControls = true;
			return false;
		}
		return true;
	});

	if(IsTLLRN_ControlTLLRN_RigModule())
	{
		UTLLRN_RigHierarchy* DebuggedHierarchy = Hierarchy;
		if(UTLLRN_ControlRig* DebuggedRig = Cast<UTLLRN_ControlRig>(GetObjectBeingDebugged()))
		{
			DebuggedHierarchy = DebuggedRig->GetHierarchy();
		}

		TGuardValue<bool> SuspendNotifGuard(Hierarchy->GetSuspendNotificationsFlag(), true);
		TGuardValue<bool> SuspendNotifGuardOnDebuggedHierarchy(DebuggedHierarchy->GetSuspendNotificationsFlag(), true);

		UpdateExposedModuleConnectors();
	}

	if (IsTLLRN_ControlTLLRN_RigModule())
	{
		TLLRN_ControlRigType = ETLLRN_ControlRigType::TLLRN_RigModule;
		ItemTypeDisplayName = TEXT("Rig Module");
		CustomThumbnail = TLLRN_RigModuleSettings.Icon.ToString();
	}
	else if (GetTLLRN_ControlRigClass()->IsChildOf(UTLLRN_ModularRig::StaticClass()))
	{
		TLLRN_ControlRigType = ETLLRN_ControlRigType::TLLRN_ModularRig;
		ItemTypeDisplayName = TEXT("Modular Rig");
	}
	else
	{
		TLLRN_ControlRigType = ETLLRN_ControlRigType::IndependentRig;
		ItemTypeDisplayName = TEXT("TLLRN Control Rig");
	}

	if (IsTLLRN_ModularRig())
	{
		TLLRN_ModuleReferenceData = GetTLLRN_ModuleReferenceData();
		IAssetRegistry::GetChecked().AssetTagsFinalized(*this);
	}
}

TArray<FTLLRN_ModuleReferenceData> UTLLRN_ControlRigBlueprint::FindReferencesToModule() const
{
	TArray<FTLLRN_ModuleReferenceData> Result;
	if (!IsTLLRN_ControlTLLRN_RigModule())
	{
		return Result;
	}

	const UClass* TLLRN_RigModuleClass = GetTLLRN_ControlRigClass();
	if (!TLLRN_RigModuleClass)
	{
		return Result;
	}

	// Load the asset registry module
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Collect a full list of assets with the control rig class
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByClass(UTLLRN_ControlRigBlueprint::StaticClass()->GetClassPathName(), AssetDataList, true);

	static const FLazyName TLLRN_ModuleReferenceDataName(GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, TLLRN_ModuleReferenceData));
	FArrayProperty* TLLRN_ModuleReferenceDataProperty = CastField<FArrayProperty>(UTLLRN_ControlRigBlueprint::StaticClass()->FindPropertyByName(TLLRN_ModuleReferenceDataName));

	for(const FAssetData& AssetData : AssetDataList)
	{
		// Check only modular rigs
		if (UTLLRN_ControlRigBlueprint::GetRigType(AssetData) != ETLLRN_ControlRigType::TLLRN_ModularRig)
		{
			continue;
		}
		
		const FString TLLRN_ModularRigDataString = AssetData.GetTagValueRef<FString>(TLLRN_ModuleReferenceDataName);
		if (TLLRN_ModularRigDataString.IsEmpty())
		{
			continue;
		}

		TArray<FTLLRN_ModuleReferenceData> Modules;
		TLLRN_ModuleReferenceDataProperty->ImportText_Direct(*TLLRN_ModularRigDataString, &Modules, nullptr, EPropertyPortFlags::PPF_None);

		for (FTLLRN_ModuleReferenceData& Module : Modules)
		{
			if (Module.ReferencedModule == TLLRN_RigModuleClass)
			{
				Result.Add(Module);
			}
		}
	}

	return Result;
}

ETLLRN_ControlRigType UTLLRN_ControlRigBlueprint::GetRigType(const FAssetData& InAsset)
{
	ETLLRN_ControlRigType Result = ETLLRN_ControlRigType::MAX;
	static const FLazyName TLLRN_ControlRigTypeName(GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, TLLRN_ControlRigType));
	FProperty* TLLRN_ControlRigTypeProperty = CastField<FProperty>(UTLLRN_ControlRigBlueprint::StaticClass()->FindPropertyByName(TLLRN_ControlRigTypeName));
	const FString TLLRN_ControlRigTypeString = InAsset.GetTagValueRef<FString>(TLLRN_ControlRigTypeName);
	if (TLLRN_ControlRigTypeString.IsEmpty())
	{
		return Result;
	}

	ETLLRN_ControlRigType RigType;
	TLLRN_ControlRigTypeProperty->ImportText_Direct(*TLLRN_ControlRigTypeString, &RigType, nullptr, EPropertyPortFlags::PPF_None);
	return RigType;
}

TArray<FSoftObjectPath> UTLLRN_ControlRigBlueprint::GetReferencesToTLLRN_RigModule(const FAssetData& InModuleAsset)
{
	TArray<FSoftObjectPath> Result;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.GetRegistry();
	
	TArray<FName> PackageDependencies;
	AssetRegistry.GetReferencers(InModuleAsset.PackageName, PackageDependencies);

	for (FName& DependencyPath : PackageDependencies)
	{
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPackageName(DependencyPath, Assets);

		for (const FAssetData& DependencyData : Assets)
		{
			if (DependencyData.IsAssetLoaded())
			{
				if (UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(DependencyData.GetAsset()))
				{
					if (Blueprint->IsTLLRN_ModularRig())
					{
						TArray<const FTLLRN_RigModuleReference*> Modules = Blueprint->TLLRN_ModularRigModel.FindModuleInstancesOfClass(InModuleAsset);
						for (const FTLLRN_RigModuleReference* Module : Modules)
						{
							FSoftObjectPath ModulePath = DependencyData.GetSoftObjectPath();
							ModulePath.SetSubPathString(Module->GetPath());
							Result.Add(ModulePath);
						}
					}
				}
			}
			else
			{
				// Check only modular rigs
				if (UTLLRN_ControlRigBlueprint::GetRigType(DependencyData) != ETLLRN_ControlRigType::TLLRN_ModularRig)
				{
					continue;
				}

				static const FLazyName TLLRN_ModuleReferenceDataName(GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, TLLRN_ModuleReferenceData));
				FArrayProperty* TLLRN_ModuleReferenceDataProperty = CastField<FArrayProperty>(UTLLRN_ControlRigBlueprint::StaticClass()->FindPropertyByName(TLLRN_ModuleReferenceDataName));
				const FString TLLRN_ModularRigDataString = DependencyData.GetTagValueRef<FString>(TLLRN_ModuleReferenceDataName);
				if (TLLRN_ModularRigDataString.IsEmpty())
				{
					continue;
				}

				TArray<FTLLRN_ModuleReferenceData> Modules;
				TLLRN_ModuleReferenceDataProperty->ImportText_Direct(*TLLRN_ModularRigDataString, &Modules, nullptr, EPropertyPortFlags::PPF_None);

				for (const FTLLRN_ModuleReferenceData& Module : Modules)
				{
					FTopLevelAssetPath ModulePath = Module.ReferencedModule.GetAssetPath();
					FString AssetName = ModulePath.GetAssetName().ToString();
					AssetName.RemoveFromEnd(TEXT("_C"));
					ModulePath = FTopLevelAssetPath(ModulePath.GetPackageName(), *AssetName);
					if (ModulePath == InModuleAsset.GetSoftObjectPath().GetAssetPath())
					{
						FSoftObjectPath ResultModulePath = DependencyData.GetSoftObjectPath();
						ResultModulePath.SetSubPathString(Module.ModulePath);
						Result.Add(ResultModulePath);
					}
				}
			}
		}
	}
	
	return Result;
}

TArray<FTLLRN_ModuleReferenceData> UTLLRN_ControlRigBlueprint::GetTLLRN_ModuleReferenceData() const
{
	TArray<FTLLRN_ModuleReferenceData> Result;
	Result.Reserve(TLLRN_ModularRigModel.Modules.Num());
	TLLRN_ModularRigModel.ForEachModule([&Result](const FTLLRN_RigModuleReference* Module) -> bool
	{
		Result.Add(Module);
		return true;
	});
	return Result;
}

void UTLLRN_ControlRigBlueprint::UpdateExposedModuleConnectors() const
{
	UTLLRN_ControlRigBlueprint* MutableThis = ((UTLLRN_ControlRigBlueprint*)this);
	MutableThis->TLLRN_RigModuleSettings.ExposedConnectors.Reset();
	Hierarchy->ForEach<FTLLRN_RigConnectorElement>([MutableThis](const FTLLRN_RigConnectorElement* ConnectorElement) -> bool
	{
		FTLLRN_RigModuleConnector ExposedConnector;
		ExposedConnector.Name = ConnectorElement->GetName();
		ExposedConnector.Settings = ConnectorElement->Settings;
		MutableThis->TLLRN_RigModuleSettings.ExposedConnectors.Add(ExposedConnector);
		return true;
	});
	PropagateHierarchyFromBPToInstances();
}

bool UTLLRN_ControlRigBlueprint::ResolveConnector(const FTLLRN_RigElementKey& DraggedKey, const FTLLRN_RigElementKey& TargetKey, bool bSetupUndoRedo)
{
	FScopedTransaction Transaction(LOCTEXT("ResolveConnector", "Resolve connector"));

	if(bSetupUndoRedo)
	{
		Modify();
	}
	if(TargetKey.IsValid())
	{
		FTLLRN_RigElementKey& ExistingTargetKey = ConnectionMap.FindOrAdd(DraggedKey);
		if(ExistingTargetKey == TargetKey)
		{
			return false;
		}
		ExistingTargetKey = TargetKey;

		if (IsTLLRN_ModularRig())
		{
			// Add connection to the model
			if (UTLLRN_ModularTLLRN_RigController* Controller = GetTLLRN_ModularTLLRN_RigController())
			{
				Controller->ConnectConnectorToElement(DraggedKey, TargetKey, bSetupUndoRedo, TLLRN_ModularRigSettings.bAutoResolve);
			}
		}
		else
		{
			ConnectionMap.FindOrAdd(DraggedKey) = TargetKey;
		}
	}
	else
	{
		if (IsTLLRN_ModularRig())
		{
			// Add connection to the model
			if (UTLLRN_ModularTLLRN_RigController* Controller = GetTLLRN_ModularTLLRN_RigController())
			{
				Controller->DisconnectConnector(DraggedKey, false, bSetupUndoRedo);
			}
		}
		else
		{
			ConnectionMap.Remove(DraggedKey);
		}
	}

	RecompileTLLRN_ModularRig();

	PropagateHierarchyFromBPToInstances();

	if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetObjectBeingDebugged()))
	{
		for (UEdGraph* Graph : UbergraphPages)
		{
			UTLLRN_ControlRigGraph* RigGraph = Cast<UTLLRN_ControlRigGraph>(Graph);
			if (RigGraph == nullptr)
			{
				continue;
			}
			RigGraph->CacheNameLists(TLLRN_ControlRig->GetHierarchy(), &DrawContainer, ShapeLibraries);
		}
	}

	return true;
}

void UTLLRN_ControlRigBlueprint::UpdateConnectionMapFromModel()
{
	if (IsTLLRN_ModularRig())
	{
		ConnectionMap.Reset();

		for (const FTLLRN_ModularRigSingleConnection& Connection : TLLRN_ModularRigModel.Connections)
		{
			ConnectionMap.Add(Connection.Connector, Connection.Target);
		}
	}
}

void UTLLRN_ControlRigBlueprint::PostLoad()
{
	Super::PostLoad();

	{
#if WITH_EDITOR
		
		// correct the offset transforms
		if (GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::ControlOffsetTransform)
		{
			HierarchyContainer_DEPRECATED.ControlHierarchy.PostLoad();
			if (HierarchyContainer_DEPRECATED.ControlHierarchy.Num() > 0)
			{
				MarkDirtyDuringLoad();
			}

			for (FTLLRN_RigControl& Control : HierarchyContainer_DEPRECATED.ControlHierarchy)
			{
				const FTransform PreviousOffsetTransform = Control.GetTransformFromValue(ETLLRN_RigControlValueType::Initial);
				Control.OffsetTransform = PreviousOffsetTransform;
				Control.InitialValue = Control.Value;

				if (Control.ControlType == ETLLRN_RigControlType::Transform)
				{
					Control.InitialValue = FTLLRN_RigControlValue::Make<FTransform>(FTransform::Identity);
				}
				else if (Control.ControlType == ETLLRN_RigControlType::TransformNoScale)
				{
					Control.InitialValue = FTLLRN_RigControlValue::Make<FTransformNoScale>(FTransformNoScale::Identity);
				}
				else if (Control.ControlType == ETLLRN_RigControlType::EulerTransform)
				{
					Control.InitialValue = FTLLRN_RigControlValue::Make<FEulerTransform>(FEulerTransform::Identity);
				}
			}
		}

		// convert the hierarchy from V1 to V2
		if (GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::TLLRN_RigHierarchyV2)
		{
			Modify();
			
			TGuardValue<bool> SuspendNotifGuard(Hierarchy->GetSuspendNotificationsFlag(), true);
			
			Hierarchy->Reset();
			GetHierarchyController()->ImportFromHierarchyContainer(HierarchyContainer_DEPRECATED, false);
		}

		// perform backwards compat value upgrades
		TArray<URigVMGraph*> GraphsToValidate = GetAllModels();
		for (int32 GraphIndex = 0; GraphIndex < GraphsToValidate.Num(); GraphIndex++)
		{
			URigVMGraph* GraphToValidate = GraphsToValidate[GraphIndex];
			if(GraphToValidate == nullptr)
			{
				continue;
			}

			for(URigVMNode* Node : GraphToValidate->GetNodes())
			{
				TArray<URigVMPin*> Pins = Node->GetAllPinsRecursively();
				for(URigVMPin* Pin : Pins)
				{
					if(Pin->GetCPPTypeObject() == StaticEnum<ETLLRN_RigElementType>())
					{
						if(Pin->GetDefaultValue() == TEXT("Space"))
						{
							if(URigVMController* Controller = GetController(GraphToValidate))
							{
								FRigVMControllerNotifGuard NotifGuard(Controller, true);
								FRigVMDefaultValueTypeGuard _(Controller, ERigVMPinDefaultValueType::Override);
								Controller->SetPinDefaultValue(Pin->GetPinPath(), TEXT("Null"), false, false, false);
							}
						}
					}
				}
			}
		}

#endif
	}

	// upgrade the gizmo libraries to shape libraries
	if(!GizmoLibrary_DEPRECATED.IsNull() || GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::RenameGizmoToShape)
	{
		// if it's an older file and it doesn't have the GizmoLibrary stored,
		// refer to the previous default.
		ShapeLibraries.Reset();

		if(!GizmoLibrary_DEPRECATED.IsNull())
		{
			ShapeLibrariesToLoadOnPackageLoaded.Add(GizmoLibrary_DEPRECATED.ToString());
		}
		else
		{
			static const FString DefaultGizmoLibraryPath = TEXT("/TLLRN_ControlRig/Controls/DefaultGizmoLibrary.DefaultGizmoLibrary");
			ShapeLibrariesToLoadOnPackageLoaded.Add(DefaultGizmoLibraryPath);
		}

		URigVMBlueprintGeneratedClass* RigClass = GetRigVMBlueprintGeneratedClass();
		UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(false /* create if needed */));

		TArray<UObject*> ArchetypeInstances;
		CDO->GetArchetypeInstances(ArchetypeInstances);
		ArchetypeInstances.Insert(CDO, 0);

		for (UObject* Instance : ArchetypeInstances)
		{
			if (UTLLRN_ControlRig* InstanceRig = Cast<UTLLRN_ControlRig>(Instance))
			{
				InstanceRig->ShapeLibraries.Reset();
				InstanceRig->GizmoLibrary_DEPRECATED.Reset();
			}
		}
	}

#if WITH_EDITOR
	if(IsTLLRN_ControlTLLRN_RigModule() && Hierarchy)
	{
		// backwards compat - makes sure to only ever allow one primary connector
		TArray<FTLLRN_RigConnectorElement*> Connectors = Hierarchy->GetConnectors();
		const int32 NumPrimaryConnectors = Algo::CountIf(Connectors, [](const FTLLRN_RigConnectorElement* InConnector) -> bool
		{
			return InConnector->IsPrimary();
		});
		if(NumPrimaryConnectors > 1)
		{
			bool bHasSeenPrimary = false;
			for(FTLLRN_RigConnectorElement* Connector : Connectors)
			{
				if(bHasSeenPrimary)
				{
					Connector->Settings.Type = ETLLRN_ConnectorType::Secondary;
				}
				else
				{
					bHasSeenPrimary = Connector->IsPrimary();
				}
			}
			UpdateExposedModuleConnectors();
		}
	}
#endif

	TLLRN_ModularRigModel.PatchModelsOnLoad();
	UpdateModularDependencyDelegates();
}

#if WITH_EDITOR

void UTLLRN_ControlRigBlueprint::HandlePackageDone()
{
	if (ShapeLibrariesToLoadOnPackageLoaded.Num() > 0)
	{
		for(const FString& ShapeLibraryToLoadOnPackageLoaded : ShapeLibrariesToLoadOnPackageLoaded)
		{
			ShapeLibraries.Add(LoadObject<UTLLRN_ControlRigShapeLibrary>(nullptr, *ShapeLibraryToLoadOnPackageLoaded));
		}

		URigVMBlueprintGeneratedClass* RigClass = GetRigVMBlueprintGeneratedClass();
		UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(false /* create if needed */));

		TArray<UObject*> ArchetypeInstances;
		CDO->GetArchetypeInstances(ArchetypeInstances);
		ArchetypeInstances.Insert(CDO, 0);

		for (UObject* Instance : ArchetypeInstances)
		{
			if (UTLLRN_ControlRig* InstanceRig = Cast<UTLLRN_ControlRig>(Instance))
			{
				InstanceRig->ShapeLibraries = ShapeLibraries;
			}
		}

		ShapeLibrariesToLoadOnPackageLoaded.Reset();
	}

	PropagateHierarchyFromBPToInstances();

	Super::HandlePackageDone();
	
	if(IsTLLRN_ModularRig())
	{
		// force load all dependencies
		TLLRN_ModularRigModel.ForEachModule([](const FTLLRN_RigModuleReference* Element) -> bool
		{
			(void)Element->Class.LoadSynchronous();
			return true;
		});

		RecompileTLLRN_ModularRig();
	}
}

void UTLLRN_ControlRigBlueprint::HandleConfigureRigVMController(const FRigVMClient* InClient, URigVMController* InControllerToConfigure)
{
	Super::HandleConfigureRigVMController(InClient, InControllerToConfigure);

	TWeakObjectPtr<URigVMBlueprint> WeakThis(this);
	InControllerToConfigure->ConfigureWorkflowOptionsDelegate.BindLambda([WeakThis](URigVMUserWorkflowOptions* Options)
	{
		if(UTLLRN_ControlRigWorkflowOptions* TLLRN_ControlRigNodeWorkflowOptions = Cast<UTLLRN_ControlRigWorkflowOptions>(Options))
		{
			TLLRN_ControlRigNodeWorkflowOptions->Hierarchy = nullptr;
			TLLRN_ControlRigNodeWorkflowOptions->Selection.Reset();
			
			if(const URigVMBlueprint* StrongThis = WeakThis.Get())
			{
				if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(StrongThis->GetObjectBeingDebugged()))
				{
					TLLRN_ControlRigNodeWorkflowOptions->Hierarchy = TLLRN_ControlRig->GetHierarchy();
					TLLRN_ControlRigNodeWorkflowOptions->Selection = TLLRN_ControlRig->GetHierarchy()->GetSelectedKeys();
				}
			}
		}
	});
}

#endif

void UTLLRN_ControlRigBlueprint::UpdateConnectionMapAfterRename(const FString& InOldNameSpace)
{
	const FString OldNameSpace = InOldNameSpace + UTLLRN_ModularRig::NamespaceSeparator;
	const FString NewNameSpace = TLLRN_RigModuleSettings.Identifier.Name + UTLLRN_ModularRig::NamespaceSeparator;
	
	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> FixedConnectionMap;
	for(const TPair<FTLLRN_RigElementKey, FTLLRN_RigElementKey>& Pair : ConnectionMap)
	{
		auto FixUpConnectionMap = [OldNameSpace, NewNameSpace](const FTLLRN_RigElementKey& InKey) -> FTLLRN_RigElementKey
		{
			const FString NameString = InKey.Name.ToString();
			if(NameString.StartsWith(OldNameSpace, ESearchCase::CaseSensitive))
			{
				return FTLLRN_RigElementKey(*(NewNameSpace + NameString.Mid(OldNameSpace.Len())), InKey.Type);
			}
			return InKey;
		};

		const FTLLRN_RigElementKey Key = FixUpConnectionMap(Pair.Key);
		const FTLLRN_RigElementKey Value = FixUpConnectionMap(Pair.Value);
		FixedConnectionMap.FindOrAdd(Key) = Value;
	}

	Swap(ConnectionMap, FixedConnectionMap);
}

UClass* UTLLRN_ControlRigBlueprint::GetRigVMEdGraphNodeClass() const
{
	return UTLLRN_ControlRigGraphNode::StaticClass();
}

UClass* UTLLRN_ControlRigBlueprint::GetRigVMEdGraphSchemaClass() const
{
	return UTLLRN_ControlRigGraphSchema::StaticClass();
}

UClass* UTLLRN_ControlRigBlueprint::GetRigVMEdGraphClass() const
{
	return UTLLRN_ControlRigGraph::StaticClass();
}

UClass* UTLLRN_ControlRigBlueprint::GetRigVMEditorSettingsClass() const
{
	return UTLLRN_ControlRigEditorSettings::StaticClass();
}

void UTLLRN_ControlRigBlueprint::GetPreloadDependencies(TArray<UObject*>& OutDeps)
{
	Super::GetPreloadDependencies(OutDeps);

	for (FTLLRN_RigModuleReference& Module : TLLRN_ModularRigModel.Modules)
	{
		OutDeps.Add(Module.Class.Get());
	}
}

#if WITH_EDITOR
const FLazyName& UTLLRN_ControlRigBlueprint::GetPanelPinFactoryName() const
{
	return TLLRN_ControlRigPanelNodeFactoryName;
}

IRigVMEditorModule* UTLLRN_ControlRigBlueprint::GetEditorModule() const
{
	return &ITLLRN_ControlRigEditorModule::Get();
}
#endif

TArray<FString> UTLLRN_ControlRigBlueprint::GeneratePythonCommands(const FString InNewBlueprintName)
{
	TArray<FString> InternalCommands;
	InternalCommands.Add(TEXT("import unreal"));
	InternalCommands.Add(TEXT("unreal.load_module('TLLRN_ControlRigDeveloper')"));
	InternalCommands.Add(TEXT("factory = unreal.TLLRN_ControlRigBlueprintFactory"));
	InternalCommands.Add(FString::Printf(TEXT("blueprint = factory.create_new_control_rig_asset(desired_package_path = '%s')"), *InNewBlueprintName));
	InternalCommands.Add(TEXT("hierarchy = blueprint.hierarchy"));
	InternalCommands.Add(TEXT("hierarchy_controller = hierarchy.get_controller()"));

	// Hierarchy
	InternalCommands.Append(Hierarchy->GetController(true)->GeneratePythonCommands());

#if WITH_EDITORONLY_DATA
	const FString PreviewMeshPath = GetPreviewMesh()->GetPathName();
	InternalCommands.Add(FString::Printf(TEXT("blueprint.set_preview_mesh(unreal.load_object(name='%s', outer=None))"),
		*PreviewMeshPath));
#endif

	InternalCommands.Append(Super::GeneratePythonCommands(InNewBlueprintName));
	return InternalCommands;
}

void UTLLRN_ControlRigBlueprint::GetTypeActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	ITLLRN_ControlRigEditorModule::Get().GetTypeActions((UTLLRN_ControlRigBlueprint*)this, ActionRegistrar);
}

void UTLLRN_ControlRigBlueprint::GetInstanceActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	ITLLRN_ControlRigEditorModule::Get().GetInstanceActions((UTLLRN_ControlRigBlueprint*)this, ActionRegistrar);
}

void UTLLRN_ControlRigBlueprint::PostTransacted(const FTransactionObjectEvent& TransactionEvent)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()
	Super::PostTransacted(TransactionEvent);

	if (TransactionEvent.GetEventType() == ETransactionObjectEventType::UndoRedo)
	{
		TArray<FName> PropertiesChanged = TransactionEvent.GetChangedProperties();
		int32 TransactionIndex = GEditor->Trans->FindTransactionIndex(TransactionEvent.GetTransactionId());
		const FTransaction* Transaction = GEditor->Trans->GetTransaction(TransactionIndex);
		if (Transaction && Transaction->ContainsObject(Hierarchy))
		{
			if (Transaction->GetTitle().BuildSourceString() == TEXT("Transform Gizmo"))
			{
				PropagatePoseFromBPToInstances();
				return;
			}

			PropagateHierarchyFromBPToInstances();

			// make sure the bone name list is up 2 date for the editor graph
			for (UEdGraph* Graph : UbergraphPages)
			{
				UTLLRN_ControlRigGraph* RigGraph = Cast<UTLLRN_ControlRigGraph>(Graph);
				if (RigGraph == nullptr)
				{
					continue;
				}
				RigGraph->CacheNameLists(Hierarchy, &DrawContainer, ShapeLibraries);
			}

			RequestAutoVMRecompilation();
			(void)MarkPackageDirty();
		}

		if (PropertiesChanged.Contains(GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, TLLRN_ModularRigModel)))
		{
			if (IsTLLRN_ModularRig())
			{
				TLLRN_ModularRigModel.UpdateCachedChildren();
				TLLRN_ModularRigModel.Connections.UpdateFromConnectionList();
				RecompileTLLRN_ModularRig();
			}
		}
		
		if (PropertiesChanged.Contains(GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, DrawContainer)))
		{
			PropagateDrawInstructionsFromBPToInstances();
		}

		if (PropertiesChanged.Contains(GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, ConnectionMap)))
		{
			PropagateHierarchyFromBPToInstances();
		}
	}
}

void UTLLRN_ControlRigBlueprint::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	
	if (UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true))
	{
		Controller->OnModified().RemoveAll(this);
		Controller->OnModified().AddUObject(this, &UTLLRN_ControlRigBlueprint::HandleHierarchyModified);
	}

	if (UTLLRN_ModularTLLRN_RigController* ModularController = TLLRN_ModularRigModel.GetController())
	{
		ModularController->OnModified().RemoveAll(this);
		ModularController->OnModified().AddUObject(this, &UTLLRN_ControlRigBlueprint::HandleTLLRN_RigModulesModified);
	}

	// update the rig module identifier after save-as or duplicate asset
	if(IsTLLRN_ControlTLLRN_RigModule())
	{
		const FString OldNameSpace = TLLRN_RigModuleSettings.Identifier.Name;
		TLLRN_RigModuleSettings.Identifier.Name = UTLLRN_RigHierarchy::GetSanitizedName(FTLLRN_RigName(GetName())).ToString();
		UpdateConnectionMapAfterRename(OldNameSpace);
	}

	TLLRN_ModularRigModel.UpdateCachedChildren();
	TLLRN_ModularRigModel.Connections.UpdateFromConnectionList();
}

void UTLLRN_ControlRigBlueprint::PostRename(UObject* OldOuter, const FName OldName)
{
	Super::PostRename(OldOuter, OldName);

	// update the rig module identifier after renaming the asset
	if(IsTLLRN_ControlTLLRN_RigModule())
	{
		const FString OldNameSpace = TLLRN_RigModuleSettings.Identifier.Name; 
		TLLRN_RigModuleSettings.Identifier.Name = UTLLRN_RigHierarchy::GetSanitizedName(FTLLRN_RigName(GetName())).ToString();
		UpdateConnectionMapAfterRename(OldNameSpace);
	}
}

TArray<UTLLRN_ControlRigBlueprint*> UTLLRN_ControlRigBlueprint::GetCurrentlyOpenRigBlueprints()
{
	return sCurrentlyOpenedRigBlueprints;
}

#if WITH_EDITOR

const FTLLRN_ControlRigShapeDefinition* UTLLRN_ControlRigBlueprint::GetControlShapeByName(const FName& InName) const
{
	TMap<FString, FString> LibraryNameMap;
	if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetObjectBeingDebugged()))
	{
		LibraryNameMap = TLLRN_ControlRig->ShapeLibraryNameMap;
	}
	return UTLLRN_ControlRigShapeLibrary::GetShapeByName(InName, ShapeLibraries, LibraryNameMap);
}

FName UTLLRN_ControlRigBlueprint::AddTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget)
{
	TUniquePtr<FControlValueScope> ValueScope;
	if (!UTLLRN_ControlRigEditorSettings::Get()->bResetControlsOnPinValueInteraction) // if we need to retain the controls
	{
		ValueScope = MakeUnique<FControlValueScope>(this);
	}

	// for now we only allow one pin control at the same time
	ClearTransientControls();

	URigVMBlueprintGeneratedClass* RigClass = GetRigVMBlueprintGeneratedClass();
	UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(true /* create if needed */));

	FName ReturnName = NAME_None;
	TArray<UObject*> ArchetypeInstances;
	CDO->GetArchetypeInstances(ArchetypeInstances);
	for (UObject* ArchetypeInstance : ArchetypeInstances)
	{
		UTLLRN_ControlRig* InstancedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance);
		if (InstancedTLLRN_ControlRig)
		{
			FName ControlName = InstancedTLLRN_ControlRig->AddTransientControl(InNode, InTarget);
			if (ReturnName == NAME_None)
			{
				ReturnName = ControlName;
			}
		}
	}

	return ReturnName;
}

FName UTLLRN_ControlRigBlueprint::RemoveTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget)
{
	TUniquePtr<FControlValueScope> ValueScope;
	if (!UTLLRN_ControlRigEditorSettings::Get()->bResetControlsOnPinValueInteraction) // if we need to retain the controls
	{
		ValueScope = MakeUnique<FControlValueScope>(this);
	}

	URigVMBlueprintGeneratedClass* RigClass = GetRigVMBlueprintGeneratedClass();
	UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(true /* create if needed */));

	FName RemovedName = NAME_None;
	TArray<UObject*> ArchetypeInstances;
	CDO->GetArchetypeInstances(ArchetypeInstances);
	for (UObject* ArchetypeInstance : ArchetypeInstances)
	{
		UTLLRN_ControlRig* InstancedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance);
		if (InstancedTLLRN_ControlRig)
		{
			FName Name = InstancedTLLRN_ControlRig->RemoveTransientControl(InNode, InTarget);
			if (RemovedName == NAME_None)
	{
				RemovedName = Name;
			}
		}
	}

	return RemovedName;
}

FName UTLLRN_ControlRigBlueprint::AddTransientControl(const FTLLRN_RigElementKey& InElement)
{
	TUniquePtr<FControlValueScope> ValueScope;
	if (!UTLLRN_ControlRigEditorSettings::Get()->bResetControlsOnPinValueInteraction) // if we need to retain the controls
	{
		ValueScope = MakeUnique<FControlValueScope>(this);
	}
	URigVMBlueprintGeneratedClass* RigClass = GetRigVMBlueprintGeneratedClass();
	UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(true /* create if needed */));

	FName ReturnName = NAME_None;
	TArray<UObject*> ArchetypeInstances;
	CDO->GetArchetypeInstances(ArchetypeInstances);

	// hierarchy transforms will be reset when ClearTransientControls() is called,
	// so to retain any bone transform modifications we have to save them
	TMap<UObject*, FTransform> SavedElementLocalTransforms;
	for (UObject* ArchetypeInstance : ArchetypeInstances)
	{
		UTLLRN_ControlRig* InstancedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance);
		if (InstancedTLLRN_ControlRig)
		{
			if (InstancedTLLRN_ControlRig->DynamicHierarchy)
			{ 
				SavedElementLocalTransforms.FindOrAdd(InstancedTLLRN_ControlRig) = InstancedTLLRN_ControlRig->DynamicHierarchy->GetLocalTransform(InElement);
			}
		}
	}

	// for now we only allow one pin control at the same time
	ClearTransientControls();
	
	for (UObject* ArchetypeInstance : ArchetypeInstances)
	{
		UTLLRN_ControlRig* InstancedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance);
		if (InstancedTLLRN_ControlRig)
		{
			// restore the element transforms so that transient controls are created at the right place
			if (const FTransform* SavedTransform = SavedElementLocalTransforms.Find(InstancedTLLRN_ControlRig))
			{
				if (InstancedTLLRN_ControlRig->DynamicHierarchy)
				{ 
					InstancedTLLRN_ControlRig->DynamicHierarchy->SetLocalTransform(InElement, *SavedTransform);
				}
			}
			
			FName ControlName = InstancedTLLRN_ControlRig->AddTransientControl(InElement);
			if (ReturnName == NAME_None)
			{
				ReturnName = ControlName;
			}
		}
	}

	return ReturnName;

}

FName UTLLRN_ControlRigBlueprint::RemoveTransientControl(const FTLLRN_RigElementKey& InElement)
{
	TUniquePtr<FControlValueScope> ValueScope;
	if (!UTLLRN_ControlRigEditorSettings::Get()->bResetControlsOnPinValueInteraction) // if we need to retain the controls
	{
		ValueScope = MakeUnique<FControlValueScope>(this);
	}

	URigVMBlueprintGeneratedClass* RigClass = GetRigVMBlueprintGeneratedClass();
	UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(true /* create if needed */));

	FName RemovedName = NAME_None;
	TArray<UObject*> ArchetypeInstances;
	CDO->GetArchetypeInstances(ArchetypeInstances);
	for (UObject* ArchetypeInstance : ArchetypeInstances)
	{
		UTLLRN_ControlRig* InstancedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance);
		if (InstancedTLLRN_ControlRig)
		{
			FName Name = InstancedTLLRN_ControlRig->RemoveTransientControl(InElement);
			if (RemovedName == NAME_None)
			{
				RemovedName = Name;
			}
		}
	}

	return RemovedName;
}

void UTLLRN_ControlRigBlueprint::ClearTransientControls()
{
	bool bHasAnyTransientControls = false;
	
	if (URigVMBlueprintGeneratedClass* RigClass = GetRigVMBlueprintGeneratedClass())
	{
		UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(true /* create if needed */));

		TArray<UObject*> ArchetypeInstances;
		CDO->GetArchetypeInstances(ArchetypeInstances);
		for (UObject* ArchetypeInstance : ArchetypeInstances)
		{
			UTLLRN_ControlRig* InstancedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance);
			if (InstancedTLLRN_ControlRig)
			{
				if(!InstancedTLLRN_ControlRig->GetHierarchy()->GetTransientControls().IsEmpty())
				{
					bHasAnyTransientControls = true;
					break;
				}
			}
		}
	}

	if(!bHasAnyTransientControls)
	{
		return;
	}

	TUniquePtr<FControlValueScope> ValueScope;
	if (!UTLLRN_ControlRigEditorSettings::Get()->bResetControlsOnPinValueInteraction) // if we need to retain the controls
	{
		ValueScope = MakeUnique<FControlValueScope>(this);
	}

	if (URigVMBlueprintGeneratedClass* RigClass = GetRigVMBlueprintGeneratedClass())
	{
		UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(true /* create if needed */));

		TArray<UObject*> ArchetypeInstances;
		CDO->GetArchetypeInstances(ArchetypeInstances);
		for (UObject* ArchetypeInstance : ArchetypeInstances)
		{
			UTLLRN_ControlRig* InstancedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance);
			if (InstancedTLLRN_ControlRig)
			{
				InstancedTLLRN_ControlRig->ClearTransientControls();
			}
		}
	}
}

UTLLRN_ModularTLLRN_RigController* UTLLRN_ControlRigBlueprint::GetTLLRN_ModularTLLRN_RigController() 
{
	if (!GetTLLRN_ControlRigClass()->IsChildOf(UTLLRN_ModularRig::StaticClass()))
	{
		return nullptr;
	}

	return TLLRN_ModularRigModel.GetController();
}

void UTLLRN_ControlRigBlueprint::RecompileTLLRN_ModularRig()
{
	RefreshModuleConnectors();
	OnTLLRN_ModularRigPreCompiled().Broadcast(this);
	if (const UClass* MyTLLRN_ControlRigClass = GeneratedClass)
	{
		if (UTLLRN_ModularRig* DefaultObject = Cast<UTLLRN_ModularRig>(MyTLLRN_ControlRigClass->GetDefaultObject(false)))
		{
			PropagateModuleHierarchyFromBPToInstances();
		}
	}
	UpdateModularDependencyDelegates();
	OnTLLRN_ModularRigCompiled().Broadcast(this);
}

#endif

void UTLLRN_ControlRigBlueprint::SetupDefaultObjectDuringCompilation(URigVMHost* InCDO)
{
	Super::SetupDefaultObjectDuringCompilation(InCDO);
	CastChecked<UTLLRN_ControlRig>(InCDO)->GetHierarchy()->CopyHierarchy(Hierarchy);
}

void UTLLRN_ControlRigBlueprint::SetupPinRedirectorsForBackwardsCompatibility()
{
	for(URigVMGraph* Model : RigVMClient)
	{
		for (URigVMNode* Node : Model->GetNodes())
		{
			if (URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(Node))
			{
				UScriptStruct* Struct = UnitNode->GetScriptStruct();
				if (Struct == FTLLRN_RigUnit_SetBoneTransform::StaticStruct())
				{
					URigVMPin* TransformPin = UnitNode->FindPin(TEXT("Transform"));
					URigVMPin* ResultPin = UnitNode->FindPin(TEXT("Result"));
					GetOrCreateController()->AddPinRedirector(false, true, TransformPin->GetPinPath(), ResultPin->GetPinPath());
				}
			}
		}
	}
}

void UTLLRN_ControlRigBlueprint::PathDomainSpecificContentOnLoad()
{
	PatchTLLRN_RigElementKeyCacheOnLoad();
	PatchPropagateToChildren();
}

void UTLLRN_ControlRigBlueprint::PatchTLLRN_RigElementKeyCacheOnLoad()
{
	if (GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::TLLRN_RigElementKeyCache)
	{
		for (URigVMGraph* Graph : GetAllModels())
		{
			URigVMController* Controller = GetOrCreateController(Graph);
			TGuardValue<bool> DisablePinDefaultValueValidation(Controller->bValidatePinDefaults, false);
			FRigVMControllerNotifGuard NotifGuard(Controller, true);
			for (URigVMNode* Node : Graph->GetNodes())
			{
				if (URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(Node))
				{
					UScriptStruct* ScriptStruct = UnitNode->GetScriptStruct();
					FString FunctionName = FString::Printf(TEXT("%s::%s"), *ScriptStruct->GetStructCPPName(), *UnitNode->GetMethodName().ToString());
					const FRigVMFunction* Function = FRigVMRegistry::Get().FindFunction(*FunctionName);
					check(Function);
					for (TFieldIterator<FProperty> It(Function->Struct); It; ++It)
					{
						if (It->GetCPPType() == TEXT("FTLLRN_CachedTLLRN_RigElement"))
						{
							if (URigVMPin* Pin = Node->FindPin(It->GetName()))
							{
								int32 BoneIndex = FCString::Atoi(*Pin->GetDefaultValue());
								FTLLRN_RigElementKey Key = Hierarchy->GetKey(BoneIndex);
								FTLLRN_CachedTLLRN_RigElement DefaultValueElement(Key, Hierarchy);
								FString Result;
								TBaseStructure<FTLLRN_CachedTLLRN_RigElement>::Get()->ExportText(Result, &DefaultValueElement, nullptr, nullptr, PPF_None, nullptr);								
								FRigVMDefaultValueTypeGuard _(Controller, ERigVMPinDefaultValueType::Override);
								Controller->SetPinDefaultValue(Pin->GetPinPath(), Result, true, false, false);
								MarkDirtyDuringLoad();
							}							
						}
					}
				}
			}
		}
	}
}

// change the default value form False to True for transform nodes
void UTLLRN_ControlRigBlueprint::PatchPropagateToChildren()
{
	// no need to update default value past this version
	if (GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::RenameGizmoToShape)
	{
		return;
	}
	
	auto IsNullOrControl = [](const URigVMPin* InPin)
	{
		const bool bHasItem = InPin->GetCPPTypeObject() == FTLLRN_RigElementKey::StaticStruct() && InPin->GetName() == "Item";
		if (!bHasItem)
		{
			return false;
		}

		if (const URigVMPin* TypePin = InPin->FindSubPin(TEXT("Type")))
		{
			const FString& TypeValue = TypePin->GetDefaultValue();
			return TypeValue == TEXT("Null") || TypeValue == TEXT("Space") || TypeValue == TEXT("Control");
		}
		
		return false;
	};

	auto IsPropagateChildren = [](const URigVMPin* InPin)
	{
		return InPin->GetCPPType() == TEXT("bool") && InPin->GetName() == TEXT("bPropagateToChildren");
	};

	auto FindPropagatePin = [IsNullOrControl, IsPropagateChildren](const URigVMNode* InNode)-> URigVMPin*
	{
		URigVMPin* PropagatePin = nullptr;
		URigVMPin* ItemPin = nullptr;  
		for (URigVMPin* Pin: InNode->GetPins())
		{
			// look for Item pin
			if (!ItemPin && IsNullOrControl(Pin))
			{
				ItemPin = Pin;
			}

			// look for bPropagateToChildren pin
			if (!PropagatePin && IsPropagateChildren(Pin))
			{
				PropagatePin = Pin;
			}

			// return propagation pin if both found
			if (ItemPin && PropagatePin)
			{
				return PropagatePin;
			}
		}
		return nullptr;
	};

	for (URigVMGraph* Graph : GetAllModels())
	{
		TArray< const URigVMPin* > PinsToUpdate;
		for (const URigVMNode* Node : Graph->GetNodes())
		{
			if (const URigVMPin* PropagatePin = FindPropagatePin(Node))
			{
				PinsToUpdate.Add(PropagatePin);
			}
		}
		
		if (URigVMController* Controller = GetOrCreateController(Graph))
		{
			FRigVMControllerNotifGuard NotifGuard(Controller, true);
			for (const URigVMPin* Pin: PinsToUpdate)
			{
				Controller->SetPinDefaultValue(Pin->GetPinPath(), TEXT("True"), false, false, false);
			}
		}
	}
}

void UTLLRN_ControlRigBlueprint::GetBackwardsCompatibilityPublicFunctions(TArray<FName>& BackwardsCompatiblePublicFunctions, TMap<URigVMLibraryNode*, FRigVMGraphFunctionHeader>& OldHeaders)
{
	URigVMBlueprintGeneratedClass* CRGeneratedClass = GetRigVMBlueprintGeneratedClass();
	FRigVMGraphFunctionStore& Store = CRGeneratedClass->GraphFunctionStore;
	if (GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::StoreFunctionsInGeneratedClass)
	{
		for (const FRigVMOldPublicFunctionData& OldPublicFunction : PublicFunctions_DEPRECATED)
		{
			BackwardsCompatiblePublicFunctions.Add(OldPublicFunction.Name);
		}
	}
	else
	{
		if (GetLinkerCustomVersion(FUE5MainStreamObjectVersion::GUID) < FUE5MainStreamObjectVersion::RigVMSaveFunctionAccessInModel)
		{
			for (const FRigVMGraphFunctionData& FunctionData : Store.PublicFunctions)
			{
				BackwardsCompatiblePublicFunctions.Add(FunctionData.Header.Name);
				URigVMLibraryNode* LibraryNode = Cast<URigVMLibraryNode>(FunctionData.Header.LibraryPointer.GetNodeSoftPath().ResolveObject());
				OldHeaders.Add(LibraryNode, FunctionData.Header);
			}
		}
	}

	// Addressing issue where PublicGraphFunctions is populated, but the model PublicFunctionNames is not
	URigVMFunctionLibrary* FunctionLibrary = GetLocalFunctionLibrary();
	if (FunctionLibrary)
	{
		// if (PublicGraphFunctions.Num() > FunctionLibrary->PublicFunctionNames.Num())
		// {
		// 	for (const FRigVMGraphFunctionHeader& PublicHeader : PublicGraphFunctions)
		// 	{
		// 		BackwardsCompatiblePublicFunctions.Add(PublicHeader.Name);
		// 	}
		// }
	}
}

void UTLLRN_ControlRigBlueprint::CreateMemberVariablesOnLoad()
{
#if WITH_EDITOR

	const int32 LinkerVersion = GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);
	if (LinkerVersion < FTLLRN_ControlRigObjectVersion::SwitchedToRigVM)
	{
		// ignore errors during the first potential compile of the VM
		// since that this point variable nodes may still be ill-formed.
		TGuardValue<FRigVMReportDelegate> SuspendReportDelegate(VMCompileSettings.ASTSettings.ReportDelegate,
			FRigVMReportDelegate::CreateLambda([](EMessageSeverity::Type,  UObject*, const FString&)
			{
				// do nothing
			})
		);
		InitializeModelIfRequired();
	}

	AddedMemberVariableMap.Reset();

	for (int32 VariableIndex = 0; VariableIndex < NewVariables.Num(); VariableIndex++)
	{
		AddedMemberVariableMap.Add(NewVariables[VariableIndex].VarName, VariableIndex);
	}

	if (RigVMClient.Num() == 0)
	{
		return;
	}

	// setup variables on the blueprint based on the previous "parameters"
	if (GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::BlueprintVariableSupport)
	{
		TSharedPtr<FKismetNameValidator> NameValidator = MakeShareable(new FKismetNameValidator(this, NAME_None, nullptr));

		auto CreateVariable = [this, NameValidator](const URigVMVariableNode* InVariableNode)
		{
			if (!InVariableNode)
			{
				return;
			}
			
			static const FString VariableString = TEXT("Variable");
			if (URigVMPin* VariablePin = InVariableNode->FindPin(VariableString))
			{
				if (VariablePin->GetDirection() != ERigVMPinDirection::Visible)
				{
					return;
				}
			}

			const FRigVMGraphVariableDescription Description = InVariableNode->GetVariableDescription();
			if (AddedMemberVariableMap.Contains(Description.Name))
			{
				return;
			}

			const FEdGraphPinType PinType = RigVMTypeUtils::PinTypeFromExternalVariable(Description.ToExternalVariable());
			if (!PinType.PinCategory.IsValid())
			{
				return;
			}

			const FName VarName = FindHostMemberVariableUniqueName(NameValidator, Description.Name.ToString());
			const int32 VariableIndex = AddHostMemberVariable(this, VarName, PinType, false, false, FString());
			if (VariableIndex != INDEX_NONE)
			{
				AddedMemberVariableMap.Add(Description.Name, VariableIndex);
				MarkDirtyDuringLoad();
			}
		};

		auto CreateParameter = [this, NameValidator](const URigVMParameterNode* InParameterNode)
		{
			if (!InParameterNode)
			{
				return;
			}

			static const FString ParameterString = TEXT("Parameter");
			if (const URigVMPin* ParameterPin = InParameterNode->FindPin(ParameterString))
			{
				if (ParameterPin->GetDirection() != ERigVMPinDirection::Visible)
				{
					return;
				}
			}

			const FRigVMGraphParameterDescription Description = InParameterNode->GetParameterDescription();
			if (AddedMemberVariableMap.Contains(Description.Name))
			{
				return;
			}

			const FEdGraphPinType PinType = RigVMTypeUtils::PinTypeFromExternalVariable(Description.ToExternalVariable());
			if (!PinType.PinCategory.IsValid())
			{
				return;
			}

			const FName VarName = FindHostMemberVariableUniqueName(NameValidator, Description.Name.ToString());
			const int32 VariableIndex = AddHostMemberVariable(this, VarName, PinType, true, !Description.bIsInput, FString());
			
			if (VariableIndex != INDEX_NONE)
			{
				AddedMemberVariableMap.Add(Description.Name, VariableIndex);
				MarkDirtyDuringLoad();
			}
		};
		
		for (const URigVMGraph* Model : RigVMClient)
		{
			const TArray<URigVMNode*>& Nodes = Model->GetNodes();
			for (const URigVMNode* Node : Nodes)
			{
				if (const URigVMVariableNode* VariableNode = Cast<URigVMVariableNode>(Node))
				{
					CreateVariable(VariableNode);
				}

				// Leaving this for backwards compatibility, even though we don't support parameters anymore
				// When a parameter node is found, we will create a variable
				else if (const URigVMParameterNode* ParameterNode = Cast<URigVMParameterNode>(Node))
				{
					CreateParameter(ParameterNode);
				}
			}
		}
	}

#endif
}

void UTLLRN_ControlRigBlueprint::PatchVariableNodesOnLoad()
{
#if WITH_EDITOR

	// setup variables on the blueprint based on the previous "parameters"
	if (GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::BlueprintVariableSupport)
	{
		TGuardValue<bool> GuardNotifsSelf(bSuspendModelNotificationsForSelf, true);
		
		check(GetDefaultModel());

		auto PatchVariableNode = [this](const URigVMVariableNode* InVariableNode)
		{
			if (!InVariableNode)
			{
				return;
			}

			const FRigVMGraphVariableDescription Description = InVariableNode->GetVariableDescription();
			if (!AddedMemberVariableMap.Contains(Description.Name))
			{
				return;
			}
			
			const int32 VariableIndex = AddedMemberVariableMap.FindChecked(Description.Name);
			const FName VarName = NewVariables[VariableIndex].VarName;
			
			GetOrCreateController()->RefreshVariableNode(
				InVariableNode->GetFName(), VarName, Description.CPPType, Description.CPPTypeObject, false);
			
			MarkDirtyDuringLoad();			
		};

		auto PatchParameterNode = [this](const URigVMParameterNode* InParameterNode)
		{
			if (!InParameterNode)
			{
				return;
			}
			
			const FRigVMGraphParameterDescription Description = InParameterNode->GetParameterDescription();
			if (!AddedMemberVariableMap.Contains(Description.Name))
			{
				return;
			}

			const int32 VariableIndex = AddedMemberVariableMap.FindChecked(Description.Name);
			const FName VarName = NewVariables[VariableIndex].VarName;
			
			GetOrCreateController()->ReplaceParameterNodeWithVariable(
				InParameterNode->GetFName(), VarName, Description.CPPType, Description.CPPTypeObject, false);

			MarkDirtyDuringLoad();	
		};
		
		for(const URigVMGraph* Model : RigVMClient)
		{
			TArray<URigVMNode*> Nodes = Model->GetNodes();
			for (URigVMNode* Node : Nodes)
			{
				if (const URigVMVariableNode* VariableNode = Cast<URigVMVariableNode>(Node))
				{
					PatchVariableNode(VariableNode);
				}
				else if (const URigVMParameterNode* ParameterNode = Cast<URigVMParameterNode>(Node))
				{
					PatchParameterNode(ParameterNode);
				}
			}
		}
	}

#endif

	Super::PatchVariableNodesOnLoad();
}

void UTLLRN_ControlRigBlueprint::UpdateElementKeyRedirector(UTLLRN_ControlRig* InTLLRN_ControlRig) const
{
	InTLLRN_ControlRig->HierarchySettings = HierarchySettings;
	InTLLRN_ControlRig->TLLRN_RigModuleSettings = TLLRN_RigModuleSettings;
	InTLLRN_ControlRig->ElementKeyRedirector = FTLLRN_RigElementKeyRedirector(ConnectionMap, InTLLRN_ControlRig->GetHierarchy());
}

void UTLLRN_ControlRigBlueprint::PropagatePoseFromInstanceToBP(UTLLRN_ControlRig* InTLLRN_ControlRig) const
{
	check(InTLLRN_ControlRig);
	// current transforms in BP and CDO are meaningless, no need to copy them
	// we use BP hierarchy to initialize CDO and instances' hierarchy, 
	// so it should always be in the initial state.
	Hierarchy->CopyPose(InTLLRN_ControlRig->GetHierarchy(), false, true, false, true);
}

void UTLLRN_ControlRigBlueprint::PropagatePoseFromBPToInstances() const
{
	if (UClass* MyTLLRN_ControlRigClass = GeneratedClass)
	{
		if (UTLLRN_ControlRig* DefaultObject = Cast<UTLLRN_ControlRig>(MyTLLRN_ControlRigClass->GetDefaultObject(false)))
		{
			DefaultObject->PostInitInstanceIfRequired();
			DefaultObject->GetHierarchy()->CopyPose(Hierarchy, true, true, true);

			TArray<UObject*> ArchetypeInstances;
			DefaultObject->GetArchetypeInstances(ArchetypeInstances);
			for (UObject* ArchetypeInstance : ArchetypeInstances)
			{
				if (UTLLRN_ControlRig* InstanceRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance))
				{
					InstanceRig->PostInitInstanceIfRequired();
					if(!InstanceRig->IsTLLRN_RigModuleInstance())
					{
						InstanceRig->GetHierarchy()->CopyPose(Hierarchy, true, true, true);
					}
				}
			}
		}
	}
}

void UTLLRN_ControlRigBlueprint::PropagateHierarchyFromBPToInstances() const
{
	if (UClass* MyTLLRN_ControlRigClass = GeneratedClass)
	{
		if (UTLLRN_ControlRig* DefaultObject = Cast<UTLLRN_ControlRig>(MyTLLRN_ControlRigClass->GetDefaultObject(false)))
		{
			DefaultObject->PostInitInstanceIfRequired();
			DefaultObject->GetHierarchy()->CopyHierarchy(Hierarchy);

			UpdateElementKeyRedirector(DefaultObject);

			if (!DefaultObject->HasAnyFlags(RF_NeedPostLoad)) // If CDO is loading, skip Init, it will be done later
			{
				DefaultObject->Initialize(true);
			}

			TArray<UObject*> ArchetypeInstances;
			DefaultObject->GetArchetypeInstances(ArchetypeInstances);
			for (UObject* ArchetypeInstance : ArchetypeInstances)
			{
				if (UTLLRN_ControlRig* InstanceRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance))
				{
					if (InstanceRig->IsTLLRN_RigModuleInstance())
					{
						if (UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(InstanceRig->GetOuter()))
						{
							TLLRN_ModularRig->RequestInit();
						}
					}
					else
					{
						InstanceRig->PostInitInstanceIfRequired();
						InstanceRig->GetHierarchy()->CopyHierarchy(Hierarchy);
						InstanceRig->HierarchySettings = HierarchySettings;
						UpdateElementKeyRedirector(InstanceRig);
						InstanceRig->Initialize(true);
					}
				}
			}
		}
	}
}

void UTLLRN_ControlRigBlueprint::PropagateDrawInstructionsFromBPToInstances() const
{
	if (UClass* MyTLLRN_ControlRigClass = GeneratedClass)
	{
		if (UTLLRN_ControlRig* DefaultObject = Cast<UTLLRN_ControlRig>(MyTLLRN_ControlRigClass->GetDefaultObject(false)))
	{
			DefaultObject->DrawContainer = DrawContainer;

			TArray<UObject*> ArchetypeInstances;
			DefaultObject->GetArchetypeInstances(ArchetypeInstances);

			for (UObject* ArchetypeInstance : ArchetypeInstances)
			{
				if (UTLLRN_ControlRig* InstanceRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance))
	{
					InstanceRig->DrawContainer = DrawContainer;
				}
			}
		}
	}


	// make sure the bone name list is up 2 date for the editor graph
	for (UEdGraph* Graph : UbergraphPages)
	{
		UTLLRN_ControlRigGraph* RigGraph = Cast<UTLLRN_ControlRigGraph>(Graph);
		if (RigGraph == nullptr)
		{
			continue;
		}
		RigGraph->CacheNameLists(Hierarchy, &DrawContainer, ShapeLibraries);
	}
}

void UTLLRN_ControlRigBlueprint::PropagatePropertyFromBPToInstances(FTLLRN_RigElementKey InTLLRN_RigElement, const FProperty* InProperty) const
{
	int32 ElementIndex = Hierarchy->GetIndex(InTLLRN_RigElement);
	ensure(ElementIndex != INDEX_NONE);
	check(InProperty);

	if (UClass* MyTLLRN_ControlRigClass = GeneratedClass)
	{
		if (UTLLRN_ControlRig* DefaultObject = Cast<UTLLRN_ControlRig>(MyTLLRN_ControlRigClass->GetDefaultObject(false)))
		{
			TArray<UObject*> ArchetypeInstances;
			DefaultObject->GetArchetypeInstances(ArchetypeInstances);

			const int32 PropertyOffset = InProperty->GetOffset_ReplaceWith_ContainerPtrToValuePtr();
			const int32 PropertySize = InProperty->GetSize();

			uint8* Source = ((uint8*)Hierarchy->Get(ElementIndex)) + PropertyOffset;
			for (UObject* ArchetypeInstance : ArchetypeInstances)
			{
				if (UTLLRN_ControlRig* InstanceRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance))
				{
					InstanceRig->PostInitInstanceIfRequired();
					uint8* Dest = ((uint8*)InstanceRig->GetHierarchy()->Get(ElementIndex)) + PropertyOffset;
					FMemory::Memcpy(Dest, Source, PropertySize);
				}
			}
		}
	}
}

void UTLLRN_ControlRigBlueprint::PropagatePropertyFromInstanceToBP(FTLLRN_RigElementKey InTLLRN_RigElement, const FProperty* InProperty, UTLLRN_ControlRig* InInstance) const
{
	const int32 ElementIndex = Hierarchy->GetIndex(InTLLRN_RigElement);
	ensure(ElementIndex != INDEX_NONE);
	check(InProperty);

	const int32 PropertyOffset = InProperty->GetOffset_ReplaceWith_ContainerPtrToValuePtr();
	const int32 PropertySize = InProperty->GetSize();
	uint8* Source = ((uint8*)InInstance->GetHierarchy()->Get(ElementIndex)) + PropertyOffset;
	uint8* Dest = ((uint8*)Hierarchy->Get(ElementIndex)) + PropertyOffset;
	FMemory::Memcpy(Dest, Source, PropertySize);
}

void UTLLRN_ControlRigBlueprint::PropagateModuleHierarchyFromBPToInstances() const
{
	if (const UClass* MyTLLRN_ControlRigClass = GeneratedClass)
	{
		if (UTLLRN_ModularRig* DefaultObject = Cast<UTLLRN_ModularRig>(MyTLLRN_ControlRigClass->GetDefaultObject(false)))
		{
			// We need to first transfer the model from the blueprint to the CDO
			// We then ask instances to initialize which will provoke a call UTLLRN_ModularRig::UpdateModuleHierarchyFromCDO
			
			DefaultObject->ResetModules();

			// copy the model over to the CDO.
			// non-CDO instances are going to instantiate the model into a
			// UObject module instance tree. CDO's are data only to avoid bugs / 
			// behaviors in the blueprint re-instancer - which is disregarding any
			// object under a CDO.
			DefaultObject->TLLRN_ModularRigModel = TLLRN_ModularRigModel;
			DefaultObject->TLLRN_ModularRigModel.SetOuterClientHost(DefaultObject);
			DefaultObject->TLLRN_ModularRigSettings = TLLRN_ModularRigSettings;
			
			TArray<UObject*> ArchetypeInstances;
			DefaultObject->GetArchetypeInstances(ArchetypeInstances);
			for (UObject* ArchetypeInstance : ArchetypeInstances)
			{
				if (UTLLRN_ControlRig* InstanceRig = Cast<UTLLRN_ControlRig>(ArchetypeInstance))
				{
					// this will provoke a call to InitializeFromCDO
					InstanceRig->Initialize(true);
				}
			}
		}
	}
}

void UTLLRN_ControlRigBlueprint::UpdateModularDependencyDelegates()
{
	TArray<const UBlueprint*> VisitList;
	TLLRN_ModularRigModel.ForEachModule([&VisitList, this](const FTLLRN_RigModuleReference* Element) -> bool
	{
		if(const UClass* Class = Element->Class.Get())
		{
			if(UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(Class->ClassGeneratedBy))
			{
				if(!VisitList.Contains(Blueprint))
				{
					Blueprint->OnVMCompiled().RemoveAll(this);
					Blueprint->OnTLLRN_ModularRigCompiled().RemoveAll(this);
					Blueprint->OnVMCompiled().AddUObject(this, &UTLLRN_ControlRigBlueprint::OnModularDependencyVMCompiled);
					Blueprint->OnTLLRN_ModularRigCompiled().AddUObject(this, &UTLLRN_ControlRigBlueprint::OnModularDependencyChanged);
					VisitList.Add(Blueprint);
				}
			}
		}
		return true;
	});
}

void UTLLRN_ControlRigBlueprint::OnModularDependencyVMCompiled(UObject* InBlueprint, URigVM* InVM, FRigVMExtendedExecuteContext& InExecuteContext)
{
	if(URigVMBlueprint* RigVMBlueprint = Cast<URigVMBlueprint>(InBlueprint))
	{
		OnModularDependencyChanged(RigVMBlueprint);
	}
}

void UTLLRN_ControlRigBlueprint::OnModularDependencyChanged(URigVMBlueprint* InBlueprint)
{
	RefreshModuleVariables();
	RefreshModuleConnectors();
	RecompileTLLRN_ModularRig();
}

void UTLLRN_ControlRigBlueprint::RequestConstructionOnAllModules()
{
	// the rig will perform initialize itself - but we should request construction
	check(IsTLLRN_ModularRig());
	
	const URigVMBlueprintGeneratedClass* RigClass = GetRigVMBlueprintGeneratedClass();
	check(RigClass);
	
	UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(true /* create if needed */));

	TArray<UObject*> ArchetypeInstances;
	CDO->GetArchetypeInstances(ArchetypeInstances);

	// visit all or our instances and request construction
	for (UObject* Instance : ArchetypeInstances)
	{
		if (UTLLRN_ModularRig* InstanceRig = Cast<UTLLRN_ModularRig>(Instance))
		{
			InstanceRig->RequestConstruction();
		}
	}

}

void UTLLRN_ControlRigBlueprint::RefreshModuleVariables()
{
	if(!IsTLLRN_ModularRig())
	{
		return;
	}

	if (UTLLRN_ModularTLLRN_RigController* Controller = GetTLLRN_ModularTLLRN_RigController())
	{
		Controller->RefreshModuleVariables(false);
	}
}

void UTLLRN_ControlRigBlueprint::RefreshModuleConnectors()
{
	if(!IsTLLRN_ModularRig())
	{
		return;
	}

	if (UTLLRN_ModularTLLRN_RigController* Controller = GetTLLRN_ModularTLLRN_RigController())
	{
		TGuardValue<bool> NotificationsGuard(Controller->bSuspendNotifications, true);
		TLLRN_ModularRigModel.ForEachModule([this](const FTLLRN_RigModuleReference* Element) -> bool
		{
			RefreshModuleConnectors(Element, false);
			return true;
		});
	}

	PropagateHierarchyFromBPToInstances();
}

void UTLLRN_ControlRigBlueprint::RefreshModuleConnectors(const FTLLRN_RigModuleReference* InModule, bool bPropagateHierarchy)
{
	if(!IsTLLRN_ModularRig())
	{
		return;
	}

	// avoid dead class pointers
	if(InModule->Class.Get() == nullptr)
	{
		return;
	}
	
	const bool bRemoveAllConnectors = !TLLRN_ModularRigModel.FindModule(InModule->GetPath());
	
	if (UTLLRN_RigHierarchyController* Controller = GetHierarchyController())
	{
		if (UTLLRN_ControlRig* CDO = GetTLLRN_ControlRigClass()->GetDefaultObject<UTLLRN_ControlRig>())
		{
			const FString Namespace = InModule->GetNamespace();
			const TArray<FTLLRN_RigElementKey> AllConnectors = Hierarchy->GetKeysOfType<FTLLRN_RigConnectorElement>();
			const TArray<FTLLRN_RigElementKey> ExistingConnectors = AllConnectors.FilterByPredicate([Namespace](const FTLLRN_RigElementKey& ConnectorKey) -> bool
			{
				const FString ConnectorName = ConnectorKey.Name.ToString();
				FString ConnectorNamespace;
				(void)UTLLRN_RigHierarchy::SplitNameSpace(ConnectorName, &ConnectorNamespace, nullptr);
				ConnectorNamespace.Append(UTLLRN_ModularRig::NamespaceSeparator);
				return ConnectorNamespace.Equals(Namespace, ESearchCase::CaseSensitive);
			});

			// setup the module information. this is needed so that newly added
			// connectors result in the right namespace metadata etc
			FRigVMExtendedExecuteContext& Context = CDO->GetRigVMExtendedExecuteContext();
			FTLLRN_ControlRigExecuteContext& PublicContext = Context.GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
			const UTLLRN_ControlRig* ModuleCDO = InModule->Class->GetDefaultObject<UTLLRN_ControlRig>();
			const TArray<FTLLRN_RigModuleConnector>& ExpectedConnectors = ModuleCDO->GetTLLRN_RigModuleSettings().ExposedConnectors;

			// remove the obsolete connectors
			for(const FTLLRN_RigElementKey& Connector : ExistingConnectors)
			{
				const FString ConnectorNameString = Connector.Name.ToString();
				FString ConnectorNamespace, ShortName = ConnectorNameString;
				(void)UTLLRN_RigHierarchy::SplitNameSpace(ConnectorNameString, &ConnectorNamespace, &ShortName);
				const bool bConnectorExpected = ExpectedConnectors.ContainsByPredicate(
					[ShortName](const FTLLRN_RigModuleConnector& ExpectedConnector) -> bool
					{
						return ExpectedConnector.Name.Equals(ShortName, ESearchCase::CaseSensitive);
					}
				);
				
				if(bRemoveAllConnectors || !bConnectorExpected)
				{
					Hierarchy->Modify();
					(void)Controller->RemoveElement(Connector);
					ConnectionMap.Remove(Connector);
				}
			}

			// add the missing expected connectors
			if(!bRemoveAllConnectors)
			{
				for (const FTLLRN_RigModuleConnector& Connector : ExpectedConnectors)
				{
					const FName ConnectorName = *Connector.Name;
					const FName ConnectorNameWithNameSpace = *UTLLRN_RigHierarchy::JoinNameSpace(InModule->GetNamespace(), Connector.Name);
					const FTLLRN_RigElementKey ConnectorKeyWithNameSpace(ConnectorNameWithNameSpace, ETLLRN_RigElementType::Connector);
					if(!Hierarchy->Contains(ConnectorKeyWithNameSpace))
					{
						FTLLRN_RigHierarchyExecuteContextBracket HierarchyContextGuard(Hierarchy, &Context);
						FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard TLLRN_RigModuleGuard(PublicContext, InModule->GetNamespace());
						Hierarchy->Modify();
						(void)Controller->AddConnector(ConnectorName, Connector.Settings);
					}
					else
					{
						// copy the connector settings
						FTLLRN_RigConnectorElement* ExistingConnector = Hierarchy->FindChecked<FTLLRN_RigConnectorElement>(ConnectorKeyWithNameSpace);
						ExistingConnector->Settings = Connector.Settings;
					}
				}
			}

			if (bPropagateHierarchy)
			{
				PropagateHierarchyFromBPToInstances();
			}
		}
	}
}

void UTLLRN_ControlRigBlueprint::HandleHierarchyModified(ETLLRN_RigHierarchyNotification InNotification, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
#if WITH_EDITOR

	if(bSuspendAllNotifications)
	{
		return;
	}

	switch(InNotification)
	{
		case ETLLRN_RigHierarchyNotification::ElementRemoved:
		{
			Modify();
			Influences.OnKeyRemoved(InElement->GetKey());
			PropagateHierarchyFromBPToInstances();
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementRenamed:
		{
			Modify();
			const FTLLRN_RigElementKey OldKey(InHierarchy->GetPreviousName(InElement->GetKey()), InElement->GetType());
			Influences.OnKeyRenamed(OldKey, InElement->GetKey());
			if (IsTLLRN_ControlTLLRN_RigModule() && InElement->IsTypeOf(ETLLRN_RigElementType::Connector))
			{
				if (FTLLRN_RigElementKey* Target = ConnectionMap.Find(OldKey))
				{
					ConnectionMap.FindOrAdd(InElement->GetKey(), *Target);
					ConnectionMap.Remove(OldKey);
				}
			}
			PropagateHierarchyFromBPToInstances();
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementAdded:
		case ETLLRN_RigHierarchyNotification::ParentChanged:
		case ETLLRN_RigHierarchyNotification::ElementReordered:
		case ETLLRN_RigHierarchyNotification::HierarchyReset:
		{
			Modify();
			PropagateHierarchyFromBPToInstances();
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementSelected:
		{
			bool bClearTransientControls = true;
			if (const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InElement))
			{
				if (ControlElement->Settings.bIsTransientControl)
				{
					bClearTransientControls = false;
				}
			}

			if(bClearTransientControls)
			{
				if(UTLLRN_ControlRig* RigBeingDebugged = Cast<UTLLRN_ControlRig>(GetObjectBeingDebugged()))
				{
					const FName TransientControlName = UTLLRN_ControlRig::GetNameForTransientControl(InElement->GetKey());
					const FTLLRN_RigElementKey TransientControlKey(TransientControlName, ETLLRN_RigElementType::Control);
					if (const FTLLRN_RigControlElement* ControlElement = RigBeingDebugged->GetHierarchy()->Find<FTLLRN_RigControlElement>(TransientControlKey))
					{
						if (ControlElement->Settings.bIsTransientControl)
						{
							bClearTransientControls = false;
						}
					}
				}
			}

			if(bClearTransientControls)
			{
				ClearTransientControls();
			}
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementDeselected:
		{
			if (const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InElement))
			{
				if (ControlElement->Settings.bIsTransientControl)
				{
					ClearTransientControls();
				}
			}
			break;
		}
		default:
		{
			break;
		}
	}

	HierarchyModifiedEvent.Broadcast(InNotification, InHierarchy, InElement);
	
#endif
}

void UTLLRN_ControlRigBlueprint::HandleTLLRN_RigModulesModified(ETLLRN_ModularRigNotification InNotification, const FTLLRN_RigModuleReference* InModule)
{
	bool bRecompile = true;
	switch (InNotification)
	{
		case ETLLRN_ModularRigNotification::ModuleAdded:
		{
			if (InModule)
			{
				RefreshModuleConnectors(InModule);
				UpdateModularDependencyDelegates();
			}
			break;
		}
		case ETLLRN_ModularRigNotification::ModuleRenamed:
		case ETLLRN_ModularRigNotification::ModuleReparented:
		{
			if (InModule)
			{
				if (UTLLRN_RigHierarchyController* Controller = GetHierarchyController())
				{
					if (UTLLRN_ControlRig* CDO = GetTLLRN_ControlRigClass()->GetDefaultObject<UTLLRN_ControlRig>())
					{
						Hierarchy->Modify();
						
						struct ConnectionInfo
						{
							FString NewPath;
							FTLLRN_RigElementKey TargetConnection;
							FTLLRN_RigConnectorSettings Settings;
						};
						FString OldNamespace;
						if (InNotification == ETLLRN_ModularRigNotification::ModuleRenamed)
						{
							OldNamespace = (InModule->ParentPath.IsEmpty()) ? *InModule->PreviousName.ToString() : UTLLRN_RigHierarchy::JoinNameSpace(InModule->ParentPath, InModule->PreviousName.ToString());
						}
						else if(InNotification == ETLLRN_ModularRigNotification::ModuleReparented)
						{
							OldNamespace = (InModule->PreviousParentPath.IsEmpty()) ? *InModule->PreviousName.ToString() : UTLLRN_RigHierarchy::JoinNameSpace(InModule->PreviousParentPath, InModule->PreviousName.ToString());
						}
						FString NewNamespace = (InModule->ParentPath.IsEmpty()) ? *InModule->Name.ToString() : UTLLRN_RigHierarchy::JoinNameSpace(InModule->ParentPath, InModule->Name.ToString());
						OldNamespace.Append(UTLLRN_ModularRig::NamespaceSeparator);
						NewNamespace.Append(UTLLRN_ModularRig::NamespaceSeparator);
						
						TArray<FTLLRN_RigElementKey> Connectors = Controller->GetHierarchy()->GetKeysOfType<FTLLRN_RigConnectorElement>();
						TMap<FTLLRN_RigElementKey, ConnectionInfo> RenamedConnectors; // old key -> new key
						for (const FTLLRN_RigElementKey& Connector : Connectors)
						{
							FString OldConnectorName = Connector.Name.ToString();
							FString OldConnectorNamespace, ShortName;
							(void)UTLLRN_RigHierarchy::SplitNameSpace(OldConnectorName, &OldConnectorNamespace, &ShortName);
							OldConnectorNamespace.Append(UTLLRN_ModularRig::NamespaceSeparator);
							if (OldConnectorNamespace.StartsWith(OldNamespace))
							{
								ConnectionInfo& Info = RenamedConnectors.FindOrAdd(Connector);
								Info.NewPath = OldConnectorName.Replace(*OldNamespace, *NewNamespace);
								Info.Settings = CastChecked<FTLLRN_RigConnectorElement>(Controller->GetHierarchy()->FindChecked(Connector))->Settings;
								if (FTLLRN_RigElementKey* TargetKey = ConnectionMap.Find(Connector))
								{
									Info.TargetConnection = *TargetKey;
								}
							}
						}

						// Remove connectors
						for (TPair<FTLLRN_RigElementKey, ConnectionInfo>& Pair : RenamedConnectors)
						{
							Controller->RemoveElement(Pair.Key);
						}

						// Add connectors
						{
							FRigVMExtendedExecuteContext& Context = CDO->GetRigVMExtendedExecuteContext();
							FTLLRN_RigHierarchyExecuteContextBracket HierarchyContextGuard(Controller->GetHierarchy(), &Context);
							FTLLRN_ControlRigExecuteContext& PublicContext = Context.GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
							for (TPair<FTLLRN_RigElementKey, ConnectionInfo>& Pair : RenamedConnectors)
							{
								FString Namespace, ConnectorName;
								(void)UTLLRN_RigHierarchy::SplitNameSpace(Pair.Value.NewPath, &Namespace, &ConnectorName);
								Namespace.Append(UTLLRN_ModularRig::NamespaceSeparator);
								FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard TLLRN_RigModuleGuard(PublicContext, Namespace);
								const TGuardValue<bool> DisableErrors(Controller->bReportWarningsAndErrors, false);
								Controller->AddConnector(*ConnectorName, Pair.Value.Settings);
							}
						}

						UpdateConnectionMapFromModel();
						PropagateHierarchyFromBPToInstances();
					}
				}
			}
			break;
		}
		case ETLLRN_ModularRigNotification::ModuleRemoved:
		{
			if (InModule)
			{
				RefreshModuleConnectors(InModule);
				UpdateConnectionMapFromModel();
				UpdateModularDependencyDelegates();
			}
			break;
		}
		case ETLLRN_ModularRigNotification::ConnectionChanged:
		{
			Hierarchy->Modify();
			
			UpdateConnectionMapFromModel();
			HierarchyModifiedEvent.Broadcast(ETLLRN_RigHierarchyNotification::HierarchyReset, Hierarchy, nullptr);
			break;
		}
		case ETLLRN_ModularRigNotification::ModuleClassChanged:
		{
			if (InModule)
			{
				RefreshModuleConnectors(InModule);
				UpdateConnectionMapFromModel();
			}
			break;
		}
		case ETLLRN_ModularRigNotification::ModuleShortNameChanged:
		{
			bRecompile = false;
			break;
		}
		case ETLLRN_ModularRigNotification::ModuleConfigValueChanged:
		{
			bRecompile = false;
			PropagateModuleHierarchyFromBPToInstances();
			RequestConstructionOnAllModules();
			break;
		}
		case ETLLRN_ModularRigNotification::InteractionBracketOpened:
		{
			ModulesRecompilationBracket++;
			break;
		}
		case ETLLRN_ModularRigNotification::InteractionBracketClosed:
		case ETLLRN_ModularRigNotification::InteractionBracketCanceled:
		{
			ModulesRecompilationBracket--;
			break;
		}
		case ETLLRN_ModularRigNotification::ModuleSelected:
		case ETLLRN_ModularRigNotification::ModuleDeselected:
		{
			// don't do anything during selection
			return;
		}
		default:
		{
			break;
		}
	}

	if (bRecompile && ModulesRecompilationBracket == 0)
	{
		RecompileTLLRN_ModularRig();
	}
}

UTLLRN_ControlRigBlueprint::FControlValueScope::FControlValueScope(UTLLRN_ControlRigBlueprint* InBlueprint)
: Blueprint(InBlueprint)
{
#if WITH_EDITOR
	check(Blueprint);

	if (UTLLRN_ControlRig* CR = Cast<UTLLRN_ControlRig>(Blueprint->GetObjectBeingDebugged()))
	{
		TArray<FTLLRN_RigControlElement*> Controls = CR->AvailableControls();
		for (FTLLRN_RigControlElement* ControlElement : Controls)
		{
			ControlValues.Add(ControlElement->GetFName(), CR->GetControlValue(ControlElement->GetFName()));
		}
	}
#endif
}

UTLLRN_ControlRigBlueprint::FControlValueScope::~FControlValueScope()
{
#if WITH_EDITOR
	check(Blueprint);

	if (UTLLRN_ControlRig* CR = Cast<UTLLRN_ControlRig>(Blueprint->GetObjectBeingDebugged()))
	{
		for (const TPair<FName, FTLLRN_RigControlValue>& Pair : ControlValues)
		{
			if (CR->FindControl(Pair.Key))
			{
				CR->SetControlValue(Pair.Key, Pair.Value);
			}
		}
	}
#endif
}

#undef LOCTEXT_NAMESPACE



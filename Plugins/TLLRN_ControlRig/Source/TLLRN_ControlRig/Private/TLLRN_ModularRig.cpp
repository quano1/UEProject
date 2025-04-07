// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModularRig.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InteractionExecution.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "TLLRN_ControlRigComponent.h"
#include "RigVMCore/RigVMExecuteContext.h"
#include "Units/Modules/TLLRN_RigUnit_ConnectorExecution.h"

#define LOCTEXT_NAMESPACE "TLLRN_ModularRig"

////////////////////////////////////////////////////////////////////////////////
// FModuleInstanceHandle
////////////////////////////////////////////////////////////////////////////////

FModuleInstanceHandle::FModuleInstanceHandle(const UTLLRN_ModularRig* InTLLRN_ModularRig, const FString& InPath)
: TLLRN_ModularRig(const_cast<UTLLRN_ModularRig*>(InTLLRN_ModularRig))
, Path(InPath)
{
}

FModuleInstanceHandle::FModuleInstanceHandle(const UTLLRN_ModularRig* InTLLRN_ModularRig, const FTLLRN_RigModuleInstance* InModule)
: TLLRN_ModularRig(const_cast<UTLLRN_ModularRig*>(InTLLRN_ModularRig))
, Path(InModule->GetPath())
{
}

const FTLLRN_RigModuleInstance* FModuleInstanceHandle::Get() const
{
	if(const UTLLRN_ModularRig* ResolvedRig = TLLRN_ModularRig.Get())
	{
		return ResolvedRig->FindModule(Path);
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// UTLLRN_ModularRig
////////////////////////////////////////////////////////////////////////////////

UTLLRN_ModularRig::UTLLRN_ModularRig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectsReinstanced.AddUObject(this, &UTLLRN_ModularRig::OnObjectsReplaced);
#endif
}

void UTLLRN_ModularRig::BeginDestroy()
{
	Super::BeginDestroy();
	
#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectsReinstanced.RemoveAll(this);
#endif
}

FString FTLLRN_RigModuleInstance::GetShortName() const
{
	if(const FTLLRN_RigModuleReference* ModuleReference = GetModuleReference())
	{
		const FString ShortName = ModuleReference->GetShortName();
		if(!ShortName.IsEmpty())
		{
			return ShortName;
		}
	}
	return Name.ToString();
}

FString FTLLRN_RigModuleInstance::GetPath() const
{
	if (!IsRootModule())
	{
		return UTLLRN_RigHierarchy::JoinNameSpace(ParentPath, Name.ToString()); 
	}
	return Name.ToString();
}

FString FTLLRN_RigModuleInstance::GetNamespace() const
{
	return FString::Printf(TEXT("%s:"), *GetPath());
}

UTLLRN_ControlRig* FTLLRN_RigModuleInstance::GetRig() const
{
	if(IsValid(RigPtr))
	{
		return RigPtr;
	}

	// reset the cache if it is not valid
	return RigPtr = nullptr;
}

void FTLLRN_RigModuleInstance::SetRig(UTLLRN_ControlRig* InRig)
{
	UTLLRN_ControlRig* PreviousRig = GetRig();
	if(PreviousRig && (PreviousRig != InRig))
	{
		UTLLRN_ModularRig::DiscardModuleRig(PreviousRig);
	}

	// update the cache
	RigPtr = InRig;
}

bool FTLLRN_RigModuleInstance::ContainsRig(const UTLLRN_ControlRig* InRig) const
{
	if(InRig == nullptr)
	{
		return false;
	}
	if(RigPtr == InRig)
	{
		return true;
	}
	return false;
}

const FTLLRN_RigModuleReference* FTLLRN_RigModuleInstance::GetModuleReference() const
{
	if(const UTLLRN_ControlRig* Rig = GetRig())
	{
		if(const UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(Rig->GetParentRig()))
		{
			const FTLLRN_ModularRigModel& Model = TLLRN_ModularRig->GetTLLRN_ModularRigModel();
			return Model.FindModule(GetPath());
		}
	}
	return nullptr;
}

const FTLLRN_RigModuleInstance* FTLLRN_RigModuleInstance::GetParentModule() const
{
	if(ParentPath.IsEmpty())
	{
		return this;
	}
	if(const UTLLRN_ControlRig* Rig = GetRig())
	{
		if(const UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(Rig->GetParentRig()))
		{
			return TLLRN_ModularRig->FindModule(ParentPath);
		}
	}
	return nullptr;
}

const FTLLRN_RigModuleInstance* FTLLRN_RigModuleInstance::GetRootModule() const
{
	if(ParentPath.IsEmpty())
	{
		return this;
	}
	if(const UTLLRN_ControlRig* Rig = GetRig())
	{
		if(const UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(Rig->GetParentRig()))
		{
			FString RootPath = ParentPath;
			(void)UTLLRN_RigHierarchy::SplitNameSpace(ParentPath, &RootPath, nullptr, false);
			return TLLRN_ModularRig->FindModule(RootPath);
		}
	}
	return nullptr;
}

const FTLLRN_RigConnectorElement* FTLLRN_RigModuleInstance::FindPrimaryConnector() const
{
	if(PrimaryConnector)
	{
		return PrimaryConnector;
	}
	
	if(const UTLLRN_ControlRig* Rig = GetRig())
	{
		if(const UTLLRN_RigHierarchy* Hierarchy = Rig->GetHierarchy())
		{
			const FString MyModulePath = GetPath();
			const TArray<FTLLRN_RigConnectorElement*> AllConnectors = Hierarchy->GetConnectors();
			for(const FTLLRN_RigConnectorElement* Connector : AllConnectors)
			{
				if(Connector->IsPrimary())
				{
					const FString ModulePath = Hierarchy->GetModulePath(Connector->GetKey());
					if(!ModulePath.IsEmpty())
					{
						if(ModulePath.Equals(MyModulePath, ESearchCase::IgnoreCase))
						{
							PrimaryConnector = Connector;
							return PrimaryConnector;
						}
					}
				}
			}
		}
	}
	return nullptr;
}

TArray<const FTLLRN_RigConnectorElement*> FTLLRN_RigModuleInstance::FindConnectors() const
{
	TArray<const FTLLRN_RigConnectorElement*> Connectors;
	if(const UTLLRN_ControlRig* Rig = GetRig())
	{
		if(const UTLLRN_RigHierarchy* Hierarchy = Rig->GetHierarchy())
		{
			const FString MyModulePath = GetPath();
			const TArray<FTLLRN_RigConnectorElement*> AllConnectors = Hierarchy->GetConnectors();
			for(const FTLLRN_RigConnectorElement* Connector : AllConnectors)
			{
				const FString ModulePath = Hierarchy->GetModulePath(Connector->GetKey());
				if(!ModulePath.IsEmpty())
				{
					if(ModulePath.Equals(MyModulePath, ESearchCase::IgnoreCase))
					{
						Connectors.Add(Connector);
					}
				}
			}
		}
	}
	return Connectors;
}

bool FTLLRN_RigModuleInstance::IsRootModule() const
{
	return ParentPath.IsEmpty();
}

void UTLLRN_ModularRig::PostInitProperties()
{
	Super::PostInitProperties();

	TLLRN_ModularRigModel.UpdateCachedChildren();
	TLLRN_ModularRigModel.Connections.UpdateFromConnectionList();
	UpdateSupportedEvents();
}

void UTLLRN_ModularRig::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsLoading())
	{
		TLLRN_ModularRigModel.UpdateCachedChildren();
		TLLRN_ModularRigModel.Connections.UpdateFromConnectionList();
	}
}

void UTLLRN_ModularRig::PostLoad()
{
	Super::PostLoad();
	TLLRN_ModularRigModel.UpdateCachedChildren();
	TLLRN_ModularRigModel.Connections.UpdateFromConnectionList();
	ResetShortestDisplayPathCache();
}

void UTLLRN_ModularRig::InitializeVMs(bool bRequestInit)
{
	URigVMHost::Initialize(bRequestInit);
	ForEachModule([bRequestInit](const FTLLRN_RigModuleInstance* Module) -> bool
	{
		if (UTLLRN_ControlRig* ModuleRig = Module->GetRig())
		{
			ModuleRig->InitializeVMs(bRequestInit);
		}
		return true;
	});
}

bool UTLLRN_ModularRig::InitializeVMs(const FName& InEventName)
{
	URigVMHost::InitializeVM(InEventName);
	UpdateModuleHierarchyFromCDO();

	ForEachModule([InEventName](const FTLLRN_RigModuleInstance* Module) -> bool
	{
		if (UTLLRN_ControlRig* ModuleRig = Module->GetRig())
		{
			ModuleRig->InitializeVMs(InEventName);
		}
		return true;
	});
	return true;
}

void UTLLRN_ModularRig::InitializeFromCDO()
{
	Super::InitializeFromCDO();
	UpdateModuleHierarchyFromCDO();
}

void UTLLRN_ModularRig::UpdateModuleHierarchyFromCDO()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// keep the previous rigs around
		check(PreviousModuleRigs.IsEmpty());
		for (const FTLLRN_RigModuleInstance& Module : Modules)
        {
			if(UTLLRN_ControlRig* ModuleRig = Module.GetRig())
			{
				if(IsValid(ModuleRig))
				{
					PreviousModuleRigs.Add(Module.GetPath(), ModuleRig);
				}
			}
        }

		// don't destroy the rigs when resetting
		ResetModules(false);

		// the CDO owns the model - when we ask for the model we'll always
		// get the model from the CDO. we'll now add UObject module instances
		// for each module (data only) reference in the model.
		// Note: The CDO does not contain any UObject module instances itself.
		const FTLLRN_ModularRigModel& Model = GetTLLRN_ModularRigModel();
		Model.ForEachModule([this, Model](const FTLLRN_RigModuleReference* InModuleReference) -> bool
		{
			check(InModuleReference);
			if (IsInGameThread() && !InModuleReference->Class.IsValid())
			{
				(void)InModuleReference->Class.LoadSynchronous();
			}
			if (InModuleReference->Class.IsValid())
			{
				(void)AddModuleInstance(
					InModuleReference->Name,
					InModuleReference->Class.Get(),
					FindModule(InModuleReference->ParentPath),
					Model.Connections.GetModuleConnectionMap(InModuleReference->GetPath()),
					InModuleReference->ConfigValues);
			}

			// continue to the next module
			return true;
		});

		// discard any remaining rigs
		for(const TPair<FString, UTLLRN_ControlRig*>& Pair : PreviousModuleRigs)
		{
			DiscardModuleRig(Pair.Value);
		}
		PreviousModuleRigs.Reset();

		// update the module variable bindings now - since for this all
		// modules have to exist first
		ForEachModule([this, Model](const FTLLRN_RigModuleInstance* Module) -> bool
		{
			if(const FTLLRN_RigModuleReference* ModuleReference = Model.FindModule(Module->GetPath()))
			{
				(void)SetModuleVariableBindings(ModuleReference->GetPath(), ModuleReference->Bindings);
			}
			if(UTLLRN_ControlRig* ModuleRig = Module->GetRig())
			{
				ModuleRig->Initialize();
			}
			return true;
		});

		UpdateCachedChildren();
		UpdateSupportedEvents();
	}
}

bool UTLLRN_ModularRig::Execute_Internal(const FName& InEventName)
{
	if (VM)
	{
		FRigVMExtendedExecuteContext& TLLRN_ModularRigContext = GetRigVMExtendedExecuteContext();
		const FTLLRN_ControlRigExecuteContext& PublicContext = TLLRN_ModularRigContext.GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
		const FTLLRN_RigUnitContext& UnitContext = PublicContext.UnitContext;
		const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

		ForEachModule([&InEventName, this, Hierarchy, UnitContext](FTLLRN_RigModuleInstance* Module) -> bool
		{
			if (const UTLLRN_ControlRig* ModuleRig = Module->GetRig())
			{
				if (!ModuleRig->SupportsEvent(InEventName))
				{
					return true;
				}

				// Only emit interaction event on this module if any of the interaction elements
				// belong to the module's namespace
				if (InEventName == FTLLRN_RigUnit_InteractionExecution::EventName)
				{
					const FString ModuleNamespace = Module->GetNamespace();
					const bool bIsInteracting = UnitContext.ElementsBeingInteracted.ContainsByPredicate(
						[ModuleNamespace, Hierarchy](const FTLLRN_RigElementKey& InteractionElement)
						{
							return ModuleNamespace == Hierarchy->GetNameSpace(InteractionElement);
						});
					if (!bIsInteracting)
					{
						return true;
					}
				}

				ExecutionQueue.Add(FTLLRN_RigModuleExecutionElement(Module, InEventName));
			}
			return true;
		});

		ExecuteQueue();
		return true;
	}
	return false;
}

void UTLLRN_ModularRig::Evaluate_AnyThread()
{
	ResetExecutionQueue();
	Super::Evaluate_AnyThread();
}

bool UTLLRN_ModularRig::SupportsEvent(const FName& InEventName) const
{
	return GetSupportedEvents().Contains(InEventName);
}

const TArray<FName>& UTLLRN_ModularRig::GetSupportedEvents() const
{
	if (SupportedEvents.IsEmpty())
	{
		UpdateSupportedEvents();
	}
	return SupportedEvents;
}

const FTLLRN_ModularRigSettings& UTLLRN_ModularRig::GetTLLRN_ModularRigSettings() const
{
	if(HasAnyFlags(RF_ClassDefaultObject))
	{
		return TLLRN_ModularRigSettings;
	}
	if (const UTLLRN_ModularRig* CDO = Cast<UTLLRN_ModularRig>(GetClass()->GetDefaultObject()))
	{
		return CDO->GetTLLRN_ModularRigSettings();
	}
	return TLLRN_ModularRigSettings;
}

void UTLLRN_ModularRig::ExecuteQueue()
{
	FRigVMExtendedExecuteContext& Context = GetRigVMExtendedExecuteContext();
	FTLLRN_ControlRigExecuteContext& PublicContext = Context.GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

#if WITH_EDITOR
	TMap<FTLLRN_RigModuleInstance*, FFirstEntryEventGuard> FirstModuleEvent;
#endif
	
	while(ExecutionQueue.IsValidIndex(ExecutionQueueFront))
	{
		FTLLRN_RigModuleExecutionElement& ExecutionElement = ExecutionQueue[ExecutionQueueFront];
		if (FTLLRN_RigModuleInstance* ModuleInstance = ExecutionElement.ModuleInstance)
		{
			if (UTLLRN_ControlRig* ModuleRig = ModuleInstance->GetRig())
			{
				if (!ModuleRig->SupportsEvent(ExecutionElement.EventName))
				{
					ExecutionQueueFront++;
					continue;
				}

				// Make sure the hierarchy has the correct element redirector from this module rig
				FTLLRN_RigHierarchyRedirectorGuard ElementRedirectorGuard(ModuleRig);

				FRigVMExtendedExecuteContext& RigExtendedExecuteContext= ModuleRig->GetRigVMExtendedExecuteContext();

				// Make sure the hierarchy has the correct execute context with the rig module namespace
				FTLLRN_RigHierarchyExecuteContextBracket ExecuteContextBracket(Hierarchy, &RigExtendedExecuteContext);

				FTLLRN_ControlRigExecuteContext& RigPublicContext = RigExtendedExecuteContext.GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
				FTLLRN_RigUnitContext& TLLRN_RigUnitContext = RigPublicContext.UnitContext;
				TLLRN_RigUnitContext = PublicContext.UnitContext;

				// forward important context info to each module
				RigPublicContext.SetDrawInterface(PublicContext.GetDrawInterface());
				RigPublicContext.SetDrawContainer(PublicContext.GetDrawContainer());
				RigPublicContext.TLLRN_RigModuleInstance = ExecutionElement.ModuleInstance;
				RigPublicContext.SetAbsoluteTime(PublicContext.GetAbsoluteTime());
				RigPublicContext.SetDeltaTime(PublicContext.GetDeltaTime());
				RigPublicContext.SetWorld(PublicContext.GetWorld());
				RigPublicContext.SetOwningActor(PublicContext.GetOwningActor());
				RigPublicContext.SetOwningComponent(PublicContext.GetOwningComponent());
#if WITH_EDITOR
				RigPublicContext.SetLog(PublicContext.GetLog());
#endif
				RigPublicContext.SetFramesPerSecond(PublicContext.GetFramesPerSecond());
#if WITH_EDITOR
				RigPublicContext.SetHostBeingDebugged(bIsBeingDebugged);
#endif
				RigPublicContext.SetToWorldSpaceTransform(PublicContext.GetToWorldSpaceTransform());
				RigPublicContext.OnAddShapeLibraryDelegate = PublicContext.OnAddShapeLibraryDelegate;
				RigPublicContext.OnShapeExistsDelegate = PublicContext.OnShapeExistsDelegate;
				RigPublicContext.RuntimeSettings = PublicContext.RuntimeSettings;

#if WITH_EDITOR
				if (!FirstModuleEvent.Contains(ModuleInstance))
				{
					//ModuleRig->InstructionVisitInfo.FirstEntryEventInQueue = ExecutionElement.EventName;
					FirstModuleEvent.Add(ModuleInstance, FFirstEntryEventGuard(&ModuleRig->InstructionVisitInfo, ExecutionElement.EventName));
				}
#endif

				// re-initialize the module in case only the VM side got recompiled.
				// this happens when the user relies on auto recompilation when editing the
				// module (dependency) graph - by changing a value, add / remove nodes or links.
				if(ModuleRig->IsInitRequired())
				{
					const TGuardValue<float> AbsoluteTimeGuard(ModuleRig->AbsoluteTime, ModuleRig->AbsoluteTime);
					const TGuardValue<float> DeltaTimeGuard(ModuleRig->DeltaTime, ModuleRig->DeltaTime);
					if(!ModuleRig->InitializeVM(ExecutionElement.EventName))
					{
						ExecutionQueueFront++;
						continue;
					}

					// put the variable defaults back
					if(const FTLLRN_RigModuleReference* ModuleReference = GetTLLRN_ModularRigModel().FindModule(ExecutionElement.ModulePath))
					{
						for (const TPair<FName, FString>& Variable : ModuleReference->ConfigValues)
						{
							ModuleRig->SetVariableFromString(Variable.Key, Variable.Value);
						}
					}
				}

				// Update the interaction elements to show only the ones belonging to this module
				const FString ModuleNamespace = FString::Printf(TEXT("%s:"), *ExecutionElement.ModulePath);
				TLLRN_RigUnitContext.ElementsBeingInteracted = TLLRN_RigUnitContext.ElementsBeingInteracted.FilterByPredicate(
					[ModuleNamespace, Hierarchy](const FTLLRN_RigElementKey& Key)
				{
					return ModuleNamespace == Hierarchy->GetNameSpace(Key);
				});
				TLLRN_RigUnitContext.InteractionType = TLLRN_RigUnitContext.ElementsBeingInteracted.IsEmpty() ?
					(uint8) ETLLRN_ControlRigInteractionType::None
					: TLLRN_RigUnitContext.InteractionType;

				// Make sure the module's rig has the corrct user data
				// The rig will combine the user data of the
				// - skeleton
				// - skeletalmesh
				// - SkeletalMeshComponent
				// - default control rig module
				// - outer modular rig
				// - external variables
				{
					RigPublicContext.AssetUserData.Reset();
					if(const TArray<UAssetUserData*>* TLLRN_ControlRigUserDataArray = ModuleRig->GetAssetUserDataArray())
					{
						for(const UAssetUserData* TLLRN_ControlRigUserData : *TLLRN_ControlRigUserDataArray)
						{
							RigPublicContext.AssetUserData.Add(TLLRN_ControlRigUserData);
						}
					}
					RigPublicContext.AssetUserData.Remove(nullptr);
				}

				// Copy variable bindings
				for (TPair<FName, FRigVMExternalVariable>& Pair : ExecutionElement.ModuleInstance->VariableBindings)
				{
					const FRigVMExternalVariable TargetVariable = ExecutionElement.ModuleInstance->GetRig()->GetPublicVariableByName(Pair.Key);
					if(ensure(TargetVariable.Property))
					{
						if (RigVMTypeUtils::AreCompatible(Pair.Value.Property, TargetVariable.Property))
						{
							Pair.Value.Property->CopyCompleteValue(TargetVariable.Memory, Pair.Value.Memory);
						}
					}
				}
			
				ModuleRig->Execute_Internal(ExecutionElement.EventName);
				ExecutionElement.bExecuted = true;

				// Copy result of Connection event to the TLLRN_ModularRig's unit context
				if (ExecutionElement.EventName == FTLLRN_RigUnit_ConnectorExecution::EventName)
				{
					PublicContext.UnitContext.ConnectionResolve = RigPublicContext.UnitContext.ConnectionResolve;
				}
			}
		}
		
		ExecutionQueueFront++;
	}
}

void UTLLRN_ModularRig::ResetExecutionQueue()
{
	ExecutionQueue.Reset();
	ExecutionQueueFront = 0;
}

void UTLLRN_ModularRig::OnObjectsReplaced(const TMap<UObject*, UObject*>& OldToNewInstanceMap)
{
	if(Modules.IsEmpty())
	{
		return;
	}
	
	bool bPerformedChange = false;
	for(const TPair<UObject*, UObject*>& Pair : OldToNewInstanceMap)
	{
		UObject* NewObject = Pair.Value;
		if((NewObject == nullptr) || (NewObject->GetOuter() != this) || !NewObject->IsA<UTLLRN_ControlRig>())
		{
			continue;
		}

		UTLLRN_ControlRig* NewRig = CastChecked<UTLLRN_ControlRig>(NewObject);

		// relying on GetFName since URigVMHost is overloading GetName()
		const FString& Path = NewRig->GetFName().ToString();

		// if we find a matching module update it.
		// FTLLRN_RigModuleInstance::SetRig takes care of disregarding the previous module instance.
		if(FTLLRN_RigModuleInstance* Module = const_cast<FTLLRN_RigModuleInstance*>(FindModule(Path)))
		{
			Module->SetRig(NewRig);
			NewRig->bCopyHierarchyBeforeConstruction = false;
			NewRig->SetDynamicHierarchy(GetHierarchy());
			NewRig->Initialize(true);
			bPerformedChange = true;
		}
	}

	if(bPerformedChange)
	{
		UpdateSupportedEvents();
		RequestInit();
	}
}

void UTLLRN_ModularRig::ResetModules(bool bDestroyModuleRigs)
{
	for (FTLLRN_RigModuleInstance& Module : Modules)
	{
		Module.CachedChildren.Reset();

		if(bDestroyModuleRigs)
		{
			if (const UTLLRN_ControlRig* ModuleRig = Module.GetRig())
			{
				check(ModuleRig->GetOuter() == this);
				// takes care of renaming / moving the rig to the transient package
				Module.SetRig(nullptr);
			}
		}
	}
	
	RootModules.Reset();
	Modules.Reset();
	SupportedEvents.Reset();
	ResetShortestDisplayPathCache();
}

const FTLLRN_ModularRigModel& UTLLRN_ModularRig::GetTLLRN_ModularRigModel() const
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		const UTLLRN_ModularRig* CDO = GetClass()->GetDefaultObject<UTLLRN_ModularRig>();
		return CDO->GetTLLRN_ModularRigModel();
	}
	return TLLRN_ModularRigModel;
}

void UTLLRN_ModularRig::UpdateCachedChildren()
{
	TMap<FString, FTLLRN_RigModuleInstance*> PathToModule;
	for (FTLLRN_RigModuleInstance& Module : Modules)
	{
		Module.CachedChildren.Reset();
		PathToModule.Add(Module.GetPath(), &Module);
	}
	
	RootModules.Reset();
	for (FTLLRN_RigModuleInstance& Module : Modules)
	{
		if (Module.IsRootModule())
		{
			RootModules.Add(&Module);
		}
		else
		{
			if (FTLLRN_RigModuleInstance** ParentModule = PathToModule.Find(Module.ParentPath))
			{
				(*ParentModule)->CachedChildren.Add(&Module);
			}
		}
	}
}

void UTLLRN_ModularRig::UpdateSupportedEvents() const
{
	SupportedEvents.Reset();
	TLLRN_ModularRigModel.ForEachModule([this](const FTLLRN_RigModuleReference* Module) -> bool
	{
		if (Module->Class.IsValid())
		{
			if (UTLLRN_ControlRig* CDO = Module->Class->GetDefaultObject<UTLLRN_ControlRig>())
			{
				TArray<FName> Events = CDO->GetSupportedEvents();
				for (const FName& Event : Events)
				{
					SupportedEvents.AddUnique(Event);
				}
			}
		}
		return true;
	});
}

void UTLLRN_ModularRig::RunPostConstructionEvent()
{
	RecomputeShortestDisplayPathCache();
	Super::RunPostConstructionEvent();
}

FTLLRN_RigModuleInstance* UTLLRN_ModularRig::AddModuleInstance(const FName& InModuleName, TSubclassOf<UTLLRN_ControlRig> InModuleClass, const FTLLRN_RigModuleInstance* InParent,
                                                   const TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey>& InConnectionMap, const TMap<FName, FString>& InVariableDefaultValues) 
{
	// Make sure there are no name clashes
	if (InParent)
	{
		for (const FTLLRN_RigModuleInstance* Child : InParent->CachedChildren)
		{
			if (Child->Name == InModuleName)
			{
				return nullptr;
			}
		}
	}
	else
	{
		for (const FTLLRN_RigModuleInstance* RootModule : RootModules)
		{
			if (RootModule->Name == InModuleName)
			{
				return nullptr;
			}
		}
	}

	// For now, lets only allow rig modules
	if (!InModuleClass->GetDefaultObject<UTLLRN_ControlRig>()->IsTLLRN_RigModule())
	{
		return nullptr;
	}

	FTLLRN_RigModuleInstance& NewModule = Modules.Add_GetRef(FTLLRN_RigModuleInstance());
	NewModule.Name = InModuleName;
	if (InParent)
	{
		NewModule.ParentPath = InParent->GetPath();
	}
	const FString Name = NewModule.GetPath();

	UTLLRN_ControlRig* NewModuleRig = nullptr;

	// reuse existing module rig instances first
	if(UTLLRN_ControlRig** ExistingModuleRigPtr = PreviousModuleRigs.Find(Name))
	{
		if(UTLLRN_ControlRig* ExistingModuleRig = *ExistingModuleRigPtr)
		{
			// again relying on GetFName since RigVMHost overloads GetName
			if(ExistingModuleRig->GetFName().ToString().Equals(Name) && ExistingModuleRig->GetClass() == InModuleClass)
			{
				NewModuleRig = ExistingModuleRig;
			}
			else
			{
				DiscardModuleRig(ExistingModuleRig);
			}
			PreviousModuleRigs.Remove(Name);
		}
	}

	if(NewModuleRig == nullptr)
	{
		NewModuleRig = NewObject<UTLLRN_ControlRig>(this, InModuleClass, *Name);
	}
	
	NewModule.SetRig(NewModuleRig);

	UpdateCachedChildren();
	ResetShortestDisplayPathCache();
	for (const FName& EventName : NewModule.GetRig()->GetSupportedEvents())
	{
		SupportedEvents.AddUnique(EventName);
	}

	// Configure module
	{
		UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
		FRigVMExtendedExecuteContext& ModuleContext = NewModuleRig->GetRigVMExtendedExecuteContext();
		FTLLRN_ControlRigExecuteContext& ModulePublicContext = ModuleContext.GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
		NewModuleRig->RequestInit();
		NewModuleRig->bCopyHierarchyBeforeConstruction = false;
		NewModuleRig->SetDynamicHierarchy(Hierarchy);
		ModulePublicContext.Hierarchy = Hierarchy;
		ModulePublicContext.TLLRN_ControlRig = this;
		ModulePublicContext.TLLRN_RigModuleNameSpace = NewModuleRig->GetTLLRN_RigModuleNameSpace();
		ModulePublicContext.TLLRN_RigModuleNameSpaceHash = GetTypeHash(ModulePublicContext.TLLRN_RigModuleNameSpace);
		NewModuleRig->SetElementKeyRedirector(FTLLRN_RigElementKeyRedirector(InConnectionMap, Hierarchy));

		for (const TPair<FName, FString>& Variable : InVariableDefaultValues )
		{
			NewModule.GetRig()->SetVariableFromString(Variable.Key, Variable.Value);
		}
	}

	return &NewModule;
}

bool UTLLRN_ModularRig::SetModuleVariableBindings(const FString& InModulePath, const TMap<FName, FString>& InVariableBindings)
{
	if(FTLLRN_RigModuleInstance* Module = const_cast<FTLLRN_RigModuleInstance*>(FindModule(InModulePath)))
	{
		Module->VariableBindings.Reset();
		
		for (const TPair<FName, FString>& Pair : InVariableBindings)
		{
			FString SourceModulePath, SourceVariableName = Pair.Value;
			(void)UTLLRN_RigHierarchy::SplitNameSpace(Pair.Value, &SourceModulePath, &SourceVariableName);
			FRigVMExternalVariable SourceVariable;
			if (SourceModulePath.IsEmpty())
			{
				if (const FProperty* Property = GetClass()->FindPropertyByName(*SourceVariableName))
				{
					SourceVariable = FRigVMExternalVariable::Make(Property, (UObject*)this);
				}
			}
			else if(const FTLLRN_RigModuleInstance* SourceModule = FindModule(SourceModulePath))
			{
				SourceVariable = SourceModule->GetRig()->GetPublicVariableByName(*SourceVariableName);
			}

			if(SourceVariable.Property == nullptr)
			{
				// todo: report error
				return false;
			}
			
			SourceVariable.Name = *Pair.Value; // Adapt the name of the variable to contain the full path
			Module->VariableBindings.Add(Pair.Key, SourceVariable);
		}
		return true;
	}
	return false;
}

void UTLLRN_ModularRig::DiscardModuleRig(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if(InTLLRN_ControlRig)
	{
		// rename the previous rig.
		// GC will pick it up eventually - since we won't have any
		// owning pointers to it anymore.
		InTLLRN_ControlRig->Rename(nullptr, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
		InTLLRN_ControlRig->MarkAsGarbage();
	}
}

void UTLLRN_ModularRig::ResetShortestDisplayPathCache() const
{
	ElementKeyToShortestDisplayPath.Reset();
}

void UTLLRN_ModularRig::RecomputeShortestDisplayPathCache() const
{
	ResetShortestDisplayPathCache();
	
	const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	check(Hierarchy);

	TArray<FTLLRN_RigElementKey> AllKeys = Hierarchy->GetAllKeys();

	// add all of the keys from the connections. we may be in a situation where
	// the keys in the connections are actually referring to invalid / unloaded elements
	// but we still want to show pretty short names.
	for(const FTLLRN_ModularRigSingleConnection& Connection : TLLRN_ModularRigModel.Connections)
	{
		AllKeys.AddUnique(Connection.Connector);
		AllKeys.AddUnique(Connection.Target);
	}

	auto GetNameForElement = [Hierarchy](const FTLLRN_RigElementKey& InElementKey)
	{
		const FName DesiredName = Hierarchy->GetNameMetadata(InElementKey, UTLLRN_RigHierarchy::DesiredNameMetadataName, NAME_None);
		if(!DesiredName.IsNone())
		{
			return DesiredName;
		}
		return InElementKey.Name;
	};
	
	TMap<FName, bool> IsNameUniqueInHierarchy;
	for(const FTLLRN_RigElementKey& Key : AllKeys)
	{
		auto UpdateNameUniqueness = [](const FName& InName, TMap<FName, bool>& UniqueMap)
		{
			if(bool* Unique = UniqueMap.Find(InName))
			{
				*Unique = false;
			}
			else
			{
				UniqueMap.Add(InName, true);
			}
		};
		UpdateNameUniqueness(GetNameForElement(Key), IsNameUniqueInHierarchy);
	}

	const FTLLRN_ModularRigModel& Model = GetTLLRN_ModularRigModel();
	for(const FTLLRN_RigElementKey& Key : AllKeys)
	{
		const FName Name = GetNameForElement(Key);
		const FName ModulePath = Hierarchy->GetModulePathFName(Key);
		
		if(!ModulePath.IsNone())
		{
			if(const FTLLRN_RigModuleReference* Module = Model.FindModule(ModulePath.ToString()))
			{
				const FString ModulePathString = ModulePath.ToString();
				const FString ModulePathPrefix = ModulePath.ToString() + NamespaceSeparator;
				FString NameString = Name.ToString();
				if(NameString.StartsWith(ModulePathPrefix))
				{
					NameString = NameString.Mid(ModulePathPrefix.Len());
				}
				const FString ModuleShortName = Module->GetShortName();
				const FString NameSpacedName = UTLLRN_RigHierarchy::JoinNameSpace(ModuleShortName, NameString);
				ElementKeyToShortestDisplayPath.Add(Key, { NameSpacedName, IsNameUniqueInHierarchy.FindChecked(Name) ? NameString : NameSpacedName });
				continue;
			}
		}

		if(IsNameUniqueInHierarchy.FindChecked(Name))
		{
			const FString NameString = Name.ToString();
			ElementKeyToShortestDisplayPath.Add(Key, { NameString, NameString});
		}
		else
		{
			const FString NameString = Key.ToString();
			ElementKeyToShortestDisplayPath.Add(Key, { Name.ToString(), Name.ToString()});
		}
	}
}

const FTLLRN_RigModuleInstance* UTLLRN_ModularRig::FindModule(const FString& InPath) const
{
	if(InPath.EndsWith(NamespaceSeparator))
	{
		return FindModule(InPath.Left(InPath.Len() - 1));
	}
	return Modules.FindByPredicate([InPath](const FTLLRN_RigModuleInstance& Module)
	{
		return Module.GetPath() == InPath;
	});
}

const FTLLRN_RigModuleInstance* UTLLRN_ModularRig::FindModule(const UTLLRN_ControlRig* InModuleInstance) const
{
	const FTLLRN_RigModuleInstance* FoundModule = nullptr;
	ForEachModule([InModuleInstance, &FoundModule](const FTLLRN_RigModuleInstance* Module) -> bool
	{
		if(Module->GetRig() == InModuleInstance)
		{
			FoundModule = Module;
			// don't continue ForEachModule
			return false;
		}
		return true;
	});

	return FoundModule;
}

const FTLLRN_RigModuleInstance* UTLLRN_ModularRig::FindModule(const FTLLRN_RigBaseElement* InElement) const
{
	if(InElement)
	{
		return FindModule(InElement->GetKey());
	}
	return nullptr;
}

const FTLLRN_RigModuleInstance* UTLLRN_ModularRig::FindModule(const FTLLRN_RigElementKey& InElementKey) const
{
	if(const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		const FString ModulePath = Hierarchy->GetModulePath(InElementKey);
		if(!ModulePath.IsEmpty())
		{
			return FindModule(ModulePath);
		}
	}
	return nullptr;
}

FString UTLLRN_ModularRig::GetParentPath(const FString& InPath) const
{
	if (const FTLLRN_RigModuleInstance* Element = FindModule(InPath))
	{
		return Element->ParentPath;
	}
	return FString();
}

FString UTLLRN_ModularRig::GetShortestDisplayPathForElement(const FTLLRN_RigElementKey& InElementKey, bool bAlwaysShowNameSpace) const
{
	if(const TTuple<FString, FString>* ShortestDisplayPaths = ElementKeyToShortestDisplayPath.Find(InElementKey))
	{
		return bAlwaysShowNameSpace ? ShortestDisplayPaths->Get<0>() : ShortestDisplayPaths->Get<1>();
	}
	return FString();
}

void UTLLRN_ModularRig::ForEachModule(TFunctionRef<bool(FTLLRN_RigModuleInstance*)> PerModuleFunction)
{
	TArray<FTLLRN_RigModuleInstance*> ModuleInstances = RootModules;
	for (int32 ModuleIndex = 0; ModuleIndex < ModuleInstances.Num(); ++ModuleIndex)
	{
		if (!PerModuleFunction(ModuleInstances[ModuleIndex]))
		{
			break;
		}
		ModuleInstances.Append(ModuleInstances[ModuleIndex]->CachedChildren);
	}
}

void UTLLRN_ModularRig::ForEachModule(TFunctionRef<bool(const FTLLRN_RigModuleInstance*)> PerModuleFunction) const
{
	TArray<FTLLRN_RigModuleInstance*> ModuleInstances = RootModules;
	for (int32 ModuleIndex = 0; ModuleIndex < ModuleInstances.Num(); ++ModuleIndex)
	{
		if (!PerModuleFunction(ModuleInstances[ModuleIndex]))
		{
			break;
		}
		ModuleInstances.Append(ModuleInstances[ModuleIndex]->CachedChildren);
	}
}

void UTLLRN_ModularRig::ExecuteConnectorEvent(const FTLLRN_RigElementKey& InConnector, const FTLLRN_RigModuleInstance* InModuleInstance, const FTLLRN_RigElementKeyRedirector* InRedirector, TArray<FTLLRN_RigElementResolveResult>& InOutCandidates)
{
	if (!InModuleInstance)
	{
		InOutCandidates.Reset();
		return;
	}

	if (!InRedirector)
	{
		InOutCandidates.Reset();
		return;
	}
	
	FTLLRN_RigModuleInstance* Module = Modules.FindByPredicate([InModuleInstance](FTLLRN_RigModuleInstance& Instance)
	{
		return &Instance == InModuleInstance;
	});
	if (!Module)
	{
		InOutCandidates.Reset();
		return;
	}

	TArray<FTLLRN_RigElementResolveResult> Candidates = InOutCandidates;
	
	FTLLRN_ControlRigExecuteContext& PublicContext = GetRigVMExtendedExecuteContext().GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();

	FString ShortConnectorName = InConnector.Name.ToString();
	ShortConnectorName.RemoveFromStart(Module->GetNamespace());
	TGuardValue<FTLLRN_RigElementKey> ConnectorGuard(PublicContext.UnitContext.ConnectionResolve.Connector, FTLLRN_RigElementKey(*ShortConnectorName, InConnector.Type));
	TGuardValue<TArray<FTLLRN_RigElementResolveResult>> CandidatesGuard(PublicContext.UnitContext.ConnectionResolve.Matches, Candidates);
	TGuardValue<TArray<FTLLRN_RigElementResolveResult>> MatchesGuard(PublicContext.UnitContext.ConnectionResolve.Excluded, {});
	
	FTLLRN_RigModuleExecutionElement ExecutionElement(Module, FTLLRN_RigUnit_ConnectorExecution::EventName);
	TGuardValue<TArray<FTLLRN_RigModuleExecutionElement>> ExecutionGuard(ExecutionQueue, {ExecutionElement});
	TGuardValue<int32> ExecutionFrontGuard(ExecutionQueueFront, 0);
	TGuardValue<FTLLRN_RigElementKeyRedirector> RedirectorGuard(ElementKeyRedirector, *InRedirector);
	ExecuteQueue();

	InOutCandidates = PublicContext.UnitContext.ConnectionResolve.Matches;
}

#undef LOCTEXT_NAMESPACE

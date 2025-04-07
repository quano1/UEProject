// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModularTLLRN_RigController.h"

#include "TLLRN_ControlRig.h"
#include "TLLRN_ModularRig.h"
#include "TLLRN_ModularRigModel.h"
#include "TLLRN_ModularRigRuleManager.h"
#include "Misc/DefaultValueHelper.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ModularTLLRN_RigController)

#if WITH_EDITOR
#include "ScopedTransaction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#endif

UTLLRN_ModularTLLRN_RigController::UTLLRN_ModularTLLRN_RigController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Model(nullptr)
	, bSuspendNotifications(false)
	, bAutomaticReparenting(true)
{
}

FString UTLLRN_ModularTLLRN_RigController::AddModule(const FName& InModuleName, TSubclassOf<UTLLRN_ControlRig> InClass, const FString& InParentModulePath, bool bSetupUndo)
{
	if (!InClass)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Invalid InClass"));
		return FString();
	}

	UTLLRN_ControlRig* ClassDefaultObject = InClass->GetDefaultObject<UTLLRN_ControlRig>();
	if (!ClassDefaultObject->IsTLLRN_RigModule())
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Class %s is not a rig module"), *InClass->GetClassPathName().ToString());
		return FString();
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "AddModuleTransaction", "Add Module"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif 

	FTLLRN_RigModuleReference* NewModule = nullptr;
	if (InParentModulePath.IsEmpty())
	{
		for (FTLLRN_RigModuleReference* Module : Model->RootModules)
		{
			if (Module->Name.ToString() == InModuleName)
			{
				return FString();
			}
		}

		Model->Modules.Add(FTLLRN_RigModuleReference(InModuleName, InClass, FString()));
		NewModule = &Model->Modules.Last();
	}
	else if (FTLLRN_RigModuleReference* ParentModule = FindModule(InParentModulePath))
	{
		for (FTLLRN_RigModuleReference* Module : ParentModule->CachedChildren)
		{
			if (Module->Name.ToString() == InModuleName)
			{
				return FString();
			}
		}

		Model->Modules.Add(FTLLRN_RigModuleReference(InModuleName, InClass, ParentModule->GetPath()));
		NewModule = &Model->Modules.Last();
	}

	Model->UpdateCachedChildren();
	UpdateShortNames();

	if (!NewModule)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Error while creating module %s"), *InModuleName.ToString());
		return FString();
	}

	Notify(ETLLRN_ModularRigNotification::ModuleAdded, NewModule);

#if WITH_EDITOR
	TransactionPtr.Reset();
#endif

	return NewModule->GetPath();
}

FTLLRN_RigModuleReference* UTLLRN_ModularTLLRN_RigController::FindModule(const FString& InPath)
{
	return Model->FindModule(InPath);
}

const FTLLRN_RigModuleReference* UTLLRN_ModularTLLRN_RigController::FindModule(const FString& InPath) const
{
	return const_cast<UTLLRN_ModularTLLRN_RigController*>(this)->FindModule(InPath);
}

bool UTLLRN_ModularTLLRN_RigController::CanConnectConnectorToElement(const FTLLRN_RigElementKey& InConnectorKey, const FTLLRN_RigElementKey& InTargetKey, FText& OutErrorMessage)
{
	FString ConnectorModulePath, ConnectorName;
	if (!UTLLRN_RigHierarchy::SplitNameSpace(InConnectorKey.Name.ToString(), &ConnectorModulePath, &ConnectorName))
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Connector %s does not contain a namespace"), *InConnectorKey.ToString()));
		return false;
	}
	
	FTLLRN_RigModuleReference* Module = FindModule(ConnectorModulePath);
	if (!Module)
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Could not find module %s"), *ConnectorModulePath));
		return false;
	}

	UTLLRN_ControlRig* RigCDO = Module->Class->GetDefaultObject<UTLLRN_ControlRig>();
	if (!RigCDO)
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Invalid rig module class %s"), *Module->Class->GetPathName()));
		return false;
	}

	const FTLLRN_RigModuleConnector* ModuleConnector = RigCDO->GetTLLRN_RigModuleSettings().ExposedConnectors.FindByPredicate(
		[ConnectorName](FTLLRN_RigModuleConnector& Connector)
		{
			return Connector.Name == ConnectorName;
		});
	if (!ModuleConnector)
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Could not find connector %s in class %s"), *ConnectorName, *Module->Class->GetPathName()));
		return false;
	}

	if (!InTargetKey.IsValid())
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Invalid target %s in class %s"), *InTargetKey.ToString(), *Module->Class->GetPathName()));
		return false;
	}

	if (InTargetKey == InConnectorKey)
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Cannot resolve connector %s to itself in class %s"), *InTargetKey.ToString(), *Module->Class->GetPathName()));
		return false;
	}

	FTLLRN_RigElementKey CurrentTarget = Model->Connections.FindTargetFromConnector(InConnectorKey);
	if (CurrentTarget.IsValid() && InTargetKey == CurrentTarget)
	{
		return true; // Nothing to do
	}

	if (!ModuleConnector->IsPrimary())
	{
		const FTLLRN_RigModuleConnector* PrimaryModuleConnector = RigCDO->GetTLLRN_RigModuleSettings().ExposedConnectors.FindByPredicate(
		[](const FTLLRN_RigModuleConnector& Connector)
		{
			return Connector.IsPrimary();
		});

		const FString PrimaryConnectorPath = FString::Printf(TEXT("%s:%s"), *ConnectorModulePath, *PrimaryModuleConnector->Name);
		const FTLLRN_RigElementKey PrimaryConnectorKey(*PrimaryConnectorPath, ETLLRN_RigElementType::Connector);
		const FTLLRN_RigElementKey PrimaryTarget = Model->Connections.FindTargetFromConnector(PrimaryConnectorKey);
		if (!PrimaryTarget.IsValid())
		{
			OutErrorMessage = FText::FromString(FString::Printf(TEXT("Cannot resolve connector %s because primary connector is not resolved"), *InConnectorKey.ToString()));
			return false;
		}
	}

#if WITH_EDITOR
	UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter());

	// Make sure the connection is valid
	{
		UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(Blueprint->GetObjectBeingDebugged());
		if (!TLLRN_ModularRig)
		{
			OutErrorMessage = FText::FromString(FString::Printf(TEXT("Could not find debugged modular rig in %s"), *Blueprint->GetPathName()));
			return false;
		}
	
		UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy();
		if (!Hierarchy)
		{
			OutErrorMessage = FText::FromString(FString::Printf(TEXT("Could not find hierarchy in %s"), *TLLRN_ModularRig->GetPathName()));
			return false;
		}
	
		const FTLLRN_RigConnectorElement* Connector = Cast<FTLLRN_RigConnectorElement>(Hierarchy->Find(InConnectorKey));
		if (!Connector)
		{
			OutErrorMessage = FText::FromString(FString::Printf(TEXT("Could not find connector %s"), *InConnectorKey.ToString()));
			return false;
		}

		UTLLRN_ModularRigRuleManager* RuleManager = Hierarchy->GetRuleManager();
		if (!RuleManager)
		{
			OutErrorMessage = FText::FromString(FString::Printf(TEXT("Could not get rule manager")));
			return false;
		}

		const FTLLRN_RigModuleInstance* ModuleInstance = TLLRN_ModularRig->FindModule(Module->GetPath());
		FTLLRN_ModularRigResolveResult RuleResults = RuleManager->FindMatches(Connector, ModuleInstance, TLLRN_ModularRig->GetElementKeyRedirector());
		if (!RuleResults.ContainsMatch(InTargetKey))
		{
			OutErrorMessage = FText::FromString(FString::Printf(TEXT("The target %s is not a valid match for connector %s"), *InTargetKey.ToString(), *InConnectorKey.ToString()));
			return false;
		}
	}
#endif

	return true;
}

bool UTLLRN_ModularTLLRN_RigController::ConnectConnectorToElement(const FTLLRN_RigElementKey& InConnectorKey, const FTLLRN_RigElementKey& InTargetKey, bool bSetupUndo, bool bAutoResolveOtherConnectors, bool bCheckValidConnection)
{
	FText ErrorMessage;
	if (bCheckValidConnection && !CanConnectConnectorToElement(InConnectorKey, InTargetKey, ErrorMessage))
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not connect %s to %s: %s"), *InConnectorKey.ToString(), *InTargetKey.ToString(), *ErrorMessage.ToString());
		return false;
	}
	
	FString ConnectorParentPath, ConnectorName;
	(void)UTLLRN_RigHierarchy::SplitNameSpace(InConnectorKey.Name.ToString(), &ConnectorParentPath, &ConnectorName);
	FTLLRN_RigModuleReference* Module = FindModule(ConnectorParentPath);

	FTLLRN_RigElementKey CurrentTarget = Model->Connections.FindTargetFromConnector(InConnectorKey);

	UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter());

#if WITH_EDITOR
	FName TargetModulePathName = NAME_None;
	UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(Blueprint->GetObjectBeingDebugged());
	if(TLLRN_ModularRig)
	{
		if (UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy())
		{
			TargetModulePathName = Hierarchy->GetModulePathFName(InTargetKey);
		}
	}
	
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "ConnectModuleToElementTransaction", "Connect to Element"), !GIsTransacting);
		Blueprint->Modify();
	}
#endif 

	// First disconnect before connecting to anything else. This might disconnect other secondary/optional connectors.
	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> PreviousConnections;
	if (CurrentTarget.IsValid())
	{
		const TGuardValue<bool> DisableAutomaticReparenting(bAutomaticReparenting, false);
		DisconnectConnector_Internal(InConnectorKey, false, &PreviousConnections, bSetupUndo);
	}
	
	Model->Connections.AddConnection(InConnectorKey, InTargetKey);

	// restore previous connections if possible
	for(const TPair<FTLLRN_RigElementKey, FTLLRN_RigElementKey>& PreviousConnection : PreviousConnections)
	{
		if(!Model->Connections.HasConnection(PreviousConnection.Key))
		{
			FText ErrorMessageForPreviousConnection;
			if(CanConnectConnectorToElement(PreviousConnection.Key, PreviousConnection.Value, ErrorMessageForPreviousConnection))
			{
				(void)ConnectConnectorToElement(PreviousConnection.Key, PreviousConnection.Value, bSetupUndo, false, false);
			}
		}
	}
	
	Notify(ETLLRN_ModularRigNotification::ConnectionChanged, Module);

#if WITH_EDITOR
	if (UTLLRN_ControlRig* RigCDO = Module->Class->GetDefaultObject<UTLLRN_ControlRig>())
	{
		if (TLLRN_ModularRig)
		{
			if (UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy())
			{
				bool bResolvedPrimaryConnector = false;
				if(const FTLLRN_RigConnectorElement* PrimaryConnector = Module->FindPrimaryConnector(Hierarchy))
				{
					bResolvedPrimaryConnector = PrimaryConnector->GetKey() == InConnectorKey;
				}

				// automatically re-parent the module in the module tree as well
				if(bAutomaticReparenting)
				{
					if(const FTLLRN_RigConnectorElement* Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(InConnectorKey))
					{
						if(Connector->IsPrimary())
						{
							if(!TargetModulePathName.IsNone())
							{
								const FString NewModulePath = ReparentModule(Module->GetPath(), TargetModulePathName.ToString(), bSetupUndo);
								if(!NewModulePath.IsEmpty())
								{
									Module = FindModule(NewModulePath);
								}
							}
						}
					}
				}

				if (Module && bAutoResolveOtherConnectors && bResolvedPrimaryConnector)
				{
					(void)AutoConnectModules( {Module->GetPath()}, false, bSetupUndo);
				}
			}
		}
	}
#endif

	(void)DisconnectCyclicConnectors();

#if WITH_EDITOR
	TransactionPtr.Reset();
#endif
	
	return true;
}

bool UTLLRN_ModularTLLRN_RigController::DisconnectConnector(const FTLLRN_RigElementKey& InConnectorKey, bool bDisconnectSubModules, bool bSetupUndo)
{
	return DisconnectConnector_Internal(InConnectorKey, bDisconnectSubModules, nullptr, bSetupUndo);
};

bool UTLLRN_ModularTLLRN_RigController::DisconnectConnector_Internal(const FTLLRN_RigElementKey& InConnectorKey, bool bDisconnectSubModules,
	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey>* OutRemovedConnections, bool bSetupUndo)
{
	FString ConnectorModulePath, ConnectorName;
	if (!UTLLRN_RigHierarchy::SplitNameSpace(InConnectorKey.Name.ToString(), &ConnectorModulePath, &ConnectorName))
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Connector %s does not contain a namespace"), *InConnectorKey.ToString());
		return false;
	}
	
	FTLLRN_RigModuleReference* Module = FindModule(ConnectorModulePath);
	if (!Module)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *ConnectorModulePath);
		return false;
	}

	UTLLRN_ControlRig* RigCDO = Module->Class->GetDefaultObject<UTLLRN_ControlRig>();
	if (!RigCDO)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Invalid rig module class %s"), *Module->Class->GetPathName());
		return false;
	}

	const FTLLRN_RigModuleConnector* ModuleConnector = RigCDO->GetTLLRN_RigModuleSettings().ExposedConnectors.FindByPredicate(
		[ConnectorName](FTLLRN_RigModuleConnector& Connector)
		{
			return Connector.Name == ConnectorName;
		});
	if (!ModuleConnector)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find connector %s in class %s"), *ConnectorName, *Module->Class->GetPathName());
		return false;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter());

	if(!Model->Connections.HasConnection(InConnectorKey))
	{
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "ConnectModuleToElementTransaction", "Connect to Element"), !GIsTransacting);
		Blueprint->Modify();
	}
#endif 

	if(OutRemovedConnections)
	{
		OutRemovedConnections->Add(InConnectorKey, Model->Connections.FindTargetFromConnector(InConnectorKey));
	}
	Model->Connections.RemoveConnection(InConnectorKey);

	if (ModuleConnector->IsPrimary())
	{
		// Remove connections from module and child modules
		TArray<FTLLRN_RigElementKey> ConnectionsToRemove;
		for (const FTLLRN_ModularRigSingleConnection& Connection : Model->Connections)
		{
			if (Connection.Connector.Name.ToString().StartsWith(ConnectorModulePath, ESearchCase::IgnoreCase))
			{
				ConnectionsToRemove.Add(Connection.Connector);
			}
		}
		for (const FTLLRN_RigElementKey& ToRemove : ConnectionsToRemove)
		{
			if(OutRemovedConnections)
			{
				OutRemovedConnections->Add(ToRemove, Model->Connections.FindTargetFromConnector(ToRemove));
			}
			Model->Connections.RemoveConnection(ToRemove);
		}
	}
	else if (!ModuleConnector->IsOptional() && bDisconnectSubModules)
	{
		// Remove connections from child modules
		TArray<FTLLRN_RigElementKey> ConnectionsToRemove;
		for (const FTLLRN_ModularRigSingleConnection& Connection : Model->Connections)
		{
			FString OtherConnectorModulePath, OtherConnectorName;
			(void)UTLLRN_RigHierarchy::SplitNameSpace(Connection.Connector.Name.ToString(), &OtherConnectorModulePath, &OtherConnectorName);
			if (OtherConnectorModulePath.StartsWith(ConnectorModulePath, ESearchCase::IgnoreCase) && OtherConnectorModulePath.Len() > ConnectorModulePath.Len())
			{
				ConnectionsToRemove.Add(Connection.Connector);
			}
		}
		for (const FTLLRN_RigElementKey& ToRemove : ConnectionsToRemove)
		{
			if(OutRemovedConnections)
			{
				OutRemovedConnections->Add(ToRemove, Model->Connections.FindTargetFromConnector(ToRemove));
			}
			Model->Connections.RemoveConnection(ToRemove);
		}
	}

	// todo: Make sure all the rest of the connections are still valid

	// un-parent the module if we've disconnected the primary
	if(bAutomaticReparenting)
	{
		if(ModuleConnector->IsPrimary() && !Module->IsRootModule())
		{
			(void)ReparentModule(Module->GetPath(), FString(), bSetupUndo);
		}
	}

	Notify(ETLLRN_ModularRigNotification::ConnectionChanged, Module);

#if WITH_EDITOR
	TransactionPtr.Reset();
#endif
	
	return true;
}

TArray<FTLLRN_RigElementKey> UTLLRN_ModularTLLRN_RigController::DisconnectCyclicConnectors(bool bSetupUndo)
{
	TArray<FTLLRN_RigElementKey> DisconnectedConnectors;

#if WITH_EDITOR
	const UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter());
	check(Blueprint);

	const UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(Blueprint->GetObjectBeingDebugged());
	if (!TLLRN_ModularRig)
	{
		return DisconnectedConnectors;
	}
	
	const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy();
	if (!Hierarchy)
	{
		return DisconnectedConnectors;
	}

	TArray<FTLLRN_RigElementKey> ConnectorsToDisconnect;
	for (const FTLLRN_ModularRigSingleConnection& Connection : Model->Connections)
	{
		const FString ConnectorModulePath = Hierarchy->GetModulePath(Connection.Connector);
		const FString TargetModulePath = Hierarchy->GetModulePath(Connection.Target);

		// targets in the base hierarchy are always allowed
		if(TargetModulePath.IsEmpty())
		{
			continue;
		}

		const FTLLRN_RigModuleReference* ConnectorModule = Model->FindModule(ConnectorModulePath);
		const FTLLRN_RigModuleReference* TargetModule = Model->FindModule(TargetModulePath);
		if(ConnectorModule == nullptr || TargetModule == nullptr || ConnectorModule == TargetModule)
		{
			continue;
		}

		if(!Model->IsModuleParentedTo(ConnectorModule, TargetModule))
		{
			ConnectorsToDisconnect.Add(Connection.Connector);
		}
	}

	for(const FTLLRN_RigElementKey& ConnectorToDisconnect : ConnectorsToDisconnect)
	{
		if(DisconnectConnector(ConnectorToDisconnect, false, bSetupUndo))
		{
			DisconnectedConnectors.Add(ConnectorToDisconnect);
		}
	}
#endif

	return DisconnectedConnectors;
}

bool UTLLRN_ModularTLLRN_RigController::AutoConnectSecondaryConnectors(const TArray<FTLLRN_RigElementKey>& InConnectorKeys, bool bReplaceExistingConnections, bool bSetupUndo)
{
#if WITH_EDITOR

	UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter());
	if(Blueprint == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN_ModularTLLRN_RigController is not nested under blueprint."));
		return false;
	}

	const UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(Blueprint->GetObjectBeingDebugged());
	if (!TLLRN_ModularRig)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find debugged modular rig in %s"), *Blueprint->GetPathName());
		return false;
	}
	
	UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy();
	if (!Hierarchy)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find hierarchy in %s"), *TLLRN_ModularRig->GetPathName());
		return false;
	}

	for(const FTLLRN_RigElementKey& ConnectorKey : InConnectorKeys)
	{
		if(ConnectorKey.Type != ETLLRN_RigElementType::Connector)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find debugged modular rig in %s"), *Blueprint->GetPathName());
			return false;
		}
		const FTLLRN_RigConnectorElement* Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(ConnectorKey);
		if(Connector == nullptr)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Cannot find connector %s in %s"), *ConnectorKey.ToString(), *Blueprint->GetPathName());
			return false;
		}
		if(Connector->IsPrimary())
		{
			UE_LOG(LogTLLRN_ControlRig, Warning, TEXT("Provided connector %s in %s is a primary connector. It will be skipped during auto resolval."), *ConnectorKey.ToString(), *Blueprint->GetPathName());
		}
	}

	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "AutoResolveSecondaryConnectors", "Auto-Resolve Connectors"), !GIsTransacting);
	}

	Blueprint->Modify();

	bool bResolvedAllConnectors = true;
	for(const FTLLRN_RigElementKey& ConnectorKey : InConnectorKeys)
	{
		const FString ModulePath = Hierarchy->GetModulePath(ConnectorKey);
		if(ModulePath.IsEmpty())
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Connector %s has no associated module path"), *ConnectorKey.ToString());
			bResolvedAllConnectors = false;
			continue;
		}

		const FTLLRN_RigModuleReference* Module = Model->FindModule(ModulePath);
		if(Module == nullptr)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *ModulePath);
			bResolvedAllConnectors = false;
			continue;
		}

		const FTLLRN_RigConnectorElement* PrimaryConnector = Module->FindPrimaryConnector(Hierarchy);
		if(PrimaryConnector == nullptr)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Module %s has no primary connector"), *ModulePath);
			bResolvedAllConnectors = false;
			continue;
		}
		
		const FTLLRN_RigElementKey PrimaryConnectorKey = PrimaryConnector->GetKey();
		if(ConnectorKey == PrimaryConnectorKey)
		{
			// silently skip primary connectors
			continue;
		}
		
		if(!Model->Connections.HasConnection(PrimaryConnectorKey))
		{
			UE_LOG(LogTLLRN_ControlRig, Warning, TEXT("Module %s's primary connector is not resolved"), *ModulePath);
			bResolvedAllConnectors = false;
			continue;
		}
		
		const UTLLRN_ControlRig* RigCDO = Module->Class->GetDefaultObject<UTLLRN_ControlRig>();
		if(RigCDO == nullptr)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Module %s has no default rig assigned"), *ModulePath);
			bResolvedAllConnectors = false;
			continue;
		}

		const UTLLRN_ModularRigRuleManager* RuleManager = Hierarchy->GetRuleManager();
		const FTLLRN_RigModuleInstance* ModuleInstance = TLLRN_ModularRig->FindModule(Module->GetPath());
		
		if (bReplaceExistingConnections || !Model->Connections.HasConnection(ConnectorKey))
		{
			if (const FTLLRN_RigConnectorElement* OtherConnectorElement = Cast<FTLLRN_RigConnectorElement>(Hierarchy->Find(ConnectorKey)))
			{
				FTLLRN_ModularRigResolveResult RuleResults = RuleManager->FindMatches(OtherConnectorElement, ModuleInstance, TLLRN_ModularRig->GetElementKeyRedirector());

				bool bFoundMatch = false;
				if (RuleResults.GetMatches().Num() == 1)
				{
					Model->Connections.AddConnection(ConnectorKey, RuleResults.GetMatches()[0].GetKey());
					Notify(ETLLRN_ModularRigNotification::ConnectionChanged, Module);
					bFoundMatch = true;
				}
				else
				{
					for (const FTLLRN_RigElementResolveResult& Result : RuleResults.GetMatches())
					{
						if (Result.GetState() == ETLLRN_RigElementResolveState::DefaultTarget)
						{
							Model->Connections.AddConnection(ConnectorKey, Result.GetKey());
							Notify(ETLLRN_ModularRigNotification::ConnectionChanged, Module);
							bFoundMatch = true;
							break;
						}
					}
				}

				if(!bFoundMatch)
				{
					bResolvedAllConnectors = false;
				}
			}
		}
	}

	TransactionPtr.Reset();

	return bResolvedAllConnectors;

#else
	
	return false;

#endif
}

bool UTLLRN_ModularTLLRN_RigController::AutoConnectModules(const TArray<FString>& InModulePaths, bool bReplaceExistingConnections, bool bSetupUndo)
{
#if WITH_EDITOR
	TArray<FTLLRN_RigElementKey> ConnectorKeys;

	const UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter());
	if(Blueprint == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN_ModularTLLRN_RigController is not nested under blueprint."));
		return false;
	}

	const UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(Blueprint->GetObjectBeingDebugged());
	if (!TLLRN_ModularRig)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find debugged modular rig in %s"), *Blueprint->GetPathName());
		return false;
	}
	
	const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy();
	if (!Hierarchy)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find hierarchy in %s"), *TLLRN_ModularRig->GetPathName());
		return false;
	}

	for(const FString& ModulePath : InModulePaths)
	{
		const FTLLRN_RigModuleReference* Module = FindModule(ModulePath);
		if (!Module)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *ModulePath);
			return false;
		}

		const TArray<const FTLLRN_RigConnectorElement*> Connectors = Module->FindConnectors(Hierarchy);
		for(const FTLLRN_RigConnectorElement* Connector : Connectors)
		{
			if(Connector->IsSecondary())
			{
				ConnectorKeys.Add(Connector->GetKey());
			}
		}
	}

	return AutoConnectSecondaryConnectors(ConnectorKeys, bReplaceExistingConnections, bSetupUndo);

#else

	return false;

#endif
}

bool UTLLRN_ModularTLLRN_RigController::SetConfigValueInModule(const FString& InModulePath, const FName& InVariableName, const FString& InValue, bool bSetupUndo)
{
	FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	if (!Module)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *InModulePath);
		return false;
	}

	if (!Module->Class.IsValid())
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Class defined in module %s is not valid"), *InModulePath);
		return false;
	}

	const FProperty* Property = Module->Class->FindPropertyByName(InVariableName);
	if (!Property)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find variable %s in module %s"), *InVariableName.ToString(), *InModulePath);
		return false;
	}

	if (Property->HasAllPropertyFlags(CPF_BlueprintReadOnly))
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("The target variable %s in module %s is read only"), *InVariableName.ToString(), *InModulePath);
		return false;
	}

#if WITH_EDITOR
	TArray<uint8, TAlignedHeapAllocator<16>> TempStorage;
	TempStorage.AddZeroed(Property->GetSize());
	uint8* TempMemory = TempStorage.GetData();
	Property->InitializeValue(TempMemory);

	if (!FBlueprintEditorUtils::PropertyValueFromString_Direct(Property, InValue, TempMemory))
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Value %s for variable %s in module %s is not valid"), *InValue, *InVariableName.ToString(), *InModulePath);
		return false;
	}

	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "ConfigureModuleValueTransaction", "Configure Module Value"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif 

	Module->ConfigValues.FindOrAdd(InVariableName) = InValue;

	Notify(ETLLRN_ModularRigNotification::ModuleConfigValueChanged, Module);

#if WITH_EDITOR
	TransactionPtr.Reset();
#endif

	return true;
}

TArray<FString> UTLLRN_ModularTLLRN_RigController::GetPossibleBindings(const FString& InModulePath, const FName& InVariableName)
{
	TArray<FString> PossibleBindings;
	const FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	if (!Module)
	{
		return PossibleBindings;
	}

	if (!Module->Class.IsValid())
	{
		return PossibleBindings;
	}

	const FProperty* TargetProperty = Module->Class->FindPropertyByName(InVariableName);
	if (!TargetProperty)
	{
		return PossibleBindings;
	}

	if (TargetProperty->HasAnyPropertyFlags(CPF_BlueprintReadOnly | CPF_DisableEditOnInstance))
	{
		return PossibleBindings;
	}

	// Add possible blueprint variables
	if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
	{
		TArray<FRigVMExternalVariable> Variables = Blueprint->GeneratedClass->GetDefaultObject<UTLLRN_ControlRig>()->GetExternalVariables();
		for (const FRigVMExternalVariable& Variable : Variables)
		{
			FText ErrorMessage;
			const FString VariableName = Variable.Name.ToString();
			if (CanBindModuleVariable(InModulePath, InVariableName, VariableName, ErrorMessage))
			{
				PossibleBindings.Add(VariableName);
			}
		}
	}

	// Add possible module variables
	const FString InvalidModulePrefix = InModulePath + UTLLRN_ModularRig::NamespaceSeparator;
	Model->ForEachModule([this, &PossibleBindings, InModulePath, InVariableName, InvalidModulePrefix](const FTLLRN_RigModuleReference* InModule) -> bool
	{
		const FString CurModulePath = InModule->GetPath();
		if (InModulePath != CurModulePath && !CurModulePath.StartsWith(InvalidModulePrefix, ESearchCase::IgnoreCase))
		{
			if (!InModule->Class.IsValid())
			{
				InModule->Class.LoadSynchronous();
			}
			if (InModule->Class.IsValid())
			{
				TArray<FRigVMExternalVariable> Variables = InModule->Class->GetDefaultObject<UTLLRN_ControlRig>()->GetExternalVariables();
			   for (const FRigVMExternalVariable& Variable : Variables)
			   {
				   FText ErrorMessage;
				   const FString SourceVariablePath = UTLLRN_RigHierarchy::JoinNameSpace(CurModulePath, Variable.Name.ToString());
				   if (CanBindModuleVariable(InModulePath, InVariableName, SourceVariablePath, ErrorMessage))
				   {
					   PossibleBindings.Add(SourceVariablePath);
				   }
			   }
			}
		}		
		return true;
	});

	return PossibleBindings;
}

bool UTLLRN_ModularTLLRN_RigController::CanBindModuleVariable(const FString& InModulePath, const FName& InVariableName, const FString& InSourcePath, FText& OutErrorMessage)
{
	FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	if (!Module)
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Could not find module %s"), *InModulePath));
		return false;
	}

	if (!Module->Class.IsValid())
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Class defined in module %s is not valid"), *InModulePath));
		return false;
	}

	const FProperty* TargetProperty = Module->Class->FindPropertyByName(InVariableName);
	if (!TargetProperty)
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Could not find variable %s in module %s"), *InVariableName.ToString(), *InModulePath));
		return false;
	}

	if (TargetProperty->HasAnyPropertyFlags(CPF_BlueprintReadOnly | CPF_DisableEditOnInstance))
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("The target variable %s in module %s is read only"), *InVariableName.ToString(), *InModulePath));
		return false;
	}

	FString SourceModulePath, SourceVariableName = InSourcePath;
	(void)UTLLRN_RigHierarchy::SplitNameSpace(InSourcePath, &SourceModulePath, &SourceVariableName);

	FTLLRN_RigModuleReference* SourceModule = nullptr;
	if (!SourceModulePath.IsEmpty())
	{
		SourceModule = FindModule(SourceModulePath);
		if (!SourceModule)
		{
			OutErrorMessage = FText::FromString(FString::Printf(TEXT("Could not find source module %s"), *SourceModulePath));
			return false;
		}

		if (SourceModulePath.StartsWith(InModulePath, ESearchCase::IgnoreCase))
		{
			OutErrorMessage = FText::FromString(FString::Printf(TEXT("Cannot bind variable of module %s to a variable of module %s because the source module is a child of the target module"), *InModulePath, *SourceModulePath));
			return false;
		}
	}

	const FProperty* SourceProperty = nullptr;
	if (SourceModule)
	{
		SourceProperty = SourceModule->Class->FindPropertyByName(*SourceVariableName);
	}
	else
	{
		if(const UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			SourceProperty = Blueprint->GeneratedClass->FindPropertyByName(*SourceVariableName);
		}
	}
	if (!SourceProperty)
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Could not find source variable %s"), *InSourcePath));
		return false;
	}

	FString SourcePath = (SourceModulePath.IsEmpty()) ? SourceVariableName : UTLLRN_RigHierarchy::JoinNameSpace(SourceModulePath, SourceVariableName);
	if (!RigVMTypeUtils::AreCompatible(SourceProperty, TargetProperty))
	{
		FString TargetPath = FString::Printf(TEXT("%s.%s"), *InModulePath, *InVariableName.ToString());
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Property %s of type %s and %s of type %s are not compatible"), *SourcePath, *SourceProperty->GetCPPType(), *TargetPath, *TargetProperty->GetCPPType()));
		return false;
	}

	return true;
}

bool UTLLRN_ModularTLLRN_RigController::BindModuleVariable(const FString& InModulePath, const FName& InVariableName, const FString& InSourcePath, bool bSetupUndo)
{
	FText ErrorMessage;
	if (!CanBindModuleVariable(InModulePath, InVariableName, InSourcePath, ErrorMessage))
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not bind module variable %s : %s"), *UTLLRN_RigHierarchy::JoinNameSpace(InModulePath, InVariableName.ToString()), *ErrorMessage.ToString());
		return false;
	}
	
	FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	const FProperty* TargetProperty = Module->Class->FindPropertyByName(InVariableName);

	FString SourceModulePath, SourceVariableName = InSourcePath;
	(void)UTLLRN_RigHierarchy::SplitNameSpace(InSourcePath, &SourceModulePath, &SourceVariableName);

	FTLLRN_RigModuleReference* SourceModule = nullptr;
	if (!SourceModulePath.IsEmpty())
	{
		SourceModule = FindModule(SourceModulePath);
	}

	const FProperty* SourceProperty = nullptr;
	if (SourceModule)
	{
		SourceProperty = SourceModule->Class->FindPropertyByName(*SourceVariableName);
	}
	else
	{
		if(const UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			SourceProperty = Blueprint->GeneratedClass->FindPropertyByName(*SourceVariableName);
		}
	}

	FString SourcePath = (SourceModulePath.IsEmpty()) ? SourceVariableName : UTLLRN_RigHierarchy::JoinNameSpace(SourceModulePath, SourceVariableName);

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "BindModuleVariableTransaction", "Bind Module Variable"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif

	FString& SourceStr = Module->Bindings.FindOrAdd(InVariableName);
	SourceStr = SourcePath;

	Notify(ETLLRN_ModularRigNotification::ModuleConfigValueChanged, Module);

#if WITH_EDITOR
	TransactionPtr.Reset();
#endif

	return true;
}

bool UTLLRN_ModularTLLRN_RigController::UnBindModuleVariable(const FString& InModulePath, const FName& InVariableName, bool bSetupUndo)
{
	FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	if (!Module)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *InModulePath);
		return false;
	}

	if (!Module->Bindings.Contains(InVariableName))
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Variable %s in module %s is not bound"), *InVariableName.ToString(), *InModulePath);
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "BindModuleVariableTransaction", "Bind Module Variable"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif

	Module->Bindings.Remove(InVariableName);

	Notify(ETLLRN_ModularRigNotification::ModuleConfigValueChanged, Module);

#if WITH_EDITOR
	TransactionPtr.Reset();
#endif

	return true;
}

bool UTLLRN_ModularTLLRN_RigController::DeleteModule(const FString& InModulePath, bool bSetupUndo)
{
	FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	if (!Module)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *InModulePath);
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "DeleteModuleTransaction", "Delete Module"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif

	(void)DeselectModule(Module->GetPath());

	// Delete children
	TArray<FString> ChildrenPaths;
	Algo::Transform(Module->CachedChildren, ChildrenPaths, [](const FTLLRN_RigModuleReference* Child){ return Child->GetPath(); });
	for (const FString& ChildPath : ChildrenPaths)
	{
		DeleteModule(ChildPath, bSetupUndo);
	}

	Model->DeletedModules.Add(*Module);
	Model->Modules.RemoveSingle(*Module);
	Model->UpdateCachedChildren();
	UpdateShortNames();

	// Fix connections
	{
		TArray<FTLLRN_RigElementKey> ToRemove;
		for (FTLLRN_ModularRigSingleConnection& Connection : Model->Connections)
		{
			FString ConnectionModulePath, ConnectionName;
			(void)UTLLRN_RigHierarchy::SplitNameSpace(Connection.Connector.Name.ToString(), &ConnectionModulePath, &ConnectionName);

			if (ConnectionModulePath == InModulePath)
			{
				ToRemove.Add(Connection.Connector);
			}

			FString TargetModulePath, TargetName;
			(void)UTLLRN_RigHierarchy::SplitNameSpace(Connection.Target.Name.ToString(), &TargetModulePath, &TargetName);
			if (TargetModulePath == InModulePath)
			{
				ToRemove.Add(Connection.Connector);
			}
		}
		for (FTLLRN_RigElementKey& KeyToRemove : ToRemove)
		{
			Model->Connections.RemoveConnection(KeyToRemove);
		}
		Model->Connections.UpdateFromConnectionList();
	}

	// Fix bindings
	for (FTLLRN_RigModuleReference& Reference : Model->Modules)
	{
		Reference.Bindings = Reference.Bindings.FilterByPredicate([InModulePath](const TPair<FName, FString>& Binding)
		{
			FString ModulePath, VariableName = Binding.Value;
			(void)UTLLRN_RigHierarchy::SplitNameSpace(Binding.Value, &ModulePath, &VariableName);
			if (ModulePath == InModulePath)
			{
				return false;
			}
			return true;
		});
	}

	Notify(ETLLRN_ModularRigNotification::ModuleRemoved, &Model->DeletedModules.Last());

	Model->DeletedModules.Reset();

#if WITH_EDITOR
	TransactionPtr.Reset();
#endif
	return false;
}

FString UTLLRN_ModularTLLRN_RigController::RenameModule(const FString& InModulePath, const FName& InNewName, bool bSetupUndo)
{
	FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	if (!Module)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *InModulePath);
		return FString();
	}

	const FString OldName = Module->Name.ToString();
	const FString NewName = InNewName.ToString();
	if (OldName.Equals(NewName))
	{
		return Module->GetPath();
	}

	FText ErrorMessage;
	if (!CanRenameModule(InModulePath, InNewName, ErrorMessage))
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not rename module %s: %s"), *InModulePath, *ErrorMessage.ToString());
		return FString();
	}
	
#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "RenameModuleTransaction", "Rename Module"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif
	
	const FString OldPath = (Module->ParentPath.IsEmpty()) ? OldName : UTLLRN_RigHierarchy::JoinNameSpace(Module->ParentPath, OldName);
	const FString NewPath = (Module->ParentPath.IsEmpty()) ? *NewName :  UTLLRN_RigHierarchy::JoinNameSpace(Module->ParentPath, NewName);

	const int32 SelectionIndex = Model->SelectedModulePaths.Find(OldPath);
	if(SelectionIndex != INDEX_NONE)
	{
		Notify(ETLLRN_ModularRigNotification::ModuleDeselected, Module);
	}

	Module->PreviousName = Module->Name;
	Module->Name = InNewName;
	TArray<FTLLRN_RigModuleReference*> Children;
	Children.Append(Module->CachedChildren);
	for (int32 i=0; i<Children.Num(); ++i)
	{
		FTLLRN_RigModuleReference* Child = Children[i];
		Child->ParentPath.ReplaceInline(*OldPath, *NewPath);

		Children.Append(Child->CachedChildren);
	}

	// Fix connections
	{
		const FString OldNamespace = OldPath + TEXT(":");
		const FString NewNamespace = NewPath + TEXT(":");
		for (FTLLRN_ModularRigSingleConnection& Connection : Model->Connections)
		{
			if (Connection.Connector.Name.ToString().StartsWith(OldNamespace, ESearchCase::IgnoreCase))
			{
				Connection.Connector.Name = *FString::Printf(TEXT("%s%s"), *NewNamespace, *Connection.Connector.Name.ToString().RightChop(OldNamespace.Len()));
			}
			if (Connection.Target.Name.ToString().StartsWith(OldNamespace, ESearchCase::IgnoreCase))
			{
				Connection.Target.Name = *FString::Printf(TEXT("%s%s"), *NewNamespace, *Connection.Target.Name.ToString().RightChop(OldNamespace.Len()));
			}
		}
		Model->Connections.UpdateFromConnectionList();
	}

	// Fix bindings
	for (FTLLRN_RigModuleReference& Reference : Model->Modules)
	{
		for (TPair<FName, FString>& Binding : Reference.Bindings)
		{
			FString ModulePath, VariableName = Binding.Value;
			(void)UTLLRN_RigHierarchy::SplitNameSpace(Binding.Value, &ModulePath, &VariableName);
			if (ModulePath == OldPath)
			{
				Binding.Value = UTLLRN_RigHierarchy::JoinNameSpace(NewPath, VariableName);
			}
		};
	}

	UpdateShortNames();
	Notify(ETLLRN_ModularRigNotification::ModuleRenamed, Module);

	if(SelectionIndex != INDEX_NONE)
	{
		Model->SelectedModulePaths[SelectionIndex] = NewPath;
		Notify(ETLLRN_ModularRigNotification::ModuleSelected, Module);
	}

#if WITH_EDITOR
	TransactionPtr.Reset();
#endif
	
	return NewPath;
}

bool UTLLRN_ModularTLLRN_RigController::CanRenameModule(const FString& InModulePath, const FName& InNewName, FText& OutErrorMessage) const
{
	if (InNewName.IsNone() || InNewName.ToString().IsEmpty())
	{
		OutErrorMessage = FText::FromString(TEXT("Name is empty."));
		return false;
	}

	if(InNewName.ToString().Contains(UTLLRN_ModularRig::NamespaceSeparator))
	{
		OutErrorMessage = NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "NameContainsNamespaceSeparator", "Name contains namespace separator ':'.");
		return false;
	}

	const FTLLRN_RigModuleReference* Module = const_cast<UTLLRN_ModularTLLRN_RigController*>(this)->FindModule(InModulePath);
	if (!Module)
	{
		OutErrorMessage = FText::FromString(FString::Printf(TEXT("Module %s not found."), *InModulePath));
		return false;
	}

	FString ErrorMessage;
	if(!IsNameAvailable(Module->ParentPath, InNewName, &ErrorMessage))
	{
		OutErrorMessage = FText::FromString(ErrorMessage);
		return false;
	}
	return true;
}

FString UTLLRN_ModularTLLRN_RigController::ReparentModule(const FString& InModulePath, const FString& InNewParentModulePath, bool bSetupUndo)
{
	FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	if (!Module)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *InModulePath);
		return FString();
	}

	const FTLLRN_RigModuleReference* NewParentModule = FindModule(InNewParentModulePath);
	const FString PreviousParentPath = Module->ParentPath;
	const FString ParentPath = (NewParentModule) ? NewParentModule->GetPath() : FString();
	if(PreviousParentPath.Equals(ParentPath, ESearchCase::IgnoreCase))
	{
		return Module->GetPath();
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "ReparentModuleTransaction", "Reparent Module"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif

	// Reparent or unparent children
	const FString OldPath = Module->GetPath();

	const int32 SelectionIndex = Model->SelectedModulePaths.Find(OldPath);
	if(SelectionIndex != INDEX_NONE)
	{
		Notify(ETLLRN_ModularRigNotification::ModuleDeselected, Module);
	}
	
	Module->PreviousParentPath = Module->ParentPath;
	Module->PreviousName = Module->Name;
	Module->ParentPath = (NewParentModule) ? NewParentModule->GetPath() : FString();
	Module->Name = GetSafeNewName(Module->ParentPath, FTLLRN_RigName(Module->Name));
	const FString NewPath = Module->GetPath();

	// Fix all the subtree namespaces
	TArray<FTLLRN_RigModuleReference*> SubTree = Module->CachedChildren;
	for (int32 Index=0; Index<SubTree.Num(); ++Index)
	{
		SubTree[Index]->ParentPath.ReplaceInline(*OldPath, *NewPath);
		SubTree.Append(SubTree[Index]->CachedChildren);
	}

	Model->UpdateCachedChildren();
	UpdateShortNames();

	// Fix connections
	{
		for (FTLLRN_ModularRigSingleConnection& Connection : Model->Connections)
		{
			if (Connection.Connector.Name.ToString().StartsWith(OldPath, ESearchCase::IgnoreCase))
			{
				Connection.Connector.Name = *FString::Printf(TEXT("%s%s"), *NewPath, *Connection.Connector.Name.ToString().RightChop(OldPath.Len()));
			}
			if (Connection.Target.Name.ToString().StartsWith(OldPath, ESearchCase::IgnoreCase))
			{
				Connection.Target.Name = *FString::Printf(TEXT("%s%s"), *NewPath, *Connection.Target.Name.ToString().RightChop(OldPath.Len()));
			}
		}
		Model->Connections.UpdateFromConnectionList();
	}

	// Fix bindings
	for (FTLLRN_RigModuleReference& Reference : Model->Modules)
	{
		const FString ReferencePath = Reference.GetPath();
		for (TPair<FName, FString>& Binding : Reference.Bindings)
		{
			FString ModulePath, VariableName = Binding.Value;
			(void)UTLLRN_RigHierarchy::SplitNameSpace(Binding.Value, &ModulePath, &VariableName);
			if (ModulePath == OldPath)
			{
				Binding.Value = UTLLRN_RigHierarchy::JoinNameSpace(NewPath, VariableName);
				ModulePath = NewPath;
			}

			// Remove any child dependency
			if (ModulePath.Contains(ReferencePath))
			{
				UE_LOG(LogTLLRN_ControlRig, Warning, TEXT("Binding lost due to source %s contained in child module of %s"), *Binding.Value, *ReferencePath);
				Binding.Value.Reset();
			}
		};

		Reference.Bindings = Reference.Bindings.FilterByPredicate([](const TPair<FName, FString>& Binding)
		{
			return !Binding.Value.IsEmpty();
		});
	}

	// Fix connectors in the hierarchies
	

	// since we've reparented the module now we should clear out all connectors which are cyclic
	(void)DisconnectCyclicConnectors(bSetupUndo);

	Notify(ETLLRN_ModularRigNotification::ModuleReparented, Module);

	if(SelectionIndex != INDEX_NONE)
	{
		Model->SelectedModulePaths[SelectionIndex] = NewPath;
		Notify(ETLLRN_ModularRigNotification::ModuleSelected, Module);
	}

#if WITH_EDITOR
 	TransactionPtr.Reset();
#endif
	
	return NewPath;
}

FString UTLLRN_ModularTLLRN_RigController::MirrorModule(const FString& InModulePath, const FRigVMMirrorSettings& InSettings, bool bSetupUndo)
{
	FTLLRN_RigModuleReference* OriginalModule = FindModule(InModulePath);
	if (!OriginalModule || !OriginalModule->Class.IsValid())
	{
		return FString();
	}

	FString NewModuleName = OriginalModule->Name.ToString();
	if (!InSettings.SearchString.IsEmpty())
	{
		NewModuleName = NewModuleName.Replace(*InSettings.SearchString, *InSettings.ReplaceString, ESearchCase::CaseSensitive);
		NewModuleName = GetSafeNewName(OriginalModule->ParentPath, FTLLRN_RigName(NewModuleName)).ToString();
	}

	// Before any changes, gather all the information we need from the OriginalModule, as the pointer might become invalid afterwards
	const TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> OriginalConnectionMap = Model->Connections.GetModuleConnectionMap(InModulePath);
	const TMap<FName, FString> OriginalBindings = OriginalModule->Bindings;
	const TSubclassOf<UTLLRN_ControlRig> OriginalClass = OriginalModule->Class.Get();
	const FString OriginalParentPath = OriginalModule->ParentPath;
	const TMap<FName, FString> OriginalConfigValues = OriginalModule->ConfigValues;

	FTLLRN_ModularTLLRN_RigControllerCompileBracketScope CompileBracketScope(this);

	FString NewModulePath = AddModule(*NewModuleName, OriginalClass, OriginalParentPath, bSetupUndo);
	FTLLRN_RigModuleReference* NewModule = FindModule(NewModulePath);
	if (!NewModule)
	{
		return FString();
	}

	for (const TPair<FTLLRN_RigElementKey, FTLLRN_RigElementKey>& Pair : OriginalConnectionMap)
	{
		FString OriginalTargetPath = Pair.Value.Name.ToString();
		FString NewTargetPath = OriginalTargetPath.Replace(*InSettings.SearchString, *InSettings.ReplaceString, ESearchCase::CaseSensitive);
		FTLLRN_RigElementKey NewTargetKey(*NewTargetPath, Pair.Value.Type);

		FString NewConnectorPath = UTLLRN_RigHierarchy::JoinNameSpace(NewModulePath, Pair.Key.Name.ToString());
		FTLLRN_RigElementKey NewConnectorKey(*NewConnectorPath, ETLLRN_RigElementType::Connector);
		ConnectConnectorToElement(NewConnectorKey, NewTargetKey, bSetupUndo, false, false);

		// Path might change after connecting
		NewModulePath = NewModule->GetPath();
	}

	for (const TPair<FName, FString>& Pair : OriginalBindings)
	{
		FString NewSourcePath = Pair.Value.Replace(*InSettings.SearchString, *InSettings.ReplaceString, ESearchCase::CaseSensitive);
		BindModuleVariable(NewModulePath, Pair.Key, NewSourcePath, bSetupUndo);
	}

	TSet<FName> ConfigValueSet;
#if WITH_EDITOR
	for (TFieldIterator<FProperty> PropertyIt(OriginalClass); PropertyIt; ++PropertyIt)
	{
		const FProperty* Property = *PropertyIt;
		
		// skip advanced properties for now
		if (Property->HasAnyPropertyFlags(CPF_AdvancedDisplay))
		{
			continue;
		}

		// skip non-public properties for now
		const bool bIsPublic = Property->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst);
		const bool bIsInstanceEditable = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
		if(!bIsPublic || !bIsInstanceEditable)
		{
			continue;
		}

		const FString CPPType = Property->GetCPPType();
		bool bIsVector;
		if (CPPType == TEXT("FVector"))
		{
			bIsVector = true;
		}
		else if (CPPType == TEXT("FTransform"))
		{
			bIsVector = false;
		}
		else
		{
			continue;
		}

		FString NewValueStr;
		if (const FString* OriginalValue = OriginalConfigValues.Find(Property->GetFName()))
		{
			if (bIsVector)
			{
				FVector Value;
				FBlueprintEditorUtils::PropertyValueFromString_Direct(Property, *OriginalValue, (uint8*)&Value);
				Value = InSettings.MirrorVector(Value);
				FBlueprintEditorUtils::PropertyValueToString_Direct(Property, (uint8*)&Value, NewValueStr, nullptr);
			}
			else
			{
				FTransform Value;
				FBlueprintEditorUtils::PropertyValueFromString_Direct(Property, *OriginalValue, (uint8*)&Value);
				Value = InSettings.MirrorTransform(Value);
				FBlueprintEditorUtils::PropertyValueToString_Direct(Property, (uint8*)&Value, NewValueStr, nullptr);
			}
		}
		else
		{
			if (UTLLRN_ControlRig* CDO = OriginalClass->GetDefaultObject<UTLLRN_ControlRig>())
			{
				if (bIsVector)
				{
					FVector NewVector = *Property->ContainerPtrToValuePtr<FVector>(CDO);
					NewVector = InSettings.MirrorVector(NewVector);
					FBlueprintEditorUtils::PropertyValueToString_Direct(Property, (uint8*)&NewVector, NewValueStr, nullptr);
				}
				else
				{
					FTransform NewTransform = *Property->ContainerPtrToValuePtr<FTransform>(CDO);
					NewTransform = InSettings.MirrorTransform(NewTransform);
					FBlueprintEditorUtils::PropertyValueToString_Direct(Property, (uint8*)&NewTransform, NewValueStr, nullptr);
				}
			}
		}

		ConfigValueSet.Add(Property->GetFName());
		SetConfigValueInModule(NewModulePath, Property->GetFName(), NewValueStr, bSetupUndo);
	}
#endif

	// Add any other config value that was set in the original module, but was not mirrored
	for (const TPair<FName, FString>& Pair : OriginalConfigValues)
	{
		if (!ConfigValueSet.Contains(Pair.Key))
		{
			SetConfigValueInModule(NewModulePath, Pair.Key, Pair.Value, bSetupUndo);
		}
	}
	
	return NewModulePath;
}

bool UTLLRN_ModularTLLRN_RigController::SetModuleShortName(const FString& InModulePath, const FString& InNewShortName, bool bSetupUndo)
{
	FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	if (!Module)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *InModulePath);
		return false;
	}

	FText ErrorMessage;
	if (!CanSetModuleShortName(InModulePath, InNewShortName, ErrorMessage))
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not rename module %s: %s"), *InModulePath, *ErrorMessage.ToString());
		return false;
	}

	const FString OldShortName = Module->GetShortName();
	const FString NewShortName = InNewShortName;
	if (OldShortName.Equals(NewShortName))
	{
		return true;
	}
	
#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "SetModuleShortNameTransaction", "Set Module Display Name"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif
	
	Module->ShortName = InNewShortName;
	Module->bShortNameBasedOnPath = false;

	Notify(ETLLRN_ModularRigNotification::ModuleShortNameChanged, Module);

	// update all other display named to avoid collision
	UpdateShortNames();

#if WITH_EDITOR
	TransactionPtr.Reset();
#endif
	
	return true;
}

bool UTLLRN_ModularTLLRN_RigController::CanSetModuleShortName(const FString& InModulePath, const FString& InNewShortName, FText& OutErrorMessage) const
{
	FString ErrorMessage;
	if(!IsShortNameAvailable(FTLLRN_RigName(InNewShortName), &ErrorMessage))
	{
		OutErrorMessage = FText::FromString(ErrorMessage);
		return false;
	}
	return true;
}

bool UTLLRN_ModularTLLRN_RigController::SwapModuleClass(const FString& InModulePath, TSubclassOf<UTLLRN_ControlRig> InNewClass, bool bSetupUndo)
{
	FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	if (!Module)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *InModulePath);
		return false;
	}

	if (!InNewClass)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Invalid InClass"));
		return false;
	}

	UTLLRN_ControlRig* ClassDefaultObject = InNewClass->GetDefaultObject<UTLLRN_ControlRig>();
	if (!ClassDefaultObject->IsTLLRN_RigModule())
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Class %s is not a rig module"), *InNewClass->GetClassPathName().ToString());
		return false;
	}

	if (Module->Class.Get() == InNewClass)
	{
		// Nothing to do here
		return true;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "SwapModuleClassTransaction", "Swap Module Class"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif

	Module->Class = InNewClass;

	// Remove invalid connectors/connections
	{
		const TArray<FTLLRN_ModularRigSingleConnection>& Connections = Model->Connections.GetConnectionList();
		const UTLLRN_ControlRig* CDO = InNewClass->GetDefaultObject<UTLLRN_ControlRig>();
		const TArray<FTLLRN_RigModuleConnector>& ExposedConnectors = CDO->GetTLLRN_RigModuleSettings().ExposedConnectors;

		TArray<FTLLRN_RigElementKey> ConnectionsToRemove;
		for (const FTLLRN_ModularRigSingleConnection& Connection : Connections)
		{
			FString Namespace, ConnectorName;
			UTLLRN_RigHierarchy::SplitNameSpace(Connection.Connector.Name.ToString(), &Namespace, &ConnectorName);
			if (Namespace.Equals(InModulePath))
			{
				if (!ExposedConnectors.ContainsByPredicate([ConnectorName](const FTLLRN_RigModuleConnector& Exposed)
				{
				   return Exposed.Name == ConnectorName;
				}))
				{
					ConnectionsToRemove.Add(Connection.Connector);
					continue;
				}

				FText ErrorMessage;
				if (!CanConnectConnectorToElement(Connection.Connector, Connection.Target, ErrorMessage))
				{
					ConnectionsToRemove.Add(Connection.Connector);
				}
			}
		}

		for (const FTLLRN_RigElementKey& ToRemove : ConnectionsToRemove)
		{
			DisconnectConnector(ToRemove, false, bSetupUndo);
		}
	}

	// Remove config values and bindings that are not supported anymore
	RefreshModuleVariables();

	Notify(ETLLRN_ModularRigNotification::ModuleClassChanged, Module);
	
#if WITH_EDITOR
	TransactionPtr.Reset();
#endif

	return true;
}

bool UTLLRN_ModularTLLRN_RigController::SwapModulesOfClass(TSubclassOf<UTLLRN_ControlRig> InOldClass, TSubclassOf<UTLLRN_ControlRig> InNewClass, bool bSetupUndo)
{
#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "SwapModulesOfClassTransaction", "Swap Modules of Class"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif
	
	Model->ForEachModule([this, InOldClass, InNewClass, bSetupUndo](const FTLLRN_RigModuleReference* Module) -> bool
	{
		if (Module->Class.Get() == InOldClass)
		{
			SwapModuleClass(Module->GetPath(), InNewClass, bSetupUndo);
		}
		return true;
	});
	
#if WITH_EDITOR
	TransactionPtr.Reset();
#endif
	
	return true;
}

bool UTLLRN_ModularTLLRN_RigController::SelectModule(const FString& InModulePath, const bool InSelected)
{
	const bool bCurrentlySelected = Model->SelectedModulePaths.Contains(InModulePath);
	if(bCurrentlySelected == InSelected)
	{
		return false;
	}

	const FTLLRN_RigModuleReference* Module = FindModule(InModulePath);
	if(Module == nullptr)
	{
		return false;
	}

	if(InSelected)
	{
		Model->SelectedModulePaths.Add(InModulePath);
	}
	else
	{
		Model->SelectedModulePaths.Remove(InModulePath);
	}

	Notify(InSelected ? ETLLRN_ModularRigNotification::ModuleSelected : ETLLRN_ModularRigNotification::ModuleDeselected, Module);
	return true;
}

bool UTLLRN_ModularTLLRN_RigController::DeselectModule(const FString& InModulePath)
{
	return SelectModule(InModulePath, false);
}

bool UTLLRN_ModularTLLRN_RigController::SetModuleSelection(const TArray<FString>& InModulePaths)
{
	bool bResult = false;
	const TArray<FString> OldSelection = GetSelectedModules();

	for(const FString& PreviouslySelectedModule : OldSelection)
	{
		if(!InModulePaths.Contains(PreviouslySelectedModule))
		{
			if(DeselectModule(PreviouslySelectedModule))
			{
				bResult = true;
			}
		}
	}
	for(const FString& NewModuleToSelect : InModulePaths)
	{
		if(!OldSelection.Contains(NewModuleToSelect))
		{
			if(SelectModule(NewModuleToSelect))
			{
				bResult = true;
			}
		}
	}

	return bResult;
}

TArray<FString> UTLLRN_ModularTLLRN_RigController::GetSelectedModules() const
{
	return Model->SelectedModulePaths;
}

void UTLLRN_ModularTLLRN_RigController::RefreshModuleVariables(bool bSetupUndo)
{
	Model->ForEachModule([this, bSetupUndo](const FTLLRN_RigModuleReference* Element) -> bool
	{
		TGuardValue<bool> NotificationsGuard(bSuspendNotifications, true);
		RefreshModuleVariables(Element, bSetupUndo);
		return true;
	});
}

void UTLLRN_ModularTLLRN_RigController::RefreshModuleVariables(const FTLLRN_RigModuleReference* InModule, bool bSetupUndo)
{
	if (!InModule)
	{
		return;
	}
	
	// avoid dead class pointers
	const UClass* ModuleClass = InModule->Class.Get();
	if(ModuleClass == nullptr)
	{
		return;
	}

	// Make sure the provided module belongs to our TLLRN_ModularRigModel
	const FString& ModulePath = InModule->GetPath();
	FTLLRN_RigModuleReference* Module = FindModule(ModulePath);
	if (Module != InModule)
	{
		return;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if (bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_ModularTLLRN_RigController", "RefreshModuleVariablesTransaction", "Refresh Module Variables"), !GIsTransacting);
		if(UBlueprint* Blueprint = Cast<UBlueprint>(GetOuter()))
		{
			Blueprint->Modify();
		}
	}
#endif

	for (TFieldIterator<FProperty> PropertyIt(ModuleClass); PropertyIt; ++PropertyIt)
	{
		const FProperty* Property = *PropertyIt;
		
		// remove advanced, private or not editable properties
		const bool bIsAdvanced = Property->HasAnyPropertyFlags(CPF_AdvancedDisplay);
		const bool bIsPublic = Property->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst);
		const bool bIsInstanceEditable = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
		if (bIsAdvanced || !bIsPublic || !bIsInstanceEditable)
		{
			Module->ConfigValues.Remove(Property->GetFName());
			Module->Bindings.Remove(Property->GetFName());
		}
	}

	// Make sure all the types are valid
	const TMap<FName, FString> ConfigValues = Module->ConfigValues;
	const TMap<FName, FString> Bindings = Module->Bindings;
	Module->ConfigValues.Reset();
	Module->Bindings.Reset();
	for (const TPair<FName, FString>& Pair : ConfigValues)
	{
		SetConfigValueInModule(ModulePath, Pair.Key, Pair.Value, false);
	}
	for (const TPair<FName, FString>& Pair : Bindings)
	{
		BindModuleVariable(ModulePath, Pair.Key, Pair.Value, false);
	}

	// If the module is the source of another module's binding, make sure it is still a valid binding
	Model->ForEachModule([this, InModule, ModulePath, ModuleClass](const FTLLRN_RigModuleReference* OtherModule) -> bool
	{
		if (InModule == OtherModule)
		{
			return true;
		}
		TArray<FName> BindingsToRemove;
		for (const TPair<FName, FString>& Binding : OtherModule->Bindings)
		{
			FString BindingModulePath, VariableName = Binding.Value;
			(void)UTLLRN_RigHierarchy::SplitNameSpace(Binding.Value, &BindingModulePath, &VariableName);
			if (BindingModulePath == ModulePath)
			{
				if (const FProperty* Property = ModuleClass->FindPropertyByName(*VariableName))
				{
					// remove advanced, private or not editable properties
					const bool bIsAdvanced = Property->HasAnyPropertyFlags(CPF_AdvancedDisplay);
					const bool bIsPublic = Property->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst);
					const bool bIsInstanceEditable = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
					if (bIsAdvanced || !bIsPublic || !bIsInstanceEditable)
					{
						BindingsToRemove.Add(Binding.Key);
					}
					else 
					{
						FText ErrorMessage;
						if (!CanBindModuleVariable(OtherModule->GetPath(), Binding.Key, Binding.Value, ErrorMessage))
						{
							BindingsToRemove.Add(Binding.Key);
						}
					}
				}
			}
		}

		for (const FName& ToRemove : BindingsToRemove)
		{
			UnBindModuleVariable(OtherModule->GetPath(), ToRemove);
		}
		return true;
	});
	
#if WITH_EDITOR
	TransactionPtr.Reset();
#endif
}

void UTLLRN_ModularTLLRN_RigController::SanitizeName(FTLLRN_RigName& InOutName, bool bAllowNameSpaces)
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

FTLLRN_RigName UTLLRN_ModularTLLRN_RigController::GetSanitizedName(const FTLLRN_RigName& InName, bool bAllowNameSpaces)
{
	FTLLRN_RigName Name = InName;
	SanitizeName(Name, bAllowNameSpaces);
	return Name;
}

bool UTLLRN_ModularTLLRN_RigController::IsNameAvailable(const FString& InParentModulePath, const FTLLRN_RigName& InDesiredName, FString* OutErrorMessage) const
{
	const FTLLRN_RigName DesiredName = GetSanitizedName(InDesiredName, false);
	if(DesiredName != InDesiredName)
	{
		if(OutErrorMessage)
		{
			static const FString ContainsInvalidCharactersMessage = TEXT("Name contains invalid characters.");
			*OutErrorMessage = ContainsInvalidCharactersMessage;
		}
		return false;
	}

	TArray<FTLLRN_RigModuleReference*>* Children = &Model->RootModules;
	if (!InParentModulePath.IsEmpty())
	{
		if (FTLLRN_RigModuleReference* Parent = const_cast<UTLLRN_ModularTLLRN_RigController*>(this)->FindModule(InParentModulePath))
		{
			Children = &Parent->CachedChildren;
		}
	}

	for (const FTLLRN_RigModuleReference* Child : *Children)
	{
		if (FTLLRN_RigName(Child->Name).Equals(DesiredName, ESearchCase::IgnoreCase))
		{
			if(OutErrorMessage)
			{
				static const FString NameAlreadyInUse = TEXT("This name is already in use.");
				*OutErrorMessage = NameAlreadyInUse;
			}
			return false;
		}
	}
	return true;
}

bool UTLLRN_ModularTLLRN_RigController::IsShortNameAvailable(const FTLLRN_RigName& InDesiredShortName, FString* OutErrorMessage) const
{
	const FTLLRN_RigName DesiredShortName = GetSanitizedName(InDesiredShortName, false);
	if(DesiredShortName != InDesiredShortName)
	{
		if(OutErrorMessage)
		{
			static const FString ContainsInvalidCharactersMessage = TEXT("Display Name contains invalid characters.");
			*OutErrorMessage = ContainsInvalidCharactersMessage;
		}
		return false;
	}

	for (const FTLLRN_RigModuleReference& Child : Model->Modules)
	{
		if (InDesiredShortName == FTLLRN_RigName(Child.GetShortName()))
		{
			if(OutErrorMessage)
			{
				static const FString NameAlreadyInUse = TEXT("This name is already in use.");
				*OutErrorMessage = NameAlreadyInUse;
			}
			return false;
		}
	}
	return true;
}

FTLLRN_RigName UTLLRN_ModularTLLRN_RigController::GetSafeNewName(const FString& InParentModulePath, const FTLLRN_RigName& InDesiredName) const
{
	bool bSafeToUse = false;

	// create a copy of the desired name so that the string conversion can be cached
	const FTLLRN_RigName DesiredName = GetSanitizedName(InDesiredName, false);
	FTLLRN_RigName NewName = DesiredName;
	int32 Index = 0;
	while (!bSafeToUse)
	{
		bSafeToUse = true;
		if(!IsNameAvailable(InParentModulePath, NewName))
		{
			bSafeToUse = false;
			NewName = FString::Printf(TEXT("%s_%d"), *DesiredName.ToString(), ++Index);
		}
	}
	return NewName;
}

FTLLRN_RigName UTLLRN_ModularTLLRN_RigController::GetSafeNewShortName(const FTLLRN_RigName& InDesiredShortName) const
{
	bool bSafeToUse = false;

	// create a copy of the desired name so that the string conversion can be cached
	const FTLLRN_RigName DesiredShortName = GetSanitizedName(InDesiredShortName, true);
	FTLLRN_RigName NewShortName = DesiredShortName;
	int32 Index = 0;
	while (!bSafeToUse)
	{
		bSafeToUse = true;
		if(!IsShortNameAvailable(NewShortName))
		{
			bSafeToUse = false;
			NewShortName = FString::Printf(TEXT("%s_%d"), *DesiredShortName.ToString(), ++Index);
		}
	}
	return NewShortName;
}

void UTLLRN_ModularTLLRN_RigController::Notify(const ETLLRN_ModularRigNotification& InNotification, const FTLLRN_RigModuleReference* InElement)
{
	if(!bSuspendNotifications)
	{
		ModifiedEvent.Broadcast(InNotification, InElement);
	}
}

void UTLLRN_ModularTLLRN_RigController::UpdateShortNames()
{
	TMap<FString, int32> TokenToCount;

	// collect all usages of all paths and their segments
	for(const FTLLRN_RigModuleReference& Module : Model->Modules)
	{
		if(Module.bShortNameBasedOnPath)
		{
			FString RemainingPath = Module.GetPath();
			TokenToCount.FindOrAdd(RemainingPath, 0)++;
			while(UTLLRN_RigHierarchy::SplitNameSpace(RemainingPath, nullptr, &RemainingPath, false))
			{
				TokenToCount.FindOrAdd(RemainingPath, 0)++;
			}
		}
		else
		{
			TokenToCount.FindOrAdd(Module.ShortName, 0)++;
		}
	}

	for(FTLLRN_RigModuleReference& Module : Model->Modules)
	{
		if(Module.bShortNameBasedOnPath)
		{
			FString ShortPath = Module.GetPath();
			if(!Module.ParentPath.IsEmpty())
			{
				FString Left, Right, RemainingPath = Module.GetPath();
				ShortPath.Reset();

				while(UTLLRN_RigHierarchy::SplitNameSpace(RemainingPath, &Left, &Right))
				{
					ShortPath = ShortPath.IsEmpty() ? Right : UTLLRN_RigHierarchy::JoinNameSpace(Right, ShortPath);

					// if the short path only exists once - that's what we use for the display name
					if(TokenToCount.FindChecked(ShortPath) == 1)
					{
						RemainingPath.Reset();
						break;
					}

					RemainingPath = Left;
				}

				if(!RemainingPath.IsEmpty())
				{
					ShortPath = UTLLRN_RigHierarchy::JoinNameSpace(RemainingPath, ShortPath);
				}
			}

			if(!Module.ShortName.Equals(ShortPath, ESearchCase::IgnoreCase))
			{
				Module.ShortName = ShortPath;
				Notify(ETLLRN_ModularRigNotification::ModuleShortNameChanged, &Module);
			}
		}
		else
		{
			// the display name is user defined so we won't touch it
		}
		
	}
}

FTLLRN_ModularTLLRN_RigControllerCompileBracketScope::FTLLRN_ModularTLLRN_RigControllerCompileBracketScope(UTLLRN_ModularTLLRN_RigController* InController)
	: Controller(InController), bSuspendNotifications(InController->bSuspendNotifications)
{
	check(InController);
	
	if (bSuspendNotifications)
	{
		return;
	}
	InController->Notify(ETLLRN_ModularRigNotification::InteractionBracketOpened, nullptr);
}

FTLLRN_ModularTLLRN_RigControllerCompileBracketScope::~FTLLRN_ModularTLLRN_RigControllerCompileBracketScope()
{
	check(Controller);
	if (bSuspendNotifications)
	{
		return;
	}
	Controller->Notify(ETLLRN_ModularRigNotification::InteractionBracketClosed, nullptr);
}

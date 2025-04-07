// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModularRigModel.h"
#include "TLLRN_ModularTLLRN_RigController.h"
#include "TLLRN_ModularRig.h"
#include "AssetRegistry/AssetData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ModularRigModel)

FString FTLLRN_RigModuleReference::GetShortName() const
{
	if(!ShortName.IsEmpty())
	{
		return ShortName;
	}
	return GetPath();
}

FString FTLLRN_RigModuleReference::GetPath() const
{
	if (ParentPath.IsEmpty())
	{
		return Name.ToString();
	}
	return UTLLRN_RigHierarchy::JoinNameSpace(ParentPath, Name.ToString());
}

FString FTLLRN_RigModuleReference::GetNamespace() const
{
	return GetPath() + UTLLRN_ModularRig::NamespaceSeparator;
}

const FTLLRN_RigConnectorElement* FTLLRN_RigModuleReference::FindPrimaryConnector(const UTLLRN_RigHierarchy* InHierarchy) const
{
	if(InHierarchy)
	{
		const FString MyModulePath = GetPath();
		const TArray<FTLLRN_RigConnectorElement*> AllConnectors = InHierarchy->GetConnectors();
		for(const FTLLRN_RigConnectorElement* Connector : AllConnectors)
		{
			if(Connector->IsPrimary())
			{
				const FString ModulePath = InHierarchy->GetModulePath(Connector->GetKey());
				if(!ModulePath.IsEmpty())
				{
					if(ModulePath.Equals(MyModulePath, ESearchCase::IgnoreCase))
					{
						return Connector;
					}
				}
			}
		}
	}
	return nullptr;
}

TArray<const FTLLRN_RigConnectorElement*> FTLLRN_RigModuleReference::FindConnectors(const UTLLRN_RigHierarchy* InHierarchy) const
{
	TArray<const FTLLRN_RigConnectorElement*> Connectors;
	if(InHierarchy)
	{
		const FString MyModulePath = GetPath();
		const TArray<FTLLRN_RigConnectorElement*> AllConnectors = InHierarchy->GetConnectors();
		for(const FTLLRN_RigConnectorElement* Connector : AllConnectors)
		{
			const FString ModulePath = InHierarchy->GetModulePath(Connector->GetKey());
			if(!ModulePath.IsEmpty())
			{
				if(ModulePath.Equals(MyModulePath, ESearchCase::IgnoreCase))
				{
					Connectors.Add(Connector);
				}
			}
		}
	}
	return Connectors;
}

TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> FTLLRN_ModularRigConnections::GetModuleConnectionMap(const FString& InModulePath) const
{
	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> Result;
	for (const FTLLRN_ModularRigSingleConnection& Connection : ConnectionList)
	{
		FString Path, Name;
		UTLLRN_RigHierarchy::SplitNameSpace(Connection.Connector.Name.ToString(), &Path, &Name);

		// Exactly the same path (do not return connectors from child modules)
		if (Path == InModulePath)
		{
			Result.Add(FTLLRN_RigElementKey(*Name, ETLLRN_RigElementType::Connector), Connection.Target);
		}
	}
	return Result;
}

void FTLLRN_ModularRigModel::PatchModelsOnLoad()
{
	if (Connections.IsEmpty())
	{
		ForEachModule([this](const FTLLRN_RigModuleReference* Module) -> bool
		{
			FString ModuleNamespace = Module->GetNamespace();
			for (const TTuple<FTLLRN_RigElementKey, FTLLRN_RigElementKey>& Connection : Module->Connections_DEPRECATED)
			{
				const FString ConnectorPath = FString::Printf(TEXT("%s%s"), *ModuleNamespace, *Connection.Key.Name.ToString());
				FTLLRN_RigElementKey ConnectorKey(*ConnectorPath, ETLLRN_RigElementType::Connector);
				Connections.AddConnection(ConnectorKey, Connection.Value);
			}
			return true;
		});
	}

	Connections.UpdateFromConnectionList();
}

UTLLRN_ModularTLLRN_RigController* FTLLRN_ModularRigModel::GetController(bool bCreateIfNeeded)
{
	if (bCreateIfNeeded && Controller == nullptr)
	{
		const FName SafeControllerName = *FString::Printf(TEXT("%s_TLLRN_ModularRig_Controller"), *GetOuter()->GetPathName());
		UTLLRN_ModularTLLRN_RigController* NewController = NewObject<UTLLRN_ModularTLLRN_RigController>(GetOuter(), UTLLRN_ModularTLLRN_RigController::StaticClass(), SafeControllerName);
		NewController->SetModel(this);
		Controller = NewController;
	}
	return Cast<UTLLRN_ModularTLLRN_RigController>(Controller);
}

void FTLLRN_ModularRigModel::SetOuterClientHost(UObject* InOuterClientHost)
{
	OuterClientHost = InOuterClientHost;
}

void FTLLRN_ModularRigModel::UpdateCachedChildren()
{
	TMap<FString, FTLLRN_RigModuleReference*> PathToModule;
	for (FTLLRN_RigModuleReference& Module : Modules)
	{
		Module.CachedChildren.Reset();
		PathToModule.Add(Module.GetPath(), &Module);
	}
	
	RootModules.Reset();
	for (FTLLRN_RigModuleReference& Module : Modules)
	{
		if (Module.ParentPath.IsEmpty())
		{
			RootModules.Add(&Module);
		}
		else
		{
			if (FTLLRN_RigModuleReference** ParentModule = PathToModule.Find(Module.ParentPath))
			{
				(*ParentModule)->CachedChildren.Add(&Module);
			}
		}
	}
}

FTLLRN_RigModuleReference* FTLLRN_ModularRigModel::FindModule(const FString& InPath)
{
	const TArray<FTLLRN_RigModuleReference*>* Children = &RootModules;

	FString Left = InPath, Right;
	while (UTLLRN_RigHierarchy::SplitNameSpace(Left, &Left, &Right, false))
	{
		FTLLRN_RigModuleReference* const * Child = Children->FindByPredicate([Left](FTLLRN_RigModuleReference* Module)
		{
			return Module->Name.ToString() == Left;
		});
		
		if (!Child)
		{
			return nullptr;
		}
		
		Children = &(*Child)->CachedChildren;
		Left = Right;
	}

	FTLLRN_RigModuleReference* const * Child = Children->FindByPredicate([Left](FTLLRN_RigModuleReference* Module)
		{
			return Module->Name.ToString() == Left;
		});

	if (!Child)
	{
		return nullptr;
	}
	return *Child;
}

const FTLLRN_RigModuleReference* FTLLRN_ModularRigModel::FindModule(const FString& InPath) const
{
	FTLLRN_RigModuleReference* Module = const_cast<FTLLRN_ModularRigModel*>(this)->FindModule(InPath);
	return const_cast<FTLLRN_RigModuleReference*>(Module);
}

FTLLRN_RigModuleReference* FTLLRN_ModularRigModel::GetParentModule(const FString& InPath)
{
	const FString ParentPath = GetParentPath(InPath);
	if(!ParentPath.IsEmpty())
	{
		return FindModule(ParentPath);
	}
	return nullptr;
}

const FTLLRN_RigModuleReference* FTLLRN_ModularRigModel::GetParentModule(const FString& InPath) const
{
	FTLLRN_RigModuleReference* Module = const_cast<FTLLRN_ModularRigModel*>(this)->GetParentModule(InPath);
	return const_cast<FTLLRN_RigModuleReference*>(Module);
}

FTLLRN_RigModuleReference* FTLLRN_ModularRigModel::GetParentModule(const FTLLRN_RigModuleReference* InChildModule)
{
	if(InChildModule)
	{
		return FindModule(InChildModule->ParentPath);
	}
	return nullptr;
}

const FTLLRN_RigModuleReference* FTLLRN_ModularRigModel::GetParentModule(const FTLLRN_RigModuleReference* InChildModule) const
{
	FTLLRN_RigModuleReference* Module = const_cast<FTLLRN_ModularRigModel*>(this)->GetParentModule(InChildModule);
	return const_cast<FTLLRN_RigModuleReference*>(Module);
}

bool FTLLRN_ModularRigModel::IsModuleParentedTo(const FString& InChildModulePath, const FString& InParentModulePath) const
{
	const FTLLRN_RigModuleReference* ChildModule = FindModule(InChildModulePath);
	const FTLLRN_RigModuleReference* ParentModule = FindModule(InParentModulePath);
	if(ChildModule == nullptr || ParentModule == nullptr)
	{
		return false;
	}
	return IsModuleParentedTo(ChildModule, ParentModule);
}

bool FTLLRN_ModularRigModel::IsModuleParentedTo(const FTLLRN_RigModuleReference* InChildModule, const FTLLRN_RigModuleReference* InParentModule) const
{
	if(InChildModule == nullptr || InParentModule == nullptr)
	{
		return false;
	}
	if(InChildModule == InParentModule)
	{
		return false;
	}
	
	while(InChildModule)
	{
		if(InChildModule == InParentModule)
		{
			return true;
		}
		InChildModule = GetParentModule(InChildModule);
	}

	return false;
}

TArray<const FTLLRN_RigModuleReference*> FTLLRN_ModularRigModel::FindModuleInstancesOfClass(const FString& InModuleClassPath) const
{
	TArray<const FTLLRN_RigModuleReference*> Result;
	ForEachModule([&Result, InModuleClassPath](const FTLLRN_RigModuleReference* Module) -> bool
	{
		FString PackageName = Module->Class.ToSoftObjectPath().GetAssetPathString();
		PackageName.RemoveFromEnd(TEXT("_C"));
		if (PackageName == InModuleClassPath)
		{
			Result.Add(Module);
		}
		return true;
	});
	return Result;
}

TArray<const FTLLRN_RigModuleReference*> FTLLRN_ModularRigModel::FindModuleInstancesOfClass(const FAssetData& InModuleAsset) const
{
	return FindModuleInstancesOfClass(InModuleAsset.GetObjectPathString());
}

TArray<const FTLLRN_RigModuleReference*> FTLLRN_ModularRigModel::FindModuleInstancesOfClass(TSoftClassPtr<UTLLRN_ControlRig> InClass) const
{
	return FindModuleInstancesOfClass(*InClass.ToString());
}

FString FTLLRN_ModularRigModel::GetParentPath(const FString& InPath) const
{
	if (const FTLLRN_RigModuleReference* Element = FindModule(InPath))
	{
		return Element->ParentPath;
	}
	return FString();
}

void FTLLRN_ModularRigModel::ForEachModule(TFunction<bool(const FTLLRN_RigModuleReference*)> PerModule) const
{
	TArray<FTLLRN_RigModuleReference*> ModuleInstances = RootModules;
	for (int32 Index=0; Index < ModuleInstances.Num(); ++Index)
	{
		if (!PerModule(ModuleInstances[Index]))
		{
			break;
		}
		ModuleInstances.Append(ModuleInstances[Index]->CachedChildren);
	}
}

TArray<FString> FTLLRN_ModularRigModel::SortPaths(const TArray<FString>& InPaths) const
{
	TArray<FString> Result;
	ForEachModule([InPaths, &Result](const FTLLRN_RigModuleReference* Element) -> bool
	{
		const FString& Path = Element->GetPath();
		if(InPaths.Contains(Path))
		{
			Result.AddUnique(Path);
		}
		return true;
	});
	return Result;
}

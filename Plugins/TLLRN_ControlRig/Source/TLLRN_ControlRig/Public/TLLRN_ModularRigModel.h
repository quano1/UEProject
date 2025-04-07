// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_ControlRig.h"
#include "TLLRN_ModularRigModel.generated.h"

class UTLLRN_ModularTLLRN_RigController;

UENUM()
enum class ETLLRN_ModularRigNotification : uint8
{
	ModuleAdded,

	ModuleRenamed,

	ModuleRemoved,

	ModuleReparented,

	ConnectionChanged,

	ModuleConfigValueChanged,

	ModuleShortNameChanged,

	InteractionBracketOpened, // A bracket has been opened
	
	InteractionBracketClosed, // A bracket has been opened
	
	InteractionBracketCanceled, // A bracket has been canceled

	ModuleClassChanged,

	ModuleSelected,

	ModuleDeselected,

	/** MAX - invalid */
	Max UMETA(Hidden),
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigModuleReference
{
	GENERATED_BODY()

	FTLLRN_RigModuleReference()
		: Name(NAME_None)
		, ShortName()
		, bShortNameBasedOnPath(true)
		, ParentPath(FString())
		, Class(nullptr)
	{}
	
	FTLLRN_RigModuleReference(const FName& InName, TSubclassOf<UTLLRN_ControlRig> InClass, const FString& InParentPath)
		: Name(InName)
		, ShortName()
		, bShortNameBasedOnPath(true)
		, ParentPath(InParentPath)
		, Class(InClass)
	{}

	UPROPERTY()
	FName Name;

	UPROPERTY()
	FString ShortName;

	UPROPERTY()
	bool bShortNameBasedOnPath;

	UPROPERTY()
	FString ParentPath;

	UPROPERTY()
	TSoftClassPtr<UTLLRN_ControlRig> Class;

	UPROPERTY(meta = (DeprecatedProperty))
	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> Connections_DEPRECATED; // Connectors to Connection element

	UPROPERTY()
	TMap<FName, FString> ConfigValues;

	UPROPERTY()
	TMap<FName, FString> Bindings; // ExternalVariableName (current module) -> SourceExternalVariableNamespacedPath (root rig or other module)

	UPROPERTY(transient)
	FName PreviousName;

	UPROPERTY(transient)
	FString PreviousParentPath;

	TArray<FTLLRN_RigModuleReference*> CachedChildren;

	FString GetShortName() const;

	FString GetLongName() const
	{
		return GetPath();
	}

	FString GetPath() const;

	FString GetNamespace() const;

	bool HasParentModule() const { return !ParentPath.IsEmpty(); }

	bool IsRootModule() const { return !HasParentModule(); }
	
	friend bool operator==(const FTLLRN_RigModuleReference& A, const FTLLRN_RigModuleReference& B)
	{
		return A.ParentPath == B.ParentPath &&
			A.Name == B.Name;
	}

	const FTLLRN_RigConnectorElement* FindPrimaryConnector(const UTLLRN_RigHierarchy* InHierarchy) const;
	TArray<const FTLLRN_RigConnectorElement*> FindConnectors(const UTLLRN_RigHierarchy* InHierarchy) const;

	friend class UTLLRN_ModularTLLRN_RigController;
};

USTRUCT(BlueprintType)
struct FTLLRN_ModularRigSingleConnection
{
	GENERATED_BODY()

	FTLLRN_ModularRigSingleConnection()
		: Connector(FTLLRN_RigElementKey()), Target(FTLLRN_RigElementKey()) {}
	
	FTLLRN_ModularRigSingleConnection(const FTLLRN_RigElementKey& InConnector, const FTLLRN_RigElementKey& InTarget)
		: Connector(InConnector), Target(InTarget) {}

	UPROPERTY()
	FTLLRN_RigElementKey Connector;

	UPROPERTY()
	FTLLRN_RigElementKey Target;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_ModularRigConnections
{
public:
	
	GENERATED_BODY()
	/** Connections sorted by creation order */

private:
	
	UPROPERTY()
	TArray<FTLLRN_ModularRigSingleConnection> ConnectionList;

	/** Target key to connector array */
	TMap<FTLLRN_RigElementKey, TArray<FTLLRN_RigElementKey>> ReverseConnectionMap;

public:

	const TArray<FTLLRN_ModularRigSingleConnection>& GetConnectionList() const { return ConnectionList; }

	bool IsEmpty() const { return ConnectionList.IsEmpty(); }
	int32 Num() const { return ConnectionList.Num(); }
	const FTLLRN_ModularRigSingleConnection& operator[](int32 InIndex) const { return ConnectionList[InIndex]; }
	FTLLRN_ModularRigSingleConnection& operator[](int32 InIndex) { return ConnectionList[InIndex]; }
	TArray<FTLLRN_ModularRigSingleConnection>::RangedForIteratorType begin() { return ConnectionList.begin(); }
	TArray<FTLLRN_ModularRigSingleConnection>::RangedForIteratorType end() { return ConnectionList.end(); }
	TArray<FTLLRN_ModularRigSingleConnection>::RangedForConstIteratorType begin() const { return ConnectionList.begin(); }
	TArray<FTLLRN_ModularRigSingleConnection>::RangedForConstIteratorType end() const { return ConnectionList.end(); }

	void UpdateFromConnectionList()
	{
		ReverseConnectionMap.Reset();
		for (const FTLLRN_ModularRigSingleConnection& Connection : ConnectionList)
		{
			TArray<FTLLRN_RigElementKey>& Connectors = ReverseConnectionMap.FindOrAdd(Connection.Target);
			Connectors.AddUnique(Connection.Connector);
		}
	}

	void AddConnection(const FTLLRN_RigElementKey& Connector, const FTLLRN_RigElementKey& Target)
	{
		// Remove any existing connection
		RemoveConnection(Connector);

		ConnectionList.Add(FTLLRN_ModularRigSingleConnection(Connector, Target));
		ReverseConnectionMap.FindOrAdd(Target).AddUnique(Connector);
	}

	void RemoveConnection(const FTLLRN_RigElementKey& Connector)
	{
		int32 ExistingIndex = FindConnectionIndex(Connector);
		if (ConnectionList.IsValidIndex(ExistingIndex))
		{
			if (TArray<FTLLRN_RigElementKey>* Connectors = ReverseConnectionMap.Find(ConnectionList[ExistingIndex].Target))
			{
				*Connectors = Connectors->FilterByPredicate([Connector](const FTLLRN_RigElementKey& TargetConnector)
				{
					return TargetConnector != Connector;
				});
				if (Connectors->IsEmpty())
				{
					ReverseConnectionMap.Remove(ConnectionList[ExistingIndex].Target);
				}
			}
			ConnectionList.RemoveAt(ExistingIndex);
		}
	}

	int32 FindConnectionIndex(const FTLLRN_RigElementKey& InConnectorKey) const
	{
		return ConnectionList.IndexOfByPredicate([InConnectorKey](const FTLLRN_ModularRigSingleConnection& Connection)
		{
			return InConnectorKey == Connection.Connector;
		});
	}

	FTLLRN_RigElementKey FindTargetFromConnector(const FTLLRN_RigElementKey& InConnectorKey) const
	{
		int32 Index = FindConnectionIndex(InConnectorKey);
		if (ConnectionList.IsValidIndex(Index))
		{
			return ConnectionList[Index].Target;
		}
		static const FTLLRN_RigElementKey EmptyKey;
		return EmptyKey;
	}

	const TArray<FTLLRN_RigElementKey>& FindConnectorsFromTarget(const FTLLRN_RigElementKey& InTargetKey) const
	{
		if(const TArray<FTLLRN_RigElementKey>* Connectors = ReverseConnectionMap.Find(InTargetKey))
		{
			return *Connectors;
		}
		static const TArray<FTLLRN_RigElementKey> EmptyList;
		return EmptyList;
	}

	bool HasConnection(const FTLLRN_RigElementKey& InConnectorKey) const
	{
		return ConnectionList.IsValidIndex(FindConnectionIndex(InConnectorKey));
	}

	bool HasConnection(const FTLLRN_RigElementKey& InConnectorKey, const UTLLRN_RigHierarchy* InHierarchy) const
	{
		if(HasConnection(InConnectorKey))
		{
			const FTLLRN_RigElementKey Target = FindTargetFromConnector(InConnectorKey);
			return InHierarchy->Contains(Target);
		}
		return false;
	}

	/** Gets the connection map for a single module, where the connectors are identified without its namespace*/
	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> GetModuleConnectionMap(const FString& InModulePath) const;
};

// A management struct containing all modules in the rig
USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_ModularRigModel
{
public:

	GENERATED_BODY()

	FTLLRN_ModularRigModel(){}
	FTLLRN_ModularRigModel(const FTLLRN_ModularRigModel& Other)
	{
		Modules = Other.Modules;
		Connections = Other.Connections;
		UpdateCachedChildren();
		Connections.UpdateFromConnectionList();
	}
	
	FTLLRN_ModularRigModel& operator=(const FTLLRN_ModularRigModel& Other)
	{
		Modules = Other.Modules;
		Connections = Other.Connections;
		UpdateCachedChildren();
		Connections.UpdateFromConnectionList();
		return *this;
	}
	FTLLRN_ModularRigModel(FTLLRN_ModularRigModel&&) = delete;
	FTLLRN_ModularRigModel& operator=(FTLLRN_ModularRigModel&&) = delete;

	UPROPERTY()
	TArray<FTLLRN_RigModuleReference> Modules;
	TArray<FTLLRN_RigModuleReference*> RootModules;
	TArray<FTLLRN_RigModuleReference> DeletedModules;

	UPROPERTY()
	FTLLRN_ModularRigConnections Connections;

	UPROPERTY(transient)
	TObjectPtr<UObject> Controller;

	void PatchModelsOnLoad();

	UTLLRN_ModularTLLRN_RigController* GetController(bool bCreateIfNeeded = true);

	UObject* GetOuter() const { return OuterClientHost.IsValid() ? OuterClientHost.Get() : nullptr; }

	void SetOuterClientHost(UObject* InOuterClientHost);

	void UpdateCachedChildren();

	FTLLRN_RigModuleReference* FindModule(const FString& InPath);
	const FTLLRN_RigModuleReference* FindModule(const FString& InPath) const;

	FTLLRN_RigModuleReference* GetParentModule(const FString& InPath);
	const FTLLRN_RigModuleReference* GetParentModule(const FString& InPath) const;

	FTLLRN_RigModuleReference* GetParentModule(const FTLLRN_RigModuleReference* InChildModule);
	const FTLLRN_RigModuleReference* GetParentModule(const FTLLRN_RigModuleReference* InChildModule) const;

	FString GetParentPath(const FString& InPath) const;

	void ForEachModule(TFunction<bool(const FTLLRN_RigModuleReference*)> PerModule) const;

	TArray<FString> SortPaths(const TArray<FString>& InPaths) const;

	bool IsModuleParentedTo(const FString& InChildModulePath, const FString& InParentModulePath) const;
	
	bool IsModuleParentedTo(const FTLLRN_RigModuleReference* InChildModule, const FTLLRN_RigModuleReference* InParentModule) const;

	TArray<const FTLLRN_RigModuleReference*> FindModuleInstancesOfClass(const FString& InModuleClassPath) const;
	TArray<const FTLLRN_RigModuleReference*> FindModuleInstancesOfClass(const FAssetData& InModuleAsset) const;
	TArray<const FTLLRN_RigModuleReference*> FindModuleInstancesOfClass(TSoftClassPtr<UTLLRN_ControlRig> InClass) const;

private:
	TWeakObjectPtr<UObject> OuterClientHost;
	TArray<FString> SelectedModulePaths;

	friend class UTLLRN_ModularTLLRN_RigController;
	friend struct FTLLRN_RigModuleReference;
};
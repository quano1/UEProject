// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "TLLRN_ControlRig.h"
#include "TLLRN_ModularRigModel.h"
#include "TLLRN_ModularRig.generated.h"

struct FTLLRN_RigModuleInstance;

struct TLLRN_CONTROLRIG_API FModuleInstanceHandle
{
public:

	FModuleInstanceHandle()
		: TLLRN_ModularRig(nullptr)
		, Path()
	{}

	FModuleInstanceHandle(const UTLLRN_ModularRig* InTLLRN_ModularRig, const FString& InPath);
	FModuleInstanceHandle(const UTLLRN_ModularRig* InTLLRN_ModularRig, const FTLLRN_RigModuleInstance* InElement);

	bool IsValid() const { return Get() != nullptr; }
	operator bool() const { return IsValid(); }
	
	const UTLLRN_ModularRig* GetTLLRN_ModularRig() const
	{
		if (TLLRN_ModularRig.IsValid())
		{
			return TLLRN_ModularRig.Get();
		}
		return nullptr;
	}
	const FString& GetPath() const { return Path; }

	const FTLLRN_RigModuleInstance* Get() const;

private:

	mutable TSoftObjectPtr<UTLLRN_ModularRig> TLLRN_ModularRig;
	FString Path;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigModuleInstance 
{
	GENERATED_USTRUCT_BODY()

public:
	
	FTLLRN_RigModuleInstance()
	: Name(NAME_None)
	, RigPtr(nullptr)
	, ParentPath(FString())
	{
	}

	UPROPERTY()
	FName Name;

private:

	UPROPERTY(transient)
	mutable TObjectPtr<UTLLRN_ControlRig> RigPtr;

public:
	
	UPROPERTY()
	FString ParentPath;

	UPROPERTY()
	TMap<FName, FRigVMExternalVariable> VariableBindings;

	TArray<FTLLRN_RigModuleInstance*> CachedChildren;
	mutable const FTLLRN_RigConnectorElement* PrimaryConnector = nullptr;

	FString GetShortName() const;
	FString GetLongName() const
	{
		return GetPath();
	}
	FString GetPath() const;
	FString GetNamespace() const;
	UTLLRN_ControlRig* GetRig() const;
	void SetRig(UTLLRN_ControlRig* InRig);
	bool ContainsRig(const UTLLRN_ControlRig* InRig) const;
	const FTLLRN_RigModuleReference* GetModuleReference() const;
	const FTLLRN_RigModuleInstance* GetParentModule() const;
	const FTLLRN_RigModuleInstance* GetRootModule() const;
	const FTLLRN_RigConnectorElement* FindPrimaryConnector() const;
	TArray<const FTLLRN_RigConnectorElement*> FindConnectors() const;
	bool IsRootModule() const;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigModuleExecutionElement
{
	GENERATED_USTRUCT_BODY()

	FTLLRN_RigModuleExecutionElement()
		: ModulePath(FString())
		, ModuleInstance(nullptr)
		, EventName(NAME_None)
		, bExecuted(false)
	{}

	FTLLRN_RigModuleExecutionElement(FTLLRN_RigModuleInstance* InModule, FName InEvent)
		: ModulePath(InModule->GetPath())
		, ModuleInstance(InModule)
		, EventName(InEvent)
		, bExecuted(false)
	{}

	UPROPERTY()
	FString ModulePath;
	FTLLRN_RigModuleInstance* ModuleInstance;

	UPROPERTY()
	FName EventName;

	UPROPERTY()
	bool bExecuted;
};

/** Runs logic for mapping input data to transforms (the "Rig") */
UCLASS(Blueprintable, Abstract, editinlinenew)
class TLLRN_CONTROLRIG_API UTLLRN_ModularRig : public UTLLRN_ControlRig
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FTLLRN_RigModuleInstance> Modules;
	TArray<FTLLRN_RigModuleInstance*> RootModules;

	mutable TArray<FName> SupportedEvents;

public:

	virtual void PostInitProperties() override;

	// BEGIN TLLRN_ControlRig
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual void InitializeVMs(bool bRequestInit = true) override;
	virtual bool InitializeVMs(const FName& InEventName) override;
	virtual void InitializeVMsFromCDO() override { URigVMHost::InitializeFromCDO(); }
	virtual void InitializeFromCDO() override;
	virtual void RequestInitVMs() override { URigVMHost::RequestInit(); }
	virtual bool Execute_Internal(const FName& InEventName) override;
	virtual void Evaluate_AnyThread() override;
	virtual FTLLRN_RigElementKeyRedirector& GetElementKeyRedirector() override { return ElementKeyRedirector; }
	virtual FTLLRN_RigElementKeyRedirector GetElementKeyRedirector() const override { return ElementKeyRedirector; }
	virtual bool SupportsEvent(const FName& InEventName) const override;
	virtual const TArray<FName>& GetSupportedEvents() const override;
	// END TLLRN_ControlRig

	UPROPERTY()
	FTLLRN_ModularRigSettings TLLRN_ModularRigSettings;

	// Returns the settings of the modular rig
	const FTLLRN_ModularRigSettings& GetTLLRN_ModularRigSettings() const;

	UPROPERTY()
	FTLLRN_ModularRigModel TLLRN_ModularRigModel;

	UPROPERTY()
	TArray<FTLLRN_RigModuleExecutionElement> ExecutionQueue;
	int32 ExecutionQueueFront = 0;
	void ExecuteQueue();
	void ResetExecutionQueue();

	// BEGIN UObject
	virtual void BeginDestroy() override;
	// END UObject
	
	void OnObjectsReplaced(const TMap<UObject*, UObject*>& OldToNewInstanceMap);

	void ResetModules(bool bDestroyModuleRigs = true);

	const FTLLRN_ModularRigModel& GetTLLRN_ModularRigModel() const;
	void UpdateModuleHierarchyFromCDO();
	void UpdateCachedChildren();
	void UpdateSupportedEvents() const;

	const FTLLRN_RigModuleInstance* FindModule(const FString& InPath) const;
	const FTLLRN_RigModuleInstance* FindModule(const UTLLRN_ControlRig* InModuleInstance) const;
	const FTLLRN_RigModuleInstance* FindModule(const FTLLRN_RigBaseElement* InElement) const;
	const FTLLRN_RigModuleInstance* FindModule(const FTLLRN_RigElementKey& InElementKey) const;
	FString GetParentPath(const FString& InPath) const;
	FString GetShortestDisplayPathForElement(const FTLLRN_RigElementKey& InElementKey, bool bAlwaysShowNameSpace) const;

	void ForEachModule(TFunctionRef<bool(FTLLRN_RigModuleInstance*)> PerModuleFunction);
	void ForEachModule(TFunctionRef<bool(const FTLLRN_RigModuleInstance*)> PerModuleFunction) const;

	void ExecuteConnectorEvent(const FTLLRN_RigElementKey& InConnector, const FTLLRN_RigModuleInstance* InModuleInstance, const FTLLRN_RigElementKeyRedirector* InRedirector, TArray<FTLLRN_RigElementResolveResult>& InOutCandidates);

	/**
	 * Returns a handle to an existing module
	 * @param InPath The path of the module to retrieve a handle for.
	 * @return The retrieved handle (may be invalid)
	 */
	FModuleInstanceHandle GetHandle(const FString& InPath) const
	{
		if(FindModule(InPath))
		{
			return FModuleInstanceHandle((UTLLRN_ModularRig*)this, InPath);
		}
		return FModuleInstanceHandle();
	}

	static inline const TCHAR* NamespaceSeparator = TEXT(":");

protected:

	virtual void RunPostConstructionEvent() override;

private:

	/** Adds a module to the rig*/
	FTLLRN_RigModuleInstance* AddModuleInstance(const FName& InModuleName, TSubclassOf<UTLLRN_ControlRig> InModuleClass, const FTLLRN_RigModuleInstance* InParent, const TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey>& InConnectionMap, const TMap<FName, FString>& InVariableDefaultValues);

	/** Updates the module's variable bindings */
	bool SetModuleVariableBindings(const FString& InModulePath, const TMap<FName, FString>& InVariableBindings);

	/** Destroys / discards a module instance rig */
	static void DiscardModuleRig(UTLLRN_ControlRig* InTLLRN_ControlRig);

	TMap<FString, UTLLRN_ControlRig*> PreviousModuleRigs;

	void ResetShortestDisplayPathCache() const;
	void RecomputeShortestDisplayPathCache() const;
	mutable TMap<FTLLRN_RigElementKey, TTuple<FString, FString>> ElementKeyToShortestDisplayPath; 

	friend struct FTLLRN_RigModuleInstance;
};

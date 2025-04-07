// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_ModularRigModel.h"
#include "TLLRN_ModularTLLRN_RigController.generated.h"

struct FTLLRN_RigModuleReference;

DECLARE_MULTICAST_DELEGATE_TwoParams(FTLLRN_ModularRigModifiedEvent, ETLLRN_ModularRigNotification /* type */, const FTLLRN_RigModuleReference* /* element */);

UCLASS(BlueprintType)
class TLLRN_CONTROLRIG_API UTLLRN_ModularTLLRN_RigController : public UObject
{
	GENERATED_UCLASS_BODY()

	UTLLRN_ModularTLLRN_RigController()
		: Model(nullptr)
		, bSuspendNotifications(false)
	{
	}

	FTLLRN_ModularRigModel* Model;
	FTLLRN_ModularRigModifiedEvent ModifiedEvent;
	bool bSuspendNotifications;

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig | Modules")
	FString AddModule(const FName& InModuleName, TSubclassOf<UTLLRN_ControlRig> InClass, const FString& InParentModulePath, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig | Modules")
	bool CanConnectConnectorToElement(const FTLLRN_RigElementKey& InConnectorKey, const FTLLRN_RigElementKey& InTargetKey, FText& OutErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig | Modules")
	bool ConnectConnectorToElement(const FTLLRN_RigElementKey& InConnectorKey, const FTLLRN_RigElementKey& InTargetKey, bool bSetupUndo = true, bool bAutoResolveOtherConnectors = true, bool bCheckValidConnection = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig | Modules")
	bool DisconnectConnector(const FTLLRN_RigElementKey& InConnectorKey, bool bDisconnectSubModules = false, bool bSetupUndo = true);
	bool DisconnectConnector_Internal(const FTLLRN_RigElementKey& InConnectorKey, bool bDisconnectSubModules = false, TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey>*OutRemovedConnections = nullptr, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig | Modules")
	TArray<FTLLRN_RigElementKey> DisconnectCyclicConnectors(bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig | Modules")
	bool AutoConnectSecondaryConnectors(const TArray<FTLLRN_RigElementKey>& InConnectorKeys, bool bReplaceExistingConnections, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig | Modules")
	bool AutoConnectModules(const TArray<FString>& InModulePaths, bool bReplaceExistingConnections, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig | Modules")
	bool SetConfigValueInModule(const FString& InModulePath, const FName& InVariableName, const FString& InValue, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	bool BindModuleVariable(const FString& InModulePath, const FName& InVariableName, const FString& InSourcePath, bool bSetupUndo = true);
	bool CanBindModuleVariable(const FString& InModulePath, const FName& InVariableName, const FString& InSourcePath, FText& OutErrorMessage);
	TArray<FString> GetPossibleBindings(const FString& InModulePath, const FName& InVariableName);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	bool UnBindModuleVariable(const FString& InModulePath, const FName& InVariableName, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	bool DeleteModule(const FString& InModulePath, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	FString RenameModule(const FString& InModulePath, const FName& InNewName, bool bSetupUndo = true);
	bool CanRenameModule(const FString& InModulePath, const FName& InNewName, FText& OutErrorMessage) const;

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	FString ReparentModule(const FString& InModulePath, const FString& InNewParentModulePath, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	FString MirrorModule(const FString& InModulePath, const FRigVMMirrorSettings& InSettings, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	bool SetModuleShortName(const FString& InModulePath, const FString& InNewShortName, bool bSetupUndo = true);
	bool CanSetModuleShortName(const FString& InModulePath, const FString& InNewShortName, FText& OutErrorMessage) const;

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	bool SwapModuleClass(const FString& InModulePath, TSubclassOf<UTLLRN_ControlRig> InNewClass, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	bool SwapModulesOfClass(TSubclassOf<UTLLRN_ControlRig> InOldClass, TSubclassOf<UTLLRN_ControlRig> InNewClass, bool bSetupUndo = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	bool SelectModule(const FString& InModulePath, const bool InSelected = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	bool DeselectModule(const FString& InModulePath);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig | Modules")
	bool SetModuleSelection(const TArray<FString>& InModulePaths);

	UFUNCTION(BlueprintPure, Category = "TLLRN_ControlRig | Modules")
	TArray<FString> GetSelectedModules() const;

	void RefreshModuleVariables(bool bSetupUndo = true);
	void RefreshModuleVariables(const FTLLRN_RigModuleReference* InModule, bool bSetupUndo = true);

	static int32 GetMaxNameLength() { return 100; }
	static void SanitizeName(FTLLRN_RigName& InOutName, bool bAllowNameSpaces);
	static FTLLRN_RigName GetSanitizedName(const FTLLRN_RigName& InName, bool bAllowNameSpaces);
	bool IsNameAvailable(const FString& InParentModulePath, const FTLLRN_RigName& InDesiredName, FString* OutErrorMessage = nullptr) const;
	bool IsShortNameAvailable(const FTLLRN_RigName& InDesiredShortName, FString* OutErrorMessage = nullptr) const;
	FTLLRN_RigName GetSafeNewName(const FString& InParentModulePath, const FTLLRN_RigName& InDesiredName) const;
	FTLLRN_RigName GetSafeNewShortName(const FTLLRN_RigName& InDesiredShortName) const;
	FTLLRN_RigModuleReference* FindModule(const FString& InPath);
	const FTLLRN_RigModuleReference* FindModule(const FString& InPath) const;
	FTLLRN_ModularRigModifiedEvent& OnModified() { return ModifiedEvent; }

private:

	void SetModel(FTLLRN_ModularRigModel* InModel) { Model = InModel; }
	void Notify(const ETLLRN_ModularRigNotification& InNotification, const FTLLRN_RigModuleReference* InElement);
	void UpdateShortNames();

	bool bAutomaticReparenting;
	
	friend struct FTLLRN_ModularRigModel;
	friend class FTLLRN_ModularTLLRN_RigControllerCompileBracketScope;
};

class TLLRN_CONTROLRIG_API FTLLRN_ModularTLLRN_RigControllerCompileBracketScope
{
public:
   
	FTLLRN_ModularTLLRN_RigControllerCompileBracketScope(UTLLRN_ModularTLLRN_RigController *InController);

	~FTLLRN_ModularTLLRN_RigControllerCompileBracketScope();

private:
	UTLLRN_ModularTLLRN_RigController* Controller;
	bool bSuspendNotifications;
};

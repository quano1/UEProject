// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_ControlRigBlueprintGeneratedClass.h"
#include "UObject/ObjectMacros.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "TLLRN_ControlRigDefines.h"
#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Interfaces/Interface_PreviewMeshProvider.h"
#include "TLLRN_ControlRigGizmoLibrary.h"
#include "TLLRN_ControlRigSchema.h"
#include "RigVMCore/RigVMStatistics.h"
#include "RigVMModel/RigVMClient.h"
#include "TLLRN_ControlRigValidationPass.h"
#include "RigVMBlueprint.h"
#include "Rigs/TLLRN_TLLRN_RigModuleDefines.h"
#include "TLLRN_ModularRigModel.h"

#if WITH_EDITOR
#include "Kismet2/CompilerResultsLog.h"
#endif

#include "TLLRN_ControlRigBlueprint.generated.h"

class URigVMBlueprintGeneratedClass;
class USkeletalMesh;
class UTLLRN_ControlRigGraph;
struct FEndLoadPackageContext;

UENUM(BlueprintType)
enum class ETLLRN_ControlRigType : uint8
{
	IndependentRig = 0,
	TLLRN_RigModule = 1,
	TLLRN_ModularRig = 2,
	MAX // Invalid
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIGDEVELOPER_API FTLLRN_ModuleReferenceData
{
	GENERATED_BODY()

public:
	
	FTLLRN_ModuleReferenceData(){}

	FTLLRN_ModuleReferenceData(const FTLLRN_RigModuleReference* InModule)
	{
		if (InModule)
		{
			ModulePath = InModule->GetPath();
			if (InModule->Class.IsValid())
			{
				ReferencedModule = InModule->Class.Get();
			}
		}
	}

	UPROPERTY()
	FString ModulePath;

	UPROPERTY()
	FSoftClassPath ReferencedModule;
};

UCLASS(BlueprintType, meta=(IgnoreClassThumbnail))
class TLLRN_CONTROLRIGDEVELOPER_API UTLLRN_ControlRigBlueprint : public URigVMBlueprint, public IInterface_PreviewMeshProvider, public ITLLRN_RigHierarchyProvider
{
	GENERATED_UCLASS_BODY()

public:
	UTLLRN_ControlRigBlueprint();

	//  --- IRigVMClientHost interface ---
	virtual UClass* GetRigVMSchemaClass() const override { return UTLLRN_ControlRigSchema::StaticClass(); }
	virtual UScriptStruct* GetRigVMExecuteContextStruct() const override { return FTLLRN_ControlRigExecuteContext::StaticStruct(); }
	virtual UClass* GetRigVMEdGraphClass() const override;
	virtual UClass* GetRigVMEdGraphNodeClass() const override;
	virtual UClass* GetRigVMEdGraphSchemaClass() const override;
	virtual UClass* GetRigVMEditorSettingsClass() const override;

	// URigVMBlueprint interface
	virtual UClass* GetRigVMBlueprintGeneratedClassPrototype() const override { return UTLLRN_ControlRigBlueprintGeneratedClass::StaticClass(); }
	virtual TArray<FString> GeneratePythonCommands(const FString InNewBlueprintName) override;
	virtual void GetPreloadDependencies(TArray<UObject*>& OutDeps) override;
#if WITH_EDITOR
	virtual const FLazyName& GetPanelPinFactoryName() const override;
	static inline const FLazyName TLLRN_ControlRigPanelNodeFactoryName = FLazyName(TEXT("FTLLRN_ControlRigGraphPanelPinFactory"));
	virtual IRigVMEditorModule* GetEditorModule() const override;
#endif

	virtual void Serialize(FArchive& Ar) override;

#if WITH_EDITOR

	// UBlueprint interface
	virtual UClass* RegenerateClass(UClass* ClassToRegenerate, UObject* PreviousCDO) override;
	virtual bool SupportedByDefaultBlueprintFactory() const override { return false; }
	virtual bool IsValidForBytecodeOnlyRecompile() const override { return false; }
	virtual void GetTypeActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void GetInstanceActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	virtual void PostLoad() override;
	virtual void PostTransacted(const FTransactionObjectEvent& TransactionEvent) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostRename(UObject* OldOuter, const FName OldName) override;
	virtual bool RequiresForceLoadMembers(UObject* InObject) const override;

	virtual bool SupportsGlobalVariables() const override { return true; }
	virtual bool SupportsLocalVariables() const override { return !IsTLLRN_ModularRig(); }
	virtual bool SupportsFunctions() const override { return !IsTLLRN_ModularRig(); }
	virtual bool SupportsEventGraphs() const override { return !IsTLLRN_ModularRig(); }


	// UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;

#endif	// #if WITH_EDITOR

	UFUNCTION(BlueprintCallable, Category = "VM")
	UClass* GetTLLRN_ControlRigClass() const;

	bool IsTLLRN_ModularRig() const;

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig Blueprint")
	UTLLRN_ControlRig* CreateTLLRN_ControlRig() { return Cast<UTLLRN_ControlRig>(CreateRigVMHost()); }

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig Blueprint")
	UTLLRN_ControlRig* GetDebuggedTLLRN_ControlRig() { return Cast<UTLLRN_ControlRig>(GetDebuggedRigVMHost()); } 

	/** IInterface_PreviewMeshProvider interface */
	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig Blueprint")
	virtual void SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty = true) override;
	
	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig Blueprint")
	virtual USkeletalMesh* GetPreviewMesh() const override;

	UFUNCTION(BlueprintPure, Category = "TLLRN Control Rig Blueprint")
	bool IsTLLRN_ControlTLLRN_RigModule() const;

#if WITH_EDITORONLY_DATA
	
	bool CanTurnIntoTLLRN_ControlTLLRN_RigModule_Blueprint(bool InAutoConvertHierarchy = false) const { return CanTurnIntoTLLRN_ControlTLLRN_RigModule(InAutoConvertHierarchy); }

	bool CanTurnIntoTLLRN_ControlTLLRN_RigModule(bool InAutoConvertHierarchy, FString* OutErrorMessage = nullptr) const;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "TurnIntoTLLRN_ControlTLLRN_RigModule", ScriptName = "TurnIntoTLLRN_ControlTLLRN_RigModule"), Category = "TLLRN Control Rig Blueprint")
	bool TurnIntoTLLRN_ControlTLLRN_RigModule_Blueprint() { return TurnIntoTLLRN_ControlTLLRN_RigModule(); }

	bool TurnIntoTLLRN_ControlTLLRN_RigModule(bool InAutoConvertHierarchy = false, FString* OutErrorMessage = nullptr);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "CanTurnIntoStandaloneRig", ScriptName = "CanTurnIntoStandaloneRig"), Category = "TLLRN Control Rig Blueprint")
	bool CanTurnIntoStandaloneRig_Blueprint() const { return CanTurnIntoStandaloneRig(); }

	bool CanTurnIntoStandaloneRig(FString* OutErrorMessage = nullptr) const;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "TurnIntoStandaloneRig", ScriptName = "TurnIntoStandaloneRig"), Category = "TLLRN Control Rig Blueprint")
	bool TurnIntoStandaloneRig_Blueprint() { return TurnIntoStandaloneRig(); }

	bool TurnIntoStandaloneRig(FString* OutErrorMessage = nullptr);

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig Blueprint")
	TArray<URigVMNode*> ConvertHierarchyElementsToSpawnerNodes(UTLLRN_RigHierarchy* InHierarchy, TArray<FTLLRN_RigElementKey> InKeys, bool bRemoveElements = true);

#endif // WITH_EDITORONLY_DATA

	UFUNCTION(BlueprintPure, Category = "TLLRN Control Rig Blueprint")
	UTexture2D* GetTLLRN_RigModuleIcon() const;

	DECLARE_EVENT_OneParam(UTLLRN_ControlRigBlueprint, FOnRigTypeChanged, UTLLRN_ControlRigBlueprint*);

	FOnRigTypeChanged& OnRigTypeChanged() { return OnRigTypeChangedDelegate; }

	/// ITLLRN_RigHierarchyProvider interface
	virtual UTLLRN_RigHierarchy* GetHierarchy() const override
	{
		return Hierarchy;
	}

	UPROPERTY(EditAnywhere, Category = "Modular Rig")
	FTLLRN_ModularRigSettings TLLRN_ModularRigSettings;

	UPROPERTY(EditAnywhere, Category = "Hierarchy")
	FTLLRN_RigHierarchySettings HierarchySettings;

	UPROPERTY(EditAnywhere, Category = "Hierarchy", AssetRegistrySearchable)
	FTLLRN_RigModuleSettings TLLRN_RigModuleSettings;

	// This relates to FAssetThumbnailPool::CustomThumbnailTagName and allows
	// the thumbnail pool to show the thumbnail of the icon rather than the
	// rig itself to avoid deploying the 3D renderer.
	UPROPERTY(EditAnywhere, Category = "Hierarchy", AssetRegistrySearchable)
	FString CustomThumbnail;

	/** Asset searchable information module references in this rig */
	UPROPERTY(AssetRegistrySearchable)
	TArray<FTLLRN_ModuleReferenceData> TLLRN_ModuleReferenceData;

	UPROPERTY()
	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> ConnectionMap;

	UFUNCTION(BlueprintPure, Category = "TLLRN Control Rig Blueprint")
	TArray<FTLLRN_ModuleReferenceData> FindReferencesToModule() const;

	static ETLLRN_ControlRigType GetRigType(const FAssetData& InAsset);
	static TArray<FSoftObjectPath> GetReferencesToTLLRN_RigModule(const FAssetData& InModuleAsset);

protected:

	TArray<FTLLRN_ModuleReferenceData> GetTLLRN_ModuleReferenceData() const;

	FOnRigTypeChanged OnRigTypeChangedDelegate;
	
	void UpdateExposedModuleConnectors() const;

	bool ResolveConnector(const FTLLRN_RigElementKey& DraggedKey, const FTLLRN_RigElementKey& TargetKey, bool bSetupUndoRedo = true);

	void UpdateConnectionMapFromModel();

	/** Asset searchable information about exposed public functions on this rig */
	UPROPERTY(AssetRegistrySearchable)
	TArray<FRigVMOldPublicFunctionData> PublicFunctions_DEPRECATED;

	virtual void SetupDefaultObjectDuringCompilation(URigVMHost* InCDO) override;

public:

	virtual void SetupPinRedirectorsForBackwardsCompatibility() override;

	UFUNCTION(BlueprintCallable, Category = "VM")
	static TArray<UTLLRN_ControlRigBlueprint*> GetCurrentlyOpenRigBlueprints();

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary> GizmoLibrary_DEPRECATED;

	UPROPERTY(EditAnywhere, Category = Shapes)
	TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>> ShapeLibraries;

	const FTLLRN_ControlRigShapeDefinition* GetControlShapeByName(const FName& InName) const;

	UPROPERTY(transient, DuplicateTransient, meta = (DisplayName = "VM Statistics", DisplayAfter = "VMCompileSettings"))
	FRigVMStatistics Statistics_DEPRECATED;
#endif

	UPROPERTY(EditAnywhere, Category = "Drawing")
	FRigVMDrawContainer DrawContainer;

#if WITH_EDITOR
	/** Remove a transient / temporary control used to interact with a pin */
	FName AddTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget);

	/** Remove a transient / temporary control used to interact with a pin */
	FName RemoveTransientControl(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget);

	/** Remove a transient / temporary control used to interact with a bone */
	FName AddTransientControl(const FTLLRN_RigElementKey& InElement);

	/** Remove a transient / temporary control used to interact with a bone */
	FName RemoveTransientControl(const FTLLRN_RigElementKey& InElement);

	/** Removes all  transient / temporary control used to interact with pins */
	void ClearTransientControls();

#endif

	UPROPERTY(EditAnywhere, Category = "Influence Map")
	FTLLRN_RigInfluenceMapPerEvent Influences;

public:

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FTLLRN_RigHierarchyContainer HierarchyContainer_DEPRECATED;
#endif

	UPROPERTY(BlueprintReadOnly, Category = "Hierarchy")
	TObjectPtr<UTLLRN_RigHierarchy> Hierarchy;

	UFUNCTION(BlueprintCallable, Category = "Hierarchy")
	UTLLRN_RigHierarchyController* GetHierarchyController() { return Hierarchy->GetController(true); }

	UPROPERTY(BlueprintReadOnly, Category = "Modules")
	FTLLRN_ModularRigModel TLLRN_ModularRigModel;

	UFUNCTION(BlueprintCallable, Category = "Modules")
	UTLLRN_ModularTLLRN_RigController* GetTLLRN_ModularTLLRN_RigController();

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig Blueprint")
	void RecompileTLLRN_ModularRig();

	UPROPERTY(AssetRegistrySearchable)
	ETLLRN_ControlRigType TLLRN_ControlRigType;

	UPROPERTY(AssetRegistrySearchable)
	FName ItemTypeDisplayName = TEXT("TLLRN Control Rig");

private:

	/** Whether or not this rig has an Inversion Event */
	UPROPERTY(AssetRegistrySearchable)
	bool bSupportsInversion;

	/** Whether or not this rig has Controls on It */
	UPROPERTY(AssetRegistrySearchable)
	bool bSupportsControls;

	/** The default skeletal mesh to use when previewing this asset */
#if WITH_EDITORONLY_DATA
	UPROPERTY(AssetRegistrySearchable)
	TSoftObjectPtr<USkeletalMesh> PreviewSkeletalMesh;
#endif

	/** The skeleton from import into a hierarchy */
	UPROPERTY(DuplicateTransient, AssetRegistrySearchable, EditAnywhere, Category="TLLRN Control Rig Blueprint")
	TSoftObjectPtr<UObject> SourceHierarchyImport;

	/** The skeleton from import into a curve */
	UPROPERTY(DuplicateTransient, AssetRegistrySearchable, EditAnywhere, Category="TLLRN Control Rig Blueprint")
	TSoftObjectPtr<UObject> SourceCurveImport;

	/** If set to true, this control rig has animatable controls */
	UPROPERTY(AssetRegistrySearchable)
	bool bExposesAnimatableControls;
public:

	/** If set to true, multiple control rig tracks can be created for the same rig in sequencer*/
	UPROPERTY(EditAnywhere, Category="Sequencer", AssetRegistrySearchable)
	bool bAllowMultipleInstances = false;

private:

	static TArray<UTLLRN_ControlRigBlueprint*> sCurrentlyOpenedRigBlueprints;

	virtual void PathDomainSpecificContentOnLoad() override;
	virtual void GetBackwardsCompatibilityPublicFunctions(TArray<FName>& BackwardsCompatiblePublicFunctions, TMap<URigVMLibraryNode*, FRigVMGraphFunctionHeader>& OldHeaders) override;
	void PatchTLLRN_RigElementKeyCacheOnLoad();
	void PatchPropagateToChildren();

protected:
	virtual void CreateMemberVariablesOnLoad() override;
	virtual void PatchVariableNodesOnLoad() override;

public:
	void UpdateElementKeyRedirector(UTLLRN_ControlRig* InTLLRN_ControlRig) const;
	void PropagatePoseFromInstanceToBP(UTLLRN_ControlRig* InTLLRN_ControlRig) const;
	void PropagatePoseFromBPToInstances() const;
	void PropagateHierarchyFromBPToInstances() const;
	void PropagateDrawInstructionsFromBPToInstances() const;
	void PropagatePropertyFromBPToInstances(FTLLRN_RigElementKey InTLLRN_RigElement, const FProperty* InProperty) const;
	void PropagatePropertyFromInstanceToBP(FTLLRN_RigElementKey InTLLRN_RigElement, const FProperty* InProperty, UTLLRN_ControlRig* InInstance) const;
	void PropagateModuleHierarchyFromBPToInstances() const;
	void UpdateModularDependencyDelegates();
	void OnModularDependencyVMCompiled(UObject* InBlueprint, URigVM* InVM, FRigVMExtendedExecuteContext& InExecuteContext);
	void OnModularDependencyChanged(URigVMBlueprint* InBlueprint);
	void RequestConstructionOnAllModules();
	void RefreshModuleVariables();
	void RefreshModuleVariables(const FTLLRN_RigModuleReference* InModule);
	void RefreshModuleConnectors();
	void RefreshModuleConnectors(const FTLLRN_RigModuleReference* InModule, bool bPropagateHierarchy = true);

	/**
	* Returns the modified event, which can be used to 
	* subscribe to topological changes happening within the hierarchy. The event is broadcast only after all hierarchy instances are up to date
	* @return The event used for subscription.
	*/
	FTLLRN_RigHierarchyModifiedEvent& OnHierarchyModified() { return HierarchyModifiedEvent; }

	FOnRigVMRefreshEditorEvent& OnTLLRN_ModularRigPreCompiled() { return TLLRN_ModularRigPreCompiled; }
	FOnRigVMRefreshEditorEvent& OnTLLRN_ModularRigCompiled() { return TLLRN_ModularRigCompiled; }

private:

	UPROPERTY()
	TObjectPtr<UTLLRN_ControlRigValidator> Validator;

	FTLLRN_RigHierarchyModifiedEvent	HierarchyModifiedEvent;
	FOnRigVMRefreshEditorEvent TLLRN_ModularRigPreCompiled;
	FOnRigVMRefreshEditorEvent TLLRN_ModularRigCompiled;

	UPROPERTY(transient, DuplicateTransient)
	int32 ModulesRecompilationBracket = 0;


	void HandleHierarchyModified(ETLLRN_RigHierarchyNotification InNotification, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);

	void HandleTLLRN_RigModulesModified(ETLLRN_ModularRigNotification InNotification, const FTLLRN_RigModuleReference* InModule);

#if WITH_EDITOR
	virtual void HandlePackageDone() override;
	virtual void HandleConfigureRigVMController(const FRigVMClient* InClient, URigVMController* InControllerToConfigure) override;
#endif

	void UpdateConnectionMapAfterRename(const FString& InOldNameSpace);

	// Class used to temporarily cache all 
	// current control values and reapply them
	// on destruction
	class TLLRN_CONTROLRIGDEVELOPER_API FControlValueScope
	{
	public: 
		FControlValueScope(UTLLRN_ControlRigBlueprint* InBlueprint);
		~FControlValueScope();

	private:

		UTLLRN_ControlRigBlueprint* Blueprint;
		TMap<FName, FTLLRN_RigControlValue> ControlValues;
	};

	UPROPERTY()
	float DebugBoneRadius;

#if WITH_EDITOR
		
public:

	/** Shape libraries to load during package load completed */ 
	TArray<FString> ShapeLibrariesToLoadOnPackageLoaded;

#endif

private:

	friend class FTLLRN_ControlRigBlueprintCompilerContext;
	friend class STLLRN_RigHierarchy;
	friend class STLLRN_TLLRN_RigCurveContainer;
	friend class FTLLRN_ControlRigEditor;
	friend class UEngineTestTLLRN_ControlRig;
	friend class FTLLRN_ControlRigEditMode;
	friend class FTLLRN_ControlRigBlueprintActions;
	friend class FTLLRN_ControlRigDrawContainerDetails;
	friend class UDefaultTLLRN_ControlRigManipulationLayer;
	friend struct FRigValidationTabSummoner;
	friend class UAnimGraphNode_TLLRN_ControlRig;
	friend class UTLLRN_ControlRigThumbnailRenderer;
	friend class FTLLRN_ControlRigGraphDetails;
	friend class FTLLRN_ControlRigEditorModule;
	friend class UTLLRN_ControlRigComponent;
	friend struct FTLLRN_ControlRigGraphSchemaAction_PromoteToVariable;
	friend class UTLLRN_ControlRigGraphSchema;
	friend class FTLLRN_ControlRigBlueprintDetails;
};

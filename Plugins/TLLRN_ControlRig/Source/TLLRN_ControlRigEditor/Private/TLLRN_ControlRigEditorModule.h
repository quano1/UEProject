// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "UObject/WeakObjectPtr.h"
#include "ITLLRN_ControlRigEditorModule.h"
#include "ITLLRN_ControlTLLRN_RigModule.h"
#include "IAnimationEditor.h"
#include "IAnimationEditorModule.h"

class UBlueprint;
class IAssetTypeActions;
class UMaterial;
class UAnimSequence;
class USkeletalMesh;
class USkeleton;
class FToolBarBuilder;
class FExtender;
class FUICommandList;
class UMovieSceneTrack;
class FRigVMEdGraphPanelNodeFactory;
class FTLLRN_ControlRigGraphPanelPinFactory;
class FPropertySection;
class FPropertyEditorModule;

class FTLLRN_ControlRigEditorModule : public ITLLRN_ControlRigEditorModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** FRigVMEditorModule interface */
	virtual UClass* GetRigVMBlueprintClass() const override;
	virtual void GetNodeContextMenuActions(IRigVMClientHost* RigVMClientHost, const URigVMEdGraphNode* EdGraphNode, URigVMNode* ModelNode, UToolMenu* Menu) const override;
	virtual void GetPinContextMenuActions(IRigVMClientHost* RigVMClientHost, const UEdGraphPin* EdGraphPin, URigVMPin* ModelPin, UToolMenu* Menu) const override;
	virtual bool AssetsPublicFunctionsAllowed(const FAssetData& InAssetData) const override;

	/** ITLLRN_ControlRigEditorModule interface */
	virtual TSharedRef<ITLLRN_ControlRigEditor> CreateTLLRN_ControlRigEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UTLLRN_ControlRigBlueprint* Blueprint) override;
	
	/** Animation Toolbar Extender*/
	void AddTLLRN_ControlRigExtenderToToolMenu(FName InToolMenuName);
	TSharedRef< SWidget > GenerateAnimationMenu(TWeakPtr<IAnimationEditor> InAnimationEditor);
	void ToggleIsDrivenByLevelSequence(UAnimSequence* AnimSequence)const;
	bool IsDrivenByLevelSequence(UAnimSequence* AnimSequence)const;
	void EditWithFKTLLRN_ControlRig(UAnimSequence* AnimSequence, USkeletalMesh* SkelMesh, USkeleton* InSkeleton);
	void BakeToTLLRN_ControlRig(UClass* InClass,UAnimSequence* AnimSequence, USkeletalMesh* SkelMesh,USkeleton* InSkeleton);
	static void OpenLevelSequence(UAnimSequence* AnimSequence);
	static void UnLinkLevelSequence(UAnimSequence* AnimSequence);
	void ExtendAnimSequenceMenu();

	void GetDirectManipulationMenuActions(IRigVMClientHost* RigVMClientHost, URigVMNode* InNode, URigVMPin* ModelPin, UToolMenu* Menu) const;

private:
	//property sections
	TSharedRef<FPropertySection> RegisterPropertySection(FPropertyEditorModule& PropertyModule, FName ClassName, FName SectionName, FText DisplayName);
	void UnregisterPropertySectionMappings();
	TMultiMap<FName, FName> RegisteredPropertySections;


	/** Handle for our sequencer control rig parameter track editor */
	FDelegateHandle TLLRN_ControlRigParameterTrackCreateEditorHandle;

	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;

	/** StaticClass is not safe on shutdown, so we cache the name, and use this to unregister on shut down */
	TArray<FName> ClassesToUnregisterOnShutdown;
	TArray<FName> PropertiesToUnregisterOnShutdown;
	
	/** Param to hold Filter Result to pass to Filter*/
	bool bFilterAssetBySkeleton;

	/** Handles for all registered workflows */
	TArray<int32> WorkflowHandles;
};
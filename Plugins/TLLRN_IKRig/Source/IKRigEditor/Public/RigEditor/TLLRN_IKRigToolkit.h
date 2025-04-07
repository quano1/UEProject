// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "IHasPersonaToolkit.h"
#include "TLLRN_IKRigController.h"
#include "TLLRN_IKRigEditorController.h"
#include "TLLRN_IKRigMode.h"
#include "IPersonaPreviewScene.h"
#include "PersonaAssetEditorToolkit.h"
#include "TLLRN_SIKRigHierarchy.h"

namespace TLLRN_IKRigEditorModes
{
	extern const FName TLLRN_IKRigEditorMode;
}

class FTLLRN_IKRigEditorToolkit :
	public FPersonaAssetEditorToolkit,
	public IHasPersonaToolkit,
	public FSelfRegisteringEditorUndoClient,
	public FTickableEditorObject
{
public:

	FTLLRN_IKRigEditorToolkit();
	virtual ~FTLLRN_IKRigEditorToolkit() override;

	void InitAssetEditor(
		const EToolkitMode::Type Mode,
		const TSharedPtr< IToolkitHost >& InitToolkitHost,
		UTLLRN_IKRigDefinition* IKRigAsset);

	/** FAssetEditorToolkit interface */
	virtual void OnClose() override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	/** END FAssetEditorToolkit interface */

	//** FTickableEditorObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual TStatId GetStatId() const override;
	//~ END FTickableEditorObject Interface

	/** IHasPersonaToolkit interface */
	virtual TSharedRef<IPersonaToolkit> GetPersonaToolkit() const override { return PersonaToolkit.ToSharedRef(); }
	/** END IHasPersonaToolkit interface */

	/** FSelfRegisteringEditorUndoClient interface */
	virtual void PostUndo( bool bSuccess );
	virtual void PostRedo( bool bSuccess );
	/** END FSelfRegisteringEditorUndoClient interface */

	TSharedRef<FTLLRN_IKRigEditorController> GetController() const {return EditorController;};

	/** tab creation callbacks */
	void HandlePreviewSceneCreated(const TSharedRef<IPersonaPreviewScene>& InPersonaPreviewScene);
	void HandleDetailsCreated(const TSharedRef<class IDetailsView>& InDetailsView) const;
	void HandleViewportCreated(const TSharedRef<class IPersonaViewport>& InViewport);
	void HandleOnPreviewSceneSettingsCustomized(IDetailLayoutBuilder& DetailBuilder) const;
	/** END preview scene setup */

private:

	/** toolbar */
	void BindCommands();
	void ExtendToolbar();
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);
	/** END toolbar */
	
	/** centralized management of selection across skeleton view and viewport */
	TSharedRef<FTLLRN_IKRigEditorController> EditorController;

	/** Preview scene to be supplied by IHasPersonaToolkit::GetPersonaToolkit */
	TSharedPtr<IPersonaToolkit> PersonaToolkit;
};

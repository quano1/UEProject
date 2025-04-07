// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditor.h"

#include "AdvancedPreviewScene.h"
#include "ToolContextInterfaces.h"
#include "TLLRN_UVEditorToolkit.h"
#include "TLLRN_UVEditorSubsystem.h"
#include "EditorModeManager.h"
#include "EdModeInteractiveToolsContext.h"

#include "AssetEditorModeManager.h"
#include "TLLRN_UVEditorMode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVEditor)

void UTLLRN_UVEditor::Initialize(const TArray<TObjectPtr<UObject>>& InObjects)
{
	// Make sure we have valid targets.
	UTLLRN_UVEditorSubsystem* UVSubsystem = GEditor->GetEditorSubsystem<UTLLRN_UVEditorSubsystem>();
	check(UVSubsystem && UVSubsystem->AreObjectsValidTargets(InObjects));

	OriginalObjectsToEdit = InObjects;

	// This will do a variety of things including registration of the asset editor, creation of the toolkit
	// (via CreateToolkit()), and creation of the editor mode manager within the toolkit.
	// The asset editor toolkit will do the rest of the necessary initialization inside its PostInitAssetEditor.
	UAssetEditor::Initialize();
}
void UTLLRN_UVEditor::Initialize(const TArray<TObjectPtr<UObject>>& InObjects, const TArray<FTransform3d>& InTransforms)
{
	check(InTransforms.Num() == InObjects.Num());
	ObjectWorldspaceOffsets = InTransforms;
	Initialize(InObjects);
}

void UTLLRN_UVEditor::GetWorldspaceRelativeTransforms(TArray<FTransform3d>& OutTransforms)
{
	OutTransforms.Append(ObjectWorldspaceOffsets);
}

IAssetEditorInstance* UTLLRN_UVEditor::GetInstanceInterface() 
{ 
	return ToolkitInstance; 
}

void UTLLRN_UVEditor::GetObjectsToEdit(TArray<UObject*>& OutObjects)
{
	OutObjects.Append(OriginalObjectsToEdit);
	check(OutObjects.Num() > 0);
}

TSharedPtr<FBaseAssetToolkit> UTLLRN_UVEditor::CreateToolkit()
{
	TSharedPtr<FTLLRN_UVEditorToolkit> Toolkit = MakeShared<FTLLRN_UVEditorToolkit>(this);

	return Toolkit;
}


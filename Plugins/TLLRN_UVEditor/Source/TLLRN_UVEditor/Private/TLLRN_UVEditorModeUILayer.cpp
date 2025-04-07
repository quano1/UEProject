// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorModeUILayer.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Toolkits/IToolkit.h"
#include "TLLRN_UVEditorToolkit.h"
#include "TLLRN_UVEditorModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVEditorModeUILayer)

void UTLLRN_UVEditorUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	FTLLRN_UVEditorModule& TLLRN_UVEditorModule = FModuleManager::GetModuleChecked<FTLLRN_UVEditorModule>("TLLRN_UVEditor");
	TLLRN_UVEditorModule.OnRegisterLayoutExtensions().AddUObject(this, &UTLLRN_UVEditorUISubsystem::RegisterLayoutExtensions);
}
void UTLLRN_UVEditorUISubsystem::Deinitialize()
{
	FTLLRN_UVEditorModule& TLLRN_UVEditorModule = FModuleManager::GetModuleChecked<FTLLRN_UVEditorModule>("TLLRN_UVEditor");
	TLLRN_UVEditorModule.OnRegisterLayoutExtensions().RemoveAll(this);
}
void UTLLRN_UVEditorUISubsystem::RegisterLayoutExtensions(FLayoutExtender& Extender)
{
	FTabManager::FTab NewTab(FTabId(UAssetEditorUISubsystem::TopLeftTabID), ETabState::ClosedTab);
	Extender.ExtendStack("EditorSidePanelArea", ELayoutExtensionPosition::After, NewTab);
}

FTLLRN_UVEditorModeUILayer::FTLLRN_UVEditorModeUILayer(const IToolkitHost* InToolkitHost) :
	FAssetEditorModeUILayer(InToolkitHost)
{}

void FTLLRN_UVEditorModeUILayer::OnToolkitHostingStarted(const TSharedRef<IToolkit>& Toolkit)
{
	if (!Toolkit->IsAssetEditor())
	{
		FAssetEditorModeUILayer::OnToolkitHostingStarted(Toolkit);
		HostedToolkit = Toolkit;
		Toolkit->SetModeUILayer(SharedThis(this));
		Toolkit->RegisterTabSpawners(ToolkitHost->GetTabManager().ToSharedRef());
		RegisterModeTabSpawners();
		OnToolkitHostReadyForUI.ExecuteIfBound();
	}
}

void FTLLRN_UVEditorModeUILayer::OnToolkitHostingFinished(const TSharedRef<IToolkit>& Toolkit)
{
	if (HostedToolkit.IsValid() && HostedToolkit.Pin() == Toolkit)
	{
		FAssetEditorModeUILayer::OnToolkitHostingFinished(Toolkit);
	}
}

TSharedPtr<FWorkspaceItem> FTLLRN_UVEditorModeUILayer::GetModeMenuCategory() const
{
	check(TLLRN_UVEditorMenuCategory);
	return TLLRN_UVEditorMenuCategory;
}

void FTLLRN_UVEditorModeUILayer::SetModeMenuCategory(TSharedPtr<FWorkspaceItem> MenuCategoryIn)
{
	TLLRN_UVEditorMenuCategory = MenuCategoryIn;
}


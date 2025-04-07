// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/TLLRN_RigHierarchyTabSummoner.h"
#include "Editor/STLLRN_RigHierarchy.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "Editor/TLLRN_ControlRigEditor.h"
#include "Editor/STLLRN_RigHierarchy.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "TLLRN_RigHierarchyTabSummoner"

const FName FTLLRN_RigHierarchyTabSummoner::TabID(TEXT("TLLRN_RigHierarchy"));

FTLLRN_RigHierarchyTabSummoner::FTLLRN_RigHierarchyTabSummoner(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor)
	: FWorkflowTabFactory(TabID, InTLLRN_ControlRigEditor)
	, TLLRN_ControlRigEditor(InTLLRN_ControlRigEditor)
{
	TabLabel = LOCTEXT("TLLRN_RigHierarchyTabLabel", "Rig Hierarchy");
	TabIcon = FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "TLLRN_RigHierarchy.TabIcon");

	ViewMenuDescription = LOCTEXT("TLLRN_RigHierarchy_ViewMenu_Desc", "Rig Hierarchy");
	ViewMenuTooltip = LOCTEXT("TLLRN_RigHierarchy_ViewMenu_ToolTip", "Show the Rig Hierarchy tab");
}

FTabSpawnerEntry& FTLLRN_RigHierarchyTabSummoner::RegisterTabSpawner(TSharedRef<FTabManager> InTabManager, const FApplicationMode* CurrentApplicationMode) const
{
	FTabSpawnerEntry& SpawnerEntry = FWorkflowTabFactory::RegisterTabSpawner(InTabManager, CurrentApplicationMode);

	SpawnerEntry.SetReuseTabMethod(FOnFindTabToReuse::CreateLambda([](const FTabId& InTabId) ->TSharedPtr<SDockTab> {
	
		return TSharedPtr<SDockTab>();

	}));

	return SpawnerEntry;
}

TSharedRef<SWidget> FTLLRN_RigHierarchyTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TLLRN_ControlRigEditor.Pin()->TLLRN_RigHierarchyTabCount++;
	return SNew(STLLRN_RigHierarchy, TLLRN_ControlRigEditor.Pin().ToSharedRef());
}

TSharedRef<SDockTab> FTLLRN_RigHierarchyTabSummoner::SpawnTab(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedRef<SDockTab>  DockTab = FWorkflowTabFactory::SpawnTab(Info);
	TWeakPtr<SDockTab> WeakDockTab = DockTab;
	DockTab->SetCanCloseTab(SDockTab::FCanCloseTab::CreateLambda([WeakDockTab]()
    {
		int32 HierarchyTabCount = 0;
		if (TSharedPtr<SDockTab> SharedDocTab = WeakDockTab.Pin())
		{
			if(SWidget* Content = &SharedDocTab->GetContent().Get())
			{
				STLLRN_RigHierarchy* TLLRN_RigHierarchy = (STLLRN_RigHierarchy*)Content;
				if(FTLLRN_ControlRigEditor* TLLRN_ControlRigEditorForTab = TLLRN_RigHierarchy->GetTLLRN_ControlRigEditor())
				{
					HierarchyTabCount = TLLRN_ControlRigEditorForTab->GetTLLRN_RigHierarchyTabCount();
				}
			}
		}
		return HierarchyTabCount > 0;
    }));
	DockTab->SetOnTabClosed( SDockTab::FOnTabClosedCallback::CreateLambda([](TSharedRef<SDockTab> DockTab)
	{
		if(SWidget* Content = &DockTab->GetContent().Get())
		{
			STLLRN_RigHierarchy* TLLRN_RigHierarchy = (STLLRN_RigHierarchy*)Content;
			if(FTLLRN_ControlRigEditor* TLLRN_ControlRigEditorForTab = TLLRN_RigHierarchy->GetTLLRN_ControlRigEditor())
			{
				TLLRN_ControlRigEditorForTab->TLLRN_RigHierarchyTabCount--;
			}
		}
	}));
	return DockTab;
}

#undef LOCTEXT_NAMESPACE 

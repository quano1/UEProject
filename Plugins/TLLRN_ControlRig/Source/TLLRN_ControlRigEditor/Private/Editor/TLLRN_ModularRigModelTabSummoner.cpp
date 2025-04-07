// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/TLLRN_ModularRigModelTabSummoner.h"
#include "Editor/STLLRN_ModularRigModel.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "Editor/TLLRN_ControlRigEditor.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "TLLRN_ModularTLLRN_RigHierarchyTabSummoner"

const FName FTLLRN_ModularRigModelTabSummoner::TabID(TEXT("TLLRN_ModularRigModel"));

FTLLRN_ModularRigModelTabSummoner::FTLLRN_ModularRigModelTabSummoner(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor)
	: FWorkflowTabFactory(TabID, InTLLRN_ControlRigEditor)
	, TLLRN_ControlRigEditor(InTLLRN_ControlRigEditor)
{
	TabLabel = LOCTEXT("TLLRN_ModularTLLRN_RigHierarchyTabLabel", "Module Hierarchy");
	TabIcon = FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "TLLRN_ModularTLLRN_RigHierarchy.TabIcon");

	ViewMenuDescription = LOCTEXT("TLLRN_ModularTLLRN_RigHierarchy_ViewMenu_Desc", "Module Hierarchy");
	ViewMenuTooltip = LOCTEXT("TLLRN_ModularTLLRN_RigHierarchy_ViewMenu_ToolTip", "Show the Module Hierarchy tab");
}

FTabSpawnerEntry& FTLLRN_ModularRigModelTabSummoner::RegisterTabSpawner(TSharedRef<FTabManager> InTabManager, const FApplicationMode* CurrentApplicationMode) const
{
	FTabSpawnerEntry& SpawnerEntry = FWorkflowTabFactory::RegisterTabSpawner(InTabManager, CurrentApplicationMode);

	SpawnerEntry.SetReuseTabMethod(FOnFindTabToReuse::CreateLambda([](const FTabId& InTabId) ->TSharedPtr<SDockTab> {
	
		return TSharedPtr<SDockTab>();

	}));

	return SpawnerEntry;
}

TSharedRef<SWidget> FTLLRN_ModularRigModelTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TLLRN_ControlRigEditor.Pin()->TLLRN_ModularTLLRN_RigHierarchyTabCount++;
	return SNew(STLLRN_ModularRigModel, TLLRN_ControlRigEditor.Pin().ToSharedRef());
}

TSharedRef<SDockTab> FTLLRN_ModularRigModelTabSummoner::SpawnTab(const FWorkflowTabSpawnInfo& Info) const
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
				STLLRN_ModularRigModel* TLLRN_RigHierarchy = (STLLRN_ModularRigModel*)Content;
				if(FTLLRN_ControlRigEditor* TLLRN_ControlRigEditorForTab = TLLRN_RigHierarchy->GetTLLRN_ControlRigEditor())
				{
					HierarchyTabCount = TLLRN_ControlRigEditorForTab->GetTLLRN_ModularTLLRN_RigHierarchyTabCount();
				}
			}
		}
		return HierarchyTabCount > 0;
    }));
	DockTab->SetOnTabClosed( SDockTab::FOnTabClosedCallback::CreateLambda([](TSharedRef<SDockTab> DockTab)
	{
		if(SWidget* Content = &DockTab->GetContent().Get())
		{
			STLLRN_ModularRigModel* TLLRN_RigHierarchy = (STLLRN_ModularRigModel*)Content;
			if(FTLLRN_ControlRigEditor* TLLRN_ControlRigEditorForTab = TLLRN_RigHierarchy->GetTLLRN_ControlRigEditor())
			{
				TLLRN_ControlRigEditorForTab->TLLRN_ModularTLLRN_RigHierarchyTabCount--;
			}
		}
	}));
	return DockTab;
}

#undef LOCTEXT_NAMESPACE 

// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/TLLRN_ControlRigContextMenuContext.h"

#include "TLLRN_ControlRigBlueprint.h"
#include "Editor/TLLRN_ControlRigEditor.h"
#include "Framework/Application/SlateApplication.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigContextMenuContext)

FString FTLLRN_ControlRigTLLRN_RigHierarchyToGraphDragAndDropContext::GetSectionTitle() const
{
	TArray<FString> ElementNameStrings;
	for (const FTLLRN_RigElementKey& Element: DraggedElementKeys)
	{
		ElementNameStrings.Add(Element.Name.ToString());
	}
	FString SectionTitle = FString::Join(ElementNameStrings, TEXT(","));
	if(SectionTitle.Len() > 64)
	{
		SectionTitle = SectionTitle.Left(61) + TEXT("...");
	}
	return SectionTitle;
}

void UTLLRN_ControlRigContextMenuContext::Init(TWeakPtr<FTLLRN_ControlRigEditor> InTLLRN_ControlRigEditor, const FTLLRN_ControlRigMenuSpecificContext& InMenuSpecificContext)
{
	WeakTLLRN_ControlRigEditor = InTLLRN_ControlRigEditor;
	MenuSpecificContext = InMenuSpecificContext;
}

UTLLRN_ControlRigBlueprint* UTLLRN_ControlRigContextMenuContext::GetTLLRN_ControlRigBlueprint() const
{
	if (const TSharedPtr<FTLLRN_ControlRigEditor> Editor = WeakTLLRN_ControlRigEditor.Pin())
	{
		return Editor->GetTLLRN_ControlRigBlueprint();
	}
	
	return nullptr;
}

UTLLRN_ControlRig* UTLLRN_ControlRigContextMenuContext::GetTLLRN_ControlRig() const
{
	if (UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint())
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(RigBlueprint->GetObjectBeingDebugged()))
		{
			return TLLRN_ControlRig;
		}
	}
	return nullptr;
}

bool UTLLRN_ControlRigContextMenuContext::IsAltDown() const
{
	return FSlateApplication::Get().GetModifierKeys().IsAltDown();
}

FTLLRN_ControlRigTLLRN_RigHierarchyDragAndDropContext UTLLRN_ControlRigContextMenuContext::GetTLLRN_RigHierarchyDragAndDropContext()
{
	return MenuSpecificContext.TLLRN_RigHierarchyDragAndDropContext;
}

FTLLRN_ControlRigGraphNodeContextMenuContext UTLLRN_ControlRigContextMenuContext::GetGraphNodeContextMenuContext()
{
	return MenuSpecificContext.GraphNodeContextMenuContext;
}

FTLLRN_ControlRigTLLRN_RigHierarchyToGraphDragAndDropContext UTLLRN_ControlRigContextMenuContext::GetTLLRN_RigHierarchyToGraphDragAndDropContext()
{
	return MenuSpecificContext.TLLRN_RigHierarchyToGraphDragAndDropContext;
}

STLLRN_RigHierarchy* UTLLRN_ControlRigContextMenuContext::GetTLLRN_RigHierarchyPanel() const
{
	if (const TSharedPtr<STLLRN_RigHierarchy> TLLRN_RigHierarchyPanel = MenuSpecificContext.TLLRN_RigHierarchyPanel.Pin())
	{
		return TLLRN_RigHierarchyPanel.Get();
	}
	return nullptr;
}

STLLRN_ModularRigModel* UTLLRN_ControlRigContextMenuContext::GetTLLRN_ModularRigModelPanel() const
{
	if (const TSharedPtr<STLLRN_ModularRigModel> TLLRN_ModularRigModelPanel = MenuSpecificContext.TLLRN_ModularRigModelPanel.Pin())
	{
		return TLLRN_ModularRigModelPanel.Get();
	}
	return nullptr;
}

FTLLRN_ControlRigEditor* UTLLRN_ControlRigContextMenuContext::GetTLLRN_ControlRigEditor() const
{
	if (const TSharedPtr<FTLLRN_ControlRigEditor> Editor = WeakTLLRN_ControlRigEditor.Pin())
	{
		return Editor.Get();
	}
	return nullptr;
}


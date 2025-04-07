// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/TLLRN_ControlRigBaseDockableView.h"
#include "AssetRegistry/AssetData.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Styling/CoreStyle.h"
#include "ScopedTransaction.h"
#include "TLLRN_ControlRig.h"
#include "UnrealEdGlobals.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "EditorModeManager.h"
#include "ISequencer.h"


FTLLRN_ControlRigBaseDockableView::FTLLRN_ControlRigBaseDockableView()
{
}

FTLLRN_ControlRigBaseDockableView::~FTLLRN_ControlRigBaseDockableView()
{
	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		EditMode->OnTLLRN_ControlRigAddedOrRemoved().RemoveAll(this);
		EditMode->OnTLLRN_ControlRigSelected().RemoveAll(this);
	}
}

TArray<UTLLRN_ControlRig*> FTLLRN_ControlRigBaseDockableView::GetTLLRN_ControlRigs()  const
{
	TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		TLLRN_ControlRigs = EditMode->GetTLLRN_ControlRigsArray(false);
	}
	return TLLRN_ControlRigs;
}

void FTLLRN_ControlRigBaseDockableView::SetEditMode(FTLLRN_ControlRigEditMode& InEditMode)
{

	ModeTools = InEditMode.GetModeManager();
	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		EditMode->OnTLLRN_ControlRigAddedOrRemoved().RemoveAll(this);
		EditMode->OnTLLRN_ControlRigAddedOrRemoved().AddRaw(this, &FTLLRN_ControlRigBaseDockableView::HandleControlAdded);
		EditMode->OnTLLRN_ControlRigSelected().RemoveAll(this);
		EditMode->OnTLLRN_ControlRigSelected().AddRaw(this, &FTLLRN_ControlRigBaseDockableView::HandlElementSelected);
	}
}

void FTLLRN_ControlRigBaseDockableView::HandleControlAdded(UTLLRN_ControlRig* TLLRN_ControlRig, bool bIsAdded)
{
}

void FTLLRN_ControlRigBaseDockableView::HandleControlSelected(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* InControl, bool bSelected)
{
}

void FTLLRN_ControlRigBaseDockableView::HandlElementSelected(UTLLRN_ControlRig* TLLRN_ControlRig, const FTLLRN_RigElementKey& Key, bool bSelected)
{
	if (TLLRN_ControlRig)
	{
		if (Key.Type == ETLLRN_RigElementType::Control)
		{
			if (FTLLRN_RigBaseElement* BaseElement = TLLRN_ControlRig->GetHierarchy()->Find(Key))
			{
				if (FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(BaseElement))
				{
					HandleControlSelected(TLLRN_ControlRig, ControlElement, bSelected);
				}
			}
		}
	}
}


FTLLRN_ControlRigEditMode* FTLLRN_ControlRigBaseDockableView::GetEditMode() const
{
	FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
	return EditMode;
}

ISequencer* FTLLRN_ControlRigBaseDockableView::GetSequencer() const
{
	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		TWeakPtr<ISequencer> Sequencer = EditMode->GetWeakSequencer();
		return Sequencer.Pin().Get();
	}
	return nullptr;
}

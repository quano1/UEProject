// Copyright Epic Games, Inc. All Rights Reserved.
/**
* Base View for Dockable Control Rig Animation widgets Details/Outliner
*/
#pragma once

#include "CoreMinimal.h"
#include "EditorModeManager.h"


class UTLLRN_ControlRig;
class ISequencer;
class FTLLRN_ControlRigEditMode;
struct FTLLRN_RigControlElement;
struct FTLLRN_RigElementKey;

class FTLLRN_ControlRigBaseDockableView 
{
public:
	FTLLRN_ControlRigBaseDockableView();
	virtual ~FTLLRN_ControlRigBaseDockableView();
	TArray<UTLLRN_ControlRig*> GetTLLRN_ControlRigs() const;

	virtual void SetEditMode(FTLLRN_ControlRigEditMode& InEditMode);
	FTLLRN_ControlRigEditMode* GetEditMode() const;

protected:
	virtual void HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* InControl, bool bSelected);
	virtual void HandleControlAdded(UTLLRN_ControlRig* TLLRN_ControlRig, bool bIsAdded);

	void HandlElementSelected(UTLLRN_ControlRig* Subject, const FTLLRN_RigElementKey& Key, bool bSelected);

	ISequencer* GetSequencer() const;

	FEditorModeTools* ModeTools = nullptr;

};


// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FTLLRN_ControlRigEditor;
class UTLLRN_ControlRigBlueprint;
class UTLLRN_ControlRig;

class SAnimAttributeView;


class STLLRN_ControlRigAnimAttributeView : public SCompoundWidget
{
	SLATE_BEGIN_ARGS( STLLRN_ControlRigAnimAttributeView )
	{}
	
	SLATE_END_ARGS()

	~STLLRN_ControlRigAnimAttributeView();
	
	void Construct( const FArguments& InArgs, TSharedRef<FTLLRN_ControlRigEditor> InTLLRN_ControlRigEditor);

	void HandleSetObjectBeingDebugged(UObject* InObject);
	
	void StartObservingNewTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig);
	void StopObservingCurrentTLLRN_ControlRig();

	void HandleTLLRN_ControlRigPostForwardSolve(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName) const;
	
	TWeakPtr<FTLLRN_ControlRigEditor> TLLRN_ControlRigEditor;
	TWeakObjectPtr<UTLLRN_ControlRigBlueprint> TLLRN_ControlRigBlueprint;
	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRigBeingDebuggedPtr;
	TSharedPtr<SAnimAttributeView> AttributeView;
};

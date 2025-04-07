// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigComponentDetails.h"
#include "UObject/Class.h"
#include "TLLRN_ControlRigComponent.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigComponentDetails"

TSharedRef<IDetailCustomization> FTLLRN_ControlRigComponentDetails::MakeInstance()
{
	return MakeShareable( new FTLLRN_ControlRigComponentDetails );
}

void FTLLRN_ControlRigComponentDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
}

#undef LOCTEXT_NAMESPACE

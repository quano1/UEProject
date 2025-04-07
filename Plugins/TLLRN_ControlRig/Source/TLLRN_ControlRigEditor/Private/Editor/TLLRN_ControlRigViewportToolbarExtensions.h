// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SharedPointerFwd.h"
#include "UObject/NameTypes.h"

class FUICommandList;

namespace UE::TLLRN_ControlRig
{

void PopulateTLLRN_ControlRigViewportToolbarTransformsSubmenu(const FName InMenuName);
void PopulateTLLRN_ControlRigViewportToolbarSelectionSubmenu(const FName InMenuName);
void PopulateTLLRN_ControlRigViewportToolbarShowSubmenu(const FName InMenuName, const TSharedPtr<const FUICommandList>& InCommandList);

void RemoveTLLRN_ControlRigViewportToolbarExtensions();

} // namespace UE::TLLRN_ControlRig

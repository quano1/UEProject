// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigHitProxies.h"
#include "GenericPlatform/ICursor.h"

IMPLEMENT_HIT_PROXY(HTLLRN_IKRigEditorBoneProxy,HHitProxy);
IMPLEMENT_HIT_PROXY(HTLLRN_IKRigEditorGoalProxy,HHitProxy);

HTLLRN_IKRigEditorBoneProxy::HTLLRN_IKRigEditorBoneProxy(const FName& InBoneName)
	: HHitProxy(HPP_World)
	, BoneName(InBoneName)
{
}

EMouseCursor::Type HTLLRN_IKRigEditorBoneProxy::GetMouseCursor()
{
	return EMouseCursor::Crosshairs;
}

bool HTLLRN_IKRigEditorBoneProxy::AlwaysAllowsTranslucentPrimitives() const
{
	return true;
}

HTLLRN_IKRigEditorGoalProxy::HTLLRN_IKRigEditorGoalProxy(const FName& InGoalName)
	: HHitProxy(HPP_World)
	, GoalName(InGoalName)
{
}

EMouseCursor::Type HTLLRN_IKRigEditorGoalProxy::GetMouseCursor()
{
	return EMouseCursor::Crosshairs;
}

bool HTLLRN_IKRigEditorGoalProxy::AlwaysAllowsTranslucentPrimitives() const
{
	return true;
}


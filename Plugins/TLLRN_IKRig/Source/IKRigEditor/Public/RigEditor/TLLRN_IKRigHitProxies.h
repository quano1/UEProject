// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HitProxies.h"
#include "UObject/NameTypes.h"

#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#include "CoreMinimal.h"
#endif

struct HTLLRN_IKRigEditorBoneProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	FName BoneName;

	HTLLRN_IKRigEditorBoneProxy(const FName& InBoneName);

	virtual EMouseCursor::Type GetMouseCursor() override;
	virtual bool AlwaysAllowsTranslucentPrimitives() const override;
};

struct HTLLRN_IKRigEditorGoalProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	FName GoalName;
	
	HTLLRN_IKRigEditorGoalProxy(const FName& InGoalName);

	virtual EMouseCursor::Type GetMouseCursor() override;
	virtual bool AlwaysAllowsTranslucentPrimitives() const override;
};
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HitProxies.h"
#include "TLLRN_IKRetargetAnimInstanceProxy.h"

// allow bone selection to edit retarget pose
struct HTLLRN_IKRetargetEditorBoneProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	FName BoneName;
	int32 BoneIndex;
	ETLLRN_RetargetSourceOrTarget SourceOrTarget;
	
	HTLLRN_IKRetargetEditorBoneProxy(
		const FName& InBoneName,
		const int32 InBoneIndex,
		ETLLRN_RetargetSourceOrTarget InSourceOrTarget)
		: HHitProxy(HPP_World),
		BoneName(InBoneName),
		BoneIndex(InBoneIndex),
		SourceOrTarget(InSourceOrTarget){}

	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::Crosshairs;
	}

	virtual bool AlwaysAllowsTranslucentPrimitives() const override
	{
		return true;
	}
};

// select chains/goals to edit chain settings
struct HTLLRN_IKRetargetEditorChainProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	FName TargetChainName;
	
	HTLLRN_IKRetargetEditorChainProxy(const FName& InTargetChainName)
		: HHitProxy(HPP_World)
		, TargetChainName(InTargetChainName) {}

	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::Crosshairs;
	}

	virtual bool AlwaysAllowsTranslucentPrimitives() const override
	{
		return true;
	}
};

// select root control to edit root settings
struct HTLLRN_IKRetargetEditorRootProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();
	
	HTLLRN_IKRetargetEditorRootProxy()
		: HHitProxy(HPP_World){}

	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::Crosshairs;
	}

	virtual bool AlwaysAllowsTranslucentPrimitives() const override
	{
		return true;
	}
};
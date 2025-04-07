// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "TLLRN_ControlRigAnimInstance.generated.h"

struct FMeshPoseBoneIndex;

/** Proxy override for this UAnimInstance-derived class */
USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

public:
	FTLLRN_ControlRigAnimInstanceProxy()
	{
	}

	FTLLRN_ControlRigAnimInstanceProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{
	}

	virtual ~FTLLRN_ControlRigAnimInstanceProxy() override;

	// FAnimInstanceProxy interface
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;

	TMap<FMeshPoseBoneIndex, FTransform> StoredTransforms;
	TMap<FName, float> StoredCurves;
	UE::Anim::FHeapAttributeContainer StoredAttributes;
};

UCLASS(transient, NotBlueprintable)
class TLLRN_CONTROLRIG_API UTLLRN_ControlRigAnimInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

		FTLLRN_ControlRigAnimInstanceProxy* GetTLLRN_ControlRigProxyOnGameThread() { return &GetProxyOnGameThread <FTLLRN_ControlRigAnimInstanceProxy>(); }

protected:

	// UAnimInstance interface
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};
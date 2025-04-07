// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_TLLRN_ControlRigBase.h"
#include "AnimNode_TLLRN_ControlRig_ExternalSource.generated.h"

/**
 * Animation node that allows animation TLLRN_ControlRig output to be used in an animation graph
 */
USTRUCT()
struct TLLRN_CONTROLRIG_API FAnimNode_TLLRN_ControlRig_ExternalSource : public FAnimNode_TLLRN_ControlRigBase
{
	GENERATED_BODY()

	FAnimNode_TLLRN_ControlRig_ExternalSource();

	void SetTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig);
	virtual UTLLRN_ControlRig* GetTLLRN_ControlRig() const override;
	virtual TSubclassOf<UTLLRN_ControlRig> GetTLLRN_ControlRigClass() const override;
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;

private:
	UPROPERTY(transient)
	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;
};


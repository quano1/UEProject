// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimPreviewInstance.h"
#include "Rig/TLLRN_IKRigProcessor.h"
#include "AnimNodes/TLLRN_AnimNode_IKRig.h"

#include "TLLRN_IKRigAnimInstance.generated.h"

class UTLLRN_IKRigDefinition;

UCLASS(transient, NotBlueprintable)
class UTLLRN_IKRigAnimInstance : public UAnimPreviewInstance
{
	GENERATED_UCLASS_BODY()

public:

	void SetIKRigAsset(UTLLRN_IKRigDefinition* IKRigAsset);

	void SetProcessorNeedsInitialized();

	UTLLRN_IKRigProcessor* GetCurrentlyRunningProcessor();

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	UPROPERTY(Transient)
	FTLLRN_AnimNode_IKRig IKRigNode;
};

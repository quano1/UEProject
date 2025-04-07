// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Retargeter/TLLRN_IKRetargetProcessor.h"

#include "TLLRN_IKRetargetOps.generated.h"

#if WITH_EDITOR
class FTLLRN_IKRetargetEditorController;
#endif

UCLASS(abstract, hidecategories = UObject)
class TLLRN_IKRIG_API UTLLRN_RetargetOpBase : public UObject
{
	GENERATED_BODY()

public:

	UTLLRN_RetargetOpBase() { SetFlags(RF_Transactional); }
	
	// override to cache internal data when initializing the processor
	virtual bool Initialize(
	const UTLLRN_IKRetargetProcessor* Processor,
		const FTLLRN_RetargetSkeleton& SourceSkeleton,
		const FTargetSkeleton& TargetSkeleton,
		FTLLRN_IKRigLogger& Log) { return false; };

	// override to evaluate this operation and modify the output pose
	virtual void Run(
		const UTLLRN_IKRetargetProcessor* Processor,
		const TArray<FTransform>& InSourceGlobalPose,
		TArray<FTransform>& OutTargetGlobalPose){};

	UPROPERTY()
	bool bIsEnabled = true;

	bool bIsInitialized = false;
	
#if WITH_EDITOR
	// override to automate initial setup after being added to the stack
	virtual void OnAddedToStack(const UTLLRN_IKRetargeter* Asset) {};
	// override to give your operation a nice name to display in the UI
	virtual FText GetNiceName() const { return FText::FromString(TEXT("Default Op Name")); };
	// override to display a warning message in the op stack
	virtual FText WarningMessage() const { return FText::GetEmpty(); };
#endif
};

// wrapper around a TArray of Retarget Ops for display in details panel
UCLASS(BlueprintType)
class TLLRN_IKRIG_API UTLLRN_RetargetOpStack: public UObject
{
	GENERATED_BODY()
	
public:

	// stack of operations to run AFTER the main retarget phases (Order is: Root/IK/FK/Post) 
	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_RetargetOpBase>> RetargetOps;

	// pointer to editor for details customization
#if WITH_EDITOR
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;
#endif
};
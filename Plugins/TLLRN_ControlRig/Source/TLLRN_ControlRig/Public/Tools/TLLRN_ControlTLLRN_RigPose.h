// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
/**
*  Data To Store and Apply the Control Rig Pose
*/

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TransformNoScale.h"
#include "Engine/SkeletalMesh.h"
#include "TLLRN_ControlRigToolAsset.h"
#include "Tools/TLLRN_ControlTLLRN_RigPoseMirrorTable.h"
#include "Rigs/TLLRN_TLLRN_RigControlHierarchy.h"
#include "TLLRN_ControlTLLRN_RigPose.generated.h"

class UTLLRN_ControlRig;
/**
* The Data Stored For Each Control in A Pose.
*/
USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigControlCopy
{
	GENERATED_BODY()

		FTLLRN_RigControlCopy()
		: Name(NAME_None)
		, ControlType(ETLLRN_RigControlType::Transform)
		, ParentKey()
		, Value()
		, OffsetTransform(FTransform::Identity)
		, ParentTransform(FTransform::Identity)
		, LocalTransform(FTransform::Identity)
		, GlobalTransform(FTransform::Identity)

	{
	}

	FTLLRN_RigControlCopy(FTLLRN_RigControlElement* InControlElement, UTLLRN_RigHierarchy* InHierarchy)
	{
		Name = InControlElement->GetFName();
		ControlType = InControlElement->Settings.ControlType;
		Value = InHierarchy->GetControlValue(InControlElement, ETLLRN_RigControlValueType::Current);
		ParentKey = InHierarchy->GetFirstParent(InControlElement->GetKey());
		OffsetTransform = InHierarchy->GetControlOffsetTransform(InControlElement, ETLLRN_RigTransformType::CurrentLocal);

		ParentTransform = InHierarchy->GetParentTransform(InControlElement, ETLLRN_RigTransformType::CurrentGlobal);
		LocalTransform = InHierarchy->GetTransform(InControlElement, ETLLRN_RigTransformType::CurrentLocal);
		GlobalTransform = InHierarchy->GetTransform(InControlElement, ETLLRN_RigTransformType::CurrentGlobal);
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Names")
	FName Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Type")
	ETLLRN_RigControlType ControlType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Names")
	FTLLRN_RigElementKey ParentKey;

	UPROPERTY()
	FTLLRN_RigControlValue Value;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transforms")
	FTransform OffsetTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transforms")
	FTransform ParentTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transforms")
	FTransform LocalTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transforms")
	FTransform GlobalTransform;

};

/**
* The Data Stored For Each Pose and associated Functions to Store and Paste It
*/
USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_ControlTLLRN_RigControlPose
{
	GENERATED_USTRUCT_BODY()

	FTLLRN_ControlTLLRN_RigControlPose() {};
	FTLLRN_ControlTLLRN_RigControlPose(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bUseAll)
	{
		SavePose(InTLLRN_ControlRig, bUseAll);
	}
	~FTLLRN_ControlTLLRN_RigControlPose() {};

	void SavePose(UTLLRN_ControlRig* TLLRN_ControlRig, bool bUseAll);
	void PastePose(UTLLRN_ControlRig* TLLRN_ControlRig, bool bDoKey, bool bDoMirror);
	void SetControlMirrorTransform(bool bDoLocalSpace, UTLLRN_ControlRig* TLLRN_ControlRig, const FName& Name, bool bIsMatched,
		const FTransform& GlobalTrnnsform, const FTransform& LocalTransform, bool bNotify, const  FTLLRN_RigControlModifiedContext& Context, bool bSetupUndo);
	void PastePoseInternal(UTLLRN_ControlRig* TLLRN_ControlRig, bool bDoKey, bool bDoMirror, const TArray<FTLLRN_RigControlCopy>& ControlsToPaste);
	void BlendWithInitialPoses(FTLLRN_ControlTLLRN_RigControlPose& InitialPose, UTLLRN_ControlRig* TLLRN_ControlRig, bool bDoKey, bool bDoMirror, float BlendValue);

	bool ContainsName(const FName& Name) const;
	void ReplaceControlName(const FName& Name, const FName& NewName);
	TArray<FName> GetControlNames() const;

	void SetUpControlMap();
	TArray<FTLLRN_RigControlCopy> GetPoses() const {return CopyOfControls;};

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Controls")
	TArray<FTLLRN_RigControlCopy> CopyOfControls;

	//Cache of the Map, Used to make pasting faster.
	TMap<FName, int32>  CopyOfControlsNameToIndex;
};


/**
* An individual Pose made of Control Rig Controls
*/
UCLASS(BlueprintType)
class TLLRN_CONTROLRIG_API UTLLRN_ControlTLLRN_RigPoseAsset : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	//UOBJECT
	virtual void PostLoad() override;

	UFUNCTION(BlueprintCallable, Category = "Pose")
	void SavePose(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bUseAll);

	UFUNCTION(BlueprintCallable, Category = "Pose")
	void PastePose(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bDoKey = false, bool bDoMirror = false);
	
	UFUNCTION(BlueprintCallable, Category = "Pose")
	void SelectControls(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bDoMirror = false);

	TArray<FTLLRN_RigControlCopy> GetCurrentPose(UTLLRN_ControlRig* InTLLRN_ControlRig);

	UFUNCTION(BlueprintCallable, Category = "Pose")
	void GetCurrentPose(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_ControlTLLRN_RigControlPose& OutPose);

	UFUNCTION(BlueprintPure, Category = "Pose")
	TArray<FName> GetControlNames() const;

	UFUNCTION(BlueprintCallable, Category = "Pose")
	void ReplaceControlName(const FName& CurrentName, const FName& NewName);

	UFUNCTION(BlueprintPure, Category = "Pose")
	bool DoesMirrorMatch(UTLLRN_ControlRig* TLLRN_ControlRig, const FName& ControlName) const;

	void BlendWithInitialPoses(FTLLRN_ControlTLLRN_RigControlPose& InitialPose, UTLLRN_ControlRig* TLLRN_ControlRig, bool bDoKey, bool bdoMirror, float BlendValue);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pose")
	FTLLRN_ControlTLLRN_RigControlPose Pose;

};

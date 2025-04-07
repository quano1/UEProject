// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "TLLRN_ControlRig.h"
#include "Units/Hierarchy/TLLRN_RigUnit_AddBoneTransform.h"
#include "AdditiveTLLRN_ControlRig.generated.h"

class USkeletalMesh;
struct FReferenceSkeleton;
struct FSmartNameMapping;

/** Rig that allows additive layer editing per joint */
UCLASS(NotBlueprintable)
class TLLRN_CONTROLRIG_API UAdditiveTLLRN_ControlRig : public UTLLRN_ControlRig
{
	GENERATED_UCLASS_BODY()

public: 
	// BEGIN TLLRN_ControlRig
	virtual void Initialize(bool bInitTLLRN_RigUnits = true) override;
	virtual void InitializeVMs(bool bRequestInit = true) override { URigVMHost::Initialize(bRequestInit); }
	virtual bool InitializeVMs(const FName& InEventName) override { return URigVMHost::InitializeVM(InEventName); }
	virtual void InitializeVMsFromCDO() override { URigVMHost::InitializeFromCDO(); }
	virtual void RequestInitVMs() override { URigVMHost::RequestInit(); }
	virtual bool Execute_Internal(const FName& InEventName) override;
	// END TLLRN_ControlRig

	// utility function to 
	static FName GetControlName(const FName& InBoneName);
	static FName GetNullName(const FName& InBoneName);

private:
	// custom units that are used to execute this rig
	TArray<FTLLRN_RigUnit_AddBoneTransform> AddBoneTLLRN_RigUnits;

	/** Create TLLRN_RigElements - bone hierarchy and curves - from incoming skeleton */
	void CreateTLLRN_RigElements(const USkeletalMesh* InReferenceMesh);
	void CreateTLLRN_RigElements(const FReferenceSkeleton& InReferenceSkeleton, const USkeleton* InSkeleton);
};
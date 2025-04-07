// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "TLLRN_ControlRig.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetCurveValue.h"
#include "FKTLLRN_ControlRig.generated.h"

class USkeletalMesh;
struct FReferenceSkeleton;
struct FSmartNameMapping;
/** Structs used to specify which bones/curves/controls we should have active, since if all controls or active we can't passthrough some previous bone transform*/
struct FFKBoneCheckInfo
{
	FName BoneName;
	int32 BoneID;
	bool  bActive;
};

UENUM(Blueprintable)
enum class ETLLRN_ControlRigFKRigExecuteMode: uint8
{
	/** Replaces the current pose */
	Replace,

	/** Applies the authored pose as an additive layer */
	Additive,

	/** Sets the current pose without the use of offset transforms */
	Direct,

	/** MAX - invalid */
	Max UMETA(Hidden),
};

/** Rig that allows override editing per joint */
UCLASS(NotBlueprintable, Meta = (DisplayName = "FK Control Rig"))
class TLLRN_CONTROLRIG_API UFKTLLRN_ControlRig : public UTLLRN_ControlRig
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
	virtual void SetBoneInitialTransformsFromSkeletalMeshComponent(USkeletalMeshComponent* InSkelMeshComp, bool bUseAnimInstance = false) override;
	// END TLLRN_ControlRig

	// utility function to generate a valid control element name
	static FName GetControlName(const FName& InName, const ETLLRN_RigElementType& InType);
	// utility function to generate a target element name for control
	static FName GetControlTargetName(const FName& InName, const ETLLRN_RigElementType& InType);

	TArray<FName> GetControlNames();
	bool GetControlActive(int32 Index) const;
	void SetControlActive(int32 Index, bool bActive);
	void SetControlActive(const TArray<FFKBoneCheckInfo>& InBoneChecks);

	void SetApplyMode(ETLLRN_ControlRigFKRigExecuteMode InMode);
	void ToggleApplyMode();
	bool CanToggleApplyMode() const { return true; }
	bool IsAdditive() const override { return ApplyMode == ETLLRN_ControlRigFKRigExecuteMode::Additive; }
	ETLLRN_ControlRigFKRigExecuteMode GetApplyMode() const { return ApplyMode; }

	// Ensures that controls mask is updated according to contained TLLRN_ControlRig (control) elements
	void RefreshActiveControls();

	struct FTLLRN_RigElementInitializationOptions
	{	
		// Flag whether or not to generate a transform control for bones
		bool bGenerateBoneControls = true;
		// Flag whether or not to generate a float control for all curves in the hierarchy
		bool bGenerateCurveControls = true;
		
		// Flag whether or not to import all curves from SmartNameMapping
		bool bImportCurves = true;

		// Set of bone names to generate a transform control for
		TArray<FName> BoneNames;
		// Set of curve names to generate a float control for (requires bImportCurves to be false)
		TArray<FName> CurveNames;
	};
	void SetInitializationOptions(const FTLLRN_RigElementInitializationOptions& Options) { InitializationOptions = Options; }
	
private:

	/** Create TLLRN_RigElements - bone hierarchy and curves - from incoming skeleton */
	void CreateTLLRN_RigElements(const USkeletalMesh* InReferenceMesh);
	void CreateTLLRN_RigElements(const FReferenceSkeleton& InReferenceSkeleton, const USkeleton* InSkeleton);
	void SetControlOffsetsFromBoneInitials();

	UPROPERTY()
	TArray<bool> IsControlActive;

	UPROPERTY()
	ETLLRN_ControlRigFKRigExecuteMode ApplyMode;
	ETLLRN_ControlRigFKRigExecuteMode CachedToggleApplyMode;

	FTLLRN_RigElementInitializationOptions InitializationOptions;
	friend class FTLLRN_ControlRigInteractionTest;
};
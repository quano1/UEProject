// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/AdditiveTLLRN_ControlRig.h"
#include "Animation/SmartName.h"
#include "Engine/SkeletalMesh.h"
#include "ITLLRN_ControlRigObjectBinding.h"
#include "Components/SkeletalMeshComponent.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AdditiveTLLRN_ControlRig)

#define LOCTEXT_NAMESPACE "AdditiveTLLRN_ControlRig"

UAdditiveTLLRN_ControlRig::UAdditiveTLLRN_ControlRig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCopyHierarchyBeforeConstruction = false;
	bResetInitialTransformsBeforeConstruction = false;
}

FName UAdditiveTLLRN_ControlRig::GetControlName(const FName& InBoneName)
{
	if (InBoneName != NAME_None)
	{
		return FName(*(InBoneName.ToString() + TEXT("_CONTROL")));
	}

	// if bone name is coming as none, we don't append
	return NAME_None;
}

FName UAdditiveTLLRN_ControlRig::GetNullName(const FName& InBoneName)
{
	if (InBoneName != NAME_None)
	{
		return FName(*(InBoneName.ToString() + TEXT("_NULL")));
	}

	// if bone name is coming as none, we don't append
	return NAME_None;
}

bool UAdditiveTLLRN_ControlRig::Execute_Internal(const FName& InEventName)
{
	if (InEventName == FTLLRN_RigUnit_BeginExecution::EventName)
	{
		FTLLRN_ControlRigExecuteContext ExecuteContext;
		ExecuteContext.Hierarchy = GetHierarchy();
		ExecuteContext.TLLRN_ControlRig = this;
		ExecuteContext.SetEventName(InEventName);
		ExecuteContext.UnitContext = FTLLRN_RigUnitContext();

		for (FTLLRN_RigUnit_AddBoneTransform& Unit : AddBoneTLLRN_RigUnits)
		{
			FName ControlName = GetControlName(Unit.Bone);
			const int32 Index = GetHierarchy()->GetIndex(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control));
			Unit.Transform = GetHierarchy()->GetLocalTransform(Index);
			Unit.Execute(ExecuteContext);
		}
	}
	else if(InEventName == FTLLRN_RigUnit_PrepareForExecution::EventName)
	{
		if(const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = GetObjectBinding())
		{
			if (const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Binding->GetBoundObject()))
			{
				CreateTLLRN_RigElements(SkeletalMeshComponent->GetSkeletalMeshAsset());
			}
		}

		// add units and initialize
		AddBoneTLLRN_RigUnits.Reset();

		GetHierarchy()->ForEach<FTLLRN_RigBoneElement>([&](FTLLRN_RigBoneElement* BoneElement) -> bool
		{
			FTLLRN_RigUnit_AddBoneTransform NewUnit;
			NewUnit.Bone = BoneElement->GetFName();
			NewUnit.bPropagateToChildren = true;
			AddBoneTLLRN_RigUnits.Add(NewUnit);
			return true;
		});
	}

	return true;
}

void UAdditiveTLLRN_ControlRig::Initialize(bool bInitTLLRN_RigUnits /*= true*/)
{
	PostInitInstanceIfRequired();
	
	Super::Initialize(bInitTLLRN_RigUnits);

	if (GetObjectBinding() == nullptr)
	{
		return;
	}

	Execute_Internal(FTLLRN_RigUnit_PrepareForExecution::EventName);
}

void UAdditiveTLLRN_ControlRig::CreateTLLRN_RigElements(const FReferenceSkeleton& InReferenceSkeleton, const USkeleton* InSkeleton)
{
	PostInitInstanceIfRequired();
	
	GetHierarchy()->Reset();
	if (UTLLRN_RigHierarchyController* Controller = GetHierarchy()->GetController(true))
	{
		Controller->ImportBones(InReferenceSkeleton, NAME_None, false, false, true, false);

		if (InSkeleton)
		{
			InSkeleton->ForEachCurveMetaData([Controller](const FName& InCurveName, const FCurveMetaData& InMetaData)
			{
				Controller->AddCurve(InCurveName, 0.f, false);
			});
		}

		// add control for all bone hierarchy
		GetHierarchy()->ForEach<FTLLRN_RigBoneElement>([&](FTLLRN_RigBoneElement* BoneElement) -> bool
        {
            const FName BoneName = BoneElement->GetFName();
            const int32 ParentIndex = GetHierarchy()->GetFirstParent(BoneElement->GetIndex());
            const FName NullName = GetNullName(BoneName);// name conflict?
            const FName ControlName = GetControlName(BoneName); // name conflict?

            FTLLRN_RigElementKey NullKey;
		
            if (ParentIndex != INDEX_NONE)
            {
                FTransform GlobalTransform = GetHierarchy()->GetGlobalTransform(BoneElement->GetIndex());
                FTransform ParentTransform = GetHierarchy()->GetGlobalTransform(ParentIndex);
                FTransform LocalTransform = GlobalTransform.GetRelativeTransform(ParentTransform);
                NullKey = Controller->AddNull(NullName, GetHierarchy()->GetKey(ParentIndex), LocalTransform, false, false);
            }
            else
            {
                FTransform GlobalTransform = GetHierarchy()->GetGlobalTransform(BoneElement->GetIndex());
                NullKey = Controller->AddNull(NullName, FTLLRN_RigElementKey(), GlobalTransform, true, false);
            }

            FTLLRN_RigControlSettings Settings;
            Settings.DisplayName = BoneName;
			Settings.ControlType = ETLLRN_RigControlType::Transform;
            Controller->AddControl(ControlName, NullKey, Settings, FTLLRN_RigControlValue::Make(FTransform::Identity), FTransform::Identity, FTransform::Identity, false);

            return true;
        });
	}
}

void UAdditiveTLLRN_ControlRig::CreateTLLRN_RigElements(const USkeletalMesh* InReferenceMesh)
{
	if (InReferenceMesh)
	{
		const USkeleton* Skeleton = InReferenceMesh->GetSkeleton();
		CreateTLLRN_RigElements(InReferenceMesh->GetRefSkeleton(), Skeleton);
	}
}

#undef LOCTEXT_NAMESPACE




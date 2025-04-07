// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Highlevel/Hierarchy/TLLRN_RigUnit_CCDIK.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_CCDIK)

FTLLRN_RigUnit_CCDIK_Execute()
{
	FTLLRN_RigElementKeyCollection Items;
	if(WorkData.CachedItems.Num() == 0)
	{
		Items = FTLLRN_RigElementKeyCollection::MakeFromChain(
			ExecuteContext.Hierarchy,
			FTLLRN_RigElementKey(StartBone, ETLLRN_RigElementType::Bone),
			FTLLRN_RigElementKey(EffectorBone, ETLLRN_RigElementType::Bone),
			false /* reverse */
		);
	}

	// transfer the rotation limits
	TArray<FTLLRN_RigUnit_CCDIK_RotationLimitPerItem> RotationLimitsPerItem;
	for (int32 RotationLimitIndex = 0; RotationLimitIndex < RotationLimits.Num(); RotationLimitIndex++)
	{
		FTLLRN_RigUnit_CCDIK_RotationLimitPerItem RotationLimitPerItem;
		RotationLimitPerItem.Item = FTLLRN_RigElementKey(RotationLimits[RotationLimitIndex].Bone, ETLLRN_RigElementType::Bone);
		RotationLimitPerItem.Limit = RotationLimits[RotationLimitIndex].Limit;
		RotationLimitsPerItem.Add(RotationLimitPerItem);
	}

	FTLLRN_RigUnit_CCDIKPerItem::StaticExecute(
		ExecuteContext, 
		Items,
		EffectorTransform,
		Precision,
		Weight,
		MaxIterations,
		bStartFromTail,
		BaseRotationLimit,
		RotationLimitsPerItem,
		bPropagateToChildren,
		WorkData);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CCDIK::GetUpgradeInfo() const
{
	// this node is no longer supported and the upgrade path is too complex.
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_CCDIKPerItem_Execute()
{
	FTLLRN_RigUnit_CCDIKItemArray::StaticExecute(ExecuteContext, Items.Keys, EffectorTransform, Precision, Weight, MaxIterations, bStartFromTail, BaseRotationLimit, RotationLimits, bPropagateToChildren, WorkData);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CCDIKPerItem::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_CCDIKItemArray NewNode;
	NewNode.Items = Items.Keys;
	NewNode.EffectorTransform = EffectorTransform;
	NewNode.Precision = Precision;
	NewNode.Weight = Weight;
	NewNode.MaxIterations = MaxIterations;
	NewNode.bStartFromTail = bStartFromTail;
	NewNode.BaseRotationLimit = BaseRotationLimit;
	NewNode.RotationLimits = RotationLimits;
	NewNode.bPropagateToChildren = bPropagateToChildren;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_CCDIKItemArray_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return;
	}

	TArray<FCCDIKChainLink>& Chain = WorkData.Chain;
	TArray<FTLLRN_CachedTLLRN_RigElement>& CachedItems = WorkData.CachedItems;
	TArray<int32>& RotationLimitIndex = WorkData.RotationLimitIndex;
	TArray<float>& RotationLimitsPerItem = WorkData.RotationLimitsPerItem;
	FTLLRN_CachedTLLRN_RigElement& CachedEffector = WorkData.CachedEffector;

	if ((RotationLimits.Num() != RotationLimitIndex.Num()) &&
		(RotationLimitIndex.Num() > 0))
	{
		CachedItems.Reset();
		RotationLimitIndex.Reset();
		RotationLimitsPerItem.Reset();
		CachedEffector.Reset();
	}
	
	if (CachedItems.Num() == 0 && Items.Num() > 0)
	{
		CachedItems.Add(FTLLRN_CachedTLLRN_RigElement(Hierarchy->GetFirstParent(Items[0]), Hierarchy));

		for (FTLLRN_RigElementKey Item : Items)
		{
			CachedItems.Add(FTLLRN_CachedTLLRN_RigElement(Item, Hierarchy));
		}

		CachedEffector = CachedItems.Last();
		Chain.Reserve(CachedItems.Num());

		RotationLimitsPerItem.SetNumUninitialized(CachedItems.Num());
		for (const FTLLRN_RigUnit_CCDIK_RotationLimitPerItem& RotationLimit : RotationLimits)
		{
			FTLLRN_CachedTLLRN_RigElement BoneIndex(RotationLimit.Item, Hierarchy);
			int32 BoneIndexInLookup = CachedItems.Find(BoneIndex);
			RotationLimitIndex.Add(BoneIndexInLookup);
		}

		if (CachedItems.Num() < 2)
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("No bones found."));
		}
	}

	if (CachedItems.Num() > 1)
	{
		// Gather chain links. These are non zero length bones.
		Chain.Reset();
		
		for (int32 ChainIndex = 0; ChainIndex < CachedItems.Num(); ChainIndex++)
		{
			const FTransform& GlobalTransform = Hierarchy->GetGlobalTransform(CachedItems[ChainIndex]);
			const FTransform& LocalTransform = Hierarchy->GetLocalTransform(CachedItems[ChainIndex]);
			Chain.Add(FCCDIKChainLink(GlobalTransform, LocalTransform, ChainIndex));
		}

		for (float& Limit : RotationLimitsPerItem)
		{
			Limit = BaseRotationLimit;
		}
		
		for (int32 LimitIndex = 0; LimitIndex < RotationLimitIndex.Num(); LimitIndex++)
		{
			if (RotationLimitIndex[LimitIndex] != INDEX_NONE)
			{
				RotationLimitsPerItem[RotationLimitIndex[LimitIndex]] = RotationLimits[LimitIndex].Limit;
			}
		}

		bool bBoneLocationUpdated = AnimationCore::SolveCCDIK(Chain, EffectorTransform.GetLocation(), Precision, MaxIterations, bStartFromTail, RotationLimits.Num() > 0, RotationLimitsPerItem);

		// If we moved some bones, update bone transforms.
		if (bBoneLocationUpdated)
		{
			if (FMath::IsNearlyEqual(Weight, 1.f))
			{
				for (int32 LinkIndex = 0; LinkIndex < CachedItems.Num(); LinkIndex++)
				{
					const FCCDIKChainLink& CurrentLink = Chain[LinkIndex];
					Hierarchy->SetGlobalTransform(CachedItems[LinkIndex], CurrentLink.Transform, bPropagateToChildren);
				}
			}
			else
			{
				float T = FMath::Clamp<float>(Weight, 0.f, 1.f);

				for (int32 LinkIndex = 0; LinkIndex < CachedItems.Num(); LinkIndex++)
				{
					const FCCDIKChainLink& CurrentLink = Chain[LinkIndex];
					FTransform PreviousXfo = Hierarchy->GetGlobalTransform(CachedItems[LinkIndex]);
					FTransform Xfo = FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousXfo, CurrentLink.Transform, T);
					Hierarchy->SetGlobalTransform(CachedItems[LinkIndex], Xfo, bPropagateToChildren);
				}
			}
		}

		if (FMath::IsNearlyEqual(Weight, 1.f))
		{
			Hierarchy->SetGlobalTransform(CachedEffector, EffectorTransform, bPropagateToChildren);
		}
		else
		{
			float T = FMath::Clamp<float>(Weight, 0.f, 1.f);
			FTransform PreviousXfo = Hierarchy->GetGlobalTransform(CachedEffector);
			FTransform Xfo = FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousXfo, EffectorTransform, T);
			Hierarchy->SetGlobalTransform(CachedEffector, Xfo, bPropagateToChildren);
		}
	}
}


// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Highlevel/Hierarchy/TLLRN_RigUnit_SlideChain.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SlideChain)

FTLLRN_RigUnit_SlideChain_Execute()
{
	FTLLRN_RigElementKeyCollection Items;
	if(WorkData.CachedItems.Num() == 0)
	{
		Items = FTLLRN_RigElementKeyCollection::MakeFromChain(
			ExecuteContext.Hierarchy,
			FTLLRN_RigElementKey(StartBone, ETLLRN_RigElementType::Bone),
			FTLLRN_RigElementKey(EndBone, ETLLRN_RigElementType::Bone),
			false /* reverse */
		);
	}

	FTLLRN_RigUnit_SlideChainPerItem::StaticExecute(
		ExecuteContext, 
		Items,
		SlideAmount,
		bPropagateToChildren,
		WorkData);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SlideChain::GetUpgradeInfo() const
{
	// this node is no longer supported and the upgrade path is too complex.
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_SlideChainPerItem_Execute()
{
	FTLLRN_RigUnit_SlideChainItemArray::StaticExecute(ExecuteContext, Items.Keys, SlideAmount, bPropagateToChildren, WorkData);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SlideChainPerItem::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_SlideChainItemArray NewNode;
	NewNode.Items = Items.Keys;
	NewNode.SlideAmount = SlideAmount;
	NewNode.bPropagateToChildren = bPropagateToChildren;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_SlideChainItemArray_Execute()
{
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return;
	}

	float& ChainLength = WorkData.ChainLength;
	TArray<float>& ItemSegments = WorkData.ItemSegments;
	TArray<FTLLRN_CachedTLLRN_RigElement>& CachedItems = WorkData.CachedItems;
	TArray<FTransform>& Transforms = WorkData.Transforms;
	TArray<FTransform>& BlendedTransforms = WorkData.BlendedTransforms;

	if(CachedItems.Num() == 0 && Items.Num() > 1)
	{
		ItemSegments.Reset();
		Transforms.Reset();
		BlendedTransforms.Reset();
		ChainLength = 0.f;

		for (FTLLRN_RigElementKey Item : Items)
		{
			CachedItems.Add(FTLLRN_CachedTLLRN_RigElement(Item, Hierarchy));
		}

		if (CachedItems.Num() < 2)
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Didn't find enough bones. You need at least two in the chain!"));
			return;
		}

		ItemSegments.SetNumZeroed(CachedItems.Num());
		ItemSegments[0] = 0;
		for (int32 Index = 1; Index < CachedItems.Num(); Index++)
		{
			FVector A = Hierarchy->GetGlobalTransform(CachedItems[Index - 1]).GetLocation();
			FVector B = Hierarchy->GetGlobalTransform(CachedItems[Index]).GetLocation();
			ItemSegments[Index] = (A - B).Size();
			ChainLength += ItemSegments[Index];
		}

		Transforms.SetNum(CachedItems.Num());
		BlendedTransforms.SetNum(CachedItems.Num());
	}

	if (CachedItems.Num() < 2 || ChainLength < SMALL_NUMBER)
	{
		return;
	}

	for (int32 Index = 0; Index < CachedItems.Num(); Index++)
	{
		Transforms[Index] = Hierarchy->GetGlobalTransform(CachedItems[Index]);
	}

	for (int32 Index = 0; Index < Transforms.Num(); Index++)
	{
		int32 TargetIndex = Index;
		float Ratio = 0.f;
		float SlidePerBone = -SlideAmount * ChainLength;

		if (SlidePerBone > 0)
		{
			while (SlidePerBone > SMALL_NUMBER && TargetIndex < Transforms.Num() - 1)
			{
				TargetIndex++;
				SlidePerBone -= ItemSegments[TargetIndex];
			}
		}
		else
		{
			while (SlidePerBone < -SMALL_NUMBER && TargetIndex > 0)
			{
				SlidePerBone += ItemSegments[TargetIndex];
				TargetIndex--;
			}
		}

		if (TargetIndex < Transforms.Num() - 1)
		{
			if (ItemSegments[TargetIndex + 1] > SMALL_NUMBER)
			{
				if (SlideAmount < -SMALL_NUMBER)
				{
					Ratio = FMath::Clamp<float>(1.f - FMath::Abs<float>(SlidePerBone / ItemSegments[TargetIndex + 1]), 0.f, 1.f);
				}
				else
				{
					Ratio = FMath::Clamp<float>(SlidePerBone / ItemSegments[TargetIndex + 1], 0.f, 1.f);
				}
			}
		}

		BlendedTransforms[Index] = Transforms[TargetIndex];
		if (TargetIndex < Transforms.Num() - 1 && Ratio > SMALL_NUMBER && Ratio < 1.f - SMALL_NUMBER)
		{
			BlendedTransforms[Index] = FTLLRN_ControlRigMathLibrary::LerpTransform(BlendedTransforms[Index], Transforms[TargetIndex + 1], Ratio);
		}
	}

	for (int32 Index = 0; Index < CachedItems.Num(); Index++)
	{
		if (Index < CachedItems.Num() - 1)
		{
			FVector CurrentX = BlendedTransforms[Index].GetRotation().GetAxisX();
			FVector DesiredX = BlendedTransforms[Index + 1].GetLocation() - BlendedTransforms[Index].GetLocation();
			FQuat OffsetQuat = FQuat::FindBetweenVectors(CurrentX, DesiredX);
			BlendedTransforms[Index].SetRotation(OffsetQuat * BlendedTransforms[Index].GetRotation());
		}
		Hierarchy->SetGlobalTransform(CachedItems[Index], BlendedTransforms[Index], bPropagateToChildren);
	}
}


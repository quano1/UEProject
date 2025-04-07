// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Highlevel/Hierarchy/TLLRN_RigUnit_ModifyTransforms.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_ModifyTransforms)

FTLLRN_RigUnit_ModifyTransforms_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	TArray<FTLLRN_CachedTLLRN_RigElement>& CachedItems = WorkData.CachedItems;

	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (CachedItems.Num() == 0)
		{
			CachedItems.SetNum(ItemToModify.Num());
		}

		float Minimum = FMath::Min<float>(WeightMinimum, WeightMaximum);
		float Maximum = FMath::Max<float>(WeightMinimum, WeightMaximum);

		if (Weight <= Minimum + SMALL_NUMBER || FMath::IsNearlyEqual(Minimum, Maximum))
		{
			return;
		}

		if (CachedItems.Num() == ItemToModify.Num())
		{
			float T = FMath::Clamp<float>((Weight - Minimum) / (Maximum - Minimum), 0.f, 1.f);
			bool bNeedsBlend = T < 1.f - SMALL_NUMBER;

			int32 EntryIndex = 0;
			for (const FTLLRN_RigUnit_ModifyTransforms_PerItem& Entry : ItemToModify)
			{
				FTLLRN_CachedTLLRN_RigElement& CachedItem = CachedItems[EntryIndex];
				if (!CachedItem.UpdateCache(Entry.Item, Hierarchy))
				{
					continue;
				}

				FTransform Transform = Entry.Transform;

				switch (Mode)
				{
					case ETLLRN_ControlRigModifyBoneMode::OverrideLocal:
					{
						if (bNeedsBlend)
						{
							Transform = FTLLRN_ControlRigMathLibrary::LerpTransform(Hierarchy->GetLocalTransform(CachedItem), Transform, T);
						}
						Hierarchy->SetLocalTransform(CachedItem, Transform, true);
						break;
					}
					case ETLLRN_ControlRigModifyBoneMode::OverrideGlobal:
					{
						if (bNeedsBlend)
						{
							Transform = FTLLRN_ControlRigMathLibrary::LerpTransform(Hierarchy->GetGlobalTransform(CachedItem), Transform, T);
						}
						Hierarchy->SetGlobalTransform(CachedItem, Transform, true);
						break;
					}
					case ETLLRN_ControlRigModifyBoneMode::AdditiveLocal:
					{
						if (bNeedsBlend)
						{
							Transform = FTLLRN_ControlRigMathLibrary::LerpTransform(FTransform::Identity, Transform, T);
						}
							
						FTLLRN_RigTransformElement* TransformElement = const_cast<FTLLRN_RigTransformElement*>(Cast<FTLLRN_RigTransformElement>(CachedItem.GetElement()));
						if(TransformElement == nullptr)
						{
							return;
						}

						// figure out which transform type has already been computed (is clean) to avoid compute
						ETLLRN_RigTransformType::Type TransformTypeToUse = ETLLRN_RigTransformType::CurrentLocal;
						if(TransformElement->GetDirtyState().IsDirty(ETLLRN_RigTransformType::CurrentLocal))
						{
							check(!TransformElement->GetDirtyState().IsDirty(ETLLRN_RigTransformType::CurrentGlobal));
							TransformTypeToUse = ETLLRN_RigTransformType::CurrentGlobal;
						}

						Transform = Transform * Hierarchy->GetTransform(TransformElement, TransformTypeToUse);
						Hierarchy->SetTransform(TransformElement, Transform, TransformTypeToUse, true);
						break;
					}
					case ETLLRN_ControlRigModifyBoneMode::AdditiveGlobal:
					{
						if (bNeedsBlend)
						{
							Transform = FTLLRN_ControlRigMathLibrary::LerpTransform(FTransform::Identity, Transform, T);
						}
						Transform = Hierarchy->GetGlobalTransform(CachedItem) * Transform;
						Hierarchy->SetGlobalTransform(CachedItem, Transform, true);
						break;
					}
					default:
					{
						break;
					}
				}
				EntryIndex++;
			}
		}
	}
}

#if WITH_EDITOR

bool FTLLRN_RigUnit_ModifyTransforms::GetDirectManipulationTargets(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, UTLLRN_RigHierarchy* InHierarchy, TArray<FRigDirectManipulationTarget>& InOutTargets, FString* OutFailureReason) const
{
	for(const FTLLRN_RigUnit_ModifyTransforms_PerItem& ItemInfo : ItemToModify)
	{
		if(ItemInfo.Item.IsValid())
		{
			static const UEnum* TypeEnum = StaticEnum<ETLLRN_RigElementType>();
			const FString Prefix = TypeEnum->GetDisplayNameTextByValue((int64)ItemInfo.Item.Type).ToString();
			InOutTargets.Emplace(FString::Printf(TEXT("%s %s"), *Prefix, *ItemInfo.Item.Name.ToString()), ETLLRN_RigControlType::EulerTransform);
		}
	}
	return !InOutTargets.IsEmpty();
}

bool FTLLRN_RigUnit_ModifyTransforms::UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	UTLLRN_RigHierarchy* Hierarchy = InContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return false;
	}

	const int32 Index = GetIndexFromTarget(InInfo->Target.Name);
	if (ItemToModify.IsValidIndex(Index))
	{
		if(!InInfo->bInitialized)
		{
			InInfo->OffsetTransform = FTransform::Identity;

			const FTLLRN_RigElementKey FirstParent = Hierarchy->GetFirstParent(ItemToModify[Index].Item);

			switch(Mode)
			{
				case ETLLRN_ControlRigModifyBoneMode::AdditiveLocal:
				{
					// place the control offset transform where the target is now "without" the change provided by the pin
					InInfo->OffsetTransform = ItemToModify[Index].Transform.Inverse() * Hierarchy->GetGlobalTransform(ItemToModify[Index].Item);
					break;
				}
				case ETLLRN_ControlRigModifyBoneMode::OverrideLocal:
				{
					// changing local means let's place the control offset transform where the parent is
					InInfo->OffsetTransform = Hierarchy->GetGlobalTransform(FirstParent);
					break;
				}
				case ETLLRN_ControlRigModifyBoneMode::AdditiveGlobal:
				case ETLLRN_ControlRigModifyBoneMode::OverrideGlobal:
				default:
				{
					// in this case the value input is a global transform
					// so we'll leave the control offset transform at identity
					break;
				}
			}

		}

		Hierarchy->SetControlOffsetTransform(InInfo->ControlKey, InInfo->OffsetTransform, false);
		Hierarchy->SetLocalTransform(InInfo->ControlKey, ItemToModify[Index].Transform, false);
		if(!InInfo->bInitialized)
		{
			Hierarchy->SetLocalTransform(InInfo->ControlKey, ItemToModify[Index].Transform, true);
		}
		return true;
	}
	return false;
}

bool FTLLRN_RigUnit_ModifyTransforms::UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	UTLLRN_RigHierarchy* Hierarchy = InContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return false;
	}

	const int32 Index = GetIndexFromTarget(InInfo->Target.Name);
	if (ItemToModify.IsValidIndex(Index))
	{
		ItemToModify[Index].Transform = Hierarchy->GetLocalTransform(InInfo->ControlKey);;
		return true;
	}
	return false;
}

TArray<const URigVMPin*> FTLLRN_RigUnit_ModifyTransforms::GetPinsForDirectManipulation(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget) const
{
	TArray<const URigVMPin*> AffectedPins;
	const int32 Index = GetIndexFromTarget(InTarget.Name);
	if (ItemToModify.IsValidIndex(Index))
	{
		if(const URigVMPin* ItemToModifyArrayPin = InNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_ModifyTransforms, ItemToModify)))
		{
			if(const URigVMPin* ItemToModifyElementPin = ItemToModifyArrayPin->FindSubPin(FString::FromInt(Index)))
			{
				if(const URigVMPin* TransformPin = ItemToModifyElementPin->FindSubPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_ModifyTransforms_PerItem, Transform)))
				{
					AffectedPins.Add(TransformPin);
				}
			}
		}
	}
	return AffectedPins;
}

int32 FTLLRN_RigUnit_ModifyTransforms::GetIndexFromTarget(const FString& InTarget) const
{
	FString Left, Right;
	if(InTarget.Split(TEXT(" "), &Left, &Right))
	{
		static const UEnum* TypeEnum = StaticEnum<ETLLRN_RigElementType>();
		static TArray<FString> DisplayNames;
		if(DisplayNames.IsEmpty())
		{
			const UEnum* ElementTypeEnum = StaticEnum<ETLLRN_RigElementType>();
			for(int64 Index = 0; ; Index++)
			{
				if ((ETLLRN_RigElementType)ElementTypeEnum->GetValueByIndex(Index) == ETLLRN_RigElementType::All)
				{
					break;
				}
				DisplayNames.Add(TypeEnum->GetDisplayNameTextByValue(Index).ToString());
			}
		}

		const int32 TypeIndex = DisplayNames.Find(Left);
		if(TypeIndex != INDEX_NONE)
		{
			const ETLLRN_RigElementType ElementType = (ETLLRN_RigElementType)TypeIndex;
			const FName ElementName(*Right);
			for(int32 Index = 0; Index < ItemToModify.Num(); Index++)
			{
				if(ItemToModify[Index].Item == FTLLRN_RigElementKey(ElementName, ElementType))
				{
					return Index;
				}
			}
		}
	}
	return INDEX_NONE;
}

#endif
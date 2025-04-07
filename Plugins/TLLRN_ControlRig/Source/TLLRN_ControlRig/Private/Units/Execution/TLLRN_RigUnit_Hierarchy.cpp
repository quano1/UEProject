// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Execution/TLLRN_RigUnit_Hierarchy.h"
#include "Units/Execution/TLLRN_RigUnit_Item.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Units/Execution/TLLRN_RigUnit_Collection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_Hierarchy)

FTLLRN_RigUnit_HierarchyGetParent_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (bDefaultParent)
	{
		if(CachedChild.IsIdentical(Child, ExecuteContext.Hierarchy))
		{
			Parent = CachedParent.GetKey();
		}
		else
		{
			Parent.Reset();
			CachedParent.Reset();

			if(CachedChild.UpdateCache(Child, ExecuteContext.Hierarchy))
			{
				Parent = ExecuteContext.Hierarchy->GetFirstParent(CachedChild.GetResolvedKey());
				if(Parent.IsValid())
				{
					CachedParent.UpdateCache(Parent, ExecuteContext.Hierarchy);
				}
			}
		}
	}
	else
	{
		CachedChild.UpdateCache(Child, ExecuteContext.Hierarchy);
		const FTLLRN_RigElementKey& ChildKey = CachedChild.GetResolvedKey();

		Parent = ExecuteContext.Hierarchy->GetActiveParent(ChildKey, false);
	}
}

FTLLRN_RigUnit_HierarchyGetParents_Execute()
{
	FTLLRN_RigUnit_HierarchyGetParentsItemArray::StaticExecute(ExecuteContext, Child, bIncludeChild, bReverse, true, Parents.Keys, CachedChild, CachedParents);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_HierarchyGetParents::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_HierarchyGetParentsItemArray NewNode;
	NewNode.Child = Child;
	NewNode.bIncludeChild = bIncludeChild;
	NewNode.bReverse = bReverse;
	NewNode.Parents = Parents.Keys;
	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_HierarchyGetParentsItemArray_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (bDefaultParent)
	{
		if(!CachedChild.IsIdentical(Child, ExecuteContext.Hierarchy))
		{
			CachedParents.Reset();

			if(CachedChild.UpdateCache(Child, ExecuteContext.Hierarchy))
			{
				TArray<FTLLRN_RigElementKey> Keys;
				FTLLRN_RigElementKey Parent = CachedChild.GetResolvedKey();
				do
				{
					if(bIncludeChild || Parent != Child)
					{
						Keys.Add(Parent);
					}
					Parent = ExecuteContext.Hierarchy->GetFirstParent(Parent);
				}
				while(Parent.IsValid());

				CachedParents = FTLLRN_RigElementKeyCollection(Keys);
				if(bReverse)
				{
					CachedParents = FTLLRN_RigElementKeyCollection::MakeReversed(CachedParents);
				}
			}
		}

		Parents = CachedParents.Keys;
	}
	else
	{
		CachedChild.UpdateCache(Child, ExecuteContext.Hierarchy);

		TArray<FTLLRN_RigElementKey> Keys;
		FTLLRN_RigElementKey Parent = CachedChild.GetResolvedKey();
		do
		{
			if(bIncludeChild || Parent != Child)
			{
				Keys.Add(Parent);
			}
			const FTLLRN_RigElementKey PreviousParent = Parent;

			Parent = ExecuteContext.Hierarchy->GetActiveParent(Parent, false);
		}
		while(Parent.IsValid());

		if(bReverse)
		{
			Algo::Reverse(Keys);
		}

		Parents = Keys;
	}
}

FTLLRN_RigUnit_HierarchyGetChildren_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if(!CachedParent.IsIdentical(Parent, ExecuteContext.Hierarchy))
	{
		CachedChildren.Reset();

		if(CachedParent.UpdateCache(Parent, ExecuteContext.Hierarchy))
		{
			const FTLLRN_RigElementKey ResolvedParent = CachedParent.GetResolvedKey();

			TArray<FTLLRN_RigElementKey> Keys;

			if(bIncludeParent)
			{
				Keys.Add(ResolvedParent);
			}

			Keys.Append(ExecuteContext.Hierarchy->GetChildren(ResolvedParent, bRecursive));

			CachedChildren = FTLLRN_RigElementKeyCollection(Keys);
		}
	}

	Children = CachedChildren;
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_HierarchyGetChildren::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_CollectionChildrenArray NewNode;
	NewNode.Parent = Parent;
	NewNode.bRecursive = bRecursive;
	NewNode.bIncludeParent = bIncludeParent;
	NewNode.TypeToSearch = ETLLRN_RigElementType::All;
	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_HierarchyGetSiblings_Execute()
{
	FTLLRN_RigUnit_HierarchyGetSiblingsItemArray::StaticExecute(ExecuteContext, Item, bIncludeItem, true, Siblings.Keys, CachedItem, CachedSiblings);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_HierarchyGetSiblings::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_HierarchyGetSiblingsItemArray NewNode;
	NewNode.Item = Item;
	NewNode.bIncludeItem = bIncludeItem;
	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_HierarchyGetSiblingsItemArray_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (bDefaultSiblings)
	{
		if(!CachedItem.IsIdentical(Item, ExecuteContext.Hierarchy))
		{
			CachedSiblings.Reset();

			if(CachedItem.UpdateCache(Item, ExecuteContext.Hierarchy))
			{
				TArray<FTLLRN_RigElementKey> Keys;

				FTLLRN_RigElementKey Parent = ExecuteContext.Hierarchy->GetFirstParent(CachedItem.GetResolvedKey());
				if(Parent.IsValid())
				{
					TArray<FTLLRN_RigElementKey> Children = ExecuteContext.Hierarchy->GetChildren(Parent, false);
					for(FTLLRN_RigElementKey Child : Children)
					{
						if(bIncludeItem || Child != Item)
						{
							Keys.Add(Child);
						}
					}
				}

				if(Keys.Num() == 0 && bIncludeItem)
				{
					Keys.Add(Item);
				}

				CachedSiblings = FTLLRN_RigElementKeyCollection(Keys);
			}
		}

		Siblings = CachedSiblings.Keys;
	}
	else
	{
		Siblings.Reset();
		
		const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
		FTLLRN_RigElementKey ParentKey = Hierarchy->GetActiveParent(Item, false);

		if (!ParentKey.IsValid())
		{
			ParentKey = UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey();
		}

		FTLLRN_RigBaseElementChildrenArray SiblingsPtrs = Hierarchy->GetActiveChildren(Hierarchy->Find(ParentKey), false);
		Algo::Transform(SiblingsPtrs, Siblings, [](const FTLLRN_RigBaseElement* Element) { return Element->GetKey(); });

		if (!bIncludeItem)
		{
			Siblings.Remove(Item);
		}
	}
}

FTLLRN_RigUnit_HierarchyGetChainItemArray_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if(!CachedStart.IsIdentical(Start, ExecuteContext.Hierarchy) ||
		!CachedEnd.IsIdentical(End, ExecuteContext.Hierarchy))
	{
		CachedChain.Reset();

		if(CachedStart.UpdateCache(Start, ExecuteContext.Hierarchy) &&
			CachedEnd.UpdateCache(End, ExecuteContext.Hierarchy))
		{
			TArray<FTLLRN_RigElementKey> Keys;

			const FTLLRN_RigTransformElement* StartElement = Cast<FTLLRN_RigTransformElement>(CachedStart.GetElement());
			if(StartElement && (StartElement->GetType() == ETLLRN_RigElementType::Socket))
			{
				StartElement = Cast<FTLLRN_RigTransformElement>(ExecuteContext.Hierarchy->GetFirstParent(StartElement));
			}
			const FTLLRN_RigTransformElement* EndElement = Cast<FTLLRN_RigTransformElement>(CachedEnd.GetElement());
			if(EndElement && (EndElement->GetType() == ETLLRN_RigElementType::Socket))
			{
				EndElement = Cast<FTLLRN_RigTransformElement>(ExecuteContext.Hierarchy->GetFirstParent(EndElement));
			}
			
			if(StartElement == nullptr || EndElement == nullptr)
			{
				Keys.Reset();
			}
			else
			{
				if(bIncludeEnd)
				{
					Keys.Add(EndElement->GetKey());
				}

				const FTLLRN_RigTransformElement* Parent = Cast<FTLLRN_RigTransformElement>(ExecuteContext.Hierarchy->GetFirstParent(EndElement));
				while(Parent && Parent != StartElement)
				{
					Keys.Add(Parent->GetKey());
					Parent = Cast<FTLLRN_RigTransformElement>(ExecuteContext.Hierarchy->GetFirstParent(Parent));
				}

				if(Parent != StartElement)
				{
					Keys.Reset();
#if WITH_EDITOR
					ExecuteContext.Report(EMessageSeverity::Info, ExecuteContext.GetFunctionName(), ExecuteContext.GetInstructionIndex(), TEXT("Start and End are not part of the same chain."));
#endif
				}
				else if(bIncludeStart)
				{
					Keys.Add(StartElement->GetKey());
				}

				CachedChain = FTLLRN_RigElementKeyCollection(Keys);

				// we are collecting the chain in reverse order (from tail to head)
				// so we'll reverse it again if we are expecting the chain to be in order parent to child.
				if(!bReverse)
				{
					CachedChain = FTLLRN_RigElementKeyCollection::MakeReversed(CachedChain);
				}
			}
		}
	}

	Chain = CachedChain.Keys;
}

FTLLRN_RigUnit_HierarchyGetPose_Execute()
{
	FTLLRN_RigUnit_HierarchyGetPoseItemArray::StaticExecute(ExecuteContext, Initial, ElementType, ItemsToGet.Keys, Pose);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_HierarchyGetPose::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_HierarchyGetPoseItemArray NewNode;
	NewNode.Initial = Initial;
	NewNode.ElementType = ElementType;
	NewNode.ItemsToGet = ItemsToGet.Keys;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_HierarchyGetPoseItemArray_Execute()
{
	Pose = ExecuteContext.Hierarchy->GetPose(Initial, ElementType, ItemsToGet);
}

FTLLRN_RigUnit_HierarchySetPose_Execute()
{
	FTLLRN_RigUnit_HierarchySetPoseItemArray::StaticExecute(ExecuteContext, Pose, ElementType, Space, ItemsToSet.Keys, Weight);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_HierarchySetPose::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_HierarchySetPoseItemArray NewNode;
	NewNode.Pose = Pose;
	NewNode.ElementType = ElementType;
	NewNode.Space = Space;
	NewNode.ItemsToSet = ItemsToSet.Keys;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_HierarchySetPoseItemArray_Execute()
{
	ExecuteContext.Hierarchy->SetPose(
		Pose,
		Space == ERigVMTransformSpace::GlobalSpace ? ETLLRN_RigTransformType::CurrentGlobal : ETLLRN_RigTransformType::CurrentLocal,
		ElementType,
		ItemsToSet,
		Weight
	);
}

FTLLRN_RigUnit_PoseIsEmpty_Execute()
{
	IsEmpty = Pose.Num() == 0;
}

FTLLRN_RigUnit_PoseGetItems_Execute()
{
	FTLLRN_RigUnit_PoseGetItemsItemArray::StaticExecute(ExecuteContext, Pose, ElementType, Items.Keys);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_PoseGetItems::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_PoseGetItemsItemArray NewNode;
	NewNode.Pose = Pose;
	NewNode.ElementType = ElementType;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_PoseGetItemsItemArray_Execute()
{
	Items.Reset();

	for(const FTLLRN_RigPoseElement& PoseElement : Pose)
	{
		// filter by type
		if (((uint8)ElementType & (uint8)PoseElement.Index.GetKey().Type) == 0)
		{
			continue;
		}
		Items.Add(PoseElement.Index.GetKey());
	}
}

FTLLRN_RigUnit_PoseGetDelta_Execute()
{
	PosesAreEqual = true;
	ItemsWithDelta.Reset();

	if(PoseA.Num() == 0 && PoseB.Num() == 0)
	{
		PosesAreEqual = true;
		return;
	}

	if(PoseA.Num() == 0 && PoseB.Num() != 0)
	{
		PosesAreEqual = false;
		FTLLRN_RigUnit_PoseGetItems::StaticExecute(ExecuteContext, PoseB, ElementType, ItemsWithDelta);
		return;
	}

	if(PoseA.Num() != 0 && PoseB.Num() == 0)
	{
		PosesAreEqual = false;
		FTLLRN_RigUnit_PoseGetItems::StaticExecute(ExecuteContext, PoseA, ElementType, ItemsWithDelta);
		return;
	}

	const float PositionU = FMath::Abs(PositionThreshold);
	const float RotationU = FMath::Abs(RotationThreshold);
	const float ScaleU = FMath::Abs(ScaleThreshold);
	const float CurveU = FMath::Abs(CurveThreshold);

	// if we should compare a sub set of things
	if(!ItemsToCompare.IsEmpty())
	{
		for(int32 Index = 0; Index < ItemsToCompare.Num(); Index++)
		{
			const FTLLRN_RigElementKey& Key = ItemsToCompare[Index];
			if (((uint8)ElementType & (uint8)Key.Type) == 0)
			{
				continue;
			}

			const int32 IndexA = PoseA.GetIndex(Key);
			if(IndexA == INDEX_NONE)
			{
				PosesAreEqual = false;
				ItemsWithDelta.Add(Key);
				continue;
			}
			
			const int32 IndexB = PoseB.GetIndex(Key);
			if(IndexB == INDEX_NONE)
			{
				PosesAreEqual = false;
				ItemsWithDelta.Add(Key);
				continue;
			}

			const FTLLRN_RigPoseElement& A = PoseA[IndexA];
			const FTLLRN_RigPoseElement& B = PoseB[IndexB];

			if(!ArePoseElementsEqual(A, B, Space, PositionU, RotationU, ScaleU, CurveU))
			{
				PosesAreEqual = false;
				ItemsWithDelta.Add(Key);
			}
		}		
	}
	
	// if the poses have the same hash we can accelerate this
	// since they are structurally the same
	else if(PoseA.PoseHash == PoseB.PoseHash)
	{
		for(int32 Index = 0; Index < PoseA.Num(); Index++)
		{
			const FTLLRN_RigPoseElement& A = PoseA[Index];
			const FTLLRN_RigPoseElement& B = PoseB[Index];
			const FTLLRN_RigElementKey& KeyA = A.Index.GetKey(); 
			const FTLLRN_RigElementKey& KeyB = B.Index.GetKey(); 
			check(KeyA == KeyB);
			
			if (((uint8)ElementType & (uint8)KeyA.Type) == 0)
			{
				continue;
			}

			if(!ArePoseElementsEqual(A, B, Space, PositionU, RotationU, ScaleU, CurveU))
			{
				PosesAreEqual = false;
				ItemsWithDelta.Add(KeyA);
			}
		}
	}
	
	// if the poses have different hashes they might not contain the same elements
	else
	{
		for(int32 IndexA = 0; IndexA < PoseA.Num(); IndexA++)
		{
			const FTLLRN_RigPoseElement& A = PoseA[IndexA];
			const FTLLRN_RigElementKey& KeyA = A.Index.GetKey(); 
			
			if (((uint8)ElementType & (uint8)KeyA.Type) == 0)
			{
				continue;
			}

			const int32 IndexB = PoseB.GetIndex(KeyA);
			if(IndexB == INDEX_NONE)
			{
				PosesAreEqual = false;
				ItemsWithDelta.Add(KeyA);
				continue;
			}

			const FTLLRN_RigPoseElement& B = PoseB[IndexB];
			
			if(!ArePoseElementsEqual(A, B, Space, PositionU, RotationU, ScaleU, CurveU))
			{
				PosesAreEqual = false;
				ItemsWithDelta.Add(KeyA);
			}
		}

		// let's loop the other way as well and find relevant
		// elements which are not part of this
		for(int32 IndexB = 0; IndexB < PoseB.Num(); IndexB++)
		{
			const FTLLRN_RigPoseElement& B = PoseB[IndexB];
			const FTLLRN_RigElementKey& KeyB = B.Index.GetKey(); 
			
			if (((uint8)ElementType & (uint8)KeyB.Type) == 0)
			{
				continue;
			}

			const int32 IndexA = PoseA.GetIndex(KeyB);
			if(IndexA == INDEX_NONE)
			{
				PosesAreEqual = false;
				ItemsWithDelta.AddUnique(KeyB);
			}
		}
	}
}

bool FTLLRN_RigUnit_PoseGetDelta::ArePoseElementsEqual(const FTLLRN_RigPoseElement& A, const FTLLRN_RigPoseElement& B,
                                                 ERigVMTransformSpace Space, float PositionU, float RotationU, float ScaleU, float CurveU)
{
	const FTLLRN_RigElementKey& KeyA = A.Index.GetKey();
	const FTLLRN_RigElementKey& KeyB = B.Index.GetKey();
	check(KeyA == KeyB);
		
	if(KeyA.Type == ETLLRN_RigElementType::Curve)
	{
		return AreCurvesEqual(A.CurveValue, B.CurveValue, CurveU);
	}

	return AreTransformsEqual(
		(Space == ERigVMTransformSpace::GlobalSpace) ? A.GlobalTransform : A.LocalTransform,
		(Space == ERigVMTransformSpace::GlobalSpace) ? B.GlobalTransform : B.LocalTransform,
		PositionU, RotationU, ScaleU);
}

bool FTLLRN_RigUnit_PoseGetDelta::AreTransformsEqual(const FTransform& A, const FTransform& B, float PositionU,
	float RotationU, float ScaleU)
{
	if(PositionU > SMALL_NUMBER)
	{
		const FVector PositionA = A.GetLocation();
		const FVector PositionB = B.GetLocation();
		const FVector Delta = (PositionA - PositionB).GetAbs();
		if( (Delta.X >= PositionU) ||
			(Delta.Y >= PositionU) ||
			(Delta.Z >= PositionU))
		{
			return false;
		}
	}

	if(RotationU > SMALL_NUMBER)
	{
		const FVector RotationA = A.GetRotation().Rotator().Euler();
		const FVector RotationB = B.GetRotation().Rotator().Euler();
		const FVector Delta = (RotationA - RotationB).GetAbs();
		if( (Delta.X >= RotationU) ||
			(Delta.Y >= RotationU) ||
			(Delta.Z >= RotationU))
		{
			return false;
		}
	}

	if(ScaleU > SMALL_NUMBER)
	{
		const FVector ScaleA = A.GetScale3D();
		const FVector ScaleB = B.GetScale3D();
		const FVector Delta = (ScaleA - ScaleB).GetAbs();
		if( (Delta.X >= ScaleU) ||
			(Delta.Y >= ScaleU) ||
			(Delta.Z >= ScaleU))
		{
			return false;
		}
	}

	return true;
}

bool FTLLRN_RigUnit_PoseGetDelta::AreCurvesEqual(float A, float B, float CurveU)
{
	if(CurveU > SMALL_NUMBER)
	{
		return FMath::Abs(A - B) < CurveU;
	}
	return true;
}

FTLLRN_RigUnit_PoseGetTransform_Execute()
{
	// set up defaults
	Valid = false;
	Transform = FTransform::Identity;

	if(CachedPoseHash != Pose.HierarchyTopologyVersion)
	{
		CachedPoseHash = Pose.PoseHash;
		CachedPoseElementIndex = Pose.GetIndex(Item);
	}

	if(CachedPoseElementIndex == INDEX_NONE)
	{
		return;
	}

	Valid = true;

	const FTLLRN_RigPoseElement& PoseElement = Pose[CachedPoseElementIndex];

	if(Space == ERigVMTransformSpace::GlobalSpace)
	{
		Transform = PoseElement.GlobalTransform;
	}
	else
	{
		Transform = PoseElement.LocalTransform;
	}
}

FTLLRN_RigUnit_PoseGetTransformArray_Execute()
{
	// set up defaults
	Valid = false;
	Transforms.Reset();

	Valid = true;

	Transforms.SetNum(Pose.Num());

	if(Space == ERigVMTransformSpace::GlobalSpace)
	{
		for(int32 Index=0; Index<Pose.Num(); Index++)
		{
			Transforms[Index] = Pose[Index].GlobalTransform;
		}
	}
	else
	{
		for(int32 Index=0; Index<Pose.Num(); Index++)
		{
			Transforms[Index] = Pose[Index].LocalTransform;
		}
	}
}

FTLLRN_RigUnit_PoseGetCurve_Execute()
{
	// set up defaults
	Valid = false;
	CurveValue = 0.f;

	if(CachedPoseHash != Pose.HierarchyTopologyVersion)
	{
		CachedPoseHash = Pose.PoseHash;
		CachedPoseElementIndex = Pose.GetIndex(FTLLRN_RigElementKey(Curve, ETLLRN_RigElementType::Curve));
	}

	if(CachedPoseElementIndex == INDEX_NONE)
	{
		return;
	}

	Valid = true;

	const FTLLRN_RigPoseElement& PoseElement = Pose[CachedPoseElementIndex];
	CurveValue = PoseElement.CurveValue;
}

FTLLRN_RigUnit_PoseLoop_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if(BlockToRun.IsNone())
	{
		Count = Pose.Num();
		Index = 0;
		BlockToRun = ExecuteContextName;
	}
	else if(BlockToRun == ExecuteContextName)
	{
		Index++;
	}

	if(Index == Count)
	{
		Item = FTLLRN_RigElementKey();
		GlobalTransform = LocalTransform = FTransform::Identity;
		CurveValue = 0.f;
		BlockToRun = ControlFlowCompletedName;
	}
	else
	{
		const FTLLRN_RigPoseElement& PoseElement = Pose.Elements[Index];
		Item = PoseElement.Index.GetKey();
		GlobalTransform = PoseElement.GlobalTransform;
		LocalTransform = PoseElement.LocalTransform;
		CurveValue = PoseElement.CurveValue;
	}

	Ratio = GetRatioFromIndex(Index, Count);
}

FTLLRN_RigUnit_HierarchyCreatePoseItemArray_Execute()
{
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if(Hierarchy == nullptr)
	{
		Pose.Reset();
		return;
	}
	
	Pose.Elements.Reset(Entries.Num());
	Pose.HierarchyTopologyVersion = Hierarchy->GetTopologyVersion();
	Pose.PoseHash = Pose.HierarchyTopologyVersion;

	for(const FTLLRN_RigUnit_HierarchyCreatePoseItemArray_Entry& Entry : Entries)
	{
		if(const FTLLRN_RigTransformElement* TransformElement = Hierarchy->Find<FTLLRN_RigTransformElement>(Entry.Item))
		{
			const FTLLRN_RigElementKey& Key = Entry.Item;
			FTLLRN_RigPoseElement Element;
			Element.Index = FTLLRN_CachedTLLRN_RigElement(Key, Hierarchy);
			Element.ActiveParent = Hierarchy->GetActiveParent(Key);
			Element.LocalTransform = Entry.LocalTransform;
			Element.GlobalTransform = Entry.GlobalTransform;
			Element.CurveValue = Entry.CurveValue;
			Element.PreferredEulerAngle = FVector::ZeroVector;

			if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(TransformElement))
			{
				if(Entry.UseEulerAngles)
				{
					Element.PreferredEulerAngle = Entry.EulerAngles;
				}
				else
				{
					FTLLRN_RigPreferredEulerAngles EulerAngles = ControlElement->PreferredEulerAngles;
					EulerAngles.SetRotator(Entry.LocalTransform.Rotator(), false, false);
					Element.PreferredEulerAngle = EulerAngles.GetAngles(false, EulerAngles.RotationOrder); 
				}
			}

			Pose.Elements.Add(Element);
			Pose.PoseHash = HashCombine(Pose.PoseHash, GetTypeHash(Key));
		}
	}
}
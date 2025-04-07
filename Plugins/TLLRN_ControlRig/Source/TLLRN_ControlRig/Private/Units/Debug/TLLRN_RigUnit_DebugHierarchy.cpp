// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Debug/TLLRN_RigUnit_DebugHierarchy.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "TLLRN_ControlRigDefines.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_DebugHierarchy)

FTLLRN_RigUnit_DebugHierarchy_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		DrawHierarchy(ExecuteContext, WorldOffset, Hierarchy, ETLLRN_ControlRigDrawHierarchyMode::Axes, Scale, Color, Thickness, nullptr, &Items);
	}
}

FTLLRN_RigUnit_DebugPose_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		FTLLRN_RigUnit_DebugHierarchy::DrawHierarchy(ExecuteContext, WorldOffset, Hierarchy, ETLLRN_ControlRigDrawHierarchyMode::Axes, Scale, Color, Thickness, &Pose, &Items);
	}
}

void FTLLRN_RigUnit_DebugHierarchy::DrawHierarchy(const FRigVMExecuteContext& InContext, const FTransform& WorldOffset, UTLLRN_RigHierarchy* Hierarchy, ETLLRN_ControlRigDrawHierarchyMode::Type Mode, float Scale, const FLinearColor& Color, float Thickness, const FTLLRN_RigPose* InPose, const TArrayView<const FTLLRN_RigElementKey>* InItems)
{
	FRigVMDrawInterface* DrawInterface = InContext.GetDrawInterface();
	if(DrawInterface == nullptr)
	{
		return;
	}
	
	if (!DrawInterface->IsEnabled())
	{
		return;
	}

	TMap<const FTLLRN_RigBaseElement*,  bool> ElementMap;
	if(InItems)
	{
		for(const FTLLRN_RigElementKey& Item : *InItems)
		{
			if(const FTLLRN_RigBaseElement* Element = Hierarchy->Find(Item))
			{
				ElementMap.Add(Element, true);
			}
		}
	}

	switch (Mode)
	{
		case ETLLRN_ControlRigDrawHierarchyMode::Axes:
		{
			FRigVMDrawInstruction InstructionX(ERigVMDrawSettings::Lines, FLinearColor::Red, Thickness, WorldOffset);
			FRigVMDrawInstruction InstructionY(ERigVMDrawSettings::Lines, FLinearColor::Green, Thickness, WorldOffset);
			FRigVMDrawInstruction InstructionZ(ERigVMDrawSettings::Lines, FLinearColor::Blue, Thickness, WorldOffset);
			FRigVMDrawInstruction InstructionParent(ERigVMDrawSettings::Lines, Color, Thickness, WorldOffset);
			InstructionX.Positions.Reserve(Hierarchy->Num() * 2);
			InstructionY.Positions.Reserve(Hierarchy->Num() * 2);
			InstructionZ.Positions.Reserve(Hierarchy->Num() * 2);
			InstructionParent.Positions.Reserve(Hierarchy->Num() * 6);

			Hierarchy->ForEach<FTLLRN_RigTransformElement>([&](FTLLRN_RigTransformElement* Child) -> bool
			{
				auto GetTransformLambda = [InPose, Hierarchy](FTLLRN_RigTransformElement* InElement, FTransform& Transform) -> bool
				{
					bool bValid = true;
					if(InPose)
					{
						const int32 ElementIndex = InPose->GetIndex(InElement->GetKey());
						if(ElementIndex != INDEX_NONE)
						{
							Transform = InPose->operator[](ElementIndex).GlobalTransform;
							return true;
						}
						else
						{
							bValid = false;
						}
					}

					Transform = Hierarchy->GetTransform(InElement, ETLLRN_RigTransformType::CurrentGlobal);
					return bValid;
				};

				// use the optional filter
				if(!ElementMap.IsEmpty() && !ElementMap.Contains(Child))
				{
					return true;
				}
				
				FTransform Transform = FTransform::Identity;
				if(!GetTransformLambda(Child, Transform))
				{
					return true;
				}

				const FVector P0 = Transform.GetLocation();
				const FVector PX = Transform.TransformPosition(FVector(Scale, 0.f, 0.f));
				const FVector PY = Transform.TransformPosition(FVector(0.f, Scale, 0.f));
				const FVector PZ = Transform.TransformPosition(FVector(0.f, 0.f, Scale));
				InstructionX.Positions.Add(P0);
				InstructionX.Positions.Add(PX);
				InstructionY.Positions.Add(P0);
				InstructionY.Positions.Add(PY);
				InstructionZ.Positions.Add(P0);
				InstructionZ.Positions.Add(PZ);

				FTLLRN_RigBaseElementParentArray Parents = Hierarchy->GetParents(Child);
				TArray<FTLLRN_RigElementWeight> Weights = Hierarchy->GetParentWeightArray(Child);
				
				for (int32 ParentIndex = 0; ParentIndex < Parents.Num(); ParentIndex++)
				{
					if(FTLLRN_RigTransformElement* ParentTransformElement = Cast<FTLLRN_RigTransformElement>(Parents[ParentIndex]))
					{
						// use the optional filter
						if(!ElementMap.IsEmpty() && !ElementMap.Contains(ParentTransformElement))
						{
							continue;
						}

						if(Weights.IsValidIndex(ParentIndex))
						{
							if(Weights[ParentIndex].IsAlmostZero())
							{
								continue;
							}
						}
						
						FTransform ParentTransform = FTransform::Identity;
						GetTransformLambda(ParentTransformElement, ParentTransform);

						const FVector P1 = ParentTransform.GetLocation();
						InstructionParent.Positions.Add(P0);
						InstructionParent.Positions.Add(P1);
					}
				}
				return true;
			});

			DrawInterface->DrawInstruction(InstructionX);
			DrawInterface->DrawInstruction(InstructionY);
			DrawInterface->DrawInstruction(InstructionZ);
			DrawInterface->DrawInstruction(InstructionParent);
			break;
		}
		default:
		{
			break;
		}
	}
}

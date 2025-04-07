// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Highlevel/Harmonics/TLLRN_RigUnit_BoneHarmonics.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "AnimationCoreLibrary.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_BoneHarmonics)

FTLLRN_RigUnit_BoneHarmonics_Execute()
{
	TArray<FTLLRN_RigUnit_Harmonics_TargetItem> Targets;
	for(int32 BoneIndex = 0;BoneIndex<Bones.Num();BoneIndex++)
	{
		FTLLRN_RigUnit_Harmonics_TargetItem Target;
		Target.Item = FTLLRN_RigElementKey(Bones[BoneIndex].Bone, ETLLRN_RigElementType::Bone);
		Target.Ratio = Bones[BoneIndex].Ratio;
		Targets.Add(Target);
	}


	FTLLRN_RigUnit_ItemHarmonics::StaticExecute(
		ExecuteContext, 
		Targets,
		WaveSpeed,
		WaveFrequency,
		WaveAmplitude,
		WaveOffset,
		WaveNoise,
		WaveEase,
		WaveMinimum,
		WaveMaximum,
		RotationOrder,
		WorkData);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_BoneHarmonics::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_ItemHarmonics NewNode;
	
	for(int32 BoneIndex = 0;BoneIndex<Bones.Num();BoneIndex++)
	{
		FTLLRN_RigUnit_Harmonics_TargetItem Target;
		Target.Item = FTLLRN_RigElementKey(Bones[BoneIndex].Bone, ETLLRN_RigElementType::Bone);
		Target.Ratio = Bones[BoneIndex].Ratio;
		NewNode.Targets.Add(Target);
	}

	NewNode.WaveSpeed = WaveSpeed;
	NewNode.WaveFrequency = WaveFrequency;
	NewNode.WaveAmplitude = WaveAmplitude;
	NewNode.WaveOffset = WaveOffset;
	NewNode.WaveNoise = WaveNoise;
	NewNode.WaveEase = WaveEase;
	NewNode.WaveMinimum = WaveMinimum;
	NewNode.WaveMaximum = WaveMaximum;
	NewNode.RotationOrder = RotationOrder;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	for(int32 BoneIndex = 0;BoneIndex<Bones.Num();BoneIndex++)
	{
		Info.AddRemappedPin(
			FString::Printf(TEXT("Bones.%d.Bone"), BoneIndex),
			FString::Printf(TEXT("Targets.%d.Item.Name"), BoneIndex));
	}
	return Info;
}

FTLLRN_RigUnit_ItemHarmonics_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return;
	}

	TArray<FTLLRN_CachedTLLRN_RigElement>& CachedItems = WorkData.CachedItems;
	FVector& WaveTime = WorkData.WaveTime;

	if (CachedItems.Num() != Targets.Num())
	{
		CachedItems.Reset(Targets.Num());
		
		WaveTime = FVector::ZeroVector;

	if (CachedItems.IsEmpty())
		for (int32 ItemIndex = 0; ItemIndex < Targets.Num(); ItemIndex++)
		{
			FTLLRN_CachedTLLRN_RigElement CachedItem(Targets[ItemIndex].Item, Hierarchy);
			if (!CachedItem.IsValid())
			{
				UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Item not found."));
			}
			CachedItems.Add(CachedItem);
		}
	}

	for (int32 ItemIndex = 0; ItemIndex < CachedItems.Num(); ItemIndex++)
	{
		if (CachedItems[ItemIndex].IsValid())
		{
			float Ease = FMath::Clamp<float>(Targets[ItemIndex].Ratio, 0.f, 1.f);
			Ease = FTLLRN_ControlRigMathLibrary::EaseFloat(Ease, WaveEase);
			Ease = FMath::Lerp<float>(WaveMinimum, WaveMaximum, Ease);

			FVector U = WaveTime + WaveFrequency * Targets[ItemIndex].Ratio;

			FVector Noise;
			Noise.X = FMath::PerlinNoise1D(U.X + 132.4f);
			Noise.Y = FMath::PerlinNoise1D(U.Y + 9.2f);
			Noise.Z = FMath::PerlinNoise1D(U.Z + 217.9f);
			Noise = Noise * WaveNoise * 2.f;
			U = U + Noise;

			FVector Angles;
			Angles.X = FMath::Sin(U.X + WaveOffset.X);
			Angles.Y = FMath::Sin(U.Y + WaveOffset.Y);
			Angles.Z = FMath::Sin(U.Z + WaveOffset.Z);
			Angles = Angles * WaveAmplitude * Ease;

			FQuat Rotation = AnimationCore::QuatFromEuler(Angles, RotationOrder);

			FTransform Transform = Hierarchy->GetGlobalTransform(CachedItems[ItemIndex]);
			Transform.SetRotation(Transform.GetRotation() * Rotation);
			Hierarchy->SetGlobalTransform(CachedItems[ItemIndex], Transform);
		}
	}

	WaveTime += WaveSpeed * ExecuteContext.GetDeltaTime();
}


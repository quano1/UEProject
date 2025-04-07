// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetControlTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetTransform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetControlTransform)

FTLLRN_RigUnit_SetControlBool_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Control, ETLLRN_RigElementType::Control);
		if (CachedControlIndex.UpdateCache(Key, Hierarchy))
		{
			Hierarchy->SetControlValue(CachedControlIndex, FTLLRN_RigControlValue::Make<bool>(BoolValue));
		}
	}
}

FTLLRN_RigUnit_SetMultiControlBool_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		CachedControlIndices.SetNum(Entries.Num());

		for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); EntryIndex++)
		{
			bool BoolValue = Entries[EntryIndex].BoolValue;

			FTLLRN_RigUnit_SetControlBool::StaticExecute(ExecuteContext, Entries[EntryIndex].Control, BoolValue, CachedControlIndices[EntryIndex]);
		}
	}
}

FTLLRN_RigUnit_SetControlFloat_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Control, ETLLRN_RigElementType::Control);
		if (CachedControlIndex.UpdateCache(Key, Hierarchy))
		{
			if(FMath::IsNearlyEqual((float)Weight, 1.f))
			{
				Hierarchy->SetControlValue(CachedControlIndex, FTLLRN_RigControlValue::Make<float>(FloatValue));
			}
			else
			{
				float PreviousValue = Hierarchy->GetControlValue(CachedControlIndex).Get<float>();
				Hierarchy->SetControlValue(CachedControlIndex, FTLLRN_RigControlValue::Make<float>(FMath::Lerp<float>(PreviousValue, FloatValue, FMath::Clamp<float>(Weight, 0.f, 1.f))));
			}
		}
	}
}

FTLLRN_RigUnit_SetMultiControlFloat_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{ 
		CachedControlIndices.SetNum(Entries.Num());
	 
		for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); EntryIndex++)
		{
			float FloatValue = Entries[EntryIndex].FloatValue;

			FTLLRN_RigUnit_SetControlFloat::StaticExecute(ExecuteContext, Entries[EntryIndex].Control, Weight, FloatValue, CachedControlIndices[EntryIndex]); 
		}
	}
}

FTLLRN_RigUnit_SetControlInteger_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Control, ETLLRN_RigElementType::Control);
		if (CachedControlIndex.UpdateCache(Key, Hierarchy))
		{
			if(FMath::IsNearlyEqual((float)Weight, 1.f))
			{
				Hierarchy->SetControlValue(CachedControlIndex, FTLLRN_RigControlValue::Make<int32>(IntegerValue));
			}
			else
			{
				int32 PreviousValue = Hierarchy->GetControlValue(CachedControlIndex).Get<int32>();
				Hierarchy->SetControlValue(CachedControlIndex, FTLLRN_RigControlValue::Make<int32>((int32)FMath::Lerp<float>((float)PreviousValue, (float)IntegerValue, FMath::Clamp<float>(Weight, 0.f, 1.f))));
			}
		}
	}
}

FTLLRN_RigUnit_SetMultiControlInteger_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		CachedControlIndices.SetNum(Entries.Num());

		for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); EntryIndex++)
		{
			int32 IntegerValue = Entries[EntryIndex].IntegerValue;

			FTLLRN_RigUnit_SetControlInteger::StaticExecute(ExecuteContext, Entries[EntryIndex].Control, Weight, IntegerValue, CachedControlIndices[EntryIndex]);
		}
	}
}

FTLLRN_RigUnit_SetControlVector2D_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Control, ETLLRN_RigElementType::Control);
		if (CachedControlIndex.UpdateCache(Key, Hierarchy))
		{
			const FVector3f CurrentValue(Vector.X, Vector.Y, 0.f);
			if(FMath::IsNearlyEqual((float)Weight, 1.f))
			{
				Hierarchy->SetControlValue(CachedControlIndex, FTLLRN_RigControlValue::Make<FVector3f>(CurrentValue));
			}
			else
			{
				const FVector3f PreviousValue = Hierarchy->GetControlValue(CachedControlIndex).Get<FVector3f>();
				Hierarchy->SetControlValue(CachedControlIndex, FTLLRN_RigControlValue::Make<FVector3f>(FMath::Lerp<FVector3f>(PreviousValue, CurrentValue, FMath::Clamp<float>(Weight, 0.f, 1.f))));
			}
		}
	}
}

FTLLRN_RigUnit_SetMultiControlVector2D_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		CachedControlIndices.SetNum(Entries.Num());

		for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); EntryIndex++)
		{
			FVector2D Vector = Entries[EntryIndex].Vector;

			FTLLRN_RigUnit_SetControlVector2D::StaticExecute(ExecuteContext, Entries[EntryIndex].Control, Weight, Vector, CachedControlIndices[EntryIndex]);
		}
	}
}

FTLLRN_RigUnit_SetControlVector_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Control, ETLLRN_RigElementType::Control);
		if (CachedControlIndex.UpdateCache(Key, Hierarchy))
		{
			const ETLLRN_RigControlType ControlType = Hierarchy->GetChecked<FTLLRN_RigControlElement>(CachedControlIndex)->Settings.ControlType;
			
			FTransform Transform = FTransform::Identity;
			if (Space == ERigVMTransformSpace::GlobalSpace)
			{
				Transform = Hierarchy->GetGlobalTransform(CachedControlIndex);
			}

			if (ControlType == ETLLRN_RigControlType::Position)
			{
				if(FMath::IsNearlyEqual((float)Weight, 1.f))
				{
					Transform.SetLocation(Vector);
				}
				else
				{
					FVector PreviousValue = Transform.GetLocation();
					Transform.SetLocation(FMath::Lerp<FVector>(PreviousValue, Vector, FMath::Clamp<float>(Weight, 0.f, 1.f)));
				}
			}
			else if (ControlType == ETLLRN_RigControlType::Scale)
			{
				if(FMath::IsNearlyEqual((float)Weight, 1.f))
				{
					Transform.SetScale3D(Vector);
				}
				else
				{
					FVector PreviousValue = Transform.GetScale3D();
					Transform.SetScale3D(FMath::Lerp<FVector>(PreviousValue, Vector, FMath::Clamp<float>(Weight, 0.f, 1.f)));
				}
			}

			switch (Space)
			{
				case ERigVMTransformSpace::GlobalSpace:
				{
					Hierarchy->SetGlobalTransform(CachedControlIndex, Transform);
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					Hierarchy->SetLocalTransform(CachedControlIndex, Transform);
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}
}

FTLLRN_RigUnit_SetControlRotator_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Control, ETLLRN_RigElementType::Control);
		if (CachedControlIndex.UpdateCache(Key, Hierarchy))
		{
			FTransform Transform = FTransform::Identity;
			if (Space == ERigVMTransformSpace::GlobalSpace)
			{
				Transform = Hierarchy->GetGlobalTransform(CachedControlIndex);
			}

			FQuat Quat = FQuat(Rotator);
			if (FMath::IsNearlyEqual((float)Weight, 1.f))
			{
				Transform.SetRotation(Quat);
			}
			else
			{
				FQuat PreviousValue = Transform.GetRotation();
				Transform.SetRotation(FQuat::Slerp(PreviousValue, Quat, FMath::Clamp<float>(Weight, 0.f, 1.f)));
			}
			Transform.NormalizeRotation();

			switch (Space)
			{
				case ERigVMTransformSpace::GlobalSpace:
				{
					Hierarchy->SetGlobalTransform(CachedControlIndex, Transform);
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					Hierarchy->SetLocalTransform(CachedControlIndex, Transform);
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}
}

FTLLRN_RigUnit_SetMultiControlRotator_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		CachedControlIndices.SetNum(Entries.Num());

		for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); EntryIndex++)
		{
			FRotator Rotator = Entries[EntryIndex].Rotator;

			FTLLRN_RigUnit_SetControlRotator::StaticExecute(ExecuteContext, Entries[EntryIndex].Control, Weight, Rotator, Entries[EntryIndex].Space , CachedControlIndices[EntryIndex]);
		}
	}
}

FTLLRN_RigUnit_SetControlTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Control, ETLLRN_RigElementType::Control);
		if (CachedControlIndex.UpdateCache(Key, Hierarchy))
		{
			switch (Space)
			{
				case ERigVMTransformSpace::GlobalSpace:
				{
					if(FMath::IsNearlyEqual((float)Weight, 1.f))
					{
						Hierarchy->SetGlobalTransform(CachedControlIndex, Transform);
					}
					else
					{
						FTransform PreviousTransform = Hierarchy->GetGlobalTransform(CachedControlIndex);
						Hierarchy->SetGlobalTransform(CachedControlIndex, FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, Transform, FMath::Clamp<float>(Weight, 0.f, 1.f)));
					}
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					if(FMath::IsNearlyEqual((float)Weight, 1.f))
					{
						Hierarchy->SetLocalTransform(CachedControlIndex, Transform);
					}
					else
					{
						FTransform PreviousTransform = Hierarchy->GetLocalTransform(CachedControlIndex);
						Hierarchy->SetLocalTransform(CachedControlIndex, FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, Transform, FMath::Clamp<float>(Weight, 0.f, 1.f)));
					}
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SetControlTransform::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_SetTransform NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control);
	NewNode.Space = Space;
	NewNode.Value = Transform;
	NewNode.Weight = Weight;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Control"), TEXT("Item.Name"));
	Info.AddRemappedPin(TEXT("Transform"), TEXT("Value"));
	return Info;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"
#include "RigVMFunctions/Math/RigVMFunction_MathTransform.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_SetMultiControlBool)
{
	FTLLRN_RigControlSettings Settings;
	Settings.ControlType = ETLLRN_RigControlType::Bool;
	
	Controller->AddControl(TEXT("Control1"), FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue(), FTransform::Identity, FTransform::Identity);
	Controller->AddControl(TEXT("Control2"), FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue(), FTransform::Identity, FTransform::Identity);
	Unit.ExecuteContext.Hierarchy = Hierarchy.Get();

	// this unit has an empty entry by default, clear it for testing purpose
	Unit.Entries.Reset();

	FTLLRN_RigUnit_SetMultiControlBool_Entry Entry;

	// functional test, whether the unit can set multiple controls
	Entry.Control = TEXT("Control1");
	Entry.BoolValue = true;
	Unit.Entries.Add(Entry);

	Entry.Control = TEXT("Control2");
	Entry.BoolValue = true;
	Unit.Entries.Add(Entry);
	AddErrorIfFalse(Unit.Entries.Num() == 2, TEXT("unexpected number of entries"));

	Execute();

	AddErrorIfFalse(Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control1"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<bool>() == true, TEXT("unexpected control value"));
	AddErrorIfFalse(Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control2"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<bool>() == true, TEXT("unexpected control value")); 

	return true;
}

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_SetMultiControlFloat)
{
	FTLLRN_RigControlSettings Settings;
	Settings.ControlType = ETLLRN_RigControlType::Float;
	
	Controller->AddControl(TEXT("Control1"), FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue(), FTransform::Identity, FTransform::Identity);
	Controller->AddControl(TEXT("Control2"), FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue(), FTransform::Identity, FTransform::Identity);
	Unit.ExecuteContext.Hierarchy = Hierarchy.Get();

	// this unit has an empty entry by default, clear it for testing purpose
	Unit.Entries.Reset();

	FTLLRN_RigUnit_SetMultiControlFloat_Entry Entry;

	// functional test, whether the unit can set multiple controls
	Entry.Control = TEXT("Control1");
	Entry.FloatValue = 10.0f;
	Unit.Entries.Add(Entry);

	Entry.Control = TEXT("Control2");
	Entry.FloatValue = 20.0f;
	Unit.Entries.Add(Entry);
	AddErrorIfFalse(Unit.Entries.Num() == 2, TEXT("unexpected number of entries"));

	Execute();

	AddErrorIfFalse(Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control1"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<float>() == 10.0f, TEXT("unexpected control value"));
	AddErrorIfFalse(Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control2"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<float>() == 20.0f, TEXT("unexpected control value"));

	// test if the caches were updated 
	AddErrorIfFalse(Unit.CachedControlIndices.Num() == 2, TEXT("unexpected number of cache entries"));
	AddErrorIfFalse(Unit.CachedControlIndices[0].GetKey().Name == Unit.Entries[0].Control, TEXT("unexpected cached control"));
	AddErrorIfFalse(Unit.CachedControlIndices[1].GetKey().Name == Unit.Entries[1].Control, TEXT("unexpected cached control"));

	// test if we are reallocating cache unnecessarily
	FTLLRN_CachedTLLRN_RigElement* AddressOfCachedElement1 = &Unit.CachedControlIndices[0];
	FTLLRN_CachedTLLRN_RigElement* AddressOfCachedElement2 = &Unit.CachedControlIndices[1];
	Execute();
	AddErrorIfFalse(AddressOfCachedElement1 == &Unit.CachedControlIndices[0], TEXT("unexpected cache reallocation"));
	AddErrorIfFalse(AddressOfCachedElement2 == &Unit.CachedControlIndices[1], TEXT("unexpected cache reallocation"));

	// test if the caches can be updated upon changing the entries
	Unit.Entries.Reset();

	Entry.Control = TEXT("Control2");
	Entry.FloatValue = 30.0f;
	Unit.Entries.Add(Entry);

	AddErrorIfFalse(Unit.Entries.Num() == 1, TEXT("unexpected number of entries"));

	Execute();

	AddErrorIfFalse(Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control2"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<float>() == 30.0f, TEXT("unexpected control value"));

	AddErrorIfFalse(Unit.CachedControlIndices.Num() == 1, TEXT("unexpected number of cache entries"));
	AddErrorIfFalse(Unit.CachedControlIndices[0].GetKey().Name == Unit.Entries[0].Control, TEXT("unexpected cached control"));

	return true;
}

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_SetMultiControlInteger)
{
	FTLLRN_RigControlSettings Settings;
	Settings.ControlType = ETLLRN_RigControlType::Integer;
	
	Controller->AddControl(TEXT("Control1"), FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue(), FTransform::Identity, FTransform::Identity);
	Controller->AddControl(TEXT("Control2"), FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue(), FTransform::Identity, FTransform::Identity);
	Unit.ExecuteContext.Hierarchy = Hierarchy.Get();

	// this unit has an empty entry by default, clear it for testing purpose
	Unit.Entries.Reset();

	FTLLRN_RigUnit_SetMultiControlInteger_Entry Entry;

	// functional test, whether the unit can set multiple controls
	Entry.Control = TEXT("Control1");
	Entry.IntegerValue = 10;
	Unit.Entries.Add(Entry);

	Entry.Control = TEXT("Control2");
	Entry.IntegerValue = 20;
	Unit.Entries.Add(Entry);
	AddErrorIfFalse(Unit.Entries.Num() == 2, TEXT("unexpected number of entries"));

	Execute();

	AddErrorIfFalse(Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control1"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<int32>() == 10, TEXT("unexpected control value"));
	AddErrorIfFalse(Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control2"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<int32>() == 20, TEXT("unexpected control value"));

	return true;
}

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_SetMultiControlVector2D)
{
	FTLLRN_RigControlSettings Settings;
	Settings.ControlType = ETLLRN_RigControlType::Vector2D;
	
	Controller->AddControl(TEXT("Control1"), FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue(), FTransform::Identity, FTransform::Identity);
	Controller->AddControl(TEXT("Control2"), FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue(), FTransform::Identity, FTransform::Identity);
	Unit.ExecuteContext.Hierarchy = Hierarchy.Get();

	// this unit has an empty entry by default, clear it for testing purpose
	Unit.Entries.Reset();

	FTLLRN_RigUnit_SetMultiControlVector2D_Entry Entry;

	// functional test, whether the unit can set multiple controls
	Entry.Control = TEXT("Control1");
	Entry.Vector = FVector2D(1.f, 1.f);
	Unit.Entries.Add(Entry);

	Entry.Control = TEXT("Control2");
	Entry.Vector = FVector2D(2.f, 2.f);
	Unit.Entries.Add(Entry);
	AddErrorIfFalse(Unit.Entries.Num() == 2, TEXT("unexpected number of entries"));

	Execute();

	const FVector3f TempValue1 = Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control1"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<FVector3f>();
	const FVector3f TempValue2 = Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control2"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<FVector3f>();
	AddErrorIfFalse(FVector2D(TempValue1.X, TempValue1.Y) == FVector2D(1.f, 1.f), TEXT("unexpected control value"));
	AddErrorIfFalse(FVector2D(TempValue2.X, TempValue2.Y) == FVector2D(2.f, 2.f), TEXT("unexpected control value"));

	return true;
}

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_SetMultiControlRotator)
{
	FTLLRN_RigControlSettings Settings;
	Settings.ControlType = ETLLRN_RigControlType::Rotator;
	
	Controller->AddControl(TEXT("Control1"), FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue(), FTransform::Identity, FTransform::Identity);
	Controller->AddControl(TEXT("Control2"), FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue(), FTransform::Identity, FTransform::Identity);
	Unit.ExecuteContext.Hierarchy = Hierarchy.Get();

	// this unit has an empty entry by default, clear it for testing purpose
	Unit.Entries.Reset();

	FTLLRN_RigUnit_SetMultiControlRotator_Entry Entry;

	// functional test, whether the unit can set multiple controls
	Entry.Control = TEXT("Control1");
	Entry.Rotator = FRotator(90.f,0.f,0.f);
	Entry.Space = ERigVMTransformSpace::LocalSpace;
	Unit.Entries.Add(Entry);

	Entry.Control = TEXT("Control2");
	Entry.Rotator = FRotator(0.f,90.f,0.f);
	Entry.Space = ERigVMTransformSpace::LocalSpace;
	Unit.Entries.Add(Entry);
	AddErrorIfFalse(Unit.Entries.Num() == 2, TEXT("unexpected number of entries"));

	Execute(); 

	const FVector TempValue1 = (FVector)Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control1"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<FVector3f>();
	const FVector TempValue2 = (FVector)Hierarchy->GetControlValue(FTLLRN_RigElementKey(TEXT("Control2"), ETLLRN_RigElementType::Control), ETLLRN_RigControlValueType::Current).Get<FVector3f>();
	AddErrorIfFalse(FRotator::MakeFromEuler(TempValue1) != FRotator(0.f, 0.f, 0.f), TEXT("unexpected control value"));
	AddErrorIfFalse(FRotator::MakeFromEuler(TempValue2) != FRotator(0.f, 0.f, 0.f), TEXT("unexpected control value"));

	return true;
}

#endif


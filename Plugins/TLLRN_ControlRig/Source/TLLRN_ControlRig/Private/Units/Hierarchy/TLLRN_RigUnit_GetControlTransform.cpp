// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_GetControlTransform.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_GetControlTransform)

FTLLRN_RigUnit_GetControlBool_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Control '%s' is not valid."), *Control.ToString());
		}
		else
		{
			BoolValue = Hierarchy->GetControlValue(CachedControlIndex).Get<bool>();
		}
	}
}

FTLLRN_RigUnit_GetControlFloat_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Control '%s' is not valid."), *Control.ToString());
		}
		else
		{
			FloatValue = Hierarchy->GetControlValue(CachedControlIndex).Get<float>();
			Minimum = Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Minimum).Get<float>();
			Maximum = Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Maximum).Get<float>();
		}
	}
}


FTLLRN_RigUnit_GetControlInteger_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Control '%s' is not valid."), *Control.ToString());
		}
		else
		{
			IntegerValue = Hierarchy->GetControlValue(CachedControlIndex).Get<int32>();
			Minimum = Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Minimum).Get<int32>();
			Maximum = Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Maximum).Get<int32>();
		}
	}
}

FTLLRN_RigUnit_GetControlVector2D_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Control '%s' is not valid."), *Control.ToString());
		}
		else
		{
			const FVector3f TempVector = Hierarchy->GetControlValue(CachedControlIndex).Get<FVector3f>();
			const FVector3f TempMinimum = Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Minimum).Get<FVector3f>();
			const FVector3f TempMaximum = Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Maximum).Get<FVector3f>();
			Vector = FVector2D(TempVector.X, TempVector.Y);
			Minimum = FVector2D(TempMinimum.X, TempMinimum.Y);
			Maximum = FVector2D(TempMaximum.X, TempMaximum.Y);
		}
	}
}

FTLLRN_RigUnit_GetControlVector_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Control '%s' is not valid."), *Control.ToString());
		}
		else
		{
			FTransform Transform = FTransform::Identity;
			switch (Space)
			{
				case ERigVMTransformSpace::GlobalSpace:
				{
					Transform = Hierarchy->GetGlobalTransform(CachedControlIndex);
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					Transform = Hierarchy->GetLocalTransform(CachedControlIndex);
					break;
				}
				default:
				{
					break;
				}
			}

			const ETLLRN_RigControlType ControlType = Hierarchy->GetChecked<FTLLRN_RigControlElement>(CachedControlIndex)->Settings.ControlType;
			
			if(ControlType == ETLLRN_RigControlType::Position)
			{
				Vector = Transform.GetLocation();
			}
			else if(ControlType == ETLLRN_RigControlType::Scale)
			{
				Vector = Transform.GetScale3D();
			}

			Minimum = (FVector)Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Minimum).Get<FVector3f>();
			Maximum = (FVector)Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Maximum).Get<FVector3f>();
		}
	}
}

FTLLRN_RigUnit_GetControlRotator_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Control '%s' is not valid."), *Control.ToString());
		}
		else
		{
			FTransform Transform = FTransform::Identity;
			switch (Space)
			{
				case ERigVMTransformSpace::GlobalSpace:
				{
					Transform = Hierarchy->GetGlobalTransform(CachedControlIndex);
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					Transform = Hierarchy->GetLocalTransform(CachedControlIndex);
					break;
				}
				default:
				{
					break;
				}
			}

			Rotator = Transform.GetRotation().Rotator();

			const FVector MinimumVector = (FVector)Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Minimum).Get<FVector3f>();
			const FVector MaximumVector = (FVector)Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Maximum).Get<FVector3f>();
			Minimum = FRotator::MakeFromEuler(MinimumVector); 
			Maximum = FRotator::MakeFromEuler(MaximumVector); 
		}
	}
}

FTLLRN_RigUnit_GetControlTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Control '%s' is not valid."), *Control.ToString());
		}
		else
		{
			switch (Space)
			{
				case ERigVMTransformSpace::GlobalSpace:
				{
					Transform = Hierarchy->GetGlobalTransform(CachedControlIndex);
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					Transform = Hierarchy->GetLocalTransform(CachedControlIndex);
					break;
				}
				default:
				{
					break;
				}
			}
			
			Minimum = Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Minimum).Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
			Maximum = Hierarchy->GetControlValue(CachedControlIndex, ETLLRN_RigControlValueType::Maximum).Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
		}
	}
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_GetControlTransform)
{
	FTLLRN_RigControlSettings Settings;
	Settings.ControlType = ETLLRN_RigControlType::Transform;

	const FTLLRN_RigElementKey Root = Controller->AddControl(
		TEXT("Root"),
		FTLLRN_RigElementKey(),
		Settings,
		FTLLRN_RigControlValue::Make(FTransform(FVector(1.f, 0.f, 0.f))),
		FTransform::Identity,
		FTransform::Identity);

	const FTLLRN_RigElementKey ControlA = Controller->AddControl(
	    TEXT("ControlA"),
	    Root,
	    Settings,
	    FTLLRN_RigControlValue::Make(FTransform(FVector(1.f, 2.f, 3.f))),
	    FTransform::Identity,
	    FTransform::Identity);

	Unit.Control = TEXT("Unknown");
	Unit.Space = ERigVMTransformSpace::GlobalSpace;
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(0.f, 0.f, 0.f)), TEXT("unexpected global transform (0)"));
	Unit.Space = ERigVMTransformSpace::LocalSpace;
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(0.f, 0.f, 0.f)), TEXT("unexpected local transform (0)"));

	Unit.Control = TEXT("Root");
	Unit.Space = ERigVMTransformSpace::GlobalSpace;
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected global transform (1)"));
	Unit.Space = ERigVMTransformSpace::LocalSpace;
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected local transform (1)"));

	Unit.Control = TEXT("ControlA");
	Unit.Space = ERigVMTransformSpace::GlobalSpace;
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(2.f, 2.f, 3.f)), TEXT("unexpected global transform (2)"));
	Unit.Space = ERigVMTransformSpace::LocalSpace;
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(1.f, 2.f, 3.f)), TEXT("unexpected local transform (2)"));

	return true;
}
#endif

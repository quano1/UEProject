// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_ControlChannelFromItem.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_ControlChannelFromItem)

FTLLRN_RigUnit_GetBoolAnimationChannelFromItem_Execute()
{
	Value = false;
	
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Bool)
		{
			const FTLLRN_RigControlValue StoredValue = ExecuteContext.Hierarchy->GetControlValueByIndex(ChannelElement->GetIndex(), bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
			Value = StoredValue.Get<bool>();
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a Bool."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_GetFloatAnimationChannelFromItem_Execute()
{
	Value = 0.f;
	
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Float || ChannelElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat)
		{
			const FTLLRN_RigControlValue StoredValue = ExecuteContext.Hierarchy->GetControlValueByIndex(ChannelElement->GetIndex(), bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
			Value = StoredValue.Get<float>();
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a Float."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_GetIntAnimationChannelFromItem_Execute()
{
	Value = 0;
	
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Integer)
		{
			const FTLLRN_RigControlValue StoredValue = ExecuteContext.Hierarchy->GetControlValueByIndex(ChannelElement->GetIndex(), bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
			Value = StoredValue.Get<int32>();
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not an Integer (or Enum)."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_GetVector2DAnimationChannelFromItem_Execute()
{
	Value = FVector2D::ZeroVector;
	
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Vector2D)
		{
			const FTLLRN_RigControlValue StoredValue = ExecuteContext.Hierarchy->GetControlValueByIndex(ChannelElement->GetIndex(), bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
			Value = FVector2D(StoredValue.Get<FVector2f>());
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a Vector2D."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_GetVectorAnimationChannelFromItem_Execute()
{
	Value = FVector::ZeroVector;
	
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Position || ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Scale)
		{
			const FTLLRN_RigControlValue StoredValue = ExecuteContext.Hierarchy->GetControlValueByIndex(ChannelElement->GetIndex(), bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
			Value = FVector(StoredValue.Get<FVector3f>());
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a Vector (Position or Scale)."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_GetRotatorAnimationChannelFromItem_Execute()
{
	Value = FRotator::ZeroRotator;
	
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
		{
			const FTLLRN_RigControlValue StoredValue = ExecuteContext.Hierarchy->GetControlValueByIndex(ChannelElement->GetIndex(), bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
			Value = FRotator::MakeFromEuler(FVector(StoredValue.Get<FVector3f>()));
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a Rotator."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_GetTransformAnimationChannelFromItem_Execute()
{
	Value = FTransform::Identity;
	
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Transform)
		{
			const FTLLRN_RigControlValue StoredValue = ExecuteContext.Hierarchy->GetControlValueByIndex(ChannelElement->GetIndex(), bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
			Value = StoredValue.Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
		}
		else if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
		{
			const FTLLRN_RigControlValue StoredValue = ExecuteContext.Hierarchy->GetControlValueByIndex(ChannelElement->GetIndex(), bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
			Value = StoredValue.Get<FTLLRN_RigControlValue::FEulerTransform_Float>().ToTransform().ToFTransform();
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a EulerTransform / Transform."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_SetBoolAnimationChannelFromItem_Execute()
{
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Bool)
		{
			const FTLLRN_RigControlValue ValueToStore = FTLLRN_RigControlValue::Make<bool>(Value);
			ExecuteContext.Hierarchy->SetControlValueByIndex(ChannelElement->GetIndex(), ValueToStore, bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a Bool."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_SetFloatAnimationChannelFromItem_Execute()
{
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Float || ChannelElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat)
		{
			const FTLLRN_RigControlValue ValueToStore = FTLLRN_RigControlValue::Make<float>(Value);
			ExecuteContext.Hierarchy->SetControlValueByIndex(ChannelElement->GetIndex(), ValueToStore, bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a Float."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_SetIntAnimationChannelFromItem_Execute()
{
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Integer)
		{
			const FTLLRN_RigControlValue ValueToStore = FTLLRN_RigControlValue::Make<int32>(Value);
			ExecuteContext.Hierarchy->SetControlValueByIndex(ChannelElement->GetIndex(), ValueToStore, bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not an Integer (or Enum)."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_SetVector2DAnimationChannelFromItem_Execute()
{
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Vector2D)
		{
			const FTLLRN_RigControlValue ValueToStore = FTLLRN_RigControlValue::Make<FVector2f>(FVector2f(Value));
			ExecuteContext.Hierarchy->SetControlValueByIndex(ChannelElement->GetIndex(), ValueToStore, bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a Vector2D."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_SetVectorAnimationChannelFromItem_Execute()
{
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Position || ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Scale)
		{
			const FTLLRN_RigControlValue ValueToStore = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(Value));
			ExecuteContext.Hierarchy->SetControlValueByIndex(ChannelElement->GetIndex(), ValueToStore, bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a Vector (Position or Scale)."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_SetRotatorAnimationChannelFromItem_Execute()
{
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
		{
			const FTLLRN_RigControlValue ValueToStore = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(Value.Euler()));
			ExecuteContext.Hierarchy->SetControlValueByIndex(ChannelElement->GetIndex(), ValueToStore, bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a Rotator."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_SetTransformAnimationChannelFromItem_Execute()
{
	if(const FTLLRN_RigControlElement* ChannelElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigControlElement>(Item))
	{
		if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::Transform)
		{
			const FTLLRN_RigControlValue ValueToStore = FTLLRN_RigControlValue::Make<FTLLRN_RigControlValue::FTransform_Float>(Value);
			ExecuteContext.Hierarchy->SetControlValueByIndex(ChannelElement->GetIndex(), ValueToStore, bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
		}
		else if(ChannelElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
		{
			const FTLLRN_RigControlValue ValueToStore = FTLLRN_RigControlValue::Make<FTLLRN_RigControlValue::FEulerTransform_Float>(FEulerTransform(Value));
			ExecuteContext.Hierarchy->SetControlValueByIndex(ChannelElement->GetIndex(), ValueToStore, bInitial ? ETLLRN_RigControlValueType::Initial : ETLLRN_RigControlValueType::Current);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel %s is not a EulerTransform / Transform."), *Item.ToString());
		}
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Execution/TLLRN_RigUnit_DynamicHierarchy.h"
#include "Engine/SkeletalMesh.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "TLLRN_ControlRig.h"
#include "Components/SkeletalMeshComponent.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_DynamicHierarchy)

bool FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(
	const FTLLRN_ControlRigExecuteContext& InExecuteContext,
	bool bAllowOnlyConstructionEvent,
	FString* OutErrorMessage)
{
	if(InExecuteContext.Hierarchy == nullptr)
	{
		return false;
	}

	if(bAllowOnlyConstructionEvent)
	{
		if(!InExecuteContext.IsRunningConstructionEvent())
		{
			if(OutErrorMessage)
			{
				static constexpr TCHAR ErrorMessageFormat[] = TEXT("Node can only run in %s or %s Event");
				*OutErrorMessage = FString::Printf(ErrorMessageFormat, *FTLLRN_RigUnit_PrepareForExecution::EventName.ToString(), *FTLLRN_RigUnit_PostPrepareForExecution::EventName.ToString());
			}
			return false;
		}
	}

	if(InExecuteContext.Hierarchy->Num() >= InExecuteContext.UnitContext.HierarchySettings.ProceduralElementLimit)
	{
		if(OutErrorMessage)
		{
			static constexpr TCHAR ErrorMessageFormat[] = TEXT("Node has hit the Procedural Element Limit. Check the Class Settings under Hierarchy.");
			*OutErrorMessage = ErrorMessageFormat;
		}
		return false;
	}

	return true;
}

FTLLRN_RigUnit_AddParent_Execute()
{
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true))
	{
		return;
	}
	
	FTLLRN_RigTransformElement* ChildElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigTransformElement>(Child);
	if(ChildElement == nullptr)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Child item %s does not exist."), *Child.ToString())
		return;
	}

	FTLLRN_RigTransformElement* ParentElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigTransformElement>(Parent);
	if(ParentElement == nullptr)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Parent item %s does not exist."), *Parent.ToString())
		return;
	}

	FTLLRN_RigHierarchyEnableControllerBracket EnableController(ExecuteContext.Hierarchy, true);
	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		Controller->AddParent(ChildElement, ParentElement, 0.f, true, false);
	}
}

FTLLRN_RigUnit_SetDefaultParent_Execute()
{
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true))
	{
		return;
	}
	
	FTLLRN_RigTransformElement* ChildElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigTransformElement>(Child);
	if(ChildElement == nullptr)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Child item %s does not exist."), *Child.ToString())
		return;
	}

	FTLLRN_RigTransformElement* ParentElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigTransformElement>(Parent);
	if(ParentElement == nullptr)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Parent item %s does not exist."), *Parent.ToString())
		return;
	}

	FTLLRN_RigHierarchyEnableControllerBracket EnableController(ExecuteContext.Hierarchy, true);
	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		Controller->AddParent(ChildElement, ParentElement, 1.0f, true, true);
	}
}

FTLLRN_RigUnit_SetChannelHosts_Execute()
{
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true))
	{
		return;
	}
	
	for(const FTLLRN_RigElementKey& Host : Hosts)
	{
		FTLLRN_RigHierarchyEnableControllerBracket EnableController(ExecuteContext.Hierarchy, true);
		if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
		{
			(void)Controller->AddChannelHost(Channel, Host);
		}
	}
}

FTLLRN_RigUnit_SwitchParent_Execute()
{
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, false))
	{
		return;
	}

	FTLLRN_RigTransformElement* ChildElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigTransformElement>(Child);
	if(ChildElement == nullptr)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Child item %s does not exist."), *Child.ToString())
		return;
	}
	if(!ChildElement->IsA<FTLLRN_RigMultiParentElement>())
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Child item %s cannot be space switched (only Nulls and Controls can)."), *Child.ToString())
		return;
	}

	FTLLRN_RigTransformElement* ParentElement = nullptr;

	if(Mode == TLLRN_ERigSwitchParentMode::ParentItem)
	{
		ParentElement = ExecuteContext.Hierarchy->Find<FTLLRN_RigTransformElement>(Parent);
		if(ParentElement == nullptr)
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Parent item %s does not exist."), *Parent.ToString())
			return;
		}

		if(!ParentElement->IsA<FTLLRN_RigTransformElement>())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Parent item %s does not have a transform."), *Parent.ToString())
			return;
		}
	}

	const ETLLRN_RigTransformType::Type TransformTypeToMaintain =
		bMaintainGlobal ? ETLLRN_RigTransformType::CurrentGlobal : ETLLRN_RigTransformType::CurrentLocal;
	
	const FTransform Transform = ExecuteContext.Hierarchy->GetTransform(ChildElement, TransformTypeToMaintain);

	switch(Mode)
	{
		case TLLRN_ERigSwitchParentMode::World:
		{
			if(!ExecuteContext.Hierarchy->SwitchToWorldSpace(ChildElement, false, true))
			{
				return;
			}
			break;
		}
		case TLLRN_ERigSwitchParentMode::DefaultParent:
		{
			if(!ExecuteContext.Hierarchy->SwitchToDefaultParent(ChildElement, false, true))
			{
				return;
			}
			break;
		}
		case TLLRN_ERigSwitchParentMode::ParentItem:
		default:
		{
			FString FailureReason;
			static const UTLLRN_RigHierarchy::TElementDependencyMap EmptyDependencyMap;
			if(!ExecuteContext.Hierarchy->SwitchToParent(ChildElement, ParentElement, false, true, EmptyDependencyMap, &FailureReason))
			{
				if(!FailureReason.IsEmpty())
				{
					UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("%s"), *FailureReason);
				}
				return;
			}

			// during construction event also change the initial weights
			if(ExecuteContext.IsRunningConstructionEvent())
			{
				if(!ExecuteContext.Hierarchy->SwitchToParent(ChildElement, ParentElement, true, true, EmptyDependencyMap, &FailureReason))
				{
					if(!FailureReason.IsEmpty())
					{
						UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("%s"), *FailureReason);
					}
					return;
				}
			}
			break;
		}
	}
	
	ExecuteContext.Hierarchy->SetTransform(ChildElement, Transform, TransformTypeToMaintain, true);
}

FTLLRN_RigUnit_HierarchyGetParentWeights_Execute()
{
	FTLLRN_RigUnit_HierarchyGetParentWeightsArray::StaticExecute(ExecuteContext, Child, Weights, Parents.Keys);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_HierarchyGetParentWeights::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_HierarchyGetParentWeightsArray NewNode;
	NewNode.Child = Child;
	NewNode.Weights = Weights;
	NewNode.Parents = Parents.Keys;

	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_HierarchyGetParentWeightsArray_Execute()
{
	if(ExecuteContext.Hierarchy == nullptr)
	{
		return;
	}

	FTLLRN_RigBaseElement* ChildElement = ExecuteContext.Hierarchy->Find(Child);
	if(ChildElement == nullptr)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Item %s does not exist."), *Child.ToString())
		return;
	}
	
	Weights = ExecuteContext.Hierarchy->GetParentWeightArray(ChildElement, false);
	Parents = ExecuteContext.Hierarchy->GetParents(ChildElement->GetKey(), false);
}

FTLLRN_RigUnit_HierarchySetParentWeights_Execute()
{
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, false))
	{
		return;
	}

	FTLLRN_RigBaseElement* ChildElement = ExecuteContext.Hierarchy->Find(Child);
	if(ChildElement == nullptr)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Item %s does not exist."), *Child.ToString())
		return;
	}

	if(Weights.Num() != ExecuteContext.Hierarchy->GetNumberOfParents(ChildElement))
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Provided incorrect number of weights(%d) for %s - expected %d."), Weights.Num(), *Child.ToString(), ExecuteContext.Hierarchy->GetNumberOfParents(Child))
		return;
	}

	ExecuteContext.Hierarchy->SetParentWeightArray(ChildElement, Weights, false, true);

	// during construction event also change the initial weights
	if(ExecuteContext.IsRunningConstructionEvent())
	{
		ExecuteContext.Hierarchy->SetParentWeightArray(ChildElement, Weights, true, true);
	}
}

FTLLRN_RigUnit_HierarchyReset_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}
	ExecuteContext.Hierarchy->ResetToDefault();
}

FTLLRN_RigUnit_HierarchyImportFromSkeleton_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Items.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		
		if(const USkeletalMeshComponent* SkelMeshComponent = ExecuteContext.UnitContext.DataSourceRegistry->RequestSource<USkeletalMeshComponent>(UTLLRN_ControlRig::OwnerComponent))
		{
			if(SkelMeshComponent->GetSkeletalMeshAsset())
			{
				const FReferenceSkeleton& ReferenceSkeleton = SkelMeshComponent->GetSkeletalMeshAsset()->GetRefSkeleton();
				Items = Controller->ImportBones(ReferenceSkeleton, NameSpace, false, false, false, false);
				if(bIncludeCurves)
				{
					if(USkeleton* Skeleton = SkelMeshComponent->GetSkeletalMeshAsset()->GetSkeleton())
					{
						Items.Append(Controller->ImportCurves(Skeleton, NameSpace, false, false, false));
					}
				}
			}
		}
	}
}

FTLLRN_RigUnit_HierarchyRemoveElement_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}
	
	bSuccess = false;

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		bSuccess = Controller->RemoveElement(Item);
	}
}

FTLLRN_RigUnit_HierarchyAddBone_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddBone(Name, Parent, Transform, Space == ERigVMTransformSpace::GlobalSpace, ETLLRN_RigBoneType::Imported, false, false);
	}
}

FTLLRN_RigUnit_HierarchyAddNull_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddNull(Name, Parent, Transform, Space == ERigVMTransformSpace::GlobalSpace, false, false);
	}
}

FTransform FTLLRN_RigUnit_HierarchyAddControlElement::ProjectOffsetTransform(const FTransform& InOffsetTransform,
	ERigVMTransformSpace InOffsetSpace, const FTLLRN_RigElementKey& InParent, const UTLLRN_RigHierarchy* InHierarchy)
{
	if(InOffsetSpace == ERigVMTransformSpace::GlobalSpace && InParent.IsValid())
	{
		const FTransform ParentTransform = InHierarchy->GetGlobalTransform(InParent);
		FTransform OffsetTransform = InOffsetTransform.GetRelativeTransform(ParentTransform);
		OffsetTransform.NormalizeRotation();
		return OffsetTransform;
	}
	return InOffsetTransform;
}

void FTLLRN_RigUnit_HierarchyAddControl_Settings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	DisplayName = InSettings.DisplayName;
}

void FTLLRN_RigUnit_HierarchyAddControl_Settings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	OutSettings.DisplayName = DisplayName;
}

void FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	bVisible = InSettings.bShapeVisible;
	Name = InSettings.ShapeName;
	Color = InSettings.ShapeColor;
	Transform = InControlElement->GetShapeTransform().Get(ETLLRN_RigTransformType::InitialLocal);
}

void FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	OutSettings.bShapeVisible = bVisible;
	OutSettings.ShapeName = Name;
	OutSettings.ShapeColor = Color;
}

void FTLLRN_RigUnit_HierarchyAddControl_ProxySettings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	bIsProxy = InSettings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl;
	DrivenControls = InSettings.DrivenControls;
	ShapeVisibility = InSettings.ShapeVisibility;
}

void FTLLRN_RigUnit_HierarchyAddControl_ProxySettings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	OutSettings.AnimationType = bIsProxy ? ETLLRN_RigControlAnimationType::ProxyControl : ETLLRN_RigControlAnimationType::AnimationControl;
	OutSettings.DrivenControls = DrivenControls;
	OutSettings.ShapeVisibility = ShapeVisibility;
}

void FTLLRN_RigUnit_HierarchyAddControlFloat_LimitSettings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	Limit = InSettings.LimitEnabled[0];
	MinValue = InSettings.MinimumValue.Get<float>();
	MaxValue = InSettings.MaximumValue.Get<float>();
	bDrawLimits = InSettings.bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlFloat_LimitSettings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	OutSettings.SetupLimitArrayForType(false, false, false);
	OutSettings.LimitEnabled[0] = Limit;
	OutSettings.MinimumValue = FTLLRN_RigControlValue::Make<float>(MinValue);
	OutSettings.MaximumValue = FTLLRN_RigControlValue::Make<float>(MaxValue);
	OutSettings.bDrawLimits = bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlFloat_Settings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	FTLLRN_RigUnit_HierarchyAddControl_Settings::ConfigureFrom(InControlElement, InSettings);

	bIsScale = InSettings.ControlType == ETLLRN_RigControlType::ScaleFloat;
	PrimaryAxis = InSettings.PrimaryAxis;

	Proxy.ConfigureFrom(InControlElement, InSettings);
	Limits.ConfigureFrom(InControlElement, InSettings);
	Shape.ConfigureFrom(InControlElement, InSettings);
}

void FTLLRN_RigUnit_HierarchyAddControlFloat_Settings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	Super::Configure(OutSettings);
	
	OutSettings.ControlType = bIsScale ? ETLLRN_RigControlType::ScaleFloat : ETLLRN_RigControlType::Float;
	OutSettings.PrimaryAxis = PrimaryAxis;

	Proxy.Configure(OutSettings);
	Limits.Configure(OutSettings);
	Shape.Configure(OutSettings);
}

FTLLRN_RigUnit_HierarchyAddControlFloat_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		Settings.Configure(ControlSettings);

		const FTransform Offset = ProjectOffsetTransform(OffsetTransform, OffsetSpace, Parent, ExecuteContext.Hierarchy);

		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<float>(InitialValue);
		const FTransform ShapeTransform = Settings.Shape.Transform;
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddControl(Name, Parent, ControlSettings, Value, Offset, ShapeTransform, false, false);
	}
}

void FTLLRN_RigUnit_HierarchyAddControlInteger_LimitSettings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	Limit = InSettings.LimitEnabled[0];
	MinValue = InSettings.MinimumValue.Get<int32>();
	MaxValue = InSettings.MaximumValue.Get<int32>();
	bDrawLimits = InSettings.bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlInteger_LimitSettings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	OutSettings.SetupLimitArrayForType(false, false, false);
	OutSettings.LimitEnabled[0] = Limit;
	OutSettings.MinimumValue = FTLLRN_RigControlValue::Make<int32>(MinValue);
	if (OutSettings.ControlEnum)
	{
		OutSettings.MaximumValue = FTLLRN_RigControlValue::Make<int32>(OutSettings.ControlEnum->GetMaxEnumValue());
	}
	else
	{
		OutSettings.MaximumValue = FTLLRN_RigControlValue::Make<int32>(MaxValue);
	}
	OutSettings.bDrawLimits = bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlInteger_Settings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	FTLLRN_RigUnit_HierarchyAddControl_Settings::ConfigureFrom(InControlElement, InSettings);

	PrimaryAxis = InSettings.PrimaryAxis;
	ControlEnum = InSettings.ControlEnum;

	Proxy.ConfigureFrom(InControlElement, InSettings);
	Limits.ConfigureFrom(InControlElement, InSettings);
	Shape.ConfigureFrom(InControlElement, InSettings);
}

void FTLLRN_RigUnit_HierarchyAddControlInteger_Settings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	Super::Configure(OutSettings);
	
	OutSettings.ControlType = ETLLRN_RigControlType::Integer;
	OutSettings.PrimaryAxis = PrimaryAxis;
	OutSettings.ControlEnum = ControlEnum;

	Proxy.Configure(OutSettings);
	Limits.Configure(OutSettings);
	Shape.Configure(OutSettings);
}

FTLLRN_RigUnit_HierarchyAddControlInteger_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		Settings.Configure(ControlSettings);

		const FTransform Offset = ProjectOffsetTransform(OffsetTransform, OffsetSpace, Parent, ExecuteContext.Hierarchy);

		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<int32>(InitialValue);
		const FTransform ShapeTransform = Settings.Shape.Transform;
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddControl(Name, Parent, ControlSettings, Value, Offset, ShapeTransform, false, false);
	}
}

void FTLLRN_RigUnit_HierarchyAddControlVector2D_LimitSettings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	LimitX = InSettings.LimitEnabled[0];
	LimitY = InSettings.LimitEnabled[1];
	MinValue = FVector2D(InSettings.MinimumValue.Get<FVector3f>().X, InSettings.MinimumValue.Get<FVector3f>().Y);
	MaxValue = FVector2D(InSettings.MaximumValue.Get<FVector3f>().X, InSettings.MaximumValue.Get<FVector3f>().Y);
	bDrawLimits = InSettings.bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlVector2D_LimitSettings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	OutSettings.SetupLimitArrayForType(false, false, false);
	OutSettings.LimitEnabled[0] = LimitX;
	OutSettings.LimitEnabled[1] = LimitY;
	OutSettings.MinimumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MinValue.X, MinValue.Y, 0.f));
	OutSettings.MaximumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MaxValue.X, MaxValue.Y, 0.f));
	OutSettings.bDrawLimits = bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlVector2D_Settings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	FTLLRN_RigUnit_HierarchyAddControl_Settings::ConfigureFrom(InControlElement, InSettings);

	PrimaryAxis = InSettings.PrimaryAxis;
	FilteredChannels = InSettings.FilteredChannels;

	Proxy.ConfigureFrom(InControlElement, InSettings);
	Limits.ConfigureFrom(InControlElement, InSettings);
	Shape.ConfigureFrom(InControlElement, InSettings);
}

void FTLLRN_RigUnit_HierarchyAddControlVector2D_Settings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	Super::Configure(OutSettings);
	
	OutSettings.ControlType = ETLLRN_RigControlType::Vector2D;
	OutSettings.PrimaryAxis = PrimaryAxis;
	OutSettings.FilteredChannels = FilteredChannels;

	Proxy.Configure(OutSettings);
	Limits.Configure(OutSettings);
	Shape.Configure(OutSettings);
}

FTLLRN_RigUnit_HierarchyAddControlVector2D_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		Settings.Configure(ControlSettings);

		const FTransform Offset = ProjectOffsetTransform(OffsetTransform, OffsetSpace, Parent, ExecuteContext.Hierarchy);

		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(InitialValue.X, InitialValue.Y, 0.f));
		const FTransform ShapeTransform = Settings.Shape.Transform;
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddControl(Name, Parent, ControlSettings, Value, Offset, ShapeTransform, false, false);
	}
}

void FTLLRN_RigUnit_HierarchyAddControlVector_LimitSettings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	LimitX = InSettings.LimitEnabled[0];
	LimitY = InSettings.LimitEnabled[1];
	LimitZ = InSettings.LimitEnabled[2];
	MinValue = FVector(InSettings.MinimumValue.Get<FVector3f>());
	MaxValue = FVector(InSettings.MaximumValue.Get<FVector3f>());
	bDrawLimits = InSettings.bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlVector_LimitSettings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	OutSettings.SetupLimitArrayForType(false, false, false);
	OutSettings.LimitEnabled[0] = LimitX;
	OutSettings.LimitEnabled[1] = LimitY;
	OutSettings.LimitEnabled[2] = LimitZ;
	OutSettings.MinimumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MinValue));
	OutSettings.MaximumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MaxValue));
	OutSettings.bDrawLimits = bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlVector_Settings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	FTLLRN_RigUnit_HierarchyAddControl_Settings::ConfigureFrom(InControlElement, InSettings);

	bIsPosition = InSettings.ControlType == ETLLRN_RigControlType::Position;
	FilteredChannels = InSettings.FilteredChannels;

	Proxy.ConfigureFrom(InControlElement, InSettings);
	Limits.ConfigureFrom(InControlElement, InSettings);
	Shape.ConfigureFrom(InControlElement, InSettings);
}

void FTLLRN_RigUnit_HierarchyAddControlVector_Settings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	Super::Configure(OutSettings);
	
	OutSettings.ControlType = bIsPosition ? ETLLRN_RigControlType::Position : ETLLRN_RigControlType::Scale;
	OutSettings.FilteredChannels = FilteredChannels;

	Proxy.Configure(OutSettings);
	Limits.Configure(OutSettings);
	Shape.Configure(OutSettings);
}

FTLLRN_RigUnit_HierarchyAddControlVector_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		Settings.Configure(ControlSettings);

		const FTransform Offset = ProjectOffsetTransform(OffsetTransform, OffsetSpace, Parent, ExecuteContext.Hierarchy);

		FVector Vector = InitialValue;
		if(Settings.InitialSpace == ERigVMTransformSpace::GlobalSpace)
		{
			if(Settings.bIsPosition)
			{
				const FTransform ParentTransform = ExecuteContext.Hierarchy->GetGlobalTransform(Parent);
				const FTransform GlobalOffsetTransform = Offset * ParentTransform;
				const FTransform LocalTransform = FTransform(Vector).GetRelativeTransform(GlobalOffsetTransform);
				Vector = LocalTransform.GetLocation();
			}
			else
			{
				Vector = Vector * ExecuteContext.Hierarchy->GetGlobalTransform(Parent).GetScale3D(); 
			}
		}

		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(Vector));
		const FTransform ShapeTransform = Settings.Shape.Transform;
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddControl(Name, Parent, ControlSettings, Value, Offset, ShapeTransform, false, false);
	}
}

void FTLLRN_RigUnit_HierarchyAddControlRotator_LimitSettings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	LimitPitch = InSettings.LimitEnabled[0];
	LimitYaw = InSettings.LimitEnabled[1];
	LimitRoll = InSettings.LimitEnabled[2];
	MinValue = FRotator::MakeFromEuler(FVector(InSettings.MinimumValue.Get<FVector3f>()));
	MaxValue = FRotator::MakeFromEuler(FVector(InSettings.MaximumValue.Get<FVector3f>()));
	bDrawLimits = InSettings.bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlRotator_LimitSettings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	OutSettings.SetupLimitArrayForType(false, false, false);
	OutSettings.LimitEnabled[0] = LimitPitch;
	OutSettings.LimitEnabled[1] = LimitYaw;
	OutSettings.LimitEnabled[2] = LimitRoll;
	OutSettings.MinimumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MinValue.Euler()));
	OutSettings.MaximumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MaxValue.Euler()));
	OutSettings.bDrawLimits = bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlRotator_Settings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	FTLLRN_RigUnit_HierarchyAddControl_Settings::ConfigureFrom(InControlElement, InSettings);

	FilteredChannels = InSettings.FilteredChannels;

	Proxy.ConfigureFrom(InControlElement, InSettings);
	Limits.ConfigureFrom(InControlElement, InSettings);
	Shape.ConfigureFrom(InControlElement, InSettings);
}

void FTLLRN_RigUnit_HierarchyAddControlRotator_Settings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	Super::Configure(OutSettings);
	
	OutSettings.ControlType = ETLLRN_RigControlType::Rotator;
	OutSettings.FilteredChannels = FilteredChannels;

	Proxy.Configure(OutSettings);
	Limits.Configure(OutSettings);
	Shape.Configure(OutSettings);
}

FTLLRN_RigUnit_HierarchyAddControlRotator_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		Settings.Configure(ControlSettings);

		const FTransform Offset = ProjectOffsetTransform(OffsetTransform, OffsetSpace, Parent, ExecuteContext.Hierarchy);

		FRotator Rotator = InitialValue;
		if(Settings.InitialSpace == ERigVMTransformSpace::GlobalSpace)
		{
			const FTransform ParentTransform = ExecuteContext.Hierarchy->GetGlobalTransform(Parent);
			const FTransform GlobalOffsetTransform = Offset * ParentTransform;
			FTransform LocalTransform = FTransform(Rotator).GetRelativeTransform(GlobalOffsetTransform);
			LocalTransform.NormalizeRotation();
			Rotator = LocalTransform.Rotator(); 
		}

		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(Rotator.Euler()));
		const FTransform ShapeTransform = Settings.Shape.Transform;
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddControl(Name, Parent, ControlSettings, Value, Offset, ShapeTransform, false, false);
	}
}

void FTLLRN_RigUnit_HierarchyAddControlTransform_LimitSettings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	LimitTranslationX = InSettings.LimitEnabled[0];
	LimitTranslationY = InSettings.LimitEnabled[1];
	LimitTranslationZ = InSettings.LimitEnabled[2];
	LimitPitch = InSettings.LimitEnabled[3];
	LimitYaw = InSettings.LimitEnabled[4];
	LimitRoll = InSettings.LimitEnabled[5];
	LimitScaleX = InSettings.LimitEnabled[6];
	LimitScaleY = InSettings.LimitEnabled[7];
	LimitScaleZ = InSettings.LimitEnabled[8];
	MinValue = InSettings.MinimumValue.Get<FTLLRN_RigControlValue::FEulerTransform_Float>().ToTransform();
	MaxValue = InSettings.MaximumValue.Get<FTLLRN_RigControlValue::FEulerTransform_Float>().ToTransform();
	bDrawLimits = InSettings.bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlTransform_LimitSettings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	OutSettings.SetupLimitArrayForType(false, false, false);
	OutSettings.LimitEnabled[0] = LimitTranslationX;
	OutSettings.LimitEnabled[1] = LimitTranslationY;
	OutSettings.LimitEnabled[2] = LimitTranslationZ;
	OutSettings.LimitEnabled[3] = LimitPitch;
	OutSettings.LimitEnabled[4] = LimitYaw;
	OutSettings.LimitEnabled[5] = LimitRoll;
	OutSettings.LimitEnabled[6] = LimitScaleX;
	OutSettings.LimitEnabled[7] = LimitScaleY;
	OutSettings.LimitEnabled[8] = LimitScaleZ;
	OutSettings.MinimumValue = FTLLRN_RigControlValue::Make<FTLLRN_RigControlValue::FEulerTransform_Float>(MinValue);
	OutSettings.MaximumValue = FTLLRN_RigControlValue::Make<FTLLRN_RigControlValue::FEulerTransform_Float>(MaxValue);
	OutSettings.bDrawLimits = bDrawLimits;
}

void FTLLRN_RigUnit_HierarchyAddControlTransform_Settings::ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings)
{
	FTLLRN_RigUnit_HierarchyAddControl_Settings::ConfigureFrom(InControlElement, InSettings);

	FilteredChannels = InSettings.FilteredChannels;
	bUsePreferredRotationOrder = InSettings.bUsePreferredRotationOrder;
	PreferredRotationOrder = InSettings.PreferredRotationOrder;

	Proxy.ConfigureFrom(InControlElement, InSettings);
	Limits.ConfigureFrom(InControlElement, InSettings);
	Shape.ConfigureFrom(InControlElement, InSettings);
}

void FTLLRN_RigUnit_HierarchyAddControlTransform_Settings::Configure(FTLLRN_RigControlSettings& OutSettings) const
{
	Super::Configure(OutSettings);
	
	OutSettings.ControlType = ETLLRN_RigControlType::EulerTransform;
	OutSettings.FilteredChannels = FilteredChannels;
	OutSettings.bUsePreferredRotationOrder = bUsePreferredRotationOrder;
	OutSettings.PreferredRotationOrder = PreferredRotationOrder;

	Proxy.Configure(OutSettings);
	Limits.Configure(OutSettings);
	Shape.Configure(OutSettings);
}

FTLLRN_RigUnit_HierarchyAddControlTransform_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		Settings.Configure(ControlSettings);

		const FTransform Offset = ProjectOffsetTransform(OffsetTransform, OffsetSpace, Parent, ExecuteContext.Hierarchy);

		FTransform Transform = InitialValue;
		if(Settings.InitialSpace == ERigVMTransformSpace::GlobalSpace)
		{
			const FTransform ParentTransform = ExecuteContext.Hierarchy->GetGlobalTransform(Parent);
			const FTransform GlobalOffsetTransform = Offset * ParentTransform;
			Transform = Transform.GetRelativeTransform(GlobalOffsetTransform);
			Transform.NormalizeRotation();
		}
		
		const FEulerTransform EulerTransform(Transform);
		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FTLLRN_RigControlValue::FEulerTransform_Float>(EulerTransform);
		const FTransform ShapeTransform = Settings.Shape.Transform;
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddControl(Name, Parent, ControlSettings, Value, Offset, ShapeTransform, false, false);
	}
}

FTLLRN_RigUnit_HierarchyAddAnimationChannelBool_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	const FString NameStr = Name.ToString();
	if (NameStr.Contains(TEXT(":")))
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation channel name %s contains invalid ':' character."), *NameStr);
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		ControlSettings.ControlType = ETLLRN_RigControlType::Bool;
		ControlSettings.SetupLimitArrayForType(true, true, true);
		ControlSettings.MinimumValue = FTLLRN_RigControlValue::Make<bool>(MinimumValue);
		ControlSettings.MaximumValue = FTLLRN_RigControlValue::Make<bool>(MaximumValue);
		ControlSettings.DisplayName = Controller->GetHierarchy()->GetSafeNewDisplayName(Parent, Name);
		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<bool>(InitialValue);

		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddAnimationChannel(Name, Parent, ControlSettings, false, false);

		if(Item.IsValid())
		{
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Initial, false, false);
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Current, false, false);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel could not be added. Is Parent valid?"));
		}
	}
}

FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	const FString NameStr = Name.ToString();
	if (NameStr.Contains(TEXT(":")))
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation channel name %s contains invalid ':' character."), *NameStr);
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		ControlSettings.ControlType = ETLLRN_RigControlType::Float;
		ControlSettings.SetupLimitArrayForType(true, true, true);
		ControlSettings.LimitEnabled[0] = LimitsEnabled.Enabled;
		ControlSettings.MinimumValue = FTLLRN_RigControlValue::Make<float>(MinimumValue);
		ControlSettings.MaximumValue = FTLLRN_RigControlValue::Make<float>(MaximumValue);
		ControlSettings.DisplayName = Controller->GetHierarchy()->GetSafeNewDisplayName(Parent, Name);
		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<float>(InitialValue);
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddAnimationChannel(Name, Parent, ControlSettings, false, false);

		if(Item.IsValid())
		{
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Initial, false, false);
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Current, false, false);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel could not be added. Is Parent valid?"));
		}
	}
}

FTLLRN_RigUnit_HierarchyAddAnimationChannelScaleFloat_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	const FString NameStr = Name.ToString();
	if (NameStr.Contains(TEXT(":")))
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation channel name %s contains invalid ':' character."), *NameStr);
	}


	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		ControlSettings.ControlType = ETLLRN_RigControlType::ScaleFloat;
		ControlSettings.SetupLimitArrayForType(true, true, true);
		ControlSettings.LimitEnabled[0] = LimitsEnabled.Enabled;
		ControlSettings.MinimumValue = FTLLRN_RigControlValue::Make<float>(MinimumValue);
		ControlSettings.MaximumValue = FTLLRN_RigControlValue::Make<float>(MaximumValue);
		ControlSettings.DisplayName = Controller->GetHierarchy()->GetSafeNewDisplayName(Parent, Name);
		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<float>(InitialValue);
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddAnimationChannel(Name, Parent, ControlSettings, false, false);

		if(Item.IsValid())
		{
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Initial, false, false);
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Current, false, false);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel could not be added. Is Parent valid?"));
		}
	}
}

FTLLRN_RigUnit_HierarchyAddAnimationChannelInteger_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	const FString NameStr = Name.ToString();
	if (NameStr.Contains(TEXT(":")))
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation channel name %s contains invalid ':' character."), *NameStr);
	}


	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		ControlSettings.ControlType = ETLLRN_RigControlType::Integer;
		ControlSettings.SetupLimitArrayForType(true, true, true);
		ControlSettings.LimitEnabled[0] = LimitsEnabled.Enabled;
		ControlSettings.MinimumValue = FTLLRN_RigControlValue::Make<int32>(MinimumValue);
		ControlSettings.DisplayName = Controller->GetHierarchy()->GetSafeNewDisplayName(Parent, Name);
		ControlSettings.ControlEnum = ControlEnum;

		if (ControlEnum)
		{
			ControlSettings.MaximumValue = FTLLRN_RigControlValue::Make<int32>(ControlEnum->GetMaxEnumValue());
		}
		else
		{
			ControlSettings.MaximumValue = FTLLRN_RigControlValue::Make<int32>(MaximumValue);
		}
		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<int32>(InitialValue);
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddAnimationChannel(Name, Parent, ControlSettings, false, false);

		if(Item.IsValid())
		{
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Initial, false, false);
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Current, false, false);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel could not be added. Is Parent valid?"));
		}
	}
}

FTLLRN_RigUnit_HierarchyAddAnimationChannelVector2D_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	const FString NameStr = Name.ToString();
	if (NameStr.Contains(TEXT(":")))
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation channel name %s contains invalid ':' character."), *NameStr);
	}


	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		ControlSettings.ControlType = ETLLRN_RigControlType::Vector2D;
		ControlSettings.SetupLimitArrayForType(true, true, true);
		ControlSettings.LimitEnabled[0] = LimitsEnabled.X;
		ControlSettings.LimitEnabled[1] = LimitsEnabled.Y;
		ControlSettings.MinimumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MinimumValue.X, MinimumValue.Y, 0.f));
		ControlSettings.MaximumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MaximumValue.X, MaximumValue.Y, 0.f));
		ControlSettings.DisplayName = Controller->GetHierarchy()->GetSafeNewDisplayName(Parent, Name);
		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(InitialValue.X, InitialValue.Y, 0.f));
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddAnimationChannel(Name, Parent, ControlSettings, false, false);

		if(Item.IsValid())
		{
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Initial, false, false);
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Current, false, false);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel could not be added. Is Parent valid?"));
		}
	}
}

FTLLRN_RigUnit_HierarchyAddAnimationChannelVector_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	const FString NameStr = Name.ToString();
	if (NameStr.Contains(TEXT(":")))
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation channel name %s contains invalid ':' character."), *NameStr);
	}


	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		ControlSettings.ControlType = ETLLRN_RigControlType::Position;
		ControlSettings.SetupLimitArrayForType(true, true, true);
		ControlSettings.LimitEnabled[0] = LimitsEnabled.X;
		ControlSettings.LimitEnabled[1] = LimitsEnabled.Y;
		ControlSettings.LimitEnabled[2] = LimitsEnabled.Z;
		ControlSettings.MinimumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MinimumValue));
		ControlSettings.MaximumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MaximumValue));
		ControlSettings.DisplayName = Controller->GetHierarchy()->GetSafeNewDisplayName(Parent, Name);
		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(InitialValue));
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddAnimationChannel(Name, Parent, ControlSettings, false, false);

		if(Item.IsValid())
		{
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Initial, false, false);
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Current, false, false);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel could not be added. Is Parent valid?"));
		}
	}
}

FTLLRN_RigUnit_HierarchyAddAnimationChannelScaleVector_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	const FString NameStr = Name.ToString();
	if (NameStr.Contains(TEXT(":")))
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation channel name %s contains invalid ':' character."), *NameStr);
	}


	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		ControlSettings.ControlType = ETLLRN_RigControlType::Scale;
		ControlSettings.SetupLimitArrayForType(true, true, true);
		ControlSettings.LimitEnabled[0] = LimitsEnabled.X;
		ControlSettings.LimitEnabled[1] = LimitsEnabled.Y;
		ControlSettings.LimitEnabled[2] = LimitsEnabled.Z;
		ControlSettings.MinimumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MinimumValue));
		ControlSettings.MaximumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MaximumValue));
		ControlSettings.DisplayName = Controller->GetHierarchy()->GetSafeNewDisplayName(Parent, Name);
		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(InitialValue));
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddAnimationChannel(Name, Parent, ControlSettings, false, false);

		if(Item.IsValid())
		{
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Initial, false, false);
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Current, false, false);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel could not be added. Is Parent valid?"));
		}
	}
}

FTLLRN_RigUnit_HierarchyAddAnimationChannelRotator_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	const FString NameStr = Name.ToString();
	if (NameStr.Contains(TEXT(":")))
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation channel name %s contains invalid ':' character."), *NameStr);
	}


	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigControlSettings ControlSettings;
		ControlSettings.ControlType = ETLLRN_RigControlType::Rotator;
		ControlSettings.SetupLimitArrayForType(true, true, true);
		ControlSettings.LimitEnabled[0] = LimitsEnabled.Pitch;
		ControlSettings.LimitEnabled[1] = LimitsEnabled.Yaw;
		ControlSettings.LimitEnabled[2] = LimitsEnabled.Roll;
		ControlSettings.MinimumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MinimumValue.Euler()));
		ControlSettings.MaximumValue = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(MaximumValue.Euler()));
		ControlSettings.DisplayName = Controller->GetHierarchy()->GetSafeNewDisplayName(Parent, Name);
		const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(InitialValue.Euler()));
		
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddAnimationChannel(Name, Parent, ControlSettings, false, false);

		if(Item.IsValid())
		{
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Initial, false, false);
			ExecuteContext.Hierarchy->SetControlValue(Item, Value, ETLLRN_RigControlValueType::Current, false, false);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Animation Channel could not be added. Is Parent valid?"));
		}
	}
}

FTLLRN_RigUnit_HierarchyGetShapeSettings_Execute()
{
	Settings = FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings();

	if(UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy)
	{
		if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Item))
		{
			Settings.bVisible = ControlElement->Settings.bShapeVisible;
			Settings.Color = ControlElement->Settings.ShapeColor;
			Settings.Name = ControlElement->Settings.ShapeName;
			Settings.Transform = Hierarchy->GetControlShapeTransform((FTLLRN_RigControlElement*)ControlElement, ETLLRN_RigTransformType::InitialLocal);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Control '%s' does not exist."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_HierarchySetShapeSettings_Execute()
{
	if(UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy)
	{
		if(FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Item))
		{
			ControlElement->Settings.bShapeVisible = Settings.bVisible;
			ControlElement->Settings.ShapeColor = Settings.Color;
			ControlElement->Settings.ShapeName = Settings.Name;
			Hierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);
			Hierarchy->SetControlShapeTransformByIndex(ControlElement->GetIndex(), Settings.Transform, true, false);
			Hierarchy->SetControlShapeTransformByIndex(ControlElement->GetIndex(), Settings.Transform, false, false);
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Control '%s' does not exist."), *Item.ToString());
		}
	}
}

FTLLRN_RigUnit_HierarchyAddSocket_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddSocket(Name, Parent, Transform, Space == ERigVMTransformSpace::GlobalSpace, Color, Description, false, false);
	}
}

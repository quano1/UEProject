// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "TLLRN_ControlRig.h"
#include "HelperUtil.h"
#include "UObject/PropertyPortFlags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigHierarchyContainer)

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigHierarchyContainer
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigHierarchyContainer::FTLLRN_RigHierarchyContainer()
{
}

FTLLRN_RigHierarchyContainer::FTLLRN_RigHierarchyContainer(const FTLLRN_RigHierarchyContainer& InOther)
{
	BoneHierarchy = InOther.BoneHierarchy;
	SpaceHierarchy = InOther.SpaceHierarchy;
	ControlHierarchy = InOther.ControlHierarchy;
	CurveContainer = InOther.CurveContainer;
}

FTLLRN_RigHierarchyContainer& FTLLRN_RigHierarchyContainer::operator= (const FTLLRN_RigHierarchyContainer &InOther)
{
	BoneHierarchy = InOther.BoneHierarchy;
	SpaceHierarchy = InOther.SpaceHierarchy;
	ControlHierarchy = InOther.ControlHierarchy;
	CurveContainer = InOther.CurveContainer;

	return *this;
}

class FTLLRN_RigHierarchyContainerImportErrorContext : public FOutputDevice
{
public:

	int32 NumErrors;

	FTLLRN_RigHierarchyContainerImportErrorContext()
        : FOutputDevice()
        , NumErrors(0)
	{
	}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Error Importing To Hierarchy: %s"), V);
		NumErrors++;
	}
};

TArray<FTLLRN_RigElementKey> FTLLRN_RigHierarchyContainer::ImportFromText(const FTLLRN_RigHierarchyCopyPasteContent& InData)
{
	TArray<FTLLRN_RigElementKey> PastedKeys;

	if (InData.Contents.Num() == 0 ||
		(InData.Types.Num() != InData.Contents.Num()) ||
		(InData.LocalTransforms.Num() != InData.Contents.Num()) ||
		(InData.GlobalTransforms.Num() != InData.Contents.Num()))
	{
		return PastedKeys;
	}

	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> ElementMap;
	for (const FTLLRN_RigBone& Element : BoneHierarchy)
	{
		ElementMap.Add(Element.GetElementKey(), Element.GetElementKey());
	}
	for (const FTLLRN_RigControl& Element : ControlHierarchy)
	{
		ElementMap.Add(Element.GetElementKey(), Element.GetElementKey());
	}
	for (const FTLLRN_RigSpace& Element : SpaceHierarchy)
	{
		ElementMap.Add(Element.GetElementKey(), Element.GetElementKey());
	}
	for (const FTLLRN_RigCurve& Element : CurveContainer)
	{
		ElementMap.Add(Element.GetElementKey(), Element.GetElementKey());
	}

	FTLLRN_RigHierarchyContainerImportErrorContext ErrorPipe;
	TArray<TSharedPtr<FTLLRN_RigElement>> Elements;
	for (int32 Index = 0; Index < InData.Types.Num(); Index++)
	{
		ErrorPipe.NumErrors = 0;
		
		TSharedPtr<FTLLRN_RigElement> NewElement;
		switch (InData.Types[Index])
		{
			case ETLLRN_RigElementType::Bone:
			{
				NewElement = MakeShared<FTLLRN_RigBone>();
				FTLLRN_RigBone::StaticStruct()->ImportText(*InData.Contents[Index], NewElement.Get(), nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigBone::StaticStruct()->GetName(), true);
				break;
			}
			case ETLLRN_RigElementType::Control:
			{
				NewElement = MakeShared<FTLLRN_RigControl>();
				FTLLRN_RigControl::StaticStruct()->ImportText(*InData.Contents[Index], NewElement.Get(), nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigControl::StaticStruct()->GetName(), true);
				break;
			}
			case ETLLRN_RigElementType::Space:
			{
				NewElement = MakeShared<FTLLRN_RigSpace>();
				FTLLRN_RigSpace::StaticStruct()->ImportText(*InData.Contents[Index], NewElement.Get(), nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigSpace::StaticStruct()->GetName(), true);
				break;
			}
			case ETLLRN_RigElementType::Curve:
			{
				NewElement = MakeShared<FTLLRN_RigCurve>();
				FTLLRN_RigCurve::StaticStruct()->ImportText(*InData.Contents[Index], NewElement.Get(), nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigCurve::StaticStruct()->GetName(), true);
				break;
			}
			default:
			{
				ensure(false);
				break;
			}
		}

		if (ErrorPipe.NumErrors > 0)
		{
			return PastedKeys;
		}

		Elements.Add(NewElement);
	}

	for (int32 Index = 0; Index < InData.Types.Num(); Index++)
	{
		switch (InData.Types[Index])
		{
			case ETLLRN_RigElementType::Bone:
			{
				const FTLLRN_RigBone& Element = *static_cast<FTLLRN_RigBone*>(Elements[Index].Get());

				FName ParentName = NAME_None;
				if (const FTLLRN_RigElementKey* ParentKey = ElementMap.Find(Element.GetParentElementKey()))
				{
					ParentName = ParentKey->Name;
				}

				FTLLRN_RigBone& NewElement = BoneHierarchy.Add(Element.Name, ParentName, ETLLRN_RigBoneType::User, Element.InitialTransform, Element.LocalTransform, InData.GlobalTransforms[Index]);
				ElementMap.FindOrAdd(Element.GetElementKey()) = NewElement.GetElementKey();
				PastedKeys.Add(NewElement.GetElementKey());
				break;
			}
			case ETLLRN_RigElementType::Control:
			{
				const FTLLRN_RigControl& Element = *static_cast<FTLLRN_RigControl*>(Elements[Index].Get());

				FName ParentName = NAME_None;
				if (const FTLLRN_RigElementKey* ParentKey = ElementMap.Find(Element.GetParentElementKey()))
				{
					ParentName = ParentKey->Name;
				}

				FName SpaceName = NAME_None;
				if (const FTLLRN_RigElementKey* SpaceKey = ElementMap.Find(Element.GetSpaceElementKey()))
				{
					SpaceName = SpaceKey->Name;
				}

				FTLLRN_RigControl& NewElement = ControlHierarchy.Add(Element.Name, Element.ControlType, ParentName, SpaceName, Element.OffsetTransform, Element.InitialValue, Element.GizmoName, Element.GizmoTransform, Element.GizmoColor);

				// copy additional members
				NewElement.DisplayName = Element.DisplayName;
				NewElement.bAnimatable = Element.bAnimatable;
				NewElement.PrimaryAxis = Element.PrimaryAxis;
				NewElement.bLimitTranslation = Element.bLimitTranslation;
				NewElement.bLimitRotation = Element.bLimitRotation;
				NewElement.bLimitScale= Element.bLimitScale;
				NewElement.MinimumValue = Element.MinimumValue;
				NewElement.MaximumValue = Element.MaximumValue;
				NewElement.bDrawLimits = Element.bDrawLimits;
				NewElement.bGizmoEnabled = Element.bGizmoEnabled;
				NewElement.bGizmoVisible = Element.bGizmoVisible;
				NewElement.ControlEnum = Element.ControlEnum;

				ElementMap.FindOrAdd(Element.GetElementKey()) = NewElement.GetElementKey();
				PastedKeys.Add(NewElement.GetElementKey());

				break;
			}
			case ETLLRN_RigElementType::Space:
			{
				const FTLLRN_RigSpace& Element = *static_cast<FTLLRN_RigSpace*>(Elements[Index].Get());

				FName ParentName = NAME_None;
				if (const FTLLRN_RigElementKey* ParentKey = ElementMap.Find(Element.GetParentElementKey()))
				{
					ParentName = ParentKey->Name;
				}

				FTLLRN_RigSpace& NewElement = SpaceHierarchy.Add(Element.Name, Element.SpaceType, ParentName, Element.InitialTransform);
				ElementMap.FindOrAdd(Element.GetElementKey()) = NewElement.GetElementKey();
				PastedKeys.Add(NewElement.GetElementKey());
				break;
			}
			case ETLLRN_RigElementType::Curve:
			{
				const FTLLRN_RigCurve& Element = *static_cast<FTLLRN_RigCurve*>(Elements[Index].Get());
				FTLLRN_RigCurve& NewElement = CurveContainer.Add(Element.Name, Element.Value);
				ElementMap.FindOrAdd(Element.GetElementKey()) = NewElement.GetElementKey();
				PastedKeys.Add(NewElement.GetElementKey());
				break;
			}
			default:
			{
				ensure(false);
				break;
			}
		}
	}

	return PastedKeys;
}


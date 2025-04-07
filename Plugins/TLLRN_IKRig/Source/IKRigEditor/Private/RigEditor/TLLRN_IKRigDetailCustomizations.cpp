// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigDetailCustomizations.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "IDetailPropertyRow.h"
#include "Widgets/SWidget.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRigDetailCustomizations"

namespace TLLRN_IKRigDetailCustomizationsConstants
{
	static const float ItemWidth = 125.0f;
}

const TArray<FText>& FIKRigGenericDetailCustomization::GetButtonLabels()
{
	static const TArray<FText> ButtonLabels
	{
		LOCTEXT("CurrentTransform", "Current"),
		LOCTEXT("ReferenceTransform", "Reference")
	};
	return ButtonLabels;
}

const TArray<ETLLRN_IKRigTransformType::Type>& FIKRigGenericDetailCustomization::GetTransformTypes()
{
	static const TArray<ETLLRN_IKRigTransformType::Type> TransformTypes
	{
		ETLLRN_IKRigTransformType::Current,
		ETLLRN_IKRigTransformType::Reference
	};
	return TransformTypes;
}

template<typename ClassToCustomize>
TArray<ClassToCustomize*> GetCustomizedObjects(const TArray<TWeakObjectPtr<UObject>>& InObjectsBeingCustomized)
{
	TArray<ClassToCustomize*> CustomizedObjects;
	CustomizedObjects.Reserve(InObjectsBeingCustomized.Num());

	auto IsOfCustomType = [](TWeakObjectPtr<UObject> InObject) { return InObject.Get() && InObject->IsA<ClassToCustomize>(); };
	auto CastAsCustomType = [](TWeakObjectPtr<UObject> InObject) { return Cast<ClassToCustomize>(InObject); };
	Algo::TransformIf(InObjectsBeingCustomized, CustomizedObjects, IsOfCustomType, CastAsCustomType);
	
	return CustomizedObjects;
}

void FIKRigGenericDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized = DetailBuilder.GetSelectedObjects();
	ObjectsBeingCustomized.RemoveAll( [](const TWeakObjectPtr<UObject>& InObject) { return !InObject.IsValid(); } );

	// if no selected objects
	if (ObjectsBeingCustomized.IsEmpty())
	{
		return;
	}
	
	// make sure all types are of the same class
	const UClass* DetailsClass = ObjectsBeingCustomized[0]->GetClass();
	const int32 NumObjects = ObjectsBeingCustomized.Num();
	for (int32 Index = 1; Index < NumObjects; Index++)
	{
		if (ObjectsBeingCustomized[Index]->GetClass() != DetailsClass)
		{
			// multiple different things - fallback to default details panel behavior
			return;
		}
	}

	// assuming the classes are all the same
	if (ObjectsBeingCustomized[0]->IsA<UTLLRN_IKRigBoneDetails>())
	{
		return CustomizeDetailsForClass<UTLLRN_IKRigBoneDetails>(DetailBuilder, ObjectsBeingCustomized);
	}

	if (ObjectsBeingCustomized[0]->IsA<UTLLRN_IKRigEffectorGoal>())
	{
		return CustomizeDetailsForClass<UTLLRN_IKRigEffectorGoal>(DetailBuilder, ObjectsBeingCustomized);
	}
}

template <>
void FIKRigGenericDetailCustomization::CustomizeDetailsForClass<UTLLRN_IKRigBoneDetails>(
	IDetailLayoutBuilder& DetailBuilder,
	const TArray<TWeakObjectPtr<UObject>>& InObjectsBeingCustomized)
{
	const TArray<UTLLRN_IKRigBoneDetails*> Bones = GetCustomizedObjects<UTLLRN_IKRigBoneDetails>(InObjectsBeingCustomized);
	if (Bones.IsEmpty())
	{
		return;
	}

	const TArray<FText>& ButtonLabels = GetButtonLabels();
	const TArray<ETLLRN_IKRigTransformType::Type>& TransformTypes = GetTransformTypes();
	
	static const TArray<FText> ButtonTooltips
	{
		LOCTEXT("CurrentBoneTransformTooltip", "The current transform of the bone"),
		LOCTEXT("ReferenceBoneTransformTooltip", "The reference transform of the bone")
	};
	
	static const TAttribute<TArray<ETLLRN_IKRigTransformType::Type>> VisibleTransforms =
		TArray<ETLLRN_IKRigTransformType::Type>({ETLLRN_IKRigTransformType::Current});

	const TArray<TSharedRef<IPropertyHandle>> Properties
	{
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTLLRN_IKRigBoneDetails, CurrentTransform)),
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTLLRN_IKRigBoneDetails, ReferenceTransform))
	};

	for (TSharedRef<IPropertyHandle> Property : Properties)
	{
		DetailBuilder.HideProperty(Property);
	}

	TSharedPtr<SSegmentedControl<ETLLRN_IKRigTransformType::Type>> TransformChoiceWidget =
		SSegmentedControl<ETLLRN_IKRigTransformType::Type>::Create(
			TransformTypes,
			ButtonLabels,
			ButtonTooltips,
			VisibleTransforms
		);

	DetailBuilder.EditCategory(TEXT("Selection")).SetSortOrder(1);

	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(TEXT("Transforms"));
	CategoryBuilder.SetSortOrder(2);
	CategoryBuilder.AddCustomRow(FText::FromString(TEXT("TransformType")))
	.ValueContent()
	.MinDesiredWidth(375.f)
	.MaxDesiredWidth(375.f)
	.HAlign(HAlign_Left)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			TransformChoiceWidget.ToSharedRef()
		]
	];

	SAdvancedTransformInputBox<FTransform>::FArguments TransformWidgetArgs = SAdvancedTransformInputBox<FTransform>::FArguments()
	.IsEnabled(false)
	.DisplayRelativeWorld(true)
	.DisplayScaleLock(false)
	.AllowEditRotationRepresentation(true)
	.Font(IDetailLayoutBuilder::GetDetailFont())
	.UseQuaternionForRotation(true);
	
	for(int32 PropertyIndex=0;PropertyIndex<Properties.Num();PropertyIndex++)
	{
		const ETLLRN_IKRigTransformType::Type TransformType = (ETLLRN_IKRigTransformType::Type)PropertyIndex; 

		// get/set relative
		TransformWidgetArgs.OnGetIsComponentRelative_Lambda( [Bones, TransformType](ESlateTransformComponent::Type InComponent)
		{
			return Bones.ContainsByPredicate( [&](const UTLLRN_IKRigBoneDetails* Bone)
			{
				return Bone->IsComponentRelative(InComponent, TransformType);
			} );
		})
		.OnIsComponentRelativeChanged_Lambda( [Bones, TransformType](ESlateTransformComponent::Type InComponent, bool bIsRelative)
		{
			for (UTLLRN_IKRigBoneDetails* Bone: Bones)
			{
				Bone->OnComponentRelativeChanged(InComponent, bIsRelative, TransformType);
			}
		} );

		// get bones transforms
		TransformWidgetArgs.Transform_Lambda([Bones, TransformType]()
		{
			TOptional<FTransform> Value = Bones[0]->GetTransform(TransformType);
			if (Value)
			{
				for (int32 Index = 1; Index < Bones.Num(); Index++)
				{
					const TOptional<FTransform> CurrentValue = Bones[Index]->GetTransform(TransformType);
					if (CurrentValue)
					{
						if (!Value->Equals( *CurrentValue))
						{
							return TOptional<FTransform>();
						}
					}
				}
			}
			return Value;
		});
		
		// copy/paste bones transforms
		TransformWidgetArgs.OnCopyToClipboard_UObject(Bones[0], &UTLLRN_IKRigBoneDetails::OnCopyToClipboard, TransformType);
		// FIXME we must connect paste capabilities here otherwise copy is not enabled...
		// see DetailSingleItemRow::OnContextMenuOpening for more explanations
		TransformWidgetArgs.OnPasteFromClipboard_UObject(Bones[0], &UTLLRN_IKRigBoneDetails::OnPasteFromClipboard, TransformType);

		TransformWidgetArgs.Visibility_Lambda([TransformChoiceWidget, TransformType]() -> EVisibility
		{
			return TransformChoiceWidget->HasValue(TransformType) ? EVisibility::Visible : EVisibility::Collapsed;
		});

		SAdvancedTransformInputBox<FTransform>::ConstructGroupedTransformRows(
			CategoryBuilder, 
			ButtonLabels[PropertyIndex], 
			ButtonTooltips[PropertyIndex], 
			TransformWidgetArgs);
	}
}

template <>
void FIKRigGenericDetailCustomization::CustomizeDetailsForClass<UTLLRN_IKRigEffectorGoal>(
	IDetailLayoutBuilder& DetailBuilder,
	const TArray<TWeakObjectPtr<UObject>>& InObjectsBeingCustomized)
{
	const TArray<UTLLRN_IKRigEffectorGoal*> Goals = GetCustomizedObjects<UTLLRN_IKRigEffectorGoal>(InObjectsBeingCustomized);
	if (Goals.IsEmpty())
	{
		return;
	}

	const TArray<FText>& ButtonLabels = GetButtonLabels();
	const TArray<ETLLRN_IKRigTransformType::Type>& TransformTypes = GetTransformTypes();	

	static const TArray<FText> ButtonTooltips
	{
		LOCTEXT("CurrentGoalTransformTooltip", "The current transform of the goal"),
		LOCTEXT("ReferenceGoalTransformTooltip", "The reference transform of the goal")
	};

	static TAttribute<TArray<ETLLRN_IKRigTransformType::Type>> VisibleTransforms =
		TArray<ETLLRN_IKRigTransformType::Type>({ETLLRN_IKRigTransformType::Current});

	const TArray<TSharedRef<IPropertyHandle>> Properties
	{
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTLLRN_IKRigEffectorGoal, CurrentTransform)),
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTLLRN_IKRigEffectorGoal, InitialTransform))
	};

	for (TSharedRef<IPropertyHandle> Property : Properties)
	{
		DetailBuilder.HideProperty(Property);
	}

	TSharedPtr<SSegmentedControl<ETLLRN_IKRigTransformType::Type>> TransformChoiceWidget =
		SSegmentedControl<ETLLRN_IKRigTransformType::Type>::Create(
			TransformTypes,
			ButtonLabels,
			ButtonTooltips,
			VisibleTransforms
		);

	DetailBuilder.EditCategory(TEXT("Goal Settings")).SetSortOrder(1);
	DetailBuilder.EditCategory(TEXT("Viewport Goal Settings")).SetSortOrder(3);

	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(TEXT("Transforms"));
	CategoryBuilder.SetSortOrder(2);
	CategoryBuilder.AddCustomRow(FText::FromString(TEXT("TransformType")))
	.ValueContent()
	.MinDesiredWidth(375.f)
	.MaxDesiredWidth(375.f)
	.HAlign(HAlign_Left)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			TransformChoiceWidget.ToSharedRef()
		]
	];

	SAdvancedTransformInputBox<FTransform>::FArguments TransformWidgetArgs = SAdvancedTransformInputBox<FTransform>::FArguments()
	.IsEnabled(true)
	.DisplayRelativeWorld(false)
	.DisplayScaleLock(true)
	.AllowEditRotationRepresentation(true)
	.Font(IDetailLayoutBuilder::GetDetailFont())
	.UseQuaternionForRotation(true);
	
	for(int32 PropertyIndex=0;PropertyIndex<Properties.Num();PropertyIndex++)
	{
		const ETLLRN_IKRigTransformType::Type TransformType = (ETLLRN_IKRigTransformType::Type)PropertyIndex;

		if(PropertyIndex > 0)
		{
			TransformWidgetArgs
			.IsEnabled(false)
			.DisplayScaleLock(false);
		}

		// get goals transforms
		TransformWidgetArgs.OnGetNumericValue_Lambda( [Goals, TransformType](ESlateTransformComponent::Type Component,
														ESlateRotationRepresentation::Type Representation,
														ESlateTransformSubComponent::Type SubComponent)
		{
			TOptional<FVector::FReal> Value = Goals[0]->GetNumericValue(Component, Representation, SubComponent, TransformType);
			if (Value)
			{
				for (int32 Index = 1; Index < Goals.Num(); Index++)
				{
					const TOptional<FVector::FReal> CurrentValue = Goals[Index]->GetNumericValue(Component, Representation, SubComponent, TransformType);
					if (CurrentValue)
					{
						if (!FMath::IsNearlyEqual(*Value, *CurrentValue))
						{
							return TOptional<FVector::FReal>();
						}
					}
				}
			}
			return Value;
		} );

		// set goals transforms
		TransformWidgetArgs.OnNumericValueChanged_Lambda([this, Goals, TransformType]( ESlateTransformComponent::Type InComponent,
															  ESlateRotationRepresentation::Type InRepresentation,
															  ESlateTransformSubComponent::Type InSubComponent,
															  FVector::FReal InValue)
		{
			FTransform CurrentTransform, UpdatedTransform;
			for (UTLLRN_IKRigEffectorGoal* Goal: Goals)
			{
				Tie(CurrentTransform, UpdatedTransform) =
					Goal->PrepareNumericValueChanged(InComponent, InRepresentation, InSubComponent, InValue, TransformType);

				// don't do anything if the transform has not been changed
				if (!UpdatedTransform.Equals(CurrentTransform))
				{
					// prepare transaction if needed
					if (!ValueChangedTransaction.IsValid())
					{
						ValueChangedTransaction = MakeShareable(new FScopedTransaction(LOCTEXT("ChangeNumericValue", "Change Numeric Value")));
					}
					
					Goal->SetTransform(UpdatedTransform, TransformType);
				}
			}
		})
		.OnNumericValueCommitted_Lambda([this, Goals, TransformType](ESlateTransformComponent::Type InComponent,
											ESlateRotationRepresentation::Type InRepresentation,
											ESlateTransformSubComponent::Type InSubComponent,
											FVector::FReal InValue,
											ETextCommit::Type InCommitType)
		{
			if (!ValueChangedTransaction.IsValid())
			{
				ValueChangedTransaction = MakeShareable(new FScopedTransaction(LOCTEXT("ChangeNumericValue", "Change Numeric Value")));
			}
			
			FTransform CurrentTransform, UpdatedTransform;
			for (UTLLRN_IKRigEffectorGoal* Goal: Goals)
			{
				Tie(CurrentTransform, UpdatedTransform) = Goal->PrepareNumericValueChanged( InComponent, InRepresentation, InSubComponent, InValue, TransformType);

				// don't do anything if the transform has not been changed
				if (!UpdatedTransform.Equals(CurrentTransform))
				{
					Goal->SetTransform(UpdatedTransform, TransformType);
				}
			}
			
			ValueChangedTransaction.Reset();
		});

		// copy/paste values
		TransformWidgetArgs.OnCopyToClipboard_UObject(Goals[0], &UTLLRN_IKRigEffectorGoal::OnCopyToClipboard, TransformType)
		.OnPasteFromClipboard_Lambda([Goals, TransformType](ESlateTransformComponent::Type InComponent)
		{
			FScopedTransaction Transaction(LOCTEXT("PasteTransform", "Paste Transform"));
			for (UTLLRN_IKRigEffectorGoal* Goal: Goals)
			{
				Goal->OnPasteFromClipboard(InComponent, TransformType);
			};
		});

		// reset to default
		const TSharedPtr<IPropertyHandle> PropertyHandle = Properties[PropertyIndex];
		TransformWidgetArgs.DiffersFromDefault_Lambda([Goals,PropertyHandle](ESlateTransformComponent::Type InComponent)
		{
			return Goals.ContainsByPredicate( [&](const UTLLRN_IKRigEffectorGoal* Goal)
			{
				return Goal->TransformDiffersFromDefault(InComponent, PropertyHandle);
			} );
		} )
		.OnResetToDefault_Lambda([Goals,PropertyHandle](ESlateTransformComponent::Type InComponent)
		{
			for (UTLLRN_IKRigEffectorGoal* Goal: Goals)
			{
				Goal->ResetTransformToDefault(InComponent, PropertyHandle);
			}
		} );

		// visibility
		TransformWidgetArgs.Visibility_Lambda([TransformChoiceWidget, TransformType]() -> EVisibility
		{
			return TransformChoiceWidget->HasValue(TransformType) ? EVisibility::Visible : EVisibility::Collapsed;
		});

		SAdvancedTransformInputBox<FTransform>::ConstructGroupedTransformRows(
			CategoryBuilder, 
			ButtonLabels[PropertyIndex], 
			ButtonTooltips[PropertyIndex], 
			TransformWidgetArgs);
	}
}

#undef LOCTEXT_NAMESPACE

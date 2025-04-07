// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/STLLRN_RigHierarchyTagWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "DetailLayoutBuilder.h"
#include "TLLRN_ControlRigDragOps.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "STLLRN_RigHierarchyTagWidget"

//////////////////////////////////////////////////////////////
/// STLLRN_RigHierarchyTagWidget
///////////////////////////////////////////////////////////

void STLLRN_RigHierarchyTagWidget::Construct(const FArguments& InArgs)
{
	Text = InArgs._Text;
	Icon = InArgs._Icon;
	IconColor = InArgs._IconColor;
	Color = InArgs._Color;
	Radius = InArgs._Radius;
	Padding = InArgs._Padding;
	ContentPadding = InArgs._ContentPadding;
	Identifier = InArgs._Identifier;
	bAllowDragDrop = InArgs._AllowDragDrop;
	OnClicked = InArgs._OnClicked;
	OnRenamed = InArgs._OnRenamed;
	OnVerifyRename = InArgs._OnVerifyRename;

	const FMargin CombinedPadding = ContentPadding + Padding;

	SetVisibility(InArgs._Visibility);

	ChildSlot
	[
		SNew(SHorizontalBox)
		.ToolTipText(InArgs._TooltipText)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(CombinedPadding.Left, CombinedPadding.Top, 0, CombinedPadding.Bottom)
		[
			SNew(SImage)
			.Visibility_Lambda([this]()
			{
				return Icon.Get() ? EVisibility::Visible : EVisibility::Collapsed;
			})
			.Image(Icon)
			.ColorAndOpacity(IconColor)
			.DesiredSizeOverride(InArgs._IconSize)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(TAttribute<FMargin>::CreateSP(this, &STLLRN_RigHierarchyTagWidget::GetTextPadding))
		[
			SNew(SInlineEditableTextBlock)
			.Text(Text)
			.ColorAndOpacity(InArgs._TextColor)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.IsReadOnly_Lambda([this](){ return !OnRenamed.IsBound(); })
			.OnTextCommitted(this, &STLLRN_RigHierarchyTagWidget::HandleElementRenamed)
			.OnVerifyTextChanged(this, &STLLRN_RigHierarchyTagWidget::HandleVerifyRename)
		]
	];
}

int32 STLLRN_RigHierarchyTagWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FSlateRoundedBoxBrush RoundedBoxBrush(FLinearColor::White, Radius);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(
			AllottedGeometry.GetLocalSize() - Padding.GetDesiredSize(),
			FSlateLayoutTransform(FVector2d(Padding.Left, Padding.Top))
		),
		&RoundedBoxBrush,
		ESlateDrawEffect::NoPixelSnapping,
		Color.Get()
	);
	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

FReply STLLRN_RigHierarchyTagWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if(OnClicked.IsBound())
		{
			OnClicked.Execute();
		}
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}
	return FReply::Unhandled();
}

FReply STLLRN_RigHierarchyTagWidget::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if(bAllowDragDrop && (Identifier.IsSet() || Identifier.IsBound())) 
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !Identifier.Get().IsEmpty())
		{
			TSharedRef<FTLLRN_RigHierarchyTagDragDropOp> DragDropOp = FTLLRN_RigHierarchyTagDragDropOp::New(SharedThis(this));

			FTLLRN_RigElementKey DraggedKey;
			FTLLRN_RigElementKey::StaticStruct()->ImportText(*Identifier.Get(), &DraggedKey, nullptr, EPropertyPortFlags::PPF_None, nullptr, FTLLRN_RigElementKey::StaticStruct()->GetName(), true);

			if(DraggedKey.IsValid())
			{
				(void)OnElementKeyDragDetectedDelegate.ExecuteIfBound(DraggedKey);
			}

			return FReply::Handled().BeginDragDrop(DragDropOp);
		}
	}
	return FReply::Unhandled();
}

FMargin STLLRN_RigHierarchyTagWidget::GetTextPadding() const
{
	const FMargin CombinedPadding = ContentPadding + Padding;
	if(Icon.IsBound() || Icon.IsSet())
	{
		if(Icon.Get())
		{
			return FMargin(ContentPadding.Left * 2, ContentPadding.Top, ContentPadding.Right, ContentPadding.Bottom);
		}
	}
	return CombinedPadding;
}

void STLLRN_RigHierarchyTagWidget::HandleElementRenamed(const FText& InNewName, ETextCommit::Type InCommitType)
{
	if (InCommitType == ETextCommit::OnEnter)
	{
		if (OnRenamed.IsBound())
		{
			OnRenamed.Execute(InNewName, InCommitType);
		}
	}
}

bool STLLRN_RigHierarchyTagWidget::HandleVerifyRename(const FText& InText, FText& OutError)
{
	if (OnVerifyRename.IsBound())
	{
		return OnVerifyRename.Execute(InText, OutError);
	}
	return false;
}

#undef LOCTEXT_NAMESPACE

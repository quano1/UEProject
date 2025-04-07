// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigDragOps.h"
#include "Editor/STLLRN_RigHierarchyTagWidget.h"

//////////////////////////////////////////////////////////////
/// FTLLRN_RigElementHierarchyDragDropOp
///////////////////////////////////////////////////////////

TSharedRef<FTLLRN_RigElementHierarchyDragDropOp> FTLLRN_RigElementHierarchyDragDropOp::New(const TArray<FTLLRN_RigElementKey>& InElements)
{
	TSharedRef<FTLLRN_RigElementHierarchyDragDropOp> Operation = MakeShared<FTLLRN_RigElementHierarchyDragDropOp>();
	Operation->Elements = InElements;
	Operation->Construct();
	return Operation;
}

TSharedPtr<SWidget> FTLLRN_RigElementHierarchyDragDropOp::GetDefaultDecorator() const
{
	return SNew(SBorder)
		.Visibility(EVisibility::Visible)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		[
			SNew(STextBlock)
			.Text(FText::FromString(GetJoinedElementNames()))
			//.Font(FAppStyle::Get().GetFontStyle("FontAwesome.10"))
		];
}

FString FTLLRN_RigElementHierarchyDragDropOp::GetJoinedElementNames() const
{
	TArray<FString> ElementNameStrings;
	for (const FTLLRN_RigElementKey& Element: Elements)
	{
		ElementNameStrings.Add(Element.Name.ToString());
	}
	return FString::Join(ElementNameStrings, TEXT(","));
}

bool FTLLRN_RigElementHierarchyDragDropOp::IsDraggingSingleConnector() const
{
	if(Elements.Num() == 1)
	{
		return Elements[0].Type == ETLLRN_RigElementType::Connector;
	}
	return false;
}

bool FTLLRN_RigElementHierarchyDragDropOp::IsDraggingSingleSocket() const
{
	if(Elements.Num() == 1)
	{
		return Elements[0].Type == ETLLRN_RigElementType::Socket;
	}
	return false;
}

//////////////////////////////////////////////////////////////
/// FTLLRN_RigHierarchyTagDragDropOp
///////////////////////////////////////////////////////////

TSharedRef<FTLLRN_RigHierarchyTagDragDropOp> FTLLRN_RigHierarchyTagDragDropOp::New(TSharedPtr<STLLRN_RigHierarchyTagWidget> InTagWidget)
{
	TSharedRef<FTLLRN_RigHierarchyTagDragDropOp> Operation = MakeShared<FTLLRN_RigHierarchyTagDragDropOp>();
	Operation->Text = InTagWidget->Text.Get();
	Operation->Identifier = InTagWidget->Identifier.Get();
	Operation->Construct();
	return Operation;
}

TSharedPtr<SWidget> FTLLRN_RigHierarchyTagDragDropOp::GetDefaultDecorator() const
{
	return SNew(SBorder)
		.Visibility(EVisibility::Visible)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		[
			SNew(STextBlock)
			.Text(Text)
		];
}

//////////////////////////////////////////////////////////////
/// FTLLRN_ModularTLLRN_RigModuleDragDropOp
///////////////////////////////////////////////////////////

TSharedRef<FTLLRN_ModularTLLRN_RigModuleDragDropOp> FTLLRN_ModularTLLRN_RigModuleDragDropOp::New(const TArray<FString>& InElements)
{
	TSharedRef<FTLLRN_ModularTLLRN_RigModuleDragDropOp> Operation = MakeShared<FTLLRN_ModularTLLRN_RigModuleDragDropOp>();
	Operation->Elements = InElements;
	Operation->Construct();
	return Operation;
}

TSharedPtr<SWidget> FTLLRN_ModularTLLRN_RigModuleDragDropOp::GetDefaultDecorator() const
{
	return SNew(SBorder)
		.Visibility(EVisibility::Visible)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		[
			SNew(STextBlock)
			.Text(FText::FromString(GetJoinedElementNames()))
			//.Font(FAppStyle::Get().GetFontStyle("FontAwesome.10"))
		];
}

FString FTLLRN_ModularTLLRN_RigModuleDragDropOp::GetJoinedElementNames() const
{
	TArray<FString> ElementNameStrings;
	for (const FString& Element: Elements)
	{
		ElementNameStrings.Add(Element);
	}
	return FString::Join(ElementNameStrings, TEXT(","));
}

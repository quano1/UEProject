// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/Layout/SBox.h"

struct FTLLRN_IKRigLogger;
class IMessageLogListing;
class FTLLRN_IKRetargetEditorController;

class TLLRN_SIKRigOutputLog : public SBox
{
	
public:
	
	SLATE_BEGIN_ARGS(TLLRN_SIKRigOutputLog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const FName& InLogName);

	void ClearLog() const;
	
private:
	
	/** the output log */
	TSharedPtr<SWidget> OutputLogWidget;
	TSharedPtr<IMessageLogListing> MessageLogListing;
};
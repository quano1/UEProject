// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Actions/TLLRN_TLLRN_UVToolAction.h"
#include "CoreMinimal.h"

#include "TLLRN_UVSplitAction.generated.h"

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVSplitAction : public UTLLRN_TLLRN_UVToolAction
{	
	GENERATED_BODY()

public:
	virtual bool CanExecuteAction() const override;
	virtual bool ExecuteAction() override;
};
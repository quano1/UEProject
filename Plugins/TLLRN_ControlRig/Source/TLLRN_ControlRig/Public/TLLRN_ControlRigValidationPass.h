// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "TLLRN_ControlRig.h"
#include "Logging/TokenizedMessage.h"

#include "TLLRN_ControlRigValidationPass.generated.h"

class UTLLRN_ControlRigValidationPass;

DECLARE_DELEGATE(FTLLRN_ControlRigValidationClearDelegate);
DECLARE_DELEGATE_FourParams(FTLLRN_ControlRigValidationReportDelegate, EMessageSeverity::Type, const FTLLRN_RigElementKey&, float, const FString&);
// todo DECLARE_DELEGATE_TwoParams(FTLLRN_ControlRigValidationTLLRN_ControlRigChangedDelegate, UTLLRN_ControlRigValidator*, UTLLRN_ControlRig*);

USTRUCT()
struct FTLLRN_ControlRigValidationContext
{
	GENERATED_BODY()

public:

	FTLLRN_ControlRigValidationContext();

	void Clear();
	void Report(EMessageSeverity::Type InSeverity, const FString& InMessage);
	void Report(EMessageSeverity::Type InSeverity, const FTLLRN_RigElementKey& InKey, const FString& InMessage);
	void Report(EMessageSeverity::Type InSeverity, const FTLLRN_RigElementKey& InKey, float InQuality, const FString& InMessage);
	
	FTLLRN_ControlRigValidationClearDelegate& OnClear() { return ClearDelegate;  }
	FTLLRN_ControlRigValidationReportDelegate& OnReport() { return ReportDelegate; }

	FRigVMDrawInterface* GetDrawInterface() { return DrawInterface; }

	FString GetDisplayNameForEvent(const FName& InEventName) const;

private:

	FTLLRN_ControlRigValidationClearDelegate ClearDelegate;;
	FTLLRN_ControlRigValidationReportDelegate ReportDelegate;

	FRigVMDrawInterface* DrawInterface;

	friend class UTLLRN_ControlRigValidator;
};

/** Used to perform validation on a debugged Control Rig */
UCLASS()
class TLLRN_CONTROLRIG_API UTLLRN_ControlRigValidator : public UObject
{
	GENERATED_UCLASS_BODY()

	UTLLRN_ControlRigValidationPass* FindPass(UClass* InClass) const;
	UTLLRN_ControlRigValidationPass* AddPass(UClass* InClass);
	void RemovePass(UClass* InClass);

	UTLLRN_ControlRig* GetTLLRN_ControlRig() const { return WeakTLLRN_ControlRig.Get(); }
	void SetTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig);

	FTLLRN_ControlRigValidationClearDelegate& OnClear() { return ValidationContext.OnClear(); }
	FTLLRN_ControlRigValidationReportDelegate& OnReport() { return ValidationContext.OnReport(); }
	// todo FTLLRN_ControlRigValidationTLLRN_ControlRigChangedDelegate& OnTLLRN_ControlRigChanged() { return TLLRN_ControlRigChangedDelegate;  }

private:

	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_ControlRigValidationPass>> Passes;

	void OnTLLRN_ControlRigInitialized(URigVMHost* Subject, const FName& EventName);
	void OnTLLRN_ControlRigExecuted(URigVMHost* Subject, const FName& EventName);

	FTLLRN_ControlRigValidationContext ValidationContext;
	TWeakObjectPtr<UTLLRN_ControlRig> WeakTLLRN_ControlRig;
	// todo FTLLRN_ControlRigValidationTLLRN_ControlRigChangedDelegate TLLRN_ControlRigChangedDelegate;
};


/** Used to perform validation on a debugged Control Rig */
UCLASS(Abstract)
class TLLRN_CONTROLRIG_API UTLLRN_ControlRigValidationPass : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	// Called whenever the rig being validated question is changed
	virtual void OnSubjectChanged(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_ControlRigValidationContext* InContext) {}

	// Called whenever the rig in question is initialized
	virtual void OnInitialize(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_ControlRigValidationContext* InContext) {}

	// Called whenever the rig is running an event
	virtual void OnEvent(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName, FTLLRN_ControlRigValidationContext* InContext) {}
};

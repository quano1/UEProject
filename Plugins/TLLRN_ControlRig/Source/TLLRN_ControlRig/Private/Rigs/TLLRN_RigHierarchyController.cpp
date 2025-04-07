// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_RigHierarchyController.h"
#include "TLLRN_ControlRig.h"
#include "AnimationCoreLibrary.h"
#include "UObject/Package.h"
#include "TLLRN_ModularRig.h"
#include "HelperUtil.h"
#include "Engine/SkeletalMesh.h"

#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Styling/AppStyle.h"
#include "ScopedTransaction.h"
#include "RigVMPythonUtils.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigHierarchyController)

////////////////////////////////////////////////////////////////////////////////
// UTLLRN_RigHierarchyController
////////////////////////////////////////////////////////////////////////////////

UTLLRN_RigHierarchyController::~UTLLRN_RigHierarchyController()
{
}

void UTLLRN_RigHierarchyController::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading())
    {
		UTLLRN_RigHierarchy* OuterHierarchy = Cast<UTLLRN_RigHierarchy>(GetOuter());
		SetHierarchy(OuterHierarchy);
    }
}

UTLLRN_RigHierarchy* UTLLRN_RigHierarchyController::GetHierarchy() const
{
	return Cast<UTLLRN_RigHierarchy>(GetOuter());;
}

void UTLLRN_RigHierarchyController::SetHierarchy(UTLLRN_RigHierarchy* InHierarchy)
{
	// since we changed the controller to be a property of the hierarchy,
	// controlling a different hierarchy is no longer allowed
	if (ensure(InHierarchy == GetOuter()))
	{
		InHierarchy->OnModified().RemoveAll(this);
		InHierarchy->OnModified().AddUObject(this, &UTLLRN_RigHierarchyController::HandleHierarchyModified);
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Invalid API Usage, Called UTLLRN_RigHierarchyController::SetHierarchy(...) with a Hierarchy that is not the outer of the controller"));
	}
}

bool UTLLRN_RigHierarchyController::SelectElement(FTLLRN_RigElementKey InKey, bool bSelect, bool bClearSelection)
{
	if(!IsValid())
	{
		return false;
	}

	if(bClearSelection)
	{
		TArray<FTLLRN_RigElementKey> KeysToSelect;
		KeysToSelect.Add(InKey);
		return SetSelection(KeysToSelect);
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	if(UTLLRN_RigHierarchy* HierarchyForSelection = Hierarchy->HierarchyForSelectionPtr.Get())
	{
		if(UTLLRN_RigHierarchyController* ControllerForSelection = HierarchyForSelection->GetController())
		{
			return ControllerForSelection->SelectElement(InKey, bSelect, bClearSelection);
		}
	}

	FTLLRN_RigElementKey Key = InKey;
	if(Hierarchy->ElementKeyRedirector)
	{
		if(const FTLLRN_CachedTLLRN_RigElement* Cache = Hierarchy->ElementKeyRedirector->Find(Key))
		{
			if(const_cast<FTLLRN_CachedTLLRN_RigElement*>(Cache)->UpdateCache(Hierarchy))
			{
				Key = Cache->GetKey();
			}
		}
	}

	FTLLRN_RigBaseElement* Element = Hierarchy->Find(Key);
	if(Element == nullptr)
	{
		return false;
	}

	const bool bSelectionState = Hierarchy->OrderedSelection.Contains(Key);
	ensure(bSelectionState == Element->bSelected);
	if(Element->bSelected == bSelect)
	{
		return false;
	}

	Element->bSelected = bSelect;
	if(bSelect)
	{
		Hierarchy->OrderedSelection.Add(Key);
	}
	else
	{
		Hierarchy->OrderedSelection.Remove(Key);
	}

	if(Element->bSelected)
	{
		Notify(ETLLRN_RigHierarchyNotification::ElementSelected, Element);
	}
	else
	{
		Notify(ETLLRN_RigHierarchyNotification::ElementDeselected, Element);
	}

	Hierarchy->UpdateVisibilityOnProxyControls();

	return true;
}

bool UTLLRN_RigHierarchyController::SetSelection(const TArray<FTLLRN_RigElementKey>& InKeys, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	if(UTLLRN_RigHierarchy* HierarchyForSelection = Hierarchy->HierarchyForSelectionPtr.Get())
	{
		if(UTLLRN_RigHierarchyController* ControllerForSelection = HierarchyForSelection->GetController())
		{
			return ControllerForSelection->SetSelection(InKeys);
		}
	}

	TArray<FTLLRN_RigElementKey> PreviousSelection = Hierarchy->GetSelectedKeys();
	bool bResult = true;

	{
		// disable python printing here as we only want to print a single command instead of one per selected item
		const TGuardValue<bool> Guard(bSuspendPythonPrinting, true);

		for(const FTLLRN_RigElementKey& KeyToDeselect : PreviousSelection)
		{
			if(!InKeys.Contains(KeyToDeselect))
			{
				if(!SelectElement(KeyToDeselect, false))
				{
					bResult = false;
				}
			}
		}

		for(const FTLLRN_RigElementKey& KeyToSelect : InKeys)
		{
			if(!PreviousSelection.Contains(KeyToSelect))
			{
				if(!SelectElement(KeyToSelect, true))
				{
					bResult = false;
				}
			}
		}
	}

#if WITH_EDITOR
	if (bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			const FString Selection = FString::JoinBy( InKeys, TEXT(", "), [](const FTLLRN_RigElementKey& Key)
			{
				return Key.ToPythonString();
			});
			
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.set_selection([%s])"),
				*Selection ) );
		}
	}
#endif

	return bResult;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::AddBone(FName InName, FTLLRN_RigElementKey InParent, FTransform InTransform,
                                                bool bTransformInGlobal, ETLLRN_RigBoneType InBoneType, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return FTLLRN_RigElementKey();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Add Bone", "Add Bone"));
        Hierarchy->Modify();
	}
#endif

	FTLLRN_RigBoneElement* NewElement = MakeElement<FTLLRN_RigBoneElement>();
	{
		TGuardValue<bool> DisableCacheValidityChecks(Hierarchy->bEnableCacheValidityCheck, false);
		NewElement->Key.Type = ETLLRN_RigElementType::Bone;
		NewElement->Key.Name = GetSafeNewName(InName, NewElement->Key.Type);
		NewElement->BoneType = InBoneType;
		AddElement(NewElement, Hierarchy->Get(Hierarchy->GetIndex(InParent)), true, InName);

		if(bTransformInGlobal)
		{
			Hierarchy->SetTransform(NewElement, InTransform, ETLLRN_RigTransformType::InitialGlobal, true, false);
			Hierarchy->SetTransform(NewElement, InTransform, ETLLRN_RigTransformType::CurrentGlobal, true, false);
		}
		else
		{
			Hierarchy->SetTransform(NewElement, InTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
			Hierarchy->SetTransform(NewElement, InTransform, ETLLRN_RigTransformType::CurrentLocal, true, false);
		}

		NewElement->GetTransform().Current = NewElement->GetTransform().Initial;
		NewElement->GetDirtyState().Current = NewElement->GetDirtyState().Initial;
	}

#if WITH_EDITOR
	TransactionPtr.Reset();

	if (bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			TArray<FString> Commands = GetAddBonePythonCommands(NewElement);
			for (const FString& Command : Commands)
			{			
				RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
					FString::Printf(TEXT("%s"), *Command));
			}
		}
	}
#endif

	Hierarchy->EnsureCacheValidity();
		
	return NewElement->Key;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::AddNull(FName InName, FTLLRN_RigElementKey InParent, FTransform InTransform,
                                                bool bTransformInGlobal, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return FTLLRN_RigElementKey();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Add Null", "Add Null"));
		Hierarchy->Modify();
	}
#endif

	FTLLRN_RigNullElement* NewElement = MakeElement<FTLLRN_RigNullElement>();
	{
		TGuardValue<bool> DisableCacheValidityChecks(Hierarchy->bEnableCacheValidityCheck, false);		
		NewElement->Key.Type = ETLLRN_RigElementType::Null;
		NewElement->Key.Name = GetSafeNewName(InName, NewElement->Key.Type);
		AddElement(NewElement, Hierarchy->Get(Hierarchy->GetIndex(InParent)), false, InName);

		if(bTransformInGlobal)
		{
			Hierarchy->SetTransform(NewElement, InTransform, ETLLRN_RigTransformType::InitialGlobal, true, false);
		}
		else
		{
			Hierarchy->SetTransform(NewElement, InTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
		}

		NewElement->GetTransform().Current = NewElement->GetTransform().Initial;
		NewElement->GetDirtyState().Current = NewElement->GetDirtyState().Initial;
	}

#if WITH_EDITOR
	TransactionPtr.Reset();

	if (bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			TArray<FString> Commands = GetAddNullPythonCommands(NewElement);
			for (const FString& Command : Commands)
			{
				RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
					FString::Printf(TEXT("%s"), *Command));
			}
		}
	}
#endif

	Hierarchy->EnsureCacheValidity();

	return NewElement->Key;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::AddControl(
	FName InName,
	FTLLRN_RigElementKey InParent,
	FTLLRN_RigControlSettings InSettings,
	FTLLRN_RigControlValue InValue,
	FTransform InOffsetTransform,
	FTransform InShapeTransform,
	bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return FTLLRN_RigElementKey();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Add Control", "Add Control"));
		Hierarchy->Modify();
	}
#endif

	FTLLRN_RigControlElement* NewElement = MakeElement<FTLLRN_RigControlElement>();
	{
		TGuardValue<bool> DisableCacheValidityChecks(Hierarchy->bEnableCacheValidityCheck, false);		
		NewElement->Key.Type = ETLLRN_RigElementType::Control;
		NewElement->Key.Name = GetSafeNewName(InName, NewElement->Key.Type);
		NewElement->Settings = InSettings;
		if(NewElement->Settings.LimitEnabled.IsEmpty())
		{
			NewElement->Settings.SetupLimitArrayForType();
		}

		if(!NewElement->Settings.DisplayName.IsNone())
		{
			NewElement->Settings.DisplayName = Hierarchy->GetSafeNewDisplayName(InParent, NewElement->Settings.DisplayName); 
		}
		else if(Hierarchy->HasExecuteContext())
		{
			const FTLLRN_ControlRigExecuteContext& CRContext = Hierarchy->ExecuteContext->GetPublicData<FTLLRN_ControlRigExecuteContext>();
			if(!CRContext.GetTLLRN_RigModuleNameSpace().IsEmpty())
			{
				NewElement->Settings.DisplayName = Hierarchy->GetSafeNewDisplayName(InParent, InName);
			}
		}
		
		AddElement(NewElement, Hierarchy->Get(Hierarchy->GetIndex(InParent)), false, InName);
		
		NewElement->GetOffsetTransform().Set(ETLLRN_RigTransformType::InitialLocal, InOffsetTransform);  
		NewElement->GetOffsetDirtyState().MarkClean(ETLLRN_RigTransformType::InitialLocal);
		NewElement->GetShapeTransform().Set(ETLLRN_RigTransformType::InitialLocal, InShapeTransform);
		NewElement->GetShapeDirtyState().MarkClean(ETLLRN_RigTransformType::InitialLocal);
		Hierarchy->SetControlValue(NewElement, InValue, ETLLRN_RigControlValueType::Initial, false);
		const FTransform LocalTransform = Hierarchy->GetTransform(NewElement, ETLLRN_RigTransformType::InitialLocal);
		static constexpr bool bInitial = true;
		Hierarchy->SetControlPreferredEulerAngles(NewElement, LocalTransform, bInitial);

		NewElement->GetOffsetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
		NewElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
		NewElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
		NewElement->GetOffsetTransform().Current = NewElement->GetOffsetTransform().Initial;
		NewElement->GetOffsetDirtyState().Current = NewElement->GetOffsetDirtyState().Initial; 
		NewElement->GetTransform().Current = NewElement->GetTransform().Initial;
		NewElement->GetDirtyState().Current = NewElement->GetDirtyState().Initial;
		NewElement->PreferredEulerAngles.Current = NewElement->PreferredEulerAngles.Initial;
		NewElement->GetShapeTransform().Current = NewElement->GetShapeTransform().Initial;
		NewElement->GetShapeDirtyState().Current = NewElement->GetShapeDirtyState().Initial;
	}

#if WITH_EDITOR
	TransactionPtr.Reset();

	if (bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			TArray<FString> Commands = GetAddControlPythonCommands(NewElement);
			for (const FString& Command : Commands)
			{
				RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
					FString::Printf(TEXT("%s"), *Command));
			}
		}
	}
#endif

	Hierarchy->EnsureCacheValidity();

	return NewElement->Key;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::AddAnimationChannel(FName InName, FTLLRN_RigElementKey InParentControl,
	FTLLRN_RigControlSettings InSettings, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return FTLLRN_RigElementKey();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	if(const FTLLRN_RigControlElement* ParentControl = Hierarchy->Find<FTLLRN_RigControlElement>(InParentControl))
	{
		InSettings.AnimationType = ETLLRN_RigControlAnimationType::AnimationChannel;
		InSettings.bGroupWithParentControl = true;

		return AddControl(InName, ParentControl->GetKey(), InSettings, InSettings.GetIdentityValue(),
			FTransform::Identity, FTransform::Identity, bSetupUndo, bPrintPythonCommand);
	}

	return FTLLRN_RigElementKey();
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::AddCurve(FName InName, float InValue, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return FTLLRN_RigElementKey();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Add Curve", "Add Curve"));
		Hierarchy->Modify();
	}
#endif

	FTLLRN_RigCurveElement* NewElement = MakeElement<FTLLRN_RigCurveElement>();
	{
		TGuardValue<bool> DisableCacheValidityChecks(Hierarchy->bEnableCacheValidityCheck, false);		
		NewElement->Key.Type = ETLLRN_RigElementType::Curve;
		NewElement->Key.Name = GetSafeNewName(InName, NewElement->Key.Type);
		AddElement(NewElement, nullptr, false, InName);
		NewElement->Set(InValue);
		NewElement->bIsValueSet = false;
	}

#if WITH_EDITOR
	TransactionPtr.Reset();

	if (bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			TArray<FString> Commands = GetAddCurvePythonCommands(NewElement);
			for (const FString& Command : Commands)
			{
				RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
					FString::Printf(TEXT("%s"), *Command));
			}
		}
	}
#endif

	Hierarchy->EnsureCacheValidity();

	return NewElement->Key;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::AddPhysicsElement(FName InName, FTLLRN_RigElementKey InParent, FTLLRN_RigPhysicsSolverID InSolver, 
                                                          FTLLRN_RigPhysicsSettings InSettings, FTransform InLocalTransform, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return FTLLRN_RigElementKey();
	}

#if WITH_EDITOR
	if(CVarTLLRN_ControlTLLRN_RigHierarchyEnablePhysics.GetValueOnAnyThread() == false)
	{
		return FTLLRN_RigElementKey();
	}
#endif

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	if(!InSolver.IsValid())
	{
		ReportError(TEXT("Physics Solver Guid is not valid"));
		return FTLLRN_RigElementKey();
	}

	if(Hierarchy->FindPhysicsSolver(InSolver) == nullptr)
	{
		ReportErrorf(TEXT("Physics Solver with guid '%s' cannot be found."), *InSolver.ToString());
		return FTLLRN_RigElementKey();
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Add Physics Element", "Add Physics Element"));
		Hierarchy->Modify();
	}
#endif

	FTLLRN_RigPhysicsElement* NewElement = MakeElement<FTLLRN_RigPhysicsElement>();
	{
		TGuardValue<bool> DisableCacheValidityChecks(Hierarchy->bEnableCacheValidityCheck, false);		
		NewElement->Key.Type = ETLLRN_RigElementType::Physics;
		NewElement->Key.Name = GetSafeNewName(InName, NewElement->Key.Type);
		NewElement->Solver = InSolver;
		NewElement->Settings = InSettings;
		AddElement(NewElement, Hierarchy->Get(Hierarchy->GetIndex(InParent)), true, InName);

		Hierarchy->SetTransform(NewElement, InLocalTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
		NewElement->GetTransform().Current = NewElement->GetTransform().Initial;
		NewElement->GetDirtyState().Current = NewElement->GetDirtyState().Initial;
	}
	
#if WITH_EDITOR
	TransactionPtr.Reset();

	if (bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			TArray<FString> Commands = GetAddPhysicsElementPythonCommands(NewElement);
			for (const FString& Command : Commands)
			{
				RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
					FString::Printf(TEXT("%s"), *Command));
			}
		}
	}
#endif

	GetHierarchy()->EnsureCacheValidity();

	return NewElement->Key;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::AddReference(FName InName, FTLLRN_RigElementKey InParent,
	FRigReferenceGetWorldTransformDelegate InDelegate, bool bSetupUndo)
{
	if(!IsValid())
	{
		return FTLLRN_RigElementKey();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Add Reference", "Add Reference"));
		Hierarchy->Modify();
	}
#endif

	FTLLRN_RigReferenceElement* NewElement = MakeElement<FTLLRN_RigReferenceElement>();
	{
		TGuardValue<bool> DisableCacheValidityChecks(Hierarchy->bEnableCacheValidityCheck, false);		
		NewElement->Key.Type = ETLLRN_RigElementType::Reference;
		NewElement->Key.Name = GetSafeNewName(InName, NewElement->Key.Type);
		NewElement->GetWorldTransformDelegate = InDelegate;
		AddElement(NewElement, Hierarchy->Get(Hierarchy->GetIndex(InParent)), true, InName);

		Hierarchy->SetTransform(NewElement, FTransform::Identity, ETLLRN_RigTransformType::InitialLocal, true, false);
		NewElement->GetTransform().Current = NewElement->GetTransform().Initial;
		NewElement->GetDirtyState().Current = NewElement->GetDirtyState().Initial;
	}

#if WITH_EDITOR
	TransactionPtr.Reset();
#endif

	Hierarchy->EnsureCacheValidity();

	return NewElement->Key;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::AddConnector(FName InName, FTLLRN_RigConnectorSettings InSettings, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return FTLLRN_RigElementKey();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	// only allow to add one primary connector
	if(InSettings.Type == ETLLRN_ConnectorType::Primary)
	{
		const TArray<FTLLRN_RigConnectorElement*>& Connectors = Hierarchy->GetConnectors();
		for(const FTLLRN_RigConnectorElement* Connector : Connectors)
		{
			if(Connector->IsPrimary())
			{
				if(Hierarchy->HasExecuteContext())
				{
					const FTLLRN_ControlRigExecuteContext& CRContext = Hierarchy->ExecuteContext->GetPublicData<FTLLRN_ControlRigExecuteContext>();
					const FString NameSpace = CRContext.GetTLLRN_RigModuleNameSpace();
					if(!NameSpace.IsEmpty())
					{
						const FString ConnectorNameSpace = Hierarchy->GetNameSpace(Connector->GetKey());
						if(!ConnectorNameSpace.IsEmpty() && ConnectorNameSpace.Equals(NameSpace, ESearchCase::IgnoreCase))
						{
							static constexpr TCHAR Format[] = TEXT("Cannot add connector '%s' - there already is a primary connector.");
							ReportAndNotifyErrorf(Format, *InName.ToString());
							return FTLLRN_RigElementKey();
						}
					}
				}
			}
		}

		if(InSettings.bOptional)
		{
			static constexpr TCHAR Format[] = TEXT("Cannot add connector '%s' - primary connectors cannot be optional.");
			ReportAndNotifyErrorf(Format, *InName.ToString());
			return FTLLRN_RigElementKey();
		}
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Add Connector", "Add Connector"));
		Hierarchy->Modify();
	}
#endif

	FTLLRN_RigConnectorElement* NewElement = MakeElement<FTLLRN_RigConnectorElement>();
	{
		TGuardValue<bool> DisableCacheValidityChecks(Hierarchy->bEnableCacheValidityCheck, false);
		NewElement->Key.Type = ETLLRN_RigElementType::Connector;
		NewElement->Key.Name = GetSafeNewName(InName, NewElement->Key.Type);
		NewElement->Settings = InSettings;
		AddElement(NewElement, nullptr, true, InName);
	}

#if WITH_EDITOR
	TransactionPtr.Reset();

	if (bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			TArray<FString> Commands = GetAddConnectorPythonCommands(NewElement);
			for (const FString& Command : Commands)
			{			
				RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
					FString::Printf(TEXT("%s"), *Command));
			}
		}
	}
#endif

	Hierarchy->EnsureCacheValidity();
		
	return NewElement->Key;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::AddSocket(FName InName, FTLLRN_RigElementKey InParent, FTransform InTransform,
	bool bTransformInGlobal, const FLinearColor& InColor, const FString& InDescription, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return FTLLRN_RigElementKey();
	}

	UTLLRN_RigHierarchy* CurrentHierarchy = GetHierarchy();

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Add Socket", "Add Socket"));
		CurrentHierarchy->Modify();
	}
#endif

	FTLLRN_RigSocketElement* NewElement = MakeElement<FTLLRN_RigSocketElement>();
	{
		TGuardValue<bool> DisableCacheValidityChecks(CurrentHierarchy->bEnableCacheValidityCheck, false);
		NewElement->Key.Type = ETLLRN_RigElementType::Socket;
		NewElement->Key.Name = GetSafeNewName(InName, NewElement->Key.Type);
		AddElement(NewElement, CurrentHierarchy->Get(CurrentHierarchy->GetIndex(InParent)), true, InName);

		if(bTransformInGlobal)
		{
			CurrentHierarchy->SetTransform(NewElement, InTransform, ETLLRN_RigTransformType::InitialGlobal, true, false);
			CurrentHierarchy->SetTransform(NewElement, InTransform, ETLLRN_RigTransformType::CurrentGlobal, true, false);
		}
		else
		{
			CurrentHierarchy->SetTransform(NewElement, InTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
			CurrentHierarchy->SetTransform(NewElement, InTransform, ETLLRN_RigTransformType::CurrentLocal, true, false);
		}

		NewElement->GetTransform().Current = NewElement->GetTransform().Initial;
		NewElement->GetDirtyState().Current = NewElement->GetDirtyState().Initial;

		NewElement->SetColor(InColor, CurrentHierarchy);
		NewElement->SetDescription(InDescription, CurrentHierarchy);
		CurrentHierarchy->SetTLLRN_RigElementKeyMetadata(NewElement->Key, FTLLRN_RigSocketElement::DesiredParentMetaName, InParent);
	}

#if WITH_EDITOR
	TransactionPtr.Reset();

	if (bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			TArray<FString> Commands = GetAddSocketPythonCommands(NewElement);
			for (const FString& Command : Commands)
			{			
				RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
					FString::Printf(TEXT("%s"), *Command));
			}
		}
	}
#endif

	CurrentHierarchy->EnsureCacheValidity();
		
	return NewElement->Key;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::AddDefaultRootSocket()
{
	FTLLRN_RigElementKey SocketKey;
	if(const UTLLRN_RigHierarchy* CurrentHierarchy = GetHierarchy())
	{
		static const FTLLRN_RigElementKey RootSocketKey(TEXT("Root"), ETLLRN_RigElementType::Socket);
		if(CurrentHierarchy->Contains(RootSocketKey))
		{
			return RootSocketKey;
		}

		CurrentHierarchy->ForEach<FTLLRN_RigBoneElement>([this, CurrentHierarchy, &SocketKey](const FTLLRN_RigBoneElement* Bone) -> bool
		{
			// find first root bone
			if(CurrentHierarchy->GetNumberOfParents(Bone) == 0)
			{
				SocketKey = AddSocket(RootSocketKey.Name, Bone->GetKey(), FTransform::Identity);

				// stop
				return false;
			}

			// continue with the search
			return true;
		});
	}
	return SocketKey; 
}

FTLLRN_RigControlSettings UTLLRN_RigHierarchyController::GetControlSettings(FTLLRN_RigElementKey InKey) const
{
	if(!IsValid())
	{
		return FTLLRN_RigControlSettings();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(InKey);
	if(ControlElement == nullptr)
	{
		return FTLLRN_RigControlSettings();
	}

	return ControlElement->Settings;
}

bool UTLLRN_RigHierarchyController::SetControlSettings(FTLLRN_RigElementKey InKey, FTLLRN_RigControlSettings InSettings, bool bSetupUndo) const
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(InKey);
	if(ControlElement == nullptr)
	{
		return false;
	}

	#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "SetControlSettings", "Set Control Settings"));
		Hierarchy->Modify();
	}
#endif

	ControlElement->Settings = InSettings;
	if(ControlElement->Settings.LimitEnabled.IsEmpty())
	{
		ControlElement->Settings.SetupLimitArrayForType(false, false, false);
	}

	FTLLRN_RigControlValue InitialValue = Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Initial);
	FTLLRN_RigControlValue CurrentValue = Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);

	ControlElement->Settings.ApplyLimits(InitialValue);
	ControlElement->Settings.ApplyLimits(CurrentValue);

	Hierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);

	Hierarchy->SetControlValue(ControlElement, InitialValue, ETLLRN_RigControlValueType::Initial, bSetupUndo);
	Hierarchy->SetControlValue(ControlElement, CurrentValue, ETLLRN_RigControlValueType::Current, bSetupUndo);

#if WITH_EDITOR
    TransactionPtr.Reset();
#endif

	Hierarchy->EnsureCacheValidity();
	
	return true;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::ImportBones(const FReferenceSkeleton& InSkeleton,
                                                            const FName& InNameSpace, bool bReplaceExistingBones, bool bRemoveObsoleteBones, bool bSelectBones,
                                                            bool bSetupUndo)
{
	TArray<FTLLRN_RigElementKey> AddedBones;

	if(!IsValid())
	{
		return AddedBones;
	}
	
	TArray<FTLLRN_RigElementKey> BonesToSelect;
	TMap<FName, FName> BoneNameMap;

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	Hierarchy->ResetPoseToInitial();

	const TArray<FMeshBoneInfo>& BoneInfos = InSkeleton.GetRefBoneInfo();
	const TArray<FTransform>& BonePoses = InSkeleton.GetRefBonePose();

	struct Local
	{
		static FName DetermineBoneName(const FName& InBoneName, const FName& InLocalNameSpace)
		{
			if (InLocalNameSpace == NAME_None || InBoneName == NAME_None)
			{
				return InBoneName;
			}
			return *FString::Printf(TEXT("%s_%s"), *InLocalNameSpace.ToString(), *InBoneName.ToString());
		}
	};

	if (bReplaceExistingBones)
	{
		TArray<FTLLRN_RigBoneElement*> AllBones = GetHierarchy()->GetElementsOfType<FTLLRN_RigBoneElement>(true);
		for(FTLLRN_RigBoneElement* BoneElement : AllBones)
		{
			BoneNameMap.Add(BoneElement->GetFName(), BoneElement->GetFName());
		}

		for (int32 Index = 0; Index < BoneInfos.Num(); ++Index)
		{
			const FTLLRN_RigElementKey ExistingBoneKey(BoneInfos[Index].Name, ETLLRN_RigElementType::Bone);
			const int32 ExistingBoneIndex = Hierarchy->GetIndex(ExistingBoneKey);
			
			const FName DesiredBoneName = Local::DetermineBoneName(BoneInfos[Index].Name, InNameSpace);
			FName ParentName = (BoneInfos[Index].ParentIndex != INDEX_NONE) ? BoneInfos[BoneInfos[Index].ParentIndex].Name : NAME_None;
			ParentName = Local::DetermineBoneName(ParentName, InNameSpace);

			const FName* MappedParentNamePtr = BoneNameMap.Find(ParentName);
			if (MappedParentNamePtr)
			{
				ParentName = *MappedParentNamePtr;
			}

			const FTLLRN_RigElementKey ParentKey(ParentName, ETLLRN_RigElementType::Bone);

			// if this bone already exists
			if (ExistingBoneIndex != INDEX_NONE)
			{
				const int32 ParentIndex = Hierarchy->GetIndex(ParentKey);
				
				// check it's parent
				if (ParentIndex != INDEX_NONE)
				{
					SetParent(ExistingBoneKey, ParentKey, bSetupUndo);
				}

				Hierarchy->SetInitialLocalTransform(ExistingBoneIndex, BonePoses[Index], true, bSetupUndo);
				Hierarchy->SetLocalTransform(ExistingBoneIndex, BonePoses[Index], true, bSetupUndo);

				BonesToSelect.Add(ExistingBoneKey);
			}
			else
			{
				const FTLLRN_RigElementKey AddedBoneKey = AddBone(DesiredBoneName, ParentKey, BonePoses[Index], false, ETLLRN_RigBoneType::Imported, bSetupUndo);
				BoneNameMap.Add(DesiredBoneName, AddedBoneKey.Name);
				AddedBones.Add(AddedBoneKey);
				BonesToSelect.Add(AddedBoneKey);
			}
		}

	}
	else // import all as new
	{
		for (int32 Index = 0; Index < BoneInfos.Num(); ++Index)
		{
			FName DesiredBoneName = Local::DetermineBoneName(BoneInfos[Index].Name, InNameSpace);
			FName ParentName = (BoneInfos[Index].ParentIndex != INDEX_NONE) ? BoneInfos[BoneInfos[Index].ParentIndex].Name : NAME_None;
			ParentName = Local::DetermineBoneName(ParentName, InNameSpace);

			const FName* MappedParentNamePtr = BoneNameMap.Find(ParentName);
			if (MappedParentNamePtr)
			{
				ParentName = *MappedParentNamePtr;
			}

			const FTLLRN_RigElementKey ParentKey(ParentName, ETLLRN_RigElementType::Bone);
			const FTLLRN_RigElementKey AddedBoneKey = AddBone(DesiredBoneName, ParentKey, BonePoses[Index], false, ETLLRN_RigBoneType::Imported, bSetupUndo);
			BoneNameMap.Add(DesiredBoneName, AddedBoneKey.Name);
			AddedBones.Add(AddedBoneKey);
			BonesToSelect.Add(AddedBoneKey);
		}
	}

	if (bReplaceExistingBones && bRemoveObsoleteBones)
	{
		TMap<FName, int32> BoneNameToIndexInSkeleton;
		for (const FMeshBoneInfo& BoneInfo : BoneInfos)
		{
			FName DesiredBoneName = Local::DetermineBoneName(BoneInfo.Name, InNameSpace);
			BoneNameToIndexInSkeleton.Add(DesiredBoneName, BoneNameToIndexInSkeleton.Num());
		}
		
		TArray<FTLLRN_RigElementKey> BonesToDelete;
		TArray<FTLLRN_RigBoneElement*> AllBones = GetHierarchy()->GetElementsOfType<FTLLRN_RigBoneElement>(true);
		for(FTLLRN_RigBoneElement* BoneElement : AllBones)
        {
            if (!BoneNameToIndexInSkeleton.Contains(BoneElement->GetFName()))
			{
				if (BoneElement->BoneType == ETLLRN_RigBoneType::Imported)
				{
					BonesToDelete.Add(BoneElement->GetKey());
				}
			}
		}

		for (const FTLLRN_RigElementKey& BoneToDelete : BonesToDelete)
		{
			TArray<FTLLRN_RigElementKey> Children = Hierarchy->GetChildren(BoneToDelete);
			Algo::Reverse(Children);
			
			for (const FTLLRN_RigElementKey& Child : Children)
			{
				if(BonesToDelete.Contains(Child))
				{
					continue;
				}
				RemoveAllParents(Child, true, bSetupUndo);
			}
		}

		for (const FTLLRN_RigElementKey& BoneToDelete : BonesToDelete)
		{
			RemoveElement(BoneToDelete);
			BonesToSelect.Remove(BoneToDelete);
		}

		// update the sub index to match the bone index in the skeleton
		for (int32 Index = 0; Index < BoneInfos.Num(); ++Index)
		{
			FName DesiredBoneName = Local::DetermineBoneName(BoneInfos[Index].Name, InNameSpace);
			const FTLLRN_RigElementKey Key(DesiredBoneName, ETLLRN_RigElementType::Bone);
			if(FTLLRN_RigBoneElement* BoneElement = Hierarchy->Find<FTLLRN_RigBoneElement>(Key))
			{
				BoneElement->SubIndex = Index;
			}
		}		
	}

	if (bSelectBones)
	{
		SetSelection(BonesToSelect);
	}

	Hierarchy->EnsureCacheValidity();

	return AddedBones;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::ImportBones(USkeleton* InSkeleton, FName InNameSpace,
                                                            bool bReplaceExistingBones, bool bRemoveObsoleteBones,
                                                            bool bSelectBones, bool bSetupUndo,
                                                            bool bPrintPythonCommand)
{
	FReferenceSkeleton EmptySkeleton;
	FReferenceSkeleton& RefSkeleton = EmptySkeleton; 
	if (InSkeleton != nullptr)
	{
		RefSkeleton = InSkeleton->GetReferenceSkeleton();
	}

	const TArray<FTLLRN_RigElementKey> BoneKeys = ImportBones(RefSkeleton, InNameSpace, bReplaceExistingBones, bRemoveObsoleteBones,
	                   bSelectBones, bSetupUndo);

#if WITH_EDITOR
	if (!BoneKeys.IsEmpty() && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.import_bones_from_asset('%s', '%s', %s, %s, %s)"),
				(InSkeleton != nullptr) ? *InSkeleton->GetPathName() : TEXT(""),
				*InNameSpace.ToString(),
				(bReplaceExistingBones) ? TEXT("True") : TEXT("False"),
				(bRemoveObsoleteBones) ? TEXT("True") : TEXT("False"),
				(bSelectBones) ? TEXT("True") : TEXT("False")));
		}
	}
#endif
	return BoneKeys;
}

#if WITH_EDITOR

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::ImportBonesFromAsset(FString InAssetPath, FName InNameSpace,
	bool bReplaceExistingBones, bool bRemoveObsoleteBones, bool bSelectBones, bool bSetupUndo)
{
	if(USkeleton* Skeleton = GetSkeletonFromAssetPath(InAssetPath))
	{
		return ImportBones(Skeleton, InNameSpace, bReplaceExistingBones, bRemoveObsoleteBones, bSelectBones, bSetupUndo);
	}
	return TArray<FTLLRN_RigElementKey>();
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::ImportCurvesFromAsset(FString InAssetPath, FName InNameSpace,
                                                                      bool bSelectCurves, bool bSetupUndo)
{
	if(USkeletalMesh* SkeletalMesh = GetSkeletalMeshFromAssetPath(InAssetPath))
	{
		return ImportCurvesFromSkeletalMesh(SkeletalMesh, InNameSpace, bSelectCurves, bSetupUndo);
	}
	if(USkeleton* Skeleton = GetSkeletonFromAssetPath(InAssetPath))
	{
		return ImportCurves(Skeleton, InNameSpace, bSelectCurves, bSetupUndo);
	}
	return TArray<FTLLRN_RigElementKey>();
}

USkeletalMesh* UTLLRN_RigHierarchyController::GetSkeletalMeshFromAssetPath(const FString& InAssetPath)
{
	UObject* AssetObject = StaticLoadObject(UObject::StaticClass(), NULL, *InAssetPath, NULL, LOAD_None, NULL);
	if(AssetObject == nullptr)
	{
		return nullptr;
	}

	if(USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(AssetObject))
	{
		return SkeletalMesh;
	}

	return nullptr;
}

USkeleton* UTLLRN_RigHierarchyController::GetSkeletonFromAssetPath(const FString& InAssetPath)
{
	UObject* AssetObject = StaticLoadObject(UObject::StaticClass(), NULL, *InAssetPath, NULL, LOAD_None, NULL);
	if(AssetObject == nullptr)
	{
		return nullptr;
    }

	if(USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(AssetObject))
	{
		return SkeletalMesh->GetSkeleton();
	}

	if(USkeleton* Skeleton = Cast<USkeleton>(AssetObject))
	{
		return Skeleton;
	}

	return nullptr;
}

#endif

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::ImportCurves(UAnimCurveMetaData* InAnimCurvesMetadata, FName InNameSpace, bool bSetupUndo)
{
	TArray<FTLLRN_RigElementKey> Keys;
	if (InAnimCurvesMetadata == nullptr)
	{
		return Keys;
	}

	if(!IsValid())
	{
		return Keys;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	FTLLRN_RigHierarchyInteractionBracket InteractionBracket(Hierarchy);

	InAnimCurvesMetadata->ForEachCurveMetaData([this, Hierarchy, InNameSpace, &Keys, bSetupUndo](const FName& InCurveName, const FCurveMetaData& InMetaData)
	{
		FName Name = InCurveName;
		if (!InNameSpace.IsNone())
		{
			Name = *FString::Printf(TEXT("%s::%s"), *InNameSpace.ToString(), *InCurveName.ToString());
		}

		const FTLLRN_RigElementKey ExpectedKey(Name, ETLLRN_RigElementType::Curve);
		if(Hierarchy->Contains(ExpectedKey))
		{
			Keys.Add(ExpectedKey);
			return;
		}
		
		const FTLLRN_RigElementKey CurveKey = AddCurve(Name, 0.f, bSetupUndo);
		Keys.Add(FTLLRN_RigElementKey(Name, ETLLRN_RigElementType::Curve));
	});

	return Keys;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::ImportCurves(USkeleton* InSkeleton, FName InNameSpace,
                                                             bool bSelectCurves, bool bSetupUndo, bool bPrintPythonCommand)
{
	TArray<FTLLRN_RigElementKey> Keys;
	if (InSkeleton == nullptr)
	{
		return Keys;
	}

	if(!IsValid())
	{
		return Keys;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	FTLLRN_RigHierarchyInteractionBracket InteractionBracket(Hierarchy);

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Import Curves", "Import Curves"));
		Hierarchy->Modify();
	}
#endif

	Keys.Append(ImportCurves(InSkeleton->GetAssetUserData<UAnimCurveMetaData>(), InNameSpace, bSetupUndo));

	if(bSelectCurves)
	{
		SetSelection(Keys);
	}
	
#if WITH_EDITOR
	if (!Keys.IsEmpty() && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.import_curves_from_asset('%s', '%s', %s)"),
				*InSkeleton->GetPathName(),
				*InNameSpace.ToString(),
				(bSelectCurves) ? TEXT("True") : TEXT("False")));
		}
	}
#endif

	Hierarchy->EnsureCacheValidity();

	return Keys;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::ImportCurvesFromSkeletalMesh(USkeletalMesh* InSkeletalMesh, FName InNameSpace,
	bool bSelectCurves, bool bSetupUndo, bool bPrintPythonCommand)
{
	TArray<FTLLRN_RigElementKey> Keys;
	if(InSkeletalMesh == nullptr)
	{
		return Keys;
	}

	if(!IsValid())
	{
		return Keys;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	FTLLRN_RigHierarchyInteractionBracket InteractionBracket(Hierarchy);

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Import Curves", "Import Curves"));
		Hierarchy->Modify();
	}
#endif

	Keys.Append(ImportCurves(InSkeletalMesh->GetSkeleton(), InNameSpace, false, bSetupUndo, false));
	Keys.Append(ImportCurves(InSkeletalMesh->GetAssetUserData<UAnimCurveMetaData>(), InNameSpace, bSetupUndo));
	
	if(bSelectCurves)
	{
		SetSelection(Keys);
	}
	
#if WITH_EDITOR
	if (!Keys.IsEmpty() && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.import_curves_from_asset('%s', '%s', %s)"),
				*InSkeletalMesh->GetPathName(),
				*InNameSpace.ToString(),
				(bSelectCurves) ? TEXT("True") : TEXT("False")));
		}
	}
#endif

	Hierarchy->EnsureCacheValidity();

	return Keys;
}

FString UTLLRN_RigHierarchyController::ExportSelectionToText() const
{
	if(!IsValid())
	{
		return FString();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	return ExportToText(Hierarchy->GetSelectedKeys());
}

FString UTLLRN_RigHierarchyController::ExportToText(TArray<FTLLRN_RigElementKey> InKeys) const
{
	if(!IsValid() || InKeys.IsEmpty())
	{
		return FString();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	Hierarchy->ComputeAllTransforms();

	// sort the keys by traversal order
	TArray<FTLLRN_RigElementKey> Keys = Hierarchy->SortKeys(InKeys);

	FTLLRN_RigHierarchyCopyPasteContent Data;
	for (const FTLLRN_RigElementKey& Key : Keys)
	{
		FTLLRN_RigBaseElement* Element = Hierarchy->Find(Key);
		if(Element == nullptr)
		{
			continue;
		}

		FTLLRN_RigHierarchyCopyPasteContentPerElement PerElementData;
		PerElementData.Key = Key;
		PerElementData.Parents = Hierarchy->GetParents(Key);

		if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(Element))
		{
			ensure(PerElementData.Parents.Num() == MultiParentElement->ParentConstraints.Num());

			for(const FTLLRN_RigElementParentConstraint& ParentConstraint : MultiParentElement->ParentConstraints)
			{
				PerElementData.ParentWeights.Add(ParentConstraint.Weight);
			}
		}
		else
		{
			PerElementData.ParentWeights.SetNumZeroed(PerElementData.Parents.Num());
			if(PerElementData.ParentWeights.Num() > 0)
			{
				PerElementData.ParentWeights[0] = 1.f;
			}
		}

		if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Element))
		{
			PerElementData.Poses.Add(Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::InitialLocal));
			PerElementData.Poses.Add(Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::CurrentLocal));
			PerElementData.Poses.Add(Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::InitialGlobal));
			PerElementData.Poses.Add(Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::CurrentGlobal));
			PerElementData.DirtyStates.Add(TransformElement->GetDirtyState().GetDirtyFlag(ETLLRN_RigTransformType::InitialLocal));
			PerElementData.DirtyStates.Add(TransformElement->GetDirtyState().GetDirtyFlag(ETLLRN_RigTransformType::CurrentLocal));
			PerElementData.DirtyStates.Add(TransformElement->GetDirtyState().GetDirtyFlag(ETLLRN_RigTransformType::InitialGlobal));
			PerElementData.DirtyStates.Add(TransformElement->GetDirtyState().GetDirtyFlag(ETLLRN_RigTransformType::CurrentGlobal));
		}

		switch (Key.Type)
		{
			case ETLLRN_RigElementType::Bone:
			{
				FTLLRN_RigBoneElement DefaultElement;
				FTLLRN_RigBoneElement::StaticStruct()->ExportText(PerElementData.Content, Element, &DefaultElement, nullptr, PPF_None, nullptr);
				break;
			}
			case ETLLRN_RigElementType::Control:
			{
				FTLLRN_RigControlElement DefaultElement;
				FTLLRN_RigControlElement::StaticStruct()->ExportText(PerElementData.Content, Element, &DefaultElement, nullptr, PPF_None, nullptr);
				break;
			}
			case ETLLRN_RigElementType::Null:
			{
				FTLLRN_RigNullElement DefaultElement;
				FTLLRN_RigNullElement::StaticStruct()->ExportText(PerElementData.Content, Element, &DefaultElement, nullptr, PPF_None, nullptr);
				break;
			}
			case ETLLRN_RigElementType::Curve:
			{
				FTLLRN_RigCurveElement DefaultElement;
				FTLLRN_RigCurveElement::StaticStruct()->ExportText(PerElementData.Content, Element, &DefaultElement, nullptr, PPF_None, nullptr);
				break;
			}
			case ETLLRN_RigElementType::Physics:
			{
				FTLLRN_RigPhysicsElement DefaultElement;
				FTLLRN_RigPhysicsElement::StaticStruct()->ExportText(PerElementData.Content, Element, &DefaultElement, nullptr, PPF_None, nullptr);
				break;
			}
			case ETLLRN_RigElementType::Reference:
			{
				FTLLRN_RigReferenceElement DefaultElement;
				FTLLRN_RigReferenceElement::StaticStruct()->ExportText(PerElementData.Content, Element, &DefaultElement, nullptr, PPF_None, nullptr);
				break;
			}
			case ETLLRN_RigElementType::Connector:
			{
				FTLLRN_RigConnectorElement DefaultElement;
				FTLLRN_RigConnectorElement::StaticStruct()->ExportText(PerElementData.Content, Element, &DefaultElement, nullptr, PPF_None, nullptr);
				break;
			}
			case ETLLRN_RigElementType::Socket:
			{
				FTLLRN_RigSocketElement DefaultElement;
				FTLLRN_RigSocketElement::StaticStruct()->ExportText(PerElementData.Content, Element, &DefaultElement, nullptr, PPF_None, nullptr);
				break;
			}
			default:
			{
				ensure(false);
				break;
			}
		}

		Data.Elements.Add(PerElementData);
	}

	FString ExportedText;
	FTLLRN_RigHierarchyCopyPasteContent DefaultContent;
	FTLLRN_RigHierarchyCopyPasteContent::StaticStruct()->ExportText(ExportedText, &Data, &DefaultContent, nullptr, PPF_None, nullptr);
	return ExportedText;
}

class FTLLRN_RigHierarchyImportErrorContext : public FOutputDevice
{
public:

	int32 NumErrors;

	FTLLRN_RigHierarchyImportErrorContext()
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

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::ImportFromText(FString InContent, bool bReplaceExistingElements, bool bSelectNewElements, bool bSetupUndo, bool bPrintPythonCommands)
{
	return ImportFromText(InContent, ETLLRN_RigElementType::All, bReplaceExistingElements, bSelectNewElements, bSetupUndo, bPrintPythonCommands);
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::ImportFromText(FString InContent, ETLLRN_RigElementType InAllowedTypes, bool bReplaceExistingElements, bool bSelectNewElements, bool bSetupUndo, bool bPrintPythonCommands)
{
	TArray<FTLLRN_RigElementKey> PastedKeys;
	if(!IsValid())
	{
		return PastedKeys;
	}

	FTLLRN_RigHierarchyCopyPasteContent Data;
	FTLLRN_RigHierarchyImportErrorContext ErrorPipe;
	FTLLRN_RigHierarchyCopyPasteContent::StaticStruct()->ImportText(*InContent, &Data, nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigHierarchyCopyPasteContent::StaticStruct()->GetName(), true);
	if (ErrorPipe.NumErrors > 0)
	{
		return PastedKeys;
	}

	if (Data.Elements.Num() == 0)
	{
		// check if this is a copy & paste buffer from pre-5.0
		if(Data.Contents.Num() > 0)
		{
			const int32 OriginalNumElements = Data.Elements.Num();
			for (int32 i=0; i<Data.Types.Num(); )
			{
				if (((uint8)InAllowedTypes & (uint8)Data.Types[i]) == 0)
				{
					Data.Contents.RemoveAt(i);
					Data.Types.RemoveAt(i);
					Data.LocalTransforms.RemoveAt(i);
					Data.GlobalTransforms.RemoveAt(i);
					continue;
				}
				++i;
			}
			if (OriginalNumElements > Data.Types.Num())
			{
				ReportAndNotifyErrorf(TEXT("Some elements were not allowed to be pasted."));
			}
			FTLLRN_RigHierarchyContainer OldHierarchy;
			if(OldHierarchy.ImportFromText(Data).Num() > 0)
			{
				return ImportFromHierarchyContainer(OldHierarchy, true);
			}
		}
		
		return PastedKeys;
	}

	const int32 OriginalNumElements = Data.Elements.Num();
	Data.Elements = Data.Elements.FilterByPredicate([InAllowedTypes](const FTLLRN_RigHierarchyCopyPasteContentPerElement& Element)
	{
		return ((uint8)InAllowedTypes & (uint8)Element.Key.Type) != 0;
	});
	if (OriginalNumElements > Data.Elements.Num())
	{
		ReportAndNotifyErrorf(TEXT("Some elements were not allowed to be pasted."));
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Add Elements", "Add Elements"));
		Hierarchy->Modify();
	}
#endif

	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> KeyMap;
	for(FTLLRN_RigBaseElement* Element : *Hierarchy)
	{
		KeyMap.Add(Element->GetKey(), Element->GetKey());
	}
	TArray<FTLLRN_RigElementKey> PreviouslyExistingKeys;

	FTLLRN_RigHierarchyInteractionBracket InteractionBracket(Hierarchy);

	for(const FTLLRN_RigHierarchyCopyPasteContentPerElement& PerElementData : Data.Elements)
	{
		ErrorPipe.NumErrors = 0;

		FTLLRN_RigBaseElement* NewElement = nullptr;

		switch (PerElementData.Key.Type)
		{
			case ETLLRN_RigElementType::Bone:
			{
				NewElement = MakeElement<FTLLRN_RigBoneElement>(true);
				FTLLRN_RigBoneElement::StaticStruct()->ImportText(*PerElementData.Content, NewElement, nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigBoneElement::StaticStruct()->GetName(), true);
				CastChecked<FTLLRN_RigBoneElement>(NewElement)->BoneType = ETLLRN_RigBoneType::User;
				break;
			}
			case ETLLRN_RigElementType::Null:
			{
				NewElement = MakeElement<FTLLRN_RigNullElement>(true);
				FTLLRN_RigNullElement::StaticStruct()->ImportText(*PerElementData.Content, NewElement, nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigNullElement::StaticStruct()->GetName(), true);
				break;
			}
			case ETLLRN_RigElementType::Control:
			{
				NewElement = MakeElement<FTLLRN_RigControlElement>(true);
				FTLLRN_RigControlElement::StaticStruct()->ImportText(*PerElementData.Content, NewElement, nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigControlElement::StaticStruct()->GetName(), true);
				break;
			}
			case ETLLRN_RigElementType::Curve:
			{
				NewElement = MakeElement<FTLLRN_RigCurveElement>(true);
				FTLLRN_RigCurveElement::StaticStruct()->ImportText(*PerElementData.Content, NewElement, nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigCurveElement::StaticStruct()->GetName(), true);
				break;
			}
			case ETLLRN_RigElementType::Physics:
			{
				NewElement = MakeElement<FTLLRN_RigPhysicsElement>(true);
				FTLLRN_RigPhysicsElement::StaticStruct()->ImportText(*PerElementData.Content, NewElement, nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigPhysicsElement::StaticStruct()->GetName(), true);
				break;
			}
			case ETLLRN_RigElementType::Reference:
			{
				NewElement = MakeElement<FTLLRN_RigReferenceElement>(true);
				FTLLRN_RigReferenceElement::StaticStruct()->ImportText(*PerElementData.Content, NewElement, nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigReferenceElement::StaticStruct()->GetName(), true);
				break;
			}
			case ETLLRN_RigElementType::Connector:
			{
				NewElement = MakeElement<FTLLRN_RigConnectorElement>(true);
				FTLLRN_RigConnectorElement::StaticStruct()->ImportText(*PerElementData.Content, NewElement, nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigConnectorElement::StaticStruct()->GetName(), true);
				break;
			}
			case ETLLRN_RigElementType::Socket:
			{
				NewElement = MakeElement<FTLLRN_RigSocketElement>(true);
				FTLLRN_RigSocketElement::StaticStruct()->ImportText(*PerElementData.Content, NewElement, nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigSocketElement::StaticStruct()->GetName(), true);
				break;
			}
			default:
			{
				ensure(false);
				break;
			}
		}

		check(NewElement);
		NewElement->Key = PerElementData.Key;

		if(bReplaceExistingElements)
		{
			if(FTLLRN_RigBaseElement* ExistingElement = Hierarchy->Find(NewElement->GetKey()))
			{
				ExistingElement->CopyPose(NewElement, true, true, false);

				if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(ExistingElement))
				{
					Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
					Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::InitialLocal);
					ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);
					ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
				}
				
				TArray<FTLLRN_RigElementKey> CurrentParents = Hierarchy->GetParents(NewElement->GetKey());

				bool bUpdateParents = CurrentParents.Num() != PerElementData.Parents.Num();
				if(!bUpdateParents)
				{
					for(const FTLLRN_RigElementKey& CurrentParent : CurrentParents)
					{
						if(!PerElementData.Parents.Contains(CurrentParent))
						{
							bUpdateParents = true;
							break;
						}
					}
				}

				if(bUpdateParents)
				{
					RemoveAllParents(ExistingElement->GetKey(), true, bSetupUndo);

					for(const FTLLRN_RigElementKey& NewParent : PerElementData.Parents)
					{
						AddParent(ExistingElement->GetKey(), NewParent, 0.f, true, bSetupUndo);
					}
				}
				
				for(int32 ParentIndex = 0; ParentIndex < PerElementData.ParentWeights.Num(); ParentIndex++)
				{
					Hierarchy->SetParentWeight(ExistingElement, ParentIndex, PerElementData.ParentWeights[ParentIndex], true, true);
					Hierarchy->SetParentWeight(ExistingElement, ParentIndex, PerElementData.ParentWeights[ParentIndex], false, true);
				}

				PastedKeys.Add(ExistingElement->GetKey());
				PreviouslyExistingKeys.Add(ExistingElement->GetKey());

				Hierarchy->DestroyElement(NewElement);
				continue;
			}
		}

		const FName DesiredName = NewElement->Key.Name;
		NewElement->Key.Name = GetSafeNewName(DesiredName, NewElement->Key.Type);
		AddElement(NewElement, nullptr, true, DesiredName);

		KeyMap.FindOrAdd(PerElementData.Key) = NewElement->Key;
	}

	Hierarchy->UpdateElementStorage();
	
	for(const FTLLRN_RigHierarchyCopyPasteContentPerElement& PerElementData : Data.Elements)
	{
		if(PreviouslyExistingKeys.Contains(PerElementData.Key))
		{
			continue;
		}
		
		FTLLRN_RigElementKey MappedKey = KeyMap.FindChecked(PerElementData.Key);
		FTLLRN_RigBaseElement* NewElement = Hierarchy->FindChecked(MappedKey);

		for(const FTLLRN_RigElementKey& OriginalParent : PerElementData.Parents)
		{
			FTLLRN_RigElementKey Parent = OriginalParent;
			if(const FTLLRN_RigElementKey* RemappedParent = KeyMap.Find(Parent))
			{
				Parent = *RemappedParent;
			}

			AddParent(NewElement->GetKey(), Parent, 0.f, true, bSetupUndo);
		}
		
		for(int32 ParentIndex = 0; ParentIndex < PerElementData.ParentWeights.Num(); ParentIndex++)
		{
			Hierarchy->SetParentWeight(NewElement, ParentIndex, PerElementData.ParentWeights[ParentIndex], true, true);
			Hierarchy->SetParentWeight(NewElement, ParentIndex, PerElementData.ParentWeights[ParentIndex], false, true);
		}

		PastedKeys.AddUnique(NewElement->GetKey());
	}

	for(const FTLLRN_RigHierarchyCopyPasteContentPerElement& PerElementData : Data.Elements)
	{
		FTLLRN_RigElementKey MappedKey = KeyMap.FindChecked(PerElementData.Key);
		FTLLRN_RigBaseElement* Element = Hierarchy->FindChecked(MappedKey);

		if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Element))
		{
			if(PerElementData.Poses.Num() >= 2)
			{
				Hierarchy->SetTransform(TransformElement, PerElementData.Poses[0], ETLLRN_RigTransformType::InitialLocal, true, true);
				Hierarchy->SetTransform(TransformElement, PerElementData.Poses[1], ETLLRN_RigTransformType::CurrentLocal, true, true);
			}
		}
	}
	
#if WITH_EDITOR
	TransactionPtr.Reset();
#endif

	if(bSelectNewElements)
	{
		SetSelection(PastedKeys);
	}

#if WITH_EDITOR
	if (bPrintPythonCommands && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			FString PythonContent = InContent.Replace(TEXT("\\\""), TEXT("\\\\\""));
		
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.import_from_text('%s', %s, %s)"),
				*PythonContent,
				(bReplaceExistingElements) ? TEXT("True") : TEXT("False"),
				(bSelectNewElements) ? TEXT("True") : TEXT("False")));
		}
	}
#endif

	Hierarchy->EnsureCacheValidity();

	return PastedKeys;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::ImportFromHierarchyContainer(const FTLLRN_RigHierarchyContainer& InContainer, bool bIsCopyAndPaste)
{
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> KeyMap;;
	FTLLRN_RigHierarchyInteractionBracket InteractionBracket(Hierarchy);

	for(const FTLLRN_RigBone& Bone : InContainer.BoneHierarchy)
	{
		const FTLLRN_RigElementKey OriginalParentKey = Bone.GetParentElementKey(true);
		const FTLLRN_RigElementKey* ParentKey = nullptr;
		if(OriginalParentKey.IsValid())
		{
			ParentKey = KeyMap.Find(OriginalParentKey);
		}
		if(ParentKey == nullptr)
		{
			ParentKey = &OriginalParentKey;
		}

		const FTLLRN_RigElementKey Key = AddBone(Bone.Name, *ParentKey, Bone.InitialTransform, true, bIsCopyAndPaste ? ETLLRN_RigBoneType::User : Bone.Type, false);
		KeyMap.Add(Bone.GetElementKey(), Key);
	}
	for(const FTLLRN_RigSpace& Space : InContainer.SpaceHierarchy)
	{
		const FTLLRN_RigElementKey Key = AddNull(Space.Name, FTLLRN_RigElementKey(), Space.InitialTransform, false, false);
		KeyMap.Add(Space.GetElementKey(), Key);
	}
	for(const FTLLRN_RigControl& Control : InContainer.ControlHierarchy)
	{
		FTLLRN_RigControlSettings Settings;
		Settings.ControlType = Control.ControlType;
		Settings.DisplayName = Control.DisplayName;
		Settings.PrimaryAxis = Control.PrimaryAxis;
		Settings.bIsCurve = Control.bIsCurve;
		Settings.SetAnimationTypeFromDeprecatedData(Control.bAnimatable, Control.bGizmoEnabled);
		Settings.SetupLimitArrayForType(Control.bLimitTranslation, Control.bLimitRotation, Control.bLimitScale);
		Settings.bDrawLimits = Control.bDrawLimits;
		Settings.MinimumValue = Control.MinimumValue;
		Settings.MaximumValue = Control.MaximumValue;
		Settings.bShapeVisible = Control.bGizmoVisible;
		Settings.ShapeName = Control.GizmoName;
		Settings.ShapeColor = Control.GizmoColor;
		Settings.ControlEnum = Control.ControlEnum;
		Settings.bGroupWithParentControl = Settings.IsAnimatable() && (
			Settings.ControlType == ETLLRN_RigControlType::Bool ||
			Settings.ControlType == ETLLRN_RigControlType::Float ||
			Settings.ControlType == ETLLRN_RigControlType::ScaleFloat ||
			Settings.ControlType == ETLLRN_RigControlType::Integer ||
			Settings.ControlType == ETLLRN_RigControlType::Vector2D
		);

		if(Settings.ShapeName == FTLLRN_RigControl().GizmoName)
		{
			Settings.ShapeName = FTLLRN_ControlRigShapeDefinition().ShapeName; 
		}

		FTLLRN_RigControlValue InitialValue = Control.InitialValue;

#if WITH_EDITORONLY_DATA
		if(!InitialValue.IsValid())
		{
			InitialValue.SetFromTransform(InitialValue.Storage_DEPRECATED, Settings.ControlType, Settings.PrimaryAxis);
		}
#endif
		
		const FTLLRN_RigElementKey Key = AddControl(
			Control.Name,
			FTLLRN_RigElementKey(),
			Settings,
			InitialValue,
			Control.OffsetTransform,
			Control.GizmoTransform,
			false);

		KeyMap.Add(Control.GetElementKey(), Key);
	}
	
	for(const FTLLRN_RigCurve& Curve : InContainer.CurveContainer)
	{
		const FTLLRN_RigElementKey Key = AddCurve(Curve.Name, Curve.Value, false);
		KeyMap.Add(Curve.GetElementKey(), Key);
	}

	for(const FTLLRN_RigSpace& Space : InContainer.SpaceHierarchy)
	{
		const FTLLRN_RigElementKey SpaceKey = KeyMap.FindChecked(Space.GetElementKey());
		const FTLLRN_RigElementKey OriginalParentKey = Space.GetParentElementKey();
		if(OriginalParentKey.IsValid())
		{
			FTLLRN_RigElementKey ParentKey;
			if(const FTLLRN_RigElementKey* ParentKeyPtr = KeyMap.Find(OriginalParentKey))
			{
				ParentKey = *ParentKeyPtr;
			}
			SetParent(SpaceKey, ParentKey, false, false);
		}
	}

	for(const FTLLRN_RigControl& Control : InContainer.ControlHierarchy)
	{
		const FTLLRN_RigElementKey ControlKey = KeyMap.FindChecked(Control.GetElementKey());
		FTLLRN_RigElementKey OriginalParentKey = Control.GetParentElementKey();
		const FTLLRN_RigElementKey SpaceKey = Control.GetSpaceElementKey();
		OriginalParentKey = SpaceKey.IsValid() ? SpaceKey : OriginalParentKey;
		if(OriginalParentKey.IsValid())
		{
			FTLLRN_RigElementKey ParentKey;
			if(const FTLLRN_RigElementKey* ParentKeyPtr = KeyMap.Find(OriginalParentKey))
			{
				ParentKey = *ParentKeyPtr;
			}
			SetParent(ControlKey, ParentKey, false, false);
		}
	}

#if WITH_EDITOR
	if(!IsRunningCommandlet()) // don't show warnings like this if we are cooking
	{
		for(const TPair<FTLLRN_RigElementKey, FTLLRN_RigElementKey>& Pair: KeyMap)
		{
			if(Pair.Key != Pair.Value)
			{
				check(Pair.Key.Type == Pair.Value.Type);
				const FText TypeLabel = StaticEnum<ETLLRN_RigElementType>()->GetDisplayNameTextByValue((int64)Pair.Key.Type);
				ReportWarningf(TEXT("%s '%s' was renamed to '%s' during load (fixing invalid name)."), *TypeLabel.ToString(), *Pair.Key.Name.ToString(), *Pair.Value.Name.ToString());
			}
		}
	}
#endif

	Hierarchy->EnsureCacheValidity();

	TArray<FTLLRN_RigElementKey> AddedKeys;
	KeyMap.GenerateValueArray(AddedKeys);
	return AddedKeys;
}

#if WITH_EDITOR
TArray<FString> UTLLRN_RigHierarchyController::GeneratePythonCommands()
{
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	TArray<FString> Commands;
	Hierarchy->Traverse([&](FTLLRN_RigBaseElement* Element, bool& bContinue)
	{
		Commands.Append(GetAddElementPythonCommands(Element));
		
		bContinue = true;
		return;
	});

	return Commands;
}

TArray<FString> UTLLRN_RigHierarchyController::GetAddElementPythonCommands(FTLLRN_RigBaseElement* Element) const
{
	if(FTLLRN_RigBoneElement* BoneElement = Cast<FTLLRN_RigBoneElement>(Element))
	{
		return GetAddBonePythonCommands(BoneElement);		
	}
	else if(FTLLRN_RigNullElement* NullElement = Cast<FTLLRN_RigNullElement>(Element))
	{
		return GetAddNullPythonCommands(NullElement);
	}
	else if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
	{
		return GetAddControlPythonCommands(ControlElement);
	}
	else if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(Element))
	{
		return GetAddCurvePythonCommands(CurveElement);
	}
	else if(FTLLRN_RigPhysicsElement* PhysicsElement = Cast<FTLLRN_RigPhysicsElement>(Element))
	{
		return GetAddPhysicsElementPythonCommands(PhysicsElement);
	}
	else if(FTLLRN_RigReferenceElement* ReferenceElement = Cast<FTLLRN_RigReferenceElement>(Element))
	{
		ensure(false);
	}
	else if(FTLLRN_RigConnectorElement* ConnectorElement = Cast<FTLLRN_RigConnectorElement>(Element))
	{
		return GetAddConnectorPythonCommands(ConnectorElement);
	}
	else if(FTLLRN_RigSocketElement* SocketElement = Cast<FTLLRN_RigSocketElement>(Element))
	{
		return GetAddSocketPythonCommands(SocketElement);
	}
	return TArray<FString>();
}

TArray<FString> UTLLRN_RigHierarchyController::GetAddBonePythonCommands(FTLLRN_RigBoneElement* Bone) const
{
	TArray<FString> Commands;
	if (!Bone)
	{
		return Commands;
	}
	
	FString TransformStr = RigVMPythonUtils::TransformToPythonString(Bone->GetTransform().Initial.Local.Get());
	FString ParentKeyStr = "''";
	if (Bone->ParentElement)
	{
		ParentKeyStr = Bone->ParentElement->GetKey().ToPythonString();
	}
	
	// AddBone(FName InName, FTLLRN_RigElementKey InParent, FTransform InTransform, bool bTransformInGlobal = true, ETLLRN_RigBoneType InBoneType = ETLLRN_RigBoneType::User, bool bSetupUndo = false);
	Commands.Add(FString::Printf(TEXT("hierarchy_controller.add_bone('%s', %s, %s, False, %s)"),
		*Bone->GetName(),
		*ParentKeyStr,
		*TransformStr,
		*RigVMPythonUtils::EnumValueToPythonString<ETLLRN_RigBoneType>((int64)Bone->BoneType)
	));

	return Commands;
}

TArray<FString> UTLLRN_RigHierarchyController::GetAddNullPythonCommands(FTLLRN_RigNullElement* Null) const
{
	FString TransformStr = RigVMPythonUtils::TransformToPythonString(Null->GetTransform().Initial.Local.Get());

	FString ParentKeyStr = "''";
	if (Null->ParentConstraints.Num() > 0)
	{
		ParentKeyStr = Null->ParentConstraints[0].ParentElement->GetKey().ToPythonString();		
	}
		
	// AddNull(FName InName, FTLLRN_RigElementKey InParent, FTransform InTransform, bool bTransformInGlobal = true, bool bSetupUndo = false);
	return {FString::Printf(TEXT("hierarchy_controller.add_null('%s', %s, %s, False)"),
		*Null->GetName(),
		*ParentKeyStr,
		*TransformStr)};
}

TArray<FString> UTLLRN_RigHierarchyController::GetAddControlPythonCommands(FTLLRN_RigControlElement* Control) const
{
	TArray<FString> Commands;
	FString TransformStr = RigVMPythonUtils::TransformToPythonString(Control->GetTransform().Initial.Local.Get());

	FString ParentKeyStr = "''";
	if (Control->ParentConstraints.Num() > 0)
	{
		ParentKeyStr = Control->ParentConstraints[0].ParentElement->GetKey().ToPythonString();
	}

	FTLLRN_RigControlSettings& Settings = Control->Settings;
	FString SettingsStr;
	{
		FString ControlNamePythonized = RigVMPythonUtils::PythonizeName(Control->GetName());
		SettingsStr = FString::Printf(TEXT("control_settings_%s"),
			*ControlNamePythonized);
			
		Commands.Append(UTLLRN_RigHierarchy::ControlSettingsToPythonCommands(Settings, SettingsStr));	
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigControlValue Value = Hierarchy->GetControlValue(Control->GetKey(), ETLLRN_RigControlValueType::Initial);
	FString ValueStr = Value.ToPythonString(Settings.ControlType);
	
	// AddControl(FName InName, FTLLRN_RigElementKey InParent, FTLLRN_RigControlSettings InSettings, FTLLRN_RigControlValue InValue, bool bSetupUndo = true);
	Commands.Add(FString::Printf(TEXT("hierarchy_controller.add_control('%s', %s, %s, %s)"),
		*Control->GetName(),
		*ParentKeyStr,
		*SettingsStr,
		*ValueStr));

	Commands.Append(GetSetControlShapeTransformPythonCommands(Control, Control->GetShapeTransform().Initial.Local.Get(), true));
	Commands.Append(GetSetControlValuePythonCommands(Control, Settings.MinimumValue, ETLLRN_RigControlValueType::Minimum));
	Commands.Append(GetSetControlValuePythonCommands(Control, Settings.MaximumValue, ETLLRN_RigControlValueType::Maximum));
	Commands.Append(GetSetControlOffsetTransformPythonCommands(Control, Control->GetOffsetTransform().Initial.Local.Get(), true, true));
	Commands.Append(GetSetControlValuePythonCommands(Control, Value, ETLLRN_RigControlValueType::Current));

	return Commands;
}

TArray<FString> UTLLRN_RigHierarchyController::GetAddCurvePythonCommands(FTLLRN_RigCurveElement* Curve) const
{
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	// FTLLRN_RigElementKey AddCurve(FName InName, float InValue = 0.f, bool bSetupUndo = true);
	return {FString::Printf(TEXT("hierarchy_controller.add_curve('%s', %f)"),
		*Curve->GetName(),
		Hierarchy->GetCurveValue(Curve))};
}

TArray<FString> UTLLRN_RigHierarchyController::GetAddPhysicsElementPythonCommands(FTLLRN_RigPhysicsElement* PhysicsElement) const
{
	TArray<FString> Commands;
	FString TransformStr = RigVMPythonUtils::TransformToPythonString(PhysicsElement->GetTransform().Initial.Local.Get());
	
	FString ParentKeyStr = "''";
	if (PhysicsElement->ParentElement)
	{
		ParentKeyStr = PhysicsElement->ParentElement->GetKey().ToPythonString();
	}

	FString PhysicsElementNamePythonized = RigVMPythonUtils::PythonizeName(PhysicsElement->GetName());
	FTLLRN_RigPhysicsSettings& Settings = PhysicsElement->Settings;
	FString SettingsStr;
	{
		SettingsStr =FString::Printf(TEXT("physics_settings%s"),
			*PhysicsElementNamePythonized);
			
		Commands.Add(FString::Printf(TEXT("physics_settings%s = unreal.TLLRN_RigPhysicsSettings()"),
			*PhysicsElementNamePythonized));

		Commands.Add(FString::Printf(TEXT("control_settings_%s.mass = %f"),
			*PhysicsElementNamePythonized,
			Settings.Mass));	
	}
	
	// FTLLRN_RigElementKey AddPhysicsElement(FName InName, FTLLRN_RigElementKey InParent, FTLLRN_RigPhysicsSettings InSettings, FTransform InLocalTransform, bool bSetupUndo = false);
	Commands.Add(FString::Printf(TEXT("hierarchy_controller.add_physics_element('%s', %s, %s, %s)"),
		*PhysicsElement->GetName(),
		*ParentKeyStr,
		*SettingsStr,
		*TransformStr));

	return Commands;
}

TArray<FString> UTLLRN_RigHierarchyController::GetAddConnectorPythonCommands(FTLLRN_RigConnectorElement* Connector) const
{
	TArray<FString> Commands;

	FTLLRN_RigConnectorSettings& Settings = Connector->Settings;
	FString SettingsStr;
	{
		FString ConnectorNamePythonized = RigVMPythonUtils::PythonizeName(Connector->GetName());
		SettingsStr = FString::Printf(TEXT("connector_settings_%s"),
			*ConnectorNamePythonized);
			
		Commands.Append(UTLLRN_RigHierarchy::ConnectorSettingsToPythonCommands(Settings, SettingsStr));	
	}

	// AddConnector(FName InName, FTLLRN_RigConnectorSettings InSettings = FTLLRN_RigConnectorSettings(), bool bSetupUndo = false);
	Commands.Add(FString::Printf(TEXT("hierarchy_controller.add_connector('%s', %s)"),
		*Connector->GetName(),
		*SettingsStr
	));

	return Commands;
}

TArray<FString> UTLLRN_RigHierarchyController::GetAddSocketPythonCommands(FTLLRN_RigSocketElement* Socket) const
{
	TArray<FString> Commands;
	FString TransformStr = RigVMPythonUtils::TransformToPythonString(Socket->GetTransform().Initial.Local.Get());

	FString ParentKeyStr = "''";
	if (Socket->ParentElement)
	{
		ParentKeyStr = Socket->ParentElement->GetKey().ToPythonString();
	}

	const UTLLRN_RigHierarchy* CurrentHierarchy = GetHierarchy();

	// AddSocket(FName InName, FTLLRN_RigElementKey InParent, FTransform InTransform, bool bTransformInGlobal = true, FLinearColor Color, FString Description, bool bSetupUndo = false);
	Commands.Add(FString::Printf(TEXT("hierarchy_controller.add_socket('%s', %s, %s, False, %s, '%s')"),
		*Socket->GetName(),
		*ParentKeyStr,
		*TransformStr,
		*RigVMPythonUtils::LinearColorToPythonString(Socket->GetColor(CurrentHierarchy)),
		*Socket->GetDescription(CurrentHierarchy)
	));

	return Commands;
}

TArray<FString> UTLLRN_RigHierarchyController::GetSetControlValuePythonCommands(const FTLLRN_RigControlElement* Control, const FTLLRN_RigControlValue& Value,
                                                                          const ETLLRN_RigControlValueType& Type) const
{
	return {FString::Printf(TEXT("hierarchy.set_control_value(%s, %s, %s)"),
		*Control->GetKey().ToPythonString(),
		*Value.ToPythonString(Control->Settings.ControlType),
		*RigVMPythonUtils::EnumValueToPythonString<ETLLRN_RigControlValueType>((int64)Type))};
}

TArray<FString> UTLLRN_RigHierarchyController::GetSetControlOffsetTransformPythonCommands(const FTLLRN_RigControlElement* Control,
                                                                                    const FTransform& Offset, bool bInitial, bool bAffectChildren) const
{
	//SetControlOffsetTransform(FTLLRN_RigElementKey InKey, FTransform InTransform, bool bInitial = false, bool bAffectChildren = true, bool bSetupUndo = false)
	return {FString::Printf(TEXT("hierarchy.set_control_offset_transform(%s, %s, %s, %s)"),
		*Control->GetKey().ToPythonString(),
		*RigVMPythonUtils::TransformToPythonString(Offset),
		bInitial ? TEXT("True") : TEXT("False"),
		bAffectChildren ? TEXT("True") : TEXT("False"))};
}

TArray<FString> UTLLRN_RigHierarchyController::GetSetControlShapeTransformPythonCommands(const FTLLRN_RigControlElement* Control,
	const FTransform& InTransform, bool bInitial) const
{
	//SetControlShapeTransform(FTLLRN_RigElementKey InKey, FTransform InTransform, bool bInitial = false, bool bSetupUndo = false)
	return {FString::Printf(TEXT("hierarchy.set_control_shape_transform(%s, %s, %s)"),
		*Control->GetKey().ToPythonString(),
		*RigVMPythonUtils::TransformToPythonString(InTransform),
		bInitial ? TEXT("True") : TEXT("False"))};
}
#endif

void UTLLRN_RigHierarchyController::Notify(ETLLRN_RigHierarchyNotification InNotifType, const FTLLRN_RigBaseElement* InElement)
{
	if(!IsValid())
	{
		return;
	}
	if(bSuspendAllNotifications)
	{
		return;
	}
	if(bSuspendSelectionNotifications)
	{
		if(InNotifType == ETLLRN_RigHierarchyNotification::ElementSelected ||
			InNotifType == ETLLRN_RigHierarchyNotification::ElementDeselected)
		{
			return;
		}
	}	
	GetHierarchy()->Notify(InNotifType, InElement);
}

void UTLLRN_RigHierarchyController::HandleHierarchyModified(ETLLRN_RigHierarchyNotification InNotifType, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement) const
{
	if(bSuspendAllNotifications)
	{
		return;
	}
	ensure(IsValid());
	ensure(InHierarchy == GetHierarchy());
	ModifiedEvent.Broadcast(InNotifType, InHierarchy, InElement);
}

bool UTLLRN_RigHierarchyController::IsValid() const
{
	if(::IsValid(this))
	{
		return ::IsValid(GetHierarchy());
	}
	return false;
}

FName UTLLRN_RigHierarchyController::GetSafeNewName(const FName& InDesiredName, ETLLRN_RigElementType InElementType, bool bAllowNameSpace) const
{
	FTLLRN_RigName Name(InDesiredName);

	// remove potential namespaces from it
	if(!bAllowNameSpace)
	{
		int32 Index = INDEX_NONE;
		if(Name.GetName().FindLastChar(TEXT(':'), Index))
		{
			Name.SetName(Name.GetName().Mid(Index + 1));
		}
	}
	
	return GetHierarchy()->GetSafeNewName(Name, InElementType, bAllowNameSpace).GetFName();
}

int32 UTLLRN_RigHierarchyController::AddElement(FTLLRN_RigBaseElement* InElementToAdd, FTLLRN_RigBaseElement* InFirstParent, bool bMaintainGlobalTransform, const FName& InDesiredName)
{
	ensure(IsValid());

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	InElementToAdd->CachedNameString.Reset();
	InElementToAdd->SubIndex = Hierarchy->Num(InElementToAdd->Key.Type);
	InElementToAdd->Index = Hierarchy->Elements.Add(InElementToAdd);
	Hierarchy->ElementsPerType[UTLLRN_RigHierarchy::TLLRN_RigElementTypeToFlatIndex(InElementToAdd->GetKey().Type)].Add(InElementToAdd);
	Hierarchy->IndexLookup.Add(InElementToAdd->Key, InElementToAdd->Index);
	Hierarchy->AllocateDefaultElementStorage(InElementToAdd, true);
	Hierarchy->IncrementTopologyVersion();

	FTLLRN_RigName DesiredName = InDesiredName;
	UTLLRN_RigHierarchy::SanitizeName(DesiredName);
	
	FTLLRN_RigName LastSegmentName;
	if(UTLLRN_RigHierarchy::SplitNameSpace(DesiredName, nullptr, &LastSegmentName, true))
	{
		DesiredName = LastSegmentName;
	}

	if(!InDesiredName.IsNone() &&
		!InElementToAdd->GetFName().IsEqual(DesiredName.GetFName(), ENameCase::CaseSensitive))
	{
		Hierarchy->SetNameMetadata(InElementToAdd->Key, UTLLRN_RigHierarchy::DesiredNameMetadataName, DesiredName.GetFName());
		Hierarchy->SetTLLRN_RigElementKeyMetadata(InElementToAdd->Key, UTLLRN_RigHierarchy::DesiredKeyMetadataName, FTLLRN_RigElementKey(DesiredName.GetFName(), InElementToAdd->Key.Type));
	}
	
	if(Hierarchy->HasExecuteContext())
	{
		const FTLLRN_ControlRigExecuteContext& CRContext = Hierarchy->ExecuteContext->GetPublicData<FTLLRN_ControlRigExecuteContext>();

		if(!CRContext.GetTLLRN_RigModuleNameSpace().IsEmpty())
		{
			if(InElementToAdd->GetName().StartsWith(CRContext.GetTLLRN_RigModuleNameSpace(), ESearchCase::IgnoreCase))
			{
				Hierarchy->SetNameMetadata(InElementToAdd->Key, UTLLRN_RigHierarchy::NameSpaceMetadataName, *CRContext.GetTLLRN_RigModuleNameSpace());
				Hierarchy->SetNameMetadata(InElementToAdd->Key, UTLLRN_RigHierarchy::ModuleMetadataName, *CRContext.GetTLLRN_RigModuleNameSpace().LeftChop(1));

				if(Hierarchy->ElementKeyRedirector)
				{
					Hierarchy->ElementKeyRedirector->Add(FTLLRN_RigElementKey(DesiredName.GetFName(), InElementToAdd->Key.Type), InElementToAdd->Key, Hierarchy);
				}
			}

			// also try to find the live module and retrieve the shortname from that to update the element's short name
			if(const FTLLRN_RigModuleInstance* Module = CRContext.GetTLLRN_RigModuleInstance())
			{
				const FString ModuleShortName = Module->GetShortName();
				if(!ModuleShortName.IsEmpty())
				{
					Hierarchy->SetNameMetadata(InElementToAdd->Key, UTLLRN_RigHierarchy::ShortModuleNameMetadataName, *ModuleShortName);
				}
			}
		}
	}
	
	{
		const TGuardValue<bool> Guard(bSuspendAllNotifications, true);
		SetParent(InElementToAdd, InFirstParent, bMaintainGlobalTransform);
	}

	if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InElementToAdd))
	{
		Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
		Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::InitialLocal);
		ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);
		ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
	}

	// only notify once at the end
	Notify(ETLLRN_RigHierarchyNotification::ElementAdded, InElementToAdd);

	return InElementToAdd->Index;
}

bool UTLLRN_RigHierarchyController::RemoveElement(FTLLRN_RigElementKey InElement, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* Element = Hierarchy->Find(InElement);
	if(Element == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Element: '%s' not found."), *InElement.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Remove Element", "Remove Element"));
		Hierarchy->Modify();
	}
#endif

	const bool bRemoved = RemoveElement(Element);
	
#if WITH_EDITOR
	if(!bRemoved && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bRemoved && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.remove_element(%s)"),
				*InElement.ToPythonString()));
		}
	}
#endif

	return bRemoved;
}

bool UTLLRN_RigHierarchyController::RemoveElement(FTLLRN_RigBaseElement* InElement)
{
	if(InElement == nullptr)
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	// make sure this element is part of this hierarchy
	ensure(Hierarchy->FindChecked(InElement->Key) == InElement);
	ensure(InElement->OwnedInstances == 1);

	// deselect if needed
	if(InElement->IsSelected())
	{
		SelectElement(InElement->GetKey(), false);
	}

	// if this is a transform element - make sure to allow dependents to store their global transforms
	if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(InElement))
	{
		FTLLRN_RigTransformElement::FElementsToDirtyArray PreviousElementsToDirty = TransformElement->ElementsToDirty; 
		for(const FTLLRN_RigTransformElement::FElementToDirty& ElementToDirty : PreviousElementsToDirty)
		{
			if(FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(ElementToDirty.Element))
			{
				if(SingleParentElement->ParentElement == InElement)
				{
					RemoveParent(SingleParentElement, InElement, true);
				}
			}
			else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(ElementToDirty.Element))
			{
				for(const FTLLRN_RigElementParentConstraint& ParentConstraint : MultiParentElement->ParentConstraints)
				{
					if(ParentConstraint.ParentElement == InElement)
					{
						RemoveParent(MultiParentElement, InElement, true);
						break;
					}
				}
			}
		}
	}

	const int32 NumElementsRemoved = Hierarchy->Elements.Remove(InElement);
	ensure(NumElementsRemoved == 1);

	const int32 NumTypeElementsRemoved = Hierarchy->ElementsPerType[UTLLRN_RigHierarchy::TLLRN_RigElementTypeToFlatIndex(InElement->GetKey().Type)].Remove(InElement);
	ensure(NumTypeElementsRemoved == 1);

	const int32 NumLookupsRemoved = Hierarchy->IndexLookup.Remove(InElement->Key);
	ensure(NumLookupsRemoved == 1);
	for(TPair<FTLLRN_RigElementKey, int32>& Pair : Hierarchy->IndexLookup)
	{
		if(Pair.Value > InElement->Index)
		{
			Pair.Value--;
		}
	}

	// update the indices of all other elements
	for (FTLLRN_RigBaseElement* RemainingElement : Hierarchy->Elements)
	{
		if(RemainingElement->Index > InElement->Index)
		{
			RemainingElement->Index--;
		}
	}

	if(FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(InElement))
	{
		RemoveElementToDirty(SingleParentElement->ParentElement, InElement);
	}
	else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InElement))
	{
		for(const FTLLRN_RigElementParentConstraint& ParentConstraint : MultiParentElement->ParentConstraints)
		{
			RemoveElementToDirty(ParentConstraint.ParentElement, InElement);
		}
	}

	if(InElement->SubIndex != INDEX_NONE)
	{
		for(FTLLRN_RigBaseElement* Element : Hierarchy->Elements)
		{
			if(Element->SubIndex > InElement->SubIndex && Element->GetType() == InElement->GetType())
			{
				Element->SubIndex--;
			}
		}
	}

	for(FTLLRN_RigBaseElement* Element : Hierarchy->Elements)
	{
		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
		{
			ControlElement->Settings.Customization.AvailableSpaces.Remove(InElement->GetKey());
			ControlElement->Settings.Customization.RemovedSpaces.Remove(InElement->GetKey());
			ControlElement->Settings.DrivenControls.Remove(InElement->GetKey());
		}
	}

	Hierarchy->DeallocateElementStorage(InElement);
	Hierarchy->IncrementTopologyVersion();

	Notify(ETLLRN_RigHierarchyNotification::ElementRemoved, InElement);
	if(Hierarchy->Num() == 0)
	{
		Notify(ETLLRN_RigHierarchyNotification::HierarchyReset, nullptr);
	}

	if (InElement->OwnedInstances == 1)
	{
		Hierarchy->DestroyElement(InElement);
	}

	Hierarchy->EnsureCacheValidity();

	return NumElementsRemoved == 1;
}

FTLLRN_RigElementKey UTLLRN_RigHierarchyController::RenameElement(FTLLRN_RigElementKey InElement, FName InName, bool bSetupUndo, bool bPrintPythonCommand, bool bClearSelection)
{
	if(!IsValid())
	{
		return FTLLRN_RigElementKey();
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* Element = Hierarchy->Find(InElement);
	if(Element == nullptr)
	{
		ReportWarningf(TEXT("Cannot Rename Element: '%s' not found."), *InElement.ToString());
		return FTLLRN_RigElementKey();
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Rename Element", "Rename Element"));
		Hierarchy->Modify();
	}
#endif

	const bool bRenamed = RenameElement(Element, InName, bClearSelection);

#if WITH_EDITOR
	if(!bRenamed && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bRenamed && bClearSelection)
	{
		ClearSelection();
	}

	if (bRenamed && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(), 
				FString::Printf(TEXT("hierarchy_controller.rename_element(%s, '%s')"),
				*InElement.ToPythonString(),
				*InName.ToString()));
		}
	}
#endif

	return bRenamed ? Element->GetKey() : FTLLRN_RigElementKey();
}

bool UTLLRN_RigHierarchyController::ReorderElement(FTLLRN_RigElementKey InElement, int32 InIndex, bool bSetupUndo,
	bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* Element = Hierarchy->Find(InElement);
	if(Element == nullptr)
	{
		ReportWarningf(TEXT("Cannot Reorder Element: '%s' not found."), *InElement.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Reorder Element", "Reorder Element"));
		Hierarchy->Modify();
	}
#endif

	const bool bReordered = ReorderElement(Element, InIndex);

#if WITH_EDITOR
	if(!bReordered && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bReordered && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(), 
				FString::Printf(TEXT("hierarchy_controller.reorder_element(%s, %d)"),
				*InElement.ToPythonString(),
				InIndex));
		}
	}
#endif

	return bReordered;
}

FName UTLLRN_RigHierarchyController::SetDisplayName(FTLLRN_RigElementKey InControl, FName InDisplayName, bool bRenameElement, bool bSetupUndo,
                                              bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return NAME_None;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(InControl);
	if(ControlElement == nullptr)
	{
		ReportWarningf(TEXT("Cannot Rename Control: '%s' not found."), *InControl.ToString());
		return NAME_None;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Set Display Name on Control", "Set Display Name on Control"));
		Hierarchy->Modify();
	}
#endif

	const FName NewDisplayName = SetDisplayName(ControlElement, InDisplayName, bRenameElement);
	const bool bDisplayNameChanged = !NewDisplayName.IsNone();

#if WITH_EDITOR
	if(!bDisplayNameChanged && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bDisplayNameChanged && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(), 
				FString::Printf(TEXT("hierarchy_controller.set_display_name(%s, '%s', %s)"),
				*InControl.ToPythonString(),
				*InDisplayName.ToString(),
				(bRenameElement) ? TEXT("True") : TEXT("False")));
		}
	}
#endif

	return NewDisplayName;
}

bool UTLLRN_RigHierarchyController::RenameElement(FTLLRN_RigBaseElement* InElement, const FName &InName, bool bClearSelection)
{
	if(InElement == nullptr)
	{
		return false;
	}

	if (InElement->GetFName().IsEqual(InName, ENameCase::CaseSensitive))
	{
		return false;
	}

	const FTLLRN_RigElementKey OldKey = InElement->GetKey();

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	// deselect the key that no longer exists
	// no need to trigger a reselect since we always clear selection after rename
	const bool bWasSelected = Hierarchy->IsSelected(InElement); 
	if (Hierarchy->IsSelected(InElement))
	{
		DeselectElement(OldKey);
	}

	{
		// create a temp copy of the map and remove the current item's key
		TMap<FTLLRN_RigElementKey, int32> TemporaryMap = Hierarchy->IndexLookup;
		TemporaryMap.Remove(OldKey);
   
		TGuardValue<TMap<FTLLRN_RigElementKey, int32>> MapGuard(Hierarchy->IndexLookup, TemporaryMap);
		InElement->Key.Name = GetSafeNewName(InName, InElement->GetType());
		InElement->CachedNameString.Reset();
	}
	
	const FTLLRN_RigElementKey NewKey = InElement->GetKey();

	Hierarchy->IndexLookup.Remove(OldKey);
	Hierarchy->IndexLookup.Add(NewKey, InElement->Index);

	// update all multi parent elements' index lookups
	for (FTLLRN_RigBaseElement* Element : Hierarchy->Elements)
	{
		if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(Element))
		{
			const int32* ExistingIndexPtr = MultiParentElement->IndexLookup.Find(OldKey);
			if(ExistingIndexPtr)
			{
				const int32 ExistingIndex = *ExistingIndexPtr;
				MultiParentElement->IndexLookup.Remove(OldKey);
				MultiParentElement->IndexLookup.Add(NewKey, ExistingIndex);
			}
		}

		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
		{
			for(FTLLRN_RigElementKey& Favorite : ControlElement->Settings.Customization.AvailableSpaces)
			{
				if(Favorite == OldKey)
				{
					Favorite.Name = NewKey.Name;
				}
			}

			for(FTLLRN_RigElementKey& DrivenControl : ControlElement->Settings.DrivenControls)
			{
				if(DrivenControl == OldKey)
				{
					DrivenControl.Name = NewKey.Name;
				}
			}
		}
	}

	Hierarchy->PreviousNameMap.FindOrAdd(NewKey) = OldKey;
	Hierarchy->IncrementTopologyVersion();
	Notify(ETLLRN_RigHierarchyNotification::ElementRenamed, InElement);

	if (!bClearSelection && bWasSelected)
	{
		SelectElement(InElement->GetKey(), true);
	}

	return true;
}

bool UTLLRN_RigHierarchyController::ReorderElement(FTLLRN_RigBaseElement* InElement, int32 InIndex)
{
	if(InElement == nullptr)
	{
		return false;
	}

	InIndex = FMath::Max<int32>(InIndex, 0);

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	TArray<FTLLRN_RigBaseElement*> LocalElements;
	if(const FTLLRN_RigBaseElement* ParentElement = Hierarchy->GetFirstParent(InElement))
	{
		LocalElements.Append(Hierarchy->GetChildren(ParentElement));
	}
	else
	{
		const TArray<FTLLRN_RigBaseElement*> RootElements = Hierarchy->GetRootElements();
		LocalElements.Append(RootElements);
	}

	const int32 CurrentIndex = LocalElements.Find(InElement);
	if(CurrentIndex == INDEX_NONE || CurrentIndex == InIndex)
	{
		return false;
	}

	Hierarchy->IncrementTopologyVersion();

	TArray<int32> GlobalIndices;
	GlobalIndices.Reserve(LocalElements.Num());
	for(const FTLLRN_RigBaseElement* Element : LocalElements)
	{
		GlobalIndices.Add(Element->GetIndex());
	}

	LocalElements.RemoveAt(CurrentIndex);
	if(InIndex >= LocalElements.Num())
	{
		LocalElements.Add(InElement);
	}
	else
	{
		LocalElements.Insert(InElement, InIndex);
	}

	InIndex = FMath::Min<int32>(InIndex, LocalElements.Num() - 1);
	const int32 LowerBound = FMath::Min<int32>(InIndex, CurrentIndex);
	const int32 UpperBound = FMath::Max<int32>(InIndex, CurrentIndex);
	for(int32 LocalIndex = LowerBound; LocalIndex <= UpperBound; LocalIndex++)
	{
		const int32 GlobalIndex = GlobalIndices[LocalIndex];
		FTLLRN_RigBaseElement* Element = LocalElements[LocalIndex];
		Hierarchy->Elements[GlobalIndex] = Element;
		Element->Index = GlobalIndex;
		Hierarchy->IndexLookup.FindOrAdd(Element->Key) = GlobalIndex;
	}

	Notify(ETLLRN_RigHierarchyNotification::ElementReordered, InElement);

	return true;
}

FName UTLLRN_RigHierarchyController::SetDisplayName(FTLLRN_RigControlElement* InControlElement, const FName& InDisplayName, bool bRenameElement)
{
	if(InControlElement == nullptr)
	{
		return NAME_None;
	}

	if (InControlElement->Settings.DisplayName.IsEqual(InDisplayName, ENameCase::CaseSensitive))
	{
		return NAME_None;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigElementKey ParentElementKey;
	if(const FTLLRN_RigBaseElement* ParentElement = Hierarchy->GetFirstParent(InControlElement))
	{
		ParentElementKey = ParentElement->GetKey();
	}

	const FName DisplayName = Hierarchy->GetSafeNewDisplayName(ParentElementKey, InDisplayName);
	InControlElement->Settings.DisplayName = DisplayName;

	Hierarchy->IncrementTopologyVersion();
	Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, InControlElement);

	if(bRenameElement)
	{
		RenameElement(InControlElement, InControlElement->Settings.DisplayName, false);
	}
#if WITH_EDITOR
	else
	{
		// if we are merely setting the display name - we want to update all listening hierarchies
		for(UTLLRN_RigHierarchy::FTLLRN_RigHierarchyListener& Listener : Hierarchy->ListeningHierarchies)
		{
			if(UTLLRN_RigHierarchy* ListeningHierarchy = Listener.Hierarchy.Get())
			{
				if(UTLLRN_RigHierarchyController* ListeningController = ListeningHierarchy->GetController())
				{
					const TGuardValue<bool> Guard(ListeningController->bSuspendAllNotifications, true);
					ListeningController->SetDisplayName(InControlElement->GetKey(), InDisplayName, bRenameElement, false, false);
				}
			}
		}
	}
#endif
	return InControlElement->Settings.DisplayName;
}

bool UTLLRN_RigHierarchyController::AddParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, float InWeight, bool bMaintainGlobalTransform, bool bSetupUndo)
{
	if(!IsValid())
	{
		return false;
	}

	if(InParent.Type == ETLLRN_RigElementType::Socket)
	{
		ReportWarningf(TEXT("Cannot parent Child '%s' under a Socket parent."), *InChild.ToString());
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* Child = Hierarchy->Find(InChild);
	if(Child == nullptr)
	{
		ReportWarningf(TEXT("Cannot Add Parent, Child '%s' not found."), *InChild.ToString());
		return false;
	}

	FTLLRN_RigBaseElement* Parent = Hierarchy->Find(InParent);
	if(Parent == nullptr)
	{
		ReportWarningf(TEXT("Cannot Add Parent, Parent '%s' not found."), *InParent.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Add Parent", "Add Parent"));
		Hierarchy->Modify();
	}
#endif

	const bool bAdded = AddParent(Child, Parent, InWeight, bMaintainGlobalTransform);

#if WITH_EDITOR
	if(!bAdded && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();
#endif
	
	return bAdded;
}

bool UTLLRN_RigHierarchyController::AddParent(FTLLRN_RigBaseElement* InChild, FTLLRN_RigBaseElement* InParent, float InWeight, bool bMaintainGlobalTransform, bool bRemoveAllParents)
{
	if(InChild == nullptr || InParent == nullptr)
	{
		return false;
	}

	// single parent children can't be parented multiple times
	if(FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(InChild))
	{
		if(SingleParentElement->ParentElement == InParent)
		{
			return false;
		}
		bRemoveAllParents = true;
	}

	else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		for(const FTLLRN_RigElementParentConstraint& ParentConstraint : MultiParentElement->ParentConstraints)
		{
			if(ParentConstraint.ParentElement == InParent)
			{
				return false;
			}
		}
	}

	// we can only parent things to controls which are not animation channels (animation channels are not 3D things)
	if(FTLLRN_RigControlElement* ParentControlElement = Cast<FTLLRN_RigControlElement>(InParent))
	{
		if(ParentControlElement->IsAnimationChannel())
		{
			return false;
		}
	}

	// we can only reparent animation channels - we cannot add a parent to them
	if(FTLLRN_RigControlElement* ChildControlElement = Cast<FTLLRN_RigControlElement>(InChild))
	{
		if(ChildControlElement->IsAnimationChannel())
		{
			bMaintainGlobalTransform = false;
			InWeight = 0.f;
		}

		if(ChildControlElement->Settings.bRestrictSpaceSwitching)
		{
			if(ChildControlElement->Settings.Customization.AvailableSpaces.Contains(InParent->GetKey()))
			{
				return false;
			}
		}
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	if(Hierarchy->IsParentedTo(InParent, InChild))
	{
		ReportErrorf(TEXT("Cannot parent '%s' to '%s' - would cause a cycle."), *InChild->Key.ToString(), *InParent->Key.ToString());
		return false;
	}

	Hierarchy->EnsureCacheValidity();

	if(bRemoveAllParents)
	{
		RemoveAllParents(InChild, bMaintainGlobalTransform);		
	}

	if(InWeight > SMALL_NUMBER || bRemoveAllParents)
	{
		if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(InChild))
		{
			if(bMaintainGlobalTransform)
			{
				Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::CurrentGlobal);
				Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::InitialGlobal);
				TransformElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentLocal);
				TransformElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialLocal);
			}
			else
			{
				Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::CurrentLocal);
				Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::InitialLocal);
				TransformElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);
				TransformElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
			}
		}

		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InChild))
		{
			Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
			Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::InitialLocal);
		}
	}

	FTLLRN_RigElementParentConstraint Constraint;
	Constraint.ParentElement = Cast<FTLLRN_RigTransformElement>(InParent);
	if(Constraint.ParentElement == nullptr)
	{
		return false;
	}
	Constraint.InitialWeight = InWeight;
	Constraint.Weight = InWeight;

	if(FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(InChild))
	{
		AddElementToDirty(Constraint.ParentElement, SingleParentElement);
		SingleParentElement->ParentElement = Constraint.ParentElement;

		if(Cast<FTLLRN_RigSocketElement>(SingleParentElement))
		{
			Hierarchy->SetTLLRN_RigElementKeyMetadata(SingleParentElement->GetKey(), FTLLRN_RigSocketElement::DesiredParentMetaName, Constraint.ParentElement->GetKey());
			Hierarchy->Notify(ETLLRN_RigHierarchyNotification::SocketDesiredParentChanged, SingleParentElement);
		}

		Hierarchy->IncrementTopologyVersion();

		if(!bMaintainGlobalTransform)
		{
			Hierarchy->PropagateDirtyFlags(SingleParentElement, true, true);
			Hierarchy->PropagateDirtyFlags(SingleParentElement, false, true);
		}

		Notify(ETLLRN_RigHierarchyNotification::ParentChanged, SingleParentElement);
		
		Hierarchy->EnsureCacheValidity();
		
		return true;
	}
	else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InChild))
		{
			if(!ControlElement->Settings.DisplayName.IsNone())
			{
				ControlElement->Settings.DisplayName =
					Hierarchy->GetSafeNewDisplayName(
						InParent->GetKey(),
						ControlElement->Settings.DisplayName);
			}
		}
		
		AddElementToDirty(Constraint.ParentElement, MultiParentElement);

		const int32 ParentIndex = MultiParentElement->ParentConstraints.Add(Constraint);
		MultiParentElement->IndexLookup.Add(Constraint.ParentElement->GetKey(), ParentIndex);

		if(InWeight > SMALL_NUMBER)
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(MultiParentElement))
			{
				ControlElement->GetOffsetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);  
				ControlElement->GetOffsetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
				ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);  
				ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
			}
		}

		Hierarchy->IncrementTopologyVersion();

		if(InWeight > SMALL_NUMBER)
		{
			if(!bMaintainGlobalTransform)
			{
				Hierarchy->PropagateDirtyFlags(MultiParentElement, true, true);
				Hierarchy->PropagateDirtyFlags(MultiParentElement, false, true);
			}
		}

		if(FTLLRN_RigControlElement* ChildControlElement = Cast<FTLLRN_RigControlElement>(InChild))
		{
			const FTransform LocalTransform = Hierarchy->GetTransform(ChildControlElement, ETLLRN_RigTransformType::InitialLocal);
			Hierarchy->SetControlPreferredEulerAngles(ChildControlElement, LocalTransform, true);
			ChildControlElement->PreferredEulerAngles.Current = ChildControlElement->PreferredEulerAngles.Initial;
		}

		Notify(ETLLRN_RigHierarchyNotification::ParentChanged, MultiParentElement);

		Hierarchy->EnsureCacheValidity();
		
		return true;
	}
	
	return false;
}

bool UTLLRN_RigHierarchyController::RemoveParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, bool bMaintainGlobalTransform, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* Child = Hierarchy->Find(InChild);
	if(Child == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Parent, Child '%s' not found."), *InChild.ToString());
		return false;
	}

	FTLLRN_RigBaseElement* Parent = Hierarchy->Find(InParent);
	if(Parent == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Parent, Parent '%s' not found."), *InParent.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Remove Parent", "Remove Parent"));
		Hierarchy->Modify();
	}
#endif

	const bool bRemoved = RemoveParent(Child, Parent, bMaintainGlobalTransform);

#if WITH_EDITOR
	if(!bRemoved && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bRemoved && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.remove_parent(%s, %s, %s)"),
				*InChild.ToPythonString(),
				*InParent.ToPythonString(),
				(bMaintainGlobalTransform) ? TEXT("True") : TEXT("False")));
		}
	}
#endif
	
	return bRemoved;
}

bool UTLLRN_RigHierarchyController::RemoveParent(FTLLRN_RigBaseElement* InChild, FTLLRN_RigBaseElement* InParent, bool bMaintainGlobalTransform)
{
	if(InChild == nullptr || InParent == nullptr)
	{
		return false;
	}

	FTLLRN_RigTransformElement* ParentTransformElement = Cast<FTLLRN_RigTransformElement>(InParent);
	if(ParentTransformElement == nullptr)
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	// single parent children can't be parented multiple times
	if(FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(InChild))
	{
		if(SingleParentElement->ParentElement == ParentTransformElement)
		{
			if(bMaintainGlobalTransform)
			{
				Hierarchy->GetTransform(SingleParentElement, ETLLRN_RigTransformType::CurrentGlobal);
				Hierarchy->GetTransform(SingleParentElement, ETLLRN_RigTransformType::InitialGlobal);
				SingleParentElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentLocal);
				SingleParentElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialLocal);
			}
			else
			{
				Hierarchy->GetTransform(SingleParentElement, ETLLRN_RigTransformType::CurrentLocal);
				Hierarchy->GetTransform(SingleParentElement, ETLLRN_RigTransformType::InitialLocal);
				SingleParentElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);
				SingleParentElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
			}

			const FTLLRN_RigElementKey PreviousParentKey = SingleParentElement->ParentElement->GetKey();
			Hierarchy->PreviousParentMap.FindOrAdd(SingleParentElement->GetKey()) = PreviousParentKey;
			
			// remove the previous parent
			SingleParentElement->ParentElement = nullptr;

			if(Cast<FTLLRN_RigSocketElement>(SingleParentElement))
			{
				Hierarchy->RemoveMetadata(SingleParentElement->GetKey(), FTLLRN_RigSocketElement::DesiredParentMetaName);
				Hierarchy->Notify(ETLLRN_RigHierarchyNotification::SocketDesiredParentChanged, SingleParentElement);
			}

			RemoveElementToDirty(InParent, SingleParentElement); 
			Hierarchy->IncrementTopologyVersion();

			if(!bMaintainGlobalTransform)
			{
				Hierarchy->PropagateDirtyFlags(SingleParentElement, true, true);
				Hierarchy->PropagateDirtyFlags(SingleParentElement, false, true);
			}

			Notify(ETLLRN_RigHierarchyNotification::ParentChanged, SingleParentElement);

			Hierarchy->EnsureCacheValidity();
			
			return true;
		}
	}

	// single parent children can't be parented multiple times
	else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		int32 ParentIndex = INDEX_NONE;
		for(int32 ConstraintIndex = 0; ConstraintIndex < MultiParentElement->ParentConstraints.Num(); ConstraintIndex++)
		{
			if(MultiParentElement->ParentConstraints[ConstraintIndex].ParentElement == ParentTransformElement)
			{
				ParentIndex = ConstraintIndex;
				break;
			}
		}
				
		if(MultiParentElement->ParentConstraints.IsValidIndex(ParentIndex))
		{
			if(bMaintainGlobalTransform)
			{
				Hierarchy->GetTransform(MultiParentElement, ETLLRN_RigTransformType::CurrentGlobal);
				Hierarchy->GetTransform(MultiParentElement, ETLLRN_RigTransformType::InitialGlobal);
				MultiParentElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentLocal);
				MultiParentElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialLocal);
			}
			else
			{
				Hierarchy->GetTransform(MultiParentElement, ETLLRN_RigTransformType::CurrentLocal);
				Hierarchy->GetTransform(MultiParentElement, ETLLRN_RigTransformType::InitialLocal);
				MultiParentElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);
				MultiParentElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
			}

			// remove the previous parent
			RemoveElementToDirty(InParent, MultiParentElement); 

			const FTLLRN_RigElementKey PreviousParentKey = MultiParentElement->ParentConstraints[ParentIndex].ParentElement->GetKey();
			Hierarchy->PreviousParentMap.FindOrAdd(MultiParentElement->GetKey()) = PreviousParentKey;

			MultiParentElement->ParentConstraints.RemoveAt(ParentIndex);
			MultiParentElement->IndexLookup.Remove(ParentTransformElement->GetKey());
			for(TPair<FTLLRN_RigElementKey, int32>& Pair : MultiParentElement->IndexLookup)
			{
				if(Pair.Value > ParentIndex)
				{
					Pair.Value--;
				}
			}

			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(MultiParentElement))
			{
				ControlElement->GetOffsetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);  
				ControlElement->GetOffsetDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
				ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);  
				ControlElement->GetShapeDirtyState().MarkDirty(ETLLRN_RigTransformType::InitialGlobal);
			}

			Hierarchy->IncrementTopologyVersion();

			if(!bMaintainGlobalTransform)
			{
				Hierarchy->PropagateDirtyFlags(MultiParentElement, true, true);
				Hierarchy->PropagateDirtyFlags(MultiParentElement, false, true);
			}

			Notify(ETLLRN_RigHierarchyNotification::ParentChanged, MultiParentElement);

			Hierarchy->EnsureCacheValidity();
			
			return true;
		}
	}

	return false;
}

bool UTLLRN_RigHierarchyController::RemoveAllParents(FTLLRN_RigElementKey InChild, bool bMaintainGlobalTransform, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* Child = Hierarchy->Find(InChild);
	if(Child == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove All Parents, Child '%s' not found."), *InChild.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Remove Parent", "Remove Parent"));
		Hierarchy->Modify();
	}
#endif

	const bool bRemoved = RemoveAllParents(Child, bMaintainGlobalTransform);

#if WITH_EDITOR
	if(!bRemoved && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();
	
	if (bRemoved && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.remove_all_parents(%s, %s)"),
				*InChild.ToPythonString(),
				(bMaintainGlobalTransform) ? TEXT("True") : TEXT("False")));
		}
	}
#endif

	return bRemoved;
}

bool UTLLRN_RigHierarchyController::RemoveAllParents(FTLLRN_RigBaseElement* InChild, bool bMaintainGlobalTransform)
{
	if(FTLLRN_RigSingleParentElement* SingleParentElement = Cast<FTLLRN_RigSingleParentElement>(InChild))
	{
		return RemoveParent(SingleParentElement, SingleParentElement->ParentElement, bMaintainGlobalTransform);
	}
	else if(FTLLRN_RigMultiParentElement* MultiParentElement = Cast<FTLLRN_RigMultiParentElement>(InChild))
	{
		bool bSuccess = true;

		FTLLRN_RigElementParentConstraintArray ParentConstraints = MultiParentElement->ParentConstraints;
		for(const FTLLRN_RigElementParentConstraint& ParentConstraint : ParentConstraints)
		{
			if(!RemoveParent(MultiParentElement, ParentConstraint.ParentElement, bMaintainGlobalTransform))
			{
				bSuccess = false;
			}
		}

		return bSuccess;
	}
	return false;
}

bool UTLLRN_RigHierarchyController::SetParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, bool bMaintainGlobalTransform, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* Child = Hierarchy->Find(InChild);
	if(Child == nullptr)
	{
		ReportWarningf(TEXT("Cannot Set Parent, Child '%s' not found."), *InChild.ToString());
		return false;
	}

	FTLLRN_RigBaseElement* Parent = Hierarchy->Find(InParent);
	if(Parent == nullptr)
	{
		if(InChild.Type == ETLLRN_RigElementType::Socket)
		{
			Hierarchy->SetTLLRN_RigElementKeyMetadata(InChild, FTLLRN_RigSocketElement::DesiredParentMetaName, InParent);
			Hierarchy->Notify(ETLLRN_RigHierarchyNotification::SocketDesiredParentChanged, Child);
			return true;
		}
		ReportWarningf(TEXT("Cannot Set Parent, Parent '%s' not found."), *InParent.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "Set Parent", "Set Parent"));
		Hierarchy->Modify();
	}
#endif

	const bool bParentSet = SetParent(Child, Parent, bMaintainGlobalTransform);

#if WITH_EDITOR
	if(!bParentSet && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bParentSet && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.set_parent(%s, %s, %s)"),
				*InChild.ToPythonString(),
				*InParent.ToPythonString(),
				(bMaintainGlobalTransform) ? TEXT("True") : TEXT("False")));
		}
	}
#endif

	return bParentSet;
}

bool UTLLRN_RigHierarchyController::AddAvailableSpace(FTLLRN_RigElementKey InControl, FTLLRN_RigElementKey InSpace, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* ControlBase = Hierarchy->Find(InControl);
	if(ControlBase == nullptr)
	{
		ReportWarningf(TEXT("Cannot Add Available Space, Control '%s' not found."), *InControl.ToString());
		return false;
	}

	FTLLRN_RigControlElement* Control = Cast<FTLLRN_RigControlElement>(ControlBase);
	if(Control == nullptr)
	{
		ReportWarningf(TEXT("Cannot Add Available Space, '%s' is not a Control."), *InControl.ToString());
		return false;
	}

	const FTLLRN_RigBaseElement* SpaceBase = Hierarchy->Find(InSpace);
	if(SpaceBase == nullptr)
	{
		ReportWarningf(TEXT("Cannot Add Available Space, Space '%s' not found."), *InSpace.ToString());
		return false;
	}

	const FTLLRN_RigTransformElement* Space = Cast<FTLLRN_RigTransformElement>(SpaceBase);
	if(Space == nullptr)
	{
		ReportWarningf(TEXT("Cannot Add Available Space, '%s' is not a Transform."), *InSpace.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "AddAvailableSpace", "Add Available Space"));
		Hierarchy->Modify();
	}
#endif

	const bool bSuccess = AddAvailableSpace(Control, Space);

#if WITH_EDITOR
	if(!bSuccess && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bSuccess && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		if (const UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.add_available_space(%s, %s)"),
				*InControl.ToPythonString(),
				*InSpace.ToPythonString()));
		}
	}
#endif

	return bSuccess;
}

bool UTLLRN_RigHierarchyController::RemoveAvailableSpace(FTLLRN_RigElementKey InControl, FTLLRN_RigElementKey InSpace, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* ControlBase = Hierarchy->Find(InControl);
	if(ControlBase == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Available Space, Control '%s' not found."), *InControl.ToString());
		return false;
	}

	FTLLRN_RigControlElement* Control = Cast<FTLLRN_RigControlElement>(ControlBase);
	if(Control == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Available Space, '%s' is not a Control."), *InControl.ToString());
		return false;
	}

	const FTLLRN_RigBaseElement* SpaceBase = Hierarchy->Find(InSpace);
	if(SpaceBase == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Available Space, Space '%s' not found."), *InSpace.ToString());
		return false;
	}

	const FTLLRN_RigTransformElement* Space = Cast<FTLLRN_RigTransformElement>(SpaceBase);
	if(Space == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Available Space, '%s' is not a Transform."), *InSpace.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "RemoveAvailableSpace", "Remove Available Space"));
		Hierarchy->Modify();
	}
#endif

	const bool bSuccess = RemoveAvailableSpace(Control, Space);

#if WITH_EDITOR
	if(!bSuccess && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bSuccess && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		if (const UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.remove_available_space(%s, %s)"),
				*InControl.ToPythonString(),
				*InSpace.ToPythonString()));
		}
	}
#endif

	return bSuccess;
}

bool UTLLRN_RigHierarchyController::SetAvailableSpaceIndex(FTLLRN_RigElementKey InControl, FTLLRN_RigElementKey InSpace, int32 InIndex, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* ControlBase = Hierarchy->Find(InControl);
	if(ControlBase == nullptr)
	{
		ReportWarningf(TEXT("Cannot Set Available Space Index, Control '%s' not found."), *InControl.ToString());
		return false;
	}

	FTLLRN_RigControlElement* Control = Cast<FTLLRN_RigControlElement>(ControlBase);
	if(Control == nullptr)
	{
		ReportWarningf(TEXT("Cannot Set Available Space Index, '%s' is not a Control."), *InControl.ToString());
		return false;
	}

	const FTLLRN_RigBaseElement* SpaceBase = Hierarchy->Find(InSpace);
	if(SpaceBase == nullptr)
	{
		ReportWarningf(TEXT("Cannot Set Available Space Index, Space '%s' not found."), *InSpace.ToString());
		return false;
	}

	const FTLLRN_RigTransformElement* Space = Cast<FTLLRN_RigTransformElement>(SpaceBase);
	if(Space == nullptr)
	{
		ReportWarningf(TEXT("Cannot Set Available Space Index, '%s' is not a Transform."), *InSpace.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "RemoveAvailableSpace", "Remove Available Space"));
		Hierarchy->Modify();
	}
#endif

	const bool bSuccess = SetAvailableSpaceIndex(Control, Space, InIndex);

#if WITH_EDITOR
	if(!bSuccess && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bSuccess && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		if (const UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.set_available_space_index(%s, %s)"),
				*InControl.ToPythonString(),
				*InSpace.ToPythonString()));
		}
	}
#endif

	return bSuccess;
}

bool UTLLRN_RigHierarchyController::AddChannelHost(FTLLRN_RigElementKey InChannel, FTLLRN_RigElementKey InHost, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* ChannelBase = Hierarchy->Find(InChannel);
	if(ChannelBase == nullptr)
	{
		ReportWarningf(TEXT("Cannot Add Channel Host, Channel '%s' not found."), *InChannel.ToString());
		return false;
	}

	FTLLRN_RigControlElement* Channel = Cast<FTLLRN_RigControlElement>(ChannelBase);
	if(Channel == nullptr)
	{
		ReportWarningf(TEXT("Cannot Add Channel Host, '%s' is not a Control."), *InChannel.ToString());
		return false;
	}

	if(!Channel->IsAnimationChannel())
	{
		ReportWarningf(TEXT("Cannot Add Channel Host, '%s' is not an animation channel."), *InChannel.ToString());
		return false;
	}

	const FTLLRN_RigBaseElement* HostBase = Hierarchy->Find(InHost);
	if(HostBase == nullptr)
	{
		ReportWarningf(TEXT("Cannot Add Channel Host, Host '%s' not found."), *InHost.ToString());
		return false;
	}

	const FTLLRN_RigControlElement* Host = Cast<FTLLRN_RigControlElement>(HostBase);
	if(Host == nullptr)
	{
		ReportWarningf(TEXT("Cannot Add Channel Host, '%s' is not a Control."), *InHost.ToString());
		return false;
	}

	if(Host->IsAnimationChannel())
	{
		ReportWarningf(TEXT("Cannot Add Channel Host, '%s' is also an animation channel."), *InHost.ToString());
		return false;
	}

	// the default parent cannot be added to the channel host list
	if(Hierarchy->GetParents(InChannel).Contains(InHost))
	{
		ReportWarningf(TEXT("Cannot Add Channel Host, '%s' is the parent of channel '%s'."), *InHost.ToString(), *InChannel.ToString());
		return false;
	}

	if(Channel->Settings.Customization.AvailableSpaces.Contains(Host->GetKey()))
	{
		ReportWarningf(TEXT("Cannot Add Channel Host, '%s' is already a host for channel '%s'."), *InHost.ToString(), *InChannel.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "AddChannelHost", "Add Channel Host"));
		Hierarchy->Modify();
	}
#endif

	const bool bSuccess = AddAvailableSpace(Channel, Host);

#if WITH_EDITOR
	if(!bSuccess && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bSuccess && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		if (const UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.add_channel_host(%s, %s)"),
				*InChannel.ToPythonString(),
				*InHost.ToPythonString()));
		}
	}
#endif

	return bSuccess;
}

bool UTLLRN_RigHierarchyController::RemoveChannelHost(FTLLRN_RigElementKey InChannel, FTLLRN_RigElementKey InHost, bool bSetupUndo, bool bPrintPythonCommand)
{
	if(!IsValid())
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

	FTLLRN_RigBaseElement* ChannelBase = Hierarchy->Find(InChannel);
	if(ChannelBase == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Channel Host, Channel '%s' not found."), *InChannel.ToString());
		return false;
	}

	FTLLRN_RigControlElement* Channel = Cast<FTLLRN_RigControlElement>(ChannelBase);
	if(Channel == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Channel Host, '%s' is not a Control."), *InChannel.ToString());
		return false;
	}

	if(!Channel->IsAnimationChannel())
	{
		ReportWarningf(TEXT("Cannot Remove Channel Host, '%s' is not an animation channel."), *InChannel.ToString());
		return false;
	}

	const FTLLRN_RigBaseElement* HostBase = Hierarchy->Find(InHost);
	if(HostBase == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Channel Host, Host '%s' not found."), *InHost.ToString());
		return false;
	}

	const FTLLRN_RigControlElement* Host = Cast<FTLLRN_RigControlElement>(HostBase);
	if(Host == nullptr)
	{
		ReportWarningf(TEXT("Cannot Remove Channel Host, '%s' is not a Control."), *InHost.ToString());
		return false;
	}

	if(Host->IsAnimationChannel())
	{
		ReportWarningf(TEXT("Cannot Remove Channel Host, '%s' is also an animation channel."), *InHost.ToString());
		return false;
	}

	if(!Channel->Settings.Customization.AvailableSpaces.Contains(Host->GetKey()))
	{
		ReportWarningf(TEXT("Cannot Remove Channel Host, '%s' is not a host for channel '%s'."), *InHost.ToString(), *InChannel.ToString());
		return false;
	}

#if WITH_EDITOR
	TSharedPtr<FScopedTransaction> TransactionPtr;
	if(bSetupUndo)
	{
		TransactionPtr = MakeShared<FScopedTransaction>(NSLOCTEXT("TLLRN_RigHierarchyController", "RemoveChannelHost", "Remove Channel Host"));
		Hierarchy->Modify();
	}
#endif

	const bool bSuccess = RemoveAvailableSpace(Channel, Host);

#if WITH_EDITOR
	if(!bSuccess && TransactionPtr.IsValid())
	{
		TransactionPtr->Cancel();
	}
	TransactionPtr.Reset();

	if (bSuccess && bPrintPythonCommand && !bSuspendPythonPrinting)
	{
		if (const UBlueprint* Blueprint = GetTypedOuter<UBlueprint>())
		{
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.remove_channel_host(%s, %s)"),
				*InChannel.ToPythonString(),
				*InHost.ToPythonString()));
		}
	}
#endif

	return bSuccess;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::DuplicateElements(TArray<FTLLRN_RigElementKey> InKeys, bool bSelectNewElements, bool bSetupUndo, bool bPrintPythonCommands)
{
	const FString Content = ExportToText(InKeys);
	TArray<FTLLRN_RigElementKey> Result = ImportFromText(Content, false, bSelectNewElements, bSetupUndo);

#if WITH_EDITOR
	if (!Result.IsEmpty() && bPrintPythonCommands && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			FString ArrayStr = TEXT("[");
			for (auto It = InKeys.CreateConstIterator(); It; ++It)
			{
				ArrayStr += It->ToPythonString();
				if (It.GetIndex() < InKeys.Num() - 1)
				{
					ArrayStr += TEXT(", ");
				}
			}
			ArrayStr += TEXT("]");		
		
			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.duplicate_elements(%s, %s)"),
				*ArrayStr,
				(bSelectNewElements) ? TEXT("True") : TEXT("False")));
		}
	}
#endif

	GetHierarchy()->EnsureCacheValidity();
	
	return Result;
}

TArray<FTLLRN_RigElementKey> UTLLRN_RigHierarchyController::MirrorElements(TArray<FTLLRN_RigElementKey> InKeys, FRigVMMirrorSettings InSettings, bool bSelectNewElements, bool bSetupUndo, bool bPrintPythonCommands)
{
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	FTLLRN_RigHierarchyInteractionBracket InteractionBracket(Hierarchy);

	TArray<FTLLRN_RigElementKey> OriginalKeys = Hierarchy->SortKeys(InKeys);
	TArray<FTLLRN_RigElementKey> DuplicatedKeys = DuplicateElements(OriginalKeys, bSelectNewElements, bSetupUndo);

	if (DuplicatedKeys.Num() != OriginalKeys.Num())
	{
		return DuplicatedKeys;
	}

	for (int32 Index = 0; Index < OriginalKeys.Num(); Index++)
	{
		if (DuplicatedKeys[Index].Type != OriginalKeys[Index].Type)
		{
			return DuplicatedKeys;
		}
	}

	// mirror the transforms
	for (int32 Index = 0; Index < OriginalKeys.Num(); Index++)
	{
		FTransform GlobalTransform = Hierarchy->GetGlobalTransform(OriginalKeys[Index]);
		FTransform InitialTransform = Hierarchy->GetInitialGlobalTransform(OriginalKeys[Index]);

		// also mirror the offset, limits and shape transform
		if (OriginalKeys[Index].Type == ETLLRN_RigElementType::Control)
		{
			if(FTLLRN_RigControlElement* DuplicatedControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(DuplicatedKeys[Index]))
			{
				TGuardValue<TArray<FTLLRN_RigControlLimitEnabled>> DisableLimits(DuplicatedControlElement->Settings.LimitEnabled, TArray<FTLLRN_RigControlLimitEnabled>());

				// mirror offset
				FTransform OriginalGlobalOffsetTransform = Hierarchy->GetGlobalControlOffsetTransform(OriginalKeys[Index]);
				FTransform ParentTransform = Hierarchy->GetParentTransform(DuplicatedKeys[Index]);
				FTransform OffsetTransform = InSettings.MirrorTransform(OriginalGlobalOffsetTransform).GetRelativeTransform(ParentTransform);
				Hierarchy->SetControlOffsetTransform(DuplicatedKeys[Index], OffsetTransform, true, false, true);
				Hierarchy->SetControlOffsetTransform(DuplicatedKeys[Index], OffsetTransform, false, false, true);

				// mirror limits
				FTransform DuplicatedGlobalOffsetTransform = Hierarchy->GetGlobalControlOffsetTransform(DuplicatedKeys[Index]);

				for (ETLLRN_RigControlValueType ValueType = ETLLRN_RigControlValueType::Minimum;
                    ValueType <= ETLLRN_RigControlValueType::Maximum;
                    ValueType = (ETLLRN_RigControlValueType)(uint8(ValueType) + 1)
                    )
				{
					const FTLLRN_RigControlValue LimitValue = Hierarchy->GetControlValue(DuplicatedKeys[Index], ValueType);
					const FTransform LocalLimitTransform = LimitValue.GetAsTransform(DuplicatedControlElement->Settings.ControlType, DuplicatedControlElement->Settings.PrimaryAxis);
					FTransform GlobalLimitTransform = LocalLimitTransform * OriginalGlobalOffsetTransform;
					FTransform DuplicatedLimitTransform = InSettings.MirrorTransform(GlobalLimitTransform).GetRelativeTransform(DuplicatedGlobalOffsetTransform);
					FTLLRN_RigControlValue DuplicatedValue;
					DuplicatedValue.SetFromTransform(DuplicatedLimitTransform, DuplicatedControlElement->Settings.ControlType, DuplicatedControlElement->Settings.PrimaryAxis);
					Hierarchy->SetControlValue(DuplicatedControlElement, DuplicatedValue, ValueType, false);
				}

				// we need to do this here to make sure that the limits don't apply ( the tguardvalue is still active within here )
				Hierarchy->SetGlobalTransform(DuplicatedKeys[Index], InSettings.MirrorTransform(GlobalTransform), true, false, true);
				Hierarchy->SetGlobalTransform(DuplicatedKeys[Index], InSettings.MirrorTransform(GlobalTransform), false, false, true);

				// mirror shape transform
				FTransform GlobalShapeTransform = Hierarchy->GetControlShapeTransform(DuplicatedControlElement, ETLLRN_RigTransformType::InitialLocal) * OriginalGlobalOffsetTransform;
				Hierarchy->SetControlShapeTransform(DuplicatedControlElement, InSettings.MirrorTransform(GlobalShapeTransform).GetRelativeTransform(DuplicatedGlobalOffsetTransform), ETLLRN_RigTransformType::InitialLocal, true);
				Hierarchy->SetControlShapeTransform(DuplicatedControlElement, InSettings.MirrorTransform(GlobalShapeTransform).GetRelativeTransform(DuplicatedGlobalOffsetTransform), ETLLRN_RigTransformType::CurrentLocal, true);
			}
		}
		else
		{
			Hierarchy->SetGlobalTransform(DuplicatedKeys[Index], InSettings.MirrorTransform(GlobalTransform), true, false, true);
			Hierarchy->SetGlobalTransform(DuplicatedKeys[Index], InSettings.MirrorTransform(GlobalTransform), false, false, true);
		}
	}

	// correct the names
	if (!InSettings.SearchString.IsEmpty() && !InSettings.ReplaceString.IsEmpty())
	{
		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);
		
		for (int32 Index = 0; Index < DuplicatedKeys.Num(); Index++)
		{
			FName OldName = OriginalKeys[Index].Name;
			FString OldNameStr = OldName.ToString();
			FString NewNameStr = OldNameStr.Replace(*InSettings.SearchString, *InSettings.ReplaceString, ESearchCase::CaseSensitive);
			if (NewNameStr != OldNameStr)
			{
				Controller->RenameElement(DuplicatedKeys[Index], *NewNameStr, true);
			}
		}
	}

#if WITH_EDITOR
	if (!DuplicatedKeys.IsEmpty() && bPrintPythonCommands && !bSuspendPythonPrinting)
	{
		UBlueprint* Blueprint = GetTypedOuter<UBlueprint>();
		if (Blueprint)
		{
			FString ArrayStr = TEXT("[");
			for (auto It = InKeys.CreateConstIterator(); It; ++It)
			{
				ArrayStr += It->ToPythonString();
				if (It.GetIndex() < InKeys.Num() - 1)
				{
					ArrayStr += TEXT(", ");
				}
			}
			ArrayStr += TEXT("]");

			RigVMPythonUtils::Print(Blueprint->GetFName().ToString(),
				FString::Printf(TEXT("hierarchy_controller.mirror_elements(%s, unreal.RigMirrorSettings(%s, %s, '%s', '%s'), %s)"),
				*ArrayStr,
				*RigVMPythonUtils::EnumValueToPythonString<EAxis::Type>((int64)InSettings.MirrorAxis.GetValue()),
				*RigVMPythonUtils::EnumValueToPythonString<EAxis::Type>((int64)InSettings.AxisToFlip.GetValue()),
				*InSettings.SearchString,
				*InSettings.ReplaceString,
				(bSelectNewElements) ? TEXT("True") : TEXT("False")));
		}
	}
#endif

	Hierarchy->EnsureCacheValidity();
	
	return DuplicatedKeys;
}

bool UTLLRN_RigHierarchyController::SetParent(FTLLRN_RigBaseElement* InChild, FTLLRN_RigBaseElement* InParent, bool bMaintainGlobalTransform)
{
	if(InChild == nullptr || InParent == nullptr)
	{
		return false;
	}
	return AddParent(InChild, InParent, 1.f, bMaintainGlobalTransform, true);
}

bool UTLLRN_RigHierarchyController::AddAvailableSpace(FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigTransformElement* InSpaceElement)
{
	if(InControlElement == nullptr || InSpaceElement == nullptr)
	{
		return false;
	}

	// we cannot use animation channels as spaces / channel hosts
	if(const FTLLRN_RigControlElement* SpaceControlElement = Cast<FTLLRN_RigControlElement>(InSpaceElement))
	{
		if(SpaceControlElement->IsAnimationChannel())
		{
			return false;
		}
	}

	// in case of animation channels - we can only relate them to controls
	if(InControlElement->IsAnimationChannel())
	{
		if(!InSpaceElement->IsA<FTLLRN_RigControlElement>())
		{
			return false;
		}
	}

	// the default parent cannot be added to the available spaces list
	if(GetHierarchy()->GetParents(InControlElement).Contains(InSpaceElement))
	{
		return false;
	}

	FTLLRN_RigControlSettings Settings = InControlElement->Settings;
	TArray<FTLLRN_RigElementKey>& AvailableSpaces = Settings.Customization.AvailableSpaces;
	if(AvailableSpaces.Contains(InSpaceElement->GetKey()))
	{
		return false;
	}

	AvailableSpaces.Add(InSpaceElement->GetKey());

	GetHierarchy()->SetControlSettings(InControlElement, Settings, false, false, false);
	return true;
}

bool UTLLRN_RigHierarchyController::RemoveAvailableSpace(FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigTransformElement* InSpaceElement)
{
	if(InControlElement == nullptr || InSpaceElement == nullptr)
	{
		return false;
	}

	FTLLRN_RigControlSettings Settings = InControlElement->Settings;
	TArray<FTLLRN_RigElementKey>& AvailableSpaces =Settings.Customization.AvailableSpaces;
	const int32 NumRemoved = AvailableSpaces.Remove(InSpaceElement->GetKey());
	if(NumRemoved == 0)
	{
		return false;
	}
	
	GetHierarchy()->SetControlSettings(InControlElement, Settings, false, false, false);
	return true;
}

bool UTLLRN_RigHierarchyController::SetAvailableSpaceIndex(FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigTransformElement* InSpaceElement, int32 InIndex)
{
	if(InControlElement == nullptr || InSpaceElement == nullptr)
	{
		return false;
	}

	FTLLRN_RigControlSettings Settings = InControlElement->Settings;
	TArray<FTLLRN_RigElementKey>& AvailableSpaces = Settings.Customization.AvailableSpaces;

	bool bAddedAvailableSpace = false;
	if(!AvailableSpaces.Contains(InSpaceElement->GetKey()))
	{
		bAddedAvailableSpace = AddAvailableSpace(InControlElement, InSpaceElement);
		if(!bAddedAvailableSpace)
		{
			return false;
		}
	}

	if(AvailableSpaces.Find(InSpaceElement->GetKey()) == InIndex)
	{
		return bAddedAvailableSpace;
	}

	(void)AvailableSpaces.Remove(InSpaceElement->GetKey());
	InIndex = FMath::Max(InIndex, 0);

	if(InIndex >= AvailableSpaces.Num())
	{
		AvailableSpaces.Add(InSpaceElement->GetKey());
	}
	else
	{
		AvailableSpaces.Insert(InSpaceElement->GetKey(), InIndex);
	}

	GetHierarchy()->SetControlSettings(InControlElement, Settings, false, false, false);
	return true;
}

void UTLLRN_RigHierarchyController::AddElementToDirty(FTLLRN_RigBaseElement* InParent, FTLLRN_RigBaseElement* InElementToAdd, int32 InHierarchyDistance) const
{
	if(InParent == nullptr)
	{
		return;
	} 

	FTLLRN_RigTransformElement* ElementToAdd = Cast<FTLLRN_RigTransformElement>(InElementToAdd);
	if(ElementToAdd == nullptr)
	{
		return;
	}

	if(FTLLRN_RigTransformElement* TransformParent = Cast<FTLLRN_RigTransformElement>(InParent))
	{
		const FTLLRN_RigTransformElement::FElementToDirty ElementToDirty(ElementToAdd, InHierarchyDistance);
		TransformParent->ElementsToDirty.AddUnique(ElementToDirty);
	}
}

void UTLLRN_RigHierarchyController::RemoveElementToDirty(FTLLRN_RigBaseElement* InParent, FTLLRN_RigBaseElement* InElementToRemove) const
{
	if(InParent == nullptr)
	{
		return;
	}

	FTLLRN_RigTransformElement* ElementToRemove = Cast<FTLLRN_RigTransformElement>(InElementToRemove);
	if(ElementToRemove == nullptr)
	{
		return;
	}

	if(FTLLRN_RigTransformElement* TransformParent = Cast<FTLLRN_RigTransformElement>(InParent))
	{
		TransformParent->ElementsToDirty.Remove(ElementToRemove);
	}
}

void UTLLRN_RigHierarchyController::ReportWarning(const FString& InMessage) const
{
	if(!bReportWarningsAndErrors)
	{
		return;
	}

	if(LogFunction)
	{
		LogFunction(EMessageSeverity::Warning, InMessage);
		return;
	}

	FString Message = InMessage;
	if (const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		if (const UPackage* Package = Cast<UPackage>(Hierarchy->GetOutermost()))
		{
			Message = FString::Printf(TEXT("%s : %s"), *Package->GetPathName(), *InMessage);
		}
	}

	FScriptExceptionHandler::Get().HandleException(ELogVerbosity::Warning, *Message, *FString());
}

void UTLLRN_RigHierarchyController::ReportError(const FString& InMessage) const
{
	if(!bReportWarningsAndErrors)
	{
		return;
	}

	if(LogFunction)
	{
		LogFunction(EMessageSeverity::Error, InMessage);
		return;
	}

	FString Message = InMessage;
	if (const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		if (const UPackage* Package = Cast<UPackage>(Hierarchy->GetOutermost()))
		{
			Message = FString::Printf(TEXT("%s : %s"), *Package->GetPathName(), *InMessage);
		}
	}

	FScriptExceptionHandler::Get().HandleException(ELogVerbosity::Error, *Message, *FString());
}

void UTLLRN_RigHierarchyController::ReportAndNotifyError(const FString& InMessage) const
{
	if (!bReportWarningsAndErrors)
	{
		return;
	}

	ReportError(InMessage);

#if WITH_EDITOR
	FNotificationInfo Info(FText::FromString(InMessage));
	Info.bUseSuccessFailIcons = true;
	Info.Image = FAppStyle::GetBrush(TEXT("MessageLog.Warning"));
	Info.bFireAndForget = true;
	Info.bUseThrobber = true;
	// longer message needs more time to read
	Info.FadeOutDuration = FMath::Clamp(0.1f * InMessage.Len(), 5.0f, 20.0f);
	Info.ExpireDuration = Info.FadeOutDuration;
	TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (NotificationPtr)
	{
		NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);
	}
#endif
}


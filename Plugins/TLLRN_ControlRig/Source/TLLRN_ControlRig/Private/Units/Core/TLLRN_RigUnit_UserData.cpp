// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Core/TLLRN_RigUnit_UserData.h"
#include "RigVMCore/RigVMRegistry.h"
#include "RigVMCore/RigVMMemoryStorage.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_UserData)

#if WITH_EDITOR

FString FTLLRN_RigDispatch_GetUserData::GetArgumentMetaData(const FName& InArgumentName, const FName& InMetaDataKey) const
{
	if(InMetaDataKey == FRigVMStruct::CustomWidgetMetaName)
	{
		if(InArgumentName == ArgNameSpaceName)
		{
			return TEXT("UserDataNameSpace");
		}
		if(InArgumentName == ArgPathName)
		{
			return TEXT("UserDataPath");
		}
	}
	return FTLLRN_RigDispatchFactory::GetArgumentMetaData(InArgumentName, InMetaDataKey);
}

#endif

const TArray<FRigVMTemplateArgumentInfo>& FTLLRN_RigDispatch_GetUserData::GetArgumentInfos() const
{
	static TArray<FRigVMTemplateArgumentInfo> Infos;
	if (Infos.IsEmpty())
	{
		static const TArray<FRigVMTemplateArgument::ETypeCategory> ValueCategories = {
			FRigVMTemplateArgument::ETypeCategory_SingleAnyValue,
			FRigVMTemplateArgument::ETypeCategory_ArrayAnyValue
		};
		
		Infos.Emplace(ArgNameSpaceName, ERigVMPinDirection::Input, RigVMTypeUtils::TypeIndex::FString);
		Infos.Emplace(ArgPathName, ERigVMPinDirection::Input, RigVMTypeUtils::TypeIndex::FString);
		Infos.Emplace(ArgDefaultName, ERigVMPinDirection::Input, ValueCategories);
		Infos.Emplace(ArgResultName, ERigVMPinDirection::Output, ValueCategories);
		Infos.Emplace(ArgFoundName, ERigVMPinDirection::Output, RigVMTypeUtils::TypeIndex::Bool);
	}
	
	return Infos;
}

FRigVMTemplateTypeMap FTLLRN_RigDispatch_GetUserData::OnNewArgumentType(const FName& InArgumentName,
	TRigVMTypeIndex InTypeIndex) const
{
	FRigVMTemplateTypeMap Types;
	Types.Add(ArgNameSpaceName, RigVMTypeUtils::TypeIndex::FString);
	Types.Add(ArgPathName, RigVMTypeUtils::TypeIndex::FString);
	Types.Add(ArgDefaultName, InTypeIndex);
	Types.Add(ArgResultName, InTypeIndex);
	Types.Add(ArgFoundName, RigVMTypeUtils::TypeIndex::Bool);
	return Types;
}

FRigVMFunctionPtr FTLLRN_RigDispatch_GetUserData::GetDispatchFunctionImpl(const FRigVMTemplateTypeMap& InTypes) const
{
	check(InTypes.FindChecked(ArgNameSpaceName) == RigVMTypeUtils::TypeIndex::FString);
	check(InTypes.FindChecked(ArgPathName) == RigVMTypeUtils::TypeIndex::FString);
	const TRigVMTypeIndex TypeIndex = InTypes.FindChecked(ArgDefaultName);
	check(FRigVMRegistry_NoLock::GetForRead().CanMatchTypes_NoLock(TypeIndex, InTypes.FindChecked(ArgResultName), true));
	check(InTypes.FindChecked(ArgFoundName) == RigVMTypeUtils::TypeIndex::Bool);
	return &FTLLRN_RigDispatch_GetUserData::Execute;
}

void FTLLRN_RigDispatch_GetUserData::Execute(FRigVMExtendedExecuteContext& InContext, FRigVMMemoryHandleArray Handles, FRigVMPredicateBranchArray RigVMBranches)
{
	const FTLLRN_ControlRigExecuteContext& TLLRN_ControlRigContext = InContext.GetPublicData<FTLLRN_ControlRigExecuteContext>();

	if(TLLRN_ControlRigContext.GetEventName().IsNone())
	{
		// may happen during init
		return;
	}
	
	// allow to run only in construction event
	if(TLLRN_ControlRigContext.GetEventName() != FTLLRN_RigUnit_PrepareForExecution::EventName)
	{
		static constexpr TCHAR ErrorMessageFormat[] = TEXT("It's recommended to use this node in the %s event. When using it in other events please consider caching the results.");
		TLLRN_ControlRigContext.Logf(EMessageSeverity::Info, ErrorMessageFormat, *FTLLRN_RigUnit_PrepareForExecution::EventName.ToString());
	}

	const FProperty* PropertyDefault = Handles[2].GetResolvedProperty(); 
	const FProperty* PropertyResult = Handles[3].GetResolvedProperty(); 
#if WITH_EDITOR
	check(PropertyDefault);
	check(PropertyResult);
	check(Handles[0].IsString());
	check(Handles[1].IsString());
	check(Handles[4].IsBool());
#endif

	const FString& NameSpace = *(FString*)Handles[0].GetData();
	const FString& Path = *(FString*)Handles[1].GetData();
	const uint8* Default = Handles[2].GetData();
	uint8* Result = Handles[3].GetData();
	bool& bFound = *(bool*)Handles[4].GetData();
	bFound = false;

	auto Log = [&TLLRN_ControlRigContext](const FString& Message)
	{
#if WITH_EDITOR
		if(TLLRN_ControlRigContext.GetLog())
		{
			TLLRN_ControlRigContext.Report(EMessageSeverity::Info, TLLRN_ControlRigContext.GetFunctionName(), TLLRN_ControlRigContext.GetInstructionIndex(), Message);
		}
		else
#endif
		{
			TLLRN_ControlRigContext.Log(EMessageSeverity::Info, Message);
		}
	};

	
	if(const UNameSpacedUserData* UserDataObject = TLLRN_ControlRigContext.FindUserData(NameSpace))
	{
		FString ErrorMessage;
		if(const UNameSpacedUserData::FUserData* UserData = UserDataObject->GetUserData(Path, &ErrorMessage))
		{
			if(UserData->GetProperty())
			{
#if WITH_EDITOR
				FString ExtendedCPPType;
				const FString CPPType = PropertyResult->GetCPPType(&ExtendedCPPType);
				const FRigVMRegistry& Registry = FRigVMRegistry::Get();
				const TRigVMTypeIndex ResultTypeIndex = Registry.GetTypeIndexFromCPPType(CPPType + ExtendedCPPType);
				const TRigVMTypeIndex UserDataTypeIndex = Registry.GetTypeIndexFromCPPType(UserData->GetCPPType().ToString());

				if(Registry.CanMatchTypes(ResultTypeIndex, UserDataTypeIndex, true))
#endif
				{
					URigVMMemoryStorage::CopyProperty(PropertyResult, Result, UserData->GetProperty(), UserData->GetMemory());
					bFound = true;
					return;
				}

#if WITH_EDITOR
				static constexpr TCHAR Format[] = TEXT("User data of type '%s' not compatible with pin of type '%s'.");
				TLLRN_ControlRigContext.Logf(EMessageSeverity::Info, Format, *UserData->GetCPPType().ToString(), *(CPPType + ExtendedCPPType));
#endif
			}
		}
#if WITH_EDITOR
		else
		{
			TLLRN_ControlRigContext.Log(EMessageSeverity::Info, ErrorMessage);
		}
#endif
	}
#if WITH_EDITOR
	else
	{
		static constexpr TCHAR Format[] = TEXT("User data namespace '%s' cannot be found.");
		TLLRN_ControlRigContext.Logf(EMessageSeverity::Info, Format, *NameSpace);
	}
#endif

	// fall back behavior - copy default to result
	URigVMMemoryStorage::CopyProperty(PropertyResult, Result, PropertyDefault, Default);
}

FTLLRN_RigUnit_SetupShapeLibraryFromUserData_Execute()
{
	// this may happen during init
	if(ExecuteContext.GetEventName().IsNone())
	{
		return;
	}

	auto Log = [&ExecuteContext](const FString& Message)
	{
#if WITH_EDITOR
		if(ExecuteContext.GetLog())
		{
			ExecuteContext.Report(EMessageSeverity::Info, ExecuteContext.GetFunctionName(), ExecuteContext.GetInstructionIndex(), Message);
		}
		else
#endif
		{
			ExecuteContext.Log(EMessageSeverity::Info, Message);
		}
	};
	
	// allow to run only in construction event
	if(ExecuteContext.GetEventName() != FTLLRN_RigUnit_PrepareForExecution::EventName)
	{
		static constexpr TCHAR MessageFormat[] = TEXT("It's recommended to use this node in the %s event. When using it in other events please consider caching the results.");
		ExecuteContext.Logf(EMessageSeverity::Info, MessageFormat, *FTLLRN_RigUnit_PrepareForExecution::EventName.ToString());
	}

	if(const UNameSpacedUserData* UserDataObject = ExecuteContext.FindUserData(NameSpace))
	{
		FString ErrorMessage;
		if(const UNameSpacedUserData::FUserData* UserData = UserDataObject->GetUserData(Path, &ErrorMessage))
		{
			if(const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(UserData->GetProperty()))
			{
				if(ObjectProperty->PropertyClass == UTLLRN_ControlRigShapeLibrary::StaticClass())
				{
					if(ExecuteContext.OnAddShapeLibraryDelegate.IsBound())
					{
						ExecuteContext.OnAddShapeLibraryDelegate.Execute(&ExecuteContext, LibraryName, (UTLLRN_ControlRigShapeLibrary*)UserData->GetMemory(), LogShapeLibraries);
					}
					return;
				}
			}
#if WITH_EDITOR
			static constexpr TCHAR Format[] = TEXT("User data for path '%s' is of type '%s' - not a shape library.");
			Log(FString::Printf(Format, *Path, *UserData->GetCPPType().ToString()));
#endif
		}
#if WITH_EDITOR
		else
		{
			Log(ErrorMessage);
		}
#endif
	}
#if WITH_EDITOR
	else
	{
		static constexpr TCHAR Format[] = TEXT("User data namespace '%s' cannot be found.");
		Log(FString::Printf(Format, *NameSpace));
	}
#endif
}

FTLLRN_RigUnit_ShapeExists_Execute()
{
	Result = false;
	if (ExecuteContext.OnShapeExistsDelegate.IsBound())
	{
		Result = ExecuteContext.OnShapeExistsDelegate.Execute(ShapeName);
	}
}
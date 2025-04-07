// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_TLLRN_RigConnectionRules.h"
#include "Rigs/TLLRN_RigHierarchyElements.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "TLLRN_ModularRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_TLLRN_RigConnectionRules)

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigConnectionRuleStash
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigConnectionRuleStash::FTLLRN_RigConnectionRuleStash()
{
}

FTLLRN_RigConnectionRuleStash::FTLLRN_RigConnectionRuleStash(const FTLLRN_RigConnectionRule* InRule)
{
	check(InRule);
	ScriptStructPath = InRule->GetScriptStruct()->GetPathName();
	InRule->GetScriptStruct()->ExportText(ExportedText, InRule, InRule, nullptr, PPF_None, nullptr);
}

void FTLLRN_RigConnectionRuleStash::Save(FArchive& Ar)
{
	Ar << ScriptStructPath;
	Ar << ExportedText;
}

void FTLLRN_RigConnectionRuleStash::Load(FArchive& Ar)
{
	Ar << ScriptStructPath;
	Ar << ExportedText;
}

bool FTLLRN_RigConnectionRuleStash::IsValid() const
{
	return !ScriptStructPath.IsEmpty() && !ExportedText.IsEmpty();
}

UScriptStruct* FTLLRN_RigConnectionRuleStash::GetScriptStruct() const
{
	if(!ScriptStructPath.IsEmpty())
	{
		return FindObject<UScriptStruct>(nullptr, *ScriptStructPath);
	}
	return nullptr;
}

TSharedPtr<FStructOnScope> FTLLRN_RigConnectionRuleStash::Get() const
{
	class FErrorPipe : public FOutputDevice
	{
	public:

		int32 NumErrors;

		FErrorPipe()
			: FOutputDevice()
			, NumErrors(0)
		{
		}

		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
		{
			NumErrors++;
		}
	};

	UScriptStruct* ScriptStruct = GetScriptStruct();
	check(ScriptStruct);
	TSharedPtr<FStructOnScope> StructOnScope = MakeShareable(new FStructOnScope(ScriptStruct));
	FErrorPipe ErrorPipe;
	ScriptStruct->ImportText(*ExportedText, StructOnScope->GetStructMemory(), nullptr, PPF_None, &ErrorPipe, ScriptStruct->GetName());
	return StructOnScope;
}

const FTLLRN_RigConnectionRule* FTLLRN_RigConnectionRuleStash::Get(TSharedPtr<FStructOnScope>& InOutStorage) const
{
	InOutStorage = Get();
	if(InOutStorage.IsValid() && InOutStorage->IsValid())
	{
		check(InOutStorage->GetStruct()->IsChildOf(FTLLRN_RigConnectionRule::StaticStruct()));
		return reinterpret_cast<const FTLLRN_RigConnectionRule*>(InOutStorage->GetStructMemory());
	}
	return nullptr;
}

bool FTLLRN_RigConnectionRuleStash::operator==(const FTLLRN_RigConnectionRuleStash& InOther) const
{
	return ScriptStructPath.Equals(InOther.ScriptStructPath, ESearchCase::CaseSensitive) &&
		ExportedText.Equals(InOther.ExportedText, ESearchCase::CaseSensitive);
}

uint32 GetTypeHash(const FTLLRN_RigConnectionRuleStash& InRuleStash)
{
	return HashCombine(GetTypeHash(InRuleStash.ScriptStructPath), GetTypeHash(InRuleStash.ExportedText));
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigConnectionRuleInput
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigConnectorElement* FTLLRN_RigConnectionRuleInput::FindPrimaryConnector(FText* OutErrorMessage) const
{
	check(Hierarchy);
	check(Module);

	const FName ModuleNameSpace = *Module->GetNamespace();

	const FTLLRN_RigConnectorElement* PrimaryConnector = nullptr;
	Hierarchy->ForEach<FTLLRN_RigConnectorElement>(
		[this, ModuleNameSpace, &PrimaryConnector](FTLLRN_RigConnectorElement* Connector) -> bool
		{
			if(Connector->IsPrimary())
			{
				const FName ConnectorNameSpace = Hierarchy->GetNameSpaceFName(Connector->GetKey());
				if(!ConnectorNameSpace.IsNone() && ConnectorNameSpace.IsEqual(ModuleNameSpace, ENameCase::IgnoreCase))
				{
					PrimaryConnector = Connector;
					return false; // stop the search
				}
			}
			return true; // continue the search
		}
	);

	if(PrimaryConnector == nullptr)
	{
		if(OutErrorMessage)
		{
			static constexpr TCHAR Format[] = TEXT("No primary connector found for module '%s'.");
			*OutErrorMessage = FText::FromString(FString::Printf(Format, *Module->GetPath()));
		}
	}

	return PrimaryConnector;
}

TArray<const FTLLRN_RigConnectorElement*> FTLLRN_RigConnectionRuleInput::FindSecondaryConnectors(bool bOptional, FText* OutErrorMessage) const
{
	check(Hierarchy);
	check(Module);

	const FName ModuleNameSpace = *Module->GetNamespace();

	TArray<const FTLLRN_RigConnectorElement*> SecondaryConnectors;
	Hierarchy->ForEach<FTLLRN_RigConnectorElement>(
		[this, bOptional, ModuleNameSpace, &SecondaryConnectors](const FTLLRN_RigConnectorElement* Connector) -> bool
		{
			if(Connector->IsSecondary() && Connector->IsOptional() == bOptional)
			{
				const FName ConnectorNameSpace = Hierarchy->GetNameSpaceFName(Connector->GetKey());
				if(!ConnectorNameSpace.IsNone() && ConnectorNameSpace.IsEqual(ModuleNameSpace, ENameCase::IgnoreCase))
				{
					SecondaryConnectors.Add(Connector);
				}
			}
			return true; // continue the search
		}
	);

	return SecondaryConnectors;
}

const FTLLRN_RigTransformElement* FTLLRN_RigConnectionRuleInput::ResolveConnector(const FTLLRN_RigConnectorElement* InConnector, FText* OutErrorMessage) const
{
	check(Redirector);

	if(const FTLLRN_CachedTLLRN_RigElement* Target = Redirector->Find(InConnector->GetKey()))
	{
		return Cast<FTLLRN_RigTransformElement>(Target->GetElement());
	}

	if(OutErrorMessage)
	{
		static constexpr TCHAR Format[] = TEXT("Resolved target not found for connector '%s'.");
		*OutErrorMessage = FText::FromString(FString::Printf(Format, *InConnector->GetName()));
	}
	return nullptr;
}

const FTLLRN_RigTransformElement* FTLLRN_RigConnectionRuleInput::ResolvePrimaryConnector(FText* OutErrorMessage) const
{
	if(const FTLLRN_RigConnectorElement* Connector = FindPrimaryConnector(OutErrorMessage))
	{
		return ResolveConnector(Connector, OutErrorMessage);
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigConnectionRule
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigElementResolveResult FTLLRN_RigConnectionRule::Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const
{
	FTLLRN_RigElementResolveResult Result(InTarget->GetKey());
	Result.SetInvalidTarget(FText());
	return Result;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigAndConnectionRule
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigElementResolveResult FTLLRN_RigAndConnectionRule::Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const
{
	FTLLRN_RigElementResolveResult Result(InTarget->GetKey());
	Result.SetPossibleTarget();
	
	TSharedPtr<FStructOnScope> Storage;
	for(const FTLLRN_RigConnectionRuleStash& ChildRule : ChildRules)
	{
		if(const FTLLRN_RigConnectionRule* Rule = ChildRule.Get(Storage))
		{
			Result = Rule->Resolve(InTarget, InRuleInput);
			if(!Result.IsValid())
			{
				return Result;
			}
		}
	}

	return Result;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigOrConnectionRule
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigElementResolveResult FTLLRN_RigOrConnectionRule::Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const
{
	FTLLRN_RigElementResolveResult Result(InTarget->GetKey());
	Result.SetPossibleTarget();
	
	TSharedPtr<FStructOnScope> Storage;
	for(const FTLLRN_RigConnectionRuleStash& ChildRule : ChildRules)
	{
		if(const FTLLRN_RigConnectionRule* Rule = ChildRule.Get(Storage))
		{
			Result = Rule->Resolve(InTarget, InRuleInput);
			if(Result.IsValid())
			{
				return Result;
			}
		}
	}

	return Result;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigTypeConnectionRule
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigElementResolveResult FTLLRN_RigTypeConnectionRule::Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const
{
	FTLLRN_RigElementResolveResult Result(InTarget->GetKey());
	Result.SetPossibleTarget();

	if(!InTarget->GetKey().IsTypeOf(ElementType))
	{
		const FString ExpectedType = StaticEnum<ETLLRN_RigElementType>()->GetDisplayNameTextByValue((int64)ElementType).ToString();
		static constexpr TCHAR Format[] = TEXT("Element '%s' is not of the expected type (%s).");
		Result.SetInvalidTarget(FText::FromString(FString::Printf(Format, *InTarget->GetKey().ToString(), *ExpectedType)));
	}
	
	return Result;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigTagConnectionRule
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigElementResolveResult FTLLRN_RigTagConnectionRule::Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const
{
	FTLLRN_RigElementResolveResult Result(InTarget->GetKey());
	Result.SetPossibleTarget();

	if(!InRuleInput.GetHierarchy()->HasTag(InTarget->GetKey(), Tag))
	{
		static constexpr TCHAR Format[] = TEXT("Element '%s' does not contain tag '%s'.");
		Result.SetInvalidTarget(FText::FromString(FString::Printf(Format, *InTarget->GetKey().ToString(), *Tag.ToString())));
	}
	
	return Result;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigChildOfPrimaryConnectionRule
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigElementResolveResult FTLLRN_RigChildOfPrimaryConnectionRule::Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const
{
	FTLLRN_RigElementResolveResult Result(InTarget->GetKey());
	Result.SetPossibleTarget();

	// find the primary resolved target
	FText ErrorMessage;
	const FTLLRN_RigTransformElement* PrimaryTarget = InRuleInput.ResolvePrimaryConnector(&ErrorMessage);
	if(PrimaryTarget == nullptr)
	{
		Result.SetInvalidTarget(ErrorMessage);
		return Result;
	}

	static constexpr TCHAR IsNotAChildOfFormat[] = TEXT("Target '%s' is not a child of the primary.");
	static constexpr TCHAR IsAlreadyUsedForPrimaryFormat[] = TEXT("Target '%s' is already used for the primary.");
	if(InTarget == PrimaryTarget)
	{
		Result.SetInvalidTarget(FText::FromString(FString::Printf(IsAlreadyUsedForPrimaryFormat, *InTarget->GetKey().ToString())));
		return Result;
	}

	// for sockets we use the parent for resolve
	if(PrimaryTarget->GetType() == ETLLRN_RigElementType::Socket)
	{
		if(const FTLLRN_RigTransformElement* FirstParent =
			Cast<FTLLRN_RigTransformElement>(InRuleInput.GetHierarchy()->GetFirstParent(PrimaryTarget)))
		{
			PrimaryTarget = FirstParent;

			if(InTarget == PrimaryTarget)
			{
				Result.SetInvalidTarget(FText::FromString(FString::Printf(IsNotAChildOfFormat, *InTarget->GetKey().ToString())));
				return Result;
			}
		}
	}

	static const UTLLRN_RigHierarchy::TElementDependencyMap EmptyDependencyMap;
	if(!InRuleInput.GetHierarchy()->IsParentedTo(
		const_cast<FTLLRN_RigBaseElement*>(InTarget), const_cast<FTLLRN_RigTransformElement*>(PrimaryTarget), EmptyDependencyMap))
	{
		Result.SetInvalidTarget(FText::FromString(FString::Printf(IsNotAChildOfFormat, *InTarget->GetKey().ToString())));
	}

	return Result;
}

////////////////////////////////////////////////////////////////////////////////
// FRigOnChainRule
////////////////////////////////////////////////////////////////////////////////

/*
FTLLRN_RigElementResolveResult FRigOnChainRule::Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const
{
	FTLLRN_RigElementResolveResult Result;
	Result.State = ETLLRN_RigElementResolveState::PossibleTarget;

	if(!InHierarchy->HasTag(InTarget->GetKey(), Tag))
	{
		static constexpr TCHAR Format[] = TEXT("Element '%s' does not contain tag '%s'.");
		Result.State = ETLLRN_RigElementResolveState::InvalidTarget;
		Result.Message = FText::FromString(FString::Printf(Format, *InTarget->GetKey().ToString(), *Tag.ToString()));
	}
	
	return Result;
}
*/
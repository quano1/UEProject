// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModularRigRuleManager.h"

#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ModularRigRuleManager)

#define LOCTEXT_NAMESPACE "TLLRN_ModularRigRuleManager"

FTLLRN_ModularRigResolveResult UTLLRN_ModularRigRuleManager::FindMatches(FWorkData& InWorkData) const
{
	check(InWorkData.Result);
	FTLLRN_ModularRigResolveResult& Result = *InWorkData.Result;

	if(!Hierarchy.IsValid())
	{
		static const FText MissingHierarchyMessage = LOCTEXT("MissingHierarchyMessage", "The rule manager is missing the hierarchy.");
		Result.Message = MissingHierarchyMessage;
		Result.State = ETLLRN_ModularRigResolveState::Error;
		return Result;
	}

	if (UTLLRN_ControlRig* TLLRN_ControlRig = Hierarchy->GetTypedOuter<UTLLRN_ControlRig>())
	{
		if (TLLRN_ControlRig->IsConstructionRequired())
		{
			const FTLLRN_RigElementKey ConnectorKey = InWorkData.Connector ? InWorkData.Connector->GetKey() : FTLLRN_RigElementKey();
			
			TLLRN_ControlRig->Execute(FTLLRN_RigUnit_PrepareForExecution::EventName);

			// restore the connector. executing the control rig may have destroyed the previous connector element
			InWorkData.Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(ConnectorKey);
			if(ConnectorKey.IsValid() && InWorkData.Connector == nullptr)
			{
				return Result;
			}
		}
	}

	// start with a full set of possible targets
	TArray<bool> VisitedElement;
	VisitedElement.AddZeroed(Hierarchy->Num());
	Result.Matches.Reserve(Hierarchy->Num());
	Hierarchy->Traverse([&VisitedElement, &Result](const FTLLRN_RigBaseElement* Element, bool& bContinue)
	{
		if(!VisitedElement[Element->GetIndex()])
		{
			VisitedElement[Element->GetIndex()] = true;
			Result.Matches.Emplace(Element->GetKey(), ETLLRN_RigElementResolveState::PossibleTarget, FText());
		}
		bContinue = true;
	}, true);

	InWorkData.Hierarchy = Hierarchy.Get();
	ResolveConnector(InWorkData);
	return Result;;
}

FTLLRN_ModularRigResolveResult UTLLRN_ModularRigRuleManager::FindMatches(
	const FTLLRN_RigConnectorElement* InConnector,
	const FTLLRN_RigModuleInstance* InModule,
	const FTLLRN_RigElementKeyRedirector& InResolvedConnectors) const
{
	FTLLRN_ModularRigResolveResult Result;
	Result.Connector = InConnector->GetKey();

	FWorkData WorkData;
	WorkData.Hierarchy = Hierarchy.Get();
	WorkData.Connector = InConnector;
	WorkData.Module = InModule;
	WorkData.ResolvedConnectors = &InResolvedConnectors;
	WorkData.Result = &Result;

	return FindMatches(WorkData);
}

FTLLRN_ModularRigResolveResult UTLLRN_ModularRigRuleManager::FindMatches(const FTLLRN_RigModuleConnector* InConnector) const
{
	FTLLRN_ModularRigResolveResult Result;
	Result.Connector = FTLLRN_RigElementKey(*InConnector->Name, ETLLRN_RigElementType::Connector);

	FWorkData WorkData;
	WorkData.Hierarchy = Hierarchy.Get();
	WorkData.ModuleConnector = InConnector;
	WorkData.Result = &Result;

	return FindMatches(WorkData);
}

FTLLRN_ModularRigResolveResult UTLLRN_ModularRigRuleManager::FindMatchesForPrimaryConnector(const FTLLRN_RigModuleInstance* InModule) const
{
	static const FTLLRN_RigElementKeyRedirector EmptyRedirector;
	FTLLRN_RigConnectionRuleInput RuleInput;
	RuleInput.Hierarchy = Hierarchy.Get();
	RuleInput.Module = InModule;
	RuleInput.Redirector = &EmptyRedirector;

	FTLLRN_ModularRigResolveResult Result;
	
	if(const FTLLRN_RigConnectorElement* PrimaryConnector = RuleInput.FindPrimaryConnector(&Result.Message))
	{
		Result = FindMatches(PrimaryConnector, InModule, EmptyRedirector);
	}
	else
	{
		Result.State = ETLLRN_ModularRigResolveState::Error;
	}

	return Result;
}

TArray<FTLLRN_ModularRigResolveResult> UTLLRN_ModularRigRuleManager::FindMatchesForSecondaryConnectors(const FTLLRN_RigModuleInstance* InModule, const FTLLRN_RigElementKeyRedirector& InResolvedConnectors) const
{
	FTLLRN_RigConnectionRuleInput RuleInput;
	RuleInput.Hierarchy = Hierarchy.Get();
	RuleInput.Module = InModule;
	RuleInput.Redirector = &InResolvedConnectors;

	TArray<FTLLRN_ModularRigResolveResult> Results;

	const TArray<const FTLLRN_RigConnectorElement*> SecondaryConnectors = RuleInput.FindSecondaryConnectors(false /* optional */);
	for(const FTLLRN_RigConnectorElement* SecondaryConnector : SecondaryConnectors)
	{
		Results.Add(FindMatches(SecondaryConnector, InModule, InResolvedConnectors));
	}

	return Results;
}

TArray<FTLLRN_ModularRigResolveResult> UTLLRN_ModularRigRuleManager::FindMatchesForOptionalConnectors(const FTLLRN_RigModuleInstance* InModule, const FTLLRN_RigElementKeyRedirector& InResolvedConnectors) const
{
	FTLLRN_RigConnectionRuleInput RuleInput;
	RuleInput.Hierarchy = Hierarchy.Get();
	RuleInput.Module = InModule;
	RuleInput.Redirector = &InResolvedConnectors;

	TArray<FTLLRN_ModularRigResolveResult> Results;

	const TArray<const FTLLRN_RigConnectorElement*> OptionalConnectors = RuleInput.FindSecondaryConnectors(true /* optional */);
	for(const FTLLRN_RigConnectorElement* OptionalConnector : OptionalConnectors)
	{
		Results.Add(FindMatches(OptionalConnector, InModule, InResolvedConnectors));
	}

	return Results;
}

void UTLLRN_ModularRigRuleManager::FWorkData::Filter(TFunction<void(FTLLRN_RigElementResolveResult&)> PerMatchFunction)
{
	const TArray<FTLLRN_RigElementResolveResult> PreviousMatches = Result->Matches;;
	Result->Matches.Reset();
	for(const FTLLRN_RigElementResolveResult& PreviousMatch : PreviousMatches)
	{
		FTLLRN_RigElementResolveResult Match = PreviousMatch;
		PerMatchFunction(Match);
		if(Match.IsValid())
		{
			Result->Matches.Add(Match);
		}
		else
		{
			Result->Excluded.Add(Match);
		}
	}
}

void UTLLRN_ModularRigRuleManager::SetHierarchy(const UTLLRN_RigHierarchy* InHierarchy)
{
	check(InHierarchy);
	Hierarchy = InHierarchy;
}

void UTLLRN_ModularRigRuleManager::ResolveConnector(FWorkData& InOutWorkData)
{
	FilterIncompatibleTypes(InOutWorkData);
	FilterInvalidNameSpaces(InOutWorkData);
	FilterByConnectorRules(InOutWorkData);
	FilterByConnectorEvent(InOutWorkData);

	if(InOutWorkData.Result->Matches.IsEmpty())
	{
		InOutWorkData.Result->State = ETLLRN_ModularRigResolveState::Error;
	}
	else
	{
		InOutWorkData.Result->State = ETLLRN_ModularRigResolveState::Success;
	}
}

void UTLLRN_ModularRigRuleManager::FilterIncompatibleTypes(FWorkData& InOutWorkData)
{
	InOutWorkData.Filter([](FTLLRN_RigElementResolveResult& Result)
	{
		if(Result.GetKey().Type == ETLLRN_RigElementType::Curve)
		{
			static const FText CurveInvalidTargetMessage = LOCTEXT("CannotConnectToCurves", "Cannot connect to curves.");
			Result.SetInvalidTarget(CurveInvalidTargetMessage);
		}
		if(Result.GetKey().Type == ETLLRN_RigElementType::Connector)
		{
			static const FText CurveInvalidTargetMessage = LOCTEXT("CannotConnectToConnectors", "Cannot connect to connectors.");
			Result.SetInvalidTarget(CurveInvalidTargetMessage);
		}
	});
}

void UTLLRN_ModularRigRuleManager::FilterInvalidNameSpaces(FWorkData& InOutWorkData)
{
	if(InOutWorkData.Connector == nullptr)
	{
		return;
	}
	
	const FName NameSpace = InOutWorkData.Hierarchy->GetNameSpaceFName(InOutWorkData.Connector->GetKey());
	if(NameSpace.IsNone())
	{
		return;
	}
	
	const FString NameSpaceString = NameSpace.ToString(); 
	InOutWorkData.Filter([NameSpaceString, InOutWorkData](FTLLRN_RigElementResolveResult& Result)
	{
		const FName MatchNameSpace = InOutWorkData.Hierarchy->GetNameSpaceFName(Result.GetKey());
		if(!MatchNameSpace.IsNone())
		{
			const FString MatchNameSpaceString = MatchNameSpace.ToString();
			if(MatchNameSpaceString.Equals(NameSpaceString, ESearchCase::IgnoreCase))
			{
				static const FText CannotConnectWithinNameSpaceMessage = LOCTEXT("CannotConnectWithinNameSpace", "Cannot connect within the same namespace.");
				Result.SetInvalidTarget(CannotConnectWithinNameSpaceMessage);
			}
			else if(MatchNameSpaceString.StartsWith(NameSpaceString, ESearchCase::IgnoreCase))
			{
				static const FText CannotConnectBelowNameSpaceMessage = LOCTEXT("CannotConnectBelowNameSpace", "Cannot connect to element below the connector's namespace.");
				Result.SetInvalidTarget(CannotConnectBelowNameSpaceMessage);
			}
		}
	});
}

void UTLLRN_ModularRigRuleManager::FilterByConnectorRules(FWorkData& InOutWorkData)
{
	check(InOutWorkData.Connector != nullptr || InOutWorkData.ModuleConnector != nullptr);
	
	const TArray<FTLLRN_RigConnectionRuleStash>& Rules =
		InOutWorkData.Connector ? InOutWorkData.Connector->Settings.Rules : InOutWorkData.ModuleConnector->Settings.Rules;
	
	for(const FTLLRN_RigConnectionRuleStash& Stash : Rules)
	{
		TSharedPtr<FStructOnScope> Storage;
		const FTLLRN_RigConnectionRule* Rule = Stash.Get(Storage);

		FTLLRN_RigConnectionRuleInput RuleInput;
		RuleInput.Hierarchy = InOutWorkData.Hierarchy;
		RuleInput.Redirector = InOutWorkData.ResolvedConnectors;
		RuleInput.Module = InOutWorkData.Module;
		
		InOutWorkData.Filter([Rule, RuleInput](FTLLRN_RigElementResolveResult& Result)
		{
			const FTLLRN_RigBaseElement* Target = RuleInput.Hierarchy->Find(Result.GetKey());
			check(Target);
			Result = Rule->Resolve(Target, RuleInput);
		});
	}
}

void UTLLRN_ModularRigRuleManager::FilterByConnectorEvent(FWorkData& InOutWorkData)
{
	// this may be null during unit tests
	if(InOutWorkData.Module == nullptr)
	{
		return;
	}

	if(InOutWorkData.Connector == nullptr)
	{
		return;
	}
	
	// see if we are nested below a modular rig
	UTLLRN_ModularRig* TLLRN_ModularRig = InOutWorkData.Hierarchy->GetTypedOuter<UTLLRN_ModularRig>();
	if(TLLRN_ModularRig == nullptr)
	{
		return;
	}
	
	FTLLRN_ModularRigResolveResult* Result = InOutWorkData.Result;

	TLLRN_ModularRig->ExecuteConnectorEvent(InOutWorkData.Connector->GetKey(), InOutWorkData.Module, InOutWorkData.ResolvedConnectors, InOutWorkData.Result->Matches);
	
	// move the default match to the front of the list
	if(!Result->Matches.IsEmpty())
	{
		const int32 DefaultMatchIndex = Result->Matches.IndexOfByPredicate([](const FTLLRN_RigElementResolveResult& ElementResult) -> bool
		{
			return ElementResult.State == ETLLRN_RigElementResolveState::DefaultTarget;
		}) ;

		if(DefaultMatchIndex != INDEX_NONE)
		{
			const FTLLRN_RigElementResolveResult DefaultResult = Result->Matches[DefaultMatchIndex];
			Result->Matches.RemoveAt(DefaultMatchIndex);
			Result->Matches.Insert(DefaultResult, 0);
		};
	}
}

#undef LOCTEXT_NAMESPACE

// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Modules/TLLRN_RigUnit_ConnectionCandidates.h"
#include "Units/Modules/TLLRN_RigUnit_ConnectorExecution.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_ConnectionCandidates)

FTLLRN_RigUnit_GetCandidates_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	// only allow these nodes during connector resolval event
	if(ExecuteContext.GetEventName() != FTLLRN_RigUnit_ConnectorExecution::EventName)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Can only use GetCandidates during %s event."), *FTLLRN_RigUnit_ConnectorExecution::EventName.ToString());
		return;
	}

	Algo::Transform(ExecuteContext.UnitContext.ConnectionResolve.Matches, Candidates, [](const FTLLRN_RigElementResolveResult& Match)
	{
		return Match.GetKey();
	});
	Connector = ExecuteContext.UnitContext.ConnectionResolve.Connector;
}

FTLLRN_RigUnit_DiscardMatches_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	// only allow these nodes during connector resolval event
	if(ExecuteContext.GetEventName() != FTLLRN_RigUnit_ConnectorExecution::EventName)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Can only use DiscardMatches during %s event."), *FTLLRN_RigUnit_ConnectorExecution::EventName.ToString());
		return;
	}

	// Filter any items that are already excluded
	TArray<FTLLRN_RigElementKey> FilterExcluded = Excluded.FilterByPredicate([ExecuteContext](const FTLLRN_RigElementKey& Exclude)
	{
		return !ExecuteContext.UnitContext.ConnectionResolve.Excluded.ContainsByPredicate([Exclude](const FTLLRN_RigElementResolveResult& Existing)
		{
			return Existing.GetKey() == Exclude;
		});
	});

	// Remove elements from matches, and add to excluded
	for (FTLLRN_RigElementKey& Exclude : FilterExcluded)
	{
		int32 Index = ExecuteContext.UnitContext.ConnectionResolve.Matches.IndexOfByPredicate([Exclude](const FTLLRN_RigElementResolveResult& Match)
		{
			return Exclude == Match.GetKey();
		});
		if (Index != INDEX_NONE)
		{
			ExecuteContext.UnitContext.ConnectionResolve.Matches.RemoveAt(Index);
		}

		ExecuteContext.UnitContext.ConnectionResolve.Excluded.Add(FTLLRN_RigElementResolveResult(Exclude, ETLLRN_RigElementResolveState::InvalidTarget, FText::FromString(Message)));
	}
}

FTLLRN_RigUnit_SetDefaultMatch_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	// only allow these nodes during connector resolval event
	if(ExecuteContext.GetEventName() != FTLLRN_RigUnit_ConnectorExecution::EventName)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Can only use SetDefaultMatch during %s event."), *FTLLRN_RigUnit_ConnectorExecution::EventName.ToString());
		return;
	}

	FTLLRN_ModularRigResolveResult& Resolve = ExecuteContext.UnitContext.ConnectionResolve;
	TArray<FTLLRN_RigElementResolveResult>& Matches = Resolve.Matches;

	// Make sure the Default element is contained in the matches array
	FTLLRN_RigElementResolveResult* NewDefault = const_cast<FTLLRN_RigElementResolveResult*>(Resolve.FindMatch(Default));
	if (!NewDefault)
	{
		UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Cannot set default match %s because it is not part of %s matches."), *Default.ToString(), *Resolve.GetConnectorKey().ToString());
		return;
	}

	// Remove the old default if it exists (set to just possible target)
	FTLLRN_RigElementResolveResult* OldDefault = const_cast<FTLLRN_RigElementResolveResult*>(Resolve.GetDefaultMatch());
	if (OldDefault)
	{
		OldDefault->SetPossibleTarget();
	}

	if (NewDefault)
	{
		NewDefault->SetDefaultTarget();
	}
}
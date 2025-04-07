// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_ModularRig.h"
#include "TLLRN_ModularRigRuleManager.generated.h"

// A management class to validate rules and pattern match
UCLASS(BlueprintType)
class TLLRN_CONTROLRIG_API UTLLRN_ModularRigRuleManager : public UObject
{
public:

	GENERATED_BODY()

	/***
	 * Returns the possible targets for the given connector in the current resolve stage
	 * Note: This method is thread-safe. 
	 * @param InConnector The connector to resolve
	 * @param InModule The module the connector belongs to
	 * @param InResolvedConnectors A redirect map of the already resolved connectors
	 * @return The resolve result including a list of matches
	 */
	FTLLRN_ModularRigResolveResult FindMatches(
		const FTLLRN_RigConnectorElement* InConnector,
		const FTLLRN_RigModuleInstance* InModule,
		const FTLLRN_RigElementKeyRedirector& InResolvedConnectors = FTLLRN_RigElementKeyRedirector()
	) const;

	/***
	 * Returns the possible targets for the given external module connector in the current resolve stage
	 * Note: This method is thread-safe. 
	 * @param InConnector The connector to resolve
	 * @return The resolve result including a list of matches
	 */
	FTLLRN_ModularRigResolveResult FindMatches(
		const FTLLRN_RigModuleConnector* InConnector
	) const;

	/***
	 * Returns the possible targets for the primary connector in the current resolve stage
	 * Note: This method is thread-safe. 
	 * @param InModule The module the connector belongs to
	 * @return The resolve result including a list of matches
	 */
	FTLLRN_ModularRigResolveResult FindMatchesForPrimaryConnector(
		const FTLLRN_RigModuleInstance* InModule
	) const;

	/***
	 * Returns the possible targets for each secondary connector
	 * Note: This method is thread-safe. 
	 * @param InModule The module the secondary connectors belongs to
	 * @param InResolvedConnectors A redirect map of the already resolved connectors
	 * @return The resolve result including a list of matches for each connector
	 */
	TArray<FTLLRN_ModularRigResolveResult> FindMatchesForSecondaryConnectors(
		const FTLLRN_RigModuleInstance* InModule,
		const FTLLRN_RigElementKeyRedirector& InResolvedConnectors = FTLLRN_RigElementKeyRedirector()
	) const;

	/***
	 * Returns the possible targets for each optional connector
	 * Note: This method is thread-safe. 
	 * @param InModule The module the optional connectors belongs to
	 * @param InResolvedConnectors A redirect map of the already resolved connectors
	 * @return The resolve result including a list of matches for each connector
	 */
	TArray<FTLLRN_ModularRigResolveResult> FindMatchesForOptionalConnectors(
		const FTLLRN_RigModuleInstance* InModule,
		const FTLLRN_RigElementKeyRedirector& InResolvedConnectors = FTLLRN_RigElementKeyRedirector()
	) const;

private:

	struct FWorkData
	{
		FWorkData()
		: Hierarchy(nullptr)
		, Connector(nullptr)
		, ModuleConnector(nullptr)
		, Module(nullptr)
		, ResolvedConnectors(nullptr)
		, Result(nullptr)
		{
		}

		void Filter(TFunction<void(FTLLRN_RigElementResolveResult&)> PerMatchFunction);

		const UTLLRN_RigHierarchy* Hierarchy;
		const FTLLRN_RigConnectorElement* Connector;
		const FTLLRN_RigModuleConnector* ModuleConnector;
		const FTLLRN_RigModuleInstance* Module;
		const FTLLRN_RigElementKeyRedirector* ResolvedConnectors;
		FTLLRN_ModularRigResolveResult* Result;
	};

	FTLLRN_ModularRigResolveResult FindMatches(FWorkData& InWorkData) const;

	void SetHierarchy(const UTLLRN_RigHierarchy* InHierarchy);
	static void ResolveConnector(FWorkData& InOutWorkData);
	static void FilterIncompatibleTypes(FWorkData& InOutWorkData);
	static void FilterInvalidNameSpaces(FWorkData& InOutWorkData);
	static void FilterByConnectorRules(FWorkData& InOutWorkData);
	static void FilterByConnectorEvent(FWorkData& InOutWorkData);

	TWeakObjectPtr<const UTLLRN_RigHierarchy> Hierarchy;

	friend class UTLLRN_RigHierarchy;
};
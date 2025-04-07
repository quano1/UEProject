// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_ConnectionCandidates.generated.h"

/**
 * Returns the connection candidates for one connector
 */
USTRUCT(meta=(DisplayName="Get Candidates", Category="Modules", TitleColor="1 0 0", NodeColor="1 1 1", Keywords="Connection,Resolve", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetCandidates : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	// The connector being resolved
	UPROPERTY(EditAnywhere, Transient, Category = "Modules", meta = (Output))
	FTLLRN_RigElementKey Connector;

	// The items being interacted on
	UPROPERTY(EditAnywhere, Transient, Category = "Modules", meta = (Output))
	TArray<FTLLRN_RigElementKey> Candidates;
};

/**
 * Discards matches during a connector event
 */
USTRUCT(meta=(DisplayName="Discard Matches", Category="Modules", TitleColor="1 0 0", NodeColor="1 1 1", Keywords="Connection,Resolve,Match", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DiscardMatches : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	// The items being interacted on
	UPROPERTY(EditAnywhere, Transient, Category = "Modules", meta = (Input))
	TArray<FTLLRN_RigElementKey> Excluded;

	UPROPERTY(EditAnywhere, Transient, Category = "Modules", meta = (Input))
	FString Message;
};

/**
 * Set default match during a connector event
 */
USTRUCT(meta=(DisplayName="Set Default Match", Category="Modules", TitleColor="1 0 0", NodeColor="1 1 1", Keywords="Connection,Resolve,Match,Default", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetDefaultMatch : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	// The items being interacted on
	UPROPERTY(EditAnywhere, Transient, Category = "Modules", meta = (Input))
	FTLLRN_RigElementKey Default;
};


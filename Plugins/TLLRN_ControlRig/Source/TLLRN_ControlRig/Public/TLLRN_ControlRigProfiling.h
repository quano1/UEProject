// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/StatsMisc.h"

struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigStats
{
	bool Enabled;
	TArray<FName> Stack;
	TMap<FName, double> Counters;

	FTLLRN_ControlRigStats()
	: Enabled(false)
	{

	}

	static FTLLRN_ControlRigStats& Get();
	void Clear();
	double& RetainCounter(const TCHAR* Key);
	double& RetainCounter(const FName& Key);
	void ReleaseCounter();
	void Dump();
};

struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigSimpleScopeSecondsCounter : public FSimpleScopeSecondsCounter
{
public:
	FTLLRN_ControlRigSimpleScopeSecondsCounter(const TCHAR* InName);
	~FTLLRN_ControlRigSimpleScopeSecondsCounter();
};

#if STATS
#if WITH_EDITOR
#define TLLRN_CONTROLRIG_SCOPE_SECONDS_COUNTER(Name) \
	FTLLRN_ControlRigSimpleScopeSecondsCounter ANONYMOUS_VARIABLE(TLLRN_ControlRigScopeSecondsCounter)(TEXT(#Name))
#else
#define TLLRN_CONTROLRIG_SCOPE_SECONDS_COUNTER(Name)
#endif
#endif

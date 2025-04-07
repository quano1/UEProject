// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigProfiling.h"

DEFINE_LOG_CATEGORY_STATIC(TLLRN_ControlRigProfilingLog, Log, All);

FTLLRN_ControlRigStats GTLLRN_ControlRigStats;
double GTLLRN_ControlRigStatsCounter = 0;

FTLLRN_ControlRigStats& FTLLRN_ControlRigStats::Get()
{
	return GTLLRN_ControlRigStats;
}

void FTLLRN_ControlRigStats::Clear()
{
	Stack.Reset();
	Counters.Reset();
}

double& FTLLRN_ControlRigStats::RetainCounter(const TCHAR* Key)
{
	FName Name(Key, FNAME_Find);
	if (Name == NAME_None)
	{
		Name = FName(Key, FNAME_Add);
	}
	return RetainCounter(Name);
}

double& FTLLRN_ControlRigStats::RetainCounter(const FName& Key)
{
	if (!Enabled)
	{
		return GTLLRN_ControlRigStatsCounter;
	}
	FName Path = Key;
	if (Stack.Num() > 0)
	{
		Path = FName(*FString::Printf(TEXT("%s.%s"), *Stack.Top().ToString(), *Key.ToString()));
	}
	Stack.Add(Path);
	return GTLLRN_ControlRigStats.Counters.FindOrAdd(Path);
}

void FTLLRN_ControlRigStats::ReleaseCounter()
{
	if (!Enabled)
	{
		return;
	}
	if (Stack.Num() > 0)
	{
		Stack.Pop(EAllowShrinking::No);
	}
}

void FTLLRN_ControlRigStats::Dump()
{
	struct FCounterTree
	{
		FName Name;
		double Seconds;
		TArray<FCounterTree*> Children;

		~FCounterTree()
		{
			for (FCounterTree* Counter : Children)
			{
				delete(Counter);
			}
		}

		int32 GetHash() const
		{
			return INT32_MAX - (int32)(Seconds * 100000.f);
		}

		FCounterTree* FindChild(const FName& InName)
		{
			for (FCounterTree* Child : Children)
			{
				if (Child->Name == InName)
				{
					return Child;
				}
			}
			return nullptr;
		}

		FCounterTree* FindOrAddChild(const FName& InName)
		{
			if (FCounterTree* Child = FindChild(InName))
			{
				return Child;
			}

			FString Path = InName.ToString();
			FString Left, Right;
			if (Path.Split(TEXT("."), &Left, &Right))
			{
				if (FCounterTree* Child = FindChild(*Left))
				{
					return Child->FindOrAddChild(*Right);;
				}
				FCounterTree* Child = new(FCounterTree);
				Child->Seconds = 0.0;
				Children.Add(Child);
				Child->Name = *Left;
				return Child->FindOrAddChild(*Right);
			}

			FCounterTree* Child = new(FCounterTree);
			Child->Seconds = 0.0;
			Children.Add(Child);
			Child->Name = InName;
			return Child;
		}

		void Dump(const FString& Indent = TEXT(""))
		{
			if (Seconds > SMALL_NUMBER)
			{
				UE_LOG(TLLRN_ControlRigProfilingLog, Display, TEXT("%s%08.3f ms -  %s"), *Indent, float(Seconds * 1000.0), *Name.ToString());
			}
			else
			{
				UE_LOG(TLLRN_ControlRigProfilingLog, Display, TEXT("%s%s"), *Indent, *Name.ToString());
			}

			TArray<FCounterTree*> LocalChildren = Children;
			TArray<int32> Order;
			for (FCounterTree* Child : LocalChildren)
			{
				Order.Add(Order.Num());
			}

			Algo::SortBy(Order, [&LocalChildren](int32 Index) -> int32
			{
				return LocalChildren[Index]->GetHash();
			});

			for(int32 Index : Order)
			{
				LocalChildren[Index]->Dump(Indent + TEXT("   "));
			}
		}
	};
	
	FCounterTree Tree;
	Tree.Name = TEXT("TLLRN Control Rig Profiling");
	Tree.Seconds = 0.0;
	for (const TPair<FName, double>& Pair : Counters)
	{
		FCounterTree* Child = Tree.FindOrAddChild(Pair.Key);
		Child->Seconds = Pair.Value;
	}

	Tree.Dump();
}

FTLLRN_ControlRigSimpleScopeSecondsCounter::FTLLRN_ControlRigSimpleScopeSecondsCounter(const TCHAR* InName)
	: FSimpleScopeSecondsCounter(FTLLRN_ControlRigStats::Get().RetainCounter(InName), FTLLRN_ControlRigStats::Get().Enabled)
{
}

FTLLRN_ControlRigSimpleScopeSecondsCounter::~FTLLRN_ControlRigSimpleScopeSecondsCounter()
{
	FTLLRN_ControlRigStats::Get().ReleaseCounter();
}


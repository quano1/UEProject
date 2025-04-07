// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Execution/TLLRN_RigUnit_TLLRN_RigModules.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "TLLRN_ControlRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_TLLRN_RigModules)

FTLLRN_RigUnit_ResolveConnector_Execute()
{
	Result = Connector;

	if (const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy)
	{
		if(const FTLLRN_RigBaseElement* ResolvedElement = Hierarchy->Find(Connector))
		{
			Result = ResolvedElement->GetKey();
			if(SkipSocket && Result.IsValid() && Result.Type == ETLLRN_RigElementType::Socket)
			{
				const FTLLRN_RigElementKey ParentOfSocket = Hierarchy->GetFirstParent(Result);
				if(ParentOfSocket.IsValid())
				{
					Result = ParentOfSocket;
				}
			}
		}
	}

	bIsConnected = Result != Connector;
}

FTLLRN_RigUnit_GetCurrentNameSpace_Execute()
{
	if(!ExecuteContext.IsTLLRN_RigModule())
	{
#if WITH_EDITOR
		
		static const FString Message = TEXT("This node should only be used in a Rig Module."); 
		ExecuteContext.Report(EMessageSeverity::Warning, ExecuteContext.GetFunctionName(), ExecuteContext.GetInstructionIndex(), Message);
		
#endif
	}
	NameSpace = *ExecuteContext.GetTLLRN_RigModuleNameSpace();
}

FTLLRN_RigUnit_GetItemShortName_Execute()
{
	ShortName = NAME_None;

	if(const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy)
	{
		ShortName = *Hierarchy->GetDisplayNameForUI(Item).ToString();
	}

	if(ShortName.IsNone())
	{
		ShortName = Item.Name;
	}
}

FTLLRN_RigUnit_GetItemNameSpace_Execute()
{
	NameSpace.Reset();
	HasNameSpace = false;

	if(const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy)
	{
		const FString NameSpaceForItem = Hierarchy->GetNameSpace(Item);
		if(!NameSpaceForItem.IsEmpty())
		{
			NameSpace = NameSpaceForItem;
			HasNameSpace = true;
		}
	}
}

FTLLRN_RigUnit_IsItemInCurrentNameSpace_Execute()
{
	FString CurrentNameSpace;
	FTLLRN_RigUnit_GetCurrentNameSpace::StaticExecute(ExecuteContext, CurrentNameSpace);
	bool bHasNameSpace = false;
	FString ItemNameSpace;
	FTLLRN_RigUnit_GetItemNameSpace::StaticExecute(ExecuteContext, Item, bHasNameSpace, ItemNameSpace);

	Result = false;
	if(!CurrentNameSpace.IsEmpty() && !ItemNameSpace.IsEmpty())
	{
		Result = ItemNameSpace.Equals(CurrentNameSpace, ESearchCase::IgnoreCase); 
	}
}

FTLLRN_RigUnit_GetItemsInNameSpace_Execute()
{
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (!Hierarchy)
	{
		Items.Reset();
		return;
	}

	FString NameSpace;
	FTLLRN_RigUnit_GetCurrentNameSpace::StaticExecute(ExecuteContext, NameSpace);
	if(NameSpace.IsEmpty())
	{
		Items.Reset();
		return;
	}
	const FName NameSpaceName = *NameSpace;
	
	uint32 Hash = GetTypeHash(StaticStruct());
	Hash = HashCombine(Hash, GetTypeHash((int32)TypeToSearch));
	Hash = HashCombine(Hash, GetTypeHash(NameSpaceName));

	if(const FTLLRN_RigElementKeyCollection* Cache = Hierarchy->FindCachedCollection(Hash))
	{
		Items = Cache->Keys;
	}
	else
	{
		FTLLRN_RigElementKeyCollection Collection;
		Hierarchy->Traverse([Hierarchy, NameSpaceName, &Collection, TypeToSearch]
			(const FTLLRN_RigBaseElement* InElement, bool &bContinue)
			{
				bContinue = true;
						
				const FTLLRN_RigElementKey Key = InElement->GetKey();
				if(((uint8)TypeToSearch & (uint8)Key.Type) == (uint8)Key.Type)
				{
					const FName ItemNameSpace = Hierarchy->GetNameSpaceFName(Key);
					if(!ItemNameSpace.IsNone())
					{
						if(ItemNameSpace.IsEqual(NameSpaceName, ENameCase::IgnoreCase))
						{
							Collection.AddUnique(Key);
						}
					}
				}
			}
		);

		Hierarchy->AddCachedCollection(Hash, Collection);
		Items = Collection.Keys;
	}
}
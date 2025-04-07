// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Execution/TLLRN_RigUnit_Item.h"
#include "RigVMFunctions/RigVMFunction_Name.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "RigVMFunctions/RigVMDispatch_Core.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_Item)

FTLLRN_RigUnit_ItemExists_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	Exists = CachedIndex.UpdateCache(Item, ExecuteContext.Hierarchy);
}

FTLLRN_RigUnit_ItemReplace_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	Result = Item;
	FRigVMFunction_NameReplace::StaticExecute(ExecuteContext, Item.Name, Old, New, Result.Name);
}

FTLLRN_RigUnit_ItemEquals_Execute()
{
	Result = (A == B);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ItemEquals::GetUpgradeInfo() const
{
	return FRigVMStructUpgradeInfo::MakeFromStructToFactory(StaticStruct(), FRigVMDispatch_CoreEquals::StaticStruct());
}

FTLLRN_RigUnit_ItemNotEquals_Execute()
{
	Result = (A != B);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ItemNotEquals::GetUpgradeInfo() const
{
	return FRigVMStructUpgradeInfo::MakeFromStructToFactory(StaticStruct(), FRigVMDispatch_CoreNotEquals::StaticStruct());
}

FTLLRN_RigUnit_ItemTypeEquals_Execute()
{
	Result = (A.Type == B.Type);
}

FTLLRN_RigUnit_ItemTypeNotEquals_Execute()
{
	Result = (A.Type != B.Type);
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_ItemReplace)
{
	Unit.Item.Name = FName(TEXT("OldItemName"));
	Unit.Item.Type = ETLLRN_RigElementType::Bone;

	Unit.Old = FName(TEXT("Old"));
	Unit.New = FName(TEXT("New"));
	
	Execute();
	AddErrorIfFalse(Unit.Result == FTLLRN_RigElementKey(TEXT("NewItemName"), ETLLRN_RigElementType::Bone), TEXT("unexpected result"));

	Unit.Item.Name = FName(TEXT("OldItemName"));
	Unit.Item.Type = ETLLRN_RigElementType::Bone;

	Unit.Old = FName(TEXT("Old"));
	Unit.New = NAME_None;

	Execute();
	AddErrorIfFalse(Unit.Result == FTLLRN_RigElementKey(TEXT("ItemName"), ETLLRN_RigElementType::Bone), TEXT("unexpected result when New is None"));

	Unit.Item.Name = FName(TEXT("OldItemName"));
	Unit.Item.Type = ETLLRN_RigElementType::Bone;

	Unit.Old = NAME_None;
	Unit.New = FName(TEXT("New")); 

	Execute();
	AddErrorIfFalse(Unit.Result == FTLLRN_RigElementKey(TEXT("OldItemName"), ETLLRN_RigElementType::Bone), TEXT("unexpected result when Old is None"));
	return true;
}
#endif

FTLLRN_RigUnit_ItemToName_Execute()
{
	Result = Value.Name;
}
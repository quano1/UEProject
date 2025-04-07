// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/TLLRN_RigUnitContext.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ModularRig.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnitContext)

bool FTLLRN_ControlRigExecuteContext::IsRunningConstructionEvent() const
{
	return GetEventName() == FTLLRN_RigUnit_PrepareForExecution::EventName ||
		GetEventName() == FTLLRN_RigUnit_PostPrepareForExecution::EventName;
}

FName FTLLRN_ControlRigExecuteContext::AddTLLRN_RigModuleNameSpace(const FName& InName) const
{
	if(IsTLLRN_RigModule())
	{
		return InName;
	}
	return *AddTLLRN_RigModuleNameSpace(InName.ToString());
}

FString FTLLRN_ControlRigExecuteContext::AddTLLRN_RigModuleNameSpace(const FString& InName) const
{
	if(IsTLLRN_RigModule())
	{
		return InName;
	}
	check(!TLLRN_RigModuleNameSpace.IsEmpty());
	return TLLRN_RigModuleNameSpace + InName;
}

FName FTLLRN_ControlRigExecuteContext::RemoveTLLRN_RigModuleNameSpace(const FName& InName) const
{
	if(IsTLLRN_RigModule())
	{
		return InName;
	}
	return *RemoveTLLRN_RigModuleNameSpace(InName.ToString());
}

FString FTLLRN_ControlRigExecuteContext::RemoveTLLRN_RigModuleNameSpace(const FString& InName) const
{
	if(IsTLLRN_RigModule())
	{
		return InName;
	}
	check(!TLLRN_RigModuleNameSpace.IsEmpty());

	if(InName.StartsWith(TLLRN_RigModuleNameSpace, ESearchCase::CaseSensitive))
	{
		return InName.Mid(TLLRN_RigModuleNameSpace.Len());
	}
	return InName;
}

FString FTLLRN_ControlRigExecuteContext::GetElementNameSpace(ETLLRN_RigMetaDataNameSpace InNameSpaceType) const
{
	if(IsTLLRN_RigModule())
	{
		// prefix the meta data name with the namespace to allow modules to store their
		// metadata in a way that doesn't collide with other modules' metadata.
		switch(InNameSpaceType)
		{
			case ETLLRN_RigMetaDataNameSpace::Self:
			{
				return GetTLLRN_RigModuleNameSpace();
			}
			case ETLLRN_RigMetaDataNameSpace::Parent:
			{
				check(GetTLLRN_RigModuleNameSpace().EndsWith(UTLLRN_ModularRig::NamespaceSeparator));
				FString ParentNameSpace;
				UTLLRN_RigHierarchy::SplitNameSpace(GetTLLRN_RigModuleNameSpace().LeftChop(1), &ParentNameSpace, nullptr, true);
				if(ParentNameSpace.IsEmpty())
				{
					return GetTLLRN_RigModuleNameSpace();
				}
				return ParentNameSpace + UTLLRN_ModularRig::NamespaceSeparator;
			}
			case ETLLRN_RigMetaDataNameSpace::Root:
			{
				FString RootNameSpace;
				UTLLRN_RigHierarchy::SplitNameSpace(GetTLLRN_RigModuleNameSpace(), &RootNameSpace, nullptr, false);
				if(RootNameSpace.IsEmpty())
				{
					return GetTLLRN_RigModuleNameSpace();
				}
				return RootNameSpace + UTLLRN_ModularRig::NamespaceSeparator;
			}
			default:
			{
				break;
			}
		}
	}
	else
	{
		// prefix the meta data with some mockup namespaces
		// so we can test this even without a module present.
		switch(InNameSpaceType)
		{
			case ETLLRN_RigMetaDataNameSpace::Self:
			{
				// if we are storing on self and this is not a modular
				// rig let's just not use a namespace.
				break;
			}
			case ETLLRN_RigMetaDataNameSpace::Parent:
			{
				static const FString ParentNameSpace = TEXT("Parent:");
				return ParentNameSpace;
			}
			case ETLLRN_RigMetaDataNameSpace::Root:
			{
				static const FString RootNameSpace = TEXT("Root:");
				return RootNameSpace;
			}
			default:
			{
				break;
			}
		}
	}

	return FString();
}

const FTLLRN_RigModuleInstance* FTLLRN_ControlRigExecuteContext::GetTLLRN_RigModuleInstance(ETLLRN_RigMetaDataNameSpace InNameSpaceType) const
{
	if(TLLRN_RigModuleInstance)
	{
		switch(InNameSpaceType)
		{
			case ETLLRN_RigMetaDataNameSpace::Self:
			{
				return TLLRN_RigModuleInstance;
			}
			case ETLLRN_RigMetaDataNameSpace::Parent:
			{
				return TLLRN_RigModuleInstance->GetParentModule();
			}
			case ETLLRN_RigMetaDataNameSpace::Root:
			{
				return TLLRN_RigModuleInstance->GetRootModule();
			}
			case ETLLRN_RigMetaDataNameSpace::None:
			default:
			{
				break;
			}
		}
		
	}
	return nullptr;
}

FName FTLLRN_ControlRigExecuteContext::AdaptMetadataName(ETLLRN_RigMetaDataNameSpace InNameSpaceType, const FName& InMetadataName) const
{
	// only if we are within a rig module let's adapt the meta data name
	const bool bUseNameSpace = InNameSpaceType != ETLLRN_RigMetaDataNameSpace::None;
	if(bUseNameSpace && !InMetadataName.IsNone())
	{
		// if the metadata name already contains a namespace - we are just going
		// to use it as is. this means that modules have access to other module's metadata,
		// and that's ok. the user will require the full path to it anyway so it is a
		// conscious user decision.
		const FString MetadataNameString = InMetadataName.ToString();
		int32 Index = INDEX_NONE;
		if(MetadataNameString.FindChar(TEXT(':'), Index))
		{
			return InMetadataName;
		}

		if(IsTLLRN_RigModule())
		{
			// prefix the meta data name with the namespace to allow modules to store their
			// metadata in a way that doesn't collide with other modules' metadata.
			switch(InNameSpaceType)
			{
				case ETLLRN_RigMetaDataNameSpace::Self:
				case ETLLRN_RigMetaDataNameSpace::Parent:
				case ETLLRN_RigMetaDataNameSpace::Root:
				{
					return *UTLLRN_RigHierarchy::JoinNameSpace(GetElementNameSpace(InNameSpaceType), MetadataNameString);
				}
				default:
				{
					break;
				}
			}
		}
		else
		{
			// prefix the meta data with some mockup namespaces
			// so we can test this even without a module present.
			switch(InNameSpaceType)
			{
				case ETLLRN_RigMetaDataNameSpace::Self:
				{
					// if we are storing on self and this is not a modular
					// rig let's just not use a namespace.
					break;
				}
				case ETLLRN_RigMetaDataNameSpace::Parent:
				case ETLLRN_RigMetaDataNameSpace::Root:
				{
					return *UTLLRN_RigHierarchy::JoinNameSpace(GetElementNameSpace(InNameSpaceType), MetadataNameString);
				}
				default:
				{
					break;
				}
			}
		}
	}
	return InMetadataName;
}

FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard::FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard(FTLLRN_ControlRigExecuteContext& InContext, const UTLLRN_ControlRig* InTLLRN_ControlRig)
	: Context(InContext)
	, PreviousTLLRN_RigModuleNameSpace(InContext.TLLRN_RigModuleNameSpace)
	, PreviousTLLRN_RigModuleNameSpaceHash(InContext.TLLRN_RigModuleNameSpaceHash)
{
	Context.TLLRN_RigModuleNameSpace = InTLLRN_ControlRig->GetTLLRN_RigModuleNameSpace();
	Context.TLLRN_RigModuleNameSpaceHash = GetTypeHash(Context.TLLRN_RigModuleNameSpace);
}

FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard::FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard(FTLLRN_ControlRigExecuteContext& InContext, const FString& InNewModuleNameSpace)
	: Context(InContext)
	, PreviousTLLRN_RigModuleNameSpace(InContext.TLLRN_RigModuleNameSpace)
	, PreviousTLLRN_RigModuleNameSpaceHash(InContext.TLLRN_RigModuleNameSpaceHash)
{
	Context.TLLRN_RigModuleNameSpace = InNewModuleNameSpace;
	Context.TLLRN_RigModuleNameSpaceHash = GetTypeHash(Context.TLLRN_RigModuleNameSpace);
}

FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard::~FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard()
{
	Context.TLLRN_RigModuleNameSpace = PreviousTLLRN_RigModuleNameSpace;
	Context.TLLRN_RigModuleNameSpaceHash = PreviousTLLRN_RigModuleNameSpaceHash; 
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_ControlChannel.h"
#include "Units/Hierarchy/TLLRN_RigUnit_ControlChannelFromItem.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_ControlChannel)

bool FTLLRN_RigUnit_GetAnimationChannelBase::UpdateCache(const UTLLRN_RigHierarchy* InHierarchy, const FName& Control, const FName& Channel, FTLLRN_RigElementKey& Key, int32& Hash)
{
	if (!IsValid(InHierarchy))
	{
		return false;
	}
	
	if(!Key.IsValid())
	{
		Hash = INDEX_NONE;
	}

	const int32 ExpectedHash = (int32)HashCombine(GetTypeHash(InHierarchy->GetTopologyVersion()), HashCombine(GetTypeHash(Control), GetTypeHash(Channel)));
	if(ExpectedHash != Hash)
	{
		if(const FTLLRN_RigControlElement* ControlElement = InHierarchy->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control)))
		{
			FString Namespace, ChannelName = Channel.ToString();
			UTLLRN_RigHierarchy::SplitNameSpace(ChannelName, &Namespace, &ChannelName);
			
			for(const FTLLRN_RigBaseElement* Child : InHierarchy->GetChildren(ControlElement))
			{
				if(const FTLLRN_RigControlElement* ChildControl = Cast<FTLLRN_RigControlElement>(Child))
				{
					if(ChildControl->IsAnimationChannel())
					{
						if(ChildControl->GetDisplayName().ToString().Equals(ChannelName))
						{
							Key = ChildControl->GetKey();
							Hash = ExpectedHash;
							return true;
						}
					}
				}
			}
		}

		return false;
	}

	return true;
}

FTLLRN_RigUnit_GetBoolAnimationChannel_Execute()
{
	Value = false;
	
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_GetBoolAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_GetFloatAnimationChannel_Execute()
{
	Value = 0.f;
	
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_GetFloatAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_GetIntAnimationChannel_Execute()
{
	Value = 0;
	
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_GetIntAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_GetVector2DAnimationChannel_Execute()
{
	Value = FVector2D::ZeroVector;
	
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_GetVector2DAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_GetVectorAnimationChannel_Execute()
{
	Value = FVector::ZeroVector;
	
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_GetVectorAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_GetRotatorAnimationChannel_Execute()
{
	Value = FRotator::ZeroRotator;
	
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_GetRotatorAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_GetTransformAnimationChannel_Execute()
{
	Value = FTransform::Identity;
	
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_GetTransformAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_SetBoolAnimationChannel_Execute()
{
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_SetBoolAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_SetFloatAnimationChannel_Execute()
{
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_SetFloatAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_SetIntAnimationChannel_Execute()
{
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_SetIntAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_SetVector2DAnimationChannel_Execute()
{
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_SetVector2DAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_SetVectorAnimationChannel_Execute()
{
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_SetVectorAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_SetRotatorAnimationChannel_Execute()
{
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_SetRotatorAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

FTLLRN_RigUnit_SetTransformAnimationChannel_Execute()
{
	if(!UpdateCache(ExecuteContext.Hierarchy, Control, Channel, CachedChannelKey, CachedChannelHash))
	{
		return;
	}
	FTLLRN_RigUnit_SetTransformAnimationChannelFromItem::StaticExecute(ExecuteContext, Value, CachedChannelKey, bInitial);
}

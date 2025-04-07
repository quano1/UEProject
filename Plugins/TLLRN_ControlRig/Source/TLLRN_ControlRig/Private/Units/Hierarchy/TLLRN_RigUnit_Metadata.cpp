// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_Metadata.h"

#include "TLLRN_ModularRig.h"
#include "RigVMTypeUtils.h"
#include "RigVMCore/RigVMStruct.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_Metadata)

#if WITH_EDITOR
#include "RigVMModel/RigVMPin.h"
#endif

#if WITH_EDITOR

FString FTLLRN_RigDispatch_MetadataBase::GetNodeTitle(const FRigVMTemplateTypeMap& InTypes) const
{
	if(const TRigVMTypeIndex* ValueTypeIndexPtr = InTypes.Find(ValueArgName))
	{
		const TRigVMTypeIndex& ValueTypeIndex = *ValueTypeIndexPtr;
		if(ValueTypeIndex != RigVMTypeUtils::TypeIndex::WildCard &&
			ValueTypeIndex != RigVMTypeUtils::TypeIndex::WildCardArray)
		{
			static constexpr TCHAR GetMetadataFormat[] = TEXT("Get %s%s Metadata");
			static constexpr TCHAR SetMetadataFormat[] = TEXT("Set %s%s Metadata");

			const FRigVMTemplateArgumentType& ValueType = FRigVMRegistry::Get().GetType(ValueTypeIndex);
			FString ValueName;
			if(ValueType.CPPTypeObject == FTLLRN_RigElementKey::StaticStruct())
			{
				ValueName = ItemArgName.ToString();
			}
			else if(ValueType.CPPTypeObject)
			{
				ValueName = ValueType.CPPTypeObject->GetName();
			}
			else if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FName)
			{
				ValueName = NameArgName.ToString();
			}
			else
			{
				ValueName = ValueType.GetBaseCPPType();
				ValueName = ValueName.Left(1).ToUpper() + ValueName.Mid(1);
			}

			if(ValueType.IsArray())
			{
				ValueName += TEXT(" Array");
			}

			return FString::Printf(IsSetMetadata() ? SetMetadataFormat : GetMetadataFormat, *GetNodeTitlePrefix(), *ValueName); 
		}
	}
	return FTLLRN_RigDispatchFactory::GetNodeTitle(InTypes);
}

#endif

const TArray<FRigVMTemplateArgumentInfo>& FTLLRN_RigDispatch_MetadataBase::GetArgumentInfos() const
{
	if(Infos.IsEmpty())
	{
		const FRigVMRegistry_NoLock& Registry = FRigVMRegistry_NoLock::GetForRead();
		ItemArgIndex = Infos.Emplace(ItemArgName, ERigVMPinDirection::Input, Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>());
		NameArgIndex = Infos.Emplace(NameArgName, ERigVMPinDirection::Input, RigVMTypeUtils::TypeIndex::FName);
		NameSpaceArgIndex = Infos.Emplace(NameSpaceArgName, ERigVMPinDirection::Input, Registry.GetTypeIndex_NoLock<ETLLRN_RigMetaDataNameSpace>());
		CacheArgIndex = Infos.Emplace(CacheArgName, ERigVMPinDirection::Hidden, Registry.GetTypeIndex_NoLock<FTLLRN_CachedTLLRN_RigElement>());
	};
	return Infos;
}

#if WITH_EDITOR

FText FTLLRN_RigDispatch_MetadataBase::GetArgumentTooltip(const FName& InArgumentName, TRigVMTypeIndex InTypeIndex) const
{
	if(InArgumentName == ItemArgName)
	{
		return NSLOCTEXT("FTLLRN_RigDispatch_MetadataBase", "ItemArgTooltip", "The item storing the metadata");
	}
	if(InArgumentName == NameArgName)
	{
		return NSLOCTEXT("FTLLRN_RigDispatch_MetadataBase", "NameArgTooltip", "The name of the metadata");
	}
	if(InArgumentName == NameSpaceArgName)
	{
		return NSLOCTEXT("FTLLRN_RigDispatch_MetadataBase", "NameSpaceArgTooltip", "Defines in which namespace the metadata will be looked up");
	}
	if(InArgumentName == DefaultArgName)
	{
		return NSLOCTEXT("FTLLRN_RigDispatch_MetadataBase", "DefaultArgTooltip", "The default value used as a fallback if the metadata does not exist");
	}
	if(InArgumentName == ValueArgName)
	{
		return NSLOCTEXT("FTLLRN_RigDispatch_MetadataBase", "ValueArgTooltip", "The value to get / set");
	}
	if(InArgumentName == FoundArgName)
	{
		return NSLOCTEXT("FTLLRN_RigDispatch_MetadataBase", "FoundArgTooltip", "Returns true if the metadata exists with the specific type");
	}
	if(InArgumentName == SuccessArgName)
	{
		return NSLOCTEXT("FTLLRN_RigDispatch_MetadataBase", "SuccessArgTooltip", "Returns true if the metadata was successfully stored");
	}
	return FTLLRN_RigDispatchFactory::GetArgumentTooltip(InArgumentName, InTypeIndex);
}

FString FTLLRN_RigDispatch_MetadataBase::GetArgumentDefaultValue(const FName& InArgumentName,
	TRigVMTypeIndex InTypeIndex) const
{
	if(InArgumentName == NameSpaceArgName)
	{
		static const FString SelfString = StaticEnum<ETLLRN_RigMetaDataNameSpace>()->GetDisplayNameTextByValue((int64)ETLLRN_RigMetaDataNameSpace::Self).ToString();
		return SelfString;
	}
	return FTLLRN_RigDispatchFactory::GetArgumentDefaultValue(InArgumentName, InTypeIndex);
}

FString FTLLRN_RigDispatch_MetadataBase::GetArgumentMetaData(const FName& InArgumentName, const FName& InMetaDataKey) const
{
	if(InArgumentName == NameArgName)
	{
		if(InMetaDataKey == FRigVMStruct::CustomWidgetMetaName)
		{
			return TEXT("MetadataName");
		}
	}
	return Super::GetArgumentMetaData(InArgumentName, InMetaDataKey);
}

#endif

const TArray<TRigVMTypeIndex>& FTLLRN_RigDispatch_MetadataBase::GetValueTypes() const
{
	static TArray<TRigVMTypeIndex> Types;
	if(Types.IsEmpty())
	{
		const FRigVMRegistry_NoLock& Registry = FRigVMRegistry_NoLock::GetForRead();
		Types = {
			RigVMTypeUtils::TypeIndex::Bool,
			RigVMTypeUtils::TypeIndex::Float,
			RigVMTypeUtils::TypeIndex::Int32,
			RigVMTypeUtils::TypeIndex::FName,
			Registry.GetTypeIndex_NoLock<FVector>(false),
			Registry.GetTypeIndex_NoLock<FRotator>(false),
			Registry.GetTypeIndex_NoLock<FQuat>(false),
			Registry.GetTypeIndex_NoLock<FTransform>(false),
			Registry.GetTypeIndex_NoLock<FLinearColor>(false),
			Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>(false),
			RigVMTypeUtils::TypeIndex::BoolArray,
			RigVMTypeUtils::TypeIndex::FloatArray,
			RigVMTypeUtils::TypeIndex::Int32Array,
			RigVMTypeUtils::TypeIndex::FNameArray,
			Registry.GetTypeIndex_NoLock<FVector>(true),
			Registry.GetTypeIndex_NoLock<FRotator>(true),
			Registry.GetTypeIndex_NoLock<FQuat>(true),
			Registry.GetTypeIndex_NoLock<FTransform>(true),
			Registry.GetTypeIndex_NoLock<FLinearColor>(true),
			Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>(true)
		};
	}
	return Types;
}

////////////////////////////////////////////////////////////////////////////////////////////////

const TArray<FRigVMTemplateArgumentInfo>& FTLLRN_RigDispatch_GetMetadata::GetArgumentInfos() const
{
	if(ValueArgIndex == INDEX_NONE)
	{
		Infos = Super::GetArgumentInfos(); 
		DefaultArgIndex = Infos.Emplace(DefaultArgName, ERigVMPinDirection::Input, GetValueTypes());
		ValueArgIndex = Infos.Emplace(ValueArgName, ERigVMPinDirection::Output, GetValueTypes());
		FoundArgIndex = Infos.Emplace(FoundArgName, ERigVMPinDirection::Output, RigVMTypeUtils::TypeIndex::Bool);
	};
	return Infos;
}

FTLLRN_RigBaseMetadata* FTLLRN_RigDispatch_GetMetadata::FindMetadata(const FRigVMExtendedExecuteContext& InContext,
                                                         const FTLLRN_RigElementKey& InKey, const FName& InName,
                                                         ETLLRN_RigMetadataType InType, ETLLRN_RigMetaDataNameSpace InNameSpace, FTLLRN_CachedTLLRN_RigElement& Cache)
{
	const FTLLRN_ControlRigExecuteContext& ExecuteContext = InContext.GetPublicData<FTLLRN_ControlRigExecuteContext>();
	if(Cache.UpdateCache(InKey, ExecuteContext.Hierarchy))
	{
		if(FTLLRN_RigBaseElement* Element = ExecuteContext.Hierarchy->Get(Cache.GetIndex()))
		{
			// first try to find the metadata in the namespace
			const FName Name = ExecuteContext.AdaptMetadataName(InNameSpace, InName);
			return ExecuteContext.Hierarchy->FindMetadataForElement(Element, Name, InType);
		};
	}
	return nullptr;
}

FRigVMFunctionPtr FTLLRN_RigDispatch_GetMetadata::GetDispatchFunctionImpl(const FRigVMTemplateTypeMap& InTypes) const
{
	const FRigVMRegistry_NoLock& Registry = FRigVMRegistry_NoLock::GetForRead();
	const TRigVMTypeIndex& ValueTypeIndex = InTypes.FindChecked(TEXT("Value"));
	
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Bool)
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<bool, FTLLRN_RigBoolMetadata, ETLLRN_RigMetadataType::Bool>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Float)
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<float, FTLLRN_RigFloatMetadata, ETLLRN_RigMetadataType::Float>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Int32)
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<int32, FTLLRN_RigInt32Metadata, ETLLRN_RigMetadataType::Int32>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FName)
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<FName, FTLLRN_RigNameMetadata, ETLLRN_RigMetadataType::Name>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FVector>(false))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<FVector, FTLLRN_RigVectorMetadata, ETLLRN_RigMetadataType::Vector>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FRotator>(false))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<FRotator, FTLLRN_RigRotatorMetadata, ETLLRN_RigMetadataType::Rotator>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FQuat>(false))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<FQuat, FTLLRN_RigQuatMetadata, ETLLRN_RigMetadataType::Quat>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTransform>(false))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<FTransform, FTLLRN_RigTransformMetadata, ETLLRN_RigMetadataType::Transform>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FLinearColor>(false))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<FLinearColor, FTLLRN_RigLinearColorMetadata, ETLLRN_RigMetadataType::LinearColor>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>(false))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<FTLLRN_RigElementKey, FTLLRN_RigElementKeyMetadata, ETLLRN_RigMetadataType::TLLRN_RigElementKey>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::BoolArray)
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<TArray<bool>, FTLLRN_RigBoolArrayMetadata, ETLLRN_RigMetadataType::BoolArray>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FloatArray)
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<TArray<float>, FTLLRN_RigFloatArrayMetadata, ETLLRN_RigMetadataType::FloatArray>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Int32Array)
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<TArray<int32>, FTLLRN_RigInt32ArrayMetadata, ETLLRN_RigMetadataType::Int32Array>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FNameArray)
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<TArray<FName>, FTLLRN_RigNameArrayMetadata, ETLLRN_RigMetadataType::NameArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FVector>(true))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<TArray<FVector>, FTLLRN_RigVectorArrayMetadata, ETLLRN_RigMetadataType::VectorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FRotator>(true))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<TArray<FRotator>, FTLLRN_RigRotatorArrayMetadata, ETLLRN_RigMetadataType::RotatorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FQuat>(true))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<TArray<FQuat>, FTLLRN_RigQuatArrayMetadata, ETLLRN_RigMetadataType::QuatArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTransform>(true))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<TArray<FTransform>, FTLLRN_RigTransformArrayMetadata, ETLLRN_RigMetadataType::TransformArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FLinearColor>(true))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<TArray<FLinearColor>, FTLLRN_RigLinearColorArrayMetadata, ETLLRN_RigMetadataType::LinearColorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>(true))
	{
		return &FTLLRN_RigDispatch_GetMetadata::GetMetadataDispatch<TArray<FTLLRN_RigElementKey>, FTLLRN_RigElementKeyArrayMetadata, ETLLRN_RigMetadataType::TLLRN_RigElementKeyArray>;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////

const TArray<FRigVMTemplateArgumentInfo>& FTLLRN_RigDispatch_SetMetadata::GetArgumentInfos() const
{
	if(ValueArgIndex == INDEX_NONE)
	{
		Infos = Super::GetArgumentInfos(); 
		ValueArgIndex = Infos.Emplace(ValueArgName, ERigVMPinDirection::Input, GetValueTypes());
		SuccessArgIndex = Infos.Emplace(SuccessArgName, ERigVMPinDirection::Output, RigVMTypeUtils::TypeIndex::Bool);
	};
	return Infos;
}

const TArray<FRigVMExecuteArgument>& FTLLRN_RigDispatch_SetMetadata::GetExecuteArguments_Impl(const FRigVMDispatchContext& InContext) const
{
	static const TArray<FRigVMExecuteArgument> ExecuteArguments = {
		{TEXT("ExecuteContext"), ERigVMPinDirection::IO}
	};
	return ExecuteArguments;
}

FTLLRN_RigBaseMetadata* FTLLRN_RigDispatch_SetMetadata::FindOrAddMetadata(const FTLLRN_ControlRigExecuteContext& InContext,
                                                              const FTLLRN_RigElementKey& InKey, const FName& InName, ETLLRN_RigMetadataType InType,
                                                              ETLLRN_RigMetaDataNameSpace NameSpace, FTLLRN_CachedTLLRN_RigElement& Cache)
{
	if(Cache.UpdateCache(InKey, InContext.Hierarchy))
	{
		if(FTLLRN_RigBaseElement* Element = InContext.Hierarchy->Get(Cache.GetIndex()))
		{
			const FName Name = InContext.AdaptMetadataName(NameSpace, InName);
			constexpr bool bNotify = true;
			return InContext.Hierarchy->GetMetadataForElement(Element, Name, InType, bNotify);
		}
	}
	return nullptr;
}

FRigVMFunctionPtr FTLLRN_RigDispatch_SetMetadata::GetDispatchFunctionImpl(const FRigVMTemplateTypeMap& InTypes) const
{
	const FRigVMRegistry_NoLock& Registry = FRigVMRegistry_NoLock::GetForRead();
	const TRigVMTypeIndex& ValueTypeIndex = InTypes.FindChecked(TEXT("Value"));
	
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Bool)
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<bool, FTLLRN_RigBoolMetadata, ETLLRN_RigMetadataType::Bool>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Float)
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<float, FTLLRN_RigFloatMetadata, ETLLRN_RigMetadataType::Float>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Int32)
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<int32, FTLLRN_RigInt32Metadata, ETLLRN_RigMetadataType::Int32>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FName)
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<FName, FTLLRN_RigNameMetadata, ETLLRN_RigMetadataType::Name>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FVector>(false))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<FVector, FTLLRN_RigVectorMetadata, ETLLRN_RigMetadataType::Vector>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FRotator>(false))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<FRotator, FTLLRN_RigRotatorMetadata, ETLLRN_RigMetadataType::Rotator>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FQuat>(false))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<FQuat, FTLLRN_RigQuatMetadata, ETLLRN_RigMetadataType::Quat>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTransform>(false))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<FTransform, FTLLRN_RigTransformMetadata, ETLLRN_RigMetadataType::Transform>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FLinearColor>(false))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<FLinearColor, FTLLRN_RigLinearColorMetadata, ETLLRN_RigMetadataType::LinearColor>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>(false))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<FTLLRN_RigElementKey, FTLLRN_RigElementKeyMetadata, ETLLRN_RigMetadataType::TLLRN_RigElementKey>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::BoolArray)
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<TArray<bool>, FTLLRN_RigBoolArrayMetadata, ETLLRN_RigMetadataType::BoolArray>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FloatArray)
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<TArray<float>, FTLLRN_RigFloatArrayMetadata, ETLLRN_RigMetadataType::FloatArray>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Int32Array)
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<TArray<int32>, FTLLRN_RigInt32ArrayMetadata, ETLLRN_RigMetadataType::Int32Array>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FNameArray)
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<TArray<FName>, FTLLRN_RigNameArrayMetadata, ETLLRN_RigMetadataType::NameArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FVector>(true))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<TArray<FVector>, FTLLRN_RigVectorArrayMetadata, ETLLRN_RigMetadataType::VectorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FRotator>(true))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<TArray<FRotator>, FTLLRN_RigRotatorArrayMetadata, ETLLRN_RigMetadataType::RotatorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FQuat>(true))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<TArray<FQuat>, FTLLRN_RigQuatArrayMetadata, ETLLRN_RigMetadataType::QuatArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTransform>(true))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<TArray<FTransform>, FTLLRN_RigTransformArrayMetadata, ETLLRN_RigMetadataType::TransformArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FLinearColor>(true))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<TArray<FLinearColor>, FTLLRN_RigLinearColorArrayMetadata, ETLLRN_RigMetadataType::LinearColorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>(true))
	{
		return &FTLLRN_RigDispatch_SetMetadata::SetMetadataDispatch<TArray<FTLLRN_RigElementKey>, FTLLRN_RigElementKeyArrayMetadata, ETLLRN_RigMetadataType::TLLRN_RigElementKeyArray>;
	}

	return nullptr;
}

FTLLRN_RigUnit_RemoveMetadata_Execute()
{
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;

	Removed = false;
	if (!Hierarchy)
	{
		return;
	}

	if(CachedIndex.UpdateCache(Item, Hierarchy))
	{
		if(FTLLRN_RigBaseElement* Element = Hierarchy->Get(CachedIndex))
		{
			const FName LocalName = ExecuteContext.AdaptMetadataName(NameSpace, Name);
			Removed = Element->RemoveMetadata(LocalName);
			if(!Removed)
			{
				Removed = Element->RemoveMetadata(Name);
			}
		}
	}
}

FTLLRN_RigUnit_RemoveAllMetadata_Execute()
{
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;

	Removed = false;
	if (!Hierarchy)
	{
		return;
	}

	if(CachedIndex.UpdateCache(Item, Hierarchy))
	{
		if(FTLLRN_RigBaseElement* Element = Hierarchy->Get(CachedIndex))
		{
			if(NameSpace != ETLLRN_RigMetaDataNameSpace::None)
			{
				// only remove the metadata within this module / with this namespace
				const TArray<FName> MetadataNames = Hierarchy->GetMetadataNames(Element->GetKey());

				Removed = false;
				const FString NameSpacePrefix = ExecuteContext.GetElementNameSpace(NameSpace);
				for(const FName& MetadataName : MetadataNames)
				{
					if(MetadataName.ToString().StartsWith(NameSpacePrefix, ESearchCase::CaseSensitive))
					{
						if(Element->RemoveMetadata(MetadataName))
						{
							Removed = true;
						}
					}
				}
				return;
			}
			Removed = Element->RemoveAllMetadata();
		}
	}
}

FTLLRN_RigUnit_HasMetadata_Execute()
{
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;

	Found = false;
	if (!Hierarchy)
	{
		return;
	}

	if(CachedIndex.UpdateCache(Item, Hierarchy))
	{
		if(const FTLLRN_RigBaseElement* Element = Hierarchy->Get(CachedIndex))
		{
			const FName LocalName = ExecuteContext.AdaptMetadataName(NameSpace, Name);
			Found = Element->GetMetadata(LocalName, Type) != nullptr;
			if(!Found)
			{
				Found = Element->GetMetadata(Name, Type) != nullptr;
			}
		}
	}
}

FTLLRN_RigUnit_FindItemsWithMetadata_Execute()
{
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;

	Items.Reset();
	if (!Hierarchy)
	{
		return;
	}

	const FName LocalName = ExecuteContext.AdaptMetadataName(NameSpace, Name);
	Hierarchy->Traverse([&Items, Name, LocalName, Type](const FTLLRN_RigBaseElement* Element, bool& bContinue)
	{
		if(Element->GetMetadata(LocalName, Type) != nullptr)
		{
			Items.AddUnique(Element->GetKey());
		}
		bContinue = true;
	});
}

FTLLRN_RigUnit_GetMetadataTags_Execute()
{
	Tags.Reset();
	
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (!Hierarchy)
	{
		return;
	}

	if(CachedIndex.UpdateCache(Item, Hierarchy))
	{
		if(const FTLLRN_RigBaseElement* Element = Hierarchy->Get(CachedIndex))
		{
			if(const FTLLRN_RigNameArrayMetadata* Md = Cast<FTLLRN_RigNameArrayMetadata>(Element->GetMetadata(UTLLRN_RigHierarchy::TagMetadataName, ETLLRN_RigMetadataType::NameArray)))
			{
				Tags = Md->GetValue();
			}
		}
	}
}

FTLLRN_RigUnit_SetMetadataTag_Execute()
{
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (!Hierarchy)
	{
		return;
	}

	if(CachedIndex.UpdateCache(Item, Hierarchy))
	{
		if(FTLLRN_RigBaseElement* Element = Hierarchy->Get(CachedIndex))
		{
			if(FTLLRN_RigNameArrayMetadata* Md = Cast<FTLLRN_RigNameArrayMetadata>(Element->SetupValidMetadata(UTLLRN_RigHierarchy::TagMetadataName, ETLLRN_RigMetadataType::NameArray)))
			{
				const int32 LastIndex = Md->GetValue().Num(); 
				const FName LocalTag = ExecuteContext.AdaptMetadataName(NameSpace, Tag);
				if(Md->GetValue().AddUnique(LocalTag) == LastIndex)
				{
					Element->NotifyMetadataTagChanged(LocalTag, true);
				}
			}
		}
	}
}

FTLLRN_RigUnit_SetMetadataTagArray_Execute()
{
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (!Hierarchy)
	{
		return;
	}

	if(CachedIndex.UpdateCache(Item, Hierarchy))
	{
		if(FTLLRN_RigBaseElement* Element = Hierarchy->Get(CachedIndex))
		{
			if(FTLLRN_RigNameArrayMetadata* Md = Cast<FTLLRN_RigNameArrayMetadata>(Element->SetupValidMetadata(UTLLRN_RigHierarchy::TagMetadataName, ETLLRN_RigMetadataType::NameArray)))
			{
				for(const FName& Tag : Tags)
				{
					const int32 LastIndex = Md->GetValue().Num(); 
					const FName LocalTag = ExecuteContext.AdaptMetadataName(NameSpace, Tag);
					if(Md->GetValue().AddUnique(LocalTag) == LastIndex)
					{
						Element->NotifyMetadataTagChanged(LocalTag, true);
					}
				}
			}
		}
	}
}

FTLLRN_RigUnit_RemoveMetadataTag_Execute()
{
	Removed = false;
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (!Hierarchy)
	{
		return;
	}

	if(CachedIndex.UpdateCache(Item, Hierarchy))
	{
		if(FTLLRN_RigBaseElement* Element = Hierarchy->Get(CachedIndex))
		{
			if(FTLLRN_RigNameArrayMetadata* Md = Cast<FTLLRN_RigNameArrayMetadata>(Element->GetMetadata(UTLLRN_RigHierarchy::TagMetadataName, ETLLRN_RigMetadataType::NameArray)))
			{
				const FName LocalTag = ExecuteContext.AdaptMetadataName(NameSpace, Tag);
				Removed = Md->GetValue().Remove(LocalTag) > 0;
				if(Removed)
				{
					Element->NotifyMetadataTagChanged(LocalTag, false);
				}
			}
		}
	}
}

FTLLRN_RigUnit_HasMetadataTag_Execute()
{
	Found = false;
	
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (!Hierarchy)
	{
		return;
	}

	if(CachedIndex.UpdateCache(Item, Hierarchy))
	{
		if(const FTLLRN_RigBaseElement* Element = Hierarchy->Get(CachedIndex))
		{
			if(const FTLLRN_RigNameArrayMetadata* Md = Cast<FTLLRN_RigNameArrayMetadata>(Element->GetMetadata(UTLLRN_RigHierarchy::TagMetadataName, ETLLRN_RigMetadataType::NameArray)))
			{
				const FName LocalTag = ExecuteContext.AdaptMetadataName(NameSpace, Tag);
				Found = Md->GetValue().Contains(LocalTag);
			}
		}
	}
}

FTLLRN_RigUnit_HasMetadataTagArray_Execute()
{
	Found = false;
	
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (!Hierarchy)
	{
		return;
	}

	if(CachedIndex.UpdateCache(Item, Hierarchy))
	{
		if(const FTLLRN_RigBaseElement* Element = Hierarchy->Get(CachedIndex))
		{
			if(const FTLLRN_RigNameArrayMetadata* Md = Cast<FTLLRN_RigNameArrayMetadata>(Element->GetMetadata(UTLLRN_RigHierarchy::TagMetadataName, ETLLRN_RigMetadataType::NameArray)))
			{
				Found = true;
				for(const FName& Tag : Tags)
				{
					const FName LocalTag = ExecuteContext.AdaptMetadataName(NameSpace, Tag);
					if(!Md->GetValue().Contains(LocalTag))
					{
						Found = false;
						break;
					}
				}
			}
		}
	}
}

FTLLRN_RigUnit_FindItemsWithMetadataTag_Execute()
{
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;

	Items.Reset();
	if (!Hierarchy)
	{
		return;
	}

	const FName LocalTag = ExecuteContext.AdaptMetadataName(NameSpace, Tag);
	Hierarchy->Traverse([&Items, LocalTag](const FTLLRN_RigBaseElement* Element, bool& bContinue)
	{
		if(const FTLLRN_RigNameArrayMetadata* Md = Cast<FTLLRN_RigNameArrayMetadata>(Element->GetMetadata(UTLLRN_RigHierarchy::TagMetadataName, ETLLRN_RigMetadataType::NameArray)))
		{
			if(Md->GetValue().Contains(LocalTag))
			{
				Items.AddUnique(Element->GetKey());
			}
		}
		bContinue = true;
	});
}

FTLLRN_RigUnit_FindItemsWithMetadataTagArray_Execute()
{
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;

	Items.Reset();
	if (!Hierarchy)
	{
		return;
	}

	TArrayView<const FName> LocalTags(Tags);
	TArray<FName> AdaptedTags;

	const bool bUseNameSpace = NameSpace != ETLLRN_RigMetaDataNameSpace::None;
	if(bUseNameSpace && ExecuteContext.IsTLLRN_RigModule())
	{
		AdaptedTags.Reserve(Tags.Num());
		for(const FName& Tag : Tags)
		{
			const FName LocalTag = ExecuteContext.AdaptMetadataName(NameSpace, Tag);
			AdaptedTags.Add(LocalTag);
		}
		LocalTags = TArrayView<const FName>(AdaptedTags);
	}
	
	Hierarchy->Traverse([&Items, LocalTags](const FTLLRN_RigBaseElement* Element, bool& bContinue)
	{
		if(const FTLLRN_RigNameArrayMetadata* Md = Cast<FTLLRN_RigNameArrayMetadata>(Element->GetMetadata(UTLLRN_RigHierarchy::TagMetadataName, ETLLRN_RigMetadataType::NameArray)))
		{
			bool bFoundAll = true;
			for(const FName& Tag : LocalTags)
			{
				if(!Md->GetValue().Contains(Tag))
				{
					bFoundAll = false;
					break;
				}
			}

			if(bFoundAll)
			{
				Items.AddUnique(Element->GetKey());
			}
		}
		bContinue = true;
	});
}

FTLLRN_RigUnit_FilterItemsByMetadataTags_Execute()
{
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;

	Result.Reset();
	if (!Hierarchy)
	{
		return;
	}
	
	if(CachedIndices.Num() != Items.Num())
	{
		CachedIndices.Reset();
		CachedIndices.SetNumZeroed(Items.Num());
	}

	TArrayView<const FName> LocalTags(Tags);
	TArray<FName> AdaptedTags;
	const bool bUseNameSpace = NameSpace != ETLLRN_RigMetaDataNameSpace::None;
	if(bUseNameSpace && ExecuteContext.IsTLLRN_RigModule())
	{
		AdaptedTags.Reserve(Tags.Num());
		for(const FName& Tag : Tags)
		{
			const FName LocalTag = ExecuteContext.AdaptMetadataName(NameSpace, Tag);
			AdaptedTags.Add(LocalTag);
		}
		LocalTags = TArrayView<const FName>(AdaptedTags);
	}

	for(int32 Index = 0; Index < Items.Num(); Index++)
	{
		if(CachedIndices[Index].UpdateCache(Items[Index], Hierarchy))
		{
			if(const FTLLRN_RigBaseElement* Element = Hierarchy->Get(CachedIndices[Index]))
			{
				if(const FTLLRN_RigNameArrayMetadata* Md = Cast<FTLLRN_RigNameArrayMetadata>(Element->GetMetadata(UTLLRN_RigHierarchy::TagMetadataName, ETLLRN_RigMetadataType::NameArray)))
				{
					if(Inclusive)
					{
						bool bFoundAll = true;
						for(const FName& Tag : LocalTags)
						{
							if(!Md->GetValue().Contains(Tag))
							{
								bFoundAll = false;
								break;
							}
						}
						if(bFoundAll)
						{
							Result.Add(Element->GetKey());
						}
					}
					else
					{
						bool bFoundAny = false;
						for(const FName& Tag : LocalTags)
						{
							if(Md->GetValue().Contains(Tag))
							{
								bFoundAny = true;
								break;
							}
						}
						if(!bFoundAny)
						{
							Result.Add(Element->GetKey());
						}
					}
				}
				else if(!Inclusive)
				{
					Result.Add(Element->GetKey());
				}
			}
		}
		else
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("Item '%s' not found"), *Items[Index].ToString());
		}
	}
}

const TArray<FRigVMTemplateArgumentInfo>& FTLLRN_RigDispatch_GetModuleMetadata::GetArgumentInfos() const
{
	if(ValueArgIndex == INDEX_NONE)
	{
		NameArgIndex = Infos.Emplace(NameArgName, ERigVMPinDirection::Input, RigVMTypeUtils::TypeIndex::FName);
		NameSpaceArgIndex = Infos.Emplace(NameSpaceArgName, ERigVMPinDirection::Input, FRigVMRegistry_NoLock::GetForRead().GetTypeIndex_NoLock<ETLLRN_RigMetaDataNameSpace>());
		DefaultArgIndex = Infos.Emplace(DefaultArgName, ERigVMPinDirection::Input, GetValueTypes());
		ValueArgIndex = Infos.Emplace(ValueArgName, ERigVMPinDirection::Output, GetValueTypes());
		FoundArgIndex = Infos.Emplace(FoundArgName, ERigVMPinDirection::Output, RigVMTypeUtils::TypeIndex::Bool);
	};
	return Infos;
}

FTLLRN_RigBaseMetadata* FTLLRN_RigDispatch_GetModuleMetadata::FindMetadata(const FRigVMExtendedExecuteContext& InContext, const FName& InName, ETLLRN_RigMetadataType InType, ETLLRN_RigMetaDataNameSpace InNameSpace)
{
	const FTLLRN_ControlRigExecuteContext& ExecuteContext = InContext.GetPublicData<FTLLRN_ControlRigExecuteContext>();
	if(const FTLLRN_RigModuleInstance* ModuleInstance = ExecuteContext.GetTLLRN_RigModuleInstance(InNameSpace))
	{
		if(const FTLLRN_RigConnectorElement* PrimaryConnector = ModuleInstance->FindPrimaryConnector())
		{
			// first try to find the metadata in the namespace
			return ExecuteContext.Hierarchy->FindMetadataForElement(PrimaryConnector, InName, InType);
		}
	}
	else if(ExecuteContext.IsTLLRN_RigModule())
	{
		// we are not in a rig module - but we still want to store the metadata for testing.
		const TArray<FTLLRN_RigConnectorElement*> Connectors = ExecuteContext.Hierarchy->GetConnectors();
		for(const FTLLRN_RigConnectorElement* Connector : Connectors)
		{
			if(Connector->IsPrimary())
			{
				const FName Name = ExecuteContext.AdaptMetadataName(InNameSpace, InName);
				return ExecuteContext.Hierarchy->FindMetadataForElement(Connector, Name, InType);
			}
		}
	}
	return nullptr;
}

FRigVMFunctionPtr FTLLRN_RigDispatch_GetModuleMetadata::GetDispatchFunctionImpl(const FRigVMTemplateTypeMap& InTypes) const
{
	const FRigVMRegistry_NoLock& Registry = FRigVMRegistry_NoLock::GetForRead();
	const TRigVMTypeIndex& ValueTypeIndex = InTypes.FindChecked(TEXT("Value"));
	
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Bool)
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<bool, FTLLRN_RigBoolMetadata, ETLLRN_RigMetadataType::Bool>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Float)
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<float, FTLLRN_RigFloatMetadata, ETLLRN_RigMetadataType::Float>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Int32)
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<int32, FTLLRN_RigInt32Metadata, ETLLRN_RigMetadataType::Int32>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FName)
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<FName, FTLLRN_RigNameMetadata, ETLLRN_RigMetadataType::Name>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FVector>(false))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<FVector, FTLLRN_RigVectorMetadata, ETLLRN_RigMetadataType::Vector>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FRotator>(false))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<FRotator, FTLLRN_RigRotatorMetadata, ETLLRN_RigMetadataType::Rotator>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FQuat>(false))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<FQuat, FTLLRN_RigQuatMetadata, ETLLRN_RigMetadataType::Quat>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTransform>(false))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<FTransform, FTLLRN_RigTransformMetadata, ETLLRN_RigMetadataType::Transform>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FLinearColor>(false))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<FLinearColor, FTLLRN_RigLinearColorMetadata, ETLLRN_RigMetadataType::LinearColor>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>(false))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<FTLLRN_RigElementKey, FTLLRN_RigElementKeyMetadata, ETLLRN_RigMetadataType::TLLRN_RigElementKey>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::BoolArray)
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<TArray<bool>, FTLLRN_RigBoolArrayMetadata, ETLLRN_RigMetadataType::BoolArray>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FloatArray)
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<TArray<float>, FTLLRN_RigFloatArrayMetadata, ETLLRN_RigMetadataType::FloatArray>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Int32Array)
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<TArray<int32>, FTLLRN_RigInt32ArrayMetadata, ETLLRN_RigMetadataType::Int32Array>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FNameArray)
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<TArray<FName>, FTLLRN_RigNameArrayMetadata, ETLLRN_RigMetadataType::NameArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FVector>(true))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<TArray<FVector>, FTLLRN_RigVectorArrayMetadata, ETLLRN_RigMetadataType::VectorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FRotator>(true))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<TArray<FRotator>, FTLLRN_RigRotatorArrayMetadata, ETLLRN_RigMetadataType::RotatorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FQuat>(true))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<TArray<FQuat>, FTLLRN_RigQuatArrayMetadata, ETLLRN_RigMetadataType::QuatArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTransform>(true))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<TArray<FTransform>, FTLLRN_RigTransformArrayMetadata, ETLLRN_RigMetadataType::TransformArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FLinearColor>(true))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<TArray<FLinearColor>, FTLLRN_RigLinearColorArrayMetadata, ETLLRN_RigMetadataType::LinearColorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>(true))
	{
		return &FTLLRN_RigDispatch_GetModuleMetadata::GetModuleMetadataDispatch<TArray<FTLLRN_RigElementKey>, FTLLRN_RigElementKeyArrayMetadata, ETLLRN_RigMetadataType::TLLRN_RigElementKeyArray>;
	}

	return nullptr;
}

const TArray<FRigVMTemplateArgumentInfo>& FTLLRN_RigDispatch_SetModuleMetadata::GetArgumentInfos() const
{
	if(ValueArgIndex == INDEX_NONE)
	{
		NameArgIndex = Infos.Emplace(NameArgName, ERigVMPinDirection::Input, RigVMTypeUtils::TypeIndex::FName);
		NameSpaceArgIndex = Infos.Emplace(NameSpaceArgName, ERigVMPinDirection::Input, FRigVMRegistry_NoLock::GetForRead().GetTypeIndex_NoLock<ETLLRN_RigMetaDataNameSpace>());
		ValueArgIndex = Infos.Emplace(ValueArgName, ERigVMPinDirection::Input, GetValueTypes());
		SuccessArgIndex = Infos.Emplace(SuccessArgName, ERigVMPinDirection::Output, RigVMTypeUtils::TypeIndex::Bool);
	};
	return Infos;
}

FTLLRN_RigBaseMetadata* FTLLRN_RigDispatch_SetModuleMetadata::FindOrAddMetadata(const FTLLRN_ControlRigExecuteContext& InContext, const FName& InName, ETLLRN_RigMetadataType InType, ETLLRN_RigMetaDataNameSpace InNameSpace)
{
	constexpr bool bNotify = true;
	
	if(const FTLLRN_RigModuleInstance* ModuleInstance = InContext.GetTLLRN_RigModuleInstance(InNameSpace))
	{
		if(const FTLLRN_RigConnectorElement* PrimaryConnector = ModuleInstance->FindPrimaryConnector())
		{
			return InContext.Hierarchy->GetMetadataForElement(const_cast<FTLLRN_RigConnectorElement*>(PrimaryConnector), InName, InType, bNotify);
		}
	}
	else if(InContext.IsTLLRN_RigModule())
	{
		// we are not in a rig module - but we still want to store the metadata for testing.
		const TArray<FTLLRN_RigConnectorElement*> Connectors = InContext.Hierarchy->GetConnectors();
		for(const FTLLRN_RigConnectorElement* Connector : Connectors)
		{
			if(Connector->IsPrimary())
			{
				const FName Name = InContext.AdaptMetadataName(InNameSpace, InName);
				return InContext.Hierarchy->GetMetadataForElement(const_cast<FTLLRN_RigConnectorElement*>(Connector), Name, InType, bNotify);
			}
		}
	}
	return nullptr;
}

FRigVMFunctionPtr FTLLRN_RigDispatch_SetModuleMetadata::GetDispatchFunctionImpl(const FRigVMTemplateTypeMap& InTypes) const
{
	const FRigVMRegistry_NoLock& Registry = FRigVMRegistry_NoLock::GetForRead();
	const TRigVMTypeIndex& ValueTypeIndex = InTypes.FindChecked(TEXT("Value"));
	
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Bool)
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<bool, FTLLRN_RigBoolMetadata, ETLLRN_RigMetadataType::Bool>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Float)
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<float, FTLLRN_RigFloatMetadata, ETLLRN_RigMetadataType::Float>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Int32)
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<int32, FTLLRN_RigInt32Metadata, ETLLRN_RigMetadataType::Int32>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FName)
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<FName, FTLLRN_RigNameMetadata, ETLLRN_RigMetadataType::Name>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FVector>(false))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<FVector, FTLLRN_RigVectorMetadata, ETLLRN_RigMetadataType::Vector>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FRotator>(false))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<FRotator, FTLLRN_RigRotatorMetadata, ETLLRN_RigMetadataType::Rotator>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FQuat>(false))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<FQuat, FTLLRN_RigQuatMetadata, ETLLRN_RigMetadataType::Quat>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTransform>(false))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<FTransform, FTLLRN_RigTransformMetadata, ETLLRN_RigMetadataType::Transform>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FLinearColor>(false))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<FLinearColor, FTLLRN_RigLinearColorMetadata, ETLLRN_RigMetadataType::LinearColor>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>(false))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<FTLLRN_RigElementKey, FTLLRN_RigElementKeyMetadata, ETLLRN_RigMetadataType::TLLRN_RigElementKey>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::BoolArray)
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<TArray<bool>, FTLLRN_RigBoolArrayMetadata, ETLLRN_RigMetadataType::BoolArray>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FloatArray)
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<TArray<float>, FTLLRN_RigFloatArrayMetadata, ETLLRN_RigMetadataType::FloatArray>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::Int32Array)
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<TArray<int32>, FTLLRN_RigInt32ArrayMetadata, ETLLRN_RigMetadataType::Int32Array>;
	}
	if(ValueTypeIndex == RigVMTypeUtils::TypeIndex::FNameArray)
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<TArray<FName>, FTLLRN_RigNameArrayMetadata, ETLLRN_RigMetadataType::NameArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FVector>(true))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<TArray<FVector>, FTLLRN_RigVectorArrayMetadata, ETLLRN_RigMetadataType::VectorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FRotator>(true))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<TArray<FRotator>, FTLLRN_RigRotatorArrayMetadata, ETLLRN_RigMetadataType::RotatorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FQuat>(true))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<TArray<FQuat>, FTLLRN_RigQuatArrayMetadata, ETLLRN_RigMetadataType::QuatArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTransform>(true))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<TArray<FTransform>, FTLLRN_RigTransformArrayMetadata, ETLLRN_RigMetadataType::TransformArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FLinearColor>(true))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<TArray<FLinearColor>, FTLLRN_RigLinearColorArrayMetadata, ETLLRN_RigMetadataType::LinearColorArray>;
	}
	if(ValueTypeIndex == Registry.GetTypeIndex_NoLock<FTLLRN_RigElementKey>(true))
	{
		return &FTLLRN_RigDispatch_SetModuleMetadata::SetModuleMetadataDispatch<TArray<FTLLRN_RigElementKey>, FTLLRN_RigElementKeyArrayMetadata, ETLLRN_RigMetadataType::TLLRN_RigElementKeyArray>;
	}

	return nullptr;
}

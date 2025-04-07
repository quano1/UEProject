// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigDispatchFactory.h"
#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_Metadata.generated.h"

USTRUCT(meta=(Abstract, Category="Hierarchy", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigDispatch_MetadataBase : public FTLLRN_RigDispatchFactory
{
	GENERATED_BODY()

	FTLLRN_RigDispatch_MetadataBase()
	{
		FactoryScriptStruct = StaticStruct();
	}

	template<typename T>
	static bool Equals(const T& A, const T& B)
	{
		return A.Equals(B);
	}

	template<typename T>
	static bool ArrayEquals(const TArray<T>& A, const TArray<T>& B)
	{
		if(A.Num() != B.Num())
		{
			return false;
		}
		for(int32 Index = 0; Index < A.Num(); Index++)
		{
			if(!Equals<T>(A[Index], B[Index]))
			{
				return false;
			}
		}
		return true;
	}

#if WITH_EDITOR
	virtual FString GetNodeTitle(const FRigVMTemplateTypeMap& InTypes) const override;;
	virtual FString GetNodeTitlePrefix() const { return FString(); }
#endif
	virtual const TArray<FRigVMTemplateArgumentInfo>& GetArgumentInfos() const override;
	virtual bool IsSetMetadata() const { return false; }

#if WITH_EDITOR
	virtual FString GetArgumentDefaultValue(const FName& InArgumentName, TRigVMTypeIndex InTypeIndex) const override;
	virtual FString GetArgumentMetaData(const FName& InArgumentName, const FName& InMetaDataKey) const override;
	virtual FText GetArgumentTooltip(const FName& InArgumentName, TRigVMTypeIndex InTypeIndex) const override;
#endif

protected:

	const TArray<TRigVMTypeIndex>& GetValueTypes() const;

	mutable TArray<FRigVMTemplateArgumentInfo> Infos;
	
	mutable int32 ItemArgIndex = INDEX_NONE;
	mutable int32 NameArgIndex = INDEX_NONE;
	mutable int32 NameSpaceArgIndex = INDEX_NONE;
	mutable int32 CacheArgIndex = INDEX_NONE;
	mutable int32 DefaultArgIndex = INDEX_NONE;
	mutable int32 ValueArgIndex = INDEX_NONE;
	mutable int32 FoundArgIndex = INDEX_NONE;
	mutable int32 SuccessArgIndex = INDEX_NONE;

	static inline const FLazyName ItemArgName = FLazyName(TEXT("Item"));
	static inline const FLazyName NameArgName = FLazyName(TEXT("Name"));
	static inline const FLazyName NameSpaceArgName = FLazyName(TEXT("NameSpace"));
	static inline const FLazyName CacheArgName = FLazyName(TEXT("Cache"));
	static inline const FLazyName DefaultArgName = FLazyName(TEXT("Default"));
	static inline const FLazyName ValueArgName = FLazyName(TEXT("Value"));
	static inline const FLazyName FoundArgName = FLazyName(TEXT("Found"));
	static inline const FLazyName SuccessArgName = FLazyName(TEXT("Success"));
};

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const bool& A, const bool& B)
{
	return A == B;
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const int32& A, const int32& B)
{
	return A == B;
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const float& A, const float& B)
{
	return FMath::IsNearlyEqual(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const double& A, const double& B)
{
	return FMath::IsNearlyEqual(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const FName& A, const FName& B)
{
	return A.IsEqual(B, ENameCase::CaseSensitive);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const FTLLRN_RigElementKey& A, const FTLLRN_RigElementKey& B)
{
	return A == B;
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<bool>& A, const TArray<bool>& B)
{
	return ArrayEquals<bool>(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<int32>& A, const TArray<int32>& B)
{
	return ArrayEquals<int32>(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<float>& A, const TArray<float>& B)
{
	return ArrayEquals<float>(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<double>& A, const TArray<double>& B)
{
	return ArrayEquals<double>(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<FName>& A, const TArray<FName>& B)
{
	return ArrayEquals<FName>(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<FTLLRN_RigElementKey>& A, const TArray<FTLLRN_RigElementKey>& B)
{
	return ArrayEquals<FTLLRN_RigElementKey>(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<FVector>& A, const TArray<FVector>& B)
{
	return ArrayEquals<FVector>(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<FRotator>& A, const TArray<FRotator>& B)
{
	return ArrayEquals<FRotator>(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<FQuat>& A, const TArray<FQuat>& B)
{
	return ArrayEquals<FQuat>(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<FTransform>& A, const TArray<FTransform>& B)
{
	return ArrayEquals<FTransform>(A, B);
}

template<>
inline bool FTLLRN_RigDispatch_MetadataBase::Equals(const TArray<FLinearColor>& A, const TArray<FLinearColor>& B)
{
	return ArrayEquals<FLinearColor>(A, B);
}

/*
 * Sets some metadata for the provided item
 */
USTRUCT(meta=(DisplayName="Get Metadata"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigDispatch_GetMetadata : public FTLLRN_RigDispatch_MetadataBase
{
	GENERATED_BODY()

	virtual const TArray<FRigVMTemplateArgumentInfo>& GetArgumentInfos() const override;

protected:

	static FTLLRN_RigBaseMetadata* FindMetadata(const FRigVMExtendedExecuteContext& InContext, const FTLLRN_RigElementKey& InKey, const FName& InName, ETLLRN_RigMetadataType InType, ETLLRN_RigMetaDataNameSpace InNameSpace, FTLLRN_CachedTLLRN_RigElement& Cache);
	virtual FRigVMFunctionPtr GetDispatchFunctionImpl(const FRigVMTemplateTypeMap& InTypes) const override;

#if WITH_EDITOR
	template<typename ValueType>
	bool CheckArgumentTypes(FRigVMMemoryHandleArray Handles) const
	{
		return CheckArgumentType(Handles[ItemArgIndex].IsType<FTLLRN_RigElementKey>(), ItemArgName) &&
			CheckArgumentType(Handles[NameArgIndex].IsType<FName>(), NameArgName) &&
			CheckArgumentType(Handles[NameSpaceArgIndex].IsType<ETLLRN_RigMetaDataNameSpace>(), NameSpaceArgName) &&
			CheckArgumentType(Handles[CacheArgIndex].IsType<FTLLRN_CachedTLLRN_RigElement>(true), CacheArgName) &&
			CheckArgumentType(Handles[DefaultArgIndex].IsType<ValueType>(), DefaultArgName) &&
			CheckArgumentType(Handles[ValueArgIndex].IsType<ValueType>(), ValueArgName) &&
			CheckArgumentType(Handles[FoundArgIndex].IsType<bool>(), FoundArgName);
	}
#endif

	template<typename ValueType, typename MetadataType, ETLLRN_RigMetadataType EnumValue>
	static void GetMetadataDispatch(FRigVMExtendedExecuteContext& InContext, FRigVMMemoryHandleArray Handles, FRigVMPredicateBranchArray Predicates)
	{
		const FTLLRN_RigDispatch_GetMetadata* Factory = static_cast<const FTLLRN_RigDispatch_GetMetadata*>(InContext.Factory);

#if WITH_EDITOR
		if(!Factory->CheckArgumentTypes<ValueType>(Handles))
		{
			return;
		}
#endif

		// unpack the memory
		const FTLLRN_RigElementKey& Item = *reinterpret_cast<const FTLLRN_RigElementKey*>(Handles[Factory->ItemArgIndex].GetData());
		const FName& Name = *reinterpret_cast<const FName*>(Handles[Factory->NameArgIndex].GetData());
		const ETLLRN_RigMetaDataNameSpace NameSpace = *reinterpret_cast<const ETLLRN_RigMetaDataNameSpace*>(Handles[Factory->NameSpaceArgIndex].GetData());
		FTLLRN_CachedTLLRN_RigElement& Cache = *reinterpret_cast<FTLLRN_CachedTLLRN_RigElement*>(Handles[Factory->CacheArgIndex].GetData(false, InContext.GetSlice().GetIndex()));
		const ValueType& Default = *reinterpret_cast<const ValueType*>(Handles[Factory->DefaultArgIndex].GetData());
		ValueType& Value = *reinterpret_cast<ValueType*>(Handles[Factory->ValueArgIndex].GetData());
		bool& Found = *reinterpret_cast<bool*>(Handles[Factory->FoundArgIndex].GetData());

		// extract the metadata
		if (const MetadataType* Md = Cast<MetadataType>(FindMetadata(InContext, Item, Name, EnumValue, NameSpace, Cache)))
		{
			Value = Md->GetValue();
			Found = true;
		}
		else
		{
			Value = Default;
			Found = false;
		}
	}
};

/*
 * Sets some metadata for the provided item
 */
USTRUCT(meta=(DisplayName="Set Metadata"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigDispatch_SetMetadata : public FTLLRN_RigDispatch_MetadataBase
{
	GENERATED_BODY()

	virtual const TArray<FRigVMTemplateArgumentInfo>& GetArgumentInfos() const override;
	virtual const TArray<FRigVMExecuteArgument>& GetExecuteArguments_Impl(const FRigVMDispatchContext& InContext) const override;
	virtual bool IsSetMetadata() const override { return true; }

protected:

	static FTLLRN_RigBaseMetadata* FindOrAddMetadata(const FTLLRN_ControlRigExecuteContext& InContext, const FTLLRN_RigElementKey& InKey, const FName& InName, ETLLRN_RigMetadataType InType, ETLLRN_RigMetaDataNameSpace InNameSpace, FTLLRN_CachedTLLRN_RigElement& Cache);
	virtual FRigVMFunctionPtr GetDispatchFunctionImpl(const FRigVMTemplateTypeMap& InTypes) const override;

#if WITH_EDITOR
	template<typename ValueType>
	bool CheckArgumentTypes(FRigVMMemoryHandleArray Handles) const
	{
		return CheckArgumentType(Handles[ItemArgIndex].IsType<FTLLRN_RigElementKey>(), ItemArgName) &&
			CheckArgumentType(Handles[NameArgIndex].IsType<FName>(), NameArgName) &&
			CheckArgumentType(Handles[NameSpaceArgIndex].IsType<ETLLRN_RigMetaDataNameSpace>(), NameSpaceArgName) &&
			CheckArgumentType(Handles[CacheArgIndex].IsType<FTLLRN_CachedTLLRN_RigElement>(true), CacheArgName) &&
			CheckArgumentType(Handles[ValueArgIndex].IsType<ValueType>(), ValueArgName) &&
			CheckArgumentType(Handles[SuccessArgIndex].IsType<bool>(), SuccessArgName);
	}
#endif
	
	template<typename ValueType, typename MetadataType, ETLLRN_RigMetadataType EnumValue>
	static void SetMetadataDispatch(FRigVMExtendedExecuteContext& InContext, FRigVMMemoryHandleArray Handles, FRigVMPredicateBranchArray Predicates)
	{
		const FTLLRN_RigDispatch_SetMetadata* Factory = static_cast<const FTLLRN_RigDispatch_SetMetadata*>(InContext.Factory);

#if WITH_EDITOR
		if(!Factory->CheckArgumentTypes<ValueType>(Handles))
		{
			return;
		}
#endif

		// unpack the memory
		FTLLRN_ControlRigExecuteContext& ExecuteContext = InContext.GetPublicData<FTLLRN_ControlRigExecuteContext>();
		const FTLLRN_RigElementKey& Item = *(const FTLLRN_RigElementKey*)Handles[Factory->ItemArgIndex].GetData();
		const FName& Name = *(const FName*)Handles[Factory->NameArgIndex].GetData();
		const ETLLRN_RigMetaDataNameSpace NameSpace = *(const ETLLRN_RigMetaDataNameSpace*)Handles[Factory->NameSpaceArgIndex].GetData();
		FTLLRN_CachedTLLRN_RigElement& Cache = *(FTLLRN_CachedTLLRN_RigElement*)Handles[Factory->CacheArgIndex].GetData(false, InContext.GetSlice().GetIndex());
		ValueType& Value = *(ValueType*)Handles[Factory->ValueArgIndex].GetData();
		bool& Success = *(bool*)Handles[Factory->SuccessArgIndex].GetData();

		// extract the metadata
		if (MetadataType* Md = Cast<MetadataType>(FindOrAddMetadata(ExecuteContext, Item, Name, EnumValue, NameSpace, Cache)))
		{
			if(!Equals<ValueType>(Md->GetValue(), Value))
			{
				Md->GetValue() = Value;
				ExecuteContext.Hierarchy->MetadataVersion++;
			}
			Success = true;
		}
		else
		{
			Success = false;
		}
	}
};

/**
 * Removes an existing metadata filed from an item
 */
USTRUCT(meta=(DisplayName="Remove Metadata", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="DeleteMetadata", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_RemoveMetadata : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_RemoveMetadata()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, Name(NAME_None)
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, Removed(false)
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to remove the metadata from 
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The name of the metadata to remove
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataName"))
	FName Name;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;

	// True if the metadata has been removed
	UPROPERTY(meta=(Output))
	bool Removed;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
 * Removes an existing metadata filed from an item
 */
USTRUCT(meta=(DisplayName="Remove All Metadata", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="DeleteMetadata", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_RemoveAllMetadata : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_RemoveAllMetadata()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, Removed(false)
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to remove the metadata from 
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;

	// True if any metadata has been removed
	UPROPERTY(meta=(Output))
	bool Removed;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
 * Returns true if a given item in the hierarchy has a specific set of metadata
 */
USTRUCT(meta=(DisplayName="Has Metadata", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="MetadataExists,HasKey,SupportsMetadata,FindMetadata", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HasMetadata : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HasMetadata()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, Name(NAME_None)
		, Type(ETLLRN_RigMetadataType::Float)
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, Found(false)
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to check the metadata for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The name of the metadata to check
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataName"))
	FName Name;

	/**
	 * The type of metadata to check for
	 */ 
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetadataType Type;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;

	// True if the item has the metadata
	UPROPERTY(meta=(Output))
	bool Found;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
 * Returns all items containing a specific set of metadata
 */
USTRUCT(meta=(DisplayName="Find Items with Metadata", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="MetadataExists,HasKey,SupportsMetadata", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_FindItemsWithMetadata : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_FindItemsWithMetadata()
		: Name(NAME_None)
		, Type(ETLLRN_RigMetadataType::Float)
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, Items()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the metadata to find
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataName"))
	FName Name;

	/**
	 * The type of metadata to find
	 */ 
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetadataType Type;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;

	// The items containing the metadata
	UPROPERTY(meta=(Output))
	TArray<FTLLRN_RigElementKey> Items;
};

/**
 * Returns the metadata tags on an item
 */
USTRUCT(meta=(DisplayName="Get Tags", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="MetadataExists,HasKey,Tagging,FindTag", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetMetadataTags : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetMetadataTags()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, Tags()
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to check the metadata for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The name of the tag to check
	 */ 
	UPROPERTY(meta = (Output))
	TArray<FName> Tags;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
 * Sets a single tag on an item 
 */
USTRUCT(meta=(DisplayName="Add Tag", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="MetadataExists,HasKey,Tagging,FindTag,SetTag", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetMetadataTag : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetMetadataTag()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, Tag(NAME_None)
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to set the metadata for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The name of the tag to set
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataTagName"))
	FName Tag;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
 * Sets multiple tags on an item 
 */
USTRUCT(meta=(DisplayName="Add Multiple Tags", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="AddTags,MetadataExists,HasKey,Tagging,FindTag,SetTags", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetMetadataTagArray : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetMetadataTagArray()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, Tags()
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to set the metadata for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The tags to set for the item
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataTagName"))
	TArray<FName> Tags;
	
	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
 * Removes a tag from an item 
 */
USTRUCT(meta=(DisplayName="Remove Tag", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="DeleteTag", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_RemoveMetadataTag : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_RemoveMetadataTag()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, Tag(NAME_None)
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, Removed(false)
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to set the metadata for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The name of the tag to set
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataTagName"))
	FName Tag;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;
	
	/**
	 * Returns true if the removal was successful
	 */ 
	UPROPERTY(meta = (Output))
	bool Removed;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
 * Returns true if a given item in the hierarchy has a specific tag stored in the metadata
 */
USTRUCT(meta=(DisplayName="Has Tag", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="MetadataExists,HasKey,Tagging,FindTag", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HasMetadataTag : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HasMetadataTag()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, Tag(NAME_None)
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, Found(false)
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to check the metadata for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The name of the tag to check
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataTagName"))
	FName Tag;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;
	
	// True if the item has the metadata
	UPROPERTY(meta=(Output))
	bool Found;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
 * Returns true if a given item in the hierarchy has all of the provided tags
 */
USTRUCT(meta=(DisplayName="Has Multiple Tags", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="MetadataExists,HasKey,Tagging,FindTag", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HasMetadataTagArray : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HasMetadataTagArray()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, Tags()
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, Found(false)
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to check the metadata for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The name of the tag to check
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataTagName"))
	TArray<FName> Tags;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;
	
	// True if the item has the metadata
	UPROPERTY(meta=(Output))
	bool Found;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
 * Returns all items with a specific tag
 */
USTRUCT(meta=(DisplayName="Find Items with Tag", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="MetadataExists,HasKey,SupportsMetadata", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_FindItemsWithMetadataTag : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_FindItemsWithMetadataTag()
		: Tag(NAME_None)
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, Items()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the tag to find
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataTagName"))
	FName Tag;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;

	// The items containing the metadata
	UPROPERTY(meta=(Output))
	TArray<FTLLRN_RigElementKey> Items;
};

/**
 * Returns all items with a specific tag
 */
USTRUCT(meta=(DisplayName="Find Items with multiple Tags", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="MetadataExists,HasKey,SupportsMetadata", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_FindItemsWithMetadataTagArray : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_FindItemsWithMetadataTagArray()
		: Tags()
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, Items()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The tags to find
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataTagName"))
	TArray<FName> Tags;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;

	// The items containing the metadata
	UPROPERTY(meta=(Output))
	TArray<FTLLRN_RigElementKey> Items;
};

/**
 * Filters an item array by a list of tags
 */
USTRUCT(meta=(DisplayName="Filter Items by Tags", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="MetadataExists,HasKey,SupportsMetadata", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_FilterItemsByMetadataTags : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_FilterItemsByMetadataTags()
		: Items()
		, Tags()
		, NameSpace(ETLLRN_RigMetaDataNameSpace::Self)
		, Inclusive(true)
		, Result()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The items to filter
	UPROPERTY(meta=(Input))
	TArray<FTLLRN_RigElementKey> Items;

	/**
	 * The tags to find
	 */ 
	UPROPERTY(meta = (Input, CustomWidget="MetadataTagName"))
	TArray<FName> Tags;

	/**
	 * Defines in which namespace the metadata will be looked up
	 */
	UPROPERTY(meta = (Input))
	ETLLRN_RigMetaDataNameSpace NameSpace;

	/**
     * If set to true only items with ALL of tags will be returned,
     * if set to false items with ANY of the tags will be removed
     */ 
	UPROPERTY(meta = (Input))
	bool Inclusive;

	// The results of the filter
	UPROPERTY(meta=(Output))
	TArray<FTLLRN_RigElementKey> Result;

	// Used to cache the internally used indices
	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> CachedIndices;
};

/*
 * Returns some metadata on a given module
 */
USTRUCT(meta=(DisplayName="Get Module Metadata"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigDispatch_GetModuleMetadata : public FTLLRN_RigDispatch_GetMetadata
{
	GENERATED_BODY()

	virtual const TArray<FRigVMTemplateArgumentInfo>& GetArgumentInfos() const override;
#if WITH_EDITOR
	virtual FString GetNodeTitlePrefix() const override { return TEXT("Module "); }
#endif
	
protected:

	static FTLLRN_RigBaseMetadata* FindMetadata(const FRigVMExtendedExecuteContext& InContext, const FName& InName, ETLLRN_RigMetadataType InType, ETLLRN_RigMetaDataNameSpace InNameSpace);
	virtual FRigVMFunctionPtr GetDispatchFunctionImpl(const FRigVMTemplateTypeMap& InTypes) const override;

#if WITH_EDITOR
	template<typename ValueType>
	bool CheckArgumentTypes(FRigVMMemoryHandleArray Handles) const
	{
		return CheckArgumentType(Handles[NameArgIndex].IsType<FName>(), NameArgName) &&
			CheckArgumentType(Handles[NameSpaceArgIndex].IsType<ETLLRN_RigMetaDataNameSpace>(), NameSpaceArgName) &&
			CheckArgumentType(Handles[DefaultArgIndex].IsType<ValueType>(), DefaultArgName) &&
			CheckArgumentType(Handles[ValueArgIndex].IsType<ValueType>(), ValueArgName) &&
			CheckArgumentType(Handles[FoundArgIndex].IsType<bool>(), FoundArgName);
	}
#endif

	template<typename ValueType, typename MetadataType, ETLLRN_RigMetadataType EnumValue>
	static void GetModuleMetadataDispatch(FRigVMExtendedExecuteContext& InContext, FRigVMMemoryHandleArray Handles, FRigVMPredicateBranchArray Predicates)
	{
		const FTLLRN_RigDispatch_GetModuleMetadata* Factory = static_cast<const FTLLRN_RigDispatch_GetModuleMetadata*>(InContext.Factory);

#if WITH_EDITOR
		if(!Factory->CheckArgumentTypes<ValueType>(Handles))
		{
			return;
		}
#endif

		// unpack the memory
		const FName& Name = *reinterpret_cast<const FName*>(Handles[Factory->NameArgIndex].GetData());
		const ETLLRN_RigMetaDataNameSpace NameSpace = *reinterpret_cast<const ETLLRN_RigMetaDataNameSpace*>(Handles[Factory->NameSpaceArgIndex].GetData());
		const ValueType& Default = *reinterpret_cast<const ValueType*>(Handles[Factory->DefaultArgIndex].GetData());
		ValueType& Value = *reinterpret_cast<ValueType*>(Handles[Factory->ValueArgIndex].GetData());
		bool& Found = *reinterpret_cast<bool*>(Handles[Factory->FoundArgIndex].GetData());

		// extract the metadata
		if (const MetadataType* Md = Cast<MetadataType>(FindMetadata(InContext, Name, EnumValue, NameSpace)))
		{
			Value = Md->GetValue();
			Found = true;
		}
		else
		{
			Value = Default;
			Found = false;
		}
	}
};

/*
 * Sets metadata on the module
 */
USTRUCT(meta=(DisplayName="Set Module Metadata"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigDispatch_SetModuleMetadata : public FTLLRN_RigDispatch_SetMetadata
{
	GENERATED_BODY()

	virtual const TArray<FRigVMTemplateArgumentInfo>& GetArgumentInfos() const override;
#if WITH_EDITOR
	virtual FString GetNodeTitlePrefix() const override { return TEXT("Module "); }
#endif

protected:

	static FTLLRN_RigBaseMetadata* FindOrAddMetadata(const FTLLRN_ControlRigExecuteContext& InContext, const FName& InName, ETLLRN_RigMetadataType InType, ETLLRN_RigMetaDataNameSpace InNameSpace);
	virtual FRigVMFunctionPtr GetDispatchFunctionImpl(const FRigVMTemplateTypeMap& InTypes) const override;

#if WITH_EDITOR
	template<typename ValueType>
	bool CheckArgumentTypes(FRigVMMemoryHandleArray Handles) const
	{
		return CheckArgumentType(Handles[NameArgIndex].IsType<FName>(), NameArgName) &&
			CheckArgumentType(Handles[NameSpaceArgIndex].IsType<ETLLRN_RigMetaDataNameSpace>(), NameSpaceArgName) &&
			CheckArgumentType(Handles[ValueArgIndex].IsType<ValueType>(), ValueArgName) &&
			CheckArgumentType(Handles[SuccessArgIndex].IsType<bool>(), SuccessArgName);
	}
#endif
	
	template<typename ValueType, typename MetadataType, ETLLRN_RigMetadataType EnumValue>
	static void SetModuleMetadataDispatch(FRigVMExtendedExecuteContext& InContext, FRigVMMemoryHandleArray Handles, FRigVMPredicateBranchArray Predicates)
	{
		const FTLLRN_RigDispatch_SetModuleMetadata* Factory = static_cast<const FTLLRN_RigDispatch_SetModuleMetadata*>(InContext.Factory);

#if WITH_EDITOR
		if(!Factory->CheckArgumentTypes<ValueType>(Handles))
		{
			return;
		}
#endif

		// unpack the memory
		FTLLRN_ControlRigExecuteContext& ExecuteContext = InContext.GetPublicData<FTLLRN_ControlRigExecuteContext>();
		const FName& Name = *(const FName*)Handles[Factory->NameArgIndex].GetData();
		const ETLLRN_RigMetaDataNameSpace NameSpace = *(const ETLLRN_RigMetaDataNameSpace*)Handles[Factory->NameSpaceArgIndex].GetData();
		ValueType& Value = *(ValueType*)Handles[Factory->ValueArgIndex].GetData();
		bool& Success = *(bool*)Handles[Factory->SuccessArgIndex].GetData();

		// extract the metadata
		if (MetadataType* Md = Cast<MetadataType>(FindOrAddMetadata(ExecuteContext, Name, EnumValue, NameSpace)))
		{
			if(!Equals<ValueType>(Md->GetValue(), Value))
			{
				Md->GetValue() = Value;
				ExecuteContext.Hierarchy->MetadataVersion++;
			}
			Success = true;
		}
		else
		{
			Success = false;
		}
	}
};

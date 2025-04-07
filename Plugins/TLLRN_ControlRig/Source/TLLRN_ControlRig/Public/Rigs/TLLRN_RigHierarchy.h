// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RigVMCore/RigVMExecuteContext.h"
#include "RigVMCore/RigVM.h"
#include "TLLRN_RigName.h"
#include "TLLRN_RigHierarchyElements.h"
#include "TLLRN_RigHierarchyCache.h"
#include "TLLRN_RigHierarchyPose.h"
#include "TLLRN_RigHierarchyPoseAdapter.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "EdGraph/EdGraphPin.h"
#include "TLLRN_RigHierarchyDefines.h"
#if WITH_EDITOR
#include "RigVMPythonUtils.h"
#endif
#include "Containers/Queue.h"
#include "TLLRN_RigHierarchy.generated.h"

class UTLLRN_ControlRig;
class UTLLRN_RigHierarchy;
class UTLLRN_RigHierarchyController;
class UTLLRN_ModularRigRuleManager; 

DECLARE_MULTICAST_DELEGATE_ThreeParams(FTLLRN_RigHierarchyModifiedEvent, ETLLRN_RigHierarchyNotification /* type */, UTLLRN_RigHierarchy* /* hierarchy */, const FTLLRN_RigBaseElement* /* element */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTLLRN_RigHierarchyModifiedDynamicEvent, ETLLRN_RigHierarchyNotification, NotifType, UTLLRN_RigHierarchy*, Hierarchy, FTLLRN_RigElementKey, Subject);
DECLARE_EVENT_FiveParams(UTLLRN_RigHierarchy, FTLLRN_RigHierarchyUndoRedoTransformEvent, UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey&, ETLLRN_RigTransformType::Type, const FTransform&, bool /* bUndo */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTLLRN_RigHierarchyMetadataChangedDelegate, const FTLLRN_RigElementKey& /* Key */, const FName& /* Name */);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTLLRN_RigHierarchyMetadataTagChangedDelegate, const FTLLRN_RigElementKey& /* Key */, const FName& /* Tag */, bool /* AddedOrRemoved */);

extern TLLRN_CONTROLRIG_API TAutoConsoleVariable<bool> CVarTLLRN_ControlTLLRN_RigHierarchyEnableRotationOrder;
extern TLLRN_CONTROLRIG_API TAutoConsoleVariable<bool> CVarTLLRN_ControlTLLRN_RigHierarchyEnableModules;
extern TLLRN_CONTROLRIG_API TAutoConsoleVariable<bool> CVarTLLRN_ControlTLLRN_RigHierarchyEnablePhysics;

UENUM()
enum ETLLRN_RigTransformStackEntryType : int
{
	TransformPose,
	ControlOffset,
	ControlShape,
	CurveValue
};

USTRUCT()
struct FTLLRN_RigTransformStackEntry
{
	GENERATED_BODY()

	FTLLRN_RigTransformStackEntry()
		: Key()
		, EntryType(ETLLRN_RigTransformStackEntryType::TransformPose)
		, TransformType(ETLLRN_RigTransformType::CurrentLocal)
		, OldTransform(FTransform::Identity)
		, NewTransform(FTransform::Identity)
		, bAffectChildren(true)
		, Callstack() 
	{}

	FTLLRN_RigTransformStackEntry(
		const FTLLRN_RigElementKey& InKey,
		ETLLRN_RigTransformStackEntryType InEntryType,
		ETLLRN_RigTransformType::Type InTransformType,
		const FTransform& InOldTransform,
		const FTransform& InNewTransform,
		bool bInAffectChildren,
		const TArray<FString>& InCallstack =  TArray<FString>())
		: Key(InKey)
		, EntryType(InEntryType)
		, TransformType(InTransformType)
		, OldTransform(InOldTransform)
		, NewTransform(InNewTransform)
		, bAffectChildren(bInAffectChildren)
		, Callstack(InCallstack)
	{}

	UPROPERTY()
	FTLLRN_RigElementKey Key;

	UPROPERTY()
	TEnumAsByte<ETLLRN_RigTransformStackEntryType> EntryType;

	UPROPERTY()
	TEnumAsByte<ETLLRN_RigTransformType::Type> TransformType;
	
	UPROPERTY()
	FTransform OldTransform;

	UPROPERTY()
	FTransform NewTransform;

	UPROPERTY()
	bool bAffectChildren;

	UPROPERTY()
	TArray<FString> Callstack;
};

template<typename T>
class TLLRN_CONTROLRIG_API THierarchyCache
{
public:

	THierarchyCache()
		: TopologyVersion(0)
	{}

	THierarchyCache(const T& InValue, uint32 InTopologyVersion)
		: THierarchyCache()
	{
		Value = InValue;
		TopologyVersion = InTopologyVersion;
	}

	bool IsValid(uint32 InTopologyVersion) const
	{
		return (TopologyVersion == InTopologyVersion) && Value.IsSet();
	}

	void Reset()
	{
		TopologyVersion = 0;
		Value = TOptional<T>();
	}

	const T& Get() const
	{
		return Value.GetValue();
	}

	T& Get()
	{
		if(!Value.IsSet())
		{
			Value = T();
		}
		return Value.GetValue();
	}

	void Set(uint32 InTopologyVersion)
	{
		check(Value.IsSet());
		TopologyVersion = InTopologyVersion;
	}

	void Set(const T& InValue, uint32 InTopologyVersion)
	{
		Value = InValue;
		TopologyVersion = InTopologyVersion;
	}

private:

	uint32 TopologyVersion;
	TOptional<T> Value;
};

UCLASS(BlueprintType)
class TLLRN_CONTROLRIG_API UTLLRN_RigHierarchy : public UObject
{
	GENERATED_BODY()

public:

	typedef TMap<int32, TArray<int32>> TElementDependencyMap;
	typedef TPair<int32, TArray<int32>> TElementDependencyMapPair;
	typedef TTuple<int32, int32, int32, ETLLRN_RigTransformType::Type> TInstructionSliceElement;
	inline static const FName TagMetadataName = TEXT("Tags");
	inline static const FName ShortModuleNameMetadataName = TEXT("ShortModuleName");
	inline static const FName DesiredNameMetadataName = TEXT("DesiredName");
	inline static const FName DesiredKeyMetadataName = TEXT("DesiredKey");
	inline static const FName ModuleMetadataName = TEXT("Module");
	inline static const FName NameSpaceMetadataName = TEXT("NameSpace");

	UTLLRN_RigHierarchy();

	// UObject interface
	virtual void BeginDestroy() override;
	virtual void Serialize(FArchive& Ar) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	void Save(FArchive& Ar);
	void Load(FArchive& Ar);
	virtual void PostLoad() override;
#if WITH_EDITORONLY_DATA
	static void DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass);
#endif
#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif

	/**
	 * Clears the whole hierarchy and removes all elements.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void Reset();

	/**
	 * Resets the hierarchy to the state of its default. This refers to the
	 * hierarchy on the default object.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void ResetToDefault();

	/**
	 * Copies the contents of a hierarchy onto this one
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void CopyHierarchy(UTLLRN_RigHierarchy* InHierarchy);

	bool IsCopyingHierarchy() const { return bIsCopyingHierarchy; }

	/**
	 * Returns true if the hierarchy currently has an execute context / the rig is running
	 */
	bool HasExecuteContext() const { return ExecuteContext != nullptr; }

	/**
	 * Returns a hash for the hierarchy representing all names
	 * as well as the topology version.
	 */
	uint32 GetNameHash() const;

	/**
	 * Returns a hash representing the topological state of the hierarchy
	 */
	uint32 GetTopologyHash(bool bIncludeTopologyVersion = true, bool bIncludeTransientControls = false) const;

#if WITH_EDITOR
	/**
	* Add dependent hierarchies that listens to changes made to this hierarchy
	* Note: By default, only changes to the initial states of this hierarchy is mirrored to the listening hierarchies
	*/	
	void RegisterListeningHierarchy(UTLLRN_RigHierarchy* InHierarchy);
	
	/**
	* Remove dependent hierarchies that listens to changes made to this hierarchy
	*/	
	void UnregisterListeningHierarchy(UTLLRN_RigHierarchy* InHierarchy);
	
	void ClearListeningHierarchy();
#endif

	/**
	 * Returns the default hierarchy for this hierarchy (or nullptr)
	 */
	UTLLRN_RigHierarchy* GetDefaultHierarchy() { return DefaultHierarchyPtr.Get(); }

public:
	/**
	 * Copies the contents of a hierarchy onto this one
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void CopyPose(UTLLRN_RigHierarchy* InHierarchy, bool bCurrent, bool bInitial, bool bWeights, bool bMatchPoseInGlobalIfNeeded = false);

	/**
	 * Update all elements that depend on external references
	 */
	void UpdateReferences(const FRigVMExecuteContext* InContext);

	/**
	 * Resets the current pose of a filtered list of elements to the initial / ref pose.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void ResetPoseToInitial(ETLLRN_RigElementType InTypeFilter);

	/**
	 * Resets the current pose of all elements to the initial / ref pose.
	 */
	void ResetPoseToInitial()
	{
		ResetPoseToInitial(ETLLRN_RigElementType::All);
	}

	/**
	 * Resets all curves to 0.0
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void ResetCurveValues();

	/**
	 * Resets all curves to be unset (defaulting to 0.0)
	 */
	void UnsetCurveValues(bool bSetupUndo = false);

	/**
	 * Returns all changed curve values
	 */
	const TArray<int32>& GetChangedCurveIndices() const;

	/**
	 * Returns all changed curve values
	 */
	void ResetChangedCurveIndices();

	/**
	 * Returns the flag used decide if we should be recording curve changes
	 */
	bool& GetRecordCurveChangesFlag() { return bRecordCurveChanges; }
	
	/**
	 * Returns the number of elements in the Hierarchy.
	 * @return The number of elements in the Hierarchy
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	int32 Num() const
	{
		return Elements.Num();
	}

	/**
	 * Returns the number of elements in the Hierarchy.
	 * @param InElementType The type filter to apply
	 * @return The number of elements in the Hierarchy
	 */
    int32 Num(ETLLRN_RigElementType InElementType) const;

	// iterators
	TArray<FTLLRN_RigBaseElement*>::RangedForIteratorType      begin() { return Elements.begin(); }
	TArray<FTLLRN_RigBaseElement*>::RangedForIteratorType      end() { return Elements.end(); }

	/**
	 * Iterator function to invoke a lambda / TFunctionRef for each element
	 * @param PerElementFunction The function to invoke for each element
	 */
	void ForEach(TFunctionRef<bool(FTLLRN_RigBaseElement*)> PerElementFunction) const
	{
		for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
		{
			if(!PerElementFunction(Elements[ElementIndex]))
			{
				return;
			}
		}
	}

	/**
	 * Filtered template Iterator function to invoke a lambda / TFunctionRef for each element of a given type.
	 * @param PerElementFunction The function to invoke for each element of a given type
	 */
	template<typename T>
	void ForEach(TFunctionRef<bool(T*)> PerElementFunction) const
	{
		for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
		{
			if(T* CastElement = Cast<T>(Elements[ElementIndex]))
			{
				if(!PerElementFunction(CastElement))
				{
					return;
				}
			}
		}
	}

	/**
	 * Returns true if the provided element index is valid
	 * @param InElementIndex The index to validate
	 * @return Returns true if the provided element index is valid
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool IsValidIndex(int32 InElementIndex) const
	{
		return Elements.IsValidIndex(InElementIndex);
	}

	/**
	 * Returns true if the provided element key is valid
	 * @param InKey The key to validate
	 * @return Returns true if the provided element key is valid
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Contains", ScriptName = "Contains"))
	bool Contains_ForBlueprint(FTLLRN_RigElementKey InKey) const
	{
		return Contains(InKey);
	}

	/**
	 * Returns true if the provided element key is valid
	 * @param InKey The key to validate
	 * @return Returns true if the provided element key is valid
	 */
	bool Contains(const FTLLRN_RigElementKey& InKey) const
	{
		return GetIndex(InKey) != INDEX_NONE;
	}

	/**
	 * Returns true if the provided element key is valid as a certain typename
	 * @param InKey The key to validate
	 * @return Returns true if the provided element key is valid
	 */
	template<typename T>
	bool Contains(const FTLLRN_RigElementKey& InKey) const
	{
		return Find<T>(InKey) != nullptr;
	}

	/**
	 * Returns true if the provided element is procedural.
	 * @param InKey The key to validate
	 * @return Returns true if the element is procedural
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool IsProcedural(const FTLLRN_RigElementKey& InKey) const;

	/**
	 * Returns true if the provided element is procedural.
	 * @param InElement The element to check
	 * @return Returns true if the element is procedural
	 */
	bool IsProcedural(const FTLLRN_RigBaseElement* InElement) const;

	/**
	 * Returns the index of an element given its key
	 * @param InKey The key of the element to retrieve the index for
	 * @return The index of the element or INDEX_NONE
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get Index", ScriptName = "GetIndex"))
	int32 GetIndex_ForBlueprint(FTLLRN_RigElementKey InKey) const
	{
		return GetIndex(InKey);
	}

	/**
	 * Returns the index of an element given its key
	 * @param InKey The key of the element to retrieve the index for
	 * @return The index of the element or INDEX_NONE
	 */
	int32 GetIndex(const FTLLRN_RigElementKey& InKey) const
	{
		if(ElementKeyRedirector)
		{
			if(FTLLRN_CachedTLLRN_RigElement* TLLRN_CachedTLLRN_RigElement = ElementKeyRedirector->Find(InKey))
			{
				if(TLLRN_CachedTLLRN_RigElement->UpdateCache(this))
				{
					return TLLRN_CachedTLLRN_RigElement->GetIndex();
				}
				return INDEX_NONE;
			}
		}
		
		if(const int32* Index = IndexLookup.Find(InKey))
		{
			return *Index;
		}
		return INDEX_NONE;
	}

	/**
	 * Returns the key and index pair of an element given its key
	 * @param InKey The key of the element to retrieve the information for
	 * @return The key and index pair of the element
	 */
	FTLLRN_RigElementKeyAndIndex GetKeyAndIndex(const FTLLRN_RigElementKey& InKey) const
	{
		return GetKeyAndIndex(GetIndex(InKey));
	};

	/**
	 * Returns the key and index pair of an element given its index
	 * @param InIndex The index of the element to retrieve the information for
	 * @return The key and index pair of the element
	 */
	FTLLRN_RigElementKeyAndIndex GetKeyAndIndex(int32 InIndex) const
	{
		if(const FTLLRN_RigBaseElement* Element = Get(InIndex))
		{
			return Element->GetKeyAndIndex();
		}
		return FTLLRN_RigElementKeyAndIndex();
	};

	/**
	 * Returns the index of an element given its key within its default parent (or root)
	 * @param InKey The key of the element to retrieve the index for
	 * @return The index of the element or INDEX_NONE
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get Local Index", ScriptName = "GetLocalIndex"))
	int32 GetLocalIndex_ForBlueprint(FTLLRN_RigElementKey InKey) const
	{
		return GetLocalIndex(InKey);
	}

	/**
	 * Returns the index of an element given its key within its default parent (or root)
	 * @param InKey The key of the element to retrieve the index for
	 * @return The index of the element or INDEX_NONE
	 */
	int32 GetLocalIndex(const FTLLRN_RigElementKey& InKey) const
	{
		return GetLocalIndex(Find(InKey));
	}

	/**
	 * Returns the indices of an array of keys
	 * @param InKeys The keys of the elements to retrieve the indices for
	 * @return The indices of the elements or INDEX_NONE
	 */
	TArray<int32> GetIndices(const TArray<FTLLRN_RigElementKey>& InKeys) const
	{
		TArray<int32> Indices;
		for(const FTLLRN_RigElementKey& Key : InKeys)
		{
			Indices.Add(GetIndex(Key));
		}
		return Indices;
	}

	/**
	 * Returns the key of an element given its index
	 * @param InElementIndex The index of the element to retrieve the key for
	 * @return The key of an element given its index
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTLLRN_RigElementKey GetKey(int32 InElementIndex) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			return Elements[InElementIndex]->Key;
		}
		return FTLLRN_RigElementKey();
	}

	/**
	 * Returns the keys of an array of indices
	 * @param InElementIndices The indices to retrieve the keys for
	 * @return The keys of the elements given the indices
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	TArray<FTLLRN_RigElementKey> GetKeys(const TArray<int32> InElementIndices) const
	{
		TArray<FTLLRN_RigElementKey> Keys;
		for(int32 Index : InElementIndices)
		{
			Keys.Add(GetKey(Index));
		}
		return Keys;
	}

	/**
	 * Returns an element at a given index or nullptr.
	 * @param InIndex The index of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	const FTLLRN_RigBaseElement* Get(int32 InIndex) const
	{
		if(Elements.IsValidIndex(InIndex))
		{
			return Elements[InIndex];
		}
		return nullptr;
	}

	/**
	 * Returns an element at a given index or nullptr.
	 * @param InIndex The index of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	FTLLRN_RigBaseElement* Get(int32 InIndex)
	{
		if(Elements.IsValidIndex(InIndex))
		{
			return Elements[InIndex];
		}
		return nullptr;
	}

	/**
	 * Returns an element at a given index or nullptr.
	 * This templated method also casts to the chosen
	 * element type but does not guarantee a valid result.
	 * @param InIndex The index of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	template<typename T>
	const T* Get(int32 InIndex) const
	{
		return Cast<T>(Get(InIndex));
	}

	/**
	 * Returns an element at a given index or nullptr.
	 * This templated method also casts to the chosen
	 * element type but does not guarantee a valid result.
	 * @param InIndex The index of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	template<typename T>
	T* Get(int32 InIndex)
	{
		return Cast<T>(Get(InIndex));
	}

	/**
	 * Returns an element at a given index.
	 * This templated method also casts to the chosen
	 * element type and checks for a the valid result.
	 * @param InIndex The index of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	template<typename T>
	const T* GetChecked(int32 InIndex) const
	{
		return CastChecked<T>(Get(InIndex));
	}

	/**
	 * Returns an element at a given index.
	 * This templated method also casts to the chosen
	 * element type and checks for a the valid result.
	 * @param InIndex The index of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	template<typename T>
    T* GetChecked(int32 InIndex)
	{
		return CastChecked<T>(Get(InIndex));
	}

	/**
     * Returns a handle to an existing element
     * @param InKey The key of the handle to retrieve.
     * @return The retrieved handle (may be invalid)
     */
	FTLLRN_RigElementHandle GetHandle(const FTLLRN_RigElementKey& InKey) const
	{
		if(Contains(InKey))
		{
			return FTLLRN_RigElementHandle((UTLLRN_RigHierarchy*)this, InKey);
		}
		return FTLLRN_RigElementHandle();
	}

	/**
	 * Returns an element for a given key or nullptr.
	 * @param InKey The key of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	const FTLLRN_RigBaseElement* Find(const FTLLRN_RigElementKey& InKey) const
	{
		return Get(GetIndex(InKey));
	}

	/**
	 * Returns an element for a given key or nullptr.
	 * @param InKey The key of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	FTLLRN_RigBaseElement* Find(const FTLLRN_RigElementKey& InKey)
	{
		return Get(GetIndex(InKey));
	}

	/**
	 * Returns an element for a given key and raises for invalid results.
	 * @param InKey The key of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	const FTLLRN_RigBaseElement* FindChecked(const FTLLRN_RigElementKey& InKey) const
	{
		const FTLLRN_RigBaseElement* Element = Get(GetIndex(InKey));
		check(Element);
		return Element;
	}

	/**
	 * Returns an element for a given key and raises for invalid results.
	 * @param InKey The key of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	FTLLRN_RigBaseElement* FindChecked(const FTLLRN_RigElementKey& InKey)
	{
		FTLLRN_RigBaseElement* Element = Get(GetIndex(InKey));
		check(Element);
		return Element;
	}

	/**
	 * Returns an element for a given key or nullptr.
	 * This templated method also casts to the chosen
	 * element type but does not guarantee a valid result.
	 * @param InKey The key of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	template<typename T>
	const T* Find(const FTLLRN_RigElementKey& InKey) const
	{
		return Get<T>(GetIndex(InKey));
	}

	/**
	 * Returns an element for a given key or nullptr.
	 * This templated method also casts to the chosen
	 * element type but does not guarantee a valid result.
	 * @param InKey The key of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	template<typename T>
	T* Find(const FTLLRN_RigElementKey& InKey)
	{
		return Get<T>(GetIndex(InKey));
	}

private:
	/**
	* Returns bone element for a given key, for scripting purpose only, for cpp usage, use Find<FTLLRN_RigBoneElement>()
	* @param InKey The key of the bone element to retrieve. 
	*/
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Find Bone", ScriptName = "FindBone"))
    FTLLRN_RigBoneElement FindBone_ForBlueprintOnly(const FTLLRN_RigElementKey& InKey) const
	{
		if (const FTLLRN_RigBoneElement* Bone = Find<FTLLRN_RigBoneElement>(InKey))
		{
			return *Bone;
		}
		return FTLLRN_RigBoneElement{};
	}	
	
	/**
	* Returns control element for a given key, for scripting purpose only, for cpp usage, use Find<FTLLRN_RigControlElement>()
	* @param InKey The key of the control element to retrieve. 
	*/
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Find Control", ScriptName = "FindControl"))
    FTLLRN_RigControlElement FindControl_ForBlueprintOnly(const FTLLRN_RigElementKey& InKey) const
	{
		if (const FTLLRN_RigControlElement* Control = Find<FTLLRN_RigControlElement>(InKey))
		{
			return *Control;
		}
		return FTLLRN_RigControlElement{};
	}	

	/**
	* Returns null element for a given key, for scripting purpose only, for cpp usage, use Find<FTLLRN_RigControlElement>()
	* @param InKey The key of the null element to retrieve. 
	*/
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Find Null", ScriptName = "FindNull"))
    FTLLRN_RigNullElement FindNull_ForBlueprintOnly(const FTLLRN_RigElementKey& InKey) const
	{
		if (const FTLLRN_RigNullElement* Null = Find<FTLLRN_RigNullElement>(InKey))
		{
			return *Null;
		}
		return FTLLRN_RigNullElement{};
	}
	
public:	
	/**
	 * Returns an element for a given key.
	 * This templated method also casts to the chosen
	 * element type and checks for a the valid result.
	 * @param InKey The key of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	template<typename T>
	const T* FindChecked(const FTLLRN_RigElementKey& InKey) const
	{
		return GetChecked<T>(GetIndex(InKey));
	}

	/**
	 * Returns an element for a given key.
	 * This templated method also casts to the chosen
	 * element type and checks for a the valid result.
	 * @param InKey The key of the element to retrieve.
	 * @return The retrieved element or nullptr.
	 */
	template<typename T>
    T* FindChecked(const FTLLRN_RigElementKey& InKey)
	{
		return GetChecked<T>(GetIndex(InKey));
	}

	/**
	 * Filtered accessor to retrieve all elements of a given type
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	template<typename T>
	TArray<T*> GetElementsOfType(bool bTraverse = false) const
	{
		TArray<T*> Results;

		if(bTraverse)
		{
			TArray<bool> ElementVisited;
			ElementVisited.AddZeroed(Elements.Num());

			Traverse([&ElementVisited, &Results](FTLLRN_RigBaseElement* InElement, bool& bContinue)
			{
			    bContinue = !ElementVisited[InElement->GetIndex()];

			    if(bContinue)
			    {
			        if(T* CastElement = Cast<T>(InElement))
			        {
			            Results.Add(CastElement);
			        }
			        ElementVisited[InElement->GetIndex()] = true;
			    }
			});
		}
		else
		{
			for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
			{
				if(T* CastElement = Cast<T>(Elements[ElementIndex]))
				{
					Results.Add(CastElement);
				}
			}
		}
		return Results;
	}

	/**
	 * Filtered accessor to retrieve all element keys of a given type
	 * @param bTraverse Returns the element keys in order of a depth first traversal
	 */
	template<typename T>
    TArray<FTLLRN_RigElementKey> GetKeysOfType(bool bTraverse = false) const
	{
		TArray<FTLLRN_RigElementKey> Keys;
		TArray<T*> Results = GetElementsOfType<T>(bTraverse);
		for(T* Element : Results)
		{
			Keys.Add(Element->GetKey());
		}
		return Keys;
	}

	/**
	 * Filtered accessor to retrieve all elements of a given type
	 * @param InKeepElementFunction A function to return true if an element is to be keep
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	template<typename T>
    TArray<T*> GetFilteredElements(TFunction<bool(T*)> InKeepElementFunction, bool bTraverse = false) const
	{
		TArray<T*> Results;

		if(bTraverse)
		{
			TArray<bool> ElementVisited;
			ElementVisited.AddZeroed(Elements.Num());
		
			Traverse([&ElementVisited, &Results, InKeepElementFunction](FTLLRN_RigBaseElement* InElement, bool& bContinue)
            {
                bContinue = !ElementVisited[InElement->GetIndex()];

                if(bContinue)
                {
                    if(T* CastElement = Cast<T>(InElement))
                    {
						if(InKeepElementFunction(CastElement))
						{
							Results.Add(CastElement);
						}
                    }
                    ElementVisited[InElement->GetIndex()] = true;
                }
            });
		}
		else
		{
			for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
			{
				if(T* CastElement = Cast<T>(Elements[ElementIndex]))
				{
					if(InKeepElementFunction(CastElement))
					{
						Results.Add(CastElement);
					}
				}
			}
		}
		return Results;
	}

	/**
	 * Returns all Bone elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	TArray<FTLLRN_RigBoneElement*> GetBones(bool bTraverse = false) const
	{
		return GetElementsOfType<FTLLRN_RigBoneElement>(bTraverse);
	}
	
	/**
   	 * Returns all Bone elements without traversing the hierarchy
   	 */
   	TArray<FTLLRN_RigBaseElement*>& GetBonesFast() const
   	{
   		return ElementsPerType[TLLRN_RigElementTypeToFlatIndex(ETLLRN_RigElementType::Bone)];
   	}

	/**
	 * Returns all Bone elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get Bones", ScriptName = "GetBones"))
	TArray<FTLLRN_RigElementKey> GetBoneKeys(bool bTraverse = true) const
	{
		return GetKeysOfType<FTLLRN_RigBoneElement>(bTraverse);
	}

	/**
	 * Returns all Null elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	TArray<FTLLRN_RigNullElement*> GetNulls(bool bTraverse = false) const
	{
		return GetElementsOfType<FTLLRN_RigNullElement>(bTraverse);
	}

	/**
	* Returns all Null elements
	* @param bTraverse Returns the elements in order of a depth first traversal
	*/
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get Nulls", ScriptName = "GetNulls"))
	TArray<FTLLRN_RigElementKey> GetNullKeys(bool bTraverse = true) const
	{
		return GetKeysOfType<FTLLRN_RigNullElement>(bTraverse);
	}

	/**
	 * Returns all Control elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	TArray<FTLLRN_RigControlElement*> GetControls(bool bTraverse = false) const
	{
		return GetElementsOfType<FTLLRN_RigControlElement>(bTraverse);
	}

	/**
	 * Returns all Control elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get Controls", ScriptName = "GetControls"))
	TArray<FTLLRN_RigElementKey> GetControlKeys(bool bTraverse = true) const
	{
		return GetKeysOfType<FTLLRN_RigControlElement>(bTraverse);
	}

	/**
	 * Returns all transient Control elements
	 */
	TArray<FTLLRN_RigControlElement*> GetTransientControls() const
	{
		return GetFilteredElements<FTLLRN_RigControlElement>([](FTLLRN_RigControlElement* ControlElement) -> bool
		{
			return ControlElement->Settings.bIsTransientControl;
		});
	}

	/**
	 * Returns all Curve elements
	 */
	TArray<FTLLRN_RigCurveElement*> GetCurves() const
	{
		return GetElementsOfType<FTLLRN_RigCurveElement>();
	}

	/**
	 * Returns all Curve elements without traversing the hierarchy
	 */
	TArray<FTLLRN_RigBaseElement*>& GetCurvesFast() const
	{
		return ElementsPerType[TLLRN_RigElementTypeToFlatIndex(ETLLRN_RigElementType::Curve)];
	}

	/**
	 * Returns all Curve elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get Curves", ScriptName = "GetCurves"))
	TArray<FTLLRN_RigElementKey> GetCurveKeys() const
	{
		return GetKeysOfType<FTLLRN_RigCurveElement>(false);
	}

	/**
	 * Returns all Physics elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	TArray<FTLLRN_RigPhysicsElement*> GetPhysicsElements(bool bTraverse = false) const
	{
		return GetElementsOfType<FTLLRN_RigPhysicsElement>(bTraverse);
	}

	/**
	 * Returns all Physics elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get Physics Keys", ScriptName = "GetPhysicsKeys"))
    TArray<FTLLRN_RigElementKey> GetPhysicsKeys(bool bTraverse = true) const
	{
		return GetKeysOfType<FTLLRN_RigPhysicsElement>(bTraverse);
	}

	/**
	 * Returns all references
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	TArray<FTLLRN_RigReferenceElement*> GetReferences(bool bTraverse = false) const
	{
		return GetElementsOfType<FTLLRN_RigReferenceElement>(bTraverse);
	}

	/**
	 * Returns all references
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get References", ScriptName = "GetReferences"))
    TArray<FTLLRN_RigElementKey> GetReferenceKeys(bool bTraverse = true) const
	{
		return GetKeysOfType<FTLLRN_RigReferenceElement>(bTraverse);
	}

	/**
	 * Returns all Connector elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	TArray<FTLLRN_RigConnectorElement*> GetConnectors(bool bTraverse = false) const
	{
		return GetElementsOfType<FTLLRN_RigConnectorElement>(bTraverse);
	}

	/**
	 * Returns all Connector elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get Connectors", ScriptName = "GetConnectors"))
	TArray<FTLLRN_RigElementKey> GetConnectorKeys(bool bTraverse = true) const
	{
		return GetKeysOfType<FTLLRN_RigConnectorElement>(bTraverse);
	}

	/**
	 * Returns all of the sockets' state
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FTLLRN_RigSocketState> GetSocketStates() const;

	/**
	 * Try to restore the sockets from the state structs
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	TArray<FTLLRN_RigElementKey> RestoreSocketsFromStates(TArray<FTLLRN_RigSocketState> InStates, bool bSetupUndoRedo = false);

	/**
	 * Returns all of the connectors' state
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FTLLRN_RigConnectorState> GetConnectorStates() const;

	/**
	 * Try to restore the connectors from the state structs
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	TArray<FTLLRN_RigElementKey> RestoreConnectorsFromStates(TArray<FTLLRN_RigConnectorState> InStates, bool bSetupUndoRedo = false);

	/**
	 * Returns all Socket elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	TArray<FTLLRN_RigSocketElement*> GetSockets(bool bTraverse = false) const
	{
		return GetElementsOfType<FTLLRN_RigSocketElement>(bTraverse);
	}

	/**
	 * Returns all Socket elements
	 * @param bTraverse Returns the elements in order of a depth first traversal
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get Sockets", ScriptName = "GetSockets"))
	TArray<FTLLRN_RigElementKey> GetSocketKeys(bool bTraverse = true) const
	{
		return GetKeysOfType<FTLLRN_RigSocketElement>(bTraverse);
	}

	/**
	 * Returns all root elements
	 */
	TArray<FTLLRN_RigBaseElement*> GetRootElements() const
	{
		return GetFilteredElements<FTLLRN_RigBaseElement>([this](FTLLRN_RigBaseElement* Element) -> bool
		{
			return GetNumberOfParents(Element) == 0;
		});
	}

	/**
	 * Returns all root element keys
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get Root Elements", ScriptName = "GetRootElements"))
	TArray<FTLLRN_RigElementKey> GetRootElementKeys() const
	{
		return GetKeysByPredicate([this](const FTLLRN_RigBaseElement& Element) -> bool
		{
			return GetNumberOfParents(Element.Index) == 0;
		}, false);
	}

	/**
	 * Finds a new physics solver given its guid
	 * @param InID The id identifying the physics solver
	 * @return The physics solver
	 */
	const FTLLRN_RigPhysicsSolverDescription* FindPhysicsSolver(const FTLLRN_RigPhysicsSolverID& InID) const; 

	/**
	 * Finds a new physics solver given its name
	 * @param InName The name identifying the physics solver in the scope of this hierarchy
	 * @return The physics solver
	 */
	const FTLLRN_RigPhysicsSolverDescription* FindPhysicsSolverByName(const FName& InName) const; 

	/**
	 * Returns the name of metadata for a given element
	 * @param InItem The element key to return the metadata keys for
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FName> GetMetadataNames(FTLLRN_RigElementKey InItem) const;

	/**
	 * Returns the type of metadata given its name the item it is stored under
	 * @param InItem The element key to return the metadata type for
	 * @param InMetadataName The name of the metadata to return the type for
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	ETLLRN_RigMetadataType GetMetadataType(FTLLRN_RigElementKey InItem, FName InMetadataName) const;

	/**
	 * Removes the metadata under a given element 
	 * @param InItem The element key to search under
	 * @param InMetadataName The name of the metadata to remove
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool RemoveMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName);

	/**
     * Removes all of the metadata under a given item 
	 * @param InItem The element key to search under
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool RemoveAllMetadata(FTLLRN_RigElementKey InItem);

	/**
	 * Queries and returns the value of bool metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 * @param DefaultValue The default value to fall back on
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	bool GetBoolMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, bool DefaultValue) const
	{
		return GetMetadata<bool>(InItem, ETLLRN_RigMetadataType::Bool, InMetadataName, DefaultValue);
	}

	/**
	 * Queries and returns the value of bool array metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<bool> GetBoolArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName) const
	{
		return GetArrayMetadata<bool>(InItem, ETLLRN_RigMetadataType::BoolArray, InMetadataName);
	}

	/**
	 * Sets the metadata to a bool value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetBoolMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, bool InValue)
	{
		return SetMetadata<bool>(InItem, ETLLRN_RigMetadataType::Bool, InMetadataName, InValue);
	}

	/**
	 * Sets the metadata to a bool array value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetBoolArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, TArray<bool> InValue)
	{
		return SetArrayMetadata<bool>(InItem, ETLLRN_RigMetadataType::BoolArray, InMetadataName, InValue);
	}

	/**
	 * Queries and returns the value of float metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 * @param DefaultValue The default value to fall back on
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	float GetFloatMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, float DefaultValue) const
	{
		return GetMetadata<float>(InItem, ETLLRN_RigMetadataType::Float, InMetadataName, DefaultValue);
	}

	/**
	 * Queries and returns the value of float array metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<float> GetFloatArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName) const
	{
		return GetArrayMetadata<float>(InItem, ETLLRN_RigMetadataType::FloatArray, InMetadataName);
	}

	/**
	 * Sets the metadata to a float value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetFloatMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, float InValue)
	{
		return SetMetadata<float>(InItem, ETLLRN_RigMetadataType::Float, InMetadataName, InValue);
	}

	/**
	 * Sets the metadata to a float array value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetFloatArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, TArray<float> InValue)
	{
		return SetArrayMetadata<float>(InItem, ETLLRN_RigMetadataType::FloatArray, InMetadataName, InValue);
	}

	/**
	 * Queries and returns the value of int32 metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 * @param DefaultValue The default value to fall back on
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	int32 GetInt32Metadata(FTLLRN_RigElementKey InItem, FName InMetadataName, int32 DefaultValue) const
	{
		return GetMetadata<int32>(InItem, ETLLRN_RigMetadataType::Int32, InMetadataName, DefaultValue);
	}

	/**
	 * Queries and returns the value of int32 array metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<int32> GetInt32ArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName) const
	{
		return GetArrayMetadata<int32>(InItem, ETLLRN_RigMetadataType::Int32Array, InMetadataName);
	}

	/**
	 * Sets the metadata to a int32 value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetInt32Metadata(FTLLRN_RigElementKey InItem, FName InMetadataName, int32 InValue)
	{
		return SetMetadata<int32>(InItem, ETLLRN_RigMetadataType::Int32, InMetadataName, InValue);
	}

	/**
	 * Sets the metadata to a int32 array value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetInt32ArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, TArray<int32> InValue)
	{
		return SetArrayMetadata<int32>(InItem, ETLLRN_RigMetadataType::Int32Array, InMetadataName, InValue);
	}

	/**
	 * Queries and returns the value of FName metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 * @param DefaultValue The default value to fall back on
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FName GetNameMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FName DefaultValue) const
	{
		return GetMetadata<FName>(InItem, ETLLRN_RigMetadataType::Name, InMetadataName, DefaultValue);
	}

	/**
	 * Queries and returns the value of FName array metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FName> GetNameArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName) const
	{
		return GetArrayMetadata<FName>(InItem, ETLLRN_RigMetadataType::NameArray, InMetadataName);
	}

	/**
	 * Sets the metadata to a FName value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetNameMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FName InValue)
	{
		return SetMetadata<FName>(InItem, ETLLRN_RigMetadataType::Name, InMetadataName, InValue);
	}

	/**
	 * Sets the metadata to a FName array value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetNameArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, TArray<FName> InValue)
	{
		return SetArrayMetadata<FName>(InItem, ETLLRN_RigMetadataType::NameArray, InMetadataName, InValue);
	}

	/**
	 * Queries and returns the value of FVector metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 * @param DefaultValue The default value to fall back on
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FVector GetVectorMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FVector DefaultValue) const
	{
		return GetMetadata<FVector>(InItem, ETLLRN_RigMetadataType::Vector, InMetadataName, DefaultValue);
	}

	/**
	 * Queries and returns the value of FVector array metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FVector> GetVectorArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName) const
	{
		return GetArrayMetadata<FVector>(InItem, ETLLRN_RigMetadataType::VectorArray, InMetadataName);
	}

	/**
	 * Sets the metadata to a FVector value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetVectorMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FVector InValue)
	{
		return SetMetadata<FVector>(InItem, ETLLRN_RigMetadataType::Vector, InMetadataName, InValue);
	}

	/**
	 * Sets the metadata to a FVector array value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetVectorArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, TArray<FVector> InValue)
	{
		return SetArrayMetadata<FVector>(InItem, ETLLRN_RigMetadataType::VectorArray, InMetadataName, InValue);
	}

	/**
	 * Queries and returns the value of FRotator metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 * @param DefaultValue The default value to fall back on
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FRotator GetRotatorMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FRotator DefaultValue) const
	{
		return GetMetadata<FRotator>(InItem, ETLLRN_RigMetadataType::Rotator, InMetadataName, DefaultValue);
	}

	/**
	 * Queries and returns the value of FRotator array metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FRotator> GetRotatorArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName) const
	{
		return GetArrayMetadata<FRotator>(InItem, ETLLRN_RigMetadataType::RotatorArray, InMetadataName);
	}

	/**
	 * Sets the metadata to a FRotator value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetRotatorMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FRotator InValue)
	{
		return SetMetadata<FRotator>(InItem, ETLLRN_RigMetadataType::Rotator, InMetadataName, InValue);
	}

	/**
	 * Sets the metadata to a FRotator array value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetRotatorArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, TArray<FRotator> InValue)
	{
		return SetArrayMetadata<FRotator>(InItem, ETLLRN_RigMetadataType::RotatorArray, InMetadataName, InValue);
	}

	/**
	 * Queries and returns the value of FQuat metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 * @param DefaultValue The default value to fall back on
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FQuat GetQuatMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FQuat DefaultValue) const
	{
		return GetMetadata<FQuat>(InItem, ETLLRN_RigMetadataType::Quat, InMetadataName, DefaultValue);
	}

	/**
	 * Queries and returns the value of FQuat array metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FQuat> GetQuatArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName) const
	{
		return GetArrayMetadata<FQuat>(InItem, ETLLRN_RigMetadataType::QuatArray, InMetadataName);
	}

	/**
	 * Sets the metadata to a FQuat value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetQuatMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FQuat InValue)
	{
		return SetMetadata<FQuat>(InItem, ETLLRN_RigMetadataType::Quat, InMetadataName, InValue);
	}

	/**
	 * Sets the metadata to a FQuat array value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetQuatArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, TArray<FQuat> InValue)
	{
		return SetArrayMetadata<FQuat>(InItem, ETLLRN_RigMetadataType::QuatArray, InMetadataName, InValue);
	}

	/**
	 * Queries and returns the value of FTransform metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 * @param DefaultValue The default value to fall back on
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FTransform GetTransformMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FTransform DefaultValue) const
	{
		return GetMetadata<FTransform>(InItem, ETLLRN_RigMetadataType::Transform, InMetadataName, DefaultValue);
	}

	/**
	 * Queries and returns the value of FTransform array metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FTransform> GetTransformArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName) const
	{
		return GetArrayMetadata<FTransform>(InItem, ETLLRN_RigMetadataType::TransformArray, InMetadataName);
	}

	/**
	 * Sets the metadata to a FTransform value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetTransformMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FTransform InValue)
	{
		return SetMetadata<FTransform>(InItem, ETLLRN_RigMetadataType::Transform, InMetadataName, InValue);
	}

	/**
	 * Sets the metadata to a FTransform array value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetTransformArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, TArray<FTransform> InValue)
	{
		return SetArrayMetadata<FTransform>(InItem, ETLLRN_RigMetadataType::TransformArray, InMetadataName, InValue);
	}

		/**
	 * Queries and returns the value of FLinearColor metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 * @param DefaultValue The default value to fall back on
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FLinearColor GetLinearColorMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FLinearColor DefaultValue) const
	{
		return GetMetadata<FLinearColor>(InItem, ETLLRN_RigMetadataType::LinearColor, InMetadataName, DefaultValue);
	}

	/**
	 * Queries and returns the value of FLinearColor array metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FLinearColor> GetLinearColorArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName) const
	{
		return GetArrayMetadata<FLinearColor>(InItem, ETLLRN_RigMetadataType::LinearColorArray, InMetadataName);
	}

	/**
	 * Sets the metadata to a FLinearColor value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetLinearColorMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FLinearColor InValue)
	{
		return SetMetadata<FLinearColor>(InItem, ETLLRN_RigMetadataType::LinearColor, InMetadataName, InValue);
	}

	/**
	 * Sets the metadata to a FLinearColor array value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetLinearColorArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, TArray<FLinearColor> InValue)
	{
		return SetArrayMetadata<FLinearColor>(InItem, ETLLRN_RigMetadataType::LinearColorArray, InMetadataName, InValue);
	}

	/**
	 * Queries and returns the value of FTLLRN_RigElementKey metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 * @param DefaultValue The default value to fall back on
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FTLLRN_RigElementKey GetTLLRN_RigElementKeyMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FTLLRN_RigElementKey DefaultValue) const
	{
		return GetMetadata<FTLLRN_RigElementKey>(InItem, ETLLRN_RigMetadataType::TLLRN_RigElementKey, InMetadataName, DefaultValue);
	}

	/**
	 * Queries and returns the value of FTLLRN_RigElementKey array metadata
	 * @param InItem The element key to return the metadata for
	 * @param InMetadataName The name of the metadata to query
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FTLLRN_RigElementKey> GetTLLRN_RigElementKeyArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName) const
	{
		return GetArrayMetadata<FTLLRN_RigElementKey>(InItem, ETLLRN_RigMetadataType::TLLRN_RigElementKeyArray, InMetadataName);
	}

	/**
	 * Sets the metadata to a FTLLRN_RigElementKey value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetTLLRN_RigElementKeyMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, FTLLRN_RigElementKey InValue)
	{
		return SetMetadata<FTLLRN_RigElementKey>(InItem, ETLLRN_RigMetadataType::TLLRN_RigElementKey, InMetadataName, InValue);
	}

	/**
	 * Sets the metadata to a FTLLRN_RigElementKey array value
	 * @param InItem The element key to set the metadata for
	 * @param InMetadataName The name of the metadata to set
	 * @param InValue The value to set
	 * @return Returns true if setting the metadata was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetTLLRN_RigElementKeyArrayMetadata(FTLLRN_RigElementKey InItem, FName InMetadataName, TArray<FTLLRN_RigElementKey> InValue)
	{
		return SetArrayMetadata<FTLLRN_RigElementKey>(InItem, ETLLRN_RigMetadataType::TLLRN_RigElementKeyArray, InMetadataName, InValue);
	}

	/**
	 * Returns the path of the module an element belong to (or NAME_None in case the element doesn't belong to a module)
	 * @return The path the element belongs to (or NAME_None)
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FName GetModulePathFName(FTLLRN_RigElementKey InItem) const;
	
	/**
	 * Returns the path of the module an element belong to (or an empty string in case the element doesn't belong to a module)
	 * @return The path the element belongs to (or empty string)
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FString GetModulePath(FTLLRN_RigElementKey InItem) const;

	/**
	 * Returns the namespace of an element belong to (or NAME_None in case the element doesn't belong to a module / namespace)
	 * @return The namespace the element belongs to (or NAME_None)
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FName GetNameSpaceFName(FTLLRN_RigElementKey InItem) const;
	
	/**
	 * Returns the namespace of an element belong to (or an empty string in case the element doesn't belong to a module / namespace)
	 * @return The namespace the element belongs to (or empty string)
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	FString GetNameSpace(FTLLRN_RigElementKey InItem) const;

	/*
	 * Returns the tags for a given item
	 * @param InItem The item to return the tags for
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	TArray<FName> GetTags(FTLLRN_RigElementKey InItem) const
	{
		return GetNameArrayMetadata(InItem, TagMetadataName);
	}

	/*
	 * Returns true if a given item has a certain tag
	 * @param InItem The item to return the tags for
	 * @param InTag The tag to check
	 */
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	bool HasTag(FTLLRN_RigElementKey InItem, FName InTag) const
	{
		return GetTags(InItem).Contains(InTag);
	}

	/*
     * Sets a tag on an element in the hierarchy
     * @param InItem The item to set the tag for
     * @param InTag The tag to set
     */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetTag(FTLLRN_RigElementKey InItem, FName InTag)
	{
		TArray<FName> Tags = GetTags(InItem);
		Tags.AddUnique(InTag);
		return SetNameArrayMetadata(InItem, TagMetadataName, Tags);
	}

	/**
	 * Returns the selected elements
	 * @InTypeFilter The types to retrieve the selection for
	 * @return An array of the currently selected elements
	 */
	TArray<const FTLLRN_RigBaseElement*> GetSelectedElements(ETLLRN_RigElementType InTypeFilter = ETLLRN_RigElementType::All) const;

	/**
	 * Returns the keys of selected elements
	 * @InTypeFilter The types to retrieve the selection for
	 * @return An array of the currently selected elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	TArray<FTLLRN_RigElementKey> GetSelectedKeys(ETLLRN_RigElementType InTypeFilter = ETLLRN_RigElementType::All) const;

	/**
	 * Returns true if a given element is selected
	 * @param InKey The key to check
	 * @return true if a given element is selected
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool IsSelected(FTLLRN_RigElementKey InKey) const
	{
		return IsSelected(Find(InKey));
	}

	/**
	 * Returns true if a given element is selected
	 * @param InIndex The index to check
	 * @return true if a given element is selected
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    bool IsSelectedByIndex(int32 InIndex) const
	{
		return IsSelected(Get(InIndex));
	}

	bool IsSelected(int32 InIndex) const
	{
		return IsSelectedByIndex(InIndex);
	}

	/**
	 * Sorts the input key list by traversing the hierarchy
	 * @param InKeys The keys to sort
	 * @return The sorted keys
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	TArray<FTLLRN_RigElementKey> SortKeys(const TArray<FTLLRN_RigElementKey>& InKeys) const
	{
		TArray<FTLLRN_RigElementKey> Result;
		Traverse([InKeys, &Result](FTLLRN_RigBaseElement* Element, bool& bContinue)
        {
            const FTLLRN_RigElementKey& Key = Element->GetKey();
            if(InKeys.Contains(Key))
            {
                Result.AddUnique(Key);
            }
        });
		return Result;
	}

	/**
	 * Returns the two name sections with the right namespace separator
	 */
	static FString JoinNameSpace(const FString& InLeft, const FString& InRight);
	static FTLLRN_RigName JoinNameSpace(const FTLLRN_RigName& InLeft, const FTLLRN_RigName& InRight);

	/**
	 * Returns the two name sections with the right namespace separator
	 */
	static TPair<FString, FString> SplitNameSpace(const FString& InNameSpacedPath, bool bFromEnd = true);
	static TPair<FTLLRN_RigName, FTLLRN_RigName> SplitNameSpace(const FTLLRN_RigName& InNameSpacedPath, bool bFromEnd = true);
	static bool SplitNameSpace(const FString& InNameSpacedPath, FString* OutNameSpace, FString* OutName, bool bFromEnd = true);
	static bool SplitNameSpace(const FTLLRN_RigName& InNameSpacedPath, FTLLRN_RigName* OutNameSpace, FTLLRN_RigName* OutName, bool bFromEnd = true);

	/**
	 * Returns the max allowed length for a name within the hierarchy.
	 * @return Returns the max allowed length for a name within the hierarchy.
	 */
	static int32 GetMaxNameLength() { return 100; }

	/**
	 * Sanitizes a name by removing invalid characters.
	 * @param InOutName The name to sanitize in place.
	 */
	static void SanitizeName(FTLLRN_RigName& InOutName, bool bAllowNameSpaces = true);

	/**
	 * Sanitizes a name by removing invalid characters.
	 * @param InName The name to sanitize.
	 * @return The sanitized name.
 	 */
	static FTLLRN_RigName GetSanitizedName(const FTLLRN_RigName& InName, bool bAllowNameSpaces = true);

	/**
	 * Returns true if a given name is available.
	 * @param InPotentialNewName The name to test for availability
	 * @param InType The type of the to-be-added element
	 * @param OutErrorMessage An optional pointer to return a potential error message 
	 * @return Returns true if the name is available.
	 */
	bool IsNameAvailable(const FTLLRN_RigName& InPotentialNewName, ETLLRN_RigElementType InType, FString* OutErrorMessage = nullptr) const;

	/**
	 * Returns true if a given display name is available.
	 * @param InParentElement The element to check the display name under
	 * @param InPotentialNewDisplayName The name to test for availability
	 * @param OutErrorMessage An optional pointer to return a potential error message 
	 * @return Returns true if the name is available.
	 */
	bool IsDisplayNameAvailable(const FTLLRN_RigElementKey& InParentElement, const FTLLRN_RigName& InPotentialNewDisplayName, FString* OutErrorMessage = nullptr) const;

	/**
	 * Returns a valid new name for a to-be-added element.
	 * @param InPotentialNewName The name to be sanitized and adjusted for availability
	 * @param InType The type of the to-be-added element
	 * @param bAllowNameSpace If true the name will be allowed to contain namespaces
	 * @return Returns the name to use for the to-be-added element.
	 */
	FTLLRN_RigName GetSafeNewName(const FTLLRN_RigName& InPotentialNewName, ETLLRN_RigElementType InType, bool bAllowNameSpace = false) const;

	/**
	 * Returns a valid new display name for a control
	 * @param InParentElement The element to check the display name under
	 * @param InPotentialNewDisplayName The name to be sanitized and adjusted for availability
	 * @return Returns the name to use for the to-be-added element.
	 */
	FTLLRN_RigName GetSafeNewDisplayName(const FTLLRN_RigElementKey& InParentElement, const FTLLRN_RigName& InPotentialNewDisplayName) const;

	/**
	 * Returns the display label for an element to be used for the UI
	 */
	FText GetDisplayNameForUI(const FTLLRN_RigBaseElement* InElement, bool bIncludeNameSpace = true) const;
	FText GetDisplayNameForUI(const FTLLRN_RigElementKey& InKey, bool bIncludeNameSpace = true) const;

	/**
	 * Returns the modified event, which can be used to 
	 * subscribe to topological changes happening within the hierarchy.
	 * @return The event used for subscription.
	 */
	FTLLRN_RigHierarchyModifiedEvent& OnModified() { return ModifiedEvent; }

	/**
	 * Returns the MetadataChanged event, which can be used to track metadata changes
	 * Note: This notification has a very high volume - so the consequences of subscribing
	 * to it may cause performance slowdowns.
	 */
	FTLLRN_RigHierarchyMetadataChangedDelegate& OnMetadataChanged() { return MetadataChangedDelegate; }

	/**
	 * Returns the MetadataTagChanged event, which can be used to track metadata tag changes
	 * Note: This notification has a very high volume - so the consequences of subscribing
	 * to it may cause performance slowdowns.
	 */
	FTLLRN_RigHierarchyMetadataTagChangedDelegate& OnMetadataTagChanged() { return MetadataTagChangedDelegate; }

	/**
	 * Returns the local current or initial value for a given key.
	 * If the key is invalid FTransform::Identity will be returned.
	 * @param InKey The key to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The local current or initial transform's value.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTransform GetLocalTransform(FTLLRN_RigElementKey InKey, bool bInitial = false) const
	{
		return GetLocalTransformByIndex(GetIndex(InKey), bInitial);
	}

	/**
	 * Returns the local current or initial value for a element index.
	 * If the index is invalid FTransform::Identity will be returned.
	 * @param InElementIndex The index to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The local current or initial transform's value.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FTransform GetLocalTransformByIndex(int32 InElementIndex, bool bInitial = false) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Elements[InElementIndex]))
			{
				return GetTransform(TransformElement, bInitial ? ETLLRN_RigTransformType::InitialLocal : ETLLRN_RigTransformType::CurrentLocal);
			}
		}
		return FTransform::Identity;
	}

	FTransform GetLocalTransform(int32 InElementIndex) const
	{
		return GetLocalTransformByIndex(InElementIndex, false);
	}
	FTransform GetInitialLocalTransform(int32 InElementIndex) const
	{
		return GetLocalTransformByIndex(InElementIndex, true);
	}

	FTransform GetInitialLocalTransform(const FTLLRN_RigElementKey &InKey) const
	{
		return GetLocalTransform(InKey, true);
	}

	/**
	 * Sets the local current or initial transform for a given key.
	 * @param InKey The key to set the transform for
	 * @param InTransform The new transform value to set
	 * @param bInitial If true the initial transform will be used
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetLocalTransform(FTLLRN_RigElementKey InKey, FTransform InTransform, bool bInitial = false, bool bAffectChildren = true, bool bSetupUndo = false, bool bPrintPythonCommands = false)
	{
		SetLocalTransformByIndex(GetIndex(InKey), InTransform, bInitial, bAffectChildren, bSetupUndo, bPrintPythonCommands);
	}

	/**
	 * Sets the local current or initial transform for a given element index.
	 * @param InElementIndex The index of the element to set the transform for
	 * @param InTransform The new transform value to set
	 * @param bInitial If true the initial transform will be used
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetLocalTransformByIndex(int32 InElementIndex, FTransform InTransform, bool bInitial = false, bool bAffectChildren = true, bool bSetupUndo = false, bool bPrintPythonCommands = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Elements[InElementIndex]))
			{
				SetTransform(TransformElement, InTransform, bInitial ? ETLLRN_RigTransformType::InitialLocal : ETLLRN_RigTransformType::CurrentLocal, bAffectChildren, bSetupUndo, false, bPrintPythonCommands);
			}
		}
	}

	void SetLocalTransform(int32 InElementIndex, const FTransform& InTransform, bool bAffectChildren = true, bool bSetupUndo = false, bool bPrintPythonCommands = false)
	{
		SetLocalTransformByIndex(InElementIndex, InTransform, false, bAffectChildren, bSetupUndo, bPrintPythonCommands);
	}

	void SetInitialLocalTransform(int32 InElementIndex, const FTransform& InTransform, bool bAffectChildren = true, bool bSetupUndo = false, bool bPrintPythonCommands = false)
	{
		SetLocalTransformByIndex(InElementIndex, InTransform, true, bAffectChildren, bSetupUndo, bPrintPythonCommands);
    }

	void SetInitialLocalTransform(const FTLLRN_RigElementKey& InKey, const FTransform& InTransform, bool bAffectChildren = true, bool bSetupUndo = false, bool bPrintPythonCommands = false)
	{
		SetLocalTransform(InKey, InTransform, true, bAffectChildren, bSetupUndo, bPrintPythonCommands);
	}

	/**
	 * Returns the global current or initial value for a given key.
	 * If the key is invalid FTransform::Identity will be returned.
	 * @param InKey The key to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The global current or initial transform's value.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTransform GetGlobalTransform(FTLLRN_RigElementKey InKey, bool bInitial = false) const
	{
		return GetGlobalTransformByIndex(GetIndex(InKey), bInitial);
	}

	/**
	 * Returns the global current or initial value for a element index.
	 * If the index is invalid FTransform::Identity will be returned.
	 * @param InElementIndex The index to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The global current or initial transform's value.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTransform GetGlobalTransformByIndex(int32 InElementIndex, bool bInitial = false) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Elements[InElementIndex]))
			{
				return GetTransform(TransformElement, bInitial ? ETLLRN_RigTransformType::InitialGlobal : ETLLRN_RigTransformType::CurrentGlobal);
			}
		}
		return FTransform::Identity;
	}

	FTransform GetGlobalTransform(int32 InElementIndex) const
	{
		return GetGlobalTransformByIndex(InElementIndex, false);
	}
	FTransform GetInitialGlobalTransform(int32 InElementIndex) const
	{
		return GetGlobalTransformByIndex(InElementIndex, true);
	}

	FTransform GetInitialGlobalTransform(const FTLLRN_RigElementKey &InKey) const
	{
		return GetGlobalTransform(InKey, true);
	}

	/**
	 * Sets the global current or initial transform for a given key.
	 * @param InKey The key to set the transform for
	 * @param InTransform The new transform value to set
	 * @param bInitial If true the initial transform will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetGlobalTransform(FTLLRN_RigElementKey InKey, FTransform InTransform, bool bInitial = false, bool bAffectChildren = true, bool bSetupUndo = false, bool bPrintPythonCommand = false)
	{
		SetGlobalTransformByIndex(GetIndex(InKey), InTransform, bInitial, bAffectChildren, bSetupUndo, bPrintPythonCommand);
	}

	/**
	 * Sets the global current or initial transform for a given element index.
	 * @param InElementIndex The index of the element to set the transform for
	 * @param InTransform The new transform value to set
	 * @param bInitial If true the initial transform will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetGlobalTransformByIndex(int32 InElementIndex, FTransform InTransform, bool bInitial = false, bool bAffectChildren = true, bool bSetupUndo = false, bool bPrintPythonCommand = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Elements[InElementIndex]))
			{
				SetTransform(TransformElement, InTransform, bInitial ? ETLLRN_RigTransformType::InitialGlobal : ETLLRN_RigTransformType::CurrentGlobal, bAffectChildren, bSetupUndo, false, bPrintPythonCommand);
			}
		}
	}

	void SetGlobalTransform(int32 InElementIndex, const FTransform& InTransform, bool bAffectChildren = true, bool bSetupUndo = false)
	{
		SetGlobalTransformByIndex(InElementIndex, InTransform, false, bAffectChildren, bSetupUndo);
	}

	void SetInitialGlobalTransform(int32 InElementIndex, const FTransform& InTransform, bool bAffectChildren = true, bool bSetupUndo = false)
	{
		SetGlobalTransformByIndex(InElementIndex, InTransform, true, bAffectChildren, bSetupUndo);
	}

	void SetInitialGlobalTransform(const FTLLRN_RigElementKey& InKey, const FTransform& InTransform, bool bAffectChildren = true, bool bSetupUndo = false)
	{
		SetGlobalTransform(InKey, InTransform, true, bAffectChildren, bSetupUndo);
	}

	/**
	 * Returns the version of the transform / pose on the element given its key.
	 * Versions are incremented with every change occured to the transform. You
	 * can use this to compare your previous "knowledge" of the pose - and see
	 * if anybody has changed it during your last access.
	 */
	int32 GetPoseVersion(const FTLLRN_RigElementKey& InKey) const;

	/**
	 * Returns the global offset transform for a given control element.
	 * @param InKey The key of the control to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The global offset transform
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTransform GetGlobalControlOffsetTransform(FTLLRN_RigElementKey InKey, bool bInitial = false) const
	{
		return GetGlobalControlOffsetTransformByIndex(GetIndex(InKey), bInitial);
	}

	/**
	 * Returns the global offset transform for a given control element.
	 * @param InElementIndex The index of the control to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The global offset transform
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTransform GetGlobalControlOffsetTransformByIndex(int32 InElementIndex, bool bInitial = false) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				return GetControlOffsetTransform(ControlElement, bInitial ? ETLLRN_RigTransformType::InitialGlobal : ETLLRN_RigTransformType::CurrentGlobal);
			}
		}
		return FTransform::Identity;
	}

	/**
	 * Returns the local shape transform for a given control element.
	 * @param InKey The key of the control to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The local shape transform
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FTransform GetLocalControlShapeTransform(FTLLRN_RigElementKey InKey, bool bInitial = false) const
	{
		return GetLocalControlShapeTransformByIndex(GetIndex(InKey), bInitial);
	}

	/**
	 * Returns the local shape transform for a given control element.
	 * @param InElementIndex The index of the control to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The local shape transform
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FTransform GetLocalControlShapeTransformByIndex(int32 InElementIndex, bool bInitial = false) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				return GetControlShapeTransform(ControlElement, bInitial ? ETLLRN_RigTransformType::InitialLocal : ETLLRN_RigTransformType::CurrentLocal);
			}
		}
		return FTransform::Identity;
	}

	/**
	 * Returns the global shape transform for a given control element.
	 * @param InKey The key of the control to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The global shape transform
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTransform GetGlobalControlShapeTransform(FTLLRN_RigElementKey InKey, bool bInitial = false) const
	{
		return GetGlobalControlShapeTransformByIndex(GetIndex(InKey), bInitial);
	}

	/**
	 * Returns the global shape transform for a given control element.
	 * @param InElementIndex The index of the control to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The global shape transform
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTransform GetGlobalControlShapeTransformByIndex(int32 InElementIndex, bool bInitial = false) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				return GetControlShapeTransform(ControlElement, bInitial ? ETLLRN_RigTransformType::InitialGlobal : ETLLRN_RigTransformType::CurrentGlobal);
			}
		}
		return FTransform::Identity;
	}

	/**
	 * Returns a control's current value given its key
	 * @param InKey The key of the element to retrieve the current value for
	 * @param InValueType The type of value to return
	 * @return Returns the current value of the control
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTLLRN_RigControlValue GetControlValue(FTLLRN_RigElementKey InKey, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current) const
	{
		return GetControlValueByIndex(GetIndex(InKey), InValueType);
	}

	/**
	 * Returns a control's current value given its key
	 * @param InKey The key of the element to retrieve the current value for
	 * @param InValueType The type of value to return
	 * @return Returns the current value of the control
	 */
	template<typename T>
	T GetControlValue(FTLLRN_RigElementKey InKey, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current) const
	{
		return GetControlValueByIndex(GetIndex(InKey), InValueType).Get<T>();
	}

	/**
	 * Returns a control's current value given its index
	 * @param InElementIndex The index of the element to retrieve the current value for
	 * @param InValueType The type of value to return
	 * @return Returns the current value of the control
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTLLRN_RigControlValue GetControlValueByIndex(int32 InElementIndex, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				return GetControlValue(ControlElement, InValueType, bUsePreferredEulerAngles);
			}
		}
		return FTLLRN_RigControlValue();
	}

	/**
	 * Returns a control's current value given its index
	 * @param InElementIndex The index of the element to retrieve the current value for
	 * @param InValueType The type of value to return
	 * @return Returns the current value of the control
	 */
	FTLLRN_RigControlValue GetControlValue(int32 InElementIndex, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current) const
	{
		return GetControlValueByIndex(InElementIndex, InValueType);
	}

	/**
	 * Returns a control's current value given its index
	 * @param InElementIndex The index of the element to retrieve the current value for
	 * @param InValueType The type of value to return
	 * @return Returns the current value of the control
	 */
	template<typename T>
	T GetControlValue(int32 InElementIndex, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current) const
	{
		return GetControlValueByIndex(InElementIndex, InValueType).Get<T>();
	}

	/**
	 * Returns a control's initial value given its index
	 * @param InElementIndex The index of the element to retrieve the initial value for
	 * @return Returns the current value of the control
	 */
	FTLLRN_RigControlValue GetInitialControlValue(int32 InElementIndex) const
	{
		return GetControlValueByIndex(InElementIndex, ETLLRN_RigControlValueType::Initial);
	}

	/**
	 * Returns a control's initial value given its index
	 * @param InElementIndex The index of the element to retrieve the current value for
	 * @return Returns the current value of the control
	 */
	template<typename T>
	T GetInitialControlValue(int32 InElementIndex) const
	{
		return GetInitialControlValue(InElementIndex).Get<T>();
	}

	/**
	 * Returns a control's preferred rotator (local transform rotation)
	 * @param InKey The key of the element to retrieve the current value for
	 * @param bInitial If true we'll return the preferred rotator for the initial - otherwise current transform
	 * @return Returns the current preferred rotator
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FRotator GetControlPreferredRotator(FTLLRN_RigElementKey InKey, bool bInitial = false) const
	{
		return GetControlPreferredRotatorByIndex(GetIndex(InKey), bInitial);
	}

	/**
	 * Returns a control's preferred rotator (local transform rotation)
	 * @param InElementIndex The element index to look up
	 * @param bInitial If true we'll return the preferred rotator for the initial - otherwise current transform
	 * @return Returns the current preferred rotator
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FRotator GetControlPreferredRotatorByIndex(int32 InElementIndex, bool bInitial = false) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				return GetControlPreferredRotator(ControlElement, bInitial);
			}
		}
		return FRotator::ZeroRotator;
	}

	/**
	 * Returns a control's preferred rotator (local transform rotation)
	 * @param InControlElement The element to look up
	 * @param bInitial If true we'll return the preferred rotator for the initial - otherwise current transform
	 * @return Returns the current preferred rotator
	 */
	FRotator GetControlPreferredRotator(const FTLLRN_RigControlElement* InControlElement, bool bInitial = false) const
	{
		if(InControlElement)
		{
			if (bUsePreferredEulerAngles)
			{
				return InControlElement->PreferredEulerAngles.GetRotator(bInitial);
			}
			const ETLLRN_RigTransformType::Type Type = bInitial ? ETLLRN_RigTransformType::InitialLocal : ETLLRN_RigTransformType::CurrentLocal;
			return GetControlValue(InControlElement->GetKey()).GetAsTransform(InControlElement->Settings.ControlType, InControlElement->Settings.PrimaryAxis).Rotator();
		}
		return FRotator::ZeroRotator;
	}

	/**
	 * Sets a control's preferred rotator (local transform rotation)
	 * @param InKey The key of the element to retrieve the current value for
	 * @param InRotator The new preferred rotator to set
	 * @param bInitial If true we'll return the preferred rotator for the initial - otherwise current transform
	 * @param bFixEulerFlips If true the new rotator value will use the shortest path
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetControlPreferredRotator(FTLLRN_RigElementKey InKey, const FRotator& InRotator, bool bInitial = false, bool bFixEulerFlips = false)
	{
		SetControlPreferredRotatorByIndex(GetIndex(InKey), InRotator, bInitial, bFixEulerFlips);
	}
	

	/**
	 * Sets a control's preferred rotator (local transform rotation)
	 * @param InElementIndex The element index to look up
	 * @param InRotator The new preferred rotator to set
	 * @param bInitial If true we'll return the preferred rotator for the initial - otherwise current transform
	 * @param bFixEulerFlips If true the new rotator value will use the shortest path
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetControlPreferredRotatorByIndex(int32 InElementIndex, const FRotator& InRotator, bool bInitial = false, bool bFixEulerFlips = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				SetControlPreferredRotator(ControlElement, InRotator, bInitial, bFixEulerFlips);
			}
		}
	}

	/**
	 * Sets a control's preferred rotator (local transform rotation)
	 * @param InControlElement The element to look up
	 * @param InRotator The new preferred rotator to set
	 * @param bInitial If true we'll return the preferred rotator for the initial - otherwise current transform
	 * @param bFixEulerFlips If true the new rotator value will use the shortest path
	 */
	void SetControlPreferredRotator(FTLLRN_RigControlElement* InControlElement, const FRotator& InRotator, bool bInitial = false, bool bFixEulerFlips = false)
	{
		if(InControlElement)
		{
			InControlElement->PreferredEulerAngles.SetRotator(InRotator, bInitial, bFixEulerFlips);
		}
	}

	/**
	 * Returns a control's preferred euler angles (local transform rotation)
	 * @param InKey The key of the element to retrieve the current value for
	 * @param InRotationOrder The rotation order to use when retrieving the euler angles
	 * @param bInitial If true we'll return the preferred euler angles for the initial - otherwise current transform
	 * @return Returns the current preferred euler angles
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FVector GetControlPreferredEulerAngles(FTLLRN_RigElementKey InKey, EEulerRotationOrder InRotationOrder, bool bInitial = false) const
	{
		return GetControlPreferredEulerAnglesByIndex(GetIndex(InKey), InRotationOrder, bInitial);
	}

	/**
	 * Returns a control's preferred euler angles (local transform rotation)
	 * @param InElementIndex The element index to look up
	 * @param InRotationOrder The rotation order to use when retrieving the euler angles
	 * @param bInitial If true we'll return the preferred euler angles for the initial - otherwise current transform
	 * @return Returns the current preferred euler angles
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FVector GetControlPreferredEulerAnglesByIndex(int32 InElementIndex, EEulerRotationOrder InRotationOrder, bool bInitial = false) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				return GetControlPreferredEulerAngles(ControlElement, InRotationOrder, bInitial);
			}
		}
		return FVector::ZeroVector;
	}

	/**
	 * Returns a control's preferred euler angles (local transform rotation)
	 * @param InControlElement The element to look up
	 * @param InRotationOrder The rotation order to use when retrieving the euler angles
	 * @param bInitial If true we'll return the preferred euler angles for the initial - otherwise current transform
	 * @return Returns the current preferred euler angles
	 */
	FVector GetControlPreferredEulerAngles(const FTLLRN_RigControlElement* InControlElement, EEulerRotationOrder InRotationOrder, bool bInitial = false) const
	{
		if(InControlElement)
		{
			return InControlElement->PreferredEulerAngles.GetAngles(bInitial, InRotationOrder);
		}
		return FVector::ZeroVector;
	}

	/**
	 * Sets a control's preferred euler angles (local transform rotation)
	 * @param InKey The key of the element to retrieve the current value for
	 * @param InEulerAngles The new preferred euler angles to set
	 * @param InRotationOrder The rotation order to use when setting the euler angles
	 * @param bInitial If true we'll return the preferred euler angles for the initial - otherwise current transform
	 * @param bFixEulerFlips If true the new euler angles value will use the shortest path
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetControlPreferredEulerAngles(FTLLRN_RigElementKey InKey, const FVector& InEulerAngles, EEulerRotationOrder InRotationOrder, bool bInitial = false, bool bFixEulerFlips = false)
	{
		SetControlPreferredEulerAnglesByIndex(GetIndex(InKey), InEulerAngles, InRotationOrder, bInitial, bFixEulerFlips);
	}
	

	/**
	 * Sets a control's preferred euler angles (local transform rotation)
	 * @param InElementIndex The element index to look up
	 * @param InEulerAngles The new preferred euler angles to set
	 * @param InRotationOrder The rotation order to use when setting the euler angles
	 * @param bInitial If true we'll return the preferred euler angles for the initial - otherwise current transform
	 * @param bFixEulerFlips If true the new euler angles value will use the shortest path
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetControlPreferredEulerAnglesByIndex(int32 InElementIndex, const FVector& InEulerAngles, EEulerRotationOrder InRotationOrder, bool bInitial = false, bool bFixEulerFlips = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				SetControlPreferredEulerAngles(ControlElement, InEulerAngles, InRotationOrder, bInitial, bFixEulerFlips);
			}
		}
	}

	/**
	 * Sets a control's preferred euler angles (local transform rotation)
	 * @param InControlElement The element to look up
	 * @param InEulerAngles The new preferred euler angles to set
	 * @param InRotationOrder The rotation order to use when setting the euler angles
	 * @param bInitial If true we'll return the preferred euler angles for the initial - otherwise current transform
	 * @param bFixEulerFlips If true the new euler angles value will use the shortest path
	 */
	void SetControlPreferredEulerAngles(FTLLRN_RigControlElement* InControlElement, const FVector& InEulerAngles, EEulerRotationOrder InRotationOrder, bool bInitial = false, bool bFixEulerFlips = false)
	{
		if(InControlElement)
		{
			InControlElement->PreferredEulerAngles.SetRotationOrder(InRotationOrder);
			InControlElement->PreferredEulerAngles.SetAngles(InEulerAngles, bInitial, InRotationOrder, bFixEulerFlips);
		}
	}

	/**
	 * Returns a control's preferred euler rotation order
	 * @param InKey The key of the element to retrieve the current value for
	 * @param bFromSettings If true the rotation order will be looked up from the control's settings, otherwise from the currently set animation value
	 * @return Returns the current preferred euler rotation order
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	EEulerRotationOrder GetControlPreferredEulerRotationOrder(FTLLRN_RigElementKey InKey, bool bFromSettings = true) const
	{
		return GetControlPreferredEulerRotationOrderByIndex(GetIndex(InKey), bFromSettings);
	}

	/**
	 * Returns a control's preferred euler rotation order
	 * @param InElementIndex The element index to look up
	 * @param bFromSettings If true the rotation order will be looked up from the control's settings, otherwise from the currently set animation value
	 * @return Returns the current preferred euler rotation order
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	EEulerRotationOrder GetControlPreferredEulerRotationOrderByIndex(int32 InElementIndex, bool bFromSettings = true) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				return GetControlPreferredEulerRotationOrder(ControlElement, bFromSettings);
			}
		}
		return FTLLRN_RigPreferredEulerAngles::DefaultRotationOrder;
	}

	/**
	 * Returns a control's preferred euler rotation order
	 * @param InControlElement The element to look up
	 * @param bFromSettings If true the rotation order will be looked up from the control's settings, otherwise from the currently set animation value
	 * @return Returns the current preferred euler rotation order
	 */
	EEulerRotationOrder GetControlPreferredEulerRotationOrder(const FTLLRN_RigControlElement* InControlElement, bool bFromSettings = true) const
	{
		if(InControlElement)
		{
			if(bFromSettings)
			{
				return InControlElement->Settings.PreferredRotationOrder;
			}
			return InControlElement->PreferredEulerAngles.RotationOrder;
		}
		return FTLLRN_RigPreferredEulerAngles::DefaultRotationOrder;
	}
	
	/**
	 * Sets a control's preferred euler rotation order
	 * @param InKey The key of the element to retrieve the current value for
	 * @param InRotationOrder The rotation order to use when setting the euler angles
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetControlPreferredRotationOrder(FTLLRN_RigElementKey InKey, EEulerRotationOrder InRotationOrder)
	{
		SetControlPreferredRotationOrderByIndex(GetIndex(InKey), InRotationOrder);
	}
	

	/**
	 * Sets a control's preferred euler rotation order
	 * @param InElementIndex The element index to look up
	 * @param InRotationOrder The rotation order to use when setting the euler angles
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetControlPreferredRotationOrderByIndex(int32 InElementIndex, EEulerRotationOrder InRotationOrder)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				SetControlPreferredRotationOrder(ControlElement, InRotationOrder);
			}
		}
	}

	/**
	 * Sets a control's preferred euler rotation order
	 * @param InControlElement The element to look up
	 * @param InRotationOrder The rotation order to use when setting the euler angles
	 */
	void SetControlPreferredRotationOrder(FTLLRN_RigControlElement* InControlElement, EEulerRotationOrder InRotationOrder)
	{
		if(InControlElement)
		{
			InControlElement->PreferredEulerAngles.SetRotationOrder(InRotationOrder);
		}
	}

	bool GetUsePreferredRotationOrder(const FTLLRN_RigControlElement* InControlElement) const
	{
		if (InControlElement && bUsePreferredEulerAngles)
		{
			{
				return InControlElement->Settings.bUsePreferredRotationOrder;
			}
		}
		return false;
	}

	void  SetUsePreferredRotationOrder(FTLLRN_RigControlElement* InControlElement, bool bVal)
	{
		if (InControlElement)
		{
			{
				InControlElement->Settings.bUsePreferredRotationOrder = bVal;
			}
		}
	}

	FVector GetControlSpecifiedEulerAngle(const FTLLRN_RigControlElement* InControlElement, bool bIsInitial = false) const
	{
		FVector EulerAngle = FVector::ZeroVector;
		if (InControlElement)
		{
			if (bIsInitial == false && GetUsePreferredRotationOrder(InControlElement))
			{
				EEulerRotationOrder RotationOrder = GetControlPreferredEulerRotationOrder(InControlElement);
				EulerAngle = InControlElement->PreferredEulerAngles.GetAngles(false, RotationOrder);
			}
			else
			{
				FRotator Rotator = GetControlPreferredRotator(InControlElement);
				EulerAngle = FVector(Rotator.Roll, Rotator.Pitch, Rotator.Yaw);
			}
		}
		return EulerAngle;
	}
	
	void SetControlSpecifiedEulerAngle(FTLLRN_RigControlElement* InControlElement, const FVector& InEulerAngle, bool bIsInitial = false)
	{
		if (InControlElement)
		{
			if (GetUsePreferredRotationOrder(InControlElement))
			{
				EEulerRotationOrder RotationOrder = GetControlPreferredEulerRotationOrder(InControlElement);
				SetControlPreferredEulerAngles(InControlElement, InEulerAngle, RotationOrder, bIsInitial);
			}
			else
			{
				FRotator Rotator(InEulerAngle[1], InEulerAngle[2], InEulerAngle[0]);
				SetControlPreferredRotator(InControlElement, Rotator, bIsInitial, false /* fix euler flips*/); //test fix todo mikez
			}
		}
	}

	void SetControlPreferredEulerAngles(FTLLRN_RigControlElement* InControlElement, const FTransform& InTransform, bool bIsInitial = false)
	{
		FEulerTransform EulerTransform(InTransform);
		if (InControlElement && InControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform)
		{
			FVector EulerAngle(EulerTransform.Rotation.Roll, EulerTransform.Rotation.Pitch, EulerTransform.Rotation.Yaw);
			SetControlSpecifiedEulerAngle(InControlElement, EulerAngle, bIsInitial);
		}
		else if (InControlElement && InControlElement->Settings.ControlType == ETLLRN_RigControlType::TransformNoScale)
		{
			const FTransformNoScale NoScale = EulerTransform.ToFTransform();
			FVector EulerAngle(EulerTransform.Rotation.Roll, EulerTransform.Rotation.Pitch, EulerTransform.Rotation.Yaw);
			SetControlSpecifiedEulerAngle(InControlElement, EulerAngle, bIsInitial);
		}
		else if (InControlElement && InControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
		{
			FVector EulerAngle(EulerTransform.Rotation.Roll, EulerTransform.Rotation.Pitch, EulerTransform.Rotation.Yaw);
			FQuat Quat = GetControlQuaternion(InControlElement, EulerAngle);
			SetControlSpecifiedEulerAngle(InControlElement, EulerAngle, bIsInitial);
		}
	}

	FQuat GetControlQuaternion(const FTLLRN_RigControlElement* InControlElement, const FVector& InEulerAngle) const
	{
		if (InControlElement)
		{
			FRotator Rotator(InEulerAngle[1], InEulerAngle[2], InEulerAngle[0]);

			if (GetUsePreferredRotationOrder(InControlElement))
			{
				return InControlElement->PreferredEulerAngles.GetQuatFromRotator(Rotator);
			}
			else
			{
				return Rotator.Quaternion();
			}
		}
		return FQuat();
	}

	FVector GetControlAnglesFromQuat(const FTLLRN_RigControlElement* InControlElement, const FQuat& InQuat, bool bUseRotationOrder) const
	{
		FVector Angle(0, 0, 0);
		if (InControlElement)
		{

			if (bUseRotationOrder && InControlElement->Settings.bUsePreferredRotationOrder)
			{
				FRotator Rotator = InControlElement->PreferredEulerAngles.GetRotatorFromQuat(InQuat);
				Angle = Rotator.Euler();
			}
			else
			{
				FRotator Rotator(InQuat);
				Angle = Rotator.Euler();
			}
		}
		return Angle;
	}
	/**
	 * Returns the pin type to use for a control
	 * @param InControlElement The control to return the pin type for
	 * @return The pin type
	 */
	FEdGraphPinType GetControlPinType(FTLLRN_RigControlElement* InControlElement) const;

	/**
	 * Returns the default value to use for a pin for a control
	 * @param InControlElement The control to return the pin default value for
	 * @param bForEdGraph If this is true to the math types' ::ToString will be used rather than text export
	 * @param InValueType The type of value to return
	 * @return The pin default value
	 */
	FString GetControlPinDefaultValue(FTLLRN_RigControlElement* InControlElement, bool bForEdGraph, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Initial) const;

	/**
	 * Sets a control's current value given its key
	 * @param InKey The key of the element to set the current value for
	 * @param InValue The value to set on the control
	 * @param InValueType The type of value to set
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetControlValue(FTLLRN_RigElementKey InKey, FTLLRN_RigControlValue InValue, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current, bool bSetupUndo = false, bool bPrintPythonCommands = false)
	{
		SetControlValueByIndex(GetIndex(InKey), InValue, InValueType, bSetupUndo, bPrintPythonCommands);
	}

	/**
	 * Sets a control's current value given its key
	 * @param InKey The key of the element to set the current value for
	 * @param InValue The value to set on the control
	 * @param InValueType The type of value to set
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	template<typename T>
	void SetControlValue(FTLLRN_RigElementKey InKey, const T& InValue, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current, bool bSetupUndo = false, bool bPrintPythonCommands = false) const
	{
		return SetControlValue(InKey, FTLLRN_RigControlValue::Make<T>(InValue), InValueType, bSetupUndo, bPrintPythonCommands);
	}

	/**
	 * Sets a control's current value given its index
	 * @param InElementIndex The index of the element to set the current value for
	 * @param InValue The value to set on the control
	 * @param InValueType The type of value to set
  	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetControlValueByIndex(int32 InElementIndex, FTLLRN_RigControlValue InValue, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current, bool bSetupUndo = false, bool bPrintPythonCommands = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				SetControlValue(ControlElement, InValue, InValueType, bSetupUndo, false, bPrintPythonCommands);
			}
		}
	}

	/**
	 * Sets a control's current value given its index
	 * @param InElementIndex The index of the element to set the current value for
	 * @param InValue The value to set on the control
	 * @param InValueType The type of value to set
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	void SetControlValue(int32 InElementIndex, const FTLLRN_RigControlValue& InValue, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current, bool bSetupUndo = false, bool bPrintPythonCommands = false)
	{
		SetControlValueByIndex(InElementIndex, InValue, InValueType, bSetupUndo, bPrintPythonCommands);
	}

	/**
	 * Sets a control's current value given its index
	 * @param InElementIndex The index of the element to set the current value for
	 * @param InValue The value to set on the control
	 * @param InValueType The type of value to set
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	template<typename T>
	void SetControlValue(int32 InElementIndex, const T& InValue, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current, bool bSetupUndo = false)
	{
		return SetControlValueByIndex(InElementIndex, FTLLRN_RigControlValue::Make<T>(InValue), InValueType, bSetupUndo, false);
	}

	/**
	 * Sets a control's initial value given its index
	 * @param InElementIndex The index of the element to set the initial value for
	 * @param InValue The value to set on the control
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	void SetInitialControlValue(int32 InElementIndex, const FTLLRN_RigControlValue& InValue, bool bSetupUndo = false)
	{
		SetControlValueByIndex(InElementIndex, InValue, ETLLRN_RigControlValueType::Initial, bSetupUndo);
	}

	/**
	 * Sets a control's initial value given its index
	 * @param InElementIndex The index of the element to set the initial value for
	 * @param InValue The value to set on the control
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	template<typename T>
	void SetInitialControlValue(int32 InElementIndex, const T& InValue, bool bSetupUndo = false) const
	{
		return SetInitialControlValue(InElementIndex, FTLLRN_RigControlValue::Make<T>(InValue), bSetupUndo);
	}

	/**
	 * Sets a control's current visibility based on a key
	 * @param InKey The key of the element to set the visibility for
	 * @param bVisibility The visibility to set on the control
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetControlVisibility(FTLLRN_RigElementKey InKey, bool bVisibility)
	{
		SetControlVisibilityByIndex(GetIndex(InKey), bVisibility);
	}

	/**
	 * Sets a control's current visibility based on a key
	 * @param InElementIndex The index of the element to set the visibility for
	 * @param bVisibility The visibility to set on the control
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetControlVisibilityByIndex(int32 InElementIndex, bool bVisibility)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				SetControlVisibility(ControlElement, bVisibility);
			}
		}
	}

	void SetControlVisibility(int32 InElementIndex, bool bVisibility)
	{
		SetControlVisibilityByIndex(InElementIndex, bVisibility);
	}

	/**
	 * Returns a curve's value given its key
	 * @param InKey The key of the element to retrieve the value for
	 * @return Returns the value of the curve
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    float GetCurveValue(FTLLRN_RigElementKey InKey) const
	{
		return GetCurveValueByIndex(GetIndex(InKey));
	}

	/**
	 * Returns a curve's value given its index
	 * @param InElementIndex The index of the element to retrieve the value for
	 * @return Returns the value of the curve
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    float GetCurveValueByIndex(int32 InElementIndex) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(Elements[InElementIndex]))
			{
				return GetCurveValue(CurveElement);
			}
		}
		return 0.f;
	}

	// TODO: Deprecate?
	float GetCurveValue(int32 InElementIndex) const
	{
		return GetCurveValueByIndex(InElementIndex);
	}

	/**
	 * Returns whether a curve's value is set, given its key
	 * @param InKey The key of the element to retrieve the value for
	 * @return Returns true if the value is set, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool IsCurveValueSet(FTLLRN_RigElementKey InKey) const
	{
		return IsCurveValueSetByIndex(GetIndex(InKey));
	}

	/**
	 * Returns a curve's value given its index
	 * @param InElementIndex The index of the element to retrieve the value for
	 * @return Returns true if the value is set, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool IsCurveValueSetByIndex(int32 InElementIndex) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(Elements[InElementIndex]))
			{
				return IsCurveValueSet(CurveElement);
			}
		}
		return false;
	}
	
	/**
	 * Sets a curve's value given its key
	 * @param InKey The key of the element to set the value for
	 * @param InValue The value to set on the curve
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetCurveValue(FTLLRN_RigElementKey InKey, float InValue, bool bSetupUndo = false)
	{
		SetCurveValueByIndex(GetIndex(InKey), InValue, bSetupUndo);
	}

	/**
	 * Sets a curve's value given its index
	 * @param InElementIndex The index of the element to set the value for
	 * @param InValue The value to set on the curve
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetCurveValueByIndex(int32 InElementIndex, float InValue, bool bSetupUndo = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(Elements[InElementIndex]))
			{
				SetCurveValue(CurveElement, InValue, bSetupUndo);
			}
		}
	}

	// TODO: Deprecate?
	void SetCurveValue(int32 InElementIndex, float InValue, bool bSetupUndo = false)
	{
		SetCurveValueByIndex(InElementIndex, InValue, bSetupUndo);
	}

	/**
	 * Sets a curve's value given its key
	 * @param InKey The key of the element to set the value for
	 * @param InValue The value to set on the curve
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void UnsetCurveValue(FTLLRN_RigElementKey InKey, bool bSetupUndo = false)
	{
		UnsetCurveValueByIndex(GetIndex(InKey), bSetupUndo);
	}

	/**
	 * Sets a curve's value given its index
	 * @param InElementIndex The index of the element to set the value for
	 * @param InValue The value to set on the curve
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void UnsetCurveValueByIndex(int32 InElementIndex, bool bSetupUndo = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigCurveElement* CurveElement = Cast<FTLLRN_RigCurveElement>(Elements[InElementIndex]))
			{
				UnsetCurveValue(CurveElement, bSetupUndo);
			}
		}
	}

	/**
	 * Sets the offset transform for a given control element by key
	 * @param InKey The key of the control element to set the offset transform for
	 * @param InTransform The new offset transform value to set
	 * @param bInitial If true the initial value will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetControlOffsetTransform(FTLLRN_RigElementKey InKey, FTransform InTransform, bool bInitial = false, bool bAffectChildren = true, bool bSetupUndo = false, bool bPrintPythonCommands = false)
	{
		return SetControlOffsetTransformByIndex(GetIndex(InKey), InTransform, bInitial, bAffectChildren, bSetupUndo, bPrintPythonCommands);
	}

	/**
	 * Sets the local offset transform for a given control element by index
	 * @param InElementIndex The index of the control element to set the offset transform for
 	 * @param InTransform The new local offset transform value to set
	 * @param bInitial If true the initial value will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetControlOffsetTransformByIndex(int32 InElementIndex, FTransform InTransform, bool bInitial = false, bool bAffectChildren = true, bool bSetupUndo = false, bool bPrintPythonCommands = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				SetControlOffsetTransform(ControlElement, InTransform, bInitial ? ETLLRN_RigTransformType::InitialLocal : ETLLRN_RigTransformType::CurrentLocal, bAffectChildren, bSetupUndo, bPrintPythonCommands);
			}
		}
	}

	/**
	 * Sets the shape transform for a given control element by key
	 * @param InKey The key of the control element to set the shape transform for
	 * @param InTransform The new shape transform value to set
	 * @param bInitial If true the initial value will be used
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetControlShapeTransform(FTLLRN_RigElementKey InKey, FTransform InTransform, bool bInitial = false, bool bSetupUndo = false)
	{
		return SetControlShapeTransformByIndex(GetIndex(InKey), InTransform, bInitial, bSetupUndo);
	}

	/**
	 * Sets the local shape transform for a given control element by index
	 * @param InElementIndex The index of the control element to set the shape transform for
	 * @param InTransform The new local shape transform value to set
	 * @param bInitial If true the initial value will be used
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    void SetControlShapeTransformByIndex(int32 InElementIndex, FTransform InTransform, bool bInitial = false, bool bSetupUndo = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				SetControlShapeTransform(ControlElement, InTransform, bInitial ? ETLLRN_RigTransformType::InitialLocal : ETLLRN_RigTransformType::CurrentLocal, bSetupUndo);
			}
		}
	}

	/**
	 * Sets the control settings for a given control element by key
	 * @param InKey The key of the control element to set the settings for
	 * @param InSettings The new control settings value to set
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetControlSettings(FTLLRN_RigElementKey InKey, FTLLRN_RigControlSettings InSettings, bool bSetupUndo = false, bool bForce = false, bool bPrintPythonCommands = false)
	{
		return SetControlSettingsByIndex(GetIndex(InKey), InSettings, bSetupUndo, bForce, bPrintPythonCommands);
	}

	/**
	 * Sets the control settings for a given control element by index
	 * @param InElementIndex The index of the control element to set the settings for
	 * @param InSettings The new control settings value to set
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetControlSettingsByIndex(int32 InElementIndex, FTLLRN_RigControlSettings InSettings, bool bSetupUndo = false, bool bForce = false, bool bPrintPythonCommands = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Elements[InElementIndex]))
			{
				SetControlSettings(ControlElement, InSettings, bSetupUndo, bForce, bPrintPythonCommands);
			}
		}
	}

	/**
	 * Sets the connector settings for a given connector element by key
	 * @param InKey The key of the connector element to set the settings for
	 * @param InSettings The new connector settings value to set
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetConnectorSettings(FTLLRN_RigElementKey InKey, FTLLRN_RigConnectorSettings InSettings, bool bSetupUndo = false, bool bForce = false, bool bPrintPythonCommands = false)
	{
		return SetConnectorSettingsByIndex(GetIndex(InKey), InSettings, bSetupUndo, bForce, bPrintPythonCommands);
	}

	/**
	 * Sets the connector settings for a given connector element by index
	 * @param InElementIndex The index of the connector element to set the settings for
	 * @param InSettings The new connector settings value to set
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SetConnectorSettingsByIndex(int32 InElementIndex, FTLLRN_RigConnectorSettings InSettings, bool bSetupUndo = false, bool bForce = false, bool bPrintPythonCommands = false)
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			if(FTLLRN_RigConnectorElement* ConnectorElement = Cast<FTLLRN_RigConnectorElement>(Elements[InElementIndex]))
			{
				SetConnectorSettings(ConnectorElement, InSettings, bSetupUndo, bForce, bPrintPythonCommands);
			}
		}
	}

	/**
	 * Returns the global current or initial value for a given key.
	 * If the element does not have a parent FTransform::Identity will be returned.
	 * @param InKey The key of the element to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The element's parent's global current or initial transform's value.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTransform GetParentTransform(FTLLRN_RigElementKey InKey, bool bInitial = false) const
	{
		return GetParentTransformByIndex(GetIndex(InKey), bInitial);
	}

	/**
	 * Returns the global current or initial value for a given element index.
	 * If the element does not have a parent FTransform::Identity will be returned.
	 * @param InElementIndex The index of the element to retrieve the transform for
	 * @param bInitial If true the initial transform will be used
	 * @return The element's parent's global current or initial transform's value.
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTransform GetParentTransformByIndex(int32 InElementIndex, bool bInitial = false) const
	{
		if(Elements.IsValidIndex(InElementIndex))
		{
			return GetParentTransform(Elements[InElementIndex], bInitial ? ETLLRN_RigTransformType::InitialGlobal : ETLLRN_RigTransformType::CurrentGlobal);
		}
		return FTransform::Identity;
	}

	/**
	 * Returns the child elements of a given element key
	 * @param InKey The key of the element to retrieve the children for
	 * @param bRecursive If set to true grand-children will also be returned etc
	 * @return Returns the child elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	TArray<FTLLRN_RigElementKey> GetChildren(FTLLRN_RigElementKey InKey, bool bRecursive = false) const;

	/**
	 * Returns the child elements of a given element key
	 * @param InKey The key of the element to retrieve the children for
	 * @param bRecursive If set to true grand-children will also be returned etc
	 * @return Returns the child elements
	 */
	FTLLRN_RigBaseElementChildrenArray GetActiveChildren(const FTLLRN_RigBaseElement* InElement, bool bRecursive = false) const;

	/**
	 * Returns the child elements of a given element index
	 * @param InIndex The index of the element to retrieve the children for
	 * @param bRecursive If set to true grand-children will also be returned etc
	 * @return Returns the child elements' indices
	 */
    TArray<int32> GetChildren(int32 InIndex, bool bRecursive = false) const;

	/**
	 * Returns the child elements of a given element
	 * @param InElement The element to retrieve the children for
	 * @return Returns the child elements
	 */
	TConstArrayView<FTLLRN_RigBaseElement*> GetChildren(const FTLLRN_RigBaseElement* InElement) const;
	TArrayView<FTLLRN_RigBaseElement*> GetChildren(const FTLLRN_RigBaseElement* InElement);

	/**
	 * Returns the child elements of a given element
	 * @param InElement The element to retrieve the children for
	 * @param bRecursive If set to true grand-children will also be returned etc
	 * @return Returns the child elements
	 */
	FTLLRN_RigBaseElementChildrenArray GetChildren(const FTLLRN_RigBaseElement* InElement, bool bRecursive) const;

	/**
	 * Returns the parent elements of a given element key
	 * @param InKey The key of the element to retrieve the parents for
	 * @param bRecursive If set to true parents of parents will also be returned
	 * @return Returns the parent elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    TArray<FTLLRN_RigElementKey> GetParents(FTLLRN_RigElementKey InKey, bool bRecursive = false) const;

	/**
	 * Returns the parent elements of a given element index
	 * @param InIndex The index of the element to retrieve the parents for
	 * @param bRecursive If set to true parents of parents will also be returned
	 * @return Returns the parent elements' indices
	 */
    TArray<int32> GetParents(int32 InIndex, bool bRecursive = false) const;

	/**
	 * Returns the parent elements of a given element
	 * @param InElement The element to retrieve the parents for
	 * @param bRecursive If set to true parents of parents will also be returned
	 * @return Returns the parent elements
	 */
	FTLLRN_RigBaseElementParentArray GetParents(const FTLLRN_RigBaseElement* InElement, bool bRecursive = false) const;

	/**
	 * Returns the default parent element's key of a given child key
	 * @param InKey The key of the element to retrieve the parent for
	 * @return Returns the default parent element key
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FTLLRN_RigElementKey GetDefaultParent(FTLLRN_RigElementKey InKey) const;

	/**
	 * Returns the first parent element of a given element key
	 * @param InKey The key of the element to retrieve the parents for
	 * @return Returns the first parent element
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTLLRN_RigElementKey GetFirstParent(FTLLRN_RigElementKey InKey) const;

	/**
	 * Returns the first parent element of a given element index
	 * @param InIndex The index of the element to retrieve the parent for
	 * @return Returns the first parent index (or INDEX_NONE)
	 */
    int32 GetFirstParent(int32 InIndex) const;

	/**
	 * Returns the first parent element of a given element
	 * @param InElement The element to retrieve the parents for
	 * @return Returns the first parent element
	 */
	FTLLRN_RigBaseElement* GetFirstParent(const FTLLRN_RigBaseElement* InElement) const;

	/**
	 * Returns the number of parents of an element
	 * @param InKey The key of the element to retrieve the number of parents for
	 * @return Returns the number of parents of an element
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    int32 GetNumberOfParents(FTLLRN_RigElementKey InKey) const;

	/**
	 * Returns the number of parents of an element
	 * @param InIndex The index of the element to retrieve the number of parents for
	 * @return Returns the number of parents of an element
	 */
    int32 GetNumberOfParents(int32 InIndex) const;

	/**
	 * Returns the number of parents of an element
	 * @param InElement The element to retrieve the number of parents for
	 * @return Returns the number of parents of an element
	 */
	int32 GetNumberOfParents(const FTLLRN_RigBaseElement* InElement) const;

	/**
	 * Returns the weight of a parent below a multi parent element
	 * @param InChild The key of the multi parented element
	 * @param InParent The key of the parent to look up the weight for
	 * @param bInitial If true the initial weights will be used
	 * @return Returns the weight of a parent below a multi parent element, or FLT_MAX if the parent is invalid
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    FTLLRN_RigElementWeight GetParentWeight(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, bool bInitial = false) const;

	/**
	 * Returns the weight of a parent below a multi parent element
	 * @param InChild The multi parented element
	 * @param InParent The parent to look up the weight for
	 * @param bInitial If true the initial weights will be used
	 * @return Returns the weight of a parent below a multi parent element, or FLT_MAX if the parent is invalid
	 */
    FTLLRN_RigElementWeight GetParentWeight(const FTLLRN_RigBaseElement* InChild, const FTLLRN_RigBaseElement* InParent, bool bInitial = false) const;

	/**
	 * Returns the weight of a parent below a multi parent element
	 * @param InChild The multi parented element
	 * @param InParentIndex The index of the parent inside of the multi parent element
	 * @param bInitial If true the initial weights will be used
	 * @return Returns the weight of a parent below a multi parent element, or FLT_MAX if the parent is invalid
	 */
	FTLLRN_RigElementWeight GetParentWeight(const FTLLRN_RigBaseElement* InChild, int32 InParentIndex, bool bInitial = false) const;

	/**
	 * Returns the weights of all parents below a multi parent element
	 * @param InChild The key of the multi parented element
	 * @param bInitial If true the initial weights will be used
	 * @return Returns the weight of a parent below a multi parent element, or FLT_MAX if the parent is invalid
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	TArray<FTLLRN_RigElementWeight> GetParentWeightArray(FTLLRN_RigElementKey InChild, bool bInitial = false) const;

	/**
	 * Returns the weights of all parents below a multi parent element
	 * @param InChild The multi parented element
	 * @param bInitial If true the initial weights will be used
	 * @return Returns the weight of a parent below a multi parent element, or FLT_MAX if the parent is invalid
	 */
	TArray<FTLLRN_RigElementWeight> GetParentWeightArray(const FTLLRN_RigBaseElement* InChild, bool bInitial = false) const;

	/**
	 * Get the current active parent for the passed in key. This is only valid when only one parent has a weight value and the other parents have zero weights
	 * @param InKey The multi parented element
	 * @param bReferenceKey Whether or not to return a reference key
	 * @return Returns the first parent with a non-zero weight
	 */
	FTLLRN_RigElementKey GetActiveParent(const FTLLRN_RigElementKey& InKey, bool bReferenceKey = true) const;

	/**
	 * Get the current active parent for a given element index. This is only valid when only one parent has a weight value and the other parents have zero weights
	 * @param InIndex The index of the element to retrieve the parent for
	 * @return Returns the first parent index (or INDEX_NONE) with a non-zero weight
	 */
	int32 GetActiveParent(int32 InIndex) const;

	/**
	 * Get the current active parent for the passed in key. This is only valid when only one parent has a weight value and the other parents have zero weights
	 * @param InElement The element to retrieve the parents for
	 * @return Returns the first parent with a non-zero weight
	 */
	FTLLRN_RigBaseElement* GetActiveParent(const FTLLRN_RigBaseElement* InElement) const;

	/**
	 * Sets the weight of a parent below a multi parent element
	 * @param InChild The key of the multi parented element
	 * @param InParent The key of the parent to look up the weight for
	 * @param InWeight The new weight to set for the parent
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @return Returns true if changing the weight was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    bool SetParentWeight(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, FTLLRN_RigElementWeight InWeight, bool bInitial = false, bool bAffectChildren = true);

	/**
	 * Sets the weight of a parent below a multi parent element
	 * @param InChild The multi parented element
	 * @param InParent The parent to look up the weight for
	 * @param InWeight The new weight to set for the parent
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @return Returns true if changing the weight was successful
	 */
	bool SetParentWeight(FTLLRN_RigBaseElement* InChild, const FTLLRN_RigBaseElement* InParent, FTLLRN_RigElementWeight InWeight, bool bInitial = false, bool bAffectChildren = true);

	/**
	 * Sets the weight of a parent below a multi parent element
	 * @param InChild The multi parented element
	 * @param InParentIndex The index of the parent inside of the multi parent element
	 * @param InWeight The new weight to set for the parent
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @return Returns true if changing the weight was successful
	 */
	bool SetParentWeight(FTLLRN_RigBaseElement* InChild, int32 InParentIndex, FTLLRN_RigElementWeight InWeight, bool bInitial = false, bool bAffectChildren = true);

	/**
	 * Sets the all of the weights of the parents of a multi parent element
	 * @param InChild The key of the multi parented element
	 * @param InWeights The new weights to set for the parents
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @return Returns true if changing the weight was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SetParentWeightArray(FTLLRN_RigElementKey InChild, TArray<FTLLRN_RigElementWeight> InWeights, bool bInitial = false, bool bAffectChildren = true);

	/**
	 * Sets the all of the weights of the parents of a multi parent element
	 * @param InChild The multi parented element
	 * @param InWeights The new weights to set for the parents
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @return Returns true if changing the weight was successful
	 */
	bool SetParentWeightArray(FTLLRN_RigBaseElement* InChild,  const TArray<FTLLRN_RigElementWeight>& InWeights, bool bInitial = false, bool bAffectChildren = true);

	/**
	* Sets the all of the weights of the parents of a multi parent element
	* @param InChild The multi parented element
	* @param InWeights The new weights to set for the parents
	* @param bInitial If true the initial weights will be used
	* @param bAffectChildren If set to false children will not move (maintain global).
	* @return Returns true if changing the weight was successful
	*/
	bool SetParentWeightArray(FTLLRN_RigBaseElement* InChild,  const TArrayView<const FTLLRN_RigElementWeight>& InWeights, bool bInitial = false, bool bAffectChildren = true);

	/**
	* Determines if the element can be switched to a provided parent
	* @param InChild The key of the multi parented element
	* @param InParent The key of the parent to look up the weight for
    * @param InDependencyMap An additional map of dependencies to respect
	* @param OutFailureReason An optional pointer to retrieve the reason for failure
	* @return Returns true if changing the weight was successful
	*/
	bool CanSwitchToParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, const TElementDependencyMap& InDependencyMap = TElementDependencyMap(), FString* OutFailureReason = nullptr);

	/**
	 * Switches a multi parent element to a single parent.
	 * This sets the new parent's weight to 1.0 and disables
	 * weights for all other potential parents.
	 * @param InChild The key of the multi parented element
	 * @param InParent The key of the parent to look up the weight for
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @return Returns true if changing the weight was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SwitchToParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, bool bInitial = false, bool bAffectChildren = true)
	{
		return SwitchToParent(InChild, InParent, bInitial, bAffectChildren, TElementDependencyMap(), nullptr);
	}
	bool SwitchToParent(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent, bool bInitial, bool bAffectChildren, const TElementDependencyMap& InDependencyMap, FString* OutFailureReason);

	/**
	 * Switches a multi parent element to a single parent.
	 * This sets the new parent's weight to 1.0 and disables
	 * weights for all other potential parents.
	 * @param InChild The multi parented element
	 * @param InParent The parent to look up the weight for
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
     * @param InDependencyMap An additional map of dependencies to respect
	 * @param OutFailureReason An optional pointer to retrieve the reason for failure
	 * @return Returns true if changing the weight was successful
	 */
	bool SwitchToParent(FTLLRN_RigBaseElement* InChild, FTLLRN_RigBaseElement* InParent, bool bInitial = false, bool bAffectChildren = true, const TElementDependencyMap& InDependencyMap = TElementDependencyMap(), FString* OutFailureReason = nullptr);

	/**
	 * Switches a multi parent element to a single parent.
	 * This sets the new parent's weight to 1.0 and disables
	 * weights for all other potential parents.
	 * @param InChild The multi parented element
	 * @param InParentIndex The index of the parent inside of the multi parent element
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @return Returns true if changing the weight was successful
	 */
	bool SwitchToParent(FTLLRN_RigBaseElement* InChild, int32 InParentIndex, bool bInitial = false, bool bAffectChildren = true);

	/**
	 * Switches a multi parent element to its first parent
	 * @param InChild The key of the multi parented element
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @return Returns true if changing the weight was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SwitchToDefaultParent(FTLLRN_RigElementKey InChild, bool bInitial = false, bool bAffectChildren = true);

	/**
	 * Switches a multi parent element to its first parent
	 * This sets the new parent's weight to 1.0 and disables
	 * weights for all other potential parents.
	 * @param InChild The multi parented element
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @return Returns true if changing the weight was successful
	 */
	bool SwitchToDefaultParent(FTLLRN_RigBaseElement* InChild, bool bInitial = false, bool bAffectChildren = true);

	/**
	 * Switches a multi parent element to world space.
	 * This injects a world space reference.
	 * @param InChild The key of the multi parented element
	 * @param bInitial If true the initial weights will be used
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @return Returns true if changing the weight was successful
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool SwitchToWorldSpace(FTLLRN_RigElementKey InChild, bool bInitial = false, bool bAffectChildren = true);

	/**
	* Switches a multi parent element to world space.
	* This injects a world space reference.
	* @param InChild The multi parented element
	* @param bInitial If true the initial weights will be used
	* @param bAffectChildren If set to false children will not move (maintain global).
	* @return Returns true if changing the weight was successful
	*/
	bool SwitchToWorldSpace(FTLLRN_RigBaseElement* InChild, bool bInitial = false, bool bAffectChildren = true);

	/**
	 * Adds the world space reference or returns it
	 */
	FTLLRN_RigElementKey GetOrAddWorldSpaceReference();
	
	static FTLLRN_RigElementKey GetDefaultParentKey();
	static FTLLRN_RigElementKey GetWorldSpaceReferenceKey();

	/**
	 * Returns true if an element is parented to another element
	 * @param InChild The key of the child element to check for a parent
	 * @param InParent The key of the parent element to check for
	 * @return True if the given parent and given child have a parent-child relationship
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	bool IsParentedTo(FTLLRN_RigElementKey InChild, FTLLRN_RigElementKey InParent) const
	{
		return IsParentedTo(GetIndex(InChild), GetIndex(InParent));
	}

	/**
	 * Returns true if an element is parented to another element
	 * @param InChildIndex The index of the child element to check for a parent
	 * @param InParentIndex The index of the parent element to check for
	 * @param InDependencyMap An additional map of dependencies to respect
	 * @return True if the given parent and given child have a parent-child relationship
	 */
    bool IsParentedTo(int32 InChildIndex, int32 InParentIndex, const TElementDependencyMap& InDependencyMap = TElementDependencyMap()) const
	{
		if(Elements.IsValidIndex(InChildIndex) && Elements.IsValidIndex(InParentIndex))
		{
			return IsParentedTo(Elements[InChildIndex], Elements[InParentIndex], InDependencyMap);
		}
		return false;
	}

	/**
	 * Returns the animation channels of a given element key
	 * @param InKey The key of the element to retrieve the animation channels for
	 * @param bOnlyDirectChildren If set to false also animation channels with secondary parenting relationships will be retrieved
	 * @return Returns the animation channels' indices
	 */
	TArray<FTLLRN_RigElementKey> GetAnimationChannels(FTLLRN_RigElementKey InKey, bool bOnlyDirectChildren = true) const;

	/**
	 * Returns the animation channels of a given element index
	 * @param InIndex The index of the element to retrieve the animation channels for
	 * @param bOnlyDirectChildren If set to false also animation channels with secondary parenting relationships will be retrieved
	 * @return Returns the animation channels' indices
	 */
	TArray<int32> GetAnimationChannels(int32 InIndex, bool bOnlyDirectChildren = true) const;

	/**
	 * Returns the animation channels of a given element
	 * @param InElement The element to retrieve the animation channels for
	 * @param bOnlyDirectChildren If set to false also animation channels with secondary parenting relationships will be retrieved
	 * @return Returns the animation channels
	 */
	TArray<FTLLRN_RigControlElement*> GetAnimationChannels(const FTLLRN_RigControlElement* InElement, bool bOnlyDirectChildren = true) const;

	/**
	 * Returns all element keys of this hierarchy
	 * @param bTraverse If set to true the keys will be returned by depth first traversal
	 * @param InElementType The type filter to apply
	 * @return The keys of all elements
	 */
	TArray<FTLLRN_RigElementKey> GetAllKeys(bool bTraverse = false, ETLLRN_RigElementType InElementType = ETLLRN_RigElementType::All) const;

	/**
	 * Returns element keys of this hierarchy, filtered by a predicate.
	 * @param InPredicateFunc The predicate function to apply. Should return \c true if the element
	 *    should be added to the result array.
	 * @param bInTraverse If set to true the keys will be returned by depth first traversal
	 * @return The keys of all elements
	 */
	TArray<FTLLRN_RigElementKey> GetKeysByPredicate(TFunctionRef<bool(const FTLLRN_RigBaseElement&)> InPredicateFunc, bool bInTraverse = false) const;

	/**
	 * Returns all element keys of this hierarchy
	 * @param bTraverse If set to true the keys will be returned by depth first traversal
	 * @return The keys of all elements
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Get All Keys", ScriptName = "GetAllKeys"))
	TArray<FTLLRN_RigElementKey> GetAllKeys_ForBlueprint(bool bTraverse = true) const
	{
		return GetAllKeys(bTraverse, ETLLRN_RigElementType::All);
	}

	/**
	 * Helper function to traverse the hierarchy
	 * @param InElement The element to start the traversal at
	 * @param bTowardsChildren If set to true the traverser walks downwards (towards the children), otherwise upwards (towards the parents)
	 * @param PerElementFunction The function to call for each visited element
	 */
	void Traverse(FTLLRN_RigBaseElement* InElement, bool bTowardsChildren, TFunction<void(FTLLRN_RigBaseElement*, bool& /* continue */)> PerElementFunction) const;

	/**
	 * Helper function to traverse the hierarchy from the root
	 * @param PerElementFunction The function to call for each visited element
	 * @param bTowardsChildren If set to true the traverser walks downwards (towards the children), otherwise upwards (towards the parents)
	 */
	void Traverse(TFunction<void(FTLLRN_RigBaseElement*, bool& /* continue */)> PerElementFunction, bool bTowardsChildren = true) const;

	/**
	 * Returns the currently resolved target for given connector key
	 */
	const FTLLRN_RigElementKey& GetResolvedTarget(const FTLLRN_RigElementKey& InConnectorKey) const;

	/**
	 * Performs undo for one transform change
	 */
	bool Undo();

	/**
	 * Performs redo for one transform change
	 */
	bool Redo();

	/**
	 * Returns the event fired during undo / redo
	 */
	FTLLRN_RigHierarchyUndoRedoTransformEvent& OnUndoRedo() { return UndoRedoEvent; }

	/**
	 * Starts an interaction on the rig.
	 * This will cause all transform actions happening to be merged
	 */
	void StartInteraction() { bIsInteracting = true; }

	/**
	 * Starts an interaction on the rig.
	 * This will cause all transform actions happening to be merged
	 */
	void EndInteraction()
	{
		bIsInteracting = false;
		LastInteractedKey.Reset();
	}

	/**
	 * Returns the transform stack index
	 */
	int32 GetTransformStackIndex() const { return TransformStackIndex; }

	/**
	 * Sends an event from the hierarchy to the world
	 * @param InEvent The event to send
	 * @param bAsynchronous If set to true the event will go on a thread safe queue
	 */
	void SendEvent(const FTLLRN_RigEventContext& InEvent, bool bAsynchronous = true);

	/**
	* Sends an autokey event from the hierarchy to the world
	* @param InElement The element to send the autokey for
	* @param InOffsetInSeconds The time offset in seconds
	* @param bAsynchronous If set to true the event will go on a thread safe queue
	*/
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	void SendAutoKeyEvent(FTLLRN_RigElementKey InElement, float InOffsetInSeconds = 0.f, bool bAsynchronous = true);

	/**
	 * Returns the delegate to listen to for events coming from this hierarchy
	 * @return The delegate to listen to for events coming from this hierarchy
	 */
	FTLLRN_RigEventDelegate& OnEventReceived() { return EventDelegate; }
	
	/**
	 * Returns true if the hierarchy controller is currently available
	 * The controller may not be available during certain events.
	 * If the controller is not available then GetController() will return nullptr.
	 */ 
	UFUNCTION(BlueprintPure, Category = UTLLRN_RigHierarchy)
	bool IsControllerAvailable() const;

	/**
	 * Returns a controller for this hierarchy.
	 * Note: If the controller is not available this will return nullptr 
	 * even if the bCreateIfNeeded flag is set to true. You can check the 
	 * controller's availability with IsControllerAvailable().
	 * @param bCreateIfNeeded Creates a controller if needed
	 * @return The Controller for this hierarchy
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	UTLLRN_RigHierarchyController* GetController(bool bCreateIfNeeded = true);

	/**
	 * Returns a rule manager for this hierarchy
	 * Note: If the manager is not available this will return nullptr 
	 * even if the bCreateIfNeeded flag is set to true.
	 * @param bCreateIfNeeded Creates a controller if needed
	 * @return The Controller for this hierarchy
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	UTLLRN_ModularRigRuleManager* GetRuleManager(bool bCreateIfNeeded = true);

	/**
	 * Returns the topology version of this hierarchy
	 */
	uint32 GetTopologyVersion() const { return TopologyVersion; }

	/**
	 * Returns the hash of this hierarchy used for cached element keys
	 */
	uint32 GetTopologyVersionHash() const
	{
		const uint32 Hash = HashCombine(
			(uint32)reinterpret_cast<long long>(this),
			GetTypeHash(TopologyVersion));
		if(ElementKeyRedirector)
		{
			return HashCombine(Hash, ElementKeyRedirector->GetHash());
		}
		return Hash;
	}

	/**
	 * Increments the topology version
	 */
	void IncrementTopologyVersion();

	/**
	 * Returns the metadata version of this hierarchy
	 */
	uint32 GetMetadataVersion() const { return MetadataVersion; }

	/**
     * Returns the metadata tag version of this hierarchy
	 */
	uint32 GetMetadataTagVersion() const { return MetadataTagVersion; }

	/**
	 * Returns the current / initial pose of the hierarchy
	 * @param bInitial If set to true the initial pose will be returned
	 * @return The pose of the hierarchy
	 * @param bIncludeTransientControls If true the transient controls will be included in the pose
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FTLLRN_RigPose GetPose(
		bool bInitial = false,
		bool bIncludeTransientControls = true
	) const
	{
		return GetPose(bInitial, ETLLRN_RigElementType::All, FTLLRN_RigElementKeyCollection(), bIncludeTransientControls);
	}

	/**
	 * Returns the current / initial pose of the hierarchy
	 * @param bInitial If set to true the initial pose will be returned
	 * @param InElementType The types of elements to get
	 * @param InItems An optional list of items to get
	 * @param bIncludeTransientControls If true the transient controls will be included in the pose
	 * @return The pose of the hierarchy
	 */
	FTLLRN_RigPose GetPose(
		bool bInitial,
		ETLLRN_RigElementType InElementType,
		const FTLLRN_RigElementKeyCollection& InItems ,
		bool bIncludeTransientControls = true
	) const;

	/**
	 * Returns the current / initial pose of the hierarchy
	 * @param bInitial If set to true the initial pose will be returned
	 * @param InElementType The types of elements to get
	 * @param InItems An optional list of items to get
	 * @param bIncludeTransientControls If true the transient controls will be included in the pose
	 * @return The pose of the hierarchy
	 */
	FTLLRN_RigPose GetPose(
		bool bInitial,
		ETLLRN_RigElementType InElementType,
		const TArrayView<const FTLLRN_RigElementKey>& InItems,
		bool bIncludeTransientControls = true
	) const;

	/**
	 * Sets the current / initial pose of the hierarchy
	 * @param InPose The pose to set on the hierarchy
	 * @param InTransformType The transform type to set
	 */
	void SetPose(
		const FTLLRN_RigPose& InPose,
		ETLLRN_RigTransformType::Type InTransformType = ETLLRN_RigTransformType::CurrentLocal
	)
	{
		SetPose(InPose, InTransformType, ETLLRN_RigElementType::All, FTLLRN_RigElementKeyCollection(), 1.f);
	}

	/**
	 * Sets the current / initial pose of the hierarchy
	 * @param InPose The pose to set on the hierarchy
	 * @param InTransformType The transform type to set
	 * @param InElementType The types of elements to set
	 * @param InItems An optional list of items to set
	 * @param InWeight A weight to define how much the pose needs to be mixed in
	 */
	void SetPose(
		const FTLLRN_RigPose& InPose,
		ETLLRN_RigTransformType::Type InTransformType,
		ETLLRN_RigElementType InElementType,
		const FTLLRN_RigElementKeyCollection& InItems,
		float InWeight
	);

	/**
	 * Sets the current / initial pose of the hierarchy
	 * @param InPose The pose to set on the hierarchy
	 * @param InTransformType The transform type to set
	 * @param InElementType The types of elements to set
	 * @param InItems An optional list of items to set
	 * @param InWeight A weight to define how much the pose needs to be mixed in
	 */
	void SetPose(
		const FTLLRN_RigPose& InPose,
		ETLLRN_RigTransformType::Type InTransformType,
		ETLLRN_RigElementType InElementType,
		const TArrayView<const FTLLRN_RigElementKey>& InItems, 
		float InWeight
	);

	/**
	 * Sets the current / initial pose of the hierarchy
	 * @param InPose The pose to set on the hierarchy
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy, meta = (DisplayName = "Set Pose", ScriptName = "SetPose"))
	void SetPose_ForBlueprint(FTLLRN_RigPose InPose)
	{
		return SetPose(InPose);
	}

	/**
	 * Sets the pose adapter used for storage of pose data
	 * @param InPoseAdapter The pose adapter to set on the hierarchy
	 */
	void LinkPoseAdapter(TSharedPtr<FTLLRN_RigHierarchyPoseAdapter> InPoseAdapter);

	/**
	 * Clears the pose adapter used for storage of pose data
	 */
	void UnlinkPoseAdapter()
	{
		return LinkPoseAdapter(nullptr);
	}

	/**
	 * Creates a rig control value from a bool value
	 * @param InValue The value to create the rig control value from
	 * @return The converted control rig val ue
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	static FTLLRN_RigControlValue MakeControlValueFromBool(bool InValue)
	{
		return FTLLRN_RigControlValue::Make<bool>(InValue);
	}

	/**
	 * Creates a rig control value from a float value
	 * @param InValue The value to create the rig control value from
	 * @return The converted control rig val ue
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    static FTLLRN_RigControlValue MakeControlValueFromFloat(float InValue)
	{
		return FTLLRN_RigControlValue::Make<float>(InValue);
	}

	/**
	 * Returns the contained float value from a a Rig Control Value
	 * @param InValue The Rig Control value to convert from
	 * @return The converted float value
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	static float GetFloatFromControlValue(FTLLRN_RigControlValue InValue)
	{
		return InValue.Get<float>();
	}

	/**
	 * Creates a rig control value from a int32 value
	 * @param InValue The value to create the rig control value from
	 * @return The converted control rig val ue
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    static FTLLRN_RigControlValue MakeControlValueFromInt(int32 InValue)
	{
		return FTLLRN_RigControlValue::Make<int32>(InValue);
	}

	/**
	 * Returns the contained int32 value from a a Rig Control Value
	 * @param InValue The Rig Control value to convert from
	 * @return The converted int32 value
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	static int32 GetIntFromControlValue(FTLLRN_RigControlValue InValue)
	{
		return InValue.Get<int32>();
	}

	/**
	 * Creates a rig control value from a FVector2D value
	 * @param InValue The value to create the rig control value from
	 * @return The converted control rig val ue
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    static FTLLRN_RigControlValue MakeControlValueFromVector2D(FVector2D InValue)
	{
		return FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(InValue.X, InValue.Y, 0.f));
	}

	/**
	 * Returns the contained FVector2D value from a a Rig Control Value
	 * @param InValue The Rig Control value to convert from
	 * @return The converted FVector2D value
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	static FVector2D GetVector2DFromControlValue(FTLLRN_RigControlValue InValue)
	{
		const FVector3f Vector = InValue.Get<FVector3f>();
		return FVector2D(Vector.X, Vector.Y);
	}

	/**
	 * Creates a rig control value from a FVector value
	 * @param InValue The value to create the rig control value from
	 * @return The converted control rig val ue
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    static FTLLRN_RigControlValue MakeControlValueFromVector(FVector InValue)
	{
		return FTLLRN_RigControlValue::Make<FVector>(InValue);
	}

	/**
	 * Returns the contained FVector value from a a Rig Control Value
	 * @param InValue The Rig Control value to convert from
	 * @return The converted FVector value
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	static FVector GetVectorFromControlValue(FTLLRN_RigControlValue InValue)
	{
		return (FVector)InValue.Get<FVector3f>();
	}

	/**
	 * Creates a rig control value from a FRotator value
	 * @param InValue The value to create the rig control value from
	 * @return The converted control rig val ue
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    static FTLLRN_RigControlValue MakeControlValueFromRotator(FRotator InValue)
	{
		return FTLLRN_RigControlValue::Make<FVector>(InValue.Euler());
	}

	/**
	 * Returns the contained FRotator value from a a Rig Control Value
	 * @param InValue The Rig Control value to convert from
	 * @return The converted FRotator value
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	static FRotator GetRotatorFromControlValue(FTLLRN_RigControlValue InValue)
	{
		return FRotator::MakeFromEuler((FVector)InValue.Get<FVector3f>());
	}

	/**
	 * Creates a rig control value from a FTransform value
	 * @param InValue The value to create the rig control value from
	 * @return The converted control rig val ue
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    static FTLLRN_RigControlValue MakeControlValueFromTransform(FTransform InValue)
	{
		return FTLLRN_RigControlValue::Make<FTLLRN_RigControlValue::FTransform_Float>(InValue);
	}

	/**
	 * Returns the contained FTransform value from a a Rig Control Value
	 * @param InValue The Rig Control value to convert from
	 * @return The converted FTransform value
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	static FTransform GetTransformFromControlValue(FTLLRN_RigControlValue InValue)
	{
		return InValue.Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
	}

	/**
	 * Creates a rig control value from a FEulerTransform value
	 * @param InValue The value to create the rig control value from
	 * @return The converted control rig val ue
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    static FTLLRN_RigControlValue MakeControlValueFromEulerTransform(FEulerTransform InValue)
	{
		return FTLLRN_RigControlValue::Make<FTLLRN_RigControlValue::FEulerTransform_Float>(InValue);
	}

	/**
	 * Returns the contained FEulerTransform value from a a Rig Control Value
	 * @param InValue The Rig Control value to convert from
	 * @return The converted FEulerTransform value
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	static FEulerTransform GetEulerTransformFromControlValue(FTLLRN_RigControlValue InValue)
	{
		return InValue.Get<FTLLRN_RigControlValue::FEulerTransform_Float>().ToTransform();
	}

	/**
	 * Creates a rig control value from a FTransformNoScale value
	 * @param InValue The value to create the rig control value from
	 * @return The converted control rig val ue
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
    static FTLLRN_RigControlValue MakeControlValueFromTransformNoScale(FTransformNoScale InValue)
	{
		return FTLLRN_RigControlValue::Make<FTLLRN_RigControlValue::FTransformNoScale_Float>(InValue);
	}

	/**
	 * Returns the contained FTransformNoScale value from a a Rig Control Value
	 * @param InValue The Rig Control value to convert from
	 * @return The converted FTransformNoScale value
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	static FTransformNoScale GetTransformNoScaleFromControlValue(FTLLRN_RigControlValue InValue)
	{
		return InValue.Get<FTLLRN_RigControlValue::FTransformNoScale_Float>().ToTransform();
	}

private:

	FTLLRN_RigHierarchyModifiedEvent ModifiedEvent;

	UPROPERTY(BlueprintReadOnly, Category = TLLRN_RigHierarchy, meta = (ScriptName = "ModifiedEvent", AllowPrivateAccess = "true"))
	FTLLRN_RigHierarchyModifiedDynamicEvent ModifiedEventDynamic;
	
	FTLLRN_RigHierarchyMetadataChangedDelegate MetadataChangedDelegate;
	FTLLRN_RigHierarchyMetadataTagChangedDelegate MetadataTagChangedDelegate;
	FTLLRN_RigEventDelegate EventDelegate;

	TSharedPtr<FTLLRN_RigHierarchyPoseAdapter> PoseAdapter;

public:

	void Notify(ETLLRN_RigHierarchyNotification InNotifType, const FTLLRN_RigBaseElement* InElement);

	/**
	 * Returns a transform based on a given transform type
	 * @param InTransformElement The element to retrieve the transform for
	 * @param InTransformType The type of transform to retrieve
	 * @return The local current or initial transform's value.
	 */
	FTransform GetTransform(FTLLRN_RigTransformElement* InTransformElement, const ETLLRN_RigTransformType::Type InTransformType) const;

	/**
	 * Returns a transform for a given element's parent based on the transform type
	 * If the element does not have a parent FTransform::Identity will be returned.
	 * @param InElement The element to retrieve the transform for
	 * @param InTransformType The type of transform to retrieve
	 * @return The element's parent's transform
	 */
	FTransform GetParentTransform(FTLLRN_RigBaseElement* InElement, const ETLLRN_RigTransformType::Type InTransformType) const;

	/**
	 * Sets a transform for a given element based on the transform type
	 * @param InTransformElement The element to set the transform for
	 * @param InTransform The type of transform to set
	 * @param InTransformType The type of transform to set
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 * @param bForce Set the transform even if it is the same as the previously set one
	 */
	void SetTransform(FTLLRN_RigTransformElement* InTransformElement, const FTransform& InTransform, const ETLLRN_RigTransformType::Type InTransformType, bool bAffectChildren, bool bSetupUndo = false, bool bForce = false, bool bPrintPythonCommands = false);

	/**
	 * Returns the global offset transform for a given control element.
	 * @param InControlElement The control element to retrieve the offset transform for
	 * @param InTransformType The type of transform to set
	 * @return The global offset transform
	 */
	FTransform GetControlOffsetTransform(FTLLRN_RigControlElement* InControlElement, const ETLLRN_RigTransformType::Type InTransformType) const;

	/**
	 * Sets the offset transform for a given control element
	 * @param InControlElement The element to set the transform for
	 * @param InTransform The offset transform to set
	 * @param InTransformType The type of transform to set. Note: for offset transform, setting the initial value also updates the current value
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 * @param bForce Set the transform even if it is the same as the previously set one
	 */
	void SetControlOffsetTransform(FTLLRN_RigControlElement* InControlElement, const FTransform& InTransform, const ETLLRN_RigTransformType::Type InTransformType, bool bAffectChildren, bool bSetupUndo = false, bool bForce = false, bool bPrintPythonCommands = false);

	/**
	 * Returns the global shape transform for a given control element.
	 * @param InControlElement The control element to retrieve the shape transform for
	 * @param InTransformType The type of transform to set
	 * @return The global shape transform
	 */
	FTransform GetControlShapeTransform(FTLLRN_RigControlElement* InControlElement, const ETLLRN_RigTransformType::Type InTransformType) const;

	/**
	 * Sets the shape transform for a given control element
	 * @param InControlElement The element to set the transform for
	 * @param InTransform The shape transform to set
	 * @param InTransformType The type of transform to set. Note: for shape transform, setting the initial value also updates the current value
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 * @param bForce Set the transform even if it is the same as the previously set one
	 */
	void SetControlShapeTransform(FTLLRN_RigControlElement* InControlElement, const FTransform& InTransform, const ETLLRN_RigTransformType::Type InTransformType, bool bSetupUndo = false, bool bForce = false, bool bPrintPythonCommands = false);

	/**
	 * Sets the control settings for a given control element
	 * @param InControlElement The element to set the settings for
	 * @param InSettings The new control settings value to set
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	void SetControlSettings(FTLLRN_RigControlElement* InControlElement, FTLLRN_RigControlSettings InSettings, bool bSetupUndo = false, bool bForce = false, bool bPrintPythonCommands = false);

	/**
	 * Returns a control's current value
	 * @param InControlElement The element to retrieve the current value for
	 * @param InValueType The type of value to return
	 * @param bUsePreferredAngles When true will use euler preferred angles to compute the value
	 * @return Returns the current value of the control
	 */
	FTLLRN_RigControlValue GetControlValue(FTLLRN_RigControlElement* InControlElement, ETLLRN_RigControlValueType InValueType, bool bUsePreferredAngles = true) const;

	void SetPreferredEulerAnglesFromValue(FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlValue& InValue,
	                                      const ETLLRN_RigControlValueType& InValueType, bool bFixEulerFlips);

	template<typename T>
	T GetControlValue(FTLLRN_RigControlElement* InControlElement, ETLLRN_RigControlValueType InValueType) const
	{
		return GetControlValue(InControlElement, InValueType).Get<T>();
	}

	/**
	 * Sets a control's current value
	 * @param InControlElement The element to set the current value for
	 * @param InValueType The type of value to set
	 * @param InValue The value to set on the control
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 * @param bForce Set the transform even if it is the same as the previously set one
	 */
	void SetControlValue(FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlValue& InValue, ETLLRN_RigControlValueType InValueType, bool bSetupUndo = false, bool bForce = false, bool bPrintPythonCommands = false, bool bFixEulerFlips = false);

	template<typename T>
	void SetControlValue(FTLLRN_RigControlElement* InControlElement, const T& InValue, ETLLRN_RigControlValueType InValueType, bool bSetupUndo = false, bool bForce = false) const
	{
		SetControlValue(InControlElement, FTLLRN_RigControlValue::Make<T>(InValue), InValueType, bSetupUndo, bForce);
	}

	/**
	 * Sets a control's current visibility
	 * @param InControlElement The element to set the visibility for
	 * @param bVisibility The new visibility for the control
	 */
	void SetControlVisibility(FTLLRN_RigControlElement* InControlElement, bool bVisibility);

	/**
	 * Sets the connector settings for a given connector element
	 * @param InConnectorElement The element to set the settings for
	 * @param InSettings The new connector settings value to set
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 */
	void SetConnectorSettings(FTLLRN_RigConnectorElement* InConnectorElement, FTLLRN_RigConnectorSettings InSettings, bool bSetupUndo = false, bool bForce = false, bool bPrintPythonCommands = false);

	/**
	 * Returns a curve's value. If the curve value is not set, returns 
	 * @param InCurveElement The element to retrieve the value for
	 * @return Returns the value of the curve
	 */
	float GetCurveValue(FTLLRN_RigCurveElement* InCurveElement) const;

	/**
	 * Returns whether a curve's value is set. If the curve value is not set, returns false. 
	 * @param InCurveElement The element to retrieve the value for
	 * @return Returns true if the value is set, false otherwise.
	 */
	bool IsCurveValueSet(FTLLRN_RigCurveElement* InCurveElement) const;

	/**
	 * Sets a curve's value
	 * @param InCurveElement The element to set the value for
	 * @param InValue The value to set on the control
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 * @param bForce Set the transform even if it is the same as the previously set one
	 */
	void SetCurveValue(FTLLRN_RigCurveElement* InCurveElement, float InValue, bool bSetupUndo = false, bool bForce = false);

	/**
	 * Unsets a curve's value. Basically the curve's value becomes meaningless.
	 * @param InCurveElement The element to set the value for
	 * @param bSetupUndo If true the transform stack will be setup for undo / redo
	 * @param bForce Unset the curve even if it was already unset.
	 */
	void UnsetCurveValue(FTLLRN_RigCurveElement* InCurveElement, bool bSetupUndo = false, bool bForce = false);

	/**
	 * Returns the previous name of an element prior to a rename operation
	 * @param InKey The key of the element to request the old name for
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FName GetPreviousName(const FTLLRN_RigElementKey& InKey) const;

	/**
	 * Returns the previous parent of an element prior to a reparent operation
	 * @param InKey The key of the element to request the old parent  for
	 */
	UFUNCTION(BlueprintCallable, Category = UTLLRN_RigHierarchy)
	FTLLRN_RigElementKey GetPreviousParent(const FTLLRN_RigElementKey& InKey) const;

	/**
	 * Returns true if an element is parented to another element
	 * @param InChild The child element to check for a parent
	 * @param InParent The parent element to check for
	 * @param InDependencyMap An additional map of dependencies to respect
	 * @return True if the given parent and given child have a parent-child relationship
	 */
	bool IsParentedTo(FTLLRN_RigBaseElement* InChild, FTLLRN_RigBaseElement* InParent, const TElementDependencyMap& InDependencyMap = TElementDependencyMap()) const;

private:
	/**
	 * Returns true if an element is affected to another element
	 * @param InDependent The dependent element to check for a dependency
	 * @param InDependency The dependency element to check for
	 * @param InDependencyMap An additional map of dependencies to respect
	 * @param bIsOnActualTopology Indicates that the passed dependent and dependency are expected to be on the current topology (if false they are provided with the dependency map)
	 * @return True if the given dependent is affected by the given dependency 
	 */
	bool IsDependentOn(FTLLRN_RigBaseElement* InDependent, FTLLRN_RigBaseElement* InDependency, const TElementDependencyMap& InDependencyMap = TElementDependencyMap(), bool bIsOnActualTopology = true) const;

public:
	/**
	 * Returns the index of an element given its key within its default parent (or root)
	 * @param InElement The element to retrieve the index for
	 * @return The index of the element or INDEX_NONE
	 */
	int32 GetLocalIndex(const FTLLRN_RigBaseElement* InElement) const;

	/**
	 * Returns a reference to the suspend notifications flag
	 */
	bool& GetSuspendNotificationsFlag() { return bSuspendNotifications; }

	/*
	 * Returns true if a hierarchy will record any change.
	 * This is used for debugging purposes.
	 */
	bool IsTracingChanges() const;

	/**
	 * Returns true if the control is animatable
	 */
	bool IsAnimatable(const FTLLRN_RigElementKey& InKey) const;

	/**
	 * Returns true if the control is animatable
	 */
	bool IsAnimatable(const FTLLRN_RigControlElement* InControlElement) const;

	/**
	 * Returns true if the control should be grouped in editor
	 */
	bool ShouldBeGrouped(const FTLLRN_RigElementKey& InKey) const;

	/**
	 * Returns true if the control should be grouped in editor
	 */
	bool ShouldBeGrouped(const FTLLRN_RigControlElement* InControlElement) const;

#if WITH_EDITOR

	/**
	 * Clears the undo / redo stack of this hierarchy
	 */
	void ResetTransformStack();

	/**
	 * Stores the current pose for tracing
	 */
	void StorePoseForTrace(const FString& InPrefix);

	/**
	 * Updates the format for trace floating point numbers
	 */
	static void CheckTraceFormatIfRequired();
	
	/**
	 * Dumps the content of the transform stack to a string
	 */
	void DumpTransformStackToFile(FString* OutFilePath = nullptr);

	/**
	 * Tells this hierarchy to trace a series of frames
	 */
	void TraceFrames(int32 InNumFramesToTrace);
	
#endif

private:

	/**
	 * Returns true if a given element is selected
	 * @param InElement The element to check
	 * @return true if a given element is selected
	 */
    bool IsSelected(const FTLLRN_RigBaseElement* InElement) const;

	/**
	 * Updates the transient cached children table if the topology version is out of date with the one
	 * stored with the cached table.
	 * @return Returns true if a change was performed
	 */
	void EnsureCachedChildrenAreCurrent() const;

	void UpdateCachedChildren();
	
	/**
	 * Corrects a parent element key for space switching
	 */
	FTLLRN_RigElementKey PreprocessParentElementKeyForSpaceSwitching(const FTLLRN_RigElementKey& InChildKey, const FTLLRN_RigElementKey& InParentKey);

	/*
	 * Helper function to create an element for a given type
	 */
	FTLLRN_RigBaseElement* MakeElement(ETLLRN_RigElementType InElementType, int32 InCount = 1, int32* OutStructureSize = nullptr); 

	/*
	* Helper function to create an element for a given type
	*/
	void DestroyElement(FTLLRN_RigBaseElement*& InElement);

	/*
	 * Templated helper function to create an element
	 */
	template<typename ElementType = FTLLRN_RigBaseElement>
	ElementType* NewElement(int32 Num = 1, bool bAllocateStorage = false)
	{
		ElementType* NewElements = static_cast<ElementType*>(FMemory::Malloc(sizeof(ElementType) * Num));
		for(int32 Index=0;Index<Num;Index++)
		{
			new(&NewElements[Index]) ElementType(this);
		}
		NewElements[0].OwnedInstances = Num;
		if(bAllocateStorage)
		{
			for(int32 Index=0;Index<Num;Index++)
			{
				AllocateDefaultElementStorage(&NewElements[Index], false);
			}
		}
		return NewElements;
	}

	/**
	 * Marks all affected elements of a given element as dirty
	 * @param InTransformElement The element that has changed
	 * @param bInitial If true the initial transform will be dirtied
	 * @param bAffectChildren If set to false children will not move (maintain global).
	 */
	void PropagateDirtyFlags(FTLLRN_RigTransformElement* InTransformElement, bool bInitial, bool bAffectChildren, bool bComputeOpposed = true, bool bMarkDirty = true) const;

public:

	/**
	* Performs validation of the cache within the hierarchy on any mutation.
	*/
	void EnsureCacheValidity() const
	{
#if WITH_EDITOR
		if(bEnableCacheValidityCheck)
		{
			UTLLRN_RigHierarchy* MutableThis = (UTLLRN_RigHierarchy*)this; 
			MutableThis->EnsureCacheValidityImpl();
		}
#endif
	}

	/*
	 * Cleans up caches after load
	 */
	void CleanupInvalidCaches();

private:
	
	/**
	 * The topology version of the hierarchy changes when elements are
	 * added, removed, re-parented or renamed.
	 */
	UPROPERTY(transient)
	uint32 TopologyVersion;

	/**
	 * The metadata version of the hierarchy changes when metadata is being
	 * created or removed (not when the metadata values changes)
	 */
	UPROPERTY(transient)
	uint32 MetadataVersion;

	/**
	 * The metadata version of the hierarchy changes when metadata is being
	 * created or removed (not when the metadata values changes)
	 */
	UPROPERTY(transient)
	uint16 MetadataTagVersion;

	/**
	 * If set to false the dirty flag propagation will be disabled
	 */
	UPROPERTY(transient)
	bool bEnableDirtyPropagation;

	// Storage for the elements
	mutable TArray<FTLLRN_RigBaseElement*> Elements;
	mutable FCriticalSection ElementsLock;

	// Storage for the elements
	mutable TArray<TArray<FTLLRN_RigBaseElement*>> ElementsPerType;

	TArray<int32> ChangedCurveIndices;
	bool bRecordCurveChanges;
	
	//
	struct FMetadataStorage
	{
		TMap<FName, FTLLRN_RigBaseMetadata*> MetadataMap;
		FName LastAccessName;
		FTLLRN_RigBaseMetadata* LastAccessMetadata = nullptr;

		void Reset();
		void Serialize(FArchive& Ar);

		friend FArchive& operator<<(FArchive& Ar, FMetadataStorage& Storage)
		{
			Storage.Serialize(Ar);
			return Ar;
		}
	};

	// Managed lookup from Key to Index
	TMap<FTLLRN_RigElementKey, int32> IndexLookup;

	TMap<FTLLRN_RigElementKey, FString> UserDefinedElementName;

	// Per element pose storage. Storage is defined here rather than on the elements
	// to reduce memory consumption. Only elements created by MakeElement point to
	// the element storage. Copied elements via the copy constructor or copy operator
	// do not have UTLLRN_RigHierarchy as an owner and therefore do not carry poses with them.
	FRigReusableElementStorage<FTransform> ElementTransforms;

	// Per element dirty state storage. Storage is defined here rather than on the elements
	// to reduce memory consumption. Only elements created by MakeElement point to
	// the element storage. Copied elements via the copy constructor or copy operator
	// do not have UTLLRN_RigHierarchy as an owner and therefore do not carry metadata with them.
	FRigReusableElementStorage<bool> ElementDirtyStates;

	// Per element curve storage. Storage is defined here rather than on the elements
	// to reduce memory consumption. Only elements created by MakeElement point to
	// the element storage. Copied elements via the copy constructor or copy operator
	// do not have UTLLRN_RigHierarchy as an owner and therefore do not carry curves with them.
	FRigReusableElementStorage<float> ElementCurves;

	// A list of ranges which can be used to copy all poses from initial to current, for example 
	TMap<ETLLRN_RigTransformType::Type, TTuple<int32, int32>> ElementTransformRanges;

	// Allocates the default element storage for an element
	void AllocateDefaultElementStorage(FTLLRN_RigBaseElement* InElement, bool bUpdateAllElements);

	// Deallocates the default element storage for an element
	void DeallocateElementStorage(FTLLRN_RigBaseElement* InElement);

	// Updates all storage pointers of the elements for poses and dirty states
	void UpdateElementStorage();

	// Orders the element storage by storing first initial, then current,
	// within each first local, then global, and within each of those lists
	// we'll place bones, nulls, controls etc in that order
	bool SortElementStorage();

	// Compacts the element storage
	bool ShrinkElementStorage();

	// Helper function to iterate all transform element storage
	void ForEachTransformElementStorage(TFunction<void(FTLLRN_RigTransformElement*,ETLLRN_RigTransformType::Type,ETLLRN_RigTransformStorageType::Type,FTLLRN_RigComputedTransform*,FTLLRN_RigTransformDirtyState*)> InCallback);

	// Returns the computed transform and dirty state for a given element
	TTuple<FTLLRN_RigComputedTransform*,FTLLRN_RigTransformDirtyState*> GetElementTransformStorage(
		const FTLLRN_RigElementKeyAndIndex& InKey,
		ETLLRN_RigTransformType::Type InTransformType,
		ETLLRN_RigTransformStorageType::Type InStorageType = ETLLRN_RigTransformStorageType::Pose);

	// Returns the range of the element transform / dirty state storage for a given
	// transform type. this is only valid if the element storage has been sorted. 
	TOptional<TTuple<int32,int32>> GetElementStorageRange(ETLLRN_RigTransformType::Type InTransformType) const;

	// Element metadata storage. Storage is defined here rather than on the elements
	// to reduce memory consumption. Only elements created by MakeElement point to
	// the element storage. Copied elements via the copy constructor or copy operator
	// do not have UTLLRN_RigHierarchy as an owner and therefore do not carry metadata with them.
	FRigReusableElementStorage<FMetadataStorage> ElementMetadata;

	// A quick-lookup cache for elements' children. Each element that has a ChildCacheIndex
	// not equal to INDEX_NONE is an index into the offset and count cache below, which in
	// turn contains an offset into the ChildElementCache, which stores consecutive runs of
	// children for that element. This makes it quick to get a TArrayView on the list of
	// children.
	struct FChildElementOffsetAndCount
	{
		int32 Offset;
		int32 Count;
	};
	
	TArray<FChildElementOffsetAndCount> ChildElementOffsetAndCountCache;
	TArray<FTLLRN_RigBaseElement*> ChildElementCache;

	// The topology version at which the child element cache was constructed. If it differs
	// from the stored TopologyVersion, then the cache is rebuilt. 
	uint32 ChildElementCacheTopologyVersion = std::numeric_limits<uint32>::max();

	///////////////////////////////////////////////
	/// Undo redo related
	///////////////////////////////////////////////

	/**
	 * The index identifying where we stand with the stack
	 */
	UPROPERTY()
	int32 TransformStackIndex;

	/**
	 * A flag to indicate if the next serialize should contain only transform changes
	 */
	bool bTransactingForTransformChange;
	
	/**
	 * The stack of actions to undo.
	 * Note: This is also used when performing traces on the hierarchy.
	 */
	TArray<FTLLRN_RigTransformStackEntry> TransformUndoStack;

	/**
	 * The stack of actions to undo
	 */
	TArray<FTLLRN_RigTransformStackEntry> TransformRedoStack;

	/**
	 * Sets the transform stack index - which in turns performs a series of undos / redos
	 * @param InTransformStackIndex The new index for the transform stack
	 */
	bool SetTransformStackIndex(int32 InTransformStackIndex);

	/**
	 * Stores a transform on the stack
	 */
	void PushTransformToStack(
			const FTLLRN_RigElementKey& InKey,
            ETLLRN_RigTransformStackEntryType InEntryType,
            ETLLRN_RigTransformType::Type InTransformType,
            const FTransform& InOldTransform,
            const FTransform& InNewTransform,
            bool bAffectChildren,
            bool bModify);

	/**
	 * Stores a curve value on the stack
	 */
	void PushCurveToStack(
            const FTLLRN_RigElementKey& InKey,
            float InOldCurveValue,
            float InNewCurveValue,
            bool bInOldIsCurveValueSet,
            bool bInNewIsCurveValueSet,
            bool bModify);

	/**
	 * Restores a transform on the stack
	 */
	bool ApplyTransformFromStack(const FTLLRN_RigTransformStackEntry& InEntry, bool bUndo);

	/**
	 * Computes all parts of the pose
	 */
	void ComputeAllTransforms();

	/**
	 * Manages merging transform actions into one during an interaction
	 */
	bool bIsInteracting;

	/**
	* Stores the last key being interacted on
	*/
	FTLLRN_RigElementKey LastInteractedKey;

	/** 
	 * If set to true all notifs coming from this hierarchy will be suspended
	 */
	bool bSuspendNotifications;

	/** 
	 * If set to true all metadata changes notifs coming from this hierarchy will be suspended
	 */
	bool bSuspendMetadataNotifications = false;

	/**
	 * The event fired during undo / redo
	 */
	FTLLRN_RigHierarchyUndoRedoTransformEvent UndoRedoEvent;

	TWeakObjectPtr<UTLLRN_RigHierarchy> HierarchyForSelectionPtr;
	TWeakObjectPtr<UTLLRN_RigHierarchy> DefaultHierarchyPtr;
	TArray<FTLLRN_RigElementKey> OrderedSelection;

	UPROPERTY(Transient)
	TObjectPtr<UTLLRN_RigHierarchyController> HierarchyController;
	bool bIsControllerAvailable;

	UPROPERTY(Transient)
	mutable TObjectPtr<UTLLRN_ModularRigRuleManager> RuleManager;

	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> PreviousParentMap;

	/*We save this so Sequencer can remap this after load*/
	UPROPERTY()
	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> PreviousNameMap;

	int32 ResetPoseHash;
	TArray<bool> ResetPoseIsFilteredOut;
	TArray<int32> ElementsToRetainLocalTransform;

	mutable THierarchyCache<TMap<TTuple<int32, int32>, bool>> ElementDependencyCache;
	mutable TArray<bool> ElementDependencyVisited;

	bool bIsCopyingHierarchy;

#if WITH_EDITOR

	// this is mainly used for propagating changes between hierarchies in the direction of blueprint -> CDO -> other instances
	struct FTLLRN_RigHierarchyListener
	{
		FTLLRN_RigHierarchyListener()
			: Hierarchy(nullptr)
			, bShouldReactToInitialChanges(true)
			, bShouldReactToCurrentChanges(true) 
		{}

		bool ShouldReactToChange(ETLLRN_RigTransformType::Type InTransformType) const
		{
			if(Hierarchy.IsValid())
			{
				if(ETLLRN_RigTransformType::IsInitial(InTransformType))
				{
					return bShouldReactToInitialChanges;
				}

				if(ETLLRN_RigTransformType::IsCurrent(InTransformType))
				{
					return bShouldReactToCurrentChanges;
				}
			}
			return false;
		}
		
		TWeakObjectPtr<UTLLRN_RigHierarchy> Hierarchy;
		bool bShouldReactToInitialChanges;
		bool bShouldReactToCurrentChanges;
	};
	
	TArray<FTLLRN_RigHierarchyListener> ListeningHierarchies;
	friend class FTLLRN_RigHierarchyListenerGuard;

	// a bool to guard against circular dependencies among listening hierarchies
	bool bPropagatingChange;

	// a bool to disable any propagation checks and force propagation
	bool bForcePropagation;
	
#endif

#if WITH_EDITOR

protected:
	
	int32 TraceFramesLeft;
	int32 TraceFramesCaptured;
	TMap<FName, FTLLRN_RigPose> TracePoses;

#endif

	static int32 TLLRN_RigElementTypeToFlatIndex(ETLLRN_RigElementType InElementType)
	{
		switch(InElementType)
		{
			case ETLLRN_RigElementType::Bone:
			{
				return 0;
			}
			case ETLLRN_RigElementType::Null:
			{
				return 1;
			}
			case ETLLRN_RigElementType::Control:
			{
				return 2;
			}
			case ETLLRN_RigElementType::Curve:
			{
				return 3;
			}
			case ETLLRN_RigElementType::Physics:
			{
				return 4;
			}
			case ETLLRN_RigElementType::Reference:
			{
				return 5;
			}
			case ETLLRN_RigElementType::Connector:
			{
				return 6;
			}
			case ETLLRN_RigElementType::Socket:
			{
				return 7;
			}
			case ETLLRN_RigElementType::All:
			default:
			{
				break;
			}
		}

		return INDEX_NONE;
	}

	static ETLLRN_RigElementType FlatIndexToTLLRN_RigElementType(int32 InIndex)
	{
		switch(InIndex)
		{
			case 0:
			{
				return ETLLRN_RigElementType::Bone;
			}
			case 1:
			{
				return ETLLRN_RigElementType::Null;
			}
			case 2:
			{
				return ETLLRN_RigElementType::Control;
			}
			case 3:
			{
				return ETLLRN_RigElementType::Curve;
			}
			case 4:
			{
				return ETLLRN_RigElementType::Physics;
			}
			case 5:
			{
				return ETLLRN_RigElementType::Reference;
			}
			case 6:
			{
				return ETLLRN_RigElementType::Connector;
			}
			case 7:
			{
				return ETLLRN_RigElementType::Socket;
			}
			default:
			{
				break;
			}
		}

		return ETLLRN_RigElementType::None;
	}

public:

	const FTLLRN_RigElementKeyCollection* FindCachedCollection(uint32 InHash) const
	{
		return KeyCollectionCache.Find(InHash);
	}
	
	FTLLRN_RigElementKeyCollection& FindOrAddCachedCollection(uint32 InHash) const
	{
		return KeyCollectionCache.FindOrAdd(InHash);
	};
	
	void AddCachedCollection(uint32 InHash, const FTLLRN_RigElementKeyCollection& InCollection) const
	{
		KeyCollectionCache.Add(InHash, InCollection);
	}

private:
	
	mutable TMap<uint32, FTLLRN_RigElementKeyCollection> KeyCollectionCache;

	FTransform GetWorldTransformForReference(const FRigVMExecuteContext* InContext, const FTLLRN_RigElementKey& InKey, bool bInitial);
	
	static float GetWeightForLerp(const float WeightA, const float WeightB)
	{
		float Weight = 0.f;
		const float ClampedWeightA = FMath::Max(WeightA, 0.f);
		const float ClampedWeightB = FMath::Max(WeightB, 0.f);
		const float OverallWeight = ClampedWeightA + ClampedWeightB;
		if(OverallWeight > SMALL_NUMBER)
		{
			Weight = ClampedWeightB / OverallWeight;
		}
		return Weight;
	}

	struct FConstraintIndex
	{
		int32 Location;
		int32 Rotation;
		int32 Scale;

		FConstraintIndex()
			: Location(INDEX_NONE)
			, Rotation(INDEX_NONE)
			, Scale(INDEX_NONE)
		{}

		FConstraintIndex(int32 InIndex)
			: Location(InIndex)
			, Rotation(InIndex)
			, Scale(InIndex)
		{}
	};

	FTransform ComputeLocalControlValue(
		FTLLRN_RigControlElement* ControlElement,
		const FTransform& InGlobalTransform,
		ETLLRN_RigTransformType::Type InTransformType) const;
	
	FTransform SolveParentConstraints(
		const FTLLRN_RigElementParentConstraintArray& InConstraints,
		const ETLLRN_RigTransformType::Type InTransformType,
		const FTransform& InLocalOffsetTransform,
		bool bApplyLocalOffsetTransform,
		const FTransform& InLocalPoseTransform,
		bool bApplyLocalPoseTransform) const;

	FTransform InverseSolveParentConstraints(
		const FTransform& InGlobalTransform,
		const FTLLRN_RigElementParentConstraintArray& InConstraints,
		const ETLLRN_RigTransformType::Type InTransformType,
		const FTransform& InLocalOffsetTransform) const;

	FTransform LazilyComputeParentConstraint(
		const FTLLRN_RigElementParentConstraintArray& Constraints,
		int32 InIndex,
		const ETLLRN_RigTransformType::Type InTransformType,
		const FTransform& InLocalOffsetTransform,
		bool bApplyLocalOffsetTransform,
		const FTransform& InLocalPoseTransform,
		bool bApplyLocalPoseTransform) const;

	static void ComputeParentConstraintIndices(
		const FTLLRN_RigElementParentConstraintArray& InConstraints,
		ETLLRN_RigTransformType::Type InTransformType,
		FConstraintIndex& OutFirstConstraint,
		FConstraintIndex& OutSecondConstraint,
		FConstraintIndex& OutNumConstraintsAffecting,
		FTLLRN_RigElementWeight& OutTotalWeight
	);

	static void IntegrateParentConstraintVector(
		FVector& OutVector,
		const FTransform& InTransform,
		float InWeight,
		bool bIsLocation);

	static void IntegrateParentConstraintQuat(
		int32& OutNumMixedRotations,
		FQuat& OutFirstRotation,
		FQuat& OutMixedRotation,
		const FTransform& InTransform,
		float InWeight);

#if WITH_EDITOR
	static TArray<FString> ControlSettingsToPythonCommands(const FTLLRN_RigControlSettings& Settings, const FString& NameSettings);
	static TArray<FString> ConnectorSettingsToPythonCommands(const FTLLRN_RigConnectorSettings& Settings, const FString& NameSettings);
#endif

	template<typename T>
	const T& GetMetadata(const FTLLRN_RigElementKey& InItem, ETLLRN_RigMetadataType InType, const FTLLRN_RigName& InMetadataName, const T& DefaultValue) const
	{
		return GetMetadata<T>(Find(InItem), InType, InMetadataName, DefaultValue);
	}

	template<typename T>
	const T& GetMetadata(const FTLLRN_RigBaseElement* InElement, ETLLRN_RigMetadataType InType, const FTLLRN_RigName& InMetadataName, const T& DefaultValue) const
	{
		if(InElement)
		{
			if(const FTLLRN_RigBaseMetadata* Metadata = FindMetadataForElement(InElement, InMetadataName, InType))
			{
				return *static_cast<const T*>(Metadata->GetValueData());
			}
		}
		return DefaultValue;
	}

	template<typename T>
	const TArray<T>& GetArrayMetadata(const FTLLRN_RigElementKey& InItem, ETLLRN_RigMetadataType InType, const FTLLRN_RigName& InMetadataName) const
	{
		return GetArrayMetadata<T>(Find(InItem), InType, InMetadataName);
	}

	template<typename T>
	const TArray<T>& GetArrayMetadata(const FTLLRN_RigBaseElement* InElement, ETLLRN_RigMetadataType InType, const FTLLRN_RigName& InMetadataName) const
	{
		static const TArray<T> EmptyArray;
		return GetMetadata<TArray<T>>(InElement, InType, InMetadataName, EmptyArray);
	}

	template<typename T>
	bool SetMetadata(const FTLLRN_RigElementKey& InItem, ETLLRN_RigMetadataType InType, const FTLLRN_RigName& InMetadataName, const T& InValue)
	{
		return SetMetadata<T>(Find(InItem), InType, InMetadataName, InValue);
	}

	template<typename T>
	bool SetMetadata(FTLLRN_RigBaseElement* InElement, ETLLRN_RigMetadataType InType, const FTLLRN_RigName& InMetadataName, const T& InValue)
	{
		if(InElement)
		{
			constexpr bool bNotify = true;
			if (FTLLRN_RigBaseMetadata* Metadata = GetMetadataForElement(InElement, InMetadataName, InType, bNotify))
			{
				return Metadata->SetValueData(&InValue, sizeof(T));
			}
		}
		return false;
	}

	template<typename T>
	bool SetArrayMetadata(const FTLLRN_RigElementKey& InItem, ETLLRN_RigMetadataType InType, const FTLLRN_RigName& InMetadataName, const TArray<T>& InValue)
	{
		return SetMetadata<TArray<T>>(Find(InItem), InType, InMetadataName, InValue);
	}

	template<typename T>
	bool SetArrayMetadata(FTLLRN_RigBaseElement* InElement, ETLLRN_RigMetadataType InType, const FTLLRN_RigName& InMetadataName, const TArray<T>& InValue)
	{
		return SetMetadata<TArray<T>>(InElement, InType, InMetadataName, InValue);
	}

public:
	
	void PropagateMetadata(const FTLLRN_RigElementKey& InKey, const FName& InName, bool bNotify = true);
	void PropagateMetadata(const FTLLRN_RigBaseElement* InElement, const FName& InName, bool bNotify = true);
	
private:
	
	void OnMetadataChanged(const FTLLRN_RigElementKey& InKey, const FName& InName);
	void OnMetadataTagChanged(const FTLLRN_RigElementKey& InKey, const FName& InTag, bool bAdded);

	/** Returns a metadata ptr to the given element's metadata. If the meta data, with the same name, doesn't exist already a new entry
	    is created for that element. If the name matches but the type differs, the existing metadata is destroyed and a new one with the
	    matching type is created instead.
	    */
	FTLLRN_RigBaseMetadata* GetMetadataForElement(FTLLRN_RigBaseElement* InElement, const FName& InName, ETLLRN_RigMetadataType InType, bool bInNotify);
	
	/** Attempts to find element's metadata of the given name and type. If either the element doesnt exist, the name doesn't exist or the
	    type doesn't match, then \c nullptr is returned.
	    */ 
	FTLLRN_RigBaseMetadata* FindMetadataForElement(const FTLLRN_RigBaseElement* InElement, const FName& InName, ETLLRN_RigMetadataType InType);
	const FTLLRN_RigBaseMetadata* FindMetadataForElement(const FTLLRN_RigBaseElement* InElement, const FName& InName, ETLLRN_RigMetadataType InType) const;
	
	/** Removes the named meta data for the given element, regardless of type. If the element doesn't exist, or it doesn't have any
	    metadata of the given name, this function does nothing and returns \c false.
		*/
	bool RemoveMetadataForElement(FTLLRN_RigBaseElement* InElement, const FName& InName);
	bool RemoveAllMetadataForElement(FTLLRN_RigBaseElement* InElement);
	
	void CopyAllMetadataFromElement(FTLLRN_RigBaseElement* InTargetElement, const FTLLRN_RigBaseElement* InSourceElement);
	
protected:
	bool bEnableCacheValidityCheck;

	static bool bEnableValidityCheckbyDefault;

	UPROPERTY(transient)
	TObjectPtr<UTLLRN_RigHierarchy> HierarchyForCacheValidation;

	mutable TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> DefaultParentPerElement;
	mutable uint32 DefaultParentCacheTopologyVersion;

	bool bUsePreferredEulerAngles;
	mutable bool bAllowNameSpaceWhenSanitizingName;

public:
	bool UsesPreferredEulerAngles() const { return bUsePreferredEulerAngles; }

private:
	
	void EnsureCacheValidityImpl();

	// NOTE: it is not safe to read or write the execute context without locking it first as the
	// FTLLRN_RigHierarchyExecuteContextBracket can change the context from the main thread (from sequencer for example)
	// at the same time that a control rig is being evaluated on an animation thread.
	mutable FCriticalSection ExecuteContextLock;
	const FRigVMExtendedExecuteContext* ExecuteContext;

#if WITH_EDITOR
	mutable bool bRecordTransformsAtRuntime;
	mutable TArray<TInstructionSliceElement> ReadTransformsAtRuntime;
	mutable TArray<TInstructionSliceElement> WrittenTransformsAtRuntime;
public:

	TElementDependencyMap GetDependenciesForVM(const URigVM* InVM, FName InEventName = NAME_None) const;

private:

#endif

	mutable TArray<int32> PoseVersionPerElement;
	FORCEINLINE int32& GetPoseVersion(int32 InIndex) const
	{
		if(!PoseVersionPerElement.IsValidIndex(InIndex))
		{
			PoseVersionPerElement.SetNumZeroed(InIndex + 1);
		}
		return PoseVersionPerElement[InIndex];
	}
	FORCEINLINE void IncrementPoseVersion(int32 InIndex) const
	{
		// don't do anything if the pose version array is empty
		// or the element has not been requested yet.
		if(PoseVersionPerElement.IsValidIndex(InIndex))
		{
			PoseVersionPerElement[InIndex]++;
		}
	}

	FTLLRN_RigElementKeyRedirector* ElementKeyRedirector;

	void UpdateVisibilityOnProxyControls();

	static const TArray<FString>& GetTransformTypeStrings();

	struct FQueuedNotification
	{
		ETLLRN_RigHierarchyNotification Type;
		FTLLRN_RigElementKey Key;
		
		bool operator == (const FQueuedNotification& InOther) const
		{
			return Type == InOther.Type && Key == InOther.Key;
		}
	};
	TQueue<FQueuedNotification> QueuedNotifications;

	void QueueNotification(ETLLRN_RigHierarchyNotification InNotification, const FTLLRN_RigBaseElement* InElement);
	void SendQueuedNotifications();
	void Reset_Impl(bool bResetElements);
#if WITH_EDITOR
	void ForEachListeningHierarchy(TFunctionRef<void(const FTLLRN_RigHierarchyListener&)> PerListeningHierarchyFunction);
#endif

public:
	
	template<typename RangeType>
	static void ConvertElementsToKeys(RangeType InElements, TArray<FTLLRN_RigElementKey>& OutKeys)
	{
		OutKeys.Reserve(OutKeys.Num() + InElements.Num());
		for (const typename RangeType::ElementType Element: InElements)
		{
			OutKeys.Add(Element->Key);
		}
	}

	template<typename RangeType>
	static void ConvertElementsToIndices(RangeType InElements, TArray<int32>& OutIndices)
	{
		OutIndices.Reserve(OutIndices.Num() + InElements.Num());
		for (const typename RangeType::ElementType Element: InElements)
		{
			OutIndices.Add(Element->Index);
		}
	}

	template<typename ElementType = FTLLRN_RigBaseElement, typename RangeType = TArray<FTLLRN_RigBaseElement*>>
	static void ConvertElements(RangeType InElements, TArray<ElementType*>& OutElements, bool bFilterNull = true)
	{
		OutElements.Reserve(OutElements.Num() + InElements.Num());
		for (const typename RangeType::ElementType Element: InElements)
		{
			ElementType* CastElement = Cast<ElementType>(Element);
			if(CastElement || bFilterNull)
			{
				OutElements.Add(CastElement);
			}
		}
	}

	template<typename RangeType>
	static TArray<FTLLRN_RigElementKey> ConvertElementsToKeys(RangeType InElements)
	{
		TArray<FTLLRN_RigElementKey> ElementKeys;
		ConvertElementsToKeys(InElements, ElementKeys);
		return ElementKeys;
	}

	template<typename RangeType>
	static TArray<int32> ConvertElementsToIndices(RangeType InElements)
	{
		TArray<int32> ElementIndices;
		ConvertElementsToIndices(InElements, ElementIndices);
		return ElementIndices;
	}

	template<typename ElementType = FTLLRN_RigBaseElement, typename RangeType = TArray<FTLLRN_RigBaseElement*>>
	static TArray<ElementType*> ConvertElements(RangeType InElements, bool bFilterNull = true)
	{
		TArray<ElementType*> OutElements;
		ConvertElements<ElementType, RangeType>(InElements, OutElements, bFilterNull);
		return OutElements;
	}

private:
	
	// the currently destroyed element - used to avoid notification storms
	const FTLLRN_RigBaseElement* ElementBeingDestroyed; 
	
	friend class UTLLRN_RigHierarchyController;
	friend class UTLLRN_ControlRig;
	friend class UTLLRN_ModularRig;
	friend class FTLLRN_ControlRigEditor;
	friend struct FTLLRN_RigBaseElement;
	friend struct FTLLRN_RigHierarchyValidityBracket;
	friend struct FTLLRN_RigHierarchyGlobalValidityBracket;
	friend struct FTLLRN_ControlRigVisualGraphUtils;
	friend struct FTLLRN_RigHierarchyEnableControllerBracket;
	friend struct FTLLRN_RigHierarchyExecuteContextBracket;
	friend struct FTLLRN_RigHierarchyRedirectorGuard;
	friend struct FTLLRN_RigDispatch_GetMetadata;
	friend struct FTLLRN_RigDispatch_SetMetadata;
	friend struct FTLLRN_RigDispatch_GetModuleMetadata;
	friend struct FTLLRN_RigDispatch_SetModuleMetadata;
	friend class FTLLRN_ControlTLLRN_RigHierarchySortElementStorage;
	friend class FTLLRN_ControlTLLRN_RigHierarchyShrinkElementStorage;
	friend class FTLLRN_ControlTLLRN_RigHierarchyRelinkElementStorage;
	friend class FTLLRN_RigHierarchyPoseAdapter;
};

struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyInteractionBracket
{
public:
	
	FTLLRN_RigHierarchyInteractionBracket(UTLLRN_RigHierarchy* InHierarchy)
		: Hierarchy(InHierarchy)
	{
		check(Hierarchy);
		Hierarchy->Notify(ETLLRN_RigHierarchyNotification::InteractionBracketOpened, nullptr);
	}

	~FTLLRN_RigHierarchyInteractionBracket()
	{
		Hierarchy->Notify(ETLLRN_RigHierarchyNotification::InteractionBracketClosed, nullptr);
	}

private:

	UTLLRN_RigHierarchy* Hierarchy;
};

struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyEnableControllerBracket
{
private:
	FTLLRN_RigHierarchyEnableControllerBracket(UTLLRN_RigHierarchy* InHierarchy, bool bEnable)
		: GuardIsControllerAvailable(InHierarchy->bIsControllerAvailable, bEnable)
	{
	}

	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_ControlRig;
	friend class UTLLRN_ControlRig;

	// certain units are allowed to use this
	friend struct FTLLRN_RigUnit_AddParent;
	friend struct FTLLRN_RigUnit_AddParents;
	friend struct FTLLRN_RigUnit_SetDefaultParent;
	friend struct FTLLRN_RigUnit_SetChannelHosts;

private:
	TGuardValue<bool> GuardIsControllerAvailable;
};

struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyExecuteContextBracket
{
private:

	FTLLRN_RigHierarchyExecuteContextBracket(UTLLRN_RigHierarchy* InHierarchy, const FRigVMExtendedExecuteContext* InContext)
		: Hierarchy(InHierarchy)
		, PreviousContext(InHierarchy->ExecuteContext)
	{
		Hierarchy->ExecuteContextLock.Lock();
		Hierarchy->ExecuteContext = InContext;
	}

	~FTLLRN_RigHierarchyExecuteContextBracket()
	{
		Hierarchy->ExecuteContext = PreviousContext;
		Hierarchy->SendQueuedNotifications();
		Hierarchy->ExecuteContextLock.Unlock();
	}

	UTLLRN_RigHierarchy* Hierarchy;
	const FRigVMExtendedExecuteContext* PreviousContext;

	friend class UTLLRN_ControlRig;
	friend class UTLLRN_ModularRig;
	friend class UTLLRN_ControlRigBlueprint;
};

struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyValidityBracket
{
	public:
	FTLLRN_RigHierarchyValidityBracket(UTLLRN_RigHierarchy* InHierarchy)
		: bPreviousValue(false)
		, HierarchyPtr() 
	{
		if(InHierarchy)
		{
			bPreviousValue = InHierarchy->bEnableCacheValidityCheck;
			InHierarchy->bEnableCacheValidityCheck = false;
			HierarchyPtr = InHierarchy;
		}
	}

	~FTLLRN_RigHierarchyValidityBracket()
	{
		if(HierarchyPtr.IsValid())
		{
			UTLLRN_RigHierarchy* Hierarchy = HierarchyPtr.Get();
			Hierarchy->bEnableCacheValidityCheck = bPreviousValue;
			Hierarchy->EnsureCacheValidity();
		}
	}

	private:

	bool bPreviousValue;
	TWeakObjectPtr<UTLLRN_RigHierarchy> HierarchyPtr;
};

struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyGlobalValidityBracket
{
public:
	FTLLRN_RigHierarchyGlobalValidityBracket(bool bEnable = true)
		: bPreviousValue(UTLLRN_RigHierarchy::bEnableValidityCheckbyDefault)
	{
		UTLLRN_RigHierarchy::bEnableValidityCheckbyDefault = true;
	}

	~FTLLRN_RigHierarchyGlobalValidityBracket()
	{
		UTLLRN_RigHierarchy::bEnableValidityCheckbyDefault = bPreviousValue;
	}

private:

	bool bPreviousValue;
};

struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyRedirectorGuard
{
public:
	FTLLRN_RigHierarchyRedirectorGuard(UTLLRN_RigHierarchy* InHierarchy, FTLLRN_RigElementKeyRedirector& InRedirector)
		: Guard(InHierarchy->ElementKeyRedirector, &InRedirector)
	{
	}

	FTLLRN_RigHierarchyRedirectorGuard(UTLLRN_ControlRig* InTLLRN_ControlRig);

private:
	TGuardValue<FTLLRN_RigElementKeyRedirector*> Guard;
};

template<>
inline FVector2D UTLLRN_RigHierarchy::GetControlValue(FTLLRN_RigControlElement* InControlElement, ETLLRN_RigControlValueType InValueType) const
{
	const FVector3f Value = GetControlValue(InControlElement, InValueType).Get<FVector3f>();
	return FVector2D(Value.X, Value.Y);
}

template<>
inline void UTLLRN_RigHierarchy::SetControlValue(int32 InElementIndex, const FVector2D& InValue, ETLLRN_RigControlValueType InValueType, bool bSetupUndo)
{
	return SetControlValue(InElementIndex, FTLLRN_RigControlValue::Make<FVector3f>(FVector3f(InValue.X, InValue.Y, 0.f)), InValueType, bSetupUndo);
}

#if WITH_EDITOR

class FTLLRN_RigHierarchyListenerGuard
{
public:
	FTLLRN_RigHierarchyListenerGuard(
		UTLLRN_RigHierarchy* InHierarchy, 
		bool bInEnableInitialChanges, 
		bool bInEnableCurrentChanges,
		UTLLRN_RigHierarchy* InListeningHierarchy = nullptr)
			: Hierarchy(InHierarchy)
			, bEnableInitialChanges(bInEnableInitialChanges)
			, bEnableCurrentChanges(bInEnableCurrentChanges)
			, ListeningHierarchy(InListeningHierarchy)
	{
		check(Hierarchy);

		if(ListeningHierarchy == nullptr)
		{
			InitialFlags.AddUninitialized(Hierarchy->ListeningHierarchies.Num());
			CurrentFlags.AddUninitialized(Hierarchy->ListeningHierarchies.Num());

			for(int32 Index = 0; Index < Hierarchy->ListeningHierarchies.Num(); Index++)
			{
				UTLLRN_RigHierarchy::FTLLRN_RigHierarchyListener& Listener = Hierarchy->ListeningHierarchies[Index];
				InitialFlags[Index] = Listener.bShouldReactToInitialChanges;
				CurrentFlags[Index] = Listener.bShouldReactToCurrentChanges;

				Listener.bShouldReactToInitialChanges = bInEnableInitialChanges; 
				Listener.bShouldReactToCurrentChanges = bInEnableCurrentChanges; 
			}
		}
		else
		{
			for(int32 Index = 0; Index < Hierarchy->ListeningHierarchies.Num(); Index++)
			{
				UTLLRN_RigHierarchy::FTLLRN_RigHierarchyListener& Listener = Hierarchy->ListeningHierarchies[Index];

				if(Listener.Hierarchy.Get() == ListeningHierarchy)
				{
					InitialFlags.Add(Listener.bShouldReactToInitialChanges);
					CurrentFlags.Add(Listener.bShouldReactToCurrentChanges);

					Listener.bShouldReactToInitialChanges = bInEnableInitialChanges; 
					Listener.bShouldReactToCurrentChanges = bInEnableCurrentChanges;
					break;
				}
			}
		}
	}

	~FTLLRN_RigHierarchyListenerGuard()
	{
		if(ListeningHierarchy == nullptr)
		{
			check(Hierarchy->ListeningHierarchies.Num() == InitialFlags.Num());
			check(Hierarchy->ListeningHierarchies.Num() == CurrentFlags.Num());
			
			for(int32 Index = 0; Index < Hierarchy->ListeningHierarchies.Num(); Index++)
			{
				UTLLRN_RigHierarchy::FTLLRN_RigHierarchyListener& Listener = Hierarchy->ListeningHierarchies[Index];
				Listener.bShouldReactToInitialChanges = InitialFlags[Index];
				Listener.bShouldReactToCurrentChanges = CurrentFlags[Index];
			}
		}
		else
		{
			for(int32 Index = 0; Index < Hierarchy->ListeningHierarchies.Num(); Index++)
			{
				UTLLRN_RigHierarchy::FTLLRN_RigHierarchyListener& Listener = Hierarchy->ListeningHierarchies[Index];

				if(Listener.Hierarchy.Get() == ListeningHierarchy)
				{
					check(InitialFlags.Num() == 1);
					check(CurrentFlags.Num() == 1);

					Listener.bShouldReactToInitialChanges = InitialFlags[0];
					Listener.bShouldReactToCurrentChanges = CurrentFlags[0];
					break;
				}
			}
		}
	}

private:

	UTLLRN_RigHierarchy* Hierarchy; 
	bool bEnableInitialChanges; 
	bool bEnableCurrentChanges;
	UTLLRN_RigHierarchy* ListeningHierarchy;
	TArray<bool> InitialFlags;
	TArray<bool> CurrentFlags;
};

#endif

UINTERFACE()
class TLLRN_CONTROLRIG_API UTLLRN_RigHierarchyProvider : public UInterface
{
	GENERATED_BODY()
};

class TLLRN_CONTROLRIG_API ITLLRN_RigHierarchyProvider
{
	GENERATED_BODY()

public:

	virtual UTLLRN_RigHierarchy* GetHierarchy() const = 0;
};

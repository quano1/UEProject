// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/LazyObjectPtr.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigSequenceObjectReference.generated.h"


/**
 * An external reference to an level sequence object, resolvable through an arbitrary context.
 */
USTRUCT()
struct FTLLRN_ControlRigSequenceObjectReference
{
	GENERATED_BODY()

	/**
	 * Default construction to a null reference
	 */
	FTLLRN_ControlRigSequenceObjectReference() {}

	/**
	 * Generates a new reference to an object within a given context.
	 *
	 * @param InTLLRN_ControlRig The TLLRN_ControlRig to create a reference for
	 */
	TLLRN_CONTROLRIG_API static FTLLRN_ControlRigSequenceObjectReference Create(UTLLRN_ControlRig* InTLLRN_ControlRig);

	/**
	 * Check whether this object reference is valid or not
	 */
	bool IsValid() const
	{
		return TLLRN_ControlRigClass.Get() != nullptr;
	}

	/**
	 * Equality comparator
	 */
	friend bool operator==(const FTLLRN_ControlRigSequenceObjectReference& A, const FTLLRN_ControlRigSequenceObjectReference& B)
	{
		return A.TLLRN_ControlRigClass == B.TLLRN_ControlRigClass;
	}

private:

	/** The type of this animation TLLRN_ControlRig */
	UPROPERTY()
	TSubclassOf<UTLLRN_ControlRig> TLLRN_ControlRigClass;
};

USTRUCT()
struct FTLLRN_ControlRigSequenceObjectReferences
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTLLRN_ControlRigSequenceObjectReference> Array;
};

USTRUCT()
struct FTLLRN_ControlRigSequenceObjectReferenceMap
{
	GENERATED_BODY()

	/**
	 * Check whether this map has a binding for the specified object id
	 * @return true if this map contains a binding for the id, false otherwise
	 */
	bool HasBinding(const FGuid& ObjectId) const;

	/**
	 * Remove a binding for the specified ID
	 *
	 * @param ObjectId	The ID to remove
	 */
	void RemoveBinding(const FGuid& ObjectId);

	/**
	 * Create a binding for the specified ID
	 *
	 * @param ObjectId				The ID to associate the component with
	 * @param ObjectReference	The component reference to bind
	 */
	void CreateBinding(const FGuid& ObjectId, const FTLLRN_ControlRigSequenceObjectReference& ObjectReference);

private:
	
	UPROPERTY()
	TArray<FGuid> BindingIds;

	UPROPERTY()
	TArray<FTLLRN_ControlRigSequenceObjectReferences> References;
};

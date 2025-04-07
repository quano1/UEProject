// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
/** Interface to allow control rigs to bind to external objects */
class ITLLRN_ControlRigObjectBinding
{
public:
	/** Bind to a runtime object */
	virtual void BindToObject(UObject* InObject) = 0;

	/** Unbind from the current bound runtime object */
	virtual void UnbindFromObject() = 0;

	/** Bindable event for external objects to be notified that a Control has been bound */
	DECLARE_EVENT_OneParam(ITLLRN_ControlRigObjectBinding, FTLLRN_ControlRigBind, UObject*);
	virtual FTLLRN_ControlRigBind& OnTLLRN_ControlRigBind() = 0;

	/** Bindable event for external objects to be notified that a Control has been unbound */
	DECLARE_EVENT(ITLLRN_ControlRigObjectBinding, FTLLRN_ControlRigUnbind);
	virtual FTLLRN_ControlRigUnbind& OnTLLRN_ControlRigUnbind() = 0;

	/** 
	 * Check whether we are bound to the supplied object. 
	 * This can be distinct from a direct pointer comparison (e.g. in the case of an actor passed
	 * to BindToObject, we may actually bind to one of its components).
	 */
	virtual bool IsBoundToObject(UObject* InObject) const = 0;

	/** Get the current object we are bound to */
	virtual UObject* GetBoundObject() const = 0;

	/** Find the actor we are bound to, if any */
	virtual AActor* GetHostingActor() const = 0;
};
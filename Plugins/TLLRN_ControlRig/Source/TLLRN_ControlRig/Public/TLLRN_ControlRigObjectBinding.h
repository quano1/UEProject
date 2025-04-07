// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "ITLLRN_ControlRigObjectBinding.h"

class USceneComponent;

class TLLRN_CONTROLRIG_API FTLLRN_ControlRigObjectBinding : public ITLLRN_ControlRigObjectBinding
{
public:
	
	virtual ~FTLLRN_ControlRigObjectBinding();

	// ITLLRN_ControlRigObjectBinding interface
	virtual void BindToObject(UObject* InObject) override;
	virtual void UnbindFromObject() override;
	virtual FTLLRN_ControlRigBind& OnTLLRN_ControlRigBind() override { return TLLRN_ControlRigBind; }
	virtual FTLLRN_ControlRigUnbind& OnTLLRN_ControlRigUnbind() override { return TLLRN_ControlRigUnbind; }
	virtual bool IsBoundToObject(UObject* InObject) const override;
	virtual UObject* GetBoundObject() const override;
	virtual AActor* GetHostingActor() const override;

	static UObject* GetBindableObject(UObject* InObject);
private:
	/** The scene component or USkeleton we are bound to */
	TWeakObjectPtr<UObject> BoundObject;

	FTLLRN_ControlRigBind TLLRN_ControlRigBind;
	FTLLRN_ControlRigUnbind TLLRN_ControlRigUnbind;
};
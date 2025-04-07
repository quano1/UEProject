// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "ScopedTransaction.h"
#include "RigEditor/TLLRN_IKRigEditorController.h"

class FIKRigGenericDetailCustomization : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FIKRigGenericDetailCustomization);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:

	/** Labels and transform types used by the transform widget */
	static const TArray<FText>& GetButtonLabels();
	static const TArray<ETLLRN_IKRigTransformType::Type>& GetTransformTypes();

	/** Per class customization */
	template<typename ClassToCustomize>
	void CustomizeDetailsForClass(IDetailLayoutBuilder& DetailBuilder, const TArray<TWeakObjectPtr<UObject>>& ObjectsBeingCustomized) {}

	/** Template specializations */
	template<>
	void CustomizeDetailsForClass<UTLLRN_IKRigBoneDetails>(IDetailLayoutBuilder& DetailBuilder, const TArray<TWeakObjectPtr<UObject>>& ObjectsBeingCustomized);
	template<>
	void CustomizeDetailsForClass<UTLLRN_IKRigEffectorGoal>(IDetailLayoutBuilder& DetailBuilder, const TArray<TWeakObjectPtr<UObject>>& ObjectsBeingCustomized);

	/** Transaction to handle value changed. It's being effective once the value committed */
	TSharedPtr<FScopedTransaction> ValueChangedTransaction;
};
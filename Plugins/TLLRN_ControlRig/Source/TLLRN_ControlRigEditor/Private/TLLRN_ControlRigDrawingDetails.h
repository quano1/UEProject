// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ControlRigDrawingDetails.generated.h"

class IDetailLayoutBuilder;

USTRUCT()
struct FTLLRN_ControlRigDrawContainerImportFbxSettings
{
	GENERATED_BODY()

	FTLLRN_ControlRigDrawContainerImportFbxSettings()
		: Scale(1.f)
		, Detail(1)
		, bMergeCurves(false)
	{}

	UPROPERTY(EditAnywhere, Category = "Fbx Import")
	float Scale;

	UPROPERTY(EditAnywhere, Category = "Fbx Import", meta = (UIMin = "1", UIMax = "8"))
	int32 Detail;

	UPROPERTY(EditAnywhere, Category = "Fbx Import")
	bool bMergeCurves;
};

class FTLLRN_ControlRigDrawContainerDetails : public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_ControlRigDrawContainerDetails);
	}

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:

	FReply OnImportCurvesFromFBXClicked();
	static void ImportCurvesFromFBX(const FString& InFilePath, UTLLRN_ControlRigBlueprint* InBlueprint, const FTLLRN_ControlRigDrawContainerImportFbxSettings& InSettings);

	UTLLRN_ControlRigBlueprint* BlueprintBeingCustomized;
};

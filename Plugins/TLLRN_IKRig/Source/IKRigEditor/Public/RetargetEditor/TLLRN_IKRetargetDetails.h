// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "RetargetEditor/TLLRN_IKRetargeterController.h"
#include "Retargeter/TLLRN_IKRetargetProcessor.h"

#include "TLLRN_IKRetargetDetails.generated.h"

class UDebugSkelMeshComponent;

enum class EIKRetargetTransformType : int8
{
	Current,
	Reference,
	RelativeOffset,
	Bone,
};

UCLASS(config = Engine, hidecategories = UObject)
class TLLRN_IKRIGEDITOR_API UTLLRN_IKRetargetBoneDetails : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, Category = "Selection")
	FName SelectedBone;

	UPROPERTY()
	FTransform LocalTransform;
	
	UPROPERTY()
	FTransform OffsetTransform;
	
	UPROPERTY()
	FTransform CurrentTransform;

	UPROPERTY()
	FTransform ReferenceTransform;
	
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;

#if WITH_EDITOR

	FEulerTransform GetTransform(EIKRetargetTransformType TransformType, const bool bLocalSpace) const;
	bool IsComponentRelative(ESlateTransformComponent::Type Component, EIKRetargetTransformType TransformType) const;
	void OnComponentRelativeChanged(ESlateTransformComponent::Type Component, bool bIsRelative, EIKRetargetTransformType TransformType);
	void OnCopyToClipboard(ESlateTransformComponent::Type Component, EIKRetargetTransformType TransformType) const;
	void OnPasteFromClipboard(ESlateTransformComponent::Type Component, EIKRetargetTransformType TransformType);
	void OnNumericValueCommitted(
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent,
		FVector::FReal Value,
		ETextCommit::Type CommitType,
		EIKRetargetTransformType TransformType,
		bool bIsCommit);

	TOptional<FVector::FReal> GetNumericValue(
		EIKRetargetTransformType TransformType,
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent);
	
	/** Method to react to changes of numeric values in the widget */
	static void OnMultiNumericValueCommitted(
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent,
		FVector::FReal Value,
		ETextCommit::Type CommitType,
		EIKRetargetTransformType TransformType,
		TArrayView<TObjectPtr<UTLLRN_IKRetargetBoneDetails>> Bones,
		bool bIsCommit);

	template<typename DataType>
	void GetContentFromData(const DataType& InData, FString& Content) const;

#endif

private:

	void CommitValueAsRelativeOffset(
		UTLLRN_IKRetargeterController* AssetController,
		const FReferenceSkeleton& RefSkeleton,
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
		const int32 BoneIndex,
		UDebugSkelMeshComponent* Mesh,
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent,
		FVector::FReal Value,
		bool bShouldTransact);

	void CommitValueAsBoneSpace(
		UTLLRN_IKRetargeterController* AssetController,
		const FReferenceSkeleton& RefSkeleton,
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
		const int32 BoneIndex,
		UDebugSkelMeshComponent* Mesh,
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent,
		FVector::FReal Value,
		bool bShouldTransact);

	static TOptional<FVector::FReal> CleanRealValue(TOptional<FVector::FReal> InValue);
	
	bool IsRootBone() const;

	bool BoneRelative[3] = { false, true, false };
	bool RelativeOffsetTransformRelative[3] = { false, true, false };
	bool CurrentTransformRelative[3];
	bool ReferenceTransformRelative[3];
};

struct FTLLRN_IKRetargetTransformWidgetData
{
	FTLLRN_IKRetargetTransformWidgetData(EIKRetargetTransformType InType, FText InLabel, FText InTooltip)
	{
		TransformType = InType;
		ButtonLabel = InLabel;
		ButtonTooltip = InTooltip;
	};
	
	EIKRetargetTransformType TransformType;
	FText ButtonLabel;
	FText ButtonTooltip;
};

struct FTLLRN_IKRetargetTransformUIData
{
	TArray<EIKRetargetTransformType> TransformTypes;
	TArray<FText> ButtonLabels;
	TArray<FText> ButtonTooltips;
	TAttribute<TArray<EIKRetargetTransformType>> VisibleTransforms;
	TArray<TSharedRef<IPropertyHandle>> Properties;
};

class FIKRetargetBoneDetailCustomization : public IDetailCustomization, FGCObject
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FIKRetargetBoneDetailCustomization);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	// End FGCObject interface

private:

	void GetTransformUIData(
		const bool bIsEditingPose,
		const IDetailLayoutBuilder& DetailBuilder,
		FTLLRN_IKRetargetTransformUIData& OutData) const;

	TArray<TObjectPtr<UTLLRN_IKRetargetBoneDetails>> Bones;
};

/** ------------------------------------- BEGIN CHAIN DETAILS CUSTOMIZATION -------------*/


class FRetargetChainSettingsCustomization : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FRetargetChainSettingsCustomization);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:

	void AddSettingsSection(
		const IDetailLayoutBuilder& DetailBuilder,
		const FTLLRN_IKRetargetEditorController* Controller,
		IDetailCategoryBuilder& SettingsCategory,
		const FString& StructPropertyName,
		const FName& GroupName,
		const FText& LocalizedGroupName,
		const UScriptStruct* SettingsClass,
		const FString& EnabledPropertyName,
		const bool& bIsSectionEnabled,
		const FText& DisabledMessage) const;
	
	TArray<TWeakObjectPtr<UTLLRN_RetargetChainSettings>> ChainSettingsObjects;
	TArray<TSharedPtr<FString>> SourceChainOptions;
};

/** ------------------------------------- BEGIN ROOT DETAILS CUSTOMIZATION -------------*/

class FRetargetRootSettingsCustomization : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FRetargetRootSettingsCustomization);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:

	TWeakObjectPtr<UTLLRN_RetargetRootSettings> RootSettingsObject;
	TWeakPtr<FTLLRN_IKRetargetEditorController> Controller;
};

/** ------------------------------------- BEGIN GLOBAL DETAILS CUSTOMIZATION -------------*/

class FTLLRN_RetargetGlobalSettingsCustomization : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RetargetGlobalSettingsCustomization);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:

	TWeakObjectPtr<UTLLRN_IKRetargetGlobalSettings> GlobalSettingsObject;
	TWeakPtr<FTLLRN_IKRetargetEditorController> Controller;

	TArray<TSharedPtr<FString>> TargetChainOptions;
};

/** ------------------------------------- BEGIN POST DETAILS CUSTOMIZATION -------------*/

class FRetargetOpStackCustomization : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FRetargetOpStackCustomization);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:

	TSharedRef<SWidget> CreateAddNewMenuWidget();

	void AddNewRetargetOp(UClass* Class);
	
	TWeakObjectPtr<UTLLRN_RetargetOpStack> RetargetOpStackObject;
	TWeakPtr<FTLLRN_IKRetargetEditorController> Controller;
};

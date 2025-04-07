// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "InteractiveTool.h"
#include "InteractiveToolBuilder.h"
#include "TLLRN_UVEditorToolAnalyticsUtils.h"

#include "TLLRN_UVEditorLayerEditTool.generated.h"

class UTLLRN_UVEditorToolMeshInput;
class UTLLRN_UVEditorChannelEditTool;
class UTLLRN_UVToolEmitChangeAPI;

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorChannelEditToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

	// This is a pointer so that it can be updated under the builder without
	// having to set it in the mode after initializing targets.
	const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>* Targets = nullptr;
};

UENUM()
enum class ETLLRN_ChannelEditToolAction
{
	NoAction,

	Add,
	Copy,
	Delete
};


UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorChannelEditSettings : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** UV Layer Edit action to preform */
	UPROPERTY(EditAnywhere, Category = Options, meta = (InvalidEnumValues = "NoAction"))
	ETLLRN_ChannelEditToolAction Action = ETLLRN_ChannelEditToolAction::Add;
};

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorChannelEditTargetProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category = "UVChannels", meta = (DisplayName = "Asset", GetOptions = GetAssetNames, EditCondition = "bActionNeedsAsset", EditConditionHides = true, HideEditConditionToggle = true))
	FString Asset;

	UFUNCTION()
	const TArray<FString>& GetAssetNames();

	UPROPERTY(EditAnywhere, Category = "UVChannels", meta = (DisplayName = "Target UV Channel", GetOptions = GetUVChannelNames, EditCondition = "bActionNeedsTarget", EditConditionHides = true, HideEditConditionToggle = true))
	FString TargetChannel;

	UPROPERTY(EditAnywhere, Category = "UVChannels", meta = (DisplayName = "Source UV Channel", GetOptions = GetUVChannelNames, EditCondition = "bActionNeedsReference", EditConditionHides = true, HideEditConditionToggle = true))
	FString ReferenceChannel;

	UFUNCTION()
	const TArray<FString>& GetUVChannelNames();

	TArray<FString> UVChannelNames;
	TArray<FString> UVAssetNames;

	// 1:1 with UVAssetNames
	TArray<int32> NumUVChannelsPerAsset;

	UPROPERTY(meta = (TransientToolProperty))
	bool bActionNeedsAsset = true;

	UPROPERTY(meta = (TransientToolProperty))
	bool bActionNeedsReference = false;

	UPROPERTY(meta = (TransientToolProperty))
	bool bActionNeedsTarget = true;

public:
	void Initialize(
		const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn,
		bool bInitializeSelection);

	void SetUsageFlags(ETLLRN_ChannelEditToolAction Action);

	/**
	 * Verify that the UV asset selection is valid
	 * @param bUpdateIfInvalid if selection is not valid, reset UVAsset to Asset0 or empty if no assets exist
	 * @return true if selection in UVAsset is an entry in UVAssetNames.
	 */
	bool ValidateUVAssetSelection(bool bUpdateIfInvalid);

	/**
	 * Verify that the UV channel selection is valid
	 * @param bUpdateIfInvalid if selection is not valid, UVChannel to UV0 or empty if no UV channels exist
	 * @return true if selection in UVChannel is an entry in UVChannelNamesList.
	 */
	bool ValidateUVChannelSelection(bool bUpdateIfInvalid);


	/**
	 * @return selected UV asset ID, or -1 if invalid selection
	 */
	int32 GetSelectedAssetID();

	/**
	 * @param bForceToZeroOnFailure if true, then instead of returning -1 we return 0 so calling code can fallback to default UV paths
	 * @param bUseReference if true, get the selected reference channel index, otherwise return the target channel's index.
	 * @return selected UV channel index, or -1 if invalid selection, or 0 if invalid selection and bool bForceToZeroOnFailure = true
	 */
	int32 GetSelectedChannelIndex(bool bForceToZeroOnFailure = false, bool bUseReference = false);
};

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorChannelEditAddProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/* Placeholder for future per action settings */
};

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorChannelEditCopyProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/* Placeholder for future per action settings */
};

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorChannelEditDeleteProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/* Placeholder for future per action settings */
};

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorTLLRN_ChannelEditToolActionPropertySet : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UTLLRN_UVEditorChannelEditTool> ParentTool;

	void Initialize(UTLLRN_UVEditorChannelEditTool* ParentToolIn) { ParentTool = ParentToolIn; }
	void PostAction(ETLLRN_ChannelEditToolAction Action);
	
	UFUNCTION(CallInEditor, Category = Actions)
	void Apply();
};

/**
 *
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorChannelEditTool : public UInteractiveTool
{
	GENERATED_BODY()

public:

	/**
	 * The tool will operate on the meshes given here.
	 */
	virtual void SetTargets(const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn)
	{
		Targets = TargetsIn;
	}

	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;

	virtual bool HasCancel() const override { return false; }
	virtual bool HasAccept() const override { return false; }
	virtual bool CanAccept() const override { return false; }

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	void RequestAction(ETLLRN_ChannelEditToolAction ActionType);
	ETLLRN_ChannelEditToolAction ActiveAction() const;


protected:
	void ApplyVisbleChannelChange();
	void UpdateChannelSelectionProperties(int32 ChangingAsset);

	void AddChannel();
	void CopyChannel();
	void DeleteChannel();
	int32 ActiveAsset;
	int32 ActiveChannel;
	int32 ReferenceChannel;

	ETLLRN_ChannelEditToolAction PendingAction = ETLLRN_ChannelEditToolAction::NoAction;

	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>> Targets;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorTLLRN_ChannelEditToolActionPropertySet> ToolActions = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorChannelEditSettings> ActionSelectionProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorChannelEditTargetProperties> SourceChannelProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorChannelEditAddProperties> AddActionProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorChannelEditCopyProperties> CopyActionProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorChannelEditDeleteProperties> DeleteActionProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolEmitChangeAPI> EmitChangeAPI = nullptr;

	//
	// Analytics
	//

	struct FActionHistoryItem
	{
		FDateTime Timestamp;
		ETLLRN_ChannelEditToolAction ActionType = ETLLRN_ChannelEditToolAction::NoAction;

		// if ActionType == Add    then FirstOperandIndex is the index of the added UV layer
		// if ActionType == Delete then FirstOperandIndex is the index of the deleted UV layer
		// if ActionType == Copy   then FirstOperandIndex is the index of the source UV layer
		int32 FirstOperandIndex = -1;
		
		// if ActionType == Add    then SecondOperandIndex is unused
		// if ActionType == Delete then SecondOperandIndex is unused
		// if ActionType == Copy   then SecondOperandIndex is the index of the target UV layer
		int32 SecondOperandIndex = -1;

		bool bDeleteActionWasActuallyClear = false;
	};
	
	TArray<FActionHistoryItem> AnalyticsActionHistory;
	UE::Geometry::TLLRN_UVEditorAnalytics::FTargetAnalytics InputTargetAnalytics;
	FDateTime ToolStartTimeAnalytics;
	void RecordAnalytics();
};

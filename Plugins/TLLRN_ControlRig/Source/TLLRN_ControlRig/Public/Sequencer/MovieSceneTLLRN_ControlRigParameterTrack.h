// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Sections/MovieSceneParameterSection.h"
#include "MovieSceneNameableTrack.h"
#include "TLLRN_ControlRig.h"
#include "Compilation/IMovieSceneTrackTemplateProducer.h"
#include "INodeAndChannelMappings.h"
#include "MovieSceneTLLRN_ControlRigParameterSection.h"
#include "EulerTransform.h"
#include "Tracks/IMovieSceneSectionsToKey.h"
#include "MovieSceneTLLRN_ControlRigParameterTrack.generated.h"

struct FEndLoadPackageContext;
struct FTLLRN_RigControlElement;

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_ControlRotationOrder
{
	GENERATED_BODY()

	FTLLRN_ControlRotationOrder();

	/* Rotation Order*/
	UPROPERTY()
	EEulerRotationOrder RotationOrder;

	/** Override the default control setting*/
	UPROPERTY()
	bool bOverrideSetting;
};

/**
 * Handles animation of skeletal mesh actors using animation TLLRN_ControlRigs
 */

UCLASS(MinimalAPI)
class UMovieSceneTLLRN_ControlRigParameterTrack
	: public UMovieSceneNameableTrack
	, public IMovieSceneTrackTemplateProducer
	, public INodeAndChannelMappings
	, public IMovieSceneSectionsToKey

{
	GENERATED_UCLASS_BODY()

public:
	// UMovieSceneTrack interface
	virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection& InSection) const override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
	virtual UMovieSceneSection* CreateNewSection() override;
	virtual void RemoveAllAnimationData() override;
	virtual bool HasSection(const UMovieSceneSection& Section) const override;
	virtual void AddSection(UMovieSceneSection& Section) override;
	virtual void RemoveSection(UMovieSceneSection& Section) override;
	virtual void RemoveSectionAt(int32 SectionIndex) override;
	virtual bool IsEmpty() const override;
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;
	virtual FName GetTrackName() const override { return TrackName; }
	// UObject
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;
#if WITH_EDITORONLY_DATA
	static void DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass);
#endif
	virtual void PostEditImport() override;
#if WITH_EDITORONLY_DATA
	virtual FText GetDefaultDisplayName() const override;
#endif

	//INodeAndMappingsInterface
	virtual TArray<FRigControlFBXNodeAndChannels>* GetNodeAndChannelMappings(UMovieSceneSection* InSection)  override;
	virtual void GetSelectedNodes(TArray<FName>& OutSelectedNodes) override;
#if WITH_EDITOR
	virtual bool GetFbxCurveDataFromChannelMetadata(const FMovieSceneChannelMetaData& MetaData, FControlRigFbxCurveData& OutCurveData) override;
#endif // WITH_EDITOR

	//UTLLRN_ControlRig Delegates
	void HandleOnPostConstructed_GameThread();
	void HandleOnPostConstructed(UTLLRN_ControlRig* Subject, const FName& InEventName);

#if WITH_EDITOR
	void HandlePackageDone(const FEndLoadPackageContext& Context);
	// control Rigs are ready only after its package is fully end-loaded
	void HandleTLLRN_ControlRigPackageDone(URigVMHost* InTLLRN_ControlRig);
#endif

public:
	/** Add a section at that start time*/
	TLLRN_CONTROLRIG_API UMovieSceneSection* CreateTLLRN_ControlRigSection(FFrameNumber StartTime, UTLLRN_ControlRig* InTLLRN_ControlRig, bool bInOwnsTLLRN_ControlRig);

	TLLRN_CONTROLRIG_API void ReplaceTLLRN_ControlRig(UTLLRN_ControlRig* NewTLLRN_ControlRig, bool RecreateChannels);

	TLLRN_CONTROLRIG_API void RenameParameterName(const FName& OldParameterName, const FName& NewParameterName);

public:
	TLLRN_CONTROLRIG_API UTLLRN_ControlRig* GetTLLRN_ControlRig() const { return TLLRN_ControlRig; }

	/**
	* Find all sections at the current time.
	*
	*@param Time  The Time relative to the owning movie scene where the section should be
	*@Return All sections at that time
	*/
	TLLRN_CONTROLRIG_API TArray<UMovieSceneSection*, TInlineAllocator<4>> FindAllSections(FFrameNumber Time);

	/**
	 * Finds a section at the current time.
	 *
	 * @param Time The time relative to the owning movie scene where the section should be
	 * @return The found section.
	 */
	TLLRN_CONTROLRIG_API class UMovieSceneSection* FindSection(FFrameNumber Time);

	/**
	 * Finds a section at the current time or extends an existing one
	 *
	 * @param Time The time relative to the owning movie scene where the section should be
	 * @param OutWeight The weight of the section if found
	 * @return The found section.
	 */
	TLLRN_CONTROLRIG_API class UMovieSceneSection* FindOrExtendSection(FFrameNumber Time, float& OutWeight);

	/**
	 * Finds a section at the current time.
	 *
	 * @param Time The time relative to the owning movie scene where the section should be
	 * @param bSectionAdded Whether a section was added or not
	 * @return The found section, or the new section.
	 */
	TLLRN_CONTROLRIG_API class UMovieSceneSection* FindOrAddSection(FFrameNumber Time, bool& bSectionAdded);

	/**
	 * Set the section we want to key and recieve globally changed values.
	 *
	 * @param Section The section that changes.
	 */

	virtual void SetSectionToKey(UMovieSceneSection* Section) override;

	/**
	 * Finds a section we want to key and recieve globally changed values.
	 * @return The Section that changes.
	 */
	virtual UMovieSceneSection* GetSectionToKey() const override;

	/**
	* Get multiple sections to key
	* @return multiple sections to key
	*/
	virtual TArray<TWeakObjectPtr<UMovieSceneSection>> GetSectionsToKey() const override;

	/**
	*  Control rig supports per control sections to key needed for anim layer workflows
	*/
	TLLRN_CONTROLRIG_API UMovieSceneSection* GetSectionToKey(const FName& InControlName) const;
	TLLRN_CONTROLRIG_API void SetSectionToKey(UMovieSceneSection* InSection, const FName& inControlName);

	TLLRN_CONTROLRIG_API void SetTrackName(FName InName) { TrackName = InName; }

	UMovieSceneTLLRN_ControlRigParameterSection::FSpaceChannelAddedEvent& SpaceChannelAdded() { return OnSpaceChannelAdded; }
	IMovieSceneConstrainedSection::FConstraintChannelAddedEvent& ConstraintChannelAdded() { return OnConstraintChannelAdded; }

public:
	TLLRN_CONTROLRIG_API TArray<FName> GetControlsWithDifferentRotationOrders() const;
	TLLRN_CONTROLRIG_API void ResetControlsToSettingsRotationOrder(const TArray<FName>& Names,
		EMovieSceneKeyInterpolation Interpolation = EMovieSceneKeyInterpolation::SmartAuto);
	// if order is not set then it uses the default FRotator conversions
	TLLRN_CONTROLRIG_API void ChangeTLLRN_ControlRotationOrder(const FName& InControlName, const TOptional<EEulerRotationOrder>& NewOrder,
		EMovieSceneKeyInterpolation Interpolation = EMovieSceneKeyInterpolation::SmartAuto);

	TLLRN_CONTROLRIG_API int32 GetPriorityOrder() const;
	TLLRN_CONTROLRIG_API void SetPriorityOrder(int32 InPriorityIndex);
private:
	UPROPERTY()
	TMap<FName, TWeakObjectPtr<UMovieSceneSection>> SectionToKeyPerControl;
public:
	bool bSetSectionToKeyPerControl = true;// set up the map when calling the UMovieSceneTrack::SetSectionToKey()
private:
	//get the rotation orderm will not be set if it's default FRotator order. If bCurrent is true, it uses what's set
	//if false it uses the default setting from the current control rig.
	TOptional<EEulerRotationOrder> GetTLLRN_ControlRotationOrder(const FTLLRN_RigControlElement* ControlElement, bool bCurrent) const;
private:

	//we register this with sections.
	void HandleOnSpaceAdded(UMovieSceneTLLRN_ControlRigParameterSection* Section, const FName& InControlName, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel);
	//we'll register this against all space channels
	void HandleOnSpaceNoLongerUsed(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* InChannel, const TArray<FTLLRN_RigElementKey>& InSpaces, FName InControlName);
	//then send this event out to the track editor.
	UMovieSceneTLLRN_ControlRigParameterSection::FSpaceChannelAddedEvent OnSpaceChannelAdded;

	void HandleOnConstraintAdded(
		IMovieSceneConstrainedSection* InSection,
		FMovieSceneConstraintChannel* InChannel) const;
	IMovieSceneConstrainedSection::FConstraintChannelAddedEvent OnConstraintChannelAdded;
	void ReconstructTLLRN_ControlRig();

private:


	/** Control Rig we control*/
	UPROPERTY()
	TObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;

	/** Section we should Key */
	UPROPERTY()
	TObjectPtr<UMovieSceneSection> SectionToKey;

	/** The sections owned by this track .*/
	UPROPERTY()
	TArray<TObjectPtr<UMovieSceneSection>> Sections;

	/** Unique Name*/
	UPROPERTY()
	FName TrackName;

	/** Uses Rotation Order*/
	UPROPERTY()
	TMap<FName,FTLLRN_ControlRotationOrder> ControlsRotationOrder;

	UPROPERTY()
	int32 PriorityOrder;

public:
	static TLLRN_CONTROLRIG_API FColor AbsoluteRigTrackColor;
	static TLLRN_CONTROLRIG_API FColor LayeredRigTrackColor;

public:
	UTLLRN_ControlRig* GetGameWorldTLLRN_ControlRig(UWorld* InWorld);
	bool IsAGameInstance(const UTLLRN_ControlRig* InTLLRN_ControlRig, const bool bCheckValidWorld = false) const;
    
private:
	/** copy of the controlled control rig that we use in the game world so editor control rig doesn't conflict*/
	UPROPERTY(transient)
	TMap<TWeakObjectPtr<UWorld>,TObjectPtr<UTLLRN_ControlRig>> GameWorldTLLRN_ControlRigs;
};




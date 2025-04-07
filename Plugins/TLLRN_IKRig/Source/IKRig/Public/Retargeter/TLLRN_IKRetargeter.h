// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_IKRetargetProfile.h"
#include "Rig/TLLRN_IKRigDefinition.h"
#include "TLLRN_IKRetargetSettings.h"

#include "TLLRN_IKRetargeter.generated.h"

#if WITH_EDITOR
class FTLLRN_IKRetargetEditorController;
#endif
struct FTLLRN_IKRetargetPose;
class UTLLRN_RetargetChainSettings;
class UTLLRN_RetargetRootSettings;
class UTLLRN_RetargetOpStack;

struct UE_DEPRECATED(5.1, "Use UTLLRN_RetargetChainSettings instead.") FTLLRN_RetargetChainMap;
USTRUCT()
struct TLLRN_IKRIG_API FTLLRN_RetargetChainMap
{
	GENERATED_BODY()

	FTLLRN_RetargetChainMap() = default;
	FTLLRN_RetargetChainMap(const FName& TargetChain) : TargetChain(TargetChain){}
	
	UPROPERTY(EditAnywhere, Category = Offsets)
	FName SourceChain = NAME_None;
	
	UPROPERTY(EditAnywhere, Category = Offsets)
	FName TargetChain = NAME_None;
};

UCLASS(BlueprintType)
class TLLRN_IKRIG_API UTLLRN_RetargetChainSettings : public UObject
{
	GENERATED_BODY()

public:
	
	UTLLRN_RetargetChainSettings() = default;
	
	UTLLRN_RetargetChainSettings(const FName& TargetChain) : TargetChain(TargetChain){}
	
	// UObject 
	virtual void Serialize(FArchive& Ar) override;
	// END UObject 
	
	// The chain on the Source IK Rig asset to copy animation FROM. 
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Chain Mapping")
	FName SourceChain;

	// The chain on the Target IK Rig asset to copy animation TO. 
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Chain Mapping")
	FName TargetChain;

	// The settings used to control the motion on this target chain. 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FTLLRN_TargetChainSettings Settings;

#if WITH_EDITORONLY_DATA
	// Deprecated properties from before FTLLRN_TargetChainSettings / profile refactor  (July 2022)
	UPROPERTY()
	bool CopyPoseUsingFK_DEPRECATED = true;
	UPROPERTY()
	ETLLRN_RetargetRotationMode RotationMode_DEPRECATED;
	UPROPERTY()
	float RotationAlpha_DEPRECATED = 1.0f;
	UPROPERTY()
	ETLLRN_RetargetTranslationMode TranslationMode_DEPRECATED;
	UPROPERTY()
	float TranslationAlpha_DEPRECATED = 1.0f;
	UPROPERTY()
	bool DriveIKGoal_DEPRECATED = true;
	UPROPERTY()
	float BlendToSource_DEPRECATED = 0.0f;
	UPROPERTY()
	FVector BlendToSourceWeights_DEPRECATED = FVector::OneVector;
	UPROPERTY()
	FVector StaticOffset_DEPRECATED;
	UPROPERTY()
	FVector StaticLocalOffset_DEPRECATED;
	UPROPERTY()
	FRotator StaticRotationOffset_DEPRECATED;
	UPROPERTY()
	float Extension_DEPRECATED = 1.0f;
	UPROPERTY()
	bool UseSpeedCurveToPlantIK_DEPRECATED = false;
	UPROPERTY()
	FName SpeedCurveName_DEPRECATED;
	UPROPERTY()
	float VelocityThreshold_DEPRECATED = 15.0f;
	UPROPERTY()
	float UnplantStiffness_DEPRECATED = 250.0f;
	UPROPERTY()
	float UnplantCriticalDamping_DEPRECATED = 1.0f;
	// END deprecated properties 
#endif

	// pointer to editor for details customization
	#if WITH_EDITOR
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;
	#endif
};

UCLASS(BlueprintType)
class TLLRN_IKRIG_API UTLLRN_RetargetRootSettings: public UObject
{
	GENERATED_BODY()
	
public:
	
	// UObject 
	virtual void Serialize(FArchive& Ar) override;
	// END UObject 

	// The settings used to control the motion of the target root bone. 
	UPROPERTY(EditAnywhere, Category = "Settings")
	FTLLRN_TargetRootSettings Settings;

	// pointer to editor for details customization
	#if WITH_EDITOR
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;
	#endif

private:
#if WITH_EDITORONLY_DATA
	// Deprecated properties from before FTLLRN_TargetRootSettings / profile refactor 
	UPROPERTY()
	bool RetargetRootTranslation_DEPRECATED = true;
	UPROPERTY()
	float GlobalScaleHorizontal_DEPRECATED = 1.0f;
	UPROPERTY()
	float GlobalScaleVertical_DEPRECATED = 1.0f;
	UPROPERTY()
	FVector BlendToSource_DEPRECATED = FVector::ZeroVector;
	UPROPERTY()
	FVector StaticOffset_DEPRECATED = FVector::ZeroVector;
	UPROPERTY()
	FRotator StaticRotationOffset_DEPRECATED = FRotator::ZeroRotator;
	// END deprecated properties 
#endif
};

UCLASS(BlueprintType)
class TLLRN_IKRIG_API UTLLRN_IKRetargetGlobalSettings: public UObject
{
	GENERATED_BODY()
	
public:

	// Global retargeter settings. 
	UPROPERTY(EditAnywhere, Category = "Settings")
	FTLLRN_RetargetGlobalSettings Settings;

	// pointer to editor for details customization
	#if WITH_EDITOR
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;
	#endif
};

USTRUCT(BlueprintType)
struct TLLRN_IKRIG_API FTLLRN_IKRetargetPose
{
	GENERATED_BODY()
	
public:
	
	FTLLRN_IKRetargetPose() = default;

	FQuat GetDeltaRotationForBone(const FName BoneName) const;
	void SetDeltaRotationForBone(FName BoneName, const FQuat& RotationDelta);
	const TMap<FName, FQuat>& GetAllDeltaRotations() const { return BoneRotationOffsets; };

	FVector GetRootTranslationDelta() const;
	void SetRootTranslationDelta(const FVector& TranslationDelta);
	void AddToRootTranslationDelta(const FVector& TranslationDelta);
	
	void SortHierarchically(const FTLLRN_IKRigSkeleton& Skeleton);
	
	int32 GetVersion() const { return Version; };
	void IncrementVersion() { ++Version; };

private:
	// a translational delta in GLOBAL space, applied only to the retarget root bone
	UPROPERTY(EditAnywhere, Category = RetargetPose)
	FVector RootTranslationOffset = FVector::ZeroVector;

	// these are LOCAL-space rotation deltas to be applied to a bone to modify it's retarget pose
	UPROPERTY(EditAnywhere, Category = RetargetPose)
	TMap<FName, FQuat> BoneRotationOffsets;
	
	// incremented by any edits to the retarget pose, indicating to any running instance that it should reinitialize
	// this is not made "editor only" to leave open the possibility of programmatically modifying a retarget pose in cooked builds
	int32 Version = INDEX_NONE;

	friend class UTLLRN_IKRetargeterController;
};

// which skeleton are we referring to?
UENUM()
enum class ETLLRN_RetargetSourceOrTarget : uint8
{
	// the SOURCE skeleton (to copy FROM)
	Source,
	// the TARGET skeleton (to copy TO)
	Target,
};

UCLASS(BlueprintType)
class TLLRN_IKRIG_API UTLLRN_IKRetargeter : public UObject
{
	GENERATED_BODY()
	
public:
	
	UTLLRN_IKRetargeter(const FObjectInitializer& ObjectInitializer);

	// Get read-only access to the source or target IK Rig asset 
	const UTLLRN_IKRigDefinition* GetIKRig(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	// Get read-write access to the source IK Rig asset.
	// WARNING: do not use for editing the data model. Use Controller class instead. 
	UTLLRN_IKRigDefinition* GetIKRigWriteable(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	#if WITH_EDITORONLY_DATA
	// Get read-only access to preview meshes
	USkeletalMesh* GetPreviewMesh(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	#endif

	// Get read-only access to the chain mapping 
	const TArray<TObjectPtr<UTLLRN_RetargetChainSettings>>& GetAllChainSettings() const { return ChainSettings; };
	// Get read-only access to the chain map for a given chain (null if chain not in retargeter) 
	const TObjectPtr<UTLLRN_RetargetChainSettings> GetChainMapByName(const FName& TargetChainName) const;
	// Get read-only access to the chain settings for a given chain (null if chain not in retargeter) 
	const FTLLRN_TargetChainSettings* GetChainSettingsByName(const FName& TargetChainName) const;
	// Get access to the root settings 
	UTLLRN_RetargetRootSettings* GetRootSettingsUObject() const { return RootSettings; };
	// Get access to the global settings uobject
	UTLLRN_IKRetargetGlobalSettings* GetGlobalSettingsUObject() const { return GlobalSettings; };
	// Get access to the global settings itself 
	const FTLLRN_RetargetGlobalSettings& GetGlobalSettings() const { return GlobalSettings->Settings; };
	// Get access to the post settings uobject
	UTLLRN_RetargetOpStack* GetPostSettingsUObject() const { return OpStack; };

	// Get read-only access to a retarget pose 
	const FTLLRN_IKRetargetPose* GetCurrentRetargetPose(const ETLLRN_RetargetSourceOrTarget& SourceOrTarget) const;
	// Get name of the current retarget pose 
	FName GetCurrentRetargetPoseName(const ETLLRN_RetargetSourceOrTarget& SourceOrTarget) const;
	// Get read-only access to a retarget pose 
	const FTLLRN_IKRetargetPose* GetRetargetPoseByName(const ETLLRN_RetargetSourceOrTarget& SourceOrTarget, const FName PoseName) const;
	// Get name of default pose 
	static const FName GetDefaultPoseName();
	
	// Fill the provided profile with settings from this asset
	void FillProfileWithAssetSettings(FTLLRN_RetargetProfile& InOutProfile) const;
	// Get the current retarget profile (may be null) 
	const FTLLRN_RetargetProfile* GetCurrentProfile() const;
	// Get the retarget profile by name (may be null) 
	const FTLLRN_RetargetProfile* GetProfileByName(const FName& ProfileName) const;

	// get current version of the data (to compare against running processor instances)
	int32 GetVersion() const { return Version; };
	// do this after any edit that would require running instance to reinitialize
	void IncrementVersion() { ++Version; };

	// BLUEPRINT GETTERS 

	// Returns the chain settings associated with a given Goal in an IK Retargeter Asset using the given profile name (optional) 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=RetargetAsset)
	static FTLLRN_TargetChainSettings GetChainUsingGoalFromRetargetAsset(
		const UTLLRN_IKRetargeter* RetargetAsset,
		const FName IKGoalName);
	
	// Returns the chain settings associated with a given target chain in an IK Retargeter Asset using the given profile name (optional) 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=RetargetAsset)
	static FTLLRN_TargetChainSettings GetChainSettingsFromRetargetAsset(
		const UTLLRN_IKRetargeter* RetargetAsset,
		const FName TargetChainName,
		const FName OptionalProfileName);

	// Returns the chain settings associated with a given target chain in the supplied Retarget Profile. 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=RetargetProfile)
	static FTLLRN_TargetChainSettings GetChainSettingsFromRetargetProfile(
		UPARAM(ref) FTLLRN_RetargetProfile& RetargetProfile,
		const FName TargetChainName);

	// Returns the root settings in an IK Retargeter Asset using the given profile name (optional) 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=RetargetAsset)
	static void GetRootSettingsFromRetargetAsset(
		const UTLLRN_IKRetargeter* RetargetAsset,
		const FName OptionalProfileName,
		UPARAM(DisplayName = "ReturnValue") FTLLRN_TargetRootSettings& OutSettings);

	// Returns the root settings in the supplied Retarget Profile. 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=RetargetProfile)
	static FTLLRN_TargetRootSettings GetRootSettingsFromRetargetProfile(UPARAM(ref) FTLLRN_RetargetProfile& RetargetProfile);

	// Returns the global settings in an IK Retargeter Asset using the given profile name (optional) 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=RetargetAsset)
	static void GetGlobalSettingsFromRetargetAsset(
		const UTLLRN_IKRetargeter* RetargetAsset,
		const FName OptionalProfileName,
		UPARAM(DisplayName = "ReturnValue") FTLLRN_RetargetGlobalSettings& OutSettings);

	// Returns the global settings in the supplied Retarget Profile. 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=RetargetProfile)
	static FTLLRN_RetargetGlobalSettings GetGlobalSettingsFromRetargetProfile(UPARAM(ref) FTLLRN_RetargetProfile& RetargetProfile);

	// Returns true if the source IK Rig has been assigned
	UFUNCTION(BlueprintPure, Category=RetargetProfile)
	bool HasSourceIKRig() const { return SourceIKRigAsset != nullptr; }

	// Returns true if the target IK Rig has been assigned
	UFUNCTION(BlueprintPure, Category=RetargetProfile)
	bool HasTargetIKRig() const { return TargetIKRigAsset != nullptr; }
	
	// BLUEPRINT SETTERS 

	// Set the global settings in a retarget profile (will set bApplyGlobalSettings to true). 
	UFUNCTION(BlueprintCallable, Category=RetargetProfile)
	static void SetGlobalSettingsInRetargetProfile(
		UPARAM(ref) FTLLRN_RetargetProfile& RetargetProfile,
		const FTLLRN_RetargetGlobalSettings& GlobalSettings);

	// Set the root settings in a retarget profile (will set bApplyRootSettings to true). 
	UFUNCTION(BlueprintCallable, Category=RetargetProfile)
	static void SetRootSettingsInRetargetProfile(
		UPARAM(ref) FTLLRN_RetargetProfile& RetargetProfile,
		const FTLLRN_TargetRootSettings& RootSettings);
	
	// Set the chain settings in a retarget profile (will set bApplyChainSettings to true). 
	UFUNCTION(BlueprintCallable, Category=RetargetProfile)
	static void SetChainSettingsInRetargetProfile(
		UPARAM(ref) FTLLRN_RetargetProfile& RetargetProfile,
		const FTLLRN_TargetChainSettings& ChainSettings,
		const FName TargetChainName);

	// Set the chain FK settings in a retarget profile (will set bApplyChainSettings to true). 
	UFUNCTION(BlueprintCallable, Category=RetargetProfile)
	static void SetChainFKSettingsInRetargetProfile(
		UPARAM(ref) FTLLRN_RetargetProfile& RetargetProfile,
		const FTLLRN_TargetChainFKSettings& FKSettings,
		const FName TargetChainName);

	// Set the chain IK settings in a retarget profile (will set bApplyChainSettings to true). 
	UFUNCTION(BlueprintCallable, Category=RetargetProfile)
	static void SetChainIKSettingsInRetargetProfile(
		UPARAM(ref) FTLLRN_RetargetProfile& RetargetProfile,
		const FTLLRN_TargetChainIKSettings& IKSettings,
		const FName TargetChainName);

	// Set the chain Speed Plant settings in a retarget profile (will set bApplyChainSettings to true). 
	UFUNCTION(BlueprintCallable, Category=RetargetProfile)
	static void SetChainSpeedPlantSettingsInRetargetProfile(
		UPARAM(ref) FTLLRN_RetargetProfile& RetargetProfile,
		const FTLLRN_TargetChainSpeedPlantSettings& SpeedPlantSettings,
		const FName TargetChainName);

	// UObject
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	// END UObject
	#if WITH_EDITORONLY_DATA
    	static void DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass);
    #endif

#if WITH_EDITOR
	// Get name of Source IK Rig property 
	static const FName GetSourceIKRigPropertyName();
	// Get name of Target IK Rig property 
	static const FName GetTargetIKRigPropertyName();
	// Get name of Source Preview Mesh property 
	static const FName GetSourcePreviewMeshPropertyName();
	// Get name of Target Preview Mesh property 
	static const FName GetTargetPreviewMeshPropertyName();
	// Get the names of the all the speed curves the retargeter will be looking for 
	void GetSpeedCurveNames(TArray<FName>& OutSpeedCurveNames) const;
#endif

private:

	void CleanAndInitialize();
	
	// incremented by any edits that require re-initialization
	UPROPERTY(Transient)
	int32 Version = INDEX_NONE;

	// The rig to copy animation FROM.
	UPROPERTY(EditAnywhere, Category = Source)
	TSoftObjectPtr<UTLLRN_IKRigDefinition> SourceIKRigAsset = nullptr;

#if WITH_EDITORONLY_DATA
	// Optional. Override the Skeletal Mesh to copy animation from. Uses the preview mesh from the Source IK Rig asset by default. 
	UPROPERTY(EditAnywhere, Category = Source, meta = (EditCondition = "HasSourceIKRig"))
	TSoftObjectPtr<USkeletalMesh> SourcePreviewMesh = nullptr;
#endif
	
	// The rig to copy animation TO.
	UPROPERTY(EditAnywhere, Category = Target)
	TSoftObjectPtr<UTLLRN_IKRigDefinition> TargetIKRigAsset = nullptr;

#if WITH_EDITORONLY_DATA
	// Optional. Override the Skeletal Mesh to preview the retarget on. Uses the preview mesh from the Target IK Rig asset by default. 
	UPROPERTY(EditAnywhere, Category = Target, meta = (EditCondition = "HasTargetIKRig"))
	TSoftObjectPtr<USkeletalMesh> TargetPreviewMesh = nullptr;
#endif
	
public:

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bRetargetRoot_DEPRECATED = true;
	UPROPERTY()
	bool bRetargetFK_DEPRECATED = true;
	UPROPERTY()
	bool bRetargetIK_DEPRECATED = true;
	UPROPERTY()
	float TargetActorOffset_DEPRECATED = 0.0f;
	UPROPERTY()
	float TargetActorScale_DEPRECATED = 0.0f;

	// The offset applied to the target mesh in the editor viewport. 
	UPROPERTY(EditAnywhere, Category = PreviewSettings)
	FVector TargetMeshOffset;

	// Scale the target mesh in the viewport for easier visualization next to the source.
	UPROPERTY(EditAnywhere, Category = PreviewSettings, meta = (UIMin = "0.01", UIMax = "10.0"))
	float TargetMeshScale = 1.0f;

	// The offset applied to the source mesh in the editor viewport. 
	UPROPERTY(EditAnywhere, Category = PreviewSettings)
	FVector SourceMeshOffset;

	// When true, animation sequences with "Force Root Lock" turned On will act as though it is Off.
	// This affects only the preview in the retarget editor. Use ExportRootLockMode to control exported animation behavior.
	// This setting has no effect on runtime retargeting where root motion is copied from the source component.
	UPROPERTY(EditAnywhere, Category = RootLockSettings)
	bool bIgnoreRootLockInPreview = true;

	// Toggle debug drawing for retargeting in the viewport. 
	UPROPERTY(EditAnywhere, Category = DebugSettings)
	bool bDebugDraw = true;

	// Draw lines on each bone chain. 
	UPROPERTY(EditAnywhere, Category = DebugSettings)
	bool bDrawChainLines = true;

	// Draw spheres on single bone chains. 
	UPROPERTY(EditAnywhere, Category = DebugSettings)
	bool bDrawSingleBoneChains = false;
	
	// Draw final IK goal locations. 
	UPROPERTY(EditAnywhere, Category = DebugSettings)
	bool bDrawFinalGoals = true;

	// Draw goal locations from source skeleton. 
	UPROPERTY(EditAnywhere, Category = DebugSettings)
	bool bDrawSourceLocations = true;

	// Draw circle on the floor below the retarget root. 
	UPROPERTY(EditAnywhere, Category = DebugSettings)
	bool bDrawRootCircle = true;

	// Draw coordinate frame used to define stride warping directions.  
	UPROPERTY(EditAnywhere, Category = DebugSettings)
	bool bDrawWarpingFrame = false;
	
	// The visual size of the IK goals in the viewport. 
	UPROPERTY(EditAnywhere, Category = DebugSettings)
	float ChainDrawSize = 5.0f;

	// The thickness of lines on the IK goals in the viewport. 
	UPROPERTY(EditAnywhere, Category = DebugSettings)
	float ChainDrawThickness = 0.5f;
	
	// The visual size of the bones in the viewport (saved between sessions). This is set from the viewport Character>Bones menu
	UPROPERTY()
	float BoneDrawSize = 1.0f;

	// The controller responsible for managing this asset's data (all editor mutation goes through this)
	UPROPERTY(Transient, DuplicateTransient, NonTransactional )
	TObjectPtr<UObject> Controller;
	
private:
	
	// only ask to fix the root height once, then warn thereafter (don't nag) 
	TSet<TObjectPtr<USkeletalMesh>> MeshesAskedToFixRootHeightFor;
#endif
	
private:

	// (OLD VERSION) Mapping of chains to copy animation between source and target rigs.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	UPROPERTY()
	TArray<FTLLRN_RetargetChainMap> ChainMapping_DEPRECATED;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	
	// Settings for how to map source chains to target chains.
	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_RetargetChainSettings>> ChainSettings;
	
	// the retarget root settings 
	UPROPERTY()
	TObjectPtr<UTLLRN_RetargetRootSettings> RootSettings;

	// the retarget root settings 
	UPROPERTY()
	TObjectPtr<UTLLRN_IKRetargetGlobalSettings> GlobalSettings;

	// the stack of ops to run after the retarget
	UPROPERTY()
	TObjectPtr<UTLLRN_RetargetOpStack> OpStack;

	// settings profiles stored in this asset 
	UPROPERTY()
	TMap<FName, FTLLRN_RetargetProfile> Profiles;
	UPROPERTY()
	FName CurrentProfile = NAME_None;
	
	// The set of retarget poses for the SOURCE skeleton.
	UPROPERTY()
	TMap<FName, FTLLRN_IKRetargetPose> SourceRetargetPoses;
	// The set of retarget poses for the TARGET skeleton.
	UPROPERTY()
	TMap<FName, FTLLRN_IKRetargetPose> TargetRetargetPoses;
	
	// The current retarget pose to use for the SOURCE.
	UPROPERTY()
	FName CurrentSourceRetargetPose;
	// The current retarget pose to use for the TARGET.
	UPROPERTY()
	FName CurrentTargetRetargetPose;

	// (OLD VERSION) Before retarget poses were stored for target AND source.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	UPROPERTY()
	TMap<FName, FTLLRN_IKRetargetPose> RetargetPoses_DEPRECATED;
	UPROPERTY()
	FName CurrentRetargetPose_DEPRECATED;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	friend class UTLLRN_IKRetargeterController;
};

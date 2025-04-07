// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_CustomProperty.h"
#include "AnimNodes/TLLRN_AnimNode_IKRig.h"
#include "IDetailCustomNodeBuilder.h"

#include "TLLRN_AnimGraphNode_IKRig.generated.h"

class FPrimitiveDrawInterface;
class USkeletalMeshComponent;

/////////////////////////////////////////////////////
// FTLLRN_IKRigGoalLayout

class FTLLRN_IKRigGoalLayout : public IDetailCustomNodeBuilder, public TSharedFromThis<FTLLRN_IKRigGoalLayout>
{
public:
	
	FTLLRN_IKRigGoalLayout(TSharedPtr<IPropertyHandle> InGoalPropHandle,
					 const bool InExposePosition,
					 const bool InExposeRotation)
		: GoalPropHandle(InGoalPropHandle)
		, bExposePosition(InExposePosition)
		, bExposeRotation(InExposeRotation)
	{}

	/** IDetailCustomNodeBuilder Interface*/
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& InOutGoalRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& InOutChildrenBuilder) override;
	virtual void Tick(float DeltaTime) override {}
	virtual bool RequiresTick() const override { return false; }
	virtual FName GetName() const override;
	virtual bool InitiallyCollapsed() const override { return true; }

	static FName GetGoalName(TSharedPtr<IPropertyHandle> InGoalHandle);
	
private:

	ETLLRN_IKRigGoalTransformSource GetTransformSource() const;

	const struct FReferenceSkeleton& GetReferenceSkeleton() const;
	TSharedPtr<IPropertyHandle> GetBoneNameHandle() const;
	void OnBoneSelectionChanged(FName Name) const;
	FName GetSelectedBone(bool& bMultipleValues) const;

	TSharedRef<SWidget> CreatePropertyWidget() const;
	TSharedRef<SWidget> CreateBoneValueWidget() const;
	TSharedRef<SWidget> CreateValueWidget() const;
	
	TSharedPtr<IPropertyHandle> GoalPropHandle = nullptr;
	bool bExposePosition = false;
	bool bExposeRotation = false;
};

/////////////////////////////////////////////////////
// FTLLRN_IKRigGoalArrayLayout

class FTLLRN_IKRigGoalArrayLayout : public IDetailCustomNodeBuilder, public TSharedFromThis<FTLLRN_IKRigGoalArrayLayout>
{
public:
	
	FTLLRN_IKRigGoalArrayLayout(TSharedPtr<IPropertyHandle> InNodePropHandle)
		: NodePropHandle(InNodePropHandle)
	{}
	
	virtual ~FTLLRN_IKRigGoalArrayLayout() {}

	/** IDetailCustomNodeBuilder Interface*/
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override {}
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override {}
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual void Tick(float DeltaTime) override {}
	virtual bool RequiresTick() const override { return false; }
	virtual FName GetName() const override { return NAME_None; }
	virtual bool InitiallyCollapsed() const override { return false; }

private:
	
	TSharedPtr<IPropertyHandle> NodePropHandle;
};

// Editor node for IKRig 
UCLASS()
class TLLRN_IKRIGDEVELOPER_API UTLLRN_AnimGraphNode_IKRig : public UAnimGraphNode_CustomProperty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FTLLRN_AnimNode_IKRig Node;

public:

	UTLLRN_AnimGraphNode_IKRig() = default;
	virtual ~UTLLRN_AnimGraphNode_IKRig();
	
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void CreateCustomPins(TArray<UEdGraphPin*>* InOldPins) override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual void CopyNodeDataToPreviewNode(FAnimNode_Base* AnimNode) override;
	virtual void Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const override;
	virtual void ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog) override;
	// End of UAnimGraphNode_Base interface

	// UK2Node interface
	UObject* GetJumpTargetForDoubleClick() const;
	// END UK2Node

	// Begin UObject Interface.
	virtual void PostLoad() override;
	virtual void PostEditUndo() override;
	// End UObject Interface.

	virtual bool NeedsToSpecifyValidTargetClass() const override { return false; }
	
protected:
	
	virtual FAnimNode_CustomProperty* GetCustomPropertyNode() override { return &Node; }
	virtual const FAnimNode_CustomProperty* GetCustomPropertyNode() const override { return &Node; }

private:

	// set pin's default value based on the FTLLRN_IKRigGoal default struct
	static void SetPinDefaultValue(UEdGraphPin* InPin, const FName& InPropertyName);

	// create custom pins from exposed goals as defined in the rig definition asset 
	void CreateCustomPinsFromValidAsset();

	// recreate custom pins from old pins if the rig definition asset is not completely loaded
	void CreateCustomPinsFromUnloadedAsset(TArray<UEdGraphPin*>* InOldPins);

	// Handle to the registered delegate
	FDelegateHandle OnAssetPropertyChangedHandle;

	// Global callback to anticipate on changes to the asset / goals
	bool NeedsUpdate(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent) const;
	void OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);
	void BindPropertyChanges();

	// update the goals' array within the anim node based on the asset
	void UpdateGoalsFromAsset();

	// setup goal based on it's asset definition 
	static void SetupGoal(const UTLLRN_IKRigEffectorGoal* InAssetGoal, FTLLRN_IKRigGoal& OutGoal);
};

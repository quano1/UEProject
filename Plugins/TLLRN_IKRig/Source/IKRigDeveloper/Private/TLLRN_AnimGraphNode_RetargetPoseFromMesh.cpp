// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_AnimGraphNode_RetargetPoseFromMesh.h"
#include "Animation/AnimInstance.h"
#include "Kismet2/CompilerResultsLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_AnimGraphNode_RetargetPoseFromMesh)

#define LOCTEXT_NAMESPACE "TLLRN_AnimGraphNode_IKRig"
const FName UTLLRN_AnimGraphNode_RetargetPoseFromMesh::AnimModeName(TEXT("IKRig.TLLRN_IKRigEditor.TLLRN_IKRigEditMode"));

void UTLLRN_AnimGraphNode_RetargetPoseFromMesh::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const
{
}

FText UTLLRN_AnimGraphNode_RetargetPoseFromMesh::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("TLLRN_AnimGraphNode_IKRetargeter_Title", "Retarget Pose From Mesh");
}

void UTLLRN_AnimGraphNode_RetargetPoseFromMesh::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FTLLRN_AnimNode_RetargetPoseFromMesh* TLLRN_IKRetargeterNode = static_cast<FTLLRN_AnimNode_RetargetPoseFromMesh*>(InPreviewNode);
}

FEditorModeID UTLLRN_AnimGraphNode_RetargetPoseFromMesh::GetEditorMode() const
{
	return AnimModeName;
}

void UTLLRN_AnimGraphNode_RetargetPoseFromMesh::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	Super::CustomizePinData(Pin, SourcePropertyName, ArrayIndex);
}

UObject* UTLLRN_AnimGraphNode_RetargetPoseFromMesh::GetJumpTargetForDoubleClick() const
{
	return Node.TLLRN_IKRetargeterAsset;
}

void UTLLRN_AnimGraphNode_RetargetPoseFromMesh::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None);
	if ((PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_AnimNode_RetargetPoseFromMesh, bUseAttachedParent)))
	{
		ReconstructNode();
	}
}

void UTLLRN_AnimGraphNode_RetargetPoseFromMesh::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton,	FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);

	// validate source mesh component is not null
	if (!Node.bUseAttachedParent)
	{
		const bool bIsLinked = IsPinExposedAndLinked(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_AnimNode_RetargetPoseFromMesh, SourceMeshComponent));
		const bool bIsBound = IsPinExposedAndBound(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_AnimNode_RetargetPoseFromMesh, SourceMeshComponent));
		if (!(bIsLinked || bIsBound))
		{
			MessageLog.Error(TEXT("@@ is missing a Source Skeletal Mesh Component reference."), this);
			return;
		}
	}

	// validate IK Rig asset has been assigned
	if (!Node.TLLRN_IKRetargeterAsset)
	{
		UEdGraphPin* Pin = FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_AnimNode_RetargetPoseFromMesh, TLLRN_IKRetargeterAsset));
		if (Pin == nullptr)
		{
			// retarget asset unassigned
			MessageLog.Error(TEXT("@@ does not have an IK Retargeter asset assigned."), this);
		}
		return;
	}

	// validate SOURCE IK Rig asset has been assigned
	const UTLLRN_IKRigDefinition* SourceIKRig = Node.TLLRN_IKRetargeterAsset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Source);
	if (!SourceIKRig)
	{
		MessageLog.Warning(TEXT("@@ has IK Retargeter that is missing a source IK Rig asset."), this);
	}

	// validate TARGET IK Rig asset has been assigned
	const UTLLRN_IKRigDefinition* TargetIKRig = Node.TLLRN_IKRetargeterAsset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target);
	if (!TargetIKRig)
	{
		MessageLog.Warning(TEXT("@@ has IK Retargeter that is missing a target IK Rig asset."), this);
	}

	if (!(SourceIKRig && TargetIKRig))
	{
		return;
	}

	// pull messages out of the processor's log
	if (!Node.bSuppressWarnings)
	{
		if (const UTLLRN_IKRetargetProcessor* Processor = Node.GetRetargetProcessor())
		{
			const TArray<FText>& Warnings = Processor->Log.GetWarnings();
			for (const FText& Warning : Warnings)
			{
				MessageLog.Warning(*Warning.ToString());
			}
		
			const TArray<FText>& Errors = Processor->Log.GetErrors();
			for (const FText& Error : Errors)
			{
				MessageLog.Error(*Error.ToString());
			}
		}
	}

	if (ForSkeleton && !Node.bSuppressWarnings)
	{
		// validate that target bone chains exist on this skeleton
		const FReferenceSkeleton &RefSkel = ForSkeleton->GetReferenceSkeleton();
		const TArray<FTLLRN_BoneChain> &TargetBoneChains = TargetIKRig->GetRetargetChains();
		for (const FTLLRN_BoneChain &Chain : TargetBoneChains)
		{
			if (RefSkel.FindBoneIndex(Chain.StartBone.BoneName) == INDEX_NONE)
			{
				MessageLog.Warning(*FText::Format(LOCTEXT("StartBoneNotFound", "@@ - Start Bone '{0}' in target IK Rig Bone Chain not found."), FText::FromName(Chain.StartBone.BoneName)).ToString(), this);
			}

			if (RefSkel.FindBoneIndex(Chain.EndBone.BoneName) == INDEX_NONE)
			{
				MessageLog.Warning(*FText::Format(LOCTEXT("EndBoneNotFound", "@@ - End Bone '{0}' in target IK Rig Bone Chain not found."), FText::FromName(Chain.EndBone.BoneName)).ToString(), this);
			}
		}
	}
}

void UTLLRN_AnimGraphNode_RetargetPoseFromMesh::PreloadRequiredAssets()
{
	Super::PreloadRequiredAssets();
	
	if (Node.TLLRN_IKRetargeterAsset)
	{
		PreloadObject(Node.TLLRN_IKRetargeterAsset);
		PreloadObject(Node.TLLRN_IKRetargeterAsset->GetIKRigWriteable(ETLLRN_RetargetSourceOrTarget::Source));
		PreloadObject(Node.TLLRN_IKRetargeterAsset->GetIKRigWriteable(ETLLRN_RetargetSourceOrTarget::Target));
	}
}

#undef LOCTEXT_NAMESPACE


// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigThumbnailRenderer.h"

#include "Engine/SkeletalMesh.h"
#include "Rig/TLLRN_IKRigDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRigThumbnailRenderer)

bool UTLLRN_IKRigThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	return GetPreviewMeshFromRig(Object) != nullptr;
}

void UTLLRN_IKRigThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	if (USkeletalMesh* MeshToDraw = GetPreviewMeshFromRig(Object))
	{
		Super::Draw(MeshToDraw, X, Y, Width, Height, RenderTarget, Canvas, bAdditionalViewFamily);	
	}
}

EThumbnailRenderFrequency UTLLRN_IKRigThumbnailRenderer::GetThumbnailRenderFrequency(UObject* Object) const
{
	return GetPreviewMeshFromRig(Object) ? EThumbnailRenderFrequency::Realtime : EThumbnailRenderFrequency::OnPropertyChange;
}

USkeletalMesh* UTLLRN_IKRigThumbnailRenderer::GetPreviewMeshFromRig(UObject* Object) const
{
	const UTLLRN_IKRigDefinition* InRig = Cast<UTLLRN_IKRigDefinition>(Object);
	if (!InRig)
	{
		return nullptr;
	}

	return InRig->GetPreviewMesh();
}

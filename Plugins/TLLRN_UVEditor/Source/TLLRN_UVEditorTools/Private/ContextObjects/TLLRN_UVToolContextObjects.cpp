// Copyright Epic Games, Inc. All Rights Reserved.

#include "ContextObjects/TLLRN_UVToolContextObjects.h"

#include "DynamicMesh/MeshIndexUtil.h"
#include "Engine/World.h"
#include "InputRouter.h" // Need to define this and UWorld so weak pointers know they are UThings
#include "InteractiveGizmoManager.h"
#include "InteractiveToolManager.h"
#include "ToolTargets/TLLRN_UVEditorToolMeshInput.h"
#include "UDIMUtilities.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVToolContextObjects)

using namespace UE::Geometry;

namespace TLLRN_UVToolContextObjectLocals
{
	/**
	 * A wrapper change that applies a given change to the unwrap canonical mesh of an input, and uses that
	 * to update the other views. Causes a broadcast of OnCanonicalModified.
	 */
	class  FTLLRN_UVEditorMeshChange : public FToolCommandChange
	{
	public:
		FTLLRN_UVEditorMeshChange(UTLLRN_UVEditorToolMeshInput* UVToolInputObjectIn, TUniquePtr<FDynamicMeshChange> UnwrapCanonicalMeshChangeIn)
			: UVToolInputObject(UVToolInputObjectIn)
			, UnwrapCanonicalMeshChange(MoveTemp(UnwrapCanonicalMeshChangeIn))
		{
			ensure(UVToolInputObjectIn);
			ensure(UnwrapCanonicalMeshChange);
		};

		virtual void Apply(UObject* Object) override
		{
			UnwrapCanonicalMeshChange->Apply(UVToolInputObject->UnwrapCanonical.Get(), false);
			UVToolInputObject->UpdateFromCanonicalUnwrapUsingMeshChange(*UnwrapCanonicalMeshChange);
		}

		virtual void Revert(UObject* Object) override
		{
			UnwrapCanonicalMeshChange->Apply(UVToolInputObject->UnwrapCanonical.Get(), true);
			UVToolInputObject->UpdateFromCanonicalUnwrapUsingMeshChange(*UnwrapCanonicalMeshChange);
		}

		virtual bool HasExpired(UObject* Object) const override
		{
			return !(UVToolInputObject.IsValid() && UVToolInputObject->IsValid() && UnwrapCanonicalMeshChange);
		}


		virtual FString ToString() const override
		{
			return TEXT("FTLLRN_UVEditorMeshChange");
		}

	protected:
		TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput> UVToolInputObject;
		TUniquePtr<FDynamicMeshChange> UnwrapCanonicalMeshChange;
	};
}

void UTLLRN_UVToolEmitChangeAPI::EmitToolIndependentChange(UObject* TargetObject, TUniquePtr<FToolCommandChange> Change, const FText& Description)
{
	ToolManager->GetContextTransactionsAPI()->AppendChange(TargetObject, MoveTemp(Change), Description);
}

void UTLLRN_UVToolEmitChangeAPI::EmitToolIndependentUnwrapCanonicalChange(UTLLRN_UVEditorToolMeshInput* InputObject, 
	TUniquePtr<FDynamicMeshChange> UnwrapCanonicalMeshChange, const FText& Description)
{
	ToolManager->GetContextTransactionsAPI()->AppendChange(InputObject, 
		MakeUnique<TLLRN_UVToolContextObjectLocals::FTLLRN_UVEditorMeshChange>(InputObject, MoveTemp(UnwrapCanonicalMeshChange)), Description);
}

void UTLLRN_UVToolEmitChangeAPI::EmitToolDependentChange(UObject* TargetObject, TUniquePtr<FToolCommandChange> Change, const FText& Description)
{
	// This should wrap the change in the proper wrapper that will expire it when the tool changes
	ToolManager->EmitObjectChange(TargetObject, MoveTemp(Change), Description);
}

void UTLLRN_UVToolLivePreviewAPI::Initialize(UWorld* WorldIn, UInputRouter* RouterIn,
	TUniqueFunction<void(FViewCameraState& CameraStateOut)> GetLivePreviewCameraStateFuncIn,
	TUniqueFunction<void(const FAxisAlignedBox3d& BoundingBox)> SetLivePreviewCameraToLookAtVolumeFuncIn,
	TUniqueFunction<void(const EMouseCursor::Type Cursor, bool bEnableOverride)> SetCursorOverrideFuncIn, 
	UInteractiveGizmoManager* GizmoManagerIn)
{
	World = WorldIn;
	InputRouter = RouterIn;
	GetLivePreviewCameraStateFunc = MoveTemp(GetLivePreviewCameraStateFuncIn);
	SetLivePreviewCameraToLookAtVolumeFunc = MoveTemp(SetLivePreviewCameraToLookAtVolumeFuncIn);
	SetCursorOverrideFunc = MoveTemp(SetCursorOverrideFuncIn);
	GizmoManager = GizmoManagerIn;
}

void UTLLRN_UVToolLivePreviewAPI::OnToolEnded(UInteractiveTool* DeadTool)
{
	OnDrawHUD.RemoveAll(DeadTool);
	OnRender.RemoveAll(DeadTool);
	ClearCursorOverride();
}

int32 FTLLRN_UDIMBlock::BlockU() const
{
	int32 BlockU, BlockV;
	UE::TextureUtilitiesCommon::ExtractUDIMCoordinates(UDIM, BlockU, BlockV);
	return BlockU;
}

int32 FTLLRN_UDIMBlock::BlockV() const
{
	int32 BlockU, BlockV;
	UE::TextureUtilitiesCommon::ExtractUDIMCoordinates(UDIM, BlockU, BlockV);
	return BlockV;
}

void FTLLRN_UDIMBlock::SetFromBlocks(int32 BlockU, int32 BlockV)
{
	UDIM = UE::TextureUtilitiesCommon::GetUDIMIndex(BlockU, BlockV);
}

void UTLLRN_UVToolAABBTreeStorage::Set(FDynamicMesh3* MeshKey, TSharedPtr<FDynamicMeshAABBTree3> Tree, 
	UTLLRN_UVEditorToolMeshInput* InputObject)
{
	AABBTreeStorage.Add(MeshKey, TreeInputObjectPair(Tree, InputObject));
	InputObject->OnCanonicalModified.AddWeakLambda(this, [MeshKey, Tree]
	(UTLLRN_UVEditorToolMeshInput* InputObject, const UTLLRN_UVEditorToolMeshInput::FCanonicalModifiedInfo& Info) {
		if (InputObject->UnwrapCanonical.Get() == MeshKey
			|| (Info.bAppliedMeshShapeChanged && InputObject->AppliedCanonical.Get() == MeshKey))
		{
			Tree->Build();
		}
	});
}

TSharedPtr<FDynamicMeshAABBTree3> UTLLRN_UVToolAABBTreeStorage::Get(FDynamicMesh3* MeshKey)
{
	TreeInputObjectPair* FoundPtr = AABBTreeStorage.Find(MeshKey);
	return FoundPtr ? FoundPtr->Key : nullptr;
}

void UTLLRN_UVToolAABBTreeStorage::Remove(FDynamicMesh3* MeshKey)
{
	TreeInputObjectPair* Result = AABBTreeStorage.Find(MeshKey);
	if (!Result)
	{
		return;
	}
	if (Result->Value.IsValid())
	{
		Result->Value->OnCanonicalModified.RemoveAll(this);
	}

	AABBTreeStorage.Remove(MeshKey);
}

void UTLLRN_UVToolAABBTreeStorage::RemoveByPredicate(TUniqueFunction<
	bool(const FDynamicMesh3* Mesh, TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput> InputObject,
		TSharedPtr<FDynamicMeshAABBTree3> Tree)> Predicate)
{
	for (auto It = AABBTreeStorage.CreateIterator(); It; ++It)
	{
		TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput> InputObject = It->Value.Value;
		if (Predicate(It->Key, InputObject, It->Value.Key)) // mesh, object, tree
		{
			if (InputObject.IsValid())
			{
				InputObject->OnCanonicalModified.RemoveAll(this);
			}
			It.RemoveCurrent();
		}
	}
}

void UTLLRN_UVToolAABBTreeStorage::Empty()
{
	for (TPair<FDynamicMesh3*, TreeInputObjectPair>& MapPair : AABBTreeStorage)
	{
		TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput> InputObject = MapPair.Value.Value;
		if (InputObject.IsValid())
		{
			InputObject->OnCanonicalModified.RemoveAll(this);
		}
	}
	AABBTreeStorage.Empty();
}

void UTLLRN_UVToolAABBTreeStorage::Shutdown()
{
	Empty();
}

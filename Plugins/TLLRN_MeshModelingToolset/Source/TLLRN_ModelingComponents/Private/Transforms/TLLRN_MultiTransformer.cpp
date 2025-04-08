// Copyright Epic Games, Inc. All Rights Reserved.

#include "Transforms/TLLRN_MultiTransformer.h"
#include "BaseGizmos/TransformGizmoUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_MultiTransformer)

using namespace UE::Geometry;


void UTLLRN_MultiTransformer::Setup(UInteractiveGizmoManager* GizmoManagerIn, IToolContextTransactionProvider* TransactionProviderIn)
{
	GizmoManager = GizmoManagerIn;
	TransactionProvider = TransactionProviderIn;

	ActiveGizmoFrame = FFrame3d();
	ActiveGizmoScale = FVector3d::One();

	ActiveMode = ETLLRN_MultiTransformerMode::DefaultGizmo;

	// Create a new TransformGizmo and associated TransformProxy. The TransformProxy will not be the
	// parent of any Components in this case, we just use it's transform and change delegate.
	TransformProxy = NewObject<UTransformProxy>(this);
	TransformProxy->SetTransform(ActiveGizmoFrame.ToFTransform());
	UpdateShowGizmoState(true);

	// listen for changes to the proxy and update the transform frame when that happens
	TransformProxy->OnTransformChanged.AddUObject(this, &UTLLRN_MultiTransformer::OnProxyTransformChanged);
	TransformProxy->OnBeginTransformEdit.AddUObject(this, &UTLLRN_MultiTransformer::OnBeginProxyTransformEdit);
	TransformProxy->OnEndTransformEdit.AddUObject(this, &UTLLRN_MultiTransformer::OnEndProxyTransformEdit);
	TransformProxy->OnPivotChanged.AddWeakLambda(this, [this](UTransformProxy* Proxy, FTransform Transform) {
		ActiveGizmoFrame = FFrame3d(Transform);
		ActiveGizmoScale = FVector3d(Transform.GetScale3D());
	});
	TransformProxy->OnEndPivotEdit.AddWeakLambda(this, [this](UTransformProxy* Proxy) {
		OnEndPivotEdit.Broadcast();
	});
}



void UTLLRN_MultiTransformer::Shutdown()
{
	GizmoManager->DestroyAllGizmosByOwner(this);
}


void UTLLRN_MultiTransformer::SetOverrideGizmoCoordinateSystem(EToolContextCoordinateSystem CoordSystem)
{
	if (GizmoCoordSystem != CoordSystem || bForceGizmoCoordSystem == false)
	{
		bForceGizmoCoordSystem = true;
		GizmoCoordSystem = CoordSystem;
		if (TransformGizmo != nullptr)
		{
			UpdateShowGizmoState(false);
			UpdateShowGizmoState(true);
		}
	}
}

void UTLLRN_MultiTransformer::SetEnabledGizmoSubElements(ETransformGizmoSubElements EnabledSubElements)
{
	if (ActiveGizmoSubElements != EnabledSubElements)
	{
		ActiveGizmoSubElements = EnabledSubElements;
		if (TransformGizmo != nullptr)
		{
			UpdateShowGizmoState(false);
			UpdateShowGizmoState(true);
		}
	}
}

void UTLLRN_MultiTransformer::SetMode(ETLLRN_MultiTransformerMode NewMode)
{
	if (NewMode != ActiveMode)
	{
		if (NewMode == ETLLRN_MultiTransformerMode::DefaultGizmo)
		{
			UpdateShowGizmoState(true);
		}
		else
		{
			UpdateShowGizmoState(false);
		}
		ActiveMode = NewMode;
	}
}


void UTLLRN_MultiTransformer::SetGizmoVisibility(bool bVisible)
{
	if (bShouldBeVisible != bVisible)
	{
		bShouldBeVisible = bVisible;
		if (TransformGizmo != nullptr)
		{
			TransformGizmo->SetVisibility(bVisible);
		}
	}
}

void UTLLRN_MultiTransformer::SetGizmoRepositionable(bool bOn)
{
	if (bRepositionableGizmo != bOn)
	{
		bRepositionableGizmo = bOn;
		if (TransformGizmo)
		{
			UpdateShowGizmoState(false);
			UpdateShowGizmoState(true);
		}
	}
}

EToolContextCoordinateSystem UTLLRN_MultiTransformer::GetGizmoCoordinateSystem()
{ 
	return TransformGizmo ? TransformGizmo->CurrentCoordinateSystem : GizmoCoordSystem; 
}


void UTLLRN_MultiTransformer::SetSnapToWorldGridSourceFunc(TUniqueFunction<bool()> EnableSnapFunc)
{
	EnableSnapToWorldGridFunc = MoveTemp(EnableSnapFunc);
}

void UTLLRN_MultiTransformer::SetIsNonUniformScaleAllowedFunction(TFunction<bool()> IsNonUniformScaleAllowedIn)
{
	IsNonUniformScaleAllowed = IsNonUniformScaleAllowedIn;
	if (TransformGizmo != nullptr)
	{
		TransformGizmo->SetIsNonUniformScaleAllowedFunction(IsNonUniformScaleAllowed);
	}
}

void UTLLRN_MultiTransformer::SetDisallowNegativeScaling(bool bDisallow)
{
	bDisallowNegativeScaling = bDisallow;
	if (TransformGizmo != nullptr)
	{
		TransformGizmo->SetDisallowNegativeScaling(bDisallowNegativeScaling);
	}
}

void UTLLRN_MultiTransformer::AddAlignmentMechanic(UTLLRN_DragAlignmentMechanic* AlignmentMechanic)
{
	TLLRN_DragAlignmentMechanic = AlignmentMechanic;
	if (TransformGizmo)
	{
		TLLRN_DragAlignmentMechanic->AddToGizmo(TransformGizmo);
	}
}

void UTLLRN_MultiTransformer::Tick(float DeltaTime)
{
	if (TransformGizmo != nullptr)
	{
		// todo this
		TransformGizmo->bSnapToWorldGrid =
			(EnableSnapToWorldGridFunc) ? EnableSnapToWorldGridFunc() : false;
	}
}


void UTLLRN_MultiTransformer::InitializeGizmoPositionFromWorldFrame(const FFrame3d& Frame, bool bResetScale)
{
	ActiveGizmoFrame = Frame;
	if (bResetScale)
	{
		ActiveGizmoScale = FVector3d::One();
	}

	if (TransformGizmo != nullptr)
	{
		// this resets the child scale to one
		TransformGizmo->ReinitializeGizmoTransform(ActiveGizmoFrame.ToFTransform());
	}
}



void UTLLRN_MultiTransformer::UpdateGizmoPositionFromWorldFrame(const FFrame3d& Frame, bool bResetScale)
{
	ActiveGizmoFrame = Frame;
	if (bResetScale)
	{
		ActiveGizmoScale = FVector3d::One();
	}

	if (TransformGizmo != nullptr)
	{
		// this resets the child scale to one
		TransformGizmo->SetNewGizmoTransform(ActiveGizmoFrame.ToFTransform());
	}
}


void UTLLRN_MultiTransformer::ResetScale()
{
	ActiveGizmoScale = FVector3d::One();
	if (TransformGizmo != nullptr)
	{
		TransformGizmo->SetNewChildScale(FVector::OneVector);
	}
}


void UTLLRN_MultiTransformer::OnProxyTransformChanged(UTransformProxy* Proxy, FTransform Transform)
{
	ActiveGizmoFrame = FFrame3d(Transform);
	ActiveGizmoScale = FVector3d(Transform.GetScale3D());
	OnTransformUpdated.Broadcast();
}


void UTLLRN_MultiTransformer::OnBeginProxyTransformEdit(UTransformProxy* Proxy)
{
	bInGizmoEdit = true;
	OnTransformStarted.Broadcast();
}

void UTLLRN_MultiTransformer::OnEndProxyTransformEdit(UTransformProxy* Proxy)
{
	bInGizmoEdit = false;
	OnTransformCompleted.Broadcast();
}



void UTLLRN_MultiTransformer::UpdateShowGizmoState(bool bNewVisibility)
{
	if (bNewVisibility == false)
	{
		GizmoManager->DestroyAllGizmosByOwner(this);
		TransformGizmo = nullptr;
	}
	else
	{
		check(TransformGizmo == nullptr);
		TransformGizmo = bRepositionableGizmo ? UE::TransformGizmoUtil::CreateCustomRepositionableTransformGizmo(GizmoManager, ActiveGizmoSubElements, this)
			: UE::TransformGizmoUtil::CreateCustomTransformGizmo(GizmoManager, ActiveGizmoSubElements, this);
		if (bForceGizmoCoordSystem)
		{
			TransformGizmo->bUseContextCoordinateSystem = false;
			TransformGizmo->CurrentCoordinateSystem = GizmoCoordSystem;
		}
		if (IsNonUniformScaleAllowed)
		{
			TransformGizmo->SetIsNonUniformScaleAllowedFunction(IsNonUniformScaleAllowed);
		}
		TransformGizmo->SetDisallowNegativeScaling(bDisallowNegativeScaling);
		if (TLLRN_DragAlignmentMechanic)
		{
			TLLRN_DragAlignmentMechanic->AddToGizmo(TransformGizmo);
		}
		TransformGizmo->SetActiveTarget(TransformProxy, TransactionProvider);
		TransformGizmo->ReinitializeGizmoTransform(ActiveGizmoFrame.ToFTransform());
		TransformGizmo->SetVisibility(bShouldBeVisible);
	}
}


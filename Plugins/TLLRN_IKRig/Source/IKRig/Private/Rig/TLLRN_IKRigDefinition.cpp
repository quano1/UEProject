// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rig/TLLRN_IKRigDefinition.h"

#include "TLLRN_IKRigObjectVersion.h"
#include "Engine/SkeletalMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRigDefinition)

#if WITH_EDITOR
#include "HAL/PlatformApplicationMisc.h"
#endif

#define LOCTEXT_NAMESPACE "TLLRN_IKRigDefinition"

#if WITH_EDITOR
const FName UTLLRN_IKRigDefinition::GetPreviewMeshPropertyName() { return GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_IKRigDefinition, PreviewSkeletalMesh); };

TOptional<FTransform::FReal> UTLLRN_IKRigEffectorGoal::GetNumericValue(
	ESlateTransformComponent::Type Component,
	ESlateRotationRepresentation::Type Representation,
	ESlateTransformSubComponent::Type SubComponent,
	ETLLRN_IKRigTransformType::Type TransformType) const
{
	switch(TransformType)
	{
		case ETLLRN_IKRigTransformType::Current:
		{
			return SAdvancedTransformInputBox<FTransform>::GetNumericValueFromTransform(
				CurrentTransform,
				Component,
				Representation,
				SubComponent);
		}
		case ETLLRN_IKRigTransformType::Reference:
		{
			return SAdvancedTransformInputBox<FTransform>::GetNumericValueFromTransform(
				InitialTransform,
				Component,
				Representation,
				SubComponent);
		}
		default:
		{
			break;
		}
	}
	return TOptional<FTransform::FReal>();
}

TTuple<FTransform, FTransform> UTLLRN_IKRigEffectorGoal::PrepareNumericValueChanged( ESlateTransformComponent::Type Component,
																			ESlateRotationRepresentation::Type Representation,
																			ESlateTransformSubComponent::Type SubComponent,
																			FTransform::FReal Value,
																			ETLLRN_IKRigTransformType::Type TransformType) const
{
	const FTransform& InTransform = TransformType == ETLLRN_IKRigTransformType::Current ? CurrentTransform : InitialTransform;
	FTransform OutTransform = InTransform;
	SAdvancedTransformInputBox<FTransform>::ApplyNumericValueChange(OutTransform, Value, Component, Representation, SubComponent);
	return MakeTuple(InTransform, OutTransform);
}

void UTLLRN_IKRigEffectorGoal::SetTransform(const FTransform& InTransform, ETLLRN_IKRigTransformType::Type InTransformType)
{
	// we assume that InTransform is not equal to the one it's being assigned to
	Modify();

	FTransform& TransformChanged = InTransformType == ETLLRN_IKRigTransformType::Current ? CurrentTransform : InitialTransform;
	TransformChanged = InTransform;
}

namespace
{
	
template<typename DataType>
void GetContentFromData(const DataType& InData, FString& Content)
{
	TBaseStructure<DataType>::Get()->ExportText(Content, &InData, &InData, nullptr, PPF_None, nullptr);
}

class FTLLRN_IKRigEffectorGoalErrorPipe : public FOutputDevice
{
public:
	int32 NumErrors;

	FTLLRN_IKRigEffectorGoalErrorPipe()
		: FOutputDevice()
		, NumErrors(0)
	{}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
	{
		NumErrors++;
	}
};

template<typename DataType>
bool GetDataFromContent(const FString& Content, DataType& OutData)
{
	FTLLRN_IKRigEffectorGoalErrorPipe ErrorPipe;
	static UScriptStruct* DataStruct = TBaseStructure<DataType>::Get();
	DataStruct->ImportText(*Content, &OutData, nullptr, PPF_None, &ErrorPipe, DataStruct->GetName(), true);
	return (ErrorPipe.NumErrors == 0);
}
	
}

void UTLLRN_IKRigEffectorGoal::OnCopyToClipboard(ESlateTransformComponent::Type Component, ETLLRN_IKRigTransformType::Type TransformType) const
{
	const FTransform& Xfo = TransformType == ETLLRN_IKRigTransformType::Current ? CurrentTransform : InitialTransform;

	FString Content;
	switch(Component)
	{
	case ESlateTransformComponent::Location:
		{
			GetContentFromData(Xfo.GetLocation(), Content);
			break;
		}
	case ESlateTransformComponent::Rotation:
		{
			GetContentFromData(Xfo.Rotator(), Content);
			break;
		}
	case ESlateTransformComponent::Scale:
		{
			GetContentFromData(Xfo.GetScale3D(), Content);
			break;
		}
	case ESlateTransformComponent::Max:
	default:
		{
			GetContentFromData(Xfo, Content);
			break;
		}
	}

	if (!Content.IsEmpty())
	{
		FPlatformApplicationMisc::ClipboardCopy(*Content);
	}
}

void UTLLRN_IKRigEffectorGoal::OnPasteFromClipboard(ESlateTransformComponent::Type Component, ETLLRN_IKRigTransformType::Type TransformType)
{
	FString Content;
	FPlatformApplicationMisc::ClipboardPaste(Content);

	if (Content.IsEmpty())
	{
		return;
	}

	FTransform& Xfo = TransformType == ETLLRN_IKRigTransformType::Current ? CurrentTransform : InitialTransform;
	Modify();
	
	switch(Component)
	{
		case ESlateTransformComponent::Location:
		{
			FVector Data = Xfo.GetLocation();
			if (GetDataFromContent(Content, Data))
			{
				Xfo.SetLocation(Data);
			}
			break;
		}
		case ESlateTransformComponent::Rotation:
		{
			FRotator Data = Xfo.Rotator();
			if (GetDataFromContent(Content, Data))
			{
				Xfo.SetRotation(FQuat(Data));
			}
			break;
		}
		case ESlateTransformComponent::Scale:
		{
			FVector Data = Xfo.GetScale3D();
			if (GetDataFromContent(Content, Data))
			{
				Xfo.SetScale3D(Data);
			}
			break;
		}
		case ESlateTransformComponent::Max:
		default:
		{
			FTransform Data = Xfo;
			if (GetDataFromContent(Content, Data))
			{
				Xfo = Data;
			}
			break;
		}
	}
}

bool UTLLRN_IKRigEffectorGoal::TransformDiffersFromDefault(
	ESlateTransformComponent::Type Component,
	TSharedPtr<IPropertyHandle> PropertyHandle) const
{
	if(PropertyHandle->GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_IKRigEffectorGoal, CurrentTransform))
	{
		switch(Component)
		{
			case ESlateTransformComponent::Location:
			{
				return !(CurrentTransform.GetLocation() - InitialTransform.GetLocation()).IsNearlyZero();
			}
			case ESlateTransformComponent::Rotation:
			{
				return !(CurrentTransform.Rotator() - InitialTransform.Rotator()).IsNearlyZero();
			}
			case ESlateTransformComponent::Scale:
			{
				return !(CurrentTransform.GetScale3D() - InitialTransform.GetScale3D()).IsNearlyZero();
			}
			default:
			{
				return false;
			}
		}
	}
	return false;
}

void UTLLRN_IKRigEffectorGoal::ResetTransformToDefault(
	ESlateTransformComponent::Type Component,
	TSharedPtr<IPropertyHandle> PropertyHandle)
{
	switch(Component)
	{
		case ESlateTransformComponent::Location:
		{
			CurrentTransform.SetLocation(InitialTransform.GetLocation());
			break;
		}
		case ESlateTransformComponent::Rotation:
		{
			CurrentTransform.SetRotation(InitialTransform.GetRotation());
			break;
		}
		case ESlateTransformComponent::Scale:
		{
			CurrentTransform.SetScale3D(InitialTransform.GetScale3D());
			break;
		}
		default:
		{
			break;
		}
	}
}

#endif

void FTLLRN_RetargetDefinition::AddBoneChain(
	const FName ChainName,
	const FName StartBone,
	const FName EndBone,
	const FName GoalName)
{
	if (FTLLRN_BoneChain* Chain = GetEditableBoneChainByName(ChainName))
	{
		Chain->StartBone = StartBone;
		Chain->EndBone = EndBone;
		Chain->IKGoalName = GoalName;
	}
	else
	{
		BoneChains.Emplace(ChainName, StartBone, EndBone, GoalName);
	}
}

FTLLRN_BoneChain* FTLLRN_RetargetDefinition::GetEditableBoneChainByName(FName ChainName)
{
	for (FTLLRN_BoneChain& Chain : BoneChains)
	{
		if (Chain.ChainName == ChainName)
		{
			return &Chain;
		}
	}
	
	return nullptr;
}

void UTLLRN_IKRigDefinition::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
#if WITH_EDITORONLY_DATA
	Controller = nullptr;
#endif
}

void UTLLRN_IKRigDefinition::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FTLLRN_IKRigObjectVersion::GUID);
}

void UTLLRN_IKRigDefinition::PostLoad()
{
	Super::PostLoad();
	
	// very early versions of the asset may not have been set as standalone
	SetFlags(RF_Standalone);
}

const FTLLRN_BoneChain* UTLLRN_IKRigDefinition::GetRetargetChainByName(FName ChainName) const
{
	for (const FTLLRN_BoneChain& Chain : RetargetDefinition.BoneChains)
	{
		if (Chain.ChainName == ChainName)
		{
			return &Chain;
		}
	}
	
	return nullptr;
}

/** IInterface_PreviewMeshProvider interface */
void UTLLRN_IKRigDefinition::SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty)
{
	PreviewSkeletalMesh = PreviewMesh;
}

USkeletalMesh* UTLLRN_IKRigDefinition::GetPreviewMesh() const
{
	return PreviewSkeletalMesh.LoadSynchronous();
}
/** END IInterface_PreviewMeshProvider interface */

#undef LOCTEXT_NAMESPACE


// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_RigHierarchyElements.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Rigs/TLLRN_TLLRN_RigControlHierarchy.h"
#include "RigVMCore/RigVMExecuteContext.h"
#include "TLLRN_ControlRigObjectVersion.h"
#include "TLLRN_ControlRigGizmoLibrary.h"
#include "AnimationCoreLibrary.h"
#include "Algo/Transform.h"

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigBaseElement
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigBaseElement::ElementTypeIndex = BaseElement;

FTLLRN_RigBaseElement::~FTLLRN_RigBaseElement()
{
	if (Owner)
	{
		Owner->RemoveAllMetadataForElement(this);
	}
}

UScriptStruct* FTLLRN_RigBaseElement::GetElementStruct() const
{
	switch(GetType())
	{
		case ETLLRN_RigElementType::Bone:
		{
			return FTLLRN_RigBoneElement::StaticStruct();
		}
		case ETLLRN_RigElementType::Null:
		{
			return FTLLRN_RigNullElement::StaticStruct();
		}
		case ETLLRN_RigElementType::Control:
		{
			return FTLLRN_RigControlElement::StaticStruct();
		}
		case ETLLRN_RigElementType::Curve:
		{
			return FTLLRN_RigCurveElement::StaticStruct();
		}
		case ETLLRN_RigElementType::Reference:
		{
			return FTLLRN_RigReferenceElement::StaticStruct();
		}
		case ETLLRN_RigElementType::Physics:
		{
			return FTLLRN_RigPhysicsElement::StaticStruct();
		}
		case ETLLRN_RigElementType::Connector:
		{
			return FTLLRN_RigConnectorElement::StaticStruct();
		}
		case ETLLRN_RigElementType::Socket:
		{
			return FTLLRN_RigSocketElement::StaticStruct();
		}
		default:
		{
			break;
		}
	}
	return FTLLRN_RigBaseElement::StaticStruct();
}

void FTLLRN_RigBaseElement::Serialize(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);

	if (Ar.IsLoading())
	{
		Load(Ar, SerializationPhase);
	}
	else
	{
		Save(Ar, SerializationPhase);
	}
}

void FTLLRN_RigBaseElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		Ar << Key;
	}
}

void FTLLRN_RigBaseElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	checkf(Owner != nullptr, TEXT("Loading should not happen on a rig element without an owner"));
	
	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		FTLLRN_RigElementKey LoadedKey;
	
		Ar << LoadedKey;

		ensure(LoadedKey.Type == Key.Type);
		Key = LoadedKey;

		ChildCacheIndex = INDEX_NONE;
		CachedNameString.Reset();

		if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::HierarchyElementMetadata &&
			Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::TLLRN_RigHierarchyStoresElementMetadata)
		{
			static const UEnum* MetadataTypeEnum = StaticEnum<ETLLRN_RigMetadataType>();

			int32 MetadataNum = 0;
			Ar << MetadataNum;

			for(int32 MetadataIndex = 0; MetadataIndex < MetadataNum; MetadataIndex++)
			{
				FName MetadataName;
				FName MetadataTypeName;
				Ar << MetadataName;
				Ar << MetadataTypeName;

				const ETLLRN_RigMetadataType MetadataType = static_cast<ETLLRN_RigMetadataType>(MetadataTypeEnum->GetValueByName(MetadataTypeName));

				FTLLRN_RigBaseMetadata* Md = Owner->GetMetadataForElement(this, MetadataName, MetadataType, false);
				Md->Serialize(Ar);
			}
		}
	}
}


FTLLRN_RigBaseMetadata* FTLLRN_RigBaseElement::GetMetadata(const FName& InName, ETLLRN_RigMetadataType InType)
{
	if (!Owner)
	{
		return nullptr;
	}
	return Owner->FindMetadataForElement(this, InName, InType);
}


const FTLLRN_RigBaseMetadata* FTLLRN_RigBaseElement::GetMetadata(const FName& InName, ETLLRN_RigMetadataType InType) const
{
	if (Owner == nullptr)
	{
		return nullptr;
	}
	return Owner->FindMetadataForElement(this, InName, InType);
}


bool FTLLRN_RigBaseElement::SetMetadata(const FName& InName, ETLLRN_RigMetadataType InType, const void* InData, int32 InSize)
{
	if (Owner)
	{
		constexpr bool bNotify = true;
		if (FTLLRN_RigBaseMetadata* Metadata = Owner->GetMetadataForElement(this, InName, InType, bNotify))
		{
			Metadata->SetValueData(InData, InSize);
			return true;
		}
	}
	return false;
}

FTLLRN_RigBaseMetadata* FTLLRN_RigBaseElement::SetupValidMetadata(const FName& InName, ETLLRN_RigMetadataType InType)
{
	if (Owner == nullptr)
	{
		return nullptr;
	}
	constexpr bool bNotify = true;
	return Owner->GetMetadataForElement(this, InName, InType, bNotify);
}


bool FTLLRN_RigBaseElement::RemoveMetadata(const FName& InName)
{
	if (Owner == nullptr)
	{
		return false;
	}
	return Owner->RemoveMetadataForElement(this, InName);
}

bool FTLLRN_RigBaseElement::RemoveAllMetadata()
{
	if (Owner == nullptr)
	{
		return false;
	}
	return Owner->RemoveAllMetadataForElement(this);
}

void FTLLRN_RigBaseElement::NotifyMetadataTagChanged(const FName& InTag, bool bAdded)
{
	if (Owner)
	{
		Owner->OnMetadataTagChanged(Key, InTag, bAdded);
	}
}


void FTLLRN_RigBaseElement::InitializeFrom(const FTLLRN_RigBaseElement* InOther)
{
	Key = InOther->Key;
	Index = InOther->Index;
	SubIndex = InOther->SubIndex;
	CreatedAtInstructionIndex = InOther->CreatedAtInstructionIndex;
	bSelected = false;
}


void FTLLRN_RigBaseElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigTransformDirtyState
////////////////////////////////////////////////////////////////////////////////

const bool& FTLLRN_RigTransformDirtyState::Get() const
{
	if(Storage)
	{
		return *Storage;
	}
	ensure(false);
	static constexpr bool bDefaultDirtyFlag = false;
	return bDefaultDirtyFlag;
}

bool& FTLLRN_RigTransformDirtyState::Get()
{
	if(Storage)
	{
		return *Storage;
	}
	ensure(false);
	static bool bDefaultDirtyFlag = false;
	return bDefaultDirtyFlag;
}

bool FTLLRN_RigTransformDirtyState::Set(bool InDirty)
{
	if(Storage)
	{
		if(*Storage != InDirty)
		{
			*Storage = InDirty;
			return true;
		}
	}
	return false;
}

FTLLRN_RigTransformDirtyState& FTLLRN_RigTransformDirtyState::operator=(const FTLLRN_RigTransformDirtyState& InOther)
{
	if(Storage)
	{
		*Storage = InOther.Get();
	}
	return *this;
}

void FTLLRN_RigTransformDirtyState::LinkStorage(const TArrayView<bool>& InStorage)
{
	if(InStorage.IsValidIndex(StorageIndex))
	{
		Storage = InStorage.GetData() + StorageIndex;
	}
}

void FTLLRN_RigTransformDirtyState::UnlinkStorage(FRigReusableElementStorage<bool>& InStorage)
{
	InStorage.Deallocate(StorageIndex, &Storage);
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigLocalAndGlobalDirtyState
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigLocalAndGlobalDirtyState& FTLLRN_RigLocalAndGlobalDirtyState::operator=(const FTLLRN_RigLocalAndGlobalDirtyState& InOther)
{
	Local = InOther.Local;
	Global = InOther.Global;
	return *this;
}

void FTLLRN_RigLocalAndGlobalDirtyState::LinkStorage(const TArrayView<bool>& InStorage)
{
	Local.LinkStorage(InStorage);
	Global.LinkStorage(InStorage);
}

void FTLLRN_RigLocalAndGlobalDirtyState::UnlinkStorage(FRigReusableElementStorage<bool>& InStorage)
{
	Local.UnlinkStorage(InStorage);
	Global.UnlinkStorage(InStorage);
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigCurrentAndInitialDirtyState
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigCurrentAndInitialDirtyState& FTLLRN_RigCurrentAndInitialDirtyState::operator=(const FTLLRN_RigCurrentAndInitialDirtyState& InOther)
{
	Current = InOther.Current;
	Initial = InOther.Initial;
	return *this;
}

void FTLLRN_RigCurrentAndInitialDirtyState::LinkStorage(const TArrayView<bool>& InStorage)
{
	Current.LinkStorage(InStorage);
	Initial.LinkStorage(InStorage);
}

void FTLLRN_RigCurrentAndInitialDirtyState::UnlinkStorage(FRigReusableElementStorage<bool>& InStorage)
{
	Current.UnlinkStorage(InStorage);
	Initial.UnlinkStorage(InStorage);
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigComputedTransform
////////////////////////////////////////////////////////////////////////////////

void FTLLRN_RigComputedTransform::Save(FArchive& Ar, const FTLLRN_RigTransformDirtyState& InDirtyState)
{
	FTransform Transform = Get();
	bool bDirty = InDirtyState.Get();
	
	Ar << Transform;
	Ar << bDirty;
}

void FTLLRN_RigComputedTransform::Load(FArchive& Ar, FTLLRN_RigTransformDirtyState& InDirtyState)
{
	FTransform Transform = FTransform::Identity;
	bool bDirty = false;

	Ar << Transform;
	Ar << bDirty;

	Set(Transform);
	(void)InDirtyState.Set(bDirty);
}

const FTransform& FTLLRN_RigComputedTransform::Get() const
{
	if(Storage)
	{
		return *Storage;
	}
	ensure(false);
	static const FTransform DefaultTransform = FTransform::Identity;
	return DefaultTransform;
}

FTLLRN_RigComputedTransform& FTLLRN_RigComputedTransform::operator=(const FTLLRN_RigComputedTransform& InOther)
{
	if(Storage)
	{
		*Storage = InOther.Get();
	}
	return *this;
}

void FTLLRN_RigComputedTransform::LinkStorage(const TArrayView<FTransform>& InStorage)
{
	if(InStorage.IsValidIndex(StorageIndex))
	{
		Storage = InStorage.GetData() + StorageIndex;
	}
}

void FTLLRN_RigComputedTransform::UnlinkStorage(FRigReusableElementStorage<FTransform>& InStorage)
{
	InStorage.Deallocate(StorageIndex, &Storage);
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigLocalAndGlobalTransform
////////////////////////////////////////////////////////////////////////////////

void FTLLRN_RigLocalAndGlobalTransform::Save(FArchive& Ar, const FTLLRN_RigLocalAndGlobalDirtyState& InDirtyState)
{
	Local.Save(Ar, InDirtyState.Local);
	Global.Save(Ar, InDirtyState.Global);
}

void FTLLRN_RigLocalAndGlobalTransform::Load(FArchive& Ar, FTLLRN_RigLocalAndGlobalDirtyState& OutDirtyState)
{
	Local.Load(Ar, OutDirtyState.Local);
	Global.Load(Ar, OutDirtyState.Global);
}

FTLLRN_RigLocalAndGlobalTransform& FTLLRN_RigLocalAndGlobalTransform::operator=(const FTLLRN_RigLocalAndGlobalTransform& InOther)
{
	Local = InOther.Local;
	Global = InOther.Global;
	return *this;
}

void FTLLRN_RigLocalAndGlobalTransform::LinkStorage(const TArrayView<FTransform>& InStorage)
{
	Local.LinkStorage(InStorage);
	Global.LinkStorage(InStorage);
}

void FTLLRN_RigLocalAndGlobalTransform::UnlinkStorage(FRigReusableElementStorage<FTransform>& InStorage)
{
	Local.UnlinkStorage(InStorage);
	Global.UnlinkStorage(InStorage);
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigCurrentAndInitialTransform
////////////////////////////////////////////////////////////////////////////////

void FTLLRN_RigCurrentAndInitialTransform::Save(FArchive& Ar, const FTLLRN_RigCurrentAndInitialDirtyState& InDirtyState)
{
	Current.Save(Ar, InDirtyState.Current);
	Initial.Save(Ar, InDirtyState.Initial);
}

void FTLLRN_RigCurrentAndInitialTransform::Load(FArchive& Ar, FTLLRN_RigCurrentAndInitialDirtyState& OutDirtyState)
{
	Current.Load(Ar, OutDirtyState.Current);
	Initial.Load(Ar, OutDirtyState.Initial);
}

FTLLRN_RigCurrentAndInitialTransform& FTLLRN_RigCurrentAndInitialTransform::operator=(const FTLLRN_RigCurrentAndInitialTransform& InOther)
{
	Current = InOther.Current;
	Initial = InOther.Initial;
	return *this;
}

void FTLLRN_RigCurrentAndInitialTransform::LinkStorage(const TArrayView<FTransform>& InStorage)
{
	Current.LinkStorage(InStorage);
	Initial.LinkStorage(InStorage);
}

void FTLLRN_RigCurrentAndInitialTransform::UnlinkStorage(FRigReusableElementStorage<FTransform>& InStorage)
{
	Current.UnlinkStorage(InStorage);
	Initial.UnlinkStorage(InStorage);
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigPreferredEulerAngles
////////////////////////////////////////////////////////////////////////////////

void FTLLRN_RigPreferredEulerAngles::Save(FArchive& Ar)
{
	static const UEnum* RotationOrderEnum = StaticEnum<EEulerRotationOrder>();
	FName RotationOrderName = RotationOrderEnum->GetNameByValue((int64)RotationOrder);
	Ar << RotationOrderName;
	Ar << Current;
	Ar << Initial;
}

void FTLLRN_RigPreferredEulerAngles::Load(FArchive& Ar)
{
	static const UEnum* RotationOrderEnum = StaticEnum<EEulerRotationOrder>();
	FName RotationOrderName;
	Ar << RotationOrderName;
	RotationOrder = (EEulerRotationOrder)RotationOrderEnum->GetValueByName(RotationOrderName);
	Ar << Current;
	Ar << Initial;
}

void FTLLRN_RigPreferredEulerAngles::Reset()
{
	RotationOrder = DefaultRotationOrder;
	Initial = Current = FVector::ZeroVector;
}

FRotator FTLLRN_RigPreferredEulerAngles::GetRotator(bool bInitial) const
{
	return FRotator::MakeFromEuler(GetAngles(bInitial, RotationOrder));
}

FRotator FTLLRN_RigPreferredEulerAngles::SetRotator(const FRotator& InValue, bool bInitial, bool bFixEulerFlips)
{
	SetAngles(InValue.Euler(), bInitial, RotationOrder, bFixEulerFlips);
	return InValue;
}

FVector FTLLRN_RigPreferredEulerAngles::GetAngles(bool bInitial, EEulerRotationOrder InRotationOrder) const
{
	if(RotationOrder == InRotationOrder)
	{
		return Get(bInitial);
	}
	return AnimationCore::ChangeEulerRotationOrder(Get(bInitial), RotationOrder, InRotationOrder);
}

void FTLLRN_RigPreferredEulerAngles::SetAngles(const FVector& InValue, bool bInitial, EEulerRotationOrder InRotationOrder, bool bFixEulerFlips)
{
	FVector Value = InValue;
	if(RotationOrder != InRotationOrder)
	{
		Value = AnimationCore::ChangeEulerRotationOrder(Value, InRotationOrder, RotationOrder);
	}

	if(bFixEulerFlips)
	{
		const FRotator CurrentRotator = FRotator::MakeFromEuler(GetAngles(bInitial, RotationOrder));
		const FRotator InRotator = FRotator::MakeFromEuler(Value);

		//Find Diff of the rotation from current and just add that instead of setting so we can go over/under -180
		FRotator CurrentWinding;
		FRotator CurrentRotRemainder;
		CurrentRotator.GetWindingAndRemainder(CurrentWinding, CurrentRotRemainder);

		FRotator DeltaRot = InRotator - CurrentRotRemainder;
		DeltaRot.Normalize();
		const FRotator FixedValue = CurrentRotator + DeltaRot;

		Get(bInitial) = FixedValue.Euler();
		return;

	}
	
	Get(bInitial) = Value;
}

void FTLLRN_RigPreferredEulerAngles::SetRotationOrder(EEulerRotationOrder InRotationOrder)
{
	if(RotationOrder != InRotationOrder)
	{
		const EEulerRotationOrder PreviousRotationOrder = RotationOrder;
		const FVector PreviousAnglesCurrent = GetAngles(false, RotationOrder);
		const FVector PreviousAnglesInitial = GetAngles(true, RotationOrder);
		RotationOrder = InRotationOrder;
		SetAngles(PreviousAnglesCurrent, false, PreviousRotationOrder);
		SetAngles(PreviousAnglesInitial, true, PreviousRotationOrder);
	}
}

FRotator FTLLRN_RigPreferredEulerAngles::GetRotatorFromQuat(const FQuat& InQuat) const
{
	FVector Vector = AnimationCore::EulerFromQuat(InQuat, RotationOrder, true);
	return FRotator::MakeFromEuler(Vector);
}

FQuat FTLLRN_RigPreferredEulerAngles::GetQuatFromRotator(const FRotator& InRotator) const
{
	FVector Vector = InRotator.Euler();
	return AnimationCore::QuatFromEuler(Vector, RotationOrder, true);
}


////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigElementHandle
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigElementHandle::FTLLRN_RigElementHandle(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InKey)
: Hierarchy(InHierarchy)
, Key(InKey)
{
}

FTLLRN_RigElementHandle::FTLLRN_RigElementHandle(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
: Hierarchy(InHierarchy)
, Key(InElement->GetKey())
{
}

const FTLLRN_RigBaseElement* FTLLRN_RigElementHandle::Get() const
{
	if(Hierarchy.IsValid())
	{
		return Hierarchy->Find(Key);
	}
	return nullptr;
}

FTLLRN_RigBaseElement* FTLLRN_RigElementHandle::Get()
{
	if(Hierarchy.IsValid())
	{
		return Hierarchy->Find(Key);
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigTransformElement
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigTransformElement::ElementTypeIndex = TransformElement;

void FTLLRN_RigTransformElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Save(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		GetTransform().Save(Ar, GetDirtyState());
	}
}

void FTLLRN_RigTransformElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Load(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		GetTransform().Load(Ar, GetDirtyState());
	}
}

void FTLLRN_RigTransformElement::CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights)
{
	Super::CopyPose(InOther, bCurrent, bInitial, bWeights);

	if(FTLLRN_RigTransformElement* Other = Cast<FTLLRN_RigTransformElement>(InOther))
	{
		if(bCurrent)
		{
			GetTransform().Current = Other->GetTransform().Current;
			GetDirtyState().Current = Other->GetDirtyState().Current; 
		}
		if(bInitial)
		{
			GetTransform().Initial = Other->GetTransform().Initial;
			GetDirtyState().Initial = Other->GetDirtyState().Initial; 
		}
	}
}

const FTLLRN_RigCurrentAndInitialTransform& FTLLRN_RigTransformElement::GetTransform() const
{
	return PoseStorage;
}

FTLLRN_RigCurrentAndInitialTransform& FTLLRN_RigTransformElement::GetTransform()
{
	return PoseStorage;
}

const FTLLRN_RigCurrentAndInitialDirtyState& FTLLRN_RigTransformElement::GetDirtyState() const
{
	return PoseDirtyState;
}

FTLLRN_RigCurrentAndInitialDirtyState& FTLLRN_RigTransformElement::GetDirtyState()
{
	return PoseDirtyState;
}

void FTLLRN_RigTransformElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
	Super::CopyFrom(InOther);
	
	const FTLLRN_RigTransformElement* SourceTransform = CastChecked<FTLLRN_RigTransformElement>(InOther);
	GetTransform() = SourceTransform->GetTransform();
	GetDirtyState() = SourceTransform->GetDirtyState();

	ElementsToDirty.Reset();
	ElementsToDirty.Reserve(SourceTransform->ElementsToDirty.Num());
	
	for(int32 ElementToDirtyIndex = 0; ElementToDirtyIndex < SourceTransform->ElementsToDirty.Num(); ElementToDirtyIndex++)
	{
		const FElementToDirty& Source = SourceTransform->ElementsToDirty[ElementToDirtyIndex];
		FTLLRN_RigTransformElement* TargetTransform = CastChecked<FTLLRN_RigTransformElement>(Owner->Get(Source.Element->Index));
		const FElementToDirty Target(TargetTransform, Source.HierarchyDistance);
		ElementsToDirty.Add(Target);
		check(ElementsToDirty[ElementToDirtyIndex].Element->GetKey() == Source.Element->GetKey());
	}
}

void FTLLRN_RigTransformElement::LinkStorage(const TArrayView<FTransform>& InTransforms, const TArrayView<bool>& InDirtyStates,
	const TArrayView<float>& InCurves)
{
	FTLLRN_RigBaseElement::LinkStorage(InTransforms, InDirtyStates, InCurves);
	PoseStorage.LinkStorage(InTransforms);
	PoseDirtyState.LinkStorage(InDirtyStates);
}

void FTLLRN_RigTransformElement::UnlinkStorage(FRigReusableElementStorage<FTransform>& InTransforms,
	FRigReusableElementStorage<bool>& InDirtyStates, FRigReusableElementStorage<float>& InCurves)
{
	FTLLRN_RigBaseElement::UnlinkStorage(InTransforms, InDirtyStates, InCurves);
	PoseStorage.UnlinkStorage(InTransforms);
	PoseDirtyState.UnlinkStorage(InDirtyStates);
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigSingleParentElement
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigSingleParentElement::ElementTypeIndex = SingleParentElement;

void FTLLRN_RigSingleParentElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Save(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::InterElementData)
	{
		FTLLRN_RigElementKey ParentKey;
		if(ParentElement)
		{
			ParentKey = ParentElement->GetKey();
		}
		Ar << ParentKey;
	}
}

void FTLLRN_RigSingleParentElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Load(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::InterElementData)
	{
		FTLLRN_RigElementKey ParentKey;
		Ar << ParentKey;

		if(ParentKey.IsValid())
		{
			ParentElement = Owner->FindChecked<FTLLRN_RigTransformElement>(ParentKey);
		}
	}
}

void FTLLRN_RigSingleParentElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
	Super::CopyFrom(InOther);

	const FTLLRN_RigSingleParentElement* Source = CastChecked<FTLLRN_RigSingleParentElement>(InOther); 
	if(Source->ParentElement)
	{
		ParentElement = CastChecked<FTLLRN_RigTransformElement>(Owner->Get(Source->ParentElement->GetIndex()));
		check(ParentElement->GetKey() == Source->ParentElement->GetKey());
	}
	else
	{
		ParentElement = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigMultiParentElement
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigMultiParentElement::ElementTypeIndex = MultiParentElement;

void FTLLRN_RigMultiParentElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Save(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		int32 NumParents = ParentConstraints.Num();
		Ar << NumParents;
	}
	else if(SerializationPhase == ESerializationPhase::InterElementData)
	{
		for(int32 ParentIndex = 0; ParentIndex < ParentConstraints.Num(); ParentIndex++)
		{
			FTLLRN_RigElementKey ParentKey;
			if(ParentConstraints[ParentIndex].ParentElement)
			{
				ParentKey = ParentConstraints[ParentIndex].ParentElement->GetKey();
			}

			Ar << ParentKey;
			Ar << ParentConstraints[ParentIndex].InitialWeight;
			Ar << ParentConstraints[ParentIndex].Weight;
		}
	}
}

void FTLLRN_RigMultiParentElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Load(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::RemovedMultiParentParentCache)
		{
			FTLLRN_RigCurrentAndInitialTransform Parent;
			FTLLRN_RigCurrentAndInitialDirtyState DirtyState;
			Parent.Load(Ar, DirtyState);
		}

		int32 NumParents = 0;
		Ar << NumParents;

		ParentConstraints.SetNum(NumParents);
	}
	else if(SerializationPhase == ESerializationPhase::InterElementData)
	{
		for(int32 ParentIndex = 0; ParentIndex < ParentConstraints.Num(); ParentIndex++)
		{
			FTLLRN_RigElementKey ParentKey;
			Ar << ParentKey;
			ensure(ParentKey.IsValid());

			ParentConstraints[ParentIndex].ParentElement = Owner->FindChecked<FTLLRN_RigTransformElement>(ParentKey);
			ParentConstraints[ParentIndex].bCacheIsDirty = true;

			if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::TLLRN_RigHierarchyMultiParentConstraints)
			{
				Ar << ParentConstraints[ParentIndex].InitialWeight;
				Ar << ParentConstraints[ParentIndex].Weight;
			}
			else
			{
				float InitialWeight = 0.f;
				Ar << InitialWeight;
				ParentConstraints[ParentIndex].InitialWeight = FTLLRN_RigElementWeight(InitialWeight);

				float Weight = 0.f;
				Ar << Weight;
				ParentConstraints[ParentIndex].Weight = FTLLRN_RigElementWeight(Weight);
			}

			IndexLookup.Add(ParentKey, ParentIndex);
		}
	}
}

void FTLLRN_RigMultiParentElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
	Super::CopyFrom(InOther);
	
	const FTLLRN_RigMultiParentElement* Source = CastChecked<FTLLRN_RigMultiParentElement>(InOther);
	ParentConstraints.Reset();
	ParentConstraints.Reserve(Source->ParentConstraints.Num());
	IndexLookup.Reset();
	IndexLookup.Reserve(Source->IndexLookup.Num());

	for(int32 ParentIndex = 0; ParentIndex < Source->ParentConstraints.Num(); ParentIndex++)
	{
		FTLLRN_RigElementParentConstraint ParentConstraint = Source->ParentConstraints[ParentIndex];
		const FTLLRN_RigTransformElement* SourceParentElement = ParentConstraint.ParentElement;
		ParentConstraint.ParentElement = CastChecked<FTLLRN_RigTransformElement>(Owner->Get(SourceParentElement->GetIndex()));
		ParentConstraints.Add(ParentConstraint);
		check(ParentConstraints[ParentIndex].ParentElement->GetKey() == SourceParentElement->GetKey());
		IndexLookup.Add(ParentConstraint.ParentElement->GetKey(), ParentIndex);
	}
}

void FTLLRN_RigMultiParentElement::CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights)
{
	Super::CopyPose(InOther, bCurrent, bInitial, bWeights);

	if(bWeights)
	{
		FTLLRN_RigMultiParentElement* Source = Cast<FTLLRN_RigMultiParentElement>(InOther);
		if(ensure(Source))
		{
			// Find the map between constraint indices
			TMap<int32, int32> ConstraintIndexToSourceConstraintIndex;
			for(int32 ConstraintIndex = 0; ConstraintIndex < ParentConstraints.Num(); ConstraintIndex++)
			{
				const FTLLRN_RigElementParentConstraint& ParentConstraint = ParentConstraints[ConstraintIndex];
				int32 SourceConstraintIndex = Source->ParentConstraints.IndexOfByPredicate([ParentConstraint](const FTLLRN_RigElementParentConstraint& Constraint)
				{
					return Constraint.ParentElement->GetKey() == ParentConstraint.ParentElement->GetKey();
				});
				if (SourceConstraintIndex != INDEX_NONE)
				{
					ConstraintIndexToSourceConstraintIndex.Add(ConstraintIndex, SourceConstraintIndex);
				}
			}
			
			for(int32 ParentIndex = 0; ParentIndex < ParentConstraints.Num(); ParentIndex++)
			{
				if (int32* SourceConstraintIndex = ConstraintIndexToSourceConstraintIndex.Find(ParentIndex))
				{
					ParentConstraints[ParentIndex].CopyPose(Source->ParentConstraints[*SourceConstraintIndex], bCurrent, bInitial);
				}
				else
				{
					// Otherwise, reset the weights to 0
					if (bCurrent)
					{
						ParentConstraints[ParentIndex].Weight = 0.f;
					}
					if (bInitial)
					{
						ParentConstraints[ParentIndex].InitialWeight = 0.f;
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigBoneElement
////////////////////////////////////////////////////////////////////////////////
#if !UE_DETECT_DELEGATES_RACE_CONDITIONS // race detector increases mem footprint but is compiled out from test/shipping builds
static_assert(sizeof(FTLLRN_RigBoneElement) <= 736, "FTLLRN_RigBoneElement was optimized to fit into 736 bytes bin of MallocBinned3");
#endif

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigBoneElement::ElementTypeIndex = BoneElement;

void FTLLRN_RigBoneElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Save(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		static const UEnum* BoneTypeEnum = StaticEnum<ETLLRN_RigBoneType>();
		FName TypeName = BoneTypeEnum->GetNameByValue((int64)BoneType);
		Ar << TypeName;
	}
}

void FTLLRN_RigBoneElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Load(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		static const UEnum* BoneTypeEnum = StaticEnum<ETLLRN_RigBoneType>();
		FName TypeName;
		Ar << TypeName;
		BoneType = (ETLLRN_RigBoneType)BoneTypeEnum->GetValueByName(TypeName);
	}
}

void FTLLRN_RigBoneElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
	Super::CopyFrom(InOther);
	
	const FTLLRN_RigBoneElement* Source = CastChecked<FTLLRN_RigBoneElement>(InOther);
	BoneType = Source->BoneType;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigNullElement
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigNullElement::ElementTypeIndex = NullElement;

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigControlSettings
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigControlSettings::FTLLRN_RigControlSettings()
: AnimationType(ETLLRN_RigControlAnimationType::AnimationControl)
, ControlType(ETLLRN_RigControlType::EulerTransform)
, DisplayName(NAME_None)
, PrimaryAxis(ETLLRN_RigControlAxis::X)
, bIsCurve(false)
, LimitEnabled()
, bDrawLimits(true)
, MinimumValue()
, MaximumValue()
, bShapeVisible(true)
, ShapeVisibility(ETLLRN_RigControlVisibility::UserDefined)
, ShapeName(NAME_None)
, ShapeColor(FLinearColor::Red)
, bIsTransientControl(false)
, ControlEnum(nullptr)
, Customization()
, bGroupWithParentControl(false)
, bRestrictSpaceSwitching(false) 
, PreferredRotationOrder(FTLLRN_RigPreferredEulerAngles::DefaultRotationOrder)
, bUsePreferredRotationOrder(false) 
{
	// rely on the default provided by the shape definition
	ShapeName = FTLLRN_ControlRigShapeDefinition().ShapeName; 
}

void FTLLRN_RigControlSettings::Save(FArchive& Ar)
{
	Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);

	static const UEnum* AnimationTypeEnum = StaticEnum<ETLLRN_RigControlAnimationType>();
	static const UEnum* ControlTypeEnum = StaticEnum<ETLLRN_RigControlType>();
	static const UEnum* ShapeVisibilityEnum = StaticEnum<ETLLRN_RigControlVisibility>();
	static const UEnum* ControlAxisEnum = StaticEnum<ETLLRN_RigControlAxis>();

	FName AnimationTypeName = AnimationTypeEnum->GetNameByValue((int64)AnimationType);
	FName ControlTypeName = ControlTypeEnum->GetNameByValue((int64)ControlType);
	FName ShapeVisibilityName = ShapeVisibilityEnum->GetNameByValue((int64)ShapeVisibility);
	FName PrimaryAxisName = ControlAxisEnum->GetNameByValue((int64)PrimaryAxis);

	FString ControlEnumPathName;
	if(ControlEnum)
	{
		ControlEnumPathName = ControlEnum->GetPathName();
		if (Ar.IsObjectReferenceCollector())
		{
			FSoftObjectPath DeclareControlEnumToCooker(ControlEnumPathName);
			Ar << DeclareControlEnumToCooker;
		}
	}

	Ar << AnimationTypeName;
	Ar << ControlTypeName;
	Ar << DisplayName;
	Ar << PrimaryAxisName;
	Ar << bIsCurve;
	Ar << LimitEnabled;
	Ar << bDrawLimits;
	Ar << MinimumValue;
	Ar << MaximumValue;
	Ar << bShapeVisible;
	Ar << ShapeVisibilityName;
	Ar << ShapeName;
	Ar << ShapeColor;
	Ar << bIsTransientControl;
	Ar << ControlEnumPathName;
	Ar << Customization.AvailableSpaces;
	Ar << DrivenControls;
	Ar << bGroupWithParentControl;
	Ar << bRestrictSpaceSwitching;
	Ar << FilteredChannels;
	Ar << PreferredRotationOrder;
	Ar << bUsePreferredRotationOrder;

}

void FTLLRN_RigControlSettings::Load(FArchive& Ar)
{
	Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);

	static const UEnum* AnimationTypeEnum = StaticEnum<ETLLRN_RigControlAnimationType>();
	static const UEnum* ControlTypeEnum = StaticEnum<ETLLRN_RigControlType>();
	static const UEnum* ShapeVisibilityEnum = StaticEnum<ETLLRN_RigControlVisibility>();
	static const UEnum* ControlAxisEnum = StaticEnum<ETLLRN_RigControlAxis>();

	FName AnimationTypeName, ControlTypeName, ShapeVisibilityName, PrimaryAxisName;
	FString ControlEnumPathName;

	bool bLimitTranslation_DEPRECATED = false;
	bool bLimitRotation_DEPRECATED = false;
	bool bLimitScale_DEPRECATED = false;
	bool bAnimatableDeprecated = false;
	bool bShapeEnabledDeprecated = false;

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::ControlAnimationType)
	{
		Ar << AnimationTypeName;
	}
	Ar << ControlTypeName;
	Ar << DisplayName;
	Ar << PrimaryAxisName;
	Ar << bIsCurve;
	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::ControlAnimationType)
	{
		Ar << bAnimatableDeprecated;
	}
	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::PerChannelLimits)
	{
		Ar << bLimitTranslation_DEPRECATED;
		Ar << bLimitRotation_DEPRECATED;
		Ar << bLimitScale_DEPRECATED;
	}
	else
	{
		Ar << LimitEnabled;
	}
	Ar << bDrawLimits;

	FTransform MinimumTransform, MaximumTransform;
	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::StorageMinMaxValuesAsFloatStorage)
	{
		Ar << MinimumValue;
		Ar << MaximumValue;
	}
	else
	{
		Ar << MinimumTransform;
		Ar << MaximumTransform;
	}

	ControlType = (ETLLRN_RigControlType)ControlTypeEnum->GetValueByName(ControlTypeName);
	
	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::ControlAnimationType)
	{
		Ar << bShapeEnabledDeprecated;
		SetAnimationTypeFromDeprecatedData(bAnimatableDeprecated, bShapeEnabledDeprecated);
		AnimationTypeName = AnimationTypeEnum->GetNameByValue((int64)AnimationType);
	}
	
	Ar << bShapeVisible;
	
	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::ControlAnimationType)
	{
		ShapeVisibilityName = ShapeVisibilityEnum->GetNameByValue((int64)ETLLRN_RigControlVisibility::UserDefined);
	}
	else
	{
		Ar << ShapeVisibilityName;
	}
	Ar << ShapeName;

	if(Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::RenameGizmoToShape)
	{
		if(ShapeName == FTLLRN_RigControl().GizmoName)
		{
			ShapeName = FTLLRN_ControlRigShapeDefinition().ShapeName; 
		}
	}
	
	Ar << ShapeColor;
	Ar << bIsTransientControl;
	Ar << ControlEnumPathName;

	AnimationType = (ETLLRN_RigControlAnimationType)AnimationTypeEnum->GetValueByName(AnimationTypeName);
	PrimaryAxis = (ETLLRN_RigControlAxis)ControlAxisEnum->GetValueByName(PrimaryAxisName);
	ShapeVisibility = (ETLLRN_RigControlVisibility)ShapeVisibilityEnum->GetValueByName(ShapeVisibilityName);

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::StorageMinMaxValuesAsFloatStorage)
	{
		MinimumValue.SetFromTransform(MinimumTransform, ControlType, PrimaryAxis);
		MaximumValue.SetFromTransform(MaximumTransform, ControlType, PrimaryAxis);
	}

	ControlEnum = nullptr;
	if(!ControlEnumPathName.IsEmpty())
	{
		if (IsInGameThread())
		{
			ControlEnum = LoadObject<UEnum>(nullptr, *ControlEnumPathName);
		}
		else
		{			
			ControlEnum = FindObject<UEnum>(nullptr, *ControlEnumPathName);
		}
	}

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::TLLRN_RigHierarchyControlSpaceFavorites)
	{
		Ar << Customization.AvailableSpaces;
	}
	else
	{
		Customization.AvailableSpaces.Reset();
	}

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::ControlAnimationType)
	{
		Ar << DrivenControls;
	}
	else
	{
		DrivenControls.Reset();
	}

	PreviouslyDrivenControls.Reset();

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::PerChannelLimits)
	{
		SetupLimitArrayForType(bLimitTranslation_DEPRECATED, bLimitRotation_DEPRECATED, bLimitScale_DEPRECATED);
	}

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::ControlAnimationType)
	{
		Ar << bGroupWithParentControl;
	}
	else
	{
		bGroupWithParentControl = IsAnimatable() && (
			ControlType == ETLLRN_RigControlType::Bool ||
			ControlType == ETLLRN_RigControlType::Float ||
			ControlType == ETLLRN_RigControlType::ScaleFloat ||
			ControlType == ETLLRN_RigControlType::Integer ||
			ControlType == ETLLRN_RigControlType::Vector2D
		);
	}

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::RestrictSpaceSwitchingForControls)
	{
		Ar << bRestrictSpaceSwitching;
	}
	else
	{
		bRestrictSpaceSwitching = false;
	}

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::ControlTransformChannelFiltering)
	{
		Ar << FilteredChannels;
	}
	else
	{
		FilteredChannels.Reset();
	}

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::TLLRN_RigHierarchyControlPreferredRotationOrder)
	{
		Ar << PreferredRotationOrder;
	}
	else
	{
		PreferredRotationOrder = FTLLRN_RigPreferredEulerAngles::DefaultRotationOrder;
	}

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::TLLRN_RigHierarchyControlPreferredRotationOrderFlag)
	{
		Ar << bUsePreferredRotationOrder;
	}
	else
	{
		bUsePreferredRotationOrder = false;
	}

}

uint32 GetTypeHash(const FTLLRN_RigControlSettings& Settings)
{
	uint32 Hash = GetTypeHash(Settings.ControlType);
	Hash = HashCombine(Hash, GetTypeHash(Settings.AnimationType));
	Hash = HashCombine(Hash, GetTypeHash(Settings.DisplayName));
	Hash = HashCombine(Hash, GetTypeHash(Settings.PrimaryAxis));
	Hash = HashCombine(Hash, GetTypeHash(Settings.bIsCurve));
	Hash = HashCombine(Hash, GetTypeHash(Settings.bDrawLimits));
	Hash = HashCombine(Hash, GetTypeHash(Settings.bShapeVisible));
	Hash = HashCombine(Hash, GetTypeHash(Settings.ShapeVisibility));
	Hash = HashCombine(Hash, GetTypeHash(Settings.ShapeName));
	Hash = HashCombine(Hash, GetTypeHash(Settings.ShapeColor));
	Hash = HashCombine(Hash, GetTypeHash(Settings.ControlEnum));
	Hash = HashCombine(Hash, GetTypeHash(Settings.DrivenControls));
	Hash = HashCombine(Hash, GetTypeHash(Settings.bGroupWithParentControl));
	Hash = HashCombine(Hash, GetTypeHash(Settings.bRestrictSpaceSwitching));
	Hash = HashCombine(Hash, GetTypeHash(Settings.FilteredChannels.Num()));
	for(const ETLLRN_RigControlTransformChannel& Channel : Settings.FilteredChannels)
	{
		Hash = HashCombine(Hash, GetTypeHash(Channel));
	}
	Hash = HashCombine(Hash, GetTypeHash(Settings.PreferredRotationOrder));
	return Hash;
}

bool FTLLRN_RigControlSettings::operator==(const FTLLRN_RigControlSettings& InOther) const
{
	if(AnimationType != InOther.AnimationType)
	{
		return false;
	}
	if(ControlType != InOther.ControlType)
	{
		return false;
	}
	if(DisplayName != InOther.DisplayName)
	{
		return false;
	}
	if(PrimaryAxis != InOther.PrimaryAxis)
	{
		return false;
	}
	if(bIsCurve != InOther.bIsCurve)
	{
		return false;
	}
	if(LimitEnabled != InOther.LimitEnabled)
	{
		return false;
	}
	if(bDrawLimits != InOther.bDrawLimits)
	{
		return false;
	}
	if(bShapeVisible != InOther.bShapeVisible)
	{
		return false;
	}
	if(ShapeVisibility != InOther.ShapeVisibility)
	{
		return false;
	}
	if(ShapeName != InOther.ShapeName)
	{
		return false;
	}
	if(bIsTransientControl != InOther.bIsTransientControl)
	{
		return false;
	}
	if(ControlEnum != InOther.ControlEnum)
	{
		return false;
	}
	if(!ShapeColor.Equals(InOther.ShapeColor, 0.001))
	{
		return false;
	}
	if(Customization.AvailableSpaces != InOther.Customization.AvailableSpaces)
	{
		return false;
	}
	if(DrivenControls != InOther.DrivenControls)
	{
		return false;
	}
	if(bGroupWithParentControl != InOther.bGroupWithParentControl)
	{
		return false;
	}
	if(bRestrictSpaceSwitching != InOther.bRestrictSpaceSwitching)
	{
		return false;
	}
	if(FilteredChannels != InOther.FilteredChannels)
	{
		return false;
	}
	if(PreferredRotationOrder != InOther.PreferredRotationOrder)
	{
		return false;
	}
	if (bUsePreferredRotationOrder != InOther.bUsePreferredRotationOrder)
	{
		return false;
	}


	const FTransform MinimumTransform = MinimumValue.GetAsTransform(ControlType, PrimaryAxis);
	const FTransform OtherMinimumTransform = InOther.MinimumValue.GetAsTransform(ControlType, PrimaryAxis);
	if(!MinimumTransform.Equals(OtherMinimumTransform, 0.001))
	{
		return false;
	}

	const FTransform MaximumTransform = MaximumValue.GetAsTransform(ControlType, PrimaryAxis);
	const FTransform OtherMaximumTransform = InOther.MaximumValue.GetAsTransform(ControlType, PrimaryAxis);
	if(!MaximumTransform.Equals(OtherMaximumTransform, 0.001))
	{
		return false;
	}

	return true;
}

void FTLLRN_RigControlSettings::SetupLimitArrayForType(bool bLimitTranslation, bool bLimitRotation, bool bLimitScale)
{
	switch(ControlType)
	{
		case ETLLRN_RigControlType::Integer:
		case ETLLRN_RigControlType::Float:
		{
			LimitEnabled.SetNum(1);
			LimitEnabled[0].Set(bLimitTranslation);
			break;
		}
		case ETLLRN_RigControlType::ScaleFloat:
		{
			LimitEnabled.SetNum(1);
			LimitEnabled[0].Set(bLimitScale);
			break;
		}
		case ETLLRN_RigControlType::Vector2D:
		{
			LimitEnabled.SetNum(2);
			LimitEnabled[0] = LimitEnabled[1].Set(bLimitTranslation);
			break;
		}
		case ETLLRN_RigControlType::Position:
		{
			LimitEnabled.SetNum(3);
			LimitEnabled[0] = LimitEnabled[1] = LimitEnabled[2].Set(bLimitTranslation);
			break;
		}
		case ETLLRN_RigControlType::Scale:
		{
			LimitEnabled.SetNum(3);
			LimitEnabled[0] = LimitEnabled[1] = LimitEnabled[2].Set(bLimitScale);
			break;
		}
		case ETLLRN_RigControlType::Rotator:
		{
			LimitEnabled.SetNum(3);
			LimitEnabled[0] = LimitEnabled[1] = LimitEnabled[2].Set(bLimitRotation);
			break;
		}
		case ETLLRN_RigControlType::TransformNoScale:
		{
			LimitEnabled.SetNum(6);
			LimitEnabled[0] = LimitEnabled[1] = LimitEnabled[2].Set(bLimitTranslation);
			LimitEnabled[3] = LimitEnabled[4] = LimitEnabled[5].Set(bLimitRotation);
			break;
		}
		case ETLLRN_RigControlType::EulerTransform:
		case ETLLRN_RigControlType::Transform:
		{
			LimitEnabled.SetNum(9);
			LimitEnabled[0] = LimitEnabled[1] = LimitEnabled[2].Set(bLimitTranslation);
			LimitEnabled[3] = LimitEnabled[4] = LimitEnabled[5].Set(bLimitRotation);
			LimitEnabled[6] = LimitEnabled[7] = LimitEnabled[8].Set(bLimitScale);
			break;
		}
		case ETLLRN_RigControlType::Bool:
		default:
		{
			LimitEnabled.Reset();
			break;
		}
	}
}

const FTLLRN_RigCurrentAndInitialTransform& FTLLRN_RigControlElement::GetOffsetTransform() const
{
	return OffsetStorage;
}

FTLLRN_RigCurrentAndInitialTransform& FTLLRN_RigControlElement::GetOffsetTransform()
{
	return OffsetStorage;
}

const FTLLRN_RigCurrentAndInitialDirtyState& FTLLRN_RigControlElement::GetOffsetDirtyState() const
{
	return OffsetDirtyState;
}

FTLLRN_RigCurrentAndInitialDirtyState& FTLLRN_RigControlElement::GetOffsetDirtyState()
{
	return OffsetDirtyState;
}

const FTLLRN_RigCurrentAndInitialTransform& FTLLRN_RigControlElement::GetShapeTransform() const
{
	return ShapeStorage;
}

FTLLRN_RigCurrentAndInitialTransform& FTLLRN_RigControlElement::GetShapeTransform()
{
	return ShapeStorage;
}

const FTLLRN_RigCurrentAndInitialDirtyState& FTLLRN_RigControlElement::GetShapeDirtyState() const
{
	return ShapeDirtyState;
}

FTLLRN_RigCurrentAndInitialDirtyState& FTLLRN_RigControlElement::GetShapeDirtyState()
{
	return ShapeDirtyState;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigControlElement
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigControlElement::ElementTypeIndex = ControlElement;

void FTLLRN_RigControlElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Save(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		Settings.Save(Ar);
		GetOffsetTransform().Save(Ar, GetOffsetDirtyState());
		GetShapeTransform().Save(Ar, GetShapeDirtyState());
		PreferredEulerAngles.Save(Ar);
	}
}

void FTLLRN_RigControlElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Load(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		Settings.Load(Ar);
		GetOffsetTransform().Load(Ar, GetOffsetDirtyState());
		GetShapeTransform().Load(Ar, GetShapeDirtyState());

		if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::PreferredEulerAnglesForControls)
		{
			PreferredEulerAngles.Load(Ar);
		}
		else
		{
			PreferredEulerAngles.Reset();
		}
		PreferredEulerAngles.SetRotationOrder(Settings.PreferredRotationOrder);
	}
}

void FTLLRN_RigControlElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
	Super::CopyFrom(InOther);
	
	const FTLLRN_RigControlElement* Source = CastChecked<FTLLRN_RigControlElement>(InOther);
	Settings = Source->Settings;
	GetOffsetTransform() = Source->GetOffsetTransform();
	GetOffsetDirtyState() = Source->GetOffsetDirtyState();
	GetShapeTransform() = Source->GetShapeTransform();
	GetShapeDirtyState() = Source->GetShapeDirtyState();
	PreferredEulerAngles = Source->PreferredEulerAngles;
}

void FTLLRN_RigControlElement::CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights)
{
	Super::CopyPose(InOther, bCurrent, bInitial, bWeights);
	
	if(FTLLRN_RigControlElement* Other = Cast<FTLLRN_RigControlElement>(InOther))
	{
		if(bCurrent)
		{
			GetOffsetTransform().Current = Other->GetOffsetTransform().Current;
			GetOffsetDirtyState().Current = Other->GetOffsetDirtyState().Current;
			GetShapeTransform().Current = Other->GetShapeTransform().Current;
			GetShapeDirtyState().Current = Other->GetShapeDirtyState().Current;
			PreferredEulerAngles.SetAngles(Other->PreferredEulerAngles.GetAngles(false), false);
		}
		if(bInitial)
		{
			GetOffsetTransform().Initial = Other->GetOffsetTransform().Initial;
			GetOffsetDirtyState().Initial = Other->GetOffsetDirtyState().Initial;
			GetShapeTransform().Initial = Other->GetShapeTransform().Initial;
			GetShapeDirtyState().Initial = Other->GetShapeDirtyState().Initial;
			PreferredEulerAngles.SetAngles(Other->PreferredEulerAngles.GetAngles(true), true);
		}
	}
}

void FTLLRN_RigControlElement::LinkStorage(const TArrayView<FTransform>& InTransforms, const TArrayView<bool>& InDirtyStates,
	const TArrayView<float>& InCurves)
{
	FTLLRN_RigMultiParentElement::LinkStorage(InTransforms, InDirtyStates, InCurves);
	OffsetStorage.LinkStorage(InTransforms);
	ShapeStorage.LinkStorage(InTransforms);
	OffsetDirtyState.LinkStorage(InDirtyStates);
	ShapeDirtyState.LinkStorage(InDirtyStates);
}

void FTLLRN_RigControlElement::UnlinkStorage(FRigReusableElementStorage<FTransform>& InTransforms, FRigReusableElementStorage<bool>& InDirtyStates,
	FRigReusableElementStorage<float>& InCurves)
{
	FTLLRN_RigMultiParentElement::UnlinkStorage(InTransforms, InDirtyStates, InCurves);
	OffsetStorage.UnlinkStorage(InTransforms);
	ShapeStorage.UnlinkStorage(InTransforms);
	OffsetDirtyState.UnlinkStorage(InDirtyStates);
	ShapeDirtyState.UnlinkStorage(InDirtyStates);
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigCurveElement
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigCurveElement::ElementTypeIndex = CurveElement;

void FTLLRN_RigCurveElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Save(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		float Value = Get();
		Ar << bIsValueSet;
		Ar << Value;
	}
}

void FTLLRN_RigCurveElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Load(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::CurveElementValueStateFlag)
		{
			Ar << bIsValueSet;
		}
		else
		{
			bIsValueSet = true;
		}

		float Value = 0.f;
		Ar << Value;

		Set(Value, bIsValueSet);
	}
}

void FTLLRN_RigCurveElement::CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights)
{
	Super::CopyPose(InOther, bCurrent, bInitial, bWeights);
	
	if(const FTLLRN_RigCurveElement* Other = Cast<FTLLRN_RigCurveElement>(InOther))
	{
		Set(Other->Get());
		bIsValueSet = Other->bIsValueSet;
	}
}

const float& FTLLRN_RigCurveElement::Get() const
{
	if(Storage)
	{
		return *Storage;
	}
	ensure(false);
	static constexpr float DefaultCurve = 0.f;
	return DefaultCurve;
}

void FTLLRN_RigCurveElement::Set(const float& InValue, bool InValueIsSet)
{
	if(Storage)
	{
		*Storage = InValue;
		bIsValueSet = InValueIsSet;
	}
}

void FTLLRN_RigCurveElement::LinkStorage(const TArrayView<FTransform>& InTransforms, const TArrayView<bool>& InDirtyStates,
	const TArrayView<float>& InCurves)
{
	FTLLRN_RigBaseElement::LinkStorage(InTransforms, InDirtyStates, InCurves);
	if(InCurves.IsValidIndex(StorageIndex))
	{
		Storage = InCurves.GetData() + StorageIndex;
	}
}

void FTLLRN_RigCurveElement::UnlinkStorage(FRigReusableElementStorage<FTransform>& InTransforms, FRigReusableElementStorage<bool>& InDirtyStates,
	FRigReusableElementStorage<float>& InCurves)
{
	FTLLRN_RigBaseElement::UnlinkStorage(InTransforms, InDirtyStates, InCurves);
	InCurves.Deallocate(StorageIndex, &Storage);
}

void FTLLRN_RigCurveElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
	Super::CopyFrom(InOther);
	
	if(const FTLLRN_RigCurveElement* Other = CastChecked<FTLLRN_RigCurveElement>(InOther))
	{
		Set(Other->Get());
		bIsValueSet = Other->bIsValueSet;
	}
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigPhysicsSolver
////////////////////////////////////////////////////////////////////////////////

void FTLLRN_RigPhysicsSolverDescription::Serialize(FArchive& Ar)
{
	if (Ar.IsSaving() || Ar.IsObjectReferenceCollector() || Ar.IsCountingMemory())
	{
		Save(Ar);
	}
	else if (Ar.IsLoading())
	{
		Load(Ar);
	}
}

void FTLLRN_RigPhysicsSolverDescription::Save(FArchive& Ar)
{
	Ar << ID;
	Ar << Name;
}

void FTLLRN_RigPhysicsSolverDescription::Load(FArchive& Ar)
{
	Ar << ID;
	Ar << Name;
}

FGuid FTLLRN_RigPhysicsSolverDescription::MakeGuid(const FString& InObjectPath, const FName& InSolverName)
{
	const FString CompletePath = FString::Printf(TEXT("%s|%s"), *InObjectPath, *InSolverName.ToString());
	return FGuid::NewDeterministicGuid(CompletePath);
}

FTLLRN_RigPhysicsSolverID FTLLRN_RigPhysicsSolverDescription::MakeID(const FString& InObjectPath, const FName& InSolverName)
{
	return FTLLRN_RigPhysicsSolverID(MakeGuid(InObjectPath, InSolverName));
}

void FTLLRN_RigPhysicsSolverDescription::CopyFrom(const FTLLRN_RigPhysicsSolverDescription* InOther)
{
	if(InOther)
	{
		ID = InOther->ID;
		Name = InOther->Name;
	}
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigPhysicsSettings
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigPhysicsSettings::FTLLRN_RigPhysicsSettings()
	: Mass(1.f)
{
}

void FTLLRN_RigPhysicsSettings::Save(FArchive& Ar)
{
	Ar << Mass;
}

void FTLLRN_RigPhysicsSettings::Load(FArchive& Ar)
{
	Ar << Mass;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigPhysicsElement
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigPhysicsElement::ElementTypeIndex = PhysicsElement;

void FTLLRN_RigPhysicsElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Save(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		Ar << Solver;
		Settings.Save(Ar);
	}
}

void FTLLRN_RigPhysicsElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Load(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		Ar << Solver;
		Settings.Load(Ar);
	}
}

void FTLLRN_RigPhysicsElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
	Super::CopyFrom(InOther);
	
	const FTLLRN_RigPhysicsElement* Source = CastChecked<FTLLRN_RigPhysicsElement>(InOther);
	Solver = Source->Solver;
	Settings = Source->Settings;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigReferenceElement
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigReferenceElement::ElementTypeIndex = ReferenceElement;

void FTLLRN_RigReferenceElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Save(Ar, SerializationPhase);
}

void FTLLRN_RigReferenceElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Load(Ar, SerializationPhase);
}

void FTLLRN_RigReferenceElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
	Super::CopyFrom(InOther);
	
	const FTLLRN_RigReferenceElement* Source = CastChecked<FTLLRN_RigReferenceElement>(InOther);
	GetWorldTransformDelegate = Source->GetWorldTransformDelegate;
}

FTransform FTLLRN_RigReferenceElement::GetReferenceWorldTransform(const FRigVMExecuteContext* InContext, bool bInitial) const
{
	if(GetWorldTransformDelegate.IsBound())
	{
		return GetWorldTransformDelegate.Execute(InContext, GetKey(), bInitial);
	}
	return FTransform::Identity;
}

void FTLLRN_RigReferenceElement::CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights)
{
	Super::CopyPose(InOther, bCurrent, bInitial, bWeights);
	
	if(FTLLRN_RigReferenceElement* Other = Cast<FTLLRN_RigReferenceElement>(InOther))
	{
		if(Other->GetWorldTransformDelegate.IsBound())
		{
			GetWorldTransformDelegate = Other->GetWorldTransformDelegate;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigConnectorSettings
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigConnectorSettings::FTLLRN_RigConnectorSettings()
	: Type(ETLLRN_ConnectorType::Primary)
	, bOptional(false)
{
}

FTLLRN_RigConnectorSettings FTLLRN_RigConnectorSettings::DefaultSettings()
{
	FTLLRN_RigConnectorSettings Settings;
	Settings.AddRule(FTLLRN_RigTypeConnectionRule(ETLLRN_RigElementType::Socket));
	return Settings;
}

void FTLLRN_RigConnectorSettings::Save(FArchive& Ar)
{
	Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);

	Ar << Description;

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::ConnectorsWithType)
	{
		Ar << Type;
		Ar << bOptional;
	}

	int32 NumRules = Rules.Num();
	Ar << NumRules;
	for(int32 Index = 0; Index < NumRules; Index++)
	{

		Rules[Index].Save(Ar);
	}
}

void FTLLRN_RigConnectorSettings::Load(FArchive& Ar)
{
	Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);

	Ar << Description;

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) >= FTLLRN_ControlRigObjectVersion::ConnectorsWithType)
	{
		Ar << Type;
		Ar << bOptional;
	}

	int32 NumRules = 0;
	Ar << NumRules;
	Rules.SetNumZeroed(NumRules);
	for(int32 Index = 0; Index < NumRules; Index++)
	{
		Rules[Index].Load(Ar);
	}
}

bool FTLLRN_RigConnectorSettings::operator==(const FTLLRN_RigConnectorSettings& InOther) const
{
	if(!Description.Equals(InOther.Description, ESearchCase::CaseSensitive))
	{
		return false;
	}
	if(Type != InOther.Type)
	{
		return false;
	}
	if(bOptional != InOther.bOptional)
	{
		return false;
	}
	if(Rules.Num() != InOther.Rules.Num())
	{
		return false;
	}
	for(int32 Index = 0; Index < Rules.Num(); Index++)
	{
		if(Rules[Index] != InOther.Rules[Index])
		{
			return false;
		}
	}
	return true;
}

uint32 FTLLRN_RigConnectorSettings::GetRulesHash() const
{
	uint32 Hash = GetTypeHash(Rules.Num());
	for(const FTLLRN_RigConnectionRuleStash& Rule : Rules)
	{
		Hash = HashCombine(Hash, GetTypeHash(Rule));
	}
	return Hash;
}

uint32 GetTypeHash(const FTLLRN_RigConnectorSettings& Settings)
{
	uint32 Hash = HashCombine(GetTypeHash(Settings.Type), Settings.GetRulesHash());
	Hash = HashCombine(Hash, GetTypeHash(Settings.bOptional));
	return Hash;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigConnectorElement
////////////////////////////////////////////////////////////////////////////////

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigConnectorElement::ElementTypeIndex = ConnectorElement;

void FTLLRN_RigConnectorElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Save(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		Settings.Save(Ar);
	}
}

void FTLLRN_RigConnectorElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Load(Ar, SerializationPhase);

	if(SerializationPhase == ESerializationPhase::StaticData)
	{
		Settings.Load(Ar);
	}
}

FTLLRN_RigConnectorState FTLLRN_RigConnectorElement::GetConnectorState(const UTLLRN_RigHierarchy* InHierarchy) const
{
	FTLLRN_RigConnectorState State;
	State.Name = Key.Name;
	State.ResolvedTarget = InHierarchy->GetResolvedTarget(Key);
	State.Settings = Settings;
	return State;
}

void FTLLRN_RigConnectorElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
	Super::CopyFrom(InOther);
	
	const FTLLRN_RigConnectorElement* Source = CastChecked<FTLLRN_RigConnectorElement>(InOther);
	Settings = Source->Settings;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigSocketElement
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigSocketState::FTLLRN_RigSocketState()
: Name(NAME_None)
, InitialLocalTransform(FTransform::Identity)
, Color(FTLLRN_RigSocketElement::SocketDefaultColor)
{
}

const FTLLRN_RigBaseElement::EElementIndex FTLLRN_RigSocketElement::ElementTypeIndex = SocketElement;
const FName FTLLRN_RigSocketElement::ColorMetaName = TEXT("SocketColor");
const FName FTLLRN_RigSocketElement::DescriptionMetaName = TEXT("SocketDescription");
const FName FTLLRN_RigSocketElement::DesiredParentMetaName = TEXT("SocketDesiredParent");
const FLinearColor FTLLRN_RigSocketElement::SocketDefaultColor = FLinearColor::White;

void FTLLRN_RigSocketElement::Save(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Save(Ar, SerializationPhase);
}

void FTLLRN_RigSocketElement::Load(FArchive& Ar, ESerializationPhase SerializationPhase)
{
	Super::Load(Ar, SerializationPhase);
}

FTLLRN_RigSocketState FTLLRN_RigSocketElement::GetSocketState(const UTLLRN_RigHierarchy* InHierarchy) const
{
	FTLLRN_RigSocketState State;
	State.Name = GetFName();
	State.Parent = InHierarchy->GetTLLRN_RigElementKeyMetadata(GetKey(), DesiredParentMetaName, FTLLRN_RigElementKey());
	if(!State.Parent.IsValid())
	{
		State.Parent = InHierarchy->GetFirstParent(GetKey());
	}
	State.InitialLocalTransform = InHierarchy->GetInitialLocalTransform(GetIndex());
	State.Color = GetColor(InHierarchy);
	State.Description = GetDescription(InHierarchy);
	return State;
}

FLinearColor FTLLRN_RigSocketElement::GetColor(const UTLLRN_RigHierarchy* InHierarchy) const
{
	return InHierarchy->GetLinearColorMetadata(GetKey(), ColorMetaName, SocketDefaultColor);
}

void FTLLRN_RigSocketElement::SetColor(const FLinearColor& InColor, UTLLRN_RigHierarchy* InHierarchy, bool bNotify)
{
	if(InHierarchy->GetLinearColorMetadata(GetKey(), ColorMetaName, SocketDefaultColor).Equals(InColor))
	{
		return;
	}
	InHierarchy->SetLinearColorMetadata(GetKey(), ColorMetaName, InColor);
	InHierarchy->PropagateMetadata(GetKey(), ColorMetaName, bNotify);
	if(bNotify)
	{
		InHierarchy->Notify(ETLLRN_RigHierarchyNotification::SocketColorChanged, this);
	}
}

FString FTLLRN_RigSocketElement::GetDescription(const UTLLRN_RigHierarchy* InHierarchy) const
{
	const FName Description = InHierarchy->GetNameMetadata(GetKey(), DescriptionMetaName, NAME_None);
	if(Description.IsNone())
	{
		return FString();
	}
	return Description.ToString();
}

void FTLLRN_RigSocketElement::SetDescription(const FString& InDescription, UTLLRN_RigHierarchy* InHierarchy, bool bNotify)
{
	const FName Description = InDescription.IsEmpty() ? FName(NAME_None) : *InDescription;
	if(InHierarchy->GetNameMetadata(GetKey(), DescriptionMetaName, NAME_None).IsEqual(Description, ENameCase::CaseSensitive))
	{
		return;
	}
	InHierarchy->SetNameMetadata(GetKey(), DescriptionMetaName, *InDescription);
	InHierarchy->PropagateMetadata(this, DescriptionMetaName, bNotify);
	if(bNotify)
	{
		InHierarchy->Notify(ETLLRN_RigHierarchyNotification::SocketDescriptionChanged, this);
	}
}

void FTLLRN_RigSocketElement::CopyFrom(const FTLLRN_RigBaseElement* InOther)
{
	Super::CopyFrom(InOther);
}

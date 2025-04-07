// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TransformNoScale.h"
#include "EulerTransform.h"
#include "TLLRN_RigHierarchyDefines.generated.h"

class UTLLRN_RigHierarchy;
struct FTLLRN_RigBaseElement;

// Debug define which performs a full check on the cache validity for all elements of the hierarchy.
// This can be useful for debugging cache validity bugs.
#define URIGHIERARCHY_ENSURE_CACHE_VALIDITY 0

/* 
 * This is rig element types that we support
 * This can be used as a mask so supported as a bitfield
 */
UENUM(BlueprintType, meta = (RigVMTypeAllowed))
enum class ETLLRN_RigElementType : uint8
{
	None = 0,
	Bone = 0x001,
	Null = 0x002,
	Space = Null UMETA(Hidden),
	Control = 0x004,
	Curve = 0x008,
	Physics = 0x010, 
	Reference = 0x020,
	Connector = 0x040,
	Socket = 0x080,
	
	First = Bone UMETA(Hidden), 
	Last = Socket UMETA(Hidden), 
	All = Bone | Null | Control | Curve | Physics | Reference | Connector | Socket,
	ToResetAfterConstructionEvent = Bone | Control | Curve | Socket UMETA(Hidden),
};

UENUM(BlueprintType)
enum class ETLLRN_RigBoneType : uint8
{
	Imported,
	User
};

/* 
 * The type of meta data stored on an element
 */
UENUM(BlueprintType, meta = (RigVMTypeAllowed))
enum class ETLLRN_RigMetadataType : uint8
{
	Bool,
	BoolArray,
	Float,
	FloatArray,
	Int32,
	Int32Array,
	Name,
	NameArray,
	Vector,
	VectorArray,
	Rotator,
	RotatorArray,
	Quat,
	QuatArray,
	Transform,
	TransformArray,
	LinearColor,
	LinearColorArray,
	TLLRN_RigElementKey,
	TLLRN_RigElementKeyArray,

	/** MAX - invalid */
	Invalid UMETA(Hidden),
};

UENUM()
enum class ETLLRN_RigHierarchyNotification : uint8
{
	ElementAdded,
	ElementRemoved,
	ElementRenamed,
	ElementSelected,
	ElementDeselected,
	ParentChanged,
	HierarchyReset,
	ControlSettingChanged,
	ControlVisibilityChanged,
	ControlDrivenListChanged,
	ControlShapeTransformChanged,
	ParentWeightsChanged,
	InteractionBracketOpened,
	InteractionBracketClosed,
	ElementReordered,
	ConnectorSettingChanged,
	SocketColorChanged,
	SocketDescriptionChanged,
	SocketDesiredParentChanged,
	HierarchyCopied,

	/** MAX - invalid */
	Max UMETA(Hidden),
};

UENUM(meta = (RigVMTypeAllowed))
enum class ETLLRN_RigEvent : uint8
{
	/** Invalid event */
	None,

	/** Request to Auto-Key the Control in Sequencer */
	RequestAutoKey,

	/** Request to open an Undo bracket in the client */
	OpenUndoBracket,

	/** Request to close an Undo bracket in the client */
	CloseUndoBracket,

	/** MAX - invalid */
	Max UMETA(Hidden),
};

/** When setting control values what to do with regards to setting key.*/
UENUM()
enum class ETLLRN_ControlRigSetKey : uint8
{
	DoNotCare = 0x0,    //Don't care if a key is set or not, may get set, say if auto key is on somewhere.
	Always,				//Always set a key here
	Never				//Never set a key here.
};

UENUM(BlueprintType, meta = (RigVMTypeAllowed))
enum class ETLLRN_RigControlType : uint8
{
	Bool,
    Float,
    Integer,
    Vector2D,
    Position,
    Scale,
    Rotator,
    Transform UMETA(Hidden),
    TransformNoScale UMETA(Hidden),
    EulerTransform,
	ScaleFloat,
};

UENUM(BlueprintType)
enum class ETLLRN_RigControlAnimationType : uint8
{
	// A visible, animatable control.
	AnimationControl,
	// An animation channel without a 3d shape
	AnimationChannel,
	// A control to drive other controls,
	// not animatable in sequencer.
	ProxyControl,
	// Visual feedback only - the control is
	// neither animatable nor selectable.
	VisualCue
};

UENUM(BlueprintType)
enum class ETLLRN_RigControlValueType : uint8
{
	Initial,
    Current,
    Minimum,
    Maximum
};

UENUM(BlueprintType, meta = (RigVMTypeAllowed))
enum class ETLLRN_RigControlVisibility : uint8
{
	// Visibility controlled by the graph
	UserDefined, 
	// Visibility Controlled by the selection of driven controls
	BasedOnSelection 
};

UENUM(BlueprintType, meta = (RigVMTypeAllowed))
enum class ETLLRN_RigControlAxis : uint8
{
	X,
    Y,
    Z
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigControlLimitEnabled
{
	GENERATED_BODY()
	
	FTLLRN_RigControlLimitEnabled()
		: bMinimum(false)
		, bMaximum(false)
	{}

	FTLLRN_RigControlLimitEnabled(bool InValue)
		: bMinimum(false)
		, bMaximum(false)
	{
		Set(InValue);
	}

	FTLLRN_RigControlLimitEnabled(bool InMinimum, bool InMaximum)
		: bMinimum(false)
		, bMaximum(false)
	{
		Set(InMinimum, InMaximum);
	}

	FTLLRN_RigControlLimitEnabled& Set(bool InValue)
	{
		return Set(InValue, InValue);
	}

	FTLLRN_RigControlLimitEnabled& Set(bool InMinimum, bool InMaximum)
	{
		bMinimum = InMinimum;
		bMaximum = InMaximum;
		return *this;
	}

	void Serialize(FArchive& Ar);
	friend FArchive& operator<<(FArchive& Ar, FTLLRN_RigControlLimitEnabled& P)
	{
		P.Serialize(Ar);
		return Ar;
	}

	bool operator ==(const FTLLRN_RigControlLimitEnabled& Other) const
	{
		return bMinimum == Other.bMinimum && bMaximum == Other.bMaximum;
	}

	bool operator !=(const FTLLRN_RigControlLimitEnabled& Other) const
	{
		return bMinimum != Other.bMinimum || bMaximum != Other.bMaximum;
	}

	bool IsOn() const { return bMinimum || bMaximum; }
	bool IsOff() const { return !bMinimum && !bMaximum; }
	bool GetForValueType(ETLLRN_RigControlValueType InValueType) const;
	void SetForValueType(ETLLRN_RigControlValueType InValueType, bool InValue);

	template<typename T>
	T Apply(const T& InValue, const T& InMinimum, const T& InMaximum) const
	{
		if(IsOff())
		{
			return InValue;
		}

		if(bMinimum && bMaximum)
		{
			if(InMinimum < InMaximum)
			{
				return FMath::Clamp<T>(InValue, InMinimum, InMaximum);
			}
			else
			{
				return FMath::Clamp<T>(InValue, InMaximum, InMinimum);
			}
		}

		if(bMinimum)
		{
			return FMath::Max<T>(InValue, InMinimum);
		}

		return FMath::Min<T>(InValue, InMaximum);
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Limit)
	bool bMinimum;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Limit)
	bool bMaximum;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigControlValueStorage
{
public:

	GENERATED_BODY()

	FTLLRN_RigControlValueStorage()
	{
		FMemory::Memzero(this, sizeof(FTLLRN_RigControlValueStorage));
	}

	UPROPERTY()
	float Float00;

	UPROPERTY()
	float Float01;

	UPROPERTY()
	float Float02;

	UPROPERTY()
	float Float03;

	UPROPERTY()
	float Float10;

	UPROPERTY()
	float Float11;

	UPROPERTY()
	float Float12;

	UPROPERTY()
	float Float13;

	UPROPERTY()
	float Float20;

	UPROPERTY()
	float Float21;

	UPROPERTY()
	float Float22;

	UPROPERTY()
	float Float23;

	UPROPERTY()
	float Float30;

	UPROPERTY()
	float Float31;

	UPROPERTY()
	float Float32;

	UPROPERTY()
	float Float33;

	UPROPERTY()
	float Float00_2;

	UPROPERTY()
	float Float01_2;

	UPROPERTY()
	float Float02_2;

	UPROPERTY()
	float Float03_2;

	UPROPERTY()
	float Float10_2;

	UPROPERTY()
	float Float11_2;

	UPROPERTY()
	float Float12_2;

	UPROPERTY()
	float Float13_2;

	UPROPERTY()
	float Float20_2;

	UPROPERTY()
	float Float21_2;

	UPROPERTY()
	float Float22_2;

	UPROPERTY()
	float Float23_2;
	
	UPROPERTY()
	float Float30_2;

	UPROPERTY()
	float Float31_2;

	UPROPERTY()
	float Float32_2;

	UPROPERTY()
	float Float33_2;
	
	UPROPERTY()
	bool bValid;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigControlValue
{
	GENERATED_BODY()

public:

	FTLLRN_RigControlValue()
		: FloatStorage()
#if WITH_EDITORONLY_DATA
		, Storage_DEPRECATED(FTransform::Identity)
#endif
	{
	}

	bool IsValid() const
	{
		return FloatStorage.bValid;
	}

	template<class T>
	T Get() const
	{
		return GetRef<T>();
	}

	template<class T>
	T& GetRef()
	{
		FloatStorage.bValid = true;
		return *(T*)&FloatStorage;
	}

	template<class T>
	const T& GetRef() const
	{
		return *(T*)&FloatStorage;
	}

	template<class T>
	void Set(T InValue)
	{
		GetRef<T>() = InValue;
	}

	template<class T>
	FString ToString() const
	{
		FString Result;
		TBaseStructure<T>::Get()->ExportText(Result, &GetRef<T>(), nullptr, nullptr, PPF_None, nullptr);
		return Result;
	}

	template<class T>
	T SetFromString(const FString& InString)
	{
		T Value;
		TBaseStructure<T>::Get()->ImportText(*InString, &Value, nullptr, PPF_None, nullptr, TBaseStructure<T>::Get()->GetName());
		Set<T>(Value);
		return Value;
	}

	friend FArchive& operator <<(FArchive& Ar, FTLLRN_RigControlValue& Value);

	FString ToPythonString(ETLLRN_RigControlType InControlType) const
	{
		FString ValueStr;
		
		switch (InControlType)
		{
			case ETLLRN_RigControlType::Bool: ValueStr = FString::Printf(TEXT("unreal.TLLRN_RigHierarchy.make_control_value_from_bool(%s)"), Get<bool>() ? TEXT("True") : TEXT("False")); break;							
			case ETLLRN_RigControlType::Float:
			case ETLLRN_RigControlType::ScaleFloat:
				ValueStr = FString::Printf(TEXT("unreal.TLLRN_RigHierarchy.make_control_value_from_float(%.6f)"), Get<float>()); break;
			case ETLLRN_RigControlType::Integer: ValueStr = FString::Printf(TEXT("unreal.TLLRN_RigHierarchy.make_control_value_from_int(%d)"), Get<int>()); break;
			case ETLLRN_RigControlType::Position:
			case ETLLRN_RigControlType::Scale:
				ValueStr = FString::Printf(TEXT("unreal.TLLRN_RigHierarchy.make_control_value_from_vector(unreal.Vector(%.6f, %.6f, %.6f))"),
					Get<FVector3f>().X, Get<FVector3f>().Y, Get<FVector3f>().Z); break;
			case ETLLRN_RigControlType::Rotator: ValueStr = FString::Printf(TEXT("unreal.TLLRN_RigHierarchy.make_control_value_from_rotator(unreal.Rotator(pitch=%.6f, roll=%.6f, yaw=%.6f))"),
				Get<FVector3f>().X, Get<FVector3f>().Z, Get<FVector3f>().Y); break;
			case ETLLRN_RigControlType::Transform: ValueStr = FString::Printf(TEXT("unreal.TLLRN_RigHierarchy.make_control_value_from_euler_transform(unreal.EulerTransform(location=[%.6f,%.6f,%.6f],rotation=[%.6f,%.6f,%.6f],scale=[%.6f,%.6f,%.6f]))"),
				Get<FTransform_Float>().TranslationX,
				Get<FTransform_Float>().TranslationY,
				Get<FTransform_Float>().TranslationZ,
				Get<FTransform_Float>().GetRotation().Rotator().Pitch,
				Get<FTransform_Float>().GetRotation().Rotator().Yaw,
				Get<FTransform_Float>().GetRotation().Rotator().Roll,
				Get<FTransform_Float>().ScaleX,
				Get<FTransform_Float>().ScaleY,
				Get<FTransform_Float>().ScaleZ); break;
			case ETLLRN_RigControlType::EulerTransform: ValueStr = FString::Printf(TEXT("unreal.TLLRN_RigHierarchy.make_control_value_from_euler_transform(unreal.EulerTransform(location=[%.6f,%.6f,%.6f],rotation=[%.6f,%.6f,%.6f],scale=[%.6f,%.6f,%.6f]))"),
				Get<FEulerTransform_Float>().TranslationX,
				Get<FEulerTransform_Float>().TranslationY,
				Get<FEulerTransform_Float>().TranslationZ,
				Get<FEulerTransform_Float>().RotationPitch,
				Get<FEulerTransform_Float>().RotationYaw,
				Get<FEulerTransform_Float>().RotationRoll,
				Get<FEulerTransform_Float>().ScaleX,
				Get<FEulerTransform_Float>().ScaleY,
				Get<FEulerTransform_Float>().ScaleZ); break;
			case ETLLRN_RigControlType::Vector2D: ValueStr = FString::Printf(TEXT("unreal.TLLRN_RigHierarchy.make_control_value_from_vector2d(unreal.Vector2D(%.6f, %.6f))"),
				Get<FVector3f>().X, Get<FVector3f>().Y); break;
			case ETLLRN_RigControlType::TransformNoScale: ValueStr = FString::Printf(TEXT("unreal.TLLRN_RigHierarchy.make_control_value_from_euler_transform(unreal.EulerTransform(location=[%.6f,%.6f,%.6f],rotation=[%.6f,%.6f,%.6f],scale=[%.6f,%.6f,%.6f]))"),
				FEulerTransform(Get<FTransformNoScale_Float>().ToTransform().ToFTransform()).Location.X,
				FEulerTransform(Get<FTransformNoScale_Float>().ToTransform().ToFTransform()).Location.Y,
				FEulerTransform(Get<FTransformNoScale_Float>().ToTransform().ToFTransform()).Location.Z,
				FEulerTransform(Get<FTransformNoScale_Float>().ToTransform().ToFTransform()).Rotation.Euler().X,
				FEulerTransform(Get<FTransformNoScale_Float>().ToTransform().ToFTransform()).Rotation.Euler().Y,
				FEulerTransform(Get<FTransformNoScale_Float>().ToTransform().ToFTransform()).Rotation.Euler().Z,
				1.f, 1.f, 1.f); break;
			default: ensure(false);
		}

		return ValueStr;
	}

	template<class T>
	static FTLLRN_RigControlValue Make(T InValue)
	{
		FTLLRN_RigControlValue Value;
		Value.Set<T>(InValue);
		return Value;
	}

	FTransform GetAsTransform(ETLLRN_RigControlType InControlType, ETLLRN_RigControlAxis InPrimaryAxis) const
	{
		FTransform Transform = FTransform::Identity;
		switch (InControlType)
		{
			case ETLLRN_RigControlType::Bool:
			{
				Transform.SetLocation(FVector(Get<bool>() ? 1.f : 0.f, 0.f, 0.f));
				break;
			}
			case ETLLRN_RigControlType::Float:
			{
				const float ValueToGet = Get<float>();
				switch (InPrimaryAxis)
				{
					case ETLLRN_RigControlAxis::X:
					{
						Transform.SetLocation(FVector(ValueToGet, 0.f, 0.f));
						break;
					}
					case ETLLRN_RigControlAxis::Y:
					{
						Transform.SetLocation(FVector(0.f, ValueToGet, 0.f));
						break;
					}
					case ETLLRN_RigControlAxis::Z:
					{
						Transform.SetLocation(FVector(0.f, 0.f, ValueToGet));
						break;
					}
				}
				break;
			}
			case ETLLRN_RigControlType::ScaleFloat:
			{
				const float ValueToGet = Get<float>();
				const FVector ScaleVector(ValueToGet, ValueToGet, ValueToGet);
				Transform.SetScale3D(ScaleVector);
				break;
			}
			case ETLLRN_RigControlType::Integer:
			{
				const int32 ValueToGet = Get<int32>();
				switch (InPrimaryAxis)
				{
					case ETLLRN_RigControlAxis::X:
					{
						Transform.SetLocation(FVector((float)ValueToGet, 0.f, 0.f));
						break;
					}
					case ETLLRN_RigControlAxis::Y:
					{
						Transform.SetLocation(FVector(0.f, (float)ValueToGet, 0.f));
						break;
					}
					case ETLLRN_RigControlAxis::Z:
					{
						Transform.SetLocation(FVector(0.f, 0.f, (float)ValueToGet));
						break;
					}
				}
				break;
			}
			case ETLLRN_RigControlType::Vector2D:
			{
				const FVector3f ValueToGet = Get<FVector3f>();
				switch (InPrimaryAxis)
				{
					case ETLLRN_RigControlAxis::X:
					{
						Transform.SetLocation(FVector(0.f, ValueToGet.X, ValueToGet.Y));
						break;
					}
					case ETLLRN_RigControlAxis::Y:
					{
						Transform.SetLocation(FVector(ValueToGet.X, 0.f, ValueToGet.Y));
						break;
					}
					case ETLLRN_RigControlAxis::Z:
					{
						Transform.SetLocation(FVector(ValueToGet.X, ValueToGet.Y, 0.f));
						break;
					}
				}
				break;
			}
			case ETLLRN_RigControlType::Position:
			{
				Transform.SetLocation((FVector)Get<FVector3f>());
				break;
			}
			case ETLLRN_RigControlType::Scale:
			{
				Transform.SetScale3D((FVector)Get<FVector3f>());
				break;
			}
			case ETLLRN_RigControlType::Rotator:
			{
				const FVector3f RotatorAxes = Get<FVector3f>();
				Transform.SetRotation(FQuat(FRotator::MakeFromEuler((FVector)RotatorAxes)));
				break;
			}
			case ETLLRN_RigControlType::Transform:
			{
				Transform = Get<FTransform_Float>().ToTransform();
				Transform.NormalizeRotation();
				break;
			}
			case ETLLRN_RigControlType::TransformNoScale:
			{
				const FTransformNoScale TransformNoScale = Get<FTransformNoScale_Float>().ToTransform();
				Transform = TransformNoScale;
				Transform.NormalizeRotation();
				break;
			}
			case ETLLRN_RigControlType::EulerTransform:
			{
				const FEulerTransform EulerTransform = Get<FEulerTransform_Float>().ToTransform();
				Transform = FTransform(EulerTransform.ToFTransform());
				Transform.NormalizeRotation();
				break;
			}
			default:
			{
				ensure(false);
				break;
			}
		}
		return Transform;
	}
	
	void SetFromTransform(const FTransform& InTransform, ETLLRN_RigControlType InControlType, ETLLRN_RigControlAxis InPrimaryAxis)
	{
		switch (InControlType)
		{
			case ETLLRN_RigControlType::Bool:
			{
				Set<bool>(InTransform.GetLocation().X > SMALL_NUMBER);
				break;
			}
			case ETLLRN_RigControlType::Float:
			{
				switch (InPrimaryAxis)
				{
					case ETLLRN_RigControlAxis::X:
					{
						Set<float>(InTransform.GetLocation().X);
						break;
					}
					case ETLLRN_RigControlAxis::Y:
					{
						Set<float>(InTransform.GetLocation().Y);
						break;
					}
					case ETLLRN_RigControlAxis::Z:
					{
						Set<float>(InTransform.GetLocation().Z);
						break;
					}
				}
				break;
			}
			case ETLLRN_RigControlType::ScaleFloat:
			{
				Set<float>(InTransform.GetScale3D().X);
				break;
			}
			case ETLLRN_RigControlType::Integer:
			{
				switch (InPrimaryAxis)
				{
					case ETLLRN_RigControlAxis::X:
					{
						Set<int32>((int32)InTransform.GetLocation().X);
						break;
					}
					case ETLLRN_RigControlAxis::Y:
					{
						Set<int32>((int32)InTransform.GetLocation().Y);
						break;
					}
					case ETLLRN_RigControlAxis::Z:
					{
						Set<int32>((int32)InTransform.GetLocation().Z);
						break;
					}
				}
				break;
			}
			case ETLLRN_RigControlType::Vector2D:
			{
				const FVector Location = InTransform.GetLocation();
				switch (InPrimaryAxis)
				{
					case ETLLRN_RigControlAxis::X:
					{
						Set<FVector3f>(FVector3f(Location.Y, Location.Z, 0.f));
						break;
					}
					case ETLLRN_RigControlAxis::Y:
					{
						Set<FVector3f>(FVector3f(Location.X, Location.Z, 0.f));
						break;
					}
					case ETLLRN_RigControlAxis::Z:
					{
						Set<FVector3f>(FVector3f(Location.X, Location.Y, 0.f));
						break;
					}
				}
				break;
			}
			case ETLLRN_RigControlType::Position:
			{
				Set<FVector3f>((FVector3f)InTransform.GetLocation());
				break;
			}
			case ETLLRN_RigControlType::Scale:
			{
				Set<FVector3f>((FVector3f)InTransform.GetScale3D());
				break;
			}
			case ETLLRN_RigControlType::Rotator:
			{
				//allow for values ><180/-180 by getting diff and adding that back in.
				FRotator CurrentRotator = FRotator::MakeFromEuler((FVector)Get<FVector3f>());
				FRotator CurrentRotWind, CurrentRotRem;
				CurrentRotator.GetWindingAndRemainder(CurrentRotWind, CurrentRotRem);

				//Get Diff
				const FRotator NewRotator = FRotator(InTransform.GetRotation());
				FRotator DeltaRot = NewRotator - CurrentRotRem;
				DeltaRot.Normalize();

				//Add Diff
				CurrentRotator = CurrentRotator + DeltaRot;
				Set<FVector3f>((FVector3f)CurrentRotator.Euler());
				break;
			}
			case ETLLRN_RigControlType::Transform:
			{
				Set<FTransform_Float>(InTransform);
				break;
			}
			case ETLLRN_RigControlType::TransformNoScale:
			{
				const FTransformNoScale NoScale = InTransform;
				Set<FTransformNoScale_Float>(NoScale);
				break;
			}
			case ETLLRN_RigControlType::EulerTransform:
			{
				//Find Diff of the rotation from current and just add that instead of setting so we can go over/under -180
				FEulerTransform NewTransform(InTransform);

				const FEulerTransform CurrentEulerTransform = Get<FEulerTransform_Float>().ToTransform();
				FRotator CurrentWinding;
				FRotator CurrentRotRemainder;
				CurrentEulerTransform.Rotation.GetWindingAndRemainder(CurrentWinding, CurrentRotRemainder);
				const FRotator NewRotator = InTransform.GetRotation().Rotator();
				FRotator DeltaRot = NewRotator - CurrentRotRemainder;
				DeltaRot.Normalize();
				const FRotator NewRotation(CurrentEulerTransform.Rotation + DeltaRot);
				NewTransform.Rotation = NewRotation;
				Set<FEulerTransform_Float>(NewTransform);
				break;
			}
			default:
			{
				ensure(false);
				break;
			}
		}
	}

	void ApplyLimits(
		const TArray<FTLLRN_RigControlLimitEnabled>& LimitEnabled,
		ETLLRN_RigControlType InControlType,
		const FTLLRN_RigControlValue& InMinimumValue,
		const FTLLRN_RigControlValue& InMaximumValue)
	{
		if(LimitEnabled.IsEmpty())
		{
			return;
		}

		switch(InControlType)
		{
			case ETLLRN_RigControlType::Float:
			case ETLLRN_RigControlType::ScaleFloat:
			{
				if (LimitEnabled[0].IsOn())
				{
					float& ValueRef = GetRef<float>();
					ValueRef = LimitEnabled[0].Apply<float>(ValueRef, InMinimumValue.Get<float>(), InMaximumValue.Get<float>());
				}
				break;
			}
			case ETLLRN_RigControlType::Integer:
			{
				if (LimitEnabled[0].IsOn())
				{
					int32& ValueRef = GetRef<int32>();
					ValueRef = LimitEnabled[0].Apply<int32>(ValueRef, InMinimumValue.Get<int32>(), InMaximumValue.Get<int32>());
				}
				break;
			}
			case ETLLRN_RigControlType::Vector2D:
			{
				if(LimitEnabled.Num() < 2)
				{
					return;
				}
				if (LimitEnabled[0].IsOn() || LimitEnabled[1].IsOn())
				{
					FVector3f& ValueRef = GetRef<FVector3f>();
					const FVector3f& Min = InMinimumValue.GetRef<FVector3f>();
					const FVector3f& Max = InMaximumValue.GetRef<FVector3f>();
					ValueRef.X = LimitEnabled[0].Apply<float>(ValueRef.X, Min.X, Max.X);
					ValueRef.Y = LimitEnabled[1].Apply<float>(ValueRef.Y, Min.Y, Max.Y);
				}
				break;
			}
			case ETLLRN_RigControlType::Position:
			{
				if(LimitEnabled.Num() < 3)
				{
					return;
				}
				if (LimitEnabled[0].IsOn() || LimitEnabled[1].IsOn() || LimitEnabled[2].IsOn())
				{
					FVector3f& ValueRef = GetRef<FVector3f>();
					const FVector3f& Min = InMinimumValue.GetRef<FVector3f>();
					const FVector3f& Max = InMaximumValue.GetRef<FVector3f>();
					ValueRef.X = LimitEnabled[0].Apply<float>(ValueRef.X, Min.X, Max.X);
					ValueRef.Y = LimitEnabled[1].Apply<float>(ValueRef.Y, Min.Y, Max.Y);
					ValueRef.Z = LimitEnabled[2].Apply<float>(ValueRef.Z, Min.Z, Max.Z);
				}
				break;
			}
			case ETLLRN_RigControlType::Scale:
			{
				if(LimitEnabled.Num() < 3)
				{
					return;
				}
				if (LimitEnabled[0].IsOn() || LimitEnabled[1].IsOn() || LimitEnabled[2].IsOn())
				{
					FVector3f& ValueRef = GetRef<FVector3f>();
					const FVector3f& Min = InMinimumValue.GetRef<FVector3f>();
					const FVector3f& Max = InMaximumValue.GetRef<FVector3f>();
					ValueRef.X = LimitEnabled[0].Apply<float>(ValueRef.X, Min.X, Max.X);
					ValueRef.Y = LimitEnabled[1].Apply<float>(ValueRef.Y, Min.Y, Max.Y);
					ValueRef.Z = LimitEnabled[2].Apply<float>(ValueRef.Z, Min.Z, Max.Z);
				}
				break;
			}
			case ETLLRN_RigControlType::Rotator:
			{
				if(LimitEnabled.Num() < 3)
				{
					return;
				}
				if (LimitEnabled[0].IsOn() || LimitEnabled[1].IsOn() || LimitEnabled[2].IsOn())
				{
					FVector3f& ValueRef = GetRef<FVector3f>();
					const FVector3f& Min = InMinimumValue.GetRef<FVector3f>();
					const FVector3f& Max = InMaximumValue.GetRef<FVector3f>();
					ValueRef.X = LimitEnabled[0].Apply<float>(ValueRef.X, Min.X, Max.X);
					ValueRef.Y = LimitEnabled[1].Apply<float>(ValueRef.Y, Min.Y, Max.Y);
					ValueRef.Z = LimitEnabled[2].Apply<float>(ValueRef.Z, Min.Z, Max.Z);
				}
				break;
			}
			case ETLLRN_RigControlType::Transform:
			{
				if(LimitEnabled.Num() < 9)
				{
					return;
				}

				FTransform_Float& ValueRef = GetRef<FTransform_Float>();
				const FTransform Min = InMinimumValue.GetRef<FTransform_Float>().ToTransform();
				const FTransform Max = InMaximumValue.GetRef<FTransform_Float>().ToTransform();

				if (LimitEnabled[0].IsOn() || LimitEnabled[1].IsOn() || LimitEnabled[2].IsOn())
				{
					ValueRef.TranslationX = LimitEnabled[0].Apply<float>(ValueRef.TranslationX, Min.GetLocation().X, Max.GetLocation().X);
					ValueRef.TranslationY = LimitEnabled[1].Apply<float>(ValueRef.TranslationY, Min.GetLocation().Y, Max.GetLocation().Y);
					ValueRef.TranslationZ = LimitEnabled[2].Apply<float>(ValueRef.TranslationZ, Min.GetLocation().Z, Max.GetLocation().Z);
				}
				if (LimitEnabled[3].IsOn() || LimitEnabled[4].IsOn() || LimitEnabled[5].IsOn())
				{
					const FRotator Rotator = FQuat(ValueRef.RotationX, ValueRef.RotationY, ValueRef.RotationZ, ValueRef.RotationW).Rotator();
					const FRotator MinRotator = Min.GetRotation().Rotator();
					const FRotator MaxRotator = Max.GetRotation().Rotator();

					FRotator LimitedRotator = Rotator;
					LimitedRotator.Pitch = LimitEnabled[3].Apply<float>(LimitedRotator.Pitch, MinRotator.Pitch, MaxRotator.Pitch);
					LimitedRotator.Yaw = LimitEnabled[4].Apply<float>(LimitedRotator.Yaw, MinRotator.Yaw, MaxRotator.Yaw);
					LimitedRotator.Roll = LimitEnabled[5].Apply<float>(LimitedRotator.Roll, MinRotator.Roll, MaxRotator.Roll);

					FQuat LimitedQuat(LimitedRotator);
					
					ValueRef.RotationX = LimitedQuat.X;
					ValueRef.RotationY = LimitedQuat.Y;
					ValueRef.RotationZ = LimitedQuat.Z;
					ValueRef.RotationW = LimitedQuat.W;
				}
				if (LimitEnabled[6].IsOn() || LimitEnabled[7].IsOn() || LimitEnabled[8].IsOn())
				{
					ValueRef.ScaleX = LimitEnabled[6].Apply<float>(ValueRef.ScaleX, Min.GetScale3D().X, Max.GetScale3D().X);
					ValueRef.ScaleY = LimitEnabled[7].Apply<float>(ValueRef.ScaleY, Min.GetScale3D().Y, Max.GetScale3D().Y);
					ValueRef.ScaleZ = LimitEnabled[8].Apply<float>(ValueRef.ScaleZ, Min.GetScale3D().Z, Max.GetScale3D().Z);
				}
				break;
			}
			case ETLLRN_RigControlType::TransformNoScale:
			{
				if(LimitEnabled.Num() < 6)
				{
					return;
				}

				FTransformNoScale_Float& ValueRef = GetRef<FTransformNoScale_Float>();
				const FTransformNoScale Min = InMinimumValue.GetRef<FTransformNoScale_Float>().ToTransform();
				const FTransformNoScale Max = InMaximumValue.GetRef<FTransformNoScale_Float>().ToTransform();

				if (LimitEnabled[0].IsOn() || LimitEnabled[1].IsOn() || LimitEnabled[2].IsOn())
				{
					ValueRef.TranslationX = LimitEnabled[0].Apply<float>(ValueRef.TranslationX, Min.Location.X, Max.Location.X);
					ValueRef.TranslationY = LimitEnabled[1].Apply<float>(ValueRef.TranslationY, Min.Location.Y, Max.Location.Y);
					ValueRef.TranslationZ = LimitEnabled[2].Apply<float>(ValueRef.TranslationZ, Min.Location.Z, Max.Location.Z);
				}
				if (LimitEnabled[3].IsOn() || LimitEnabled[4].IsOn() || LimitEnabled[5].IsOn())
				{
					const FRotator Rotator = FQuat(ValueRef.RotationX, ValueRef.RotationY, ValueRef.RotationZ, ValueRef.RotationW).Rotator();
					const FRotator MinRotator = Min.Rotation.Rotator();
					const FRotator MaxRotator = Max.Rotation.Rotator();

					FRotator LimitedRotator = Rotator;
					LimitedRotator.Pitch = LimitEnabled[3].Apply<float>(LimitedRotator.Pitch, MinRotator.Pitch, MaxRotator.Pitch);
					LimitedRotator.Yaw = LimitEnabled[4].Apply<float>(LimitedRotator.Yaw, MinRotator.Yaw, MaxRotator.Yaw);
					LimitedRotator.Roll = LimitEnabled[5].Apply<float>(LimitedRotator.Roll, MinRotator.Roll, MaxRotator.Roll);

					FQuat LimitedQuat(LimitedRotator);

					ValueRef.RotationX = LimitedQuat.X;
					ValueRef.RotationY = LimitedQuat.Y;
					ValueRef.RotationZ = LimitedQuat.Z;
					ValueRef.RotationW = LimitedQuat.W;
				}
				break;
			}

			case ETLLRN_RigControlType::EulerTransform:
			{
				if(LimitEnabled.Num() < 9)
				{
					return;
				}

				FEulerTransform_Float& ValueRef = GetRef<FEulerTransform_Float>();
				const FEulerTransform_Float& Min = InMinimumValue.GetRef<FEulerTransform_Float>();
				const FEulerTransform_Float& Max = InMaximumValue.GetRef<FEulerTransform_Float>();

				if (LimitEnabled[0].IsOn() || LimitEnabled[1].IsOn() || LimitEnabled[2].IsOn())
				{
					ValueRef.TranslationX = LimitEnabled[0].Apply<float>(ValueRef.TranslationX, Min.TranslationX, Max.TranslationX);
					ValueRef.TranslationY = LimitEnabled[1].Apply<float>(ValueRef.TranslationY, Min.TranslationY, Max.TranslationY);
					ValueRef.TranslationZ = LimitEnabled[2].Apply<float>(ValueRef.TranslationZ, Min.TranslationZ, Max.TranslationZ);
				}
				if (LimitEnabled[3].IsOn() || LimitEnabled[4].IsOn() || LimitEnabled[5].IsOn())
				{
					ValueRef.RotationPitch = LimitEnabled[3].Apply<float>(ValueRef.RotationPitch, Min.RotationPitch, Max.RotationPitch);
					ValueRef.RotationYaw = LimitEnabled[4].Apply<float>(ValueRef.RotationYaw, Min.RotationYaw, Max.RotationYaw);
					ValueRef.RotationRoll = LimitEnabled[5].Apply<float>(ValueRef.RotationRoll, Min.RotationRoll, Max.RotationRoll);
				}
				if (LimitEnabled[6].IsOn() || LimitEnabled[7].IsOn() || LimitEnabled[8].IsOn())
				{
					ValueRef.ScaleX = LimitEnabled[6].Apply<float>(ValueRef.ScaleX, Min.ScaleX, Max.ScaleX);
					ValueRef.ScaleY = LimitEnabled[7].Apply<float>(ValueRef.ScaleY, Min.ScaleY, Max.ScaleY);
					ValueRef.ScaleZ = LimitEnabled[8].Apply<float>(ValueRef.ScaleZ, Min.ScaleZ, Max.ScaleZ);
				}
				break;
			}
			case ETLLRN_RigControlType::Bool:
			default:
			{
				break;
			}
		}
	}

	struct FTransform_Float
	{
		float RotationX;
		float RotationY;
		float RotationZ;
		float RotationW;
		float TranslationX;
		float TranslationY;
		float TranslationZ;
#if ENABLE_VECTORIZED_TRANSFORM
		float TranslationW;
#endif
		float ScaleX;
		float ScaleY;
		float ScaleZ;
#if ENABLE_VECTORIZED_TRANSFORM
		float ScaleW;
#endif

		FTransform_Float()
		{
			*this = FTransform_Float(FTransform::Identity);
		}

		FTransform_Float(const FTransform& InTransform)
		{
			RotationX = InTransform.GetRotation().X;
			RotationY = InTransform.GetRotation().Y;
			RotationZ = InTransform.GetRotation().Z;
			RotationW = InTransform.GetRotation().W;
			TranslationX = InTransform.GetTranslation().X;
			TranslationY = InTransform.GetTranslation().Y;
			TranslationZ = InTransform.GetTranslation().Z;
			ScaleX = InTransform.GetScale3D().X;
			ScaleY = InTransform.GetScale3D().Y;
			ScaleZ = InTransform.GetScale3D().Z;
#if ENABLE_VECTORIZED_TRANSFORM
			TranslationW = ScaleW = 0.f;
#endif
		}

		FTransform ToTransform() const
		{
			FTransform Transform;
			Transform.SetRotation(FQuat(RotationX, RotationY, RotationZ, RotationW));
			Transform.SetTranslation(FVector(TranslationX, TranslationY, TranslationZ));
			Transform.SetScale3D(FVector(ScaleX, ScaleY, ScaleZ));
			return Transform;
		}

		FVector3f GetTranslation() const { return FVector3f(TranslationX, TranslationY, TranslationZ); }
		FQuat GetRotation() const { return FQuat(RotationX, RotationY, RotationZ, RotationW); }
		FVector3f GetScale3D() const { return FVector3f(ScaleX, ScaleY, ScaleZ); }
	};

	struct FTransformNoScale_Float
	{
		float RotationX;
		float RotationY;
		float RotationZ;
		float RotationW;
		float TranslationX;
		float TranslationY;
		float TranslationZ;
#if ENABLE_VECTORIZED_TRANSFORM
		float TranslationW;
#endif

		FTransformNoScale_Float()
		{
			*this = FTransformNoScale_Float(FTransformNoScale::Identity);
		}

		FTransformNoScale_Float(const FTransformNoScale& InTransform)
		{
			RotationX = InTransform.Rotation.X;
			RotationY = InTransform.Rotation.Y;
			RotationZ = InTransform.Rotation.Z;
			RotationW = InTransform.Rotation.W;
			TranslationX = InTransform.Location.X;
			TranslationY = InTransform.Location.Y;
			TranslationZ = InTransform.Location.Z;
#if ENABLE_VECTORIZED_TRANSFORM
			TranslationW = 0.f;
#endif
		}

		FTransformNoScale ToTransform() const
		{
			FTransformNoScale Transform;
			Transform.Rotation = FQuat(RotationX, RotationY, RotationZ, RotationW);
			Transform.Location = FVector(TranslationX, TranslationY, TranslationZ);
			return Transform;
		}

		FVector3f GetTranslation() const { return FVector3f(TranslationX, TranslationY, TranslationZ); }
		FQuat GetRotation() const { return FQuat(RotationX, RotationY, RotationZ, RotationW); }
	};

	struct FEulerTransform_Float
	{
		float RotationPitch;
		float RotationYaw;
		float RotationRoll;
		float TranslationX;
		float TranslationY;
		float TranslationZ;
		float ScaleX;
		float ScaleY;
		float ScaleZ;

		FEulerTransform_Float()
		{
			*this = FEulerTransform_Float(FEulerTransform::Identity);
		}

		FEulerTransform_Float(const FEulerTransform& InTransform)
		{
			RotationPitch = InTransform.Rotation.Pitch;
			RotationYaw = InTransform.Rotation.Yaw;
			RotationRoll = InTransform.Rotation.Roll;
			TranslationX = InTransform.Location.X;
			TranslationY = InTransform.Location.Y;
			TranslationZ = InTransform.Location.Z;
			ScaleX = InTransform.Scale.X;
			ScaleY = InTransform.Scale.Y;
			ScaleZ = InTransform.Scale.Z;
		}

		FEulerTransform ToTransform() const
		{
			FEulerTransform Transform;
			Transform.Rotation = FRotator(RotationPitch, RotationYaw, RotationRoll);
			Transform.Location = FVector(TranslationX, TranslationY, TranslationZ);
			Transform.Scale = FVector(ScaleX, ScaleY, ScaleZ);
			return Transform;
		}
		
		FVector3f GetTranslation() const { return FVector3f(TranslationX, TranslationY, TranslationZ); }
		FRotator GetRotator() const { return FRotator(RotationPitch, RotationYaw, RotationRoll); }
		FVector3f GetScale3D() const { return FVector3f(ScaleX, ScaleY, ScaleZ); }
	};

private:
	
	UPROPERTY()
	FTLLRN_RigControlValueStorage FloatStorage;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FTransform Storage_DEPRECATED;
#endif

	friend struct FTLLRN_TLLRN_RigControlHierarchy;
	friend class UTLLRN_ControlRigBlueprint;
	friend class UTLLRN_RigHierarchyController;
	friend class UTLLRN_ControlRig;
	friend struct FTLLRN_RigControlSettings;
};

template<>
inline FTLLRN_RigControlValue FTLLRN_RigControlValue::Make(FVector2D InValue)
{
	return Make<FVector3f>(FVector3f(InValue.X, InValue.Y, 0.f));
}

template<>
inline FTLLRN_RigControlValue FTLLRN_RigControlValue::Make(FVector InValue)
{
	return Make<FVector3f>((FVector3f)InValue);
}

template<>
inline FTLLRN_RigControlValue FTLLRN_RigControlValue::Make(FRotator InValue)
{
	return Make<FVector3f>((FVector3f)InValue.Euler());
}

template<>
inline FTLLRN_RigControlValue FTLLRN_RigControlValue::Make(FTransform InValue)
{
	return Make<FTransform_Float>(InValue);
}

template<>
inline FTLLRN_RigControlValue FTLLRN_RigControlValue::Make(FTransformNoScale InValue)
{
	return Make<FTransformNoScale_Float>(InValue);
}

template<>
inline FTLLRN_RigControlValue FTLLRN_RigControlValue::Make(FEulerTransform InValue)
{
	return Make<FEulerTransform_Float>(InValue);
}

template<>
inline FString FTLLRN_RigControlValue::ToString<bool>() const
{
	const bool Value = Get<bool>();
	static const TCHAR* True = TEXT("True");
	static const TCHAR* False = TEXT("False");
	return Value ? True : False;
}

template<>
inline FString FTLLRN_RigControlValue::ToString<int32>() const
{
	const int32 Value = Get<int32>();
	return FString::FromInt(Value);
}

template<>
inline FString FTLLRN_RigControlValue::ToString<float>() const
{
	const float Value = Get<float>();
	return FString::SanitizeFloat(Value);
}

template<>
inline FString FTLLRN_RigControlValue::ToString<FVector>() const
{
	FVector3f Value = GetRef<FVector3f>();
	FString Result;
	TBaseStructure<FVector>::Get()->ExportText(Result, &Value, nullptr, nullptr, PPF_None, nullptr);
	return Result;
}

template<>
inline FString FTLLRN_RigControlValue::ToString<FVector2D>() const
{
	const FVector3f Value = GetRef<FVector3f>();
	FVector2D Value2D(Value.X, Value.Y);
	FString Result;
	TBaseStructure<FVector2D>::Get()->ExportText(Result, &Value2D, nullptr, nullptr, PPF_None, nullptr);
	return Result;
}

template<>
inline FString FTLLRN_RigControlValue::ToString<FRotator>() const
{
	FRotator Value = FRotator::MakeFromEuler((FVector)GetRef<FVector3f>());
	FString Result;
	TBaseStructure<FRotator>::Get()->ExportText(Result, &Value, nullptr, nullptr, PPF_None, nullptr);
	return Result;
}

template<>
inline FString FTLLRN_RigControlValue::ToString<FTransform>() const
{
	FTransform Value = GetRef<FTransform_Float>().ToTransform();
	FString Result;
	TBaseStructure<FTransform>::Get()->ExportText(Result, &Value, nullptr, nullptr, PPF_None, nullptr);
	return Result;
}

template<>
inline FString FTLLRN_RigControlValue::ToString<FTransformNoScale>() const
{
	FTransformNoScale Value = GetRef<FTransformNoScale_Float>().ToTransform();
	FString Result;
	TBaseStructure<FTransformNoScale>::Get()->ExportText(Result, &Value, nullptr, nullptr, PPF_None, nullptr);
	return Result;
}

template<>
inline FString FTLLRN_RigControlValue::ToString<FEulerTransform>() const
{
	FEulerTransform Value = GetRef<FEulerTransform_Float>().ToTransform();
	FString Result;
	TBaseStructure<FEulerTransform>::Get()->ExportText(Result, &Value, nullptr, nullptr, PPF_None, nullptr);
	return Result;
}

template<>
inline FVector FTLLRN_RigControlValue::SetFromString<FVector>(const FString& InString)
{
	FVector Value;
	TBaseStructure<FVector>::Get()->ImportText(*InString, &Value, nullptr, PPF_None, nullptr, TBaseStructure<FVector>::Get()->GetName());
	Set<FVector3f>((FVector3f)Value);
	return Value;
}

template<>
inline FQuat FTLLRN_RigControlValue::SetFromString<FQuat>(const FString& InString)
{
	FQuat Value;
	TBaseStructure<FQuat>::Get()->ImportText(*InString, &Value, nullptr, PPF_None, nullptr, TBaseStructure<FQuat>::Get()->GetName());
	Set<FVector3f>((FVector3f)Value.Rotator().Euler());
	return Value;
}

template<>
inline FRotator FTLLRN_RigControlValue::SetFromString<FRotator>(const FString& InString)
{
	FRotator Value;
	TBaseStructure<FRotator>::Get()->ImportText(*InString, &Value, nullptr, PPF_None, nullptr, TBaseStructure<FRotator>::Get()->GetName());
	Set<FVector3f>((FVector3f)Value.Euler());
	return Value;
}

template<>
inline FTransform FTLLRN_RigControlValue::SetFromString<FTransform>(const FString& InString)
{
	FTransform Value;
	TBaseStructure<FTransform>::Get()->ImportText(*InString, &Value, nullptr, PPF_None, nullptr, TBaseStructure<FTransform>::Get()->GetName());
	Set<FTransform_Float>(Value);
	return Value;
}

template<>
inline FTransformNoScale FTLLRN_RigControlValue::SetFromString<FTransformNoScale>(const FString& InString)
{
	FTransformNoScale Value;
	TBaseStructure<FTransformNoScale>::Get()->ImportText(*InString, &Value, nullptr, PPF_None, nullptr, TBaseStructure<FTransformNoScale>::Get()->GetName());
	Set<FTransformNoScale_Float>(Value);
	return Value;
}

template<>
inline FEulerTransform FTLLRN_RigControlValue::SetFromString<FEulerTransform>(const FString& InString)
{
	FEulerTransform Value;
	TBaseStructure<FEulerTransform>::Get()->ImportText(*InString, &Value, nullptr, PPF_None, nullptr, TBaseStructure<FEulerTransform>::Get()->GetName());
	Set<FEulerTransform_Float>(Value);
	return Value;
}

#define UE_TLLRN_CONTROLRIG_VALUE_DELETE_TEMPLATE(T) \
template<> \
const T FTLLRN_RigControlValue::Get() const = delete;  \
template<>  \
const T& FTLLRN_RigControlValue::GetRef() const = delete; \
template<> \
T& FTLLRN_RigControlValue::GetRef() = delete; \
template<> \
void FTLLRN_RigControlValue::Set(T InValue) = delete;

// We disable all of these to force Control Rig
// to store values as floats. We've added our own
// float variations purely for storage purposes,
// from which we'll cast back and forth to FTransform etc.
UE_TLLRN_CONTROLRIG_VALUE_DELETE_TEMPLATE(FVector2D)
UE_TLLRN_CONTROLRIG_VALUE_DELETE_TEMPLATE(FVector)
UE_TLLRN_CONTROLRIG_VALUE_DELETE_TEMPLATE(FVector4)
UE_TLLRN_CONTROLRIG_VALUE_DELETE_TEMPLATE(FRotator)
UE_TLLRN_CONTROLRIG_VALUE_DELETE_TEMPLATE(FQuat)
UE_TLLRN_CONTROLRIG_VALUE_DELETE_TEMPLATE(FTransform)
UE_TLLRN_CONTROLRIG_VALUE_DELETE_TEMPLATE(FTransformNoScale)
UE_TLLRN_CONTROLRIG_VALUE_DELETE_TEMPLATE(FEulerTransform)

enum class ETLLRN_ControlRigContextChannelToKey : uint32
{
	None = 0x000,

	TranslationX = 0x001,
	TranslationY = 0x002,
	TranslationZ = 0x004,
	Translation = TranslationX | TranslationY | TranslationZ,

	RotationX = 0x008,
	RotationY = 0x010,
	RotationZ = 0x020,
	Rotation = RotationX | RotationY | RotationZ,

	ScaleX = 0x040,
	ScaleY = 0x080,
	ScaleZ = 0x100,
	Scale = ScaleX | ScaleY | ScaleZ,

	AllTransform = Translation | Rotation | Scale,

};
ENUM_CLASS_FLAGS(ETLLRN_ControlRigContextChannelToKey)

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigControlModifiedContext
{
	GENERATED_BODY()

	FTLLRN_RigControlModifiedContext()
	: SetKey(ETLLRN_ControlRigSetKey::DoNotCare)
	, KeyMask((uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform)
	, LocalTime(FLT_MAX)
	, EventName(NAME_None)
	{}

	FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey InSetKey)
		: SetKey(InSetKey)
		, KeyMask((uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform)
		, LocalTime(FLT_MAX)
		, EventName(NAME_None)
	{}

	FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey InSetKey, float InLocalTime, const FName& InEventName = NAME_None, ETLLRN_ControlRigContextChannelToKey InKeyMask = ETLLRN_ControlRigContextChannelToKey::AllTransform)
		: SetKey(InSetKey)
		, KeyMask((uint32)InKeyMask)
		, LocalTime(InLocalTime)
		, EventName(InEventName)
	{}
	
	ETLLRN_ControlRigSetKey SetKey;
	uint32 KeyMask;
	float LocalTime;
	FName EventName;
	bool bConstraintUpdate = false;
};

/*
 * Because it's bitfield, we support some basic functionality
 */
namespace FTLLRN_RigElementTypeHelper
{
	static uint32 Add(uint32 InMasks, ETLLRN_RigElementType InType)
	{
		return InMasks & (uint32)InType;
	}

	static uint32 Remove(uint32 InMasks, ETLLRN_RigElementType InType)
	{
		return InMasks & ~((uint32)InType);
	}

	static uint32 ToMask(ETLLRN_RigElementType InType)
	{
		return (uint32)InType;
	}

	static bool DoesHave(uint32 InMasks, ETLLRN_RigElementType InType)
	{
		return (InMasks & (uint32)InType) != 0;
	}
}

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigElementKey
{
public:
	
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Hierarchy")
	ETLLRN_RigElementType Type;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Hierarchy", meta = (CustomWidget = "ElementName"))
	FName Name;

	FTLLRN_RigElementKey(const ETLLRN_RigElementType InType = ETLLRN_RigElementType::None)
		: Type(InType)
		, Name(NAME_None)
	{}

	FTLLRN_RigElementKey(const FName& InName, const ETLLRN_RigElementType InType)
		: Type(InType)
		, Name(InName)
	{}

	FTLLRN_RigElementKey(const FString& InKeyString)
	{
		FString TypeStr, NameStr;
		check(InKeyString.Split(TEXT("("), &TypeStr, &NameStr));
		NameStr.RemoveFromEnd(TEXT(")"));
		Name = *NameStr;

		const UEnum* ElementTypeEnum = StaticEnum<ETLLRN_RigElementType>();
		Type = (ETLLRN_RigElementType)ElementTypeEnum->GetValueByName(*TypeStr);
		check(Type > ETLLRN_RigElementType::None && Type < ETLLRN_RigElementType::All);
		
	}

	void Serialize(FArchive& Ar);
	void Save(FArchive& Ar);
	void Load(FArchive& Ar);
	friend FArchive& operator<<(FArchive& Ar, FTLLRN_RigElementKey& P)
	{
		P.Serialize(Ar);
		return Ar;
	}

	bool IsValid() const
	{
		return Name.IsValid() && Name != NAME_None && Type != ETLLRN_RigElementType::None;
	}

	explicit operator bool() const
	{
		return IsValid();
	}

	void Reset()
	{
		Type = ETLLRN_RigElementType::Curve;
		Name = NAME_None;
	}

	bool IsTypeOf(ETLLRN_RigElementType InElementType) const
	{
		return ((uint8)InElementType & (uint8)Type) == (uint8)Type;
	}

	friend uint32 GetTypeHash(const FTLLRN_RigElementKey& Key)
	{
		return GetTypeHash(Key.Name) * 10 + (uint32)Key.Type;
	}

	friend uint32 GetTypeHash(const TArrayView<const FTLLRN_RigElementKey>& Keys)
	{
		uint32 Hash = (uint32)(Keys.Num() * 17 + 3);
		for (const FTLLRN_RigElementKey& Key : Keys)
		{
			Hash += GetTypeHash(Key);
		}
		return Hash;
	}

	friend uint32 GetTypeHash(const TArray<FTLLRN_RigElementKey>& Keys)
	{
		return GetTypeHash(TArrayView<const FTLLRN_RigElementKey>(Keys.GetData(), Keys.Num()));
	}

	bool operator ==(const FTLLRN_RigElementKey& Other) const
	{
		return Name == Other.Name && Type == Other.Type;
	}

	bool operator !=(const FTLLRN_RigElementKey& Other) const
	{
		return Name != Other.Name || Type != Other.Type;
	}

	bool operator <(const FTLLRN_RigElementKey& Other) const
	{
		if (Type < Other.Type)
		{
			return true;
		}
		return Name.LexicalLess(Other.Name);
	}

	bool operator >(const FTLLRN_RigElementKey& Other) const
	{
		if (Type > Other.Type)
		{
			return true;
		}
		return Other.Name.LexicalLess(Name);
	}

	FString ToString() const
	{
		switch (Type)
		{
			case ETLLRN_RigElementType::Bone:
			{
				return FString::Printf(TEXT("Bone(%s)"), *Name.ToString());
			}
			case ETLLRN_RigElementType::Null:
			{
				return FString::Printf(TEXT("Null(%s)"), *Name.ToString());
			}
			case ETLLRN_RigElementType::Control:
			{
				return FString::Printf(TEXT("Control(%s)"), *Name.ToString());
			}
			case ETLLRN_RigElementType::Curve:
			{
				return FString::Printf(TEXT("Curve(%s)"), *Name.ToString());
			}
			case ETLLRN_RigElementType::Physics:
			{
				return FString::Printf(TEXT("Physics(%s)"), *Name.ToString());
			}
			case ETLLRN_RigElementType::Reference:
			{
				return FString::Printf(TEXT("Reference(%s)"), *Name.ToString());
			}
			case ETLLRN_RigElementType::Connector:
			{
				return FString::Printf(TEXT("Connector(%s)"), *Name.ToString());
			}
			case ETLLRN_RigElementType::Socket:
			{
				return FString::Printf(TEXT("Socket(%s)"), *Name.ToString());
			}
		}
		
		return FString();
	}

	FString ToPythonString() const;
};

struct TLLRN_CONTROLRIG_API FTLLRN_RigElementKeyAndIndex
{
public:

	inline static const FTLLRN_RigElementKey InvalidKey = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
	inline static constexpr int32 InvalidIndex = INDEX_NONE;

	FTLLRN_RigElementKeyAndIndex()
		: Key(InvalidKey)
		, Index(InvalidIndex)
	{
	}

	explicit FTLLRN_RigElementKeyAndIndex(const FTLLRN_RigBaseElement* InElement);

	FTLLRN_RigElementKeyAndIndex(const FTLLRN_RigElementKey& InKey, const int32& InIndex)
		: Key(InKey)
		, Index(InIndex)
	{
	}

	bool IsValid() const
	{
		return Key.IsValid() && Index != INDEX_NONE;
	}

	operator int32() const
	{
		return Index;
	}

	friend uint32 GetTypeHash(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex)
	{
		return GetTypeHash(InKeyAndIndex.Index);
	}
	
	const FTLLRN_RigElementKey& Key;
	const int32& Index;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigElementKeyCollection
{
	GENERATED_BODY()

	FTLLRN_RigElementKeyCollection()
	{
	}

	FTLLRN_RigElementKeyCollection(const TArray<FTLLRN_RigElementKey>& InKeys)
		: Keys(InKeys)
	{
	}

	// Resets the data structure and maintains all storage.
	void Reset()
	{
		Keys.Reset();
	}

	// Resets the data structure and removes all storage.
	void Empty()
	{
		Keys.Empty();
	}

	// Returns true if a given instruction index is valid.
	bool IsValidIndex(int32 InIndex) const
	{
		return Keys.IsValidIndex(InIndex);
	}

	// Returns the number of elements in this collection.
	int32 Num() const { return Keys.Num(); }

	// Returns true if this collection contains no elements.
	bool IsEmpty() const
	{
		return Num() == 0;
	}

	// Returns the first element of this collection
	const FTLLRN_RigElementKey& First() const
	{
		return Keys[0];
	}

	// Returns the first element of this collection
	FTLLRN_RigElementKey& First()
	{
		return Keys[0];
	}

	// Returns the last element of this collection
	const FTLLRN_RigElementKey& Last() const
	{
		return Keys.Last();
	}

	// Returns the last element of this collection
	FTLLRN_RigElementKey& Last()
	{
		return Keys.Last();
	}

	int32 Add(const FTLLRN_RigElementKey& InKey)
	{
		return Keys.Add(InKey);
	}

	int32 AddUnique(const FTLLRN_RigElementKey& InKey)
	{
		return Keys.AddUnique(InKey);
	}

	bool Contains(const FTLLRN_RigElementKey& InKey) const
	{
		return Keys.Contains(InKey);
	}

	// const accessor for an element given its index
	const FTLLRN_RigElementKey& operator[](int32 InIndex) const
	{
		return Keys[InIndex];
	}

	const TArray<FTLLRN_RigElementKey>& GetKeys() const
	{
		return Keys;
	}
	   
	TArray<FTLLRN_RigElementKey>::RangedForIteratorType      begin() { return Keys.begin(); }
	TArray<FTLLRN_RigElementKey>::RangedForConstIteratorType begin() const { return Keys.begin(); }
	TArray<FTLLRN_RigElementKey>::RangedForIteratorType      end() { return Keys.end(); }
	TArray<FTLLRN_RigElementKey>::RangedForConstIteratorType end() const { return Keys.end(); }

	friend uint32 GetTypeHash(const FTLLRN_RigElementKeyCollection& Collection)
	{
		return GetTypeHash(Collection.GetKeys());
	}

	// creates a collection containing all of the children of a given 
	static FTLLRN_RigElementKeyCollection MakeFromChildren(
		UTLLRN_RigHierarchy* InHierarchy, 
		const FTLLRN_RigElementKey& InParentKey,
		bool bRecursive = true,
		bool bIncludeParent = false,
		uint8 InElementTypes = (uint8)ETLLRN_RigElementType::All);

	// creates a collection containing all of the elements with a given name
	static FTLLRN_RigElementKeyCollection MakeFromName(
		UTLLRN_RigHierarchy* InHierarchy,
		const FName& InPartialName,
		uint8 InElementTypes = (uint8)ETLLRN_RigElementType::All);

	// creates a collection containing an item chain
	static FTLLRN_RigElementKeyCollection MakeFromChain(
		UTLLRN_RigHierarchy* InHierarchy,
		const FTLLRN_RigElementKey& InFirstItem,
		const FTLLRN_RigElementKey& InLastItem,
		bool bReverse = false);

	// creates a collection containing all keys of a hierarchy
	static FTLLRN_RigElementKeyCollection MakeFromCompleteHierarchy(
		UTLLRN_RigHierarchy* InHierarchy,
		uint8 InElementTypes = (uint8)ETLLRN_RigElementType::All);

	// returns the union between two collections
	static FTLLRN_RigElementKeyCollection MakeUnion(const FTLLRN_RigElementKeyCollection& A, const FTLLRN_RigElementKeyCollection& B, bool bAllowDuplicates = false);

	// returns the intersection between two collections
	static FTLLRN_RigElementKeyCollection MakeIntersection(const FTLLRN_RigElementKeyCollection& A, const FTLLRN_RigElementKeyCollection& B);

	// returns the difference between two collections
	static FTLLRN_RigElementKeyCollection MakeDifference(const FTLLRN_RigElementKeyCollection& A, const FTLLRN_RigElementKeyCollection& B);

	// returns the collection in the reverse order
	static FTLLRN_RigElementKeyCollection MakeReversed(const FTLLRN_RigElementKeyCollection& InCollection);

	// filters a collection by element type
	FTLLRN_RigElementKeyCollection FilterByType(uint8 InElementTypes) const;

	// filters a collection by name
	FTLLRN_RigElementKeyCollection FilterByName(const FName& InPartialName) const;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Collection)
	TArray<FTLLRN_RigElementKey> Keys;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigElement
{
	GENERATED_BODY()

	FTLLRN_RigElement()
		: Name(NAME_None)
		, Index(INDEX_NONE)
	{}
	virtual ~FTLLRN_RigElement() {}
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = FTLLRN_RigElement)
	FName Name;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = FTLLRN_RigElement)
	int32 Index;

	virtual ETLLRN_RigElementType GetElementType() const
	{
		return ETLLRN_RigElementType::None; 
	}

	FTLLRN_RigElementKey GetElementKey() const
	{
		return FTLLRN_RigElementKey(Name, GetElementType());
	}
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigEventContext
{
	GENERATED_BODY()

	FTLLRN_RigEventContext()
		: Event(ETLLRN_RigEvent::None)
		, SourceEventName(NAME_None)
		, Key()
		, LocalTime(0.f)
		, Payload(nullptr)
	{}
	
	ETLLRN_RigEvent Event;
	FName SourceEventName;
	FTLLRN_RigElementKey Key;
	float LocalTime;
	void* Payload;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FTLLRN_RigEventDelegate, UTLLRN_RigHierarchy*, const FTLLRN_RigEventContext&);

UENUM()
enum class ETLLRN_RigElementResolveState : uint8
{
	Unknown,
	InvalidTarget,
	PossibleTarget,
	DefaultTarget,

	/** MAX - invalid */
	Max UMETA(Hidden),
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigElementResolveResult
{
	GENERATED_BODY()

public:
	
	FTLLRN_RigElementResolveResult()
	: State(ETLLRN_RigElementResolveState::Unknown)
	{
	}

	FTLLRN_RigElementResolveResult(const FTLLRN_RigElementKey& InKey, ETLLRN_RigElementResolveState InState = ETLLRN_RigElementResolveState::PossibleTarget, const FText& InMessage = FText())
		: Key(InKey)
		, State(InState)
		, Message(InMessage)
	{
	}

	bool IsValid() const;
	const FTLLRN_RigElementKey& GetKey() const
	{
		return Key;
	}
	const ETLLRN_RigElementResolveState& GetState() const
	{
		return State;
	}
	const FText& GetMessage() const
	{
		return Message;
	}
	void SetInvalidTarget(const FText& InMessage);
	void SetPossibleTarget(const FText& InMessage = FText());
	void SetDefaultTarget(const FText& InMessage = FText());

private:
	
	UPROPERTY()
	FTLLRN_RigElementKey Key;

	UPROPERTY()
	ETLLRN_RigElementResolveState State;

	UPROPERTY()
	FText Message;

	friend class UTLLRN_ModularRigRuleManager;
};

UENUM()
enum class ETLLRN_ModularRigResolveState : uint8
{
	Success,
	Error,

	/** MAX - invalid */
	Max UMETA(Hidden),
};


USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_ModularRigResolveResult
{
	GENERATED_BODY()

public:
	
	FTLLRN_ModularRigResolveResult()
		: State(ETLLRN_ModularRigResolveState::Success)
	{
	}

	bool IsValid() const;

	const FTLLRN_RigElementKey& GetConnectorKey() const
	{
		return Connector;
	}

	ETLLRN_ModularRigResolveState GetState() const
	{
		return State;
	}

	const FText& GetMessage() const
	{
		return Message;
	}

	const TArray<FTLLRN_RigElementResolveResult>& GetMatches() const
	{
		return Matches;
	}

	const TArray<FTLLRN_RigElementResolveResult>& GetExcluded() const
	{
		return Excluded;
	}

	bool ContainsMatch(const FTLLRN_RigElementKey& InKey, FString* OutErrorMessage = nullptr) const;
	const FTLLRN_RigElementResolveResult* FindMatch(const FTLLRN_RigElementKey& InKey) const;
	const FTLLRN_RigElementResolveResult* GetDefaultMatch() const;

private:

	UPROPERTY()
	FTLLRN_RigElementKey Connector;

	UPROPERTY()
	TArray<FTLLRN_RigElementResolveResult> Matches;

	UPROPERTY()
	TArray<FTLLRN_RigElementResolveResult> Excluded;

	UPROPERTY()
	ETLLRN_ModularRigResolveState State;

	UPROPERTY()
	FText Message;

	friend class UTLLRN_ModularRigRuleManager;
	friend class UTLLRN_ModularRig;
	friend struct FTLLRN_RigUnit_GetCandidates;
	friend struct FTLLRN_RigUnit_DiscardMatches;
	friend struct FTLLRN_RigUnit_SetDefaultMatch;
};

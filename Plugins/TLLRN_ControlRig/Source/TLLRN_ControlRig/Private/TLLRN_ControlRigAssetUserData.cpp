// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigAssetUserData.h"
#include "Misc/PackageName.h"

void UTLLRN_ControlRigShapeLibraryLink::SetShapeLibrary(TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary> InShapeLibrary)
{
	InvalidateCache();
	ShapeLibrary = InShapeLibrary;
	ShapeLibraryCached = InShapeLibrary.Get();
	ShapeNames.Reset();
	if(ShapeLibraryCached)
	{
		for(const FTLLRN_ControlRigShapeDefinition& Shape : ShapeLibraryCached->Shapes)
		{
			ShapeNames.Add(Shape.ShapeName);
		}
	}
}

const UNameSpacedUserData::FUserData* UTLLRN_ControlRigShapeLibraryLink::GetUserData(const FString& InPath, FString* OutErrorMessage) const
{
	// rely on super to look it up from the cache
	if(const FUserData* ResultFromSuper = Super::GetUserData(InPath))
	{
		return ResultFromSuper;
	}

	if(ShapeLibrary)
	{
		if(InPath.Equals(GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_ControlRigShapeLibraryLink, ShapeLibrary), ESearchCase::CaseSensitive))
		{
			static const FProperty* ShapeLibraryProperty = FindPropertyByName(StaticClass(), GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigShapeLibraryLink, ShapeLibraryCached));
			return StoreCacheForUserData({InPath, ShapeLibraryProperty, static_cast<const uint8*>(ShapeLibraryCached)});
		}
		if(InPath.Equals(DefaultShapePath, ESearchCase::CaseSensitive))
		{
			static const FProperty* DefaultShapeProperty = FindPropertyByName(FTLLRN_ControlRigShapeDefinition::StaticStruct(), GET_MEMBER_NAME_CHECKED(FTLLRN_ControlRigShapeDefinition, ShapeName));
			return StoreCacheForUserData({InPath, DefaultShapeProperty, reinterpret_cast<const uint8*>(&ShapeLibrary->DefaultShape.ShapeName)});
		}
		if(InPath.Equals(ShapeNamesPath, ESearchCase::CaseSensitive))
		{
			static const FProperty* ShapeNamesProperty = FindPropertyByName(StaticClass(), GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigShapeLibraryLink, ShapeNames));
			return StoreCacheForUserData({InPath, ShapeNamesProperty, reinterpret_cast<const uint8*>(&ShapeNames)});
		}
		if(OutErrorMessage && OutErrorMessage->IsEmpty())
		{
			(*OutErrorMessage) = FString::Printf(PathNotFoundFormat, *InPath);
		}
		return nullptr;
	}

	if(OutErrorMessage && OutErrorMessage->IsEmpty())
	{
		(*OutErrorMessage) = FString::Printf(ShapeLibraryNullFormat, *InPath);
	}
	return nullptr;
}

const TArray<const UNameSpacedUserData::FUserData*>& UTLLRN_ControlRigShapeLibraryLink::GetUserDataArray(const FString& InParentPath, FString* OutErrorMessage) const
{
	const TArray<const FUserData*>& ResultFromSuper = Super::GetUserDataArray(InParentPath);
	if(!ResultFromSuper.IsEmpty())
	{
		return ResultFromSuper;
	}

	if(ShapeLibrary)
	{
		// UTLLRN_ControlRigShapeLibraryLink doesn't offer any arrays other than the top level 
		if(InParentPath.IsEmpty())
		{
			const FUserData* ShapeLibraryUserData = GetUserData(GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_ControlRigShapeLibraryLink, ShapeLibrary), OutErrorMessage);
			const FUserData* DefaultShapeUserData = GetUserData(DefaultShapePath, OutErrorMessage);
			const FUserData* ShapeNamesUserData = GetUserData(ShapeNamesPath, OutErrorMessage);
			check(ShapeLibraryUserData);
			check(DefaultShapeUserData);
			check(ShapeNamesUserData);
			return StoreCacheForUserDataArray(InParentPath, {ShapeLibraryUserData, DefaultShapeUserData, ShapeNamesUserData});
		}
		return EmptyUserDatas;
	}

	if(OutErrorMessage && OutErrorMessage->IsEmpty())
	{
		(*OutErrorMessage) = FString::Printf(ShapeLibraryNullFormat, *InParentPath);
	}
	return EmptyUserDatas;
}

void UTLLRN_ControlRigShapeLibraryLink::Serialize(FArchive& Ar)
{
	// Treat the cached ptr as transient unless we're cooking this out.
	const bool bIsSavingAssetToStorage = Ar.IsSaving() && Ar.IsPersistent() && !Ar.IsCooking(); 
	UTLLRN_ControlRigShapeLibrary* SavedShapeLibrary = ShapeLibraryCached; 
	if (bIsSavingAssetToStorage)
	{
		ShapeLibraryCached = nullptr;
	}
	
	Super::Serialize(Ar);
	
	if (bIsSavingAssetToStorage)
	{
		ShapeLibraryCached = SavedShapeLibrary;
	}
}

#if WITH_EDITOR

void UTLLRN_ControlRigShapeLibraryLink::PostLoad()
{
	Super::PostLoad();

	ShapeLibraryCached = ShapeLibrary.Get();
	
	if(ShapeLibraryCached == nullptr && !ShapeLibrary.IsNull())
	{
		// We need to check if the mount point exists - since the shape library link may
		// refer to an editor-only asset in a runtime game.
		const FString PackagePath = ShapeLibrary.GetLongPackageName();
		const FName PluginMountPoint = FPackageName::GetPackageMountPoint(PackagePath, false);
		if (FPackageName::MountPointExists(PluginMountPoint.ToString()))
		{
			const FString ObjectPath = ShapeLibrary.ToString();

			// load without throwing additional warnings / errors
			ShapeLibraryCached = LoadObject<UTLLRN_ControlRigShapeLibrary>(nullptr, *ObjectPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
			SetShapeLibrary(ShapeLibrary);
		}
	}
}

void UTLLRN_ControlRigShapeLibraryLink::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	static const FProperty* ShapeLibraryProperty= FindPropertyByName(GetClass(), GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigShapeLibraryLink, ShapeLibrary));
	if(PropertyChangedEvent.Property == ShapeLibraryProperty ||
		PropertyChangedEvent.MemberProperty == ShapeLibraryProperty)
	{
		SetShapeLibrary(ShapeLibrary);
	}
}

#endif

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RigVMCore/RigVMAssetUserData.h"
#include "TLLRN_ControlRigGizmoLibrary.h"
#include "TLLRN_ControlRigAssetUserData.generated.h"

/**
* Namespaced user data which provides access to a linked shape library
*/
UCLASS(BlueprintType)
class TLLRN_CONTROLRIG_API UTLLRN_ControlRigShapeLibraryLink : public UNameSpacedUserData
{
	GENERATED_BODY()

public:

	/** If assigned, the data asset link will provide access to the data asset's content. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = General, BlueprintGetter = GetShapeLibrary, BlueprintSetter = SetShapeLibrary, Meta = (DisplayAfter="NameSpace"))
	TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary> ShapeLibrary;

	UFUNCTION(BlueprintGetter)
	TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary> GetShapeLibrary() const { return ShapeLibrary; }

	UFUNCTION(BlueprintSetter)
	void SetShapeLibrary(TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary> InShapeLibrary);

	virtual const FUserData* GetUserData(const FString& InPath, FString* OutErrorMessage = nullptr) const override;
	virtual const TArray<const FUserData*>& GetUserDataArray(const FString& InParentPath = FString(), FString* OutErrorMessage = nullptr) const override;

	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual bool IsPostLoadThreadSafe() const override { return false; }
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:

	UPROPERTY(transient)
	TArray<FName> ShapeNames;

	UPROPERTY()
	TObjectPtr<UTLLRN_ControlRigShapeLibrary> ShapeLibraryCached;
	

	static inline constexpr TCHAR DefaultShapePath[] = TEXT("DefaultShape");
	static inline constexpr TCHAR ShapeNamesPath[] = TEXT("ShapeNames");
	static inline constexpr TCHAR ShapeLibraryNullFormat[] = TEXT("User data path '%s' could not be found (ShapeLibrary not provided)");
};
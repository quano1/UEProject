// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "CoreMinimal.h"
#include "BodySetupEnums.h"
#include "Engine/EngineTypes.h"   // FMeshNaniteSettings
#include "MeshDescription.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "ShapeApproximation/SimpleShapeSet3.h"
#include "Misc/Optional.h"

#include "TLLRN_ModelingObjectsCreationAPI.generated.h"


class UInteractiveToolManager;
class AActor;
class UPrimitiveComponent;
class UMaterialInterface;
class UTexture2D;


/**
 * Result code returned by UTLLRN_ModelingObjectsCreationAPI functions
 */
UENUM(BlueprintType)
enum class ETLLRN_CreateModelingObjectResult : uint8
{
	Ok,
	Cancelled,
	Failed_Unknown,
	Failed_NoAPIFound,
	Failed_InvalidWorld,
	Failed_InvalidMesh,
	Failed_InvalidTexture,
	Failed_AssetCreationFailed,
	Failed_ActorCreationFailed,
	Failed_InvalidMaterial,
};

/**
 * Types of possible source meshes stored in FTLLRN_CreateMeshObjectParams
 */
UENUM(BlueprintType)
enum class ETLLRN_CreateMeshObjectSourceMeshType : uint8
{
	MeshDescription,
	DynamicMesh
};

/**
 * Hint for the type of mesh object a UTLLRN_ModelingObjectsCreationAPI might create based
 * on FTLLRN_CreateMeshObjectParams data. This can be used by clients to try to specify
 * the type of object to emit, however there is no guarantee that an API implementation
 * supports creating all types.
 */
UENUM(BlueprintType)
enum class ETLLRN_CreateObjectTypeHint : uint8
{
	Undefined = 0,
	StaticMesh = 1,
	Volume = 2,
	DynamicMeshActor = 3
};


/**
 * FTLLRN_CreateMeshObjectParams is a collection of input data intended to be passed to
 * UTLLRN_ModelingObjectsCreationAPI::CreateMeshObject(). Not all data necessarily needs
 * to be specified, this will depend on the particular implementation. The comments
 * below are representative of how this data structure is used in the Tools and
 * API implementation(s) provided with Unreal Engine, but end-user implementors
 * could abuse these fields as necessary.
 * 
 * The definition of a "mesh object" is implementation-specific.
 */
USTRUCT(Blueprintable)
struct TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateMeshObjectParams
{
	// @param bConstructWithDefaultModelingComponentSettings	Whether to initialize with the default project settings.
	// Note the modeling component settings will not be used if the CVar "modeling.CreateMesh.IgnoreProjectSettings" is enabled
	FTLLRN_CreateMeshObjectParams(bool bConstructWithDefaultModelingComponentSettings = true);

	GENERATED_BODY()

	//
	// Base data
	//

	/** A Source Component the new mesh is based on, if such a Component exists */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	TObjectPtr<UPrimitiveComponent> SourceComponent = nullptr;

	/** A suggested type for the newly-created Mesh (possibly ignored) */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	ETLLRN_CreateObjectTypeHint TypeHint = ETLLRN_CreateObjectTypeHint::Undefined;

	/** A suggested UClass type for the newly-created Object (possibly ignored) */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	TObjectPtr<UClass> TypeHintClass = nullptr;

	/** An arbitrary integer that can be used to pass data to an API implementation */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	int32 TypeHintExtended = 0;

	/** The World/Level the new mesh object should be created in (if known) */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	TObjectPtr<UWorld> TargetWorld = nullptr;

	/** The 3D local-to-world transform for the new mesh object */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	FTransform Transform;

	/** The base name of the new mesh object */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	FString BaseName;

	//
	// Materials settings
	//

	/** Materials for the new mesh object */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	/** Optional Materials for a newly-created Mesh Asset, if this is applicable for the created mesh object */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	TArray<TObjectPtr<UMaterialInterface>> AssetMaterials;

	//
	// Collision settings, if applicable for the given mesh object
	//

	/** Specify whether the new mesh object should have collision support/data */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	bool bEnableCollision = true;

	/** Which Collision mode to enable on the new mesh object, if supported */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	TEnumAsByte<enum ECollisionTraceFlag> CollisionMode = ECollisionTraceFlag::CTF_UseComplexAsSimple;

	/** Collision Shapes */
	TOptional<UE::Geometry::FSimpleShapeSet3d> CollisionShapeSet;


	//
	// Rendering Configuration Options, if this is applicable for the given mesh object
	//

	/** Specify whether normals should be automatically recomputed for this new mesh object */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	bool bEnableRaytracingSupport = true;

	/** Specify whether to auto-generate Lightmap UVs (if applicable for the output mesh type) */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	bool bGenerateLightmapUVs = false;

	//
	// Mesh Build Options, if this is applicable for the given mesh object
	// (Currently somewhat specific to Assets in the Editor)
	//

	/** Specify whether normals should be automatically recomputed for this new mesh object */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	bool bEnableRecomputeNormals = false;

	/** Specify whether tangents should be automatically recomputed for this new mesh object */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	bool bEnableRecomputeTangents = false;

	/** Specify whether Nanite should be enabled on this new mesh object */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	bool bEnableNanite = false;

	/** Specify the Nanite proxy triangle percentage for this new mesh object */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Replaced NaniteProxyTrianglePercent with usage of Engine FMeshNaniteSettings"))
	float NaniteProxyTrianglePercent_DEPRECATED = 100.0f;

	/** Specify the Nanite Settings for this new mesh object, only used if bEnableNanite=true */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectParams", EditAnywhere)
	FMeshNaniteSettings NaniteSettings = FMeshNaniteSettings();

	//
	// The Mesh Object should be created based on the mesh data structures below.
	// The assumption is that only one of these mesh data structures will be initialized.
	// Not currently exposed to BP because the types are not BP types.
	//

	ETLLRN_CreateMeshObjectSourceMeshType MeshType = ETLLRN_CreateMeshObjectSourceMeshType::MeshDescription;
	TOptional<FMeshDescription> MeshDescription;
	TOptional<UE::Geometry::FDynamicMesh3> DynamicMesh;

	void SetMesh(FMeshDescription&& MeshDescriptionIn);
	void SetMesh(const UE::Geometry::FDynamicMesh3* DynamicMeshIn);
	void SetMesh(UE::Geometry::FDynamicMesh3&& DynamicMeshIn);

};


/**
 * FTLLRN_CreateMeshObjectResult is returned by UTLLRN_ModelingObjectsCreationAPI::CreateMeshObject()
 * to indicate success/failure and provide information about created mesh objects
 */
USTRUCT(BlueprintType)
struct TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateMeshObjectResult
{
	GENERATED_BODY()

	/** Success/Failure status for the requested operation */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectResult", VisibleAnywhere)
	ETLLRN_CreateModelingObjectResult ResultCode = ETLLRN_CreateModelingObjectResult::Ok;

	/** A pointer to a newly-created Actor for the mesh object, if applicable (eg StaticMeshActor) */
	UPROPERTY(Category="TLLRN_CreateMeshObjectResult", VisibleAnywhere)
	TObjectPtr<AActor> NewActor = nullptr;

	/** A pointer to a newly-created PrimitiveComponent for the mesh object, if applicable (eg StaticMeshComponent) */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectResult", VisibleAnywhere)
	TObjectPtr<UPrimitiveComponent> NewComponent = nullptr;

	/** A pointer to a newly-created Asset for the mesh object, if applicable (eg StaticMeshAsset) */
	UPROPERTY(Category = "TLLRN_CreateMeshObjectResult", VisibleAnywhere)
	TObjectPtr<UObject> NewAsset = nullptr;


	bool IsOK() const { return ResultCode == ETLLRN_CreateModelingObjectResult::Ok; }
};





/**
 * FTLLRN_CreateTextureObjectParams is a collection of input data intended to be passed to
 * UTLLRN_ModelingObjectsCreationAPI::CreateTextureObject(). Not all data necessarily needs
 * to be specified, this will depend on the particular implementation. The comments
 * below are representative of how this data structure is used in the Tools and
 * API implementation(s) provided with Unreal Engine, but end-user implementors
 * could abuse these fields as necessary.
 *
 * The definition of a "texture object" is implementation-specific. 
 * In the UE Editor this is generally a UTexture2D
 */
USTRUCT(Blueprintable)
struct TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateTextureObjectParams
{
	GENERATED_BODY()

	//
	// Base data
	//

	/** An arbitrary integer that can be used to pass data to an API implementation */
	UPROPERTY(Category = "TLLRN_CreateTextureObjectParams", EditAnywhere)
	int32 TypeHintExtended = 0;

	/** 
	 * The World/Level the new texture object should be created in (if known).
	 * Note that Textures generally do not exist as objects in a Level. 
	 * However, it may be necessary to store the texture in a path relative to the
	 * level (for example if the level is in a Plugin, this would be necessary in-Editor)
	 */
	UPROPERTY(Category = "TLLRN_CreateTextureObjectParams", EditAnywhere)
	TObjectPtr<UWorld> TargetWorld = nullptr;

	/** An object to store the Texture relative to. For example the texture could be stored at the same path. */
	UPROPERTY(Category = "TLLRN_CreateTextureObjectParams", EditAnywhere)
	TObjectPtr<UObject> StoreRelativeToObject = nullptr;

	/** The base name of the new mesh object */
	UPROPERTY(Category = "TLLRN_CreateTextureObjectParams", EditAnywhere)
	FString BaseName;

	//
	// input data
	//

	/** 
	 * Texture source data. Generally assumed that this is a Texture created in the Transient package
	 * that is intended to be saved in a permanent package.
	 */
	UPROPERTY(Category = "TLLRN_CreateTextureObjectParams", EditAnywhere)
	TObjectPtr<UTexture2D> GeneratedTransientTexture = nullptr;

	/**
	 * A full path location to save the asset out to. If this parameter is not null, it overrides other work done to find a path
	 */
	UPROPERTY(Category= "TLLRN_CreateTextureObjectParams", EditAnywhere)
	FString FullAssetPath = "";
};


/**
 * FTLLRN_CreateTextureObjectResult is returned by UTLLRN_ModelingObjectsCreationAPI::CreateTextureObject()
 * to indicate success/failure and provide information about created texture objects
 */
USTRUCT(BlueprintType)
struct TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateTextureObjectResult
{
	GENERATED_BODY()

	/** Success/Failure status for the requested operation */
	UPROPERTY(Category = "TLLRN_CreateTextureObjectResult", VisibleAnywhere)
	ETLLRN_CreateModelingObjectResult ResultCode = ETLLRN_CreateModelingObjectResult::Ok;

	/** A pointer to a newly-created Asset for the texture object */
	UPROPERTY(Category = "TLLRN_CreateTextureObjectResult", VisibleAnywhere)
	TObjectPtr<UObject> NewAsset = nullptr;


	bool IsOK() const { return ResultCode == ETLLRN_CreateModelingObjectResult::Ok; }
};






/**
 * FTLLRN_CreateMaterialObjectParams is a collection of input data intended to be passed to
 * UTLLRN_ModelingObjectsCreationAPI::CreateMaterialObject().
 */
USTRUCT(Blueprintable)
struct TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateMaterialObjectParams
{
	GENERATED_BODY()

	//
	// Base data
	//

	/** 
	 * The World/Level the new Material object should be created in (if known).
	 * Note that Material generally do not exist as objects in a Level. 
	 * However, it may be necessary to store the texture in a path relative to the
	 * level (for example if the level is in a Plugin, this would be necessary in-Editor)
	 */
	UPROPERTY(Category = "TLLRN_CreateMaterialObjectParams", EditAnywhere)
	TObjectPtr<UWorld> TargetWorld = nullptr;

	/** An object to store the Material relative to. */
	UPROPERTY(Category = "TLLRN_CreateMaterialObjectParams", EditAnywhere)
	TObjectPtr<UObject> StoreRelativeToObject = nullptr;

	/** The base name of the new Material object */
	UPROPERTY(Category = "TLLRN_CreateMaterialObjectParams", EditAnywhere)
	FString BaseName;

	//
	// input data
	//

	/** 
	 * The parent UMaterial of this material will be duplicated to create the new UMaterial Asset.
	 */
	UPROPERTY(Category = "TLLRN_CreateMaterialObjectParams", EditAnywhere)
	TObjectPtr<UMaterialInterface> MaterialToDuplicate = nullptr;
};


/**
 * FTLLRN_CreateMaterialObjectResult is returned by UTLLRN_ModelingObjectsCreationAPI::CreateMaterialObject()
 * to indicate success/failure and provide information about created texture objects
 */
USTRUCT(BlueprintType)
struct TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateMaterialObjectResult
{
	GENERATED_BODY()

	/** Success/Failure status for the requested operation */
	UPROPERTY(Category = "TLLRN_CreateMaterialObjectResult", VisibleAnywhere)
	ETLLRN_CreateModelingObjectResult ResultCode = ETLLRN_CreateModelingObjectResult::Ok;

	/** A pointer to a newly-created Asset for the material object */
	UPROPERTY(Category = "TLLRN_CreateTextureObjectResult", VisibleAnywhere)
	TObjectPtr<UObject> NewAsset = nullptr;


	bool IsOK() const { return ResultCode == ETLLRN_CreateModelingObjectResult::Ok; }
};




/**
 * FTLLRN_CreateActorParams is a collection of input data intended to be passed to
 * UTLLRN_ModelingObjectsCreationAPI::CreateNewActor().
 */
USTRUCT(Blueprintable)
struct TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateActorParams
{
	GENERATED_BODY()
	

	
	//
	// Base data
	//

	/** 
	 * The World/Level the new Actor should be created in (if known).
	 */
	UPROPERTY(Category = "TLLRN_CreateActorParams", EditAnywhere)
	TObjectPtr<UWorld> TargetWorld = nullptr;

	/** The base name of the new Actor */
	UPROPERTY(Category = "TLLRN_CreateActorParams", EditAnywhere)
	FString BaseName;

	/** The 3D local-to-world transform for the new actor */
	UPROPERTY(Category = "TLLRN_CreateActorParams", EditAnywhere)
	FTransform Transform;

	//
	// input data
	//

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "TemplateActor is being deprecated. Please use TemplateAsset instead."))
	TObjectPtr<AActor> TemplateActor_DEPRECATED;

	/** A template Asset used to determine the type of Actor to spawn. */
	UPROPERTY(Category = "TLLRN_CreateActorParams", EditAnywhere)
	TObjectPtr<UObject> TemplateAsset = nullptr;
};


/**
 * FTLLRN_CreateActorResult is returned by UTLLRN_ModelingObjectsCreationAPI::FTLLRN_CreateActorParams()
 * to indicate success/failure and provide information about created actors
 */
USTRUCT(BlueprintType)
struct TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateActorResult
{
	GENERATED_BODY()

	/** Success/Failure status for the requested operation */
	UPROPERTY(Category = "TLLRN_CreateMaterialObjectResult", VisibleAnywhere)
	ETLLRN_CreateModelingObjectResult ResultCode = ETLLRN_CreateModelingObjectResult::Ok;

	/** A pointer to a newly-created Actor */
	UPROPERTY(Category="TLLRN_CreateMeshObjectResult", VisibleAnywhere)
	TObjectPtr<AActor> NewActor = nullptr;


	bool IsOK() const { return ResultCode == ETLLRN_CreateModelingObjectResult::Ok; }
};





/**
 * UTLLRN_ModelingObjectsCreationAPI is a base interface for functions that can be used to
 * create various types of objects from Modeling Tools, or other sources. The "type" is
 * very generic here - "Mesh", "Texture", etc - because this API is meant to provide
 * an abstraction for Tools to emit different types of objects in different situations.
 * For example an Tool might emit StaticMesh Asset/Actors in-Editor, but ProceduralMeshComponents at Runtime.
 * 
 * The creation inputs are specified via the structs above (eg FTLLRN_CreateMeshObjectParams, FTLLRN_CreateTextureObjectParams),
 * which are very extensive, kitchen-sink sort of structs. Generally "New Mesh Object"
 * creation behavior will be very complex and so this API is really just a way to route
 * the data, and very few guarantees can be made about any specific implementation.
 * 
 * The assumed (but not really required) usage of instances of this type are that they
 * will be registered with an InteractiveToolsContext's ContextObjectStore, and then 
 * fetched from there by Tools/Algorithms/etc that need to use these capabilities can
 * use the UE::Modeling::CreateXObject() helper functions below. However the interface
 * does not have any dependencies on this usage model. 
 * 
 * See UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI for an example implementation suitable for in-Editor use.
 */
UCLASS(Abstract, BlueprintType)
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_ModelingObjectsCreationAPI : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Create a new mesh object based on the data in CreateMeshParams
	 * @return a results data structure, containing a result code and information about any new objects created
	 */
	UFUNCTION(BlueprintCallable, Category = "Modeling Objects")
	virtual FTLLRN_CreateMeshObjectResult CreateMeshObject(const FTLLRN_CreateMeshObjectParams& CreateMeshParams) { return FTLLRN_CreateMeshObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_Unknown }; }

	/**
	 * Create a new texture object based on the data in CreateTexParams
	 * @return a results data structure, containing a result code and information about any new objects created
	 */
	UFUNCTION(BlueprintCallable, Category = "Modeling Objects")
	virtual FTLLRN_CreateTextureObjectResult CreateTextureObject(const FTLLRN_CreateTextureObjectParams& CreateTexParams) { return FTLLRN_CreateTextureObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_Unknown }; }

	/**
	 * Create a new material object based on the data in CreateMaterialParams
	 * @return a results data structure, containing a result code and information about any new objects created
	 */
	UFUNCTION(BlueprintCallable, Category = "Modeling Objects")
	virtual FTLLRN_CreateMaterialObjectResult CreateMaterialObject(const FTLLRN_CreateMaterialObjectParams& CreateMaterialParams) { return FTLLRN_CreateMaterialObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_Unknown }; }

	/**
	 * Create a new material object based on the data in CreateMaterialParams
	 * @return a results data structure, containing a result code and information about any new objects created
	 */
	UFUNCTION(BlueprintCallable, Category = "Modeling Objects")
	virtual FTLLRN_CreateActorResult CreateNewActor(const FTLLRN_CreateActorParams& TLLRN_CreateActorParams) { return FTLLRN_CreateActorResult{ ETLLRN_CreateModelingObjectResult::Failed_Unknown }; }

	//
	// Non-UFunction variants that support std::move operators
	//

	/**
	 * If this function returns true, then the CreateMeshObject() and CreateTextureObject() that take && types can be called.
	 * This can reduce data copying, eg if the mesh data can be directly moved into the output mesh object.
	 */
	virtual bool HasMoveVariants() const { return false; }

	virtual FTLLRN_CreateMeshObjectResult CreateMeshObject(FTLLRN_CreateMeshObjectParams&& CreateMeshParams) { return FTLLRN_CreateMeshObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_Unknown }; }
	virtual FTLLRN_CreateTextureObjectResult CreateTextureObject(FTLLRN_CreateTextureObjectParams&& CreateTexParams) { return FTLLRN_CreateTextureObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_Unknown }; }
	virtual FTLLRN_CreateMaterialObjectResult CreateMaterialObject(FTLLRN_CreateMaterialObjectParams&& CreateMaterialParams) { return FTLLRN_CreateMaterialObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_Unknown }; }
	virtual FTLLRN_CreateActorResult CreateNewActor(FTLLRN_CreateActorParams&& TLLRN_CreateActorParams) { return FTLLRN_CreateActorResult{ ETLLRN_CreateModelingObjectResult::Failed_Unknown }; }

};




namespace UE
{
namespace Modeling
{


/**
 * Create a new mesh object based on the data in CreateMeshParams.
 * This is a convenience function that will try to locate a UTLLRN_ModelingObjectsCreationAPI instance in the ToolManager's ContextObjectStore,
 * and then call UTLLRN_ModelingObjectsCreationAPI::CreateMeshObject()
 * @return a results data structure, containing a result code and information about any new objects created
 */
TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateMeshObjectResult CreateMeshObject(UInteractiveToolManager* ToolManager, FTLLRN_CreateMeshObjectParams&& CreateMeshParams);


/**
 * Create a new texture object based on the data in CreateTexParams.
 * This is a convenience function that will try to locate a UTLLRN_ModelingObjectsCreationAPI instance in the ToolManager's ContextObjectStore,
 * and then call UTLLRN_ModelingObjectsCreationAPI::CreateTextureObject()
 * @return a results data structure, containing a result code and information about any new objects created
 */
TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateTextureObjectResult CreateTextureObject(UInteractiveToolManager* ToolManager, FTLLRN_CreateTextureObjectParams&& CreateTexParams);


/**
 * Create a new material object based on the data in CreateMaterialParams.
 * This is a convenience function that will try to locate a UTLLRN_ModelingObjectsCreationAPI instance in the ToolManager's ContextObjectStore,
 * and then call UTLLRN_ModelingObjectsCreationAPI::CreateMaterialObject()
 * @return a results data structure, containing a result code and information about any new objects created
 */
TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateMaterialObjectResult CreateMaterialObject(UInteractiveToolManager* ToolManager, FTLLRN_CreateMaterialObjectParams&& CreateMaterialParams);


/**
 * Create a new actor based on the data in TLLRN_CreateActorParams.
 * This is a convenience function that will try to locate a UTLLRN_ModelingObjectsCreationAPI instance in the ToolManager's ContextObjectStore,
 * and then call UTLLRN_ModelingObjectsCreationAPI::CreateNewActor()
 * @return a results data structure, containing a result code and information about any new objects created
 */
TLLRN_MODELINGCOMPONENTS_API FTLLRN_CreateActorResult CreateNewActor(UInteractiveToolManager* ToolManager, FTLLRN_CreateActorParams&& TLLRN_CreateActorParams);



/**
 * Strip the appended auto-generated _UUID suffix on the given string, if we can detect one
 * @param InputName input string that may have _UUID suffix
 * @return InputName without _UUID suffix
 */
TLLRN_MODELINGCOMPONENTS_API FString StripGeneratedAssetSuffixFromName( FString InputName );


/**
* Generate a N-letter GUID string that contains only hex digits,
* and contains at least one letter and one number. Used to create _UUID suffixes
* for making asset names unique, etc.
*/
TLLRN_MODELINGCOMPONENTS_API FString GenerateRandomShortHexString(int32 NumChars = 8);


/**
 * Look up the name for the Asset underlying the given Component, if there is one.
 * Optionally Strip off the appended auto-generated UUID string if we can detect one.
 * If there is not a known underlying asset, returns the Component's name
 * @param Component the Component we want the Asset Base Name for
 * @param bRemoveAutoGeneratedSuffixes strip off generated UUID suffix if one is detected (default true)
 * @return the Name for the Component
 */
TLLRN_MODELINGCOMPONENTS_API FString GetComponentAssetBaseName(
	UPrimitiveComponent* Component,
	bool bRemoveAutoGeneratedSuffixes = true);

}
}

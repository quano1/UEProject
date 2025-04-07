// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Misc/Guid.h"

// Custom serialization version for changes made in Dev-Anim stream
struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigObjectVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded,

		// Added execution pins and removed hierarchy ref pins
		RemovalOfHierarchyRefPins,

		// Refactored operators to store FCachedPropertyPath instead of string
		OperatorsStoringPropertyPaths,

		// Introduced new RigVM as a backend
		SwitchedToRigVM,

		// Added a new transform as part of the control
		ControlOffsetTransform,

		// Using a cache data structure for key indices now
		TLLRN_RigElementKeyCache,

		// Full variable support
		BlueprintVariableSupport,

		// Hierarchy V2.0
		TLLRN_RigHierarchyV2,

		// TLLRN_RigHierarchy to support multi component parent constraints
		TLLRN_RigHierarchyMultiParentConstraints,

		// TLLRN_RigHierarchy now supports space favorites per control
		TLLRN_RigHierarchyControlSpaceFavorites,

		// TLLRN_RigHierarchy now stores min and max values as float storages 
		StorageMinMaxValuesAsFloatStorage,

		// RenameGizmoToShape 
		RenameGizmoToShape,

		// BoundVariableWithInjectionNode 
		BoundVariableWithInjectionNode,

		// Switch limit control over to per channel limits 
		PerChannelLimits,

		// Removed the parent cache for multi parent elements 
		RemovedMultiParentParentCache,

		// Deprecation of parameters
		RemoveParameters,

		// Added rig curve element value state flag
		CurveElementValueStateFlag,

		// Added the notion of a per control animation type
		ControlAnimationType,

		// Added preferred permutation for templates
		TemplatesPreferredPermutatation,

		// Added preferred euler angles to controls
		PreferredEulerAnglesForControls,

		// Added rig hierarchy element metadata
		HierarchyElementMetadata,

		// Converted library nodes to templates
		LibraryNodeTemplates,

		// Controls to be able specify space switch targets
		RestrictSpaceSwitchingForControls,

		// Controls to be able specify which channels should be visible in sequencer
		ControlTransformChannelFiltering,

		// Store function information (and compilation data) in blueprint generated class
		StoreFunctionsInGeneratedClass,

		// Hierarchy storing previous names
		TLLRN_RigHierarchyStoringPreviousNames,

		// Control supporting preferred rotation order
		TLLRN_RigHierarchyControlPreferredRotationOrder,

		// Last bit required for Control supporting preferred rotation order
		TLLRN_RigHierarchyControlPreferredRotationOrderFlag,

		// Element metadata is now stored on UTLLRN_RigHierarchy, rather than FTLLRN_RigBaseElement
		TLLRN_RigHierarchyStoresElementMetadata,

		// Add type (primary, secondary) and optional bool to FTLLRN_RigConnectorSettings
		ConnectorsWithType,

		// Add parent key to control rig pose
		TLLRN_RigPoseWithParentKey,

		// Physics solvers stored on hierarchy
		TLLRN_ControlRigStoresPhysicsSolvers,

		// Moved the element storage into separate buffers
		TLLRN_RigHierarchyIndirectElementStorage,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FTLLRN_ControlRigObjectVersion() {}
};

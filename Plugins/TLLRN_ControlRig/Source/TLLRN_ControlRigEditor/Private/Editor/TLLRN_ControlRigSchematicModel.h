// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_ModularRigModel.h"
#include "SchematicGraphPanel/SSchematicGraphPanel.h"
#include "Rigs/TLLRN_RigHierarchyDefines.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "Input/DragAndDrop.h"

class FTLLRN_ControlRigEditor;
class UTLLRN_ControlRig;
class UTLLRN_RigHierarchy;
struct FTLLRN_RigBaseElement;

/** Node for the schematic view */
class FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode : public FSchematicGraphGroupNode
{
public:

	SCHEMATICGRAPHNODE_BODY(FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode, FSchematicGraphGroupNode)

	virtual ~FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode() override {}
	
	const FTLLRN_RigElementKey& GetKey() const { return Key; }
	virtual FString GetDragDropDecoratorLabel() const override;
	virtual bool IsAutoScaleEnabled() const override { return true; }
	virtual bool IsDragSupported() const override;
	virtual const FText& GetLabel() const override;
	
protected:

	FTLLRN_RigElementKey Key;

	friend class FTLLRN_ControlRigSchematicModel;
};

/** Link between two rig element schematic nodes */
class FTLLRN_ControlRigSchematicTLLRN_RigElementKeyLink : public FSchematicGraphLink
{
public:

	SCHEMATICGRAPHLINK_BODY(FTLLRN_ControlRigSchematicTLLRN_RigElementKeyLink, FSchematicGraphLink)

	virtual ~FTLLRN_ControlRigSchematicTLLRN_RigElementKeyLink() override {}
	
	const FTLLRN_RigElementKey& GetSourceKey() const { return SourceKey; }
	const FTLLRN_RigElementKey& GetTargetKey() const { return TargetKey; }
	
protected:

	FTLLRN_RigElementKey SourceKey;
	FTLLRN_RigElementKey TargetKey;

	friend class FTLLRN_ControlRigSchematicModel;
};

class FTLLRN_ControlRigSchematicWarningTag : public FSchematicGraphTag
{
public:

	SCHEMATICGRAPHTAG_BODY(FTLLRN_ControlRigSchematicWarningTag, FSchematicGraphTag)

	FTLLRN_ControlRigSchematicWarningTag();
	virtual ~FTLLRN_ControlRigSchematicWarningTag() override {}
};


/** Model for the schematic views */
class FTLLRN_ControlRigSchematicModel : public FSchematicGraphModel
{
public:

	SCHEMATICGRAPHNODE_BODY(FTLLRN_ControlRigSchematicModel, FSchematicGraphModel)

	virtual ~FTLLRN_ControlRigSchematicModel() override;
	
	void SetEditor(const TSharedRef<FTLLRN_ControlRigEditor>& InEditor);
	virtual void ApplyToPanel(SSchematicGraphPanel* InPanel) override;

	virtual void Reset() override;
	virtual void Tick(float InDeltaTime) override;
	FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* AddElementKeyNode(const FTLLRN_RigElementKey& InKey, bool bNotify = true);
	const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* FindElementKeyNode(const FTLLRN_RigElementKey& InKey) const;
	FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* FindElementKeyNode(const FTLLRN_RigElementKey& InKey);
	bool ContainsElementKeyNode(const FTLLRN_RigElementKey& InKey) const;
	virtual bool RemoveNode(const FGuid& InGuid) override;
	bool RemoveElementKeyNode(const FTLLRN_RigElementKey& InKey);
	FTLLRN_ControlRigSchematicTLLRN_RigElementKeyLink* AddElementKeyLink(const FTLLRN_RigElementKey& InSourceKey, const FTLLRN_RigElementKey& InTargetKey, bool bNotify = true);
	void UpdateElementKeyNodes();
	void UpdateElementKeyLinks();
	void UpdateTLLRN_ControlRigContent();
	void UpdateConnector(const FTLLRN_RigElementKey& InElementKey);

	void OnSetObjectBeingDebugged(UObject* InObject);
	void HandleTLLRN_ModularRigModified(ETLLRN_ModularRigNotification InNotification, const FTLLRN_RigModuleReference* InModule);

	virtual bool IsAutoGroupingEnabled() const { return false; }
	virtual FSchematicGraphGroupNode* AddAutoGroupNode() override;
	virtual FVector2d GetPositionForNode(const FSchematicGraphNode* InNode) const override;
	virtual bool GetPositionAnimationEnabledForNode(const FSchematicGraphNode* InNode) const override;
	virtual int32 GetNumLayersForNode(const FSchematicGraphNode* InNode) const override;
	const FSlateBrush* GetBrushForKey(const FTLLRN_RigElementKey& InKey, const FSchematicGraphNode* InNode) const;
	virtual const FSlateBrush* GetBrushForNode(const FSchematicGraphNode* InNode, int32 InLayerIndex) const override;
	virtual FLinearColor GetColorForNode(const FSchematicGraphNode* InNode, int32 InLayerIndex) const override;
	virtual ESchematicGraphVisibility::Type GetVisibilityForNode(const FSchematicGraphNode* InNode) const override;
	virtual const FSlateBrush* GetBrushForLink(const FSchematicGraphLink* InLink) const override;
	virtual FLinearColor GetColorForLink(const FSchematicGraphLink* InLink) const override;
	virtual ESchematicGraphVisibility::Type GetVisibilityForTag(const FSchematicGraphTag* InTag) const override;
	virtual const FText GetToolTipForTag(const FSchematicGraphTag* InTag) const override;
	virtual bool GetForwardedNodeForDrag(FGuid& InOutGuid) const override;
	virtual bool GetContextMenuForNode(const FSchematicGraphNode* InNode, FMenuBuilder& OutMenu) const override;

	static TArray<FTLLRN_RigElementKey> GetElementKeysFromDragDropEvent(const FDragDropOperation& InDragDropOperation, const UTLLRN_ControlRig* InTLLRN_ControlRig);

private:

	void ConfigureElementKeyNode(FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* InNode, const FTLLRN_RigElementKey& InKey);

	void HandleSchematicNodeClicked(SSchematicGraphPanel* InPanel, SSchematicGraphNode* InNode, const FPointerEvent& InMouseEvent);
	void HandleSchematicBeginDrag(SSchematicGraphPanel* InPanel, SSchematicGraphNode* InNode, const TSharedPtr<FDragDropOperation>& InDragDropOperation);
	void HandleSchematicEndDrag(SSchematicGraphPanel* InPanel, SSchematicGraphNode* InNode, const TSharedPtr<FDragDropOperation>& InDragDropOperation);
	void HandleSchematicEnterDrag(SSchematicGraphPanel* InPanel, const TSharedPtr<FDragDropOperation>& InDragDropOperation);
	void HandleSchematicLeaveDrag(SSchematicGraphPanel* InPanel, const TSharedPtr<FDragDropOperation>& InDragDropOperation);
	void HandleSchematicCancelDrag(SSchematicGraphPanel* InPanel, SSchematicGraphNode* InNode, const TSharedPtr<FDragDropOperation>& InDragDropOperation);
	void HandleSchematicDrop(SSchematicGraphPanel* InPanel, SSchematicGraphNode* InNode, const FDragDropEvent& InDragDropEvent);
	void HandlePostConstruction(UTLLRN_ControlRig* Subject, const FName& InEventName);
	bool IsConnectorResolved(const FTLLRN_RigElementKey& InConnectorKey, FTLLRN_RigElementKey* OutKey = nullptr) const;
	void OnShowCandidatesForConnector(const FTLLRN_RigElementKey& InConnectorKey);
	void OnShowCandidatesForConnector(const FTLLRN_RigModuleConnector* InModuleConnector);
	void OnShowCandidatesForMatches(const FTLLRN_ModularRigResolveResult& InMatches);
	void OnHideCandidatesForConnector();
	void OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);

	TWeakPtr<FTLLRN_ControlRigEditor> TLLRN_ControlRigEditor;
	TWeakObjectPtr<UTLLRN_ControlRigBlueprint> TLLRN_ControlRigBlueprint;
	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRigBeingDebuggedPtr;

	TArray<FGuid> TemporaryNodeGuids;
	TMap<FGuid, ESchematicGraphVisibility::Type> PreDragVisibilityPerNode;
	TMap<FTLLRN_RigElementKey, FGuid> TLLRN_RigElementKeyToGuid;
	mutable TMap<FSoftObjectPath, FSlateBrush> ModuleIcons;
	bool bUpdateElementKeyLinks = true;

	friend class FTLLRN_ControlRigEditor;
	friend class FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode;
};
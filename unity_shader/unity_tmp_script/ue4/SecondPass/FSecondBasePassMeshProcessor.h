#pragma once

#include "MeshPassProcessor.h"
#include "ScenePrivate.h"
#include "TranslucencyPass.h"

class FSecondBasePassMeshProcessor : public FMeshPassProcessor
{
public:

	FSecondBasePassMeshProcessor
	(
		const FScene* InScene,
		ERHIFeatureLevel::Type InFeatureLevel,
		const FSceneView* InViewIfDynamicMeshCommand,
		const FMeshPassProcessorRenderState& InDrawRenderState,
		FMeshPassDrawListContext* InDrawListContext,
		ETranslucencyPass::Type InTranslucencyPassType = ETranslucencyPass::TPT_MAX
	);

	virtual void AddMeshBatch
	(
		const FMeshBatch& RESTRICT MeshBatch,
		uint64 BatchElementMask,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		int32 StaticMeshId /* = -1 */
	) override final;

	FMeshPassProcessorRenderState PassDrawRenderState;
private:

	void Process
	(
		const FMeshBatch& MeshBatch,
		uint64 BatchElementMask,
		int32 staticMeshId,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,
		const FMaterial& RESTRICT MaterialResource,
		ERasterizerFillMode MeshFillMode,
		ERasterizerCullMode MeshCullMode
	);
};
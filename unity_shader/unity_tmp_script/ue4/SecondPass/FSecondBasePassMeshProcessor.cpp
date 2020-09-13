#include "FSecondBasePassMeshProcessor.h"
#include "BasePassRendering.h"



//#include "MobileVolumeLightPassRendering.h"
#include "MeshPassProcessor.inl"

#include "ScenePrivate.h"
#include "SceneRendering.h"
#include "Shader.h"
#include "GlobalShader.h"

/**
 * Vertex shader for rendering a single, constant color.
 */
class FMobileVolumeLightVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FMobileVolumeLightVS, MeshMaterial);

	/** Default constructor. */
	FMobileVolumeLightVS() {}

public:

	FMobileVolumeLightVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
		//DepthParameter.Bind(Initializer.ParameterMap, TEXT("InputDepth"), SPF_Mandatory);
		BindSceneTextureUniformBufferDependentOnShadingPath(Initializer, PassUniformBuffer);
	}

	//static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	//{
		//FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		//OutEnvironment.SetDefine(TEXT("USING_NDC_POSITIONS"), (uint32)(bUsingNDCPositions ? 1 : 0));
		//OutEnvironment.SetDefine(TEXT("USING_LAYERS"), (uint32)(bUsingVertexLayers ? 1 : 0));
	//}

	//void SetDepthParameter(FRHICommandList& RHICmdList, float Depth)
	//{
	//	SetShaderValue(RHICmdList, GetVertexShader(), DepthParameter, Depth);
	//}

	//// FShader interface.
	//virtual bool Serialize(FArchive& Ar) override
	//{
	//	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
	//	//Ar << DepthParameter;
	//	return bShaderHasOutdatedParameters;
	//}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		// Only compile for the default material and masked materials
		return Parameters.VertexFactoryType->SupportsPositionOnly() && Parameters.MaterialParameters.bIsSpecialEngineMaterial;
	}

	void GetShaderBindings(
		const FScene* Scene, 
		ERHIFeatureLevel::Type FeatureLevel, 
		const FPrimitiveSceneProxy* PrimitiveSceneProxy, 
		const FMaterialRenderProxy& MaterialRenderProxy, 
		const FMaterial& Material, 
		const FMeshPassProcessorRenderState& DrawRenderState, 
		const FMeshMaterialShaderElementData& ShaderElementData, 
		FMeshDrawSingleShaderBindings& ShaderBindings)
	{
		FMeshMaterialShader::GetShaderBindings(Scene, FeatureLevel, PrimitiveSceneProxy, MaterialRenderProxy, Material, DrawRenderState, ShaderElementData, ShaderBindings);
	}

	//static const TCHAR* GetSourceFilename()
	//{
	//	return TEXT("/Engine/Private/OneColorShader.usf");
	//}

	//static const TCHAR* GetFunctionName()
	//{
	//	return TEXT("MainVertexShader");
	//}

private:
	//FShaderParameter DepthParameter;
};


class FMobileVolumeLightPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FMobileVolumeLightPS, MeshMaterial);
public:

	FMobileVolumeLightPS() { }
	FMobileVolumeLightPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
		//ColorParameter.Bind( Initializer.ParameterMap, TEXT("DrawColorMRT"), SPF_Mandatory);
		BindSceneTextureUniformBufferDependentOnShadingPath(Initializer, PassUniformBuffer);
	}

	//virtual void SetColors(FRHICommandList& RHICmdList, const FLinearColor* Colors, int32 NumColors);

	// FShader interface.
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		// Only compile for the default material and masked materials
		return Parameters.VertexFactoryType->SupportsPositionOnly() && Parameters.MaterialParameters.bIsSpecialEngineMaterial;
	}

	void GetShaderBindings(const FScene* Scene, ERHIFeatureLevel::Type FeatureLevel, const FPrimitiveSceneProxy* PrimitiveSceneProxy, const FMaterialRenderProxy& MaterialRenderProxy, const FMaterial& Material, const FMeshPassProcessorRenderState& DrawRenderState, const FMeshMaterialShaderElementData& ShaderElementData, FMeshDrawSingleShaderBindings& ShaderBindings)
	{
		FMeshMaterialShader::GetShaderBindings(Scene, FeatureLevel, PrimitiveSceneProxy, MaterialRenderProxy, Material, DrawRenderState, ShaderElementData, ShaderBindings);
	}
	/** The parameter to use for setting the draw Color. */
	//FShaderParameter ColorParameter;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FMobileVolumeLightVS, TEXT("/Engine/Private/meshprocess.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FMobileVolumeLightPS, TEXT("/Engine/Private/meshprocess.usf"), TEXT("MainPS"), SF_Pixel);
IMPLEMENT_SHADERPIPELINE_TYPE_VS(MobileShaderPipeline, FMobileVolumeLightVS, true);

/*
FBasePassMeshProcessor::FBasePassMeshProcessor(
	const FScene* Scene,
	ERHIFeatureLevel::Type InFeatureLevel,
	const FSceneView* InViewIfDynamicMeshCommand,
	const FMeshPassProcessorRenderState& InDrawRenderState,
	FMeshPassDrawListContext* InDrawListContext,
	EFlags Flags,
	ETranslucencyPass::Type InTranslucencyPassType)
	: FMeshPassProcessor(Scene, InFeatureLevel, InViewIfDynamicMeshCommand, InDrawListContext)
	, PassDrawRenderState(InDrawRenderState)
	, TranslucencyPassType(InTranslucencyPassType)
	, bTranslucentBasePass(InTranslucencyPassType != ETranslucencyPass::TPT_MAX)
	, bEnableReceiveDecalOutput((Flags & EFlags::CanUseDepthStencil) == EFlags::CanUseDepthStencil)
	, EarlyZPassMode(Scene ? Scene->EarlyZPassMode : DDM_None)
{
}
*/

FSecondBasePassMeshProcessor::FSecondBasePassMeshProcessor( 
	const FScene* InScene, 
	ERHIFeatureLevel::Type InFeatureLevel, 
	const FSceneView* InViewIfDynamicMeshCommand, 
	const FMeshPassProcessorRenderState& InDrawRenderState, 
	FMeshPassDrawListContext* InDrawListContext, 
	ETranslucencyPass::Type InTranslucencyPassType /* = ETranslucencyPass::TPT_MAX */ 
)
	: FMeshPassProcessor(InScene, InFeatureLevel, InViewIfDynamicMeshCommand, InDrawListContext)
	, PassDrawRenderState(InDrawRenderState)
{}

void FSecondBasePassMeshProcessor::AddMeshBatch(
	const FMeshBatch& RESTRICT MeshBatch, 
	uint64 BatchElementMask, 
	const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, 
	int32 StaticMeshId
)
{
	const FMaterialRenderProxy* FallBackMaterialRenderProxyPtr = nullptr;
	const FMaterial& Material = MeshBatch.MaterialRenderProxy->GetMaterialWithFallback(Scene->GetFeatureLevel(), FallBackMaterialRenderProxyPtr);

	const EBlendMode BlendMode = Material.GetBlendMode();
	const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
	const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(MeshBatch, Material, OverrideSettings);
	const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(MeshBatch, Material, OverrideSettings);
	const bool bIsTranslucent = IsTranslucentBlendMode(BlendMode);

	if (
		!bIsTranslucent
		&& (!PrimitiveSceneProxy || PrimitiveSceneProxy->ShouldRenderInMainPass())
		&& ShouldIncludeDomainInMeshPass(Material.GetMaterialDomain())
		)
	{
		if (
			BlendMode == BLEND_Opaque
			&& MeshBatch.VertexFactory->SupportsPositionOnlyStream()
			)
		{
			const FMaterialRenderProxy& DefualtProxy = *UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
			const FMaterial& DefaltMaterial = *DefualtProxy.GetMaterial(Scene->GetFeatureLevel());

			Process(
				MeshBatch,
				BatchElementMask,
				StaticMeshId,
				PrimitiveSceneProxy,
				DefualtProxy,
				DefaltMaterial,
				MeshFillMode,
				MeshCullMode
			);
		}
	}
}

bool _UseShaderPipelines()
{
	static const auto* CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.ShaderPipelines"));
	return CVar && CVar->GetValueOnAnyThread() != 0;
}

void GetMobileVolumeLightShaders(
	const FMaterial& material,
	FVertexFactoryType* VertexFactoryType,
	ERHIFeatureLevel::Type FeatureLevel,
	TShaderRef<FBaseHS>& HullShader,
	TShaderRef<FBaseDS>& DomainShader,
	TShaderRef<FMobileVolumeLightVS>& VertexShader,
	TShaderRef<FMobileVolumeLightPS>& PixleShader,
	FShaderPipelineRef& ShaderPipeline
)
{
	ShaderPipeline = _UseShaderPipelines() ? material.GetShaderPipeline(&MobileShaderPipeline, VertexFactoryType) : FShaderPipelineRef();
	VertexShader = material.GetShader<FMobileVolumeLightVS>(VertexFactoryType); //ShaderPipeline.IsValid() ? ShaderPipeline->GetShader<FMobileVolumeLightVS>() : material.GetShader<FMobileVolumeLightVS>(VertexFactoryType);
	PixleShader = material.GetShader<FMobileVolumeLightPS>(VertexFactoryType);
}


void FSecondBasePassMeshProcessor::Process(
	const FMeshBatch& MeshBatch, 
	uint64 BatchElementMask, 
	int32 staticMeshId, 
	const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, 
	const FMaterialRenderProxy& RESTRICT MaterialRenderProxy, 
	const FMaterial& RESTRICT MaterialResource, 
	ERasterizerFillMode MeshFillMode, 
	ERasterizerCullMode MeshCullMode
)
{
	const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

	TMeshProcessorShaders<
		FMobileVolumeLightVS,
		FBaseHS,
		FBaseDS,
		FMobileVolumeLightPS
	>MobileVolumeLightShaders;

	FShaderPipelineRef ShaderPipeline;
	GetMobileVolumeLightShaders(
		MaterialResource,
		VertexFactory->GetType(),
		FeatureLevel,
		MobileVolumeLightShaders.HullShader,
		MobileVolumeLightShaders.DomainShader,
		MobileVolumeLightShaders.VertexShader,
		MobileVolumeLightShaders.PixelShader,
		ShaderPipeline
	);

	FDepthOnlyShaderElementData ShaderElementData(0.0f);
	ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, staticMeshId, true);

	const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(MobileVolumeLightShaders.VertexShader, MobileVolumeLightShaders.PixelShader);

	BuildMeshDrawCommands(
		MeshBatch,
		BatchElementMask,
		PrimitiveSceneProxy,
		MaterialRenderProxy,
		MaterialResource,
		PassDrawRenderState,
		MobileVolumeLightShaders,
		MeshFillMode,
		MeshCullMode,
		SortKey,
		EMeshPassFeatures::PositionOnly,
		ShaderElementData
	);
}



FMeshPassProcessor* CreateSecondBasePassProcessor(const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
	FMeshPassProcessorRenderState DrawRenderState(Scene->UniformBuffers.ViewUniformBuffer, Scene->UniformBuffers.OpaqueBasePassUniformBuffer);
	DrawRenderState.SetInstancedViewUniformBuffer(Scene->UniformBuffers.InstancedViewUniformBuffer);

	FExclusiveDepthStencil::Type BasePassDepthStencilAccess_NoDepthWrite = FExclusiveDepthStencil::Type(Scene->DefaultBasePassDepthStencilAccess & ~FExclusiveDepthStencil::DepthWrite);
	SetupBasePassState(BasePassDepthStencilAccess_NoDepthWrite, false, DrawRenderState);

	return new(FMemStack::Get()) FSecondBasePassMeshProcessor(Scene , Scene->GetFeatureLevel(),InViewIfDynamicMeshCommand, DrawRenderState, InDrawListContext);
}
FRegisterPassProcessorCreateFunction RegisterSecondBasePass(&CreateSecondBasePassProcessor, EShadingPath::Deferred, EMeshPass::SecondBasePass, EMeshPassFlags::MainView);
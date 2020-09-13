#include "MyTestPass.h"  

#include "DeferredShadingRenderer.h"
#include "AtmosphereRendering.h"
#include "ScenePrivate.h"
#include "Engine/TextureCube.h"
#include "PipelineStateCache.h"
#include "SceneTextureParameters.h"
#include "PixelShaderUtils.h"

#define LOCTEXT_NAMESPACE "RDGTestShader"  

DEFINE_LOG_CATEGORY_STATIC(RDGTestPassLog, Log, All)

class FRDGShaderTest : public FGlobalShader
{
	DECLARE_INLINE_TYPE_LAYOUT(FRDGShaderTest, NonVirtual);
public:

	FRDGShaderTest() {}

	FRDGShaderTest(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		SimpleColorVal.Bind(Initializer.ParameterMap, TEXT("SimpleColor"));
		MyTexture.Bind(Initializer.ParameterMap, TEXT("MyTexture"));
		MyTextureSampler.Bind(Initializer.ParameterMap, TEXT("MyTextureSampler"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);  
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("TEST_MICRO"), 1);
	}

	template<typename TShaderRHIParamRef>
	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const TShaderRHIParamRef ShaderRHI,
		const FLinearColor &MyColor,
		FTextureResource* MyTextureRes
	)
	{
		SetShaderValue(RHICmdList, ShaderRHI, SimpleColorVal, MyColor);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), MyTexture, MyTextureSampler, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), MyTextureRes->TextureRHI);
	}
private:
	LAYOUT_FIELD(FShaderParameter, SimpleColorVal);
	LAYOUT_FIELD(FShaderResourceParameter, MyTexture)
	LAYOUT_FIELD(FShaderResourceParameter, MyTextureSampler)
};

class FRDGShaderVS : public FRDGShaderTest
{
	DECLARE_SHADER_TYPE(FRDGShaderVS, Global);

public:
	FRDGShaderVS() {}

	FRDGShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FRDGShaderTest(Initializer)
	{

	}
};


class FRDGShaderPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FRDGShaderPS)
	SHADER_USE_PARAMETER_STRUCT(FRDGShaderPS, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureParameters, SceneTextures)
		SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureSamplerParameters, SceneTextureSamplers)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)
		SHADER_PARAMETER(FVector4, SimpleColor)

		RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_SHADER_TYPE(, FRDGShaderVS, TEXT("/Engine/Private/MyRDGPass.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FRDGShaderPS, TEXT("/Engine/Private/MyRDGPass.usf"), TEXT("MainPS"), SF_Pixel)

//inline void RDGAPI_render(FDeferredShadingSceneRenderer* scene_render, FRHICommandListImmediate& RHICmdList)
//{
//	UE_LOG(RDGTestPassLog, Warning, TEXT("call rdg api render"));
//	check(IsInRenderingThread());
//
//	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
//	//SceneContext.BeginRenderingSceneColor(RHICmdList, ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthRead_StencilWrite, true);
//
//	check(RHICmdList.IsOutsideRenderPass());
//
//	ESimpleRenderTargetMode RenderTargetMode = ESimpleRenderTargetMode::EExistingColorAndDepth;
//	FExclusiveDepthStencil DepthStencilAccess = FExclusiveDepthStencil::DepthRead_StencilWrite;
//
//	SCOPED_DRAW_EVENT(RHICmdList, MyTest123);
//	SceneContext.AllocSceneColor(RHICmdList);
//
//	ERenderTargetLoadAction ColorLoadAction, DepthLoadAction, StencilLoadAction;
//	ERenderTargetStoreAction ColorStoreAction, DepthStoreAction, StencilStoreAction;
//	DecodeRenderTargetMode(RenderTargetMode, ColorLoadAction, ColorStoreAction, DepthLoadAction, DepthStoreAction, StencilLoadAction, StencilStoreAction, DepthStencilAccess);
//
//	FRHIRenderPassInfo RPInfo(SceneContext.GetSceneColorSurface(), MakeRenderTargetActions(ColorLoadAction, ColorStoreAction));
//	RPInfo.DepthStencilRenderTarget.Action = MakeDepthStencilTargetActions(MakeRenderTargetActions(DepthLoadAction, DepthStoreAction), MakeRenderTargetActions(StencilLoadAction, StencilStoreAction));
//	RPInfo.DepthStencilRenderTarget.DepthStencilTarget = SceneContext.GetSceneDepthSurface();
//	RPInfo.DepthStencilRenderTarget.ExclusiveDepthStencil = DepthStencilAccess;
//
//	TransitionRenderPassTargets(RHICmdList, RPInfo);
//
//	RHICmdList.BeginRenderPass(RPInfo, TEXT("Testabcd"));
//
//	for (int32 ViewIndex = 0; ViewIndex < scene_render->Views.Num(); ViewIndex++)
//	{
//		const FViewInfo& View = scene_render->Views[ViewIndex];
//
//		if (View.IsPerspectiveProjection() == false)
//		{
//			continue;
//		}
//
//		{
//			// Get shaders.
//			FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(scene_render->FeatureLevel);
//			TShaderMapRef< FRDGShaderVS > VertexShader(GlobalShaderMap);
//			TShaderMapRef< FRDGShaderPS > PixelShader(GlobalShaderMap);
//
//			// Set the graphic pipeline state.
//			FGraphicsPipelineStateInitializer GraphicsPSOInit;
//			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
//			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
//			//GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
//			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_SourceAlpha>::GetRHI();
//			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
//			GraphicsPSOInit.PrimitiveType = PT_TriangleList;
//			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
//			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
//			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
//			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
//
//			// Update viewport.
//			RHICmdList.SetViewport(
//				View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
//
//			FLinearColor MyColor = FLinearColor();
//			MyColor.R = 1.0f;
//			// Update shader uniform parameters.
//			//PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), MyColor, MyTextureRes);
//
//			RHICmdList.SetStreamSource(0, GScreenRectangleVertexBuffer.VertexBufferRHI, 0);
//
//			uint32 InstanceCount = 1;
//			RHICmdList.DrawIndexedPrimitive(
//				GScreenRectangleIndexBuffer.IndexBufferRHI,
//				/*BaseVertexIndex=*/ 0,
//				/*MinIndex=*/ 0,
//				/*NumVertices=*/ 4,
//				/*StartIndex=*/ 0,
//				/*NumPrimitives=*/ 2,
//				/*NumInstances=*/ InstanceCount);
//		}
//
//	}
//
//	RHICmdList.EndRenderPass();
//}

void FDeferredShadingSceneRenderer::RenderRDGPass(FRHICommandListImmediate& RHICmdList)
{
	//RDGAPI_render(this, RHICmdList);

	check(IsInRenderingThread());

	FRDGBuilder GraphBuilder(RHICmdList);

	FSceneTextureParameters SceneTextures;
	SetupSceneTextureParameters(GraphBuilder, &SceneTextures);

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
	FRDGTextureRef SceneColor = GraphBuilder.RegisterExternalTexture(SceneContext.GetSceneColor());
	FRDGTextureRef SceneDepth = GraphBuilder.RegisterExternalTexture(SceneContext.SceneDepthZ, TEXT("SceneDepth"));

	for (FViewInfo& View : Views)
	{
		FRDGShaderPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FRDGShaderPS::FParameters>();

		PassParameters->SceneTextures = SceneTextures;
		SetupSceneTextureSamplers(&PassParameters->SceneTextureSamplers);
		PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;
		PassParameters->SimpleColor = FVector4(0.0, 0.0, 1.0, 1.0);

		PassParameters->RenderTargets[0] = FRenderTargetBinding(
			SceneColor, ERenderTargetLoadAction::ELoad);
		PassParameters->RenderTargets.DepthStencil = FDepthStencilBinding(SceneDepth, ERenderTargetLoadAction::ELoad, ERenderTargetLoadAction::ENoAction, FExclusiveDepthStencil::DepthRead_StencilNop);
		//RenderTargets.DepthStencil = FDepthStencilBinding(SceneDepth, ERenderTargetLoadAction::ELoad, ERenderTargetLoadAction::ENoAction, FExclusiveDepthStencil::DepthRead_StencilNop);

		TShaderMapRef<FRDGShaderPS> PixelShader(View.ShaderMap);
		ClearUnusedGraphResources(PixelShader, PassParameters);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("RDGPass Render Test"),
			PassParameters,
			ERDGPassFlags::Raster,
			[PassParameters, &View, PixelShader](FRHICommandList& RHICmdList)
		{
			RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 0.0);

			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			FPixelShaderUtils::InitFullscreenPipelineState(RHICmdList, View.ShaderMap, PixelShader, /* out */ GraphicsPSOInit);

			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_SourceAlpha, BO_Add, BF_Zero, BF_SourceAlpha>::GetRHI();
			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

			SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);

			FPixelShaderUtils::DrawFullscreenTriangle(RHICmdList);
		});
	}

	GraphBuilder.Execute();
}
#undef LOCTEXT_NAMESPACE
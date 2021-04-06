#include "DeferredShadingRenderer.h"
#include "SceneTextureParameters.h"
#include "PixelShaderUtils.h"

#define LOCTEXT_NAMESPACE "RDGTestShader"  

DEFINE_LOG_CATEGORY_STATIC(RDGTestPassLog, Log, All)


class FRDGShaderPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FRDGShaderPS)
	SHADER_USE_PARAMETER_STRUCT(FRDGShaderPS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TestTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, TestTextureSampler)

		SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureParameters, SceneTextures)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)
		SHADER_PARAMETER(FVector4, SimpleColor)

		RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_SHADER_TYPE(, FRDGShaderPS, TEXT("/Engine/Private/MyRDGPass.usf"), TEXT("MainPS"), SF_Pixel)

void FDeferredShadingSceneRenderer::RenderRDGPass(
	FRDGBuilder& GraphBuilder,
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTexturesUniformBuffer,
	FRDGTextureRef SceneColorTexture
)
{
	//RDGAPI_render(this, RHICmdList);

	check(IsInRenderingThread());

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(GraphBuilder.RHICmdList);
	FSceneTextureParameters SceneTextures = GetSceneTextureParameters(GraphBuilder, SceneTexturesUniformBuffer);

	for (FViewInfo& View : Views)
	{
		FRDGTextureRef OutputRT;
		{
			FRDGShaderPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FRDGShaderPS::FParameters>();
			PassParameters->SimpleColor = FVector4(0.0, 0.0, 1.0, 1.0);

			FRDGTextureDesc rt_desc = FRDGTextureDesc::Create2DDesc(FIntPoint(800, 600), PF_B8G8R8A8, FClearValueBinding::Green, TexCreate_None,
				TexCreate_ShaderResource | TexCreate_RenderTargetable | TexCreate_UAV, false);

			PassParameters->TestTexture = GraphBuilder.CreateTexture(rt_desc, TEXT("MyRDGTexture"));
			PassParameters->TestTextureSampler = TStaticSamplerState<SF_Point>::GetRHI();

			PassParameters->SceneTextures = SceneTextures;
			PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;

			FRDGTextureDesc OutputDesc = FRDGTextureDesc::Create2D(FIntPoint(800, 600), PF_R8G8B8A8, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_RenderTargetable | TexCreate_UAV);
			OutputRT = GraphBuilder.CreateTexture(OutputDesc, TEXT("OutputTest"));

			PassParameters->RenderTargets[0] = FRenderTargetBinding(OutputRT, ERenderTargetLoadAction::EClear);

			TShaderMapRef<FRDGShaderPS> PixelShader(View.ShaderMap);
			ClearUnusedGraphResources(PixelShader, PassParameters);

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("RDGPass Render Test Pass1"),
				PassParameters,
				ERDGPassFlags::Raster,
				[PassParameters, &View, PixelShader](FRHICommandList& RHICmdList)
			{
				uint32 LolStride = 0;
				FRHITexture* test_tex = PassParameters->TestTexture->GetRHI();

				//char* TextureDataPtr = (char*)RHICmdList.(test_tex, 0, EResourceLockMode::RLM_WriteOnly, LolStride, false);
				//
				//for (unit32 Row = 0; Row < test_tex->GetSizeY(); ++Row)
				//{
				//	uint32* PixelPtr = (unit32*)TextureDataPtr;
				//	for (unit32 Col = 0; Col < test_tex->GetSizeX(); ++Col)
				//	{
				//		uint8 r = 255;
				//		uint8 g = 0;
				//		unit8 b = 255;
				//		unit8 a = 255;
				//		*PixelPtr = r | (g << 8) | (b << 16) | (a << 24);
				//		PixelPtr++;
				//	}
				//	TextureDataPtr += LolStride;
				//}
				//RHICmdList.UnlockTexture2D(test_tex, 0, false);

				RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 0.0);

				FGraphicsPipelineStateInitializer GraphicsPSOInit;
				FPixelShaderUtils::InitFullscreenPipelineState(RHICmdList, View.ShaderMap, PixelShader, /* out */ GraphicsPSOInit);

				GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_SourceAlpha, BO_Add, BF_Zero, BF_SourceAlpha>::GetRHI();
				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

				SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);

				FPixelShaderUtils::DrawFullscreenTriangle(RHICmdList);
			});
		}

		// Pass2
		{
			FRDGShaderPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FRDGShaderPS::FParameters>();

			PassParameters->SimpleColor = FVector4(1.0, 0.0, 0.0, 1.0);

			PassParameters->TestTexture = OutputRT;
			PassParameters->TestTextureSampler = TStaticSamplerState<SF_Point>::GetRHI();

			PassParameters->SceneTextures = SceneTextures;
			PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;

			PassParameters->RenderTargets[0] = FRenderTargetBinding(
				SceneColorTexture, ERenderTargetLoadAction::ELoad);

			TShaderMapRef<FRDGShaderPS> PixelShader(View.ShaderMap);
			ClearUnusedGraphResources(PixelShader, PassParameters);

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("RDGPass Render Test Pass2"),
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
	}
}
#undef LOCTEXT_NAMESPACE 
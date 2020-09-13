#include "MyTestPass.h"  

#include "DeferredShadingRenderer.h"
#include "AtmosphereRendering.h"
#include "ScenePrivate.h"
#include "Engine/TextureCube.h"
#include "PipelineStateCache.h"

#define LOCTEXT_NAMESPACE "TestShader"  

/** The vertex data used to filter a texture. */
struct FTestVertex
{
public:
	FVector4 Position;
	FVector2D UV;
};

/** The filter vertex declaration resource type. */
class FTestVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	/** Destructor. */
	virtual ~FTestVertexDeclaration() {}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FTestVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTestVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTestVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

TGlobalResource<FTestVertexDeclaration> GTestVertexDeclaration;

class FMyTestPassVertexBuffer : public FVertexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI() override;
};

TGlobalResource<FMyTestPassVertexBuffer> GMyTestPassVertexBuffer;


class FMyTestPassIndexBuffer : public FIndexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI() override;
};

TGlobalResource<FMyTestPassIndexBuffer> GMyTestPassIndexBuffer;


void FMyTestPassVertexBuffer::InitRHI()
{
	TResourceArray<FTestVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
	Vertices.SetNumUninitialized(4);

	Vertices[0].Position = FVector4(1, 1, 0, 1);
	Vertices[0].UV = FVector2D(1, 1);

	Vertices[1].Position = FVector4(-1, 1, 0, 1);
	Vertices[1].UV = FVector2D(0, 1);

	Vertices[2].Position = FVector4(1, -1, 0, 1);
	Vertices[2].UV = FVector2D(1, 0);

	Vertices[3].Position = FVector4(-1, -1, 0, 1);
	Vertices[3].UV = FVector2D(0, 0);

	//The final two vertices are used for the triangle optimization (a single triangle spans the entire viewport )
	//Vertices[4].Position = FVector4(-1, 1, 0, 1);
	//Vertices[4].UV = FVector2D(-1, 1);

	//Vertices[5].Position = FVector4(1, -1, 0, 1);
	//Vertices[5].UV = FVector2D(1, -1);

	// Create vertex buffer. Fill buffer with initial data upon creation
	FRHIResourceCreateInfo CreateInfo(&Vertices);
	VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
}

void FMyTestPassIndexBuffer::InitRHI()
{
	// Indices 0 - 5 are used for rendering a quad. Indices 6 - 8 are used for triangle optimization.
	const uint16 Indices[] = { 0, 1, 2, 2, 1, 3 };// , 0, 4, 5

	TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
	uint32 NumIndices = UE_ARRAY_COUNT(Indices);
	IndexBuffer.AddUninitialized(NumIndices);
	FMemory::Memcpy(IndexBuffer.GetData(), Indices, NumIndices * sizeof(uint16));

	// Create index buffer. Fill buffer with initial data upon creation
	FRHIResourceCreateInfo CreateInfo(&IndexBuffer);
	IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateInfo);
}

class FMyShaderTest : public FGlobalShader
{
	DECLARE_INLINE_TYPE_LAYOUT(FMyShaderTest, NonVirtual);
public:

	FMyShaderTest() {}

	FMyShaderTest(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
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

class FShaderTestVS : public FMyShaderTest
{
	DECLARE_SHADER_TYPE(FShaderTestVS, Global);

public:
	FShaderTestVS() {}

	FShaderTestVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyShaderTest(Initializer)
	{

	}
};


class FShaderTestPS : public FMyShaderTest
{
	DECLARE_SHADER_TYPE(FShaderTestPS, Global);

public:
	FShaderTestPS() {}

	FShaderTestPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyShaderTest(Initializer)
	{

	}
};

IMPLEMENT_SHADER_TYPE(, FShaderTestVS, TEXT("/Engine/Private/MyTestPass.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FShaderTestPS, TEXT("/Engine/Private/MyTestPass.usf"), TEXT("MainPS"), SF_Pixel)

inline void CMDListAPI_render(FDeferredShadingSceneRenderer* scene_render, FRHICommandListImmediate& RHICmdList)
{
	check(IsInRenderingThread());

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
	//SceneContext.BeginRenderingSceneColor(RHICmdList, ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthRead_StencilWrite, true);

	check(RHICmdList.IsOutsideRenderPass());

	ESimpleRenderTargetMode RenderTargetMode = ESimpleRenderTargetMode::EExistingColorAndDepth;
	FExclusiveDepthStencil DepthStencilAccess = FExclusiveDepthStencil::DepthRead_StencilWrite;

	SCOPED_DRAW_EVENT(RHICmdList, MyTest123);
	SceneContext.AllocSceneColor(RHICmdList);

	ERenderTargetLoadAction ColorLoadAction, DepthLoadAction, StencilLoadAction;
	ERenderTargetStoreAction ColorStoreAction, DepthStoreAction, StencilStoreAction;
	DecodeRenderTargetMode(RenderTargetMode, ColorLoadAction, ColorStoreAction, DepthLoadAction, DepthStoreAction, StencilLoadAction, StencilStoreAction, DepthStencilAccess);

	FRHIRenderPassInfo RPInfo(SceneContext.GetSceneColorSurface(), MakeRenderTargetActions(ColorLoadAction, ColorStoreAction));
	RPInfo.DepthStencilRenderTarget.Action = MakeDepthStencilTargetActions(MakeRenderTargetActions(DepthLoadAction, DepthStoreAction), MakeRenderTargetActions(StencilLoadAction, StencilStoreAction));
	RPInfo.DepthStencilRenderTarget.DepthStencilTarget = SceneContext.GetSceneDepthSurface();
	RPInfo.DepthStencilRenderTarget.ExclusiveDepthStencil = DepthStencilAccess;

	TransitionRenderPassTargets(RHICmdList, RPInfo);

	RHICmdList.BeginRenderPass(RPInfo, TEXT("Testabcd"));

	for (int32 ViewIndex = 0; ViewIndex < scene_render->Views.Num(); ViewIndex++)
	{
		const FViewInfo& View = scene_render->Views[ViewIndex];

		if (View.IsPerspectiveProjection() == false)
		{
			continue;
		}

		{
			// Get shaders.
			FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(scene_render->FeatureLevel);
			TShaderMapRef< FShaderTestVS > VertexShader(GlobalShaderMap);
			TShaderMapRef< FShaderTestPS > PixelShader(GlobalShaderMap);

			// Set the graphic pipeline state.
			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			//GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_SourceAlpha>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;
			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GTestVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

			// Update viewport.
			RHICmdList.SetViewport(
				View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

			FLinearColor MyColor = FLinearColor();
			MyColor.R = 1.0f;
			// Update shader uniform parameters.
			//PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), MyColor, MyTextureRes);

			RHICmdList.SetStreamSource(0, GMyTestPassVertexBuffer.VertexBufferRHI, 0);

			uint32 InstanceCount = 1;
			RHICmdList.DrawIndexedPrimitive(
				GMyTestPassIndexBuffer.IndexBufferRHI,
				/*BaseVertexIndex=*/ 0,
				/*MinIndex=*/ 0,
				/*NumVertices=*/ 4,
				/*StartIndex=*/ 0,
				/*NumPrimitives=*/ 2,
				/*NumInstances=*/ InstanceCount);
		}

	}

	RHICmdList.EndRenderPass();
}

void FDeferredShadingSceneRenderer::RenderMyPass(FRHICommandListImmediate& RHICmdList)
{
	CMDListAPI_render(this, RHICmdList);
}
#undef  LOCTEXT_NAMESPACE
// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeferredShadingRenderer.h"

#if RHI_RAYTRACING

#include "RayTracingSkyLight.h"
#include "ScenePrivate.h"
#include "SceneRenderTargets.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "RHIResources.h"
#include "UniformBuffer.h"
#include "RayGenShaderUtils.h"
#include "PathTracingUniformBuffers.h"
#include "SceneTextureParameters.h"
#include "ScreenSpaceDenoise.h"
#include "ClearQuad.h"
#include "PostProcess/PostProcessing.h"
#include "PostProcess/SceneFilterRendering.h"
#include "Raytracing/RaytracingOptions.h"
#include "BlueNoise.h"
#include "SceneTextureParameters.h"
#include "RayTracingDefinitions.h"
#include "RayTracingDeferredMaterials.h"

#include "Raytracing/RayTracingDeferredMaterials.h"
#include "RenderCore/Public/ShaderPermutation.h"
#include "Raytracing/RayTracingLighting.h"
#include "SpriteIndexBuffer.h"

static TAutoConsoleVariable<int32> CVarRayTracingGlobalIllumination(
	TEXT("r.RayTracing.GlobalIllumination"),
	-1,
	TEXT("-1: Value driven by postprocess volume (default) \n")
	TEXT(" 0: ray tracing global illumination off \n")
	TEXT(" 1: ray tracing global illumination enabled"),
	ECVF_RenderThreadSafe
);

static int32 GRayTracingGlobalIlluminationSamplesPerPixel = -1;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationSamplesPerPixel(
	TEXT("r.RayTracing.GlobalIllumination.SamplesPerPixel"),
	GRayTracingGlobalIlluminationSamplesPerPixel,
	TEXT("Samples per pixel (default = -1 (driven by postprocesing volume))")
);

static TAutoConsoleVariable<int32> CVarRayTracingGlobalIlluminationEnableFinalGather(
	TEXT("r.RayTracing.GlobalIllumination.EnableFinalGather"),
	0,
	TEXT("Enables final gather algorithm for 1-bounce global illumination (default = 0)"),
	ECVF_RenderThreadSafe
);

static float GRayTracingGlobalIlluminationFinalGatherDistance = 10.0;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationFinalGatherDistance(
	TEXT("r.RayTracing.GlobalIllumination.FinalGatherDistance"),
	GRayTracingGlobalIlluminationFinalGatherDistance,
	TEXT("Maximum world-space distance for valid, reprojected final gather points (default = 10)")
);

static float GRayTracingGlobalIlluminationMaxRayDistance = 1.0e27;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationMaxRayDistance(
	TEXT("r.RayTracing.GlobalIllumination.MaxRayDistance"),
	GRayTracingGlobalIlluminationMaxRayDistance,
	TEXT("Max ray distance (default = 1.0e27)")
);

static float GRayTracingGlobalIlluminationMaxShadowDistance = -1.0;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationMaxShadowDistance(
	TEXT("r.RayTracing.GlobalIllumination.MaxShadowDistance"),
	GRayTracingGlobalIlluminationMaxShadowDistance,
	TEXT("Max shadow distance (default = -1.0, distance adjusted automatically so shadow rays do not hit the sky sphere) ")
);

static TAutoConsoleVariable<int32> CVarRayTracingGlobalIlluminationMaxBounces(
	TEXT("r.RayTracing.GlobalIllumination.MaxBounces"),
	-1,
	TEXT("Max bounces (default = -1 (driven by postprocesing volume))"),
	ECVF_RenderThreadSafe
);

static int32 GRayTracingGlobalIlluminationNextEventEstimationSamples = 2;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationNextEventEstimationSamples(
	TEXT("r.RayTracing.GlobalIllumination.NextEventEstimationSamples"),
	GRayTracingGlobalIlluminationNextEventEstimationSamples,
	TEXT("Number of sample draws for next-event estimation (default = 2)")
	TEXT("NOTE: This parameter is experimental")
);

static float GRayTracingGlobalIlluminationDiffuseThreshold = 0.01;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationDiffuseThreshold(
	TEXT("r.RayTracing.GlobalIllumination.DiffuseThreshold"),
	GRayTracingGlobalIlluminationDiffuseThreshold,
	TEXT("Diffuse luminance threshold for evaluating global illumination")
	TEXT("NOTE: This parameter is experimental")
);

static int32 GRayTracingGlobalIlluminationDenoiser = 1;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationDenoiser(
	TEXT("r.RayTracing.GlobalIllumination.Denoiser"),
	GRayTracingGlobalIlluminationDenoiser,
	TEXT("Denoising options (default = 1)")
);

static int32 GRayTracingGlobalIlluminationEvalSkyLight = 0;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationEvalSkyLight(
	TEXT("r.RayTracing.GlobalIllumination.EvalSkyLight"),
	GRayTracingGlobalIlluminationEvalSkyLight,
	TEXT("Evaluate SkyLight multi-bounce contribution")
	TEXT("NOTE: This parameter is experimental")
);

static int32 GRayTracingGlobalIlluminationUseRussianRoulette = 0;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationUseRussianRoulette(
	TEXT("r.RayTracing.GlobalIllumination.UseRussianRoulette"),
	GRayTracingGlobalIlluminationUseRussianRoulette,
	TEXT("Perform Russian Roulette to only cast diffuse rays on surfaces with brighter albedos (default = 0)")
	TEXT("NOTE: This parameter is experimental")
);

static float GRayTracingGlobalIlluminationScreenPercentage = 50.0;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationScreenPercentage(
	TEXT("r.RayTracing.GlobalIllumination.ScreenPercentage"),
	GRayTracingGlobalIlluminationScreenPercentage,
	TEXT("Screen percentage for ray tracing global illumination (default = 50)")
);

static TAutoConsoleVariable<int32> CVarRayTracingGlobalIlluminationEnableLightAttenuation(
	TEXT("r.RayTracing.GlobalIllumination.EnableLightAttenuation"),
	1,
	TEXT("Enables light attenuation when calculating irradiance during next-event estimation (default = 1)"),
	ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<int32> CVarRayTracingGlobalIlluminationEnableTwoSidedGeometry(
	TEXT("r.RayTracing.GlobalIllumination.EnableTwoSidedGeometry"),
	1,
	TEXT("Enables two-sided geometry when tracing GI rays (default = 1)"),
	ECVF_RenderThreadSafe
);

static int32 GRayTracingGlobalIlluminationRenderTileSize = 0;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationRenderTileSize(
	TEXT("r.RayTracing.GlobalIllumination.RenderTileSize"),
	GRayTracingGlobalIlluminationRenderTileSize,
	TEXT("Render ray traced global illumination in NxN pixel tiles, where each tile is submitted as separate GPU command buffer, allowing high quality rendering without triggering timeout detection. (default = 0, tiling disabled)")
);

static TAutoConsoleVariable<int32> CVarRayTracingGlobalIlluminationMaxLightCount(
	TEXT("r.RayTracing.GlobalIllumination.MaxLightCount"),
	RAY_TRACING_LIGHT_COUNT_MAXIMUM,
	TEXT("Enables two-sided geometry when tracing GI rays (default = 256)"),
	ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<int32> CVarRayTracingGlobalIlluminationFinalGatherSortMaterials(
	TEXT("r.RayTracing.GlobalIllumination.FinalGather.SortMaterials"),
	1,
	TEXT("Sets whether refected materials will be sorted before shading\n")
	TEXT("0: Disabled\n ")
	TEXT("1: Enabled, using Trace->Sort->Trace (Default)\n"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarRayTracingGlobalIlluminationFinalGatherSortTileSize(
	TEXT("r.RayTracing.GlobalIllumination.FinalGather.SortTileSize"),
	64,
	TEXT("Size of pixel tiles for sorted global illumination (default = 64)\n"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarRayTracingGlobalIlluminationFinalGatherSortSize(
	TEXT("r.RayTracing.GlobalIllumination.FinalGather.SortSize"),
	5,
	TEXT("Size of horizon for material ID sort\n")
	TEXT("0: Disabled\n")
	TEXT("1: 256 Elements\n")
	TEXT("2: 512 Elements\n")
	TEXT("3: 1024 Elements\n")
	TEXT("4: 2048 Elements\n")
	TEXT("5: 4096 Elements (Default)\n"),
	ECVF_RenderThreadSafe);

static float GRayTracingGlobalIllumiantionNormalBias = 25.0;
static FAutoConsoleVariableRef CVarGRayTracingGlobalIllumiantionNormalBias(
	TEXT("r.RayTracing.GlobalIllumination.NormalBias"),
	GRayTracingGlobalIllumiantionNormalBias,
	TEXT("Normal bias for ray tracing global illumination (default = 25)")
);

static int32 GRayTracingGlobalIllumiantionMultiBounce = true;
static FAutoConsoleVariableRef CVarGRayTracingGlobalIllumiantionMultiBounce(
	TEXT("r.RayTracing.GlobalIllumination.MultiBounce"),
	GRayTracingGlobalIllumiantionMultiBounce,
	TEXT("Normal bias for ray tracing global illumination (default = 1)")
);

static int32 GRayTracingGlobalIllumiantionDebugMode = 0;
static FAutoConsoleVariableRef CVarGRayTracingGlobalIllumiantionDebugMode(
	TEXT("r.RayTracing.GlobalIllumination.DebugMode"),
	GRayTracingGlobalIllumiantionDebugMode,
	TEXT("Normal bias for ray tracing global illumination (default = 0)")
);

static int32 GRayTracingGlobalIllumiantionSampleRandom = 1;
static FAutoConsoleVariableRef CVarGRayTracingGlobalIllumiantionSampleRandom(
	TEXT("r.RayTracing.GlobalIllumination.RandomSample"),
	GRayTracingGlobalIllumiantionSampleRandom,
	TEXT("Normal bias for ray tracing global illumination (default = 1)")
);

static float GRayTracingGlobalIllumiantionHysterisist = 0.96;
static FAutoConsoleVariableRef CVarGRayTracingGlobalIllumiantionHysterisist(
	TEXT("r.RayTracing.GlobalIllumination.Resist"),
	GRayTracingGlobalIllumiantionHysterisist,
	TEXT("Normal bias for ray tracing global illumination (default = 0.96)")
);

static int32 GRayTracingGlobalIlluminationForceRotateUpdate = 0;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationForceRotateUpdate(
	TEXT("r.RayTracing.GlobalIllumination.ForceRotateUpdate"),
	GRayTracingGlobalIlluminationForceRotateUpdate,
	TEXT(" 0: ray tracing global illumination rotate update off \n")
	TEXT(" 1: ray tracing global illumination rotate update on")
);

static int32 GRayTracingGlobalIlluminationDebugProbeLodLevel = 0;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationDebugProbeLodLevel(
	TEXT("r.RayTracing.GlobalIllumination.ShowProbeLod"),
	GRayTracingGlobalIlluminationDebugProbeLodLevel,
	TEXT(" 0: ray tracing global illumination show probe lod 0\n")
	TEXT(" 1: ray tracing global illumination show probe lod 1\n")
	TEXT(" 2: ray tracing global illumination show probe lod 2\n")
);

static int32 GRayTracingGlobalIlluminationDebugLodLevel = 3;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationDebugLodLevel(
	TEXT("r.RayTracing.GlobalIllumination.ForceLod"),
	GRayTracingGlobalIlluminationDebugLodLevel,
	TEXT(" 0: ray tracing global illumination lod 0\n")
	TEXT(" 1: ray tracing global illumination lod 1\n")
	TEXT(" 2: ray tracing global illumination lod 2\n")
	TEXT(" 3: ray tracing global illumination lod based on distance\n")
);

static int32 GRayTracingGlobalIlluminationDebugShowCascade = 0;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationDebugShowCascade(
	TEXT("r.RayTracing.GlobalIllumination.ShowCascade"),
	GRayTracingGlobalIlluminationDebugShowCascade,
	TEXT(" 0: ray tracing global illumination show cascade off\n")
	TEXT(" 1: ray tracing global illumination show cascade on\n")
);

static float GRayTracingGlobalIlluminationHysterisistPower = 8;
static FAutoConsoleVariableRef CVarRayTracingGlobalIlluminationHysterisistPower(
	TEXT("r.RayTracing.GlobalIllumination.InterpPower"),
	GRayTracingGlobalIlluminationHysterisistPower,
	TEXT(" ray tracing global illumination taa interp power\n")
);

DECLARE_GPU_STAT_NAMED(RayTracingGIBruteForce, TEXT("Ray Tracing GI: Brute Force"));
DECLARE_GPU_STAT_NAMED(RayTracingGIFinalGather, TEXT("Ray Tracing GI: Final Gather"));
DECLARE_GPU_STAT_NAMED(RayTracingGICreateGatherPoints, TEXT("Ray Tracing GI: Create Gather Points"));

static const int GlobalProbeSideLengthIrradiance = 6, GlobalProbeSideLengthDepth = 14;
static const FVector ProbeCounts[3] = { FVector(32, 32, 16), FVector(16, 16, 8), FVector(8, 8, 4) };
static FIntVector ProbeBudgets[3] = { FIntVector(12, 12, 7), FIntVector(16, 16, 8), FIntVector(8, 8, 4) };
static FIntVector4 ProbeUpdateOrder[3] = { FIntVector4(0, 0, 0, 0), FIntVector4(0, 1, 0, 1), FIntVector4(0, 1, 0, 2) };

void SetupLightParameters(
	const FScene& Scene,
	const FViewInfo& View,
	FPathTracingLightData* LightParameters)
{
	LightParameters->Count = 0;

	// Get the SkyLight color

	FSkyLightSceneProxy* SkyLight = Scene.SkyLight;
	FVector SkyLightColor = FVector(0.0f, 0.0f, 0.0f);
	if (SkyLight && SkyLight->bAffectGlobalIllumination)
	{
		SkyLightColor = FVector(SkyLight->GetEffectiveLightColor());
	}

	// Prepend SkyLight to light buffer
	// WARNING: Until ray payload encodes Light data buffer, the execution depends on this ordering!
	uint32 SkyLightIndex = 0;
	LightParameters->Type[SkyLightIndex] = 0;
	LightParameters->Color[SkyLightIndex] = SkyLightColor;
	LightParameters->Count++;

	uint32 MaxLightCount = FMath::Min(CVarRayTracingGlobalIlluminationMaxLightCount.GetValueOnRenderThread(), RAY_TRACING_LIGHT_COUNT_MAXIMUM);
	for (auto Light : Scene.Lights)
	{
		if (LightParameters->Count >= MaxLightCount) break;

		if (Light.LightSceneInfo->Proxy->HasStaticLighting() && Light.LightSceneInfo->IsPrecomputedLightingValid()) continue;
		if (!Light.LightSceneInfo->Proxy->AffectGlobalIllumination()) continue;

		FLightShaderParameters LightShaderParameters;
		Light.LightSceneInfo->Proxy->GetLightShaderParameters(LightShaderParameters);

		ELightComponentType LightComponentType = (ELightComponentType)Light.LightSceneInfo->Proxy->GetLightType();
		switch (LightComponentType)
		{
		case LightType_Directional:
		{
			LightParameters->Type[LightParameters->Count] = 2;
			LightParameters->Normal[LightParameters->Count] = LightShaderParameters.Direction;
			LightParameters->Color[LightParameters->Count] = LightShaderParameters.Color;
			LightParameters->Attenuation[LightParameters->Count] = 1.0 / LightShaderParameters.InvRadius;
			break;
		}
		case LightType_Rect:
		{
			LightParameters->Type[LightParameters->Count] = 3;
			LightParameters->Position[LightParameters->Count] = LightShaderParameters.Position;
			LightParameters->Normal[LightParameters->Count] = -LightShaderParameters.Direction;
			LightParameters->dPdu[LightParameters->Count] = FVector::CrossProduct(LightShaderParameters.Direction, LightShaderParameters.Tangent);
			LightParameters->dPdv[LightParameters->Count] = LightShaderParameters.Tangent;
			LightParameters->Color[LightParameters->Count] = LightShaderParameters.Color;
			LightParameters->Dimensions[LightParameters->Count] = FVector(2.0f * LightShaderParameters.SourceRadius, 2.0f * LightShaderParameters.SourceLength, 0.0f);
			LightParameters->Attenuation[LightParameters->Count] = 1.0 / LightShaderParameters.InvRadius;
			LightParameters->RectLightBarnCosAngle[LightParameters->Count] = LightShaderParameters.RectLightBarnCosAngle;
			LightParameters->RectLightBarnLength[LightParameters->Count] = LightShaderParameters.RectLightBarnLength;
			break;
		}
		case LightType_Point:
		default:
		{
			LightParameters->Type[LightParameters->Count] = 1;
			LightParameters->Position[LightParameters->Count] = LightShaderParameters.Position;
			// #dxr_todo: UE-72556 define these differences from Lit..
			LightParameters->Color[LightParameters->Count] = LightShaderParameters.Color / (4.0 * PI);
			float SourceRadius = 0.0; // LightShaderParameters.SourceRadius causes too much noise for little pay off at this time
			LightParameters->Dimensions[LightParameters->Count] = FVector(0.0, 0.0, SourceRadius);
			LightParameters->Attenuation[LightParameters->Count] = 1.0 / LightShaderParameters.InvRadius;
			break;
		}
		case LightType_Spot:
		{
			LightParameters->Type[LightParameters->Count] = 4;
			LightParameters->Position[LightParameters->Count] = LightShaderParameters.Position;
			LightParameters->Normal[LightParameters->Count] = -LightShaderParameters.Direction;
			// #dxr_todo: UE-72556 define these differences from Lit..
			LightParameters->Color[LightParameters->Count] = 4.0 * PI * LightShaderParameters.Color;
			float SourceRadius = 0.0; // LightShaderParameters.SourceRadius causes too much noise for little pay off at this time
			LightParameters->Dimensions[LightParameters->Count] = FVector(LightShaderParameters.SpotAngles, SourceRadius);
			LightParameters->Attenuation[LightParameters->Count] = 1.0 / LightShaderParameters.InvRadius;
			break;
		}
		};

		LightParameters->Count++;
	}
}

void SetupGlobalIlluminationSkyLightParameters(
	const FScene& Scene,
	FSkyLightData* SkyLightData)
{
	FSkyLightSceneProxy* SkyLight = Scene.SkyLight;

	SetupSkyLightParameters(Scene, SkyLightData);

	// Override the Sky Light color if it should not affect global illumination
	if (SkyLight && !SkyLight->bAffectGlobalIllumination)
	{
		SkyLightData->Color = FVector(0.0f);
	}
}

int32 GetRayTracingGlobalIlluminationSamplesPerPixel(const FViewInfo& View)
{
	int32 SamplesPerPixel = GRayTracingGlobalIlluminationSamplesPerPixel > -1 ? GRayTracingGlobalIlluminationSamplesPerPixel : View.FinalPostProcessSettings.RayTracingGISamplesPerPixel;
	return SamplesPerPixel;
}

bool ShouldRenderRayTracingGlobalIllumination(const FViewInfo& View)
{
	if (!IsRayTracingEnabled())
	{
		return (false);
	}

	if (GetRayTracingGlobalIlluminationSamplesPerPixel(View) <= 0)
	{
		return false;
	}
	
	if (GetForceRayTracingEffectsCVarValue() >= 0)
	{
		return GetForceRayTracingEffectsCVarValue() > 0;
	}

	int32 CVarRayTracingGlobalIlluminationValue = CVarRayTracingGlobalIllumination.GetValueOnRenderThread();
	if (CVarRayTracingGlobalIlluminationValue >= 0)
	{
		return (CVarRayTracingGlobalIlluminationValue > 0);
	}
	else
	{
		return View.FinalPostProcessSettings.RayTracingGIType > ERayTracingGlobalIlluminationType::Disabled;
	}
}

bool IsFinalGatherEnabled(const FViewInfo& View)
{

	int32 CVarRayTracingGlobalIlluminationValue = CVarRayTracingGlobalIllumination.GetValueOnRenderThread();
	if (CVarRayTracingGlobalIlluminationValue >= 0)
	{
		return CVarRayTracingGlobalIlluminationValue == 2;
	}

	return View.FinalPostProcessSettings.RayTracingGIType == ERayTracingGlobalIlluminationType::FinalGather;
}

bool IsNeteaseRTGIEnabled(const FViewInfo& View)
{

	int32 CVarRayTracingGlobalIlluminationValue = CVarRayTracingGlobalIllumination.GetValueOnRenderThread();
	if (CVarRayTracingGlobalIlluminationValue >= 0)
	{
		return CVarRayTracingGlobalIlluminationValue == 3;
	}

	return View.FinalPostProcessSettings.RayTracingGIType == ERayTracingGlobalIlluminationType::NetEase;
}

class FGlobalIlluminationRGS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGlobalIlluminationRGS)
	SHADER_USE_ROOT_PARAMETER_STRUCT(FGlobalIlluminationRGS, FGlobalShader)

	class FUseAttenuationTermDim : SHADER_PERMUTATION_BOOL("USE_ATTENUATION_TERM");
	class FEnableTwoSidedGeometryDim : SHADER_PERMUTATION_BOOL("ENABLE_TWO_SIDED_GEOMETRY");

	using FPermutationDomain = TShaderPermutationDomain<FUseAttenuationTermDim, FEnableTwoSidedGeometryDim>;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, SamplesPerPixel)
		SHADER_PARAMETER(uint32, MaxBounces)
		SHADER_PARAMETER(uint32, UpscaleFactor)
		SHADER_PARAMETER(float, MaxRayDistanceForGI)
		SHADER_PARAMETER(float, MaxRayDistanceForAO)
		SHADER_PARAMETER(float, MaxShadowDistance)
		SHADER_PARAMETER(float, NextEventEstimationSamples)
		SHADER_PARAMETER(float, DiffuseThreshold)
		SHADER_PARAMETER(uint32, EvalSkyLight)
		SHADER_PARAMETER(uint32, UseRussianRoulette)
		SHADER_PARAMETER(float, MaxNormalBias)
		SHADER_PARAMETER(uint32, RenderTileOffsetX)
		SHADER_PARAMETER(uint32, RenderTileOffsetY)

		SHADER_PARAMETER_SRV(RaytracingAccelerationStructure, TLAS)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, RWGlobalIlluminationUAV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, RWRayDistanceUAV)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)
		SHADER_PARAMETER_STRUCT_REF(FHaltonIteration, HaltonIteration)
		SHADER_PARAMETER_STRUCT_REF(FHaltonPrimes, HaltonPrimes)
		SHADER_PARAMETER_STRUCT_REF(FBlueNoise, BlueNoise)
		SHADER_PARAMETER_STRUCT_REF(FPathTracingLightData, LightParameters)
		SHADER_PARAMETER_STRUCT_REF(FSkyLightData, SkyLight)
		SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureParameters, SceneTextures)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SSProfilesTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, TransmissionProfilesLinearSampler)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FGlobalIlluminationRGS, "/Engine/Private/RayTracing/RayTracingGlobalIlluminationRGS.usf", "GlobalIlluminationRGS", SF_RayGen);

// Note: This constant must match the definition in RayTracingGatherPoints.ush
constexpr int32 MAXIMUM_GATHER_POINTS_PER_PIXEL = 32;

class FGlobalIlluminationNecaRGS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGlobalIlluminationNecaRGS)
	SHADER_USE_ROOT_PARAMETER_STRUCT(FGlobalIlluminationNecaRGS, FGlobalShader)

	class FDenoiserOutput : SHADER_PERMUTATION_BOOL("DIM_DENOISER_OUTPUT");

	class FDeferredMaterialMode : SHADER_PERMUTATION_ENUM_CLASS("DIM_DEFERRED_MATERIAL_MODE", EDeferredMaterialMode);

	using FPermutationDomain = TShaderPermutationDomain<FDenoiserOutput, FDeferredMaterialMode>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(int32, ShouldDoDirectLighting)
		SHADER_PARAMETER(int32, ShouldDoNecaGILighting)
		SHADER_PARAMETER(int32, ReflectedShadowsType)
		SHADER_PARAMETER(int32, ShouldDoEmissiveAndIndirectLighting)
		SHADER_PARAMETER(float, MaxDistance)
		SHADER_PARAMETER(float, MaxRayGenDistance)
		SHADER_PARAMETER(FVector, UpdateStartPosition)
		SHADER_PARAMETER(FVector, WorldGIStartPosition)
		SHADER_PARAMETER(FVector, UpdateProbeCount)
		SHADER_PARAMETER(FVector, WorldGIProbeCount)
		SHADER_PARAMETER(FVector, ProbeStep)
		SHADER_PARAMETER(FMatrix, RandomDirection)

		SHADER_PARAMETER(int32, ProbeSideLengthIrradiance)
		SHADER_PARAMETER(int32, ProbeSideLengthDepth)
		SHADER_PARAMETER(int32, RaysCount)
		SHADER_PARAMETER(float, NormalBias)
		SHADER_PARAMETER(float, EnergyPreservation)
		SHADER_PARAMETER(FVector4, TextureSizeIrradiance)
		SHADER_PARAMETER(FVector4, TextureSizeDepth)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, RadianceTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, RadianceSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DepthTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, DepthSampler)

		SHADER_PARAMETER_SRV(RaytracingAccelerationStructure, TLAS)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SSProfilesTexture)
		SHADER_PARAMETER_SRV(StructuredBuffer<FRTLightingData>, LightDataBuffer)

		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)
		SHADER_PARAMETER_STRUCT_REF(FSceneTexturesUniformParameters, SceneTexturesStruct)
		SHADER_PARAMETER_STRUCT_REF(FRaytracingLightDataPacked, LightDataPacked)
		SHADER_PARAMETER_STRUCT_REF(FReflectionUniformParameters, ReflectionStruct)
		SHADER_PARAMETER_STRUCT_REF(FFogUniformParameters, FogUniformParameters)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, RWGIRadianceRawUAV)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}
};

enum class EGatherType
{
	Irradiance = 0,
	Depth = 1,
	MAX
};

class FRayTracingGlobalIlluminationNecaGatherCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FRayTracingGlobalIlluminationNecaGatherCS)
	SHADER_USE_PARAMETER_STRUCT(FRayTracingGlobalIlluminationNecaGatherCS, FGlobalShader)

	class FRayTracingGlobalIlluminationNecaGatherPSTypeDim : SHADER_PERMUTATION_ENUM_CLASS("GATHER_TYPE", EGatherType);

	using FPermutationDomain = TShaderPermutationDomain<FRayTracingGlobalIlluminationNecaGatherPSTypeDim>;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(int32, ProbeSideLength)
		SHADER_PARAMETER(int32, RaysCount)
		SHADER_PARAMETER(float, HysterisistPower)
		SHADER_PARAMETER(float, IsDepth)
		SHADER_PARAMETER(float, DepthSharpness)
		SHADER_PARAMETER(float, MaxDistance)
		SHADER_PARAMETER(float, Hysterisist)
		SHADER_PARAMETER(FVector4, RawRadianceTextureSize)
		SHADER_PARAMETER(FVector4, UpdateStartPosition)
		SHADER_PARAMETER(FVector4, WorldGIProbeCount)
		SHADER_PARAMETER(FVector4, UpdateProbeCount)
		SHADER_PARAMETER(FMatrix, RandomOrientation)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GIRadianceTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, GIRadianceTextureSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GatherTargetTexturePrev)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(Texture2D, GatherTargetTexture)
	END_SHADER_PARAMETER_STRUCT()
};

class FRayTracingGlobalIlluminationNecaApplyPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FRayTracingGlobalIlluminationNecaApplyPS)
	SHADER_USE_PARAMETER_STRUCT(FRayTracingGlobalIlluminationNecaApplyPS, FGlobalShader)

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector, StartPosition)
		SHADER_PARAMETER_ARRAY(FVector, WorldGIProbeCount, [3])
		SHADER_PARAMETER_ARRAY(FVector, ProbeStep, [3])
		SHADER_PARAMETER(int32, GIDebug)
		SHADER_PARAMETER(int32, ForceLod)
		SHADER_PARAMETER(int32, ShowCascade)
		SHADER_PARAMETER(int32, ProbeSideLengthIrradiance)
		SHADER_PARAMETER(int32, ProbeSideLengthDepth)
		SHADER_PARAMETER(float, NormalBias)
		SHADER_PARAMETER(float, GIIntensity)

		SHADER_PARAMETER_ARRAY(FVector4, TextureSizeIrradiance, [3])
		SHADER_PARAMETER_ARRAY(FVector4, TextureSizeDepth, [3])

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, RadianceTextureLod0)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, RadianceTextureLod1)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, RadianceTextureLod2)
		SHADER_PARAMETER_SAMPLER(SamplerState, RadianceSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DepthTextureLod0)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DepthTextureLod1)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DepthTextureLod2)
		SHADER_PARAMETER_SAMPLER(SamplerState, DepthSampler)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)
		SHADER_PARAMETER_STRUCT_REF(FSceneTexturesUniformParameters, SceneTexturesStruct)
		RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FGlobalIlluminationNecaRGS, "/Engine/Private/RayTracing/RayTracingGlobalIlluminationNecaRGS.usf", "GlobalIlluminationNecaRGS", SF_RayGen);
IMPLEMENT_GLOBAL_SHADER(FRayTracingGlobalIlluminationNecaGatherCS, "/Engine/Private/RayTracing/RayTracingGlobalIlluminationNecaGatherCS.usf", "GlobalIlluminationNecaGatherCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FRayTracingGlobalIlluminationNecaApplyPS, "/Engine/Private/RayTracing/RayTracingGlobalIlluminationNecaApplyPS.usf", "GlobalIlluminationNecaApplyPS", SF_Pixel);

struct FGatherPoint
{
	FVector CreationPoint;
	FVector Position;
	FIntPoint Irradiance;
};

class FRayTracingGlobalIlluminationCreateGatherPointsRGS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FRayTracingGlobalIlluminationCreateGatherPointsRGS)
	SHADER_USE_ROOT_PARAMETER_STRUCT(FRayTracingGlobalIlluminationCreateGatherPointsRGS, FGlobalShader)

	class FUseAttenuationTermDim : SHADER_PERMUTATION_BOOL("USE_ATTENUATION_TERM");
	class FEnableTwoSidedGeometryDim : SHADER_PERMUTATION_BOOL("ENABLE_TWO_SIDED_GEOMETRY");
	class FDeferredMaterialMode : SHADER_PERMUTATION_ENUM_CLASS("DIM_DEFERRED_MATERIAL_MODE", EDeferredMaterialMode);

	using FPermutationDomain = TShaderPermutationDomain<FUseAttenuationTermDim, FEnableTwoSidedGeometryDim, FDeferredMaterialMode>;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, GatherSamplesPerPixel)
		SHADER_PARAMETER(uint32, SamplesPerPixel)
		SHADER_PARAMETER(uint32, SampleIndex)
		SHADER_PARAMETER(uint32, MaxBounces)
		SHADER_PARAMETER(uint32, UpscaleFactor)
		SHADER_PARAMETER(uint32, RenderTileOffsetX)
		SHADER_PARAMETER(uint32, RenderTileOffsetY)
		SHADER_PARAMETER(float, MaxRayDistanceForGI)
		SHADER_PARAMETER(float, MaxShadowDistance)
		SHADER_PARAMETER(float, NextEventEstimationSamples)
		SHADER_PARAMETER(float, DiffuseThreshold)
		SHADER_PARAMETER(float, MaxNormalBias)
		SHADER_PARAMETER(uint32, EvalSkyLight)
		SHADER_PARAMETER(uint32, UseRussianRoulette)

		// Scene data
		SHADER_PARAMETER_SRV(RaytracingAccelerationStructure, TLAS)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)

		// Sampling sequence
		SHADER_PARAMETER_STRUCT_REF(FHaltonIteration, HaltonIteration)
		SHADER_PARAMETER_STRUCT_REF(FHaltonPrimes, HaltonPrimes)
		SHADER_PARAMETER_STRUCT_REF(FBlueNoise, BlueNoise)

		// Light data
		SHADER_PARAMETER_STRUCT_REF(FPathTracingLightData, LightParameters)
		SHADER_PARAMETER_STRUCT_REF(FSkyLightData, SkyLight)

		// Shading data
		SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureParameters, SceneTextures)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SSProfilesTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, TransmissionProfilesLinearSampler)

		SHADER_PARAMETER(FIntPoint, GatherPointsResolution)
		SHADER_PARAMETER(FIntPoint, TileAlignedResolution)
		SHADER_PARAMETER(int32, SortTileSize)

		// Output
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<GatherPoints>, RWGatherPointsBuffer)
		// Optional indirection buffer used for sorted materials
		SHADER_PARAMETER_RDG_BUFFER_UAV(StructuredBuffer<FDeferredMaterialPayload>, MaterialBuffer)
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FRayTracingGlobalIlluminationCreateGatherPointsRGS, "/Engine/Private/RayTracing/RayTracingCreateGatherPointsRGS.usf", "RayTracingCreateGatherPointsRGS", SF_RayGen);

class FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS)
	SHADER_USE_ROOT_PARAMETER_STRUCT(FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS, FGlobalShader)

	class FUseAttenuationTermDim : SHADER_PERMUTATION_BOOL("USE_ATTENUATION_TERM");
	class FEnableTwoSidedGeometryDim : SHADER_PERMUTATION_BOOL("ENABLE_TWO_SIDED_GEOMETRY");
	class FDeferredMaterialMode : SHADER_PERMUTATION_ENUM_CLASS("DIM_DEFERRED_MATERIAL_MODE", EDeferredMaterialMode);

	using FPermutationDomain = TShaderPermutationDomain<FUseAttenuationTermDim, FEnableTwoSidedGeometryDim, FDeferredMaterialMode>;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, GatherSamplesPerPixel)
		SHADER_PARAMETER(uint32, SamplesPerPixel)
		SHADER_PARAMETER(uint32, SampleIndex)
		SHADER_PARAMETER(uint32, MaxBounces)
		SHADER_PARAMETER(uint32, UpscaleFactor)
		SHADER_PARAMETER(uint32, RenderTileOffsetX)
		SHADER_PARAMETER(uint32, RenderTileOffsetY)
		SHADER_PARAMETER(float, MaxRayDistanceForGI)
		SHADER_PARAMETER(float, MaxShadowDistance)
		SHADER_PARAMETER(float, NextEventEstimationSamples)
		SHADER_PARAMETER(float, DiffuseThreshold)
		SHADER_PARAMETER(float, MaxNormalBias)
		SHADER_PARAMETER(uint32, EvalSkyLight)
		SHADER_PARAMETER(uint32, UseRussianRoulette)

		// Scene data
		SHADER_PARAMETER_SRV(RaytracingAccelerationStructure, TLAS)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)

		// Sampling sequence
		SHADER_PARAMETER_STRUCT_REF(FHaltonIteration, HaltonIteration)
		SHADER_PARAMETER_STRUCT_REF(FHaltonPrimes, HaltonPrimes)
		SHADER_PARAMETER_STRUCT_REF(FBlueNoise, BlueNoise)

		// Light data
		SHADER_PARAMETER_STRUCT_REF(FPathTracingLightData, LightParameters)
		SHADER_PARAMETER_STRUCT_REF(FSkyLightData, SkyLight)

		// Shading data
		SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureParameters, SceneTextures)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SSProfilesTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, TransmissionProfilesLinearSampler)

		SHADER_PARAMETER(FIntPoint, GatherPointsResolution)
		SHADER_PARAMETER(FIntPoint, TileAlignedResolution)
		SHADER_PARAMETER(int32, SortTileSize)

		// Output
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<GatherPoints>, RWGatherPointsBuffer)
		// Optional indirection buffer used for sorted materials
		SHADER_PARAMETER_RDG_BUFFER_UAV(StructuredBuffer<FDeferredMaterialPayload>, MaterialBuffer)
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS, "/Engine/Private/RayTracing/RayTracingCreateGatherPointsRGS.usf", "RayTracingCreateGatherPointsTraceRGS", SF_RayGen);

class FRayTracingGlobalIlluminationFinalGatherRGS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FRayTracingGlobalIlluminationFinalGatherRGS)
	SHADER_USE_ROOT_PARAMETER_STRUCT(FRayTracingGlobalIlluminationFinalGatherRGS, FGlobalShader)

		class FUseAttenuationTermDim : SHADER_PERMUTATION_BOOL("USE_ATTENUATION_TERM");
	class FEnableTwoSidedGeometryDim : SHADER_PERMUTATION_BOOL("ENABLE_TWO_SIDED_GEOMETRY");

	using FPermutationDomain = TShaderPermutationDomain<FUseAttenuationTermDim, FEnableTwoSidedGeometryDim>;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, SampleIndex)
		SHADER_PARAMETER(uint32, SamplesPerPixel)
		SHADER_PARAMETER(uint32, UpscaleFactor)
		SHADER_PARAMETER(uint32, RenderTileOffsetX)
		SHADER_PARAMETER(uint32, RenderTileOffsetY)
		SHADER_PARAMETER(float, DiffuseThreshold)
		SHADER_PARAMETER(float, MaxNormalBias)
		SHADER_PARAMETER(float, FinalGatherDistance)

		// Scene data
		SHADER_PARAMETER_SRV(RaytracingAccelerationStructure, TLAS)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)

		// Shading data
		SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureParameters, SceneTextures)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SSProfilesTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, TransmissionProfilesLinearSampler)

		// Gather points
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<GatherPoints>, GatherPointsBuffer)
		SHADER_PARAMETER(FIntPoint, GatherPointsResolution)

		// Output
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, RWGlobalIlluminationUAV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, RWRayDistanceUAV)
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FRayTracingGlobalIlluminationFinalGatherRGS, "/Engine/Private/RayTracing/RayTracingFinalGatherRGS.usf", "RayTracingFinalGatherRGS", SF_RayGen);

void FDeferredShadingSceneRenderer::PrepareRayTracingGlobalIllumination(const FViewInfo& View, TArray<FRHIRayTracingShader*>& OutRayGenShaders)
{
	const bool bSortMaterials = CVarRayTracingGlobalIlluminationFinalGatherSortMaterials.GetValueOnRenderThread() != 0;

	// Declare all RayGen shaders that require material closest hit shaders to be bound
	for (int UseAttenuationTerm = 0; UseAttenuationTerm < 2; ++UseAttenuationTerm)
	{
		for (int EnableTwoSidedGeometry = 0; EnableTwoSidedGeometry < 2; ++EnableTwoSidedGeometry)
		{
			FGlobalIlluminationRGS::FPermutationDomain PermutationVector;
			PermutationVector.Set<FGlobalIlluminationRGS::FUseAttenuationTermDim>(CVarRayTracingGlobalIlluminationEnableLightAttenuation.GetValueOnRenderThread() != 0);
			PermutationVector.Set<FGlobalIlluminationRGS::FEnableTwoSidedGeometryDim>(EnableTwoSidedGeometry == 1);
			TShaderMapRef<FGlobalIlluminationRGS> RayGenerationShader(View.ShaderMap, PermutationVector);
			OutRayGenShaders.Add(RayGenerationShader.GetRayTracingShader());

			if (bSortMaterials)
			{
				// Gather
				{
					FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FPermutationDomain CreateGatherPointsPermutationVector;
					CreateGatherPointsPermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FUseAttenuationTermDim>(UseAttenuationTerm == 1);
					CreateGatherPointsPermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FEnableTwoSidedGeometryDim>(EnableTwoSidedGeometry == 1);
					CreateGatherPointsPermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FDeferredMaterialMode>(EDeferredMaterialMode::Gather);
					TShaderMapRef<FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS> CreateGatherPointsRayGenerationShader(View.ShaderMap, CreateGatherPointsPermutationVector);
					OutRayGenShaders.Add(CreateGatherPointsRayGenerationShader.GetRayTracingShader());
				}

				// Shade
				{
					FRayTracingGlobalIlluminationCreateGatherPointsRGS::FPermutationDomain CreateGatherPointsPermutationVector;
					CreateGatherPointsPermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FUseAttenuationTermDim>(UseAttenuationTerm == 1);
					CreateGatherPointsPermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FEnableTwoSidedGeometryDim>(EnableTwoSidedGeometry == 1);
					CreateGatherPointsPermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FDeferredMaterialMode>(EDeferredMaterialMode::Shade);
					TShaderMapRef<FRayTracingGlobalIlluminationCreateGatherPointsRGS> CreateGatherPointsRayGenerationShader(View.ShaderMap, CreateGatherPointsPermutationVector);
					OutRayGenShaders.Add(CreateGatherPointsRayGenerationShader.GetRayTracingShader());
				}
			}
			else
			{
				FRayTracingGlobalIlluminationCreateGatherPointsRGS::FPermutationDomain CreateGatherPointsPermutationVector;
				CreateGatherPointsPermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FUseAttenuationTermDim>(UseAttenuationTerm == 1);
				CreateGatherPointsPermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FEnableTwoSidedGeometryDim>(EnableTwoSidedGeometry == 1);
				CreateGatherPointsPermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FDeferredMaterialMode>(EDeferredMaterialMode::None);
				TShaderMapRef<FRayTracingGlobalIlluminationCreateGatherPointsRGS> CreateGatherPointsRayGenerationShader(View.ShaderMap, CreateGatherPointsPermutationVector);
				OutRayGenShaders.Add(CreateGatherPointsRayGenerationShader.GetRayTracingShader());
			}

			FRayTracingGlobalIlluminationFinalGatherRGS::FPermutationDomain GatherPassPermutationVector;
			GatherPassPermutationVector.Set<FRayTracingGlobalIlluminationFinalGatherRGS::FUseAttenuationTermDim>(UseAttenuationTerm == 1);
			GatherPassPermutationVector.Set<FRayTracingGlobalIlluminationFinalGatherRGS::FEnableTwoSidedGeometryDim>(EnableTwoSidedGeometry == 1);
			TShaderMapRef<FRayTracingGlobalIlluminationFinalGatherRGS> GatherPassRayGenerationShader(View.ShaderMap, GatherPassPermutationVector);
			OutRayGenShaders.Add(GatherPassRayGenerationShader.GetRayTracingShader());
		}
	}

}

void FDeferredShadingSceneRenderer::PrepareRayTracingNecaGlobalIllumination(const FViewInfo& View, TArray<FRHIRayTracingShader*>& OutRayGenShaders)
{
	const bool bSortMaterials = CVarRayTracingGlobalIlluminationFinalGatherSortMaterials.GetValueOnRenderThread() != 0;

	const EDeferredMaterialMode DeferredMaterialMode = bSortMaterials ? EDeferredMaterialMode::Shade : EDeferredMaterialMode::None;

	FGlobalIlluminationNecaRGS::FPermutationDomain PermutationVector;
	PermutationVector.Set<FGlobalIlluminationNecaRGS::FDeferredMaterialMode>(DeferredMaterialMode);
	TShaderMapRef<FGlobalIlluminationNecaRGS> RayGenerationShader(View.ShaderMap, PermutationVector);
	OutRayGenShaders.Add(RayGenerationShader.GetRayTracingShader());
}

FMatrix ComputeRandomRotation()
{
	// This approach is based on James Arvo's implementation from Graphics Gems 3 (pg 117-120).
	// Also available at: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.53.1357&rep=rep1&type=pdf

	// Setup a random rotation matrix using 3 uniform RVs
	float u1 = 2.f * PI * FMath::Rand() / RAND_MAX;
	float cos1 = FMath::Cos(u1);
	float sin1 = FMath::Sin(u1);

	float u2 = 2.f * PI * FMath::Rand() / RAND_MAX;
	float cos2 = FMath::Cos(u2);
	float sin2 = FMath::Sin(u2);

	float u3 = FMath::Rand() / RAND_MAX;
	float sq3 = 2.f * FMath::Sqrt(u3 * (1.f - u3));

	float s2 = 2.f * u3 * sin2 * sin2 - 1.f;
	float c2 = 2.f * u3 * cos2 * cos2 - 1.f;
	float sc = 2.f * u3 * sin2 * cos2;

	// Create the random rotation matrix
	float _11 = cos1 * c2 - sin1 * sc;
	float _12 = sin1 * c2 + cos1 * sc;
	float _13 = sq3 * cos2;

	float _21 = cos1 * sc - sin1 * s2;
	float _22 = sin1 * sc + cos1 * s2;
	float _23 = sq3 * sin2;

	float _31 = cos1 * (sq3 * cos2) - sin1 * (sq3 * sin2);
	float _32 = sin1 * (sq3 * cos2) + cos1 * (sq3 * sin2);
	float _33 = 1.f - 2.f * u3;

	// HLSL is column-major
	FMatrix randomRotation = FMatrix::Identity;
	randomRotation.M[0][0] = _11;
	randomRotation.M[0][1] = _12;
	randomRotation.M[0][2] = _13;
	randomRotation.M[1][0] = _21;
	randomRotation.M[1][1] = _22;
	randomRotation.M[1][2] = _23;
	randomRotation.M[2][0] = _31;
	randomRotation.M[2][1] = _32;
	randomRotation.M[2][2] = _33;

	return randomRotation;
}

//#pragma optimize("", off)
void InitRandomAndFrameUpdateLogic(FVector ProbeCount, const FViewInfo& View, FIntVector ProbeBudget, FIntVector& StartProbeID, FMatrix& RandomRotation, int LodIndex)
{
	FIntVector NewProbeGroupCount = FIntVector(FMath::CeilToInt(ProbeCount.X / ProbeBudget.X), FMath::CeilToInt(ProbeCount.Y / ProbeBudget.Y), FMath::CeilToInt(ProbeCount.Z / ProbeBudget.Z));
	int NewProbeGroupCountNum = NewProbeGroupCount.X * NewProbeGroupCount.Y * NewProbeGroupCount.Z;

	static FIntVector ProbeGroupCount[3] = { FIntVector(1, 1, 1), FIntVector(1, 1, 1), FIntVector(1, 1, 1) };
	static int ProbeGroupCountNum[3] = {1, 1, 1};

	static int ProbeGroupIndex[3] = { 0, 0, 0 };

	static int InitUpdateFrame[3] = { 256, 256, 256 };
	static int InitUpdateProgress[3] = { 0, 0, 0 };

	static int Lod0Index = 0;
	static int Lod1Index = 0;

	if (ProbeGroupCount[LodIndex] != NewProbeGroupCount || ProbeGroupCountNum[LodIndex] != NewProbeGroupCountNum)
	{
		ProbeGroupCount[LodIndex] = NewProbeGroupCount;
		ProbeGroupCountNum[LodIndex] = NewProbeGroupCountNum;
		ProbeGroupIndex[LodIndex] = 0;
		InitUpdateProgress[LodIndex] = 0;
	}

	ProbeGroupIndex[LodIndex]++;
	InitUpdateProgress[LodIndex]++;
	if (ProbeGroupIndex[LodIndex] >= ProbeGroupCountNum[LodIndex])
	{
		ProbeGroupIndex[LodIndex] = 0;
	}

	if (InitUpdateProgress[LodIndex] >= InitUpdateFrame[LodIndex] && !GRayTracingGlobalIlluminationForceRotateUpdate)
	{
		InitUpdateProgress[LodIndex] = InitUpdateFrame[LodIndex];

		FVector StartPosition = View.FinalPostProcessSettings.RayTracingGINecaStartPosition;
		FVector EndPosition = View.FinalPostProcessSettings.RayTracingGINecaStartPosition + 2 * View.FinalPostProcessSettings.RayTracingGINecaBounds;
		FVector ProbeStep = (EndPosition - StartPosition) / (ProbeCount - 1);

		FVector NearestProbeID = (View.ViewLocation - StartPosition) / ProbeStep;
		NearestProbeID = FVector(FMath::FloorToInt(NearestProbeID.X), FMath::FloorToInt(NearestProbeID.Y), FMath::FloorToInt(NearestProbeID.Z));

		FVector StartProbeIDVector = FVector(NearestProbeID.X - ProbeBudget.X / 2 + 1, NearestProbeID.Y - ProbeBudget.Y / 2 + 1, NearestProbeID.Z - FMath::FloorToInt(ProbeBudget.Z / 2));
		
		//if (LodIndex == 0)
		//{
		//	Lod0Index++;
		//	if (Lod0Index >= 4)
		//	{
		//		Lod0Index = 0;
		//	}

		//	int Lod0IndexX = Lod0Index % 2 - 1;
		//	int Lod0IndexY = Lod0Index / 2 - 1;
		//	StartProbeIDVector = FVector(NearestProbeID.X + Lod0IndexX * ProbeBudget.X, NearestProbeID.Y + Lod0IndexY * ProbeBudget.Y, NearestProbeID.Z - ProbeBudget.Z / 2 + 1);
		//}
		//else if (LodIndex == 1)
		//{
		//	Lod1Index++;
		//	if (Lod1Index >= 4)
		//	{
		//		Lod1Index = 0;
		//	}

		//	float Lod1OffsetX = (Lod1Index % 2 - 1);// *0.5 - 0.25;
		//	float Lod1OffsetY = (Lod1Index / 2 - 1);// *0.5 - 0.25;
		//	StartProbeIDVector = FVector(NearestProbeID.X + Lod1OffsetX * ProbeBudget.X, NearestProbeID.Y + Lod1OffsetY * ProbeBudget.Y, NearestProbeID.Z - ProbeBudget.Z / 2 + 1);
		//}
		
		StartProbeIDVector = StartProbeIDVector.ComponentMax(FVector(0, 0, 0)); 
		StartProbeIDVector = StartProbeIDVector.ComponentMin(FVector(ProbeCount.X - ProbeBudget.X, ProbeCount.Y - ProbeBudget.Y, ProbeCount.Z - ProbeBudget.Z));
		StartProbeID = FIntVector(StartProbeIDVector.X, StartProbeIDVector.Y, StartProbeIDVector.Z);
	}
	else
	{
		FIntVector ProbeGroupID = FIntVector(ProbeGroupIndex[LodIndex] % ProbeGroupCount[LodIndex].X,
			ProbeGroupIndex[LodIndex] / ProbeGroupCount[LodIndex].X % ProbeGroupCount[LodIndex].Y,
			ProbeGroupIndex[LodIndex] / (ProbeGroupCount[LodIndex].X * ProbeGroupCount[LodIndex].Y));

		StartProbeID = FIntVector(ProbeGroupID.X * ProbeBudget.X, ProbeGroupID.Y * ProbeBudget.Y, ProbeGroupID.Z * ProbeBudget.Z);
	}

	RandomRotation = GRayTracingGlobalIllumiantionSampleRandom ? ComputeRandomRotation() : FRotationMatrix(FRotator(0, 0, 0));
}
//#pragma optimize("", on)
#endif // RHI_RAYTRACING

void UpdateGIRayGenAndGather()
{

}

void FDeferredShadingSceneRenderer::RenderRayTracingNecaGlobalIllumination(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	TRefCountPtr<IPooledRenderTarget>& GlobalIlluminationRT
)
#if RHI_RAYTRACING
{
	int32 CVarRayTracingGlobalIlluminationValue = CVarRayTracingGlobalIllumination.GetValueOnRenderThread();
	if (CVarRayTracingGlobalIlluminationValue == 0 || (CVarRayTracingGlobalIlluminationValue == -1 && !IsNeteaseRTGIEnabled(View)))
	{
		return;
	}

	SCOPED_GPU_STAT(GraphBuilder.RHICmdList, RayTracingGIBruteForce);

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(GraphBuilder.RHICmdList);
	
	int32 ProbeSideLengthIrradiance = GlobalProbeSideLengthIrradiance;
	int32 ProbeSideLengthDepth = GlobalProbeSideLengthDepth;
	float Hysterisist = GRayTracingGlobalIllumiantionHysterisist;
	float DepthSharpness = 50.0f;
	float NormalBias = GRayTracingGlobalIllumiantionNormalBias;

	FVector StartPosition = View.FinalPostProcessSettings.RayTracingGINecaStartPosition;
	FVector EndPosition = View.FinalPostProcessSettings.RayTracingGINecaStartPosition + 2 * View.FinalPostProcessSettings.RayTracingGINecaBounds;

	static int32 GIRaysCount = 128;
	static bool needTextReGen[3] = { true, true, true };

	FVector ProbeSteps[3];
	int IrradianceConvolutionWidth[3];
	int IrradianceConvolutionHeight[3];
	int DepthConvolutionWidth[3];
	int DepthConvolutionHeight[3];
	int IrradianceWidth[3];
	int IrradianceHeight[3];
	int DepthWidth[3];
	int DepthHeight[3];

	for (int lod = 0; lod < 3; lod++)
	{
		ProbeSteps[lod] = (EndPosition - StartPosition) / (ProbeCounts[lod] - 1);
		IrradianceWidth[lod] = (ProbeSideLengthIrradiance + 2) * ProbeCounts[lod].X * ProbeCounts[lod].Z + 2;
		IrradianceHeight[lod] = (ProbeSideLengthIrradiance + 2) * ProbeCounts[lod].Y + 2;
		DepthWidth[lod] = (ProbeSideLengthDepth + 2) * ProbeCounts[lod].X * ProbeCounts[lod].Z + 2;
		DepthHeight[lod] = (ProbeSideLengthDepth + 2) * ProbeCounts[lod].Y + 2;
		IrradianceConvolutionWidth[lod] = (ProbeSideLengthIrradiance + 2) * ProbeBudgets[lod].X * ProbeBudgets[lod].Z;
		IrradianceConvolutionHeight[lod] = (ProbeSideLengthIrradiance + 2) * ProbeBudgets[lod].Y;
		DepthConvolutionWidth[lod] = (ProbeSideLengthDepth + 2) * ProbeBudgets[lod].X * ProbeBudgets[lod].Z;
		DepthConvolutionHeight[lod] = (ProbeSideLengthDepth + 2) * ProbeBudgets[lod].Y;
	}

	int UpdateLodLevel = 0;
	{
		static int LodFrameIndex = 0;

		if (LodFrameIndex >= 4)
		{
			LodFrameIndex = 0;
			SceneContext.GIRadianceCurrent = !SceneContext.GIRadianceCurrent;
		}
		UpdateLodLevel = ProbeUpdateOrder[View.FinalPostProcessSettings.RayTracingGINecaCascadeNumber - 1][LodFrameIndex];
		LodFrameIndex++;
	}

	TRefCountPtr<IPooledRenderTarget>* GIRadianceGatherRT = SceneContext.GIRadianceGather[SceneContext.GIRadianceCurrent];
	TRefCountPtr<IPooledRenderTarget>* GIDepthGatherRT = SceneContext.GIDepthGather[SceneContext.GIRadianceCurrent];
	TRefCountPtr<IPooledRenderTarget>* GIRadianceGatherRTPrev = SceneContext.GIRadianceGather[1 - SceneContext.GIRadianceCurrent];
	TRefCountPtr<IPooledRenderTarget>* GIDepthGatherRTPrev = SceneContext.GIDepthGather[1 - SceneContext.GIRadianceCurrent];

	// Render targets Raw
	FVector4 RawIrradianceTextureSize = FVector4(GIRaysCount, 
		ProbeBudgets[UpdateLodLevel].X * ProbeBudgets[UpdateLodLevel].Y * ProbeBudgets[UpdateLodLevel].Z, 
		1.0f / GIRaysCount, 1.0f / (ProbeBudgets[UpdateLodLevel].X * ProbeBudgets[UpdateLodLevel].Y * ProbeBudgets[UpdateLodLevel].Z));
	FRDGTextureRef GIRadianceRawTexture;
	{
		FRDGTextureDesc Desc = SceneContext.GetSceneColor()->GetDesc();/*todo resolution*/
		Desc.Extent = FIntPoint(RawIrradianceTextureSize.X, RawIrradianceTextureSize.Y);
		Desc.Format = PF_FloatRGBA;
		Desc.Flags &= ~(TexCreate_FastVRAM | TexCreate_Transient);
		GIRadianceRawTexture = GraphBuilder.CreateTexture(Desc, TEXT("RayTracingRadianceRaw"));
	}

	FSceneTexturesUniformParameters SceneTextures;
	SetupSceneTextureUniformParameters(SceneContext, FeatureLevel, ESceneTextureSetupMode::All, SceneTextures);

	for (int lod = 0; lod < View.FinalPostProcessSettings.RayTracingGINecaCascadeNumber; lod++)
	{
		if (lod != UpdateLodLevel)
			continue;
		FIntVector StartProbeId = FIntVector(0, 0, 0);
		FMatrix RandomRotation = FMatrix::Identity;
		InitRandomAndFrameUpdateLogic(ProbeCounts[lod], View, ProbeBudgets[UpdateLodLevel], StartProbeId, RandomRotation, lod);

		if (needTextReGen[lod])
		{
			InitRandomAndFrameUpdateLogic(ProbeCounts[lod], View, ProbeBudgets[UpdateLodLevel], StartProbeId, RandomRotation, lod);
		}

		FVector Dummy;
		float MaxDistance = 0;
		ProbeSteps[lod].ToDirectionAndLength(Dummy, MaxDistance);
		MaxDistance *= 1.5;

		float MaxRayGenDistance = ProbeSteps[0].GetAbsMax() * 8.0f;

		FIntPoint GatherResolutionRadiance = FIntPoint(0, 0);
		{
			FRDGTextureDesc Desc = SceneContext.GetSceneColor()->GetDesc();/*todo resolution*/
			GatherResolutionRadiance = Desc.Extent = FIntPoint(IrradianceWidth[lod], IrradianceHeight[lod]);
			Desc.Format = PF_FloatRGBA;
			if (!GIRadianceGatherRT[lod] || !GIRadianceGatherRTPrev[lod] || needTextReGen[lod])
			{
				GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, GIRadianceGatherRT[lod], TEXT("RTGI Radiance Gather 0_{lod} "));
				GraphBuilder.RHICmdList.ClearUAVFloat(GIRadianceGatherRT[lod]->GetRenderTargetItem().UAV, FLinearColor::Black);
				
				GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, GIRadianceGatherRTPrev[lod], TEXT("RTGI Radiance Gather 1_{lod}"));
				GraphBuilder.RHICmdList.ClearUAVFloat(GIRadianceGatherRTPrev[lod]->GetRenderTargetItem().UAV, FLinearColor::Black);
			}
		}

		FIntPoint GatherResolutionDepth = FIntPoint(0, 0);
		{
			FRDGTextureDesc Desc = SceneContext.GetSceneColor()->GetDesc();
			GatherResolutionDepth = Desc.Extent = FIntPoint(DepthWidth[lod], DepthHeight[lod]);
			Desc.Format = PF_G32R32F;

			if (!GIDepthGatherRT[lod] || !GIDepthGatherRTPrev[lod] || needTextReGen[lod])
			{
				GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, GIDepthGatherRT[lod], TEXT("RTGI Depth Gather 0_{lod}"));
				GraphBuilder.RHICmdList.ClearUAVFloat(GIDepthGatherRT[lod]->GetRenderTargetItem().UAV, FLinearColor::Black);

				GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, GIDepthGatherRTPrev[lod], TEXT("RTGI Depth Gather 1_{lod}"));
				GraphBuilder.RHICmdList.ClearUAVFloat(GIDepthGatherRTPrev[lod]->GetRenderTargetItem().UAV, FLinearColor::Black);
			}
		}

		needTextReGen[lod] = false;
		// Ray generation raw
		{
			FGlobalIlluminationNecaRGS::FParameters* PassParameters = GraphBuilder.AllocParameters<FGlobalIlluminationNecaRGS::FParameters>();
			PassParameters->ShouldDoDirectLighting = true;
			PassParameters->ShouldDoNecaGILighting = GRayTracingGlobalIllumiantionMultiBounce;
			PassParameters->ReflectedShadowsType = 1;
			PassParameters->ShouldDoEmissiveAndIndirectLighting = true;
			PassParameters->MaxDistance = MaxDistance;
			PassParameters->MaxRayGenDistance = MaxRayGenDistance;
			PassParameters->UpdateStartPosition = StartPosition + FVector(StartProbeId.X, StartProbeId.Y, StartProbeId.Z) * ProbeSteps[lod];
			PassParameters->WorldGIStartPosition = StartPosition;
			PassParameters->UpdateProbeCount = FVector(ProbeBudgets[UpdateLodLevel].X, ProbeBudgets[UpdateLodLevel].Y, ProbeBudgets[UpdateLodLevel].Z);
			PassParameters->WorldGIProbeCount = ProbeCounts[lod];
			PassParameters->ProbeStep = ProbeSteps[lod];

			PassParameters->RandomDirection = RandomRotation;
			PassParameters->TLAS = View.RayTracingScene.RayTracingSceneRHI->GetShaderResourceView();

			// TODO: should be converted to RDG
			TRefCountPtr<IPooledRenderTarget> SubsurfaceProfileRT((IPooledRenderTarget*)GetSubsufaceProfileTexture_RT(GraphBuilder.RHICmdList));
			if (!SubsurfaceProfileRT)
			{
				SubsurfaceProfileRT = GSystemTextures.BlackDummy;
			}
			PassParameters->SSProfilesTexture = GraphBuilder.RegisterExternalTexture(SubsurfaceProfileRT);

			PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;
			PassParameters->SceneTexturesStruct = CreateUniformBufferImmediate(SceneTextures, EUniformBufferUsage::UniformBuffer_SingleDraw);
			PassParameters->RWGIRadianceRawUAV = GraphBuilder.CreateUAV(GIRadianceRawTexture);

			PassParameters->ProbeSideLengthIrradiance = ProbeSideLengthIrradiance;
			PassParameters->ProbeSideLengthDepth = ProbeSideLengthDepth;
			PassParameters->RaysCount = GIRaysCount;
			PassParameters->NormalBias = NormalBias;
			PassParameters->EnergyPreservation = View.FinalPostProcessSettings.RayTracingGINecaEnergyPreservation;
			PassParameters->TextureSizeIrradiance = FVector4(IrradianceWidth[lod], IrradianceHeight[lod], 1.0f / IrradianceWidth[lod], 1.0f / IrradianceHeight[lod]);
			PassParameters->TextureSizeDepth = FVector4(DepthWidth[lod], DepthHeight[lod], 1.0f / DepthWidth[lod], 1.0f / DepthHeight[lod]);

			PassParameters->RadianceTexture = GraphBuilder.RegisterExternalTexture(GIRadianceGatherRT[lod]);
			PassParameters->RadianceSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParameters->DepthTexture = GraphBuilder.RegisterExternalTexture(GIDepthGatherRT[lod]);
			PassParameters->DepthSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

			PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;
			FStructuredBufferRHIRef LightingDataBuffer;
			PassParameters->LightDataPacked = View.RayTracingLightingDataUniformBuffer;
			PassParameters->LightDataBuffer = View.RayTracingLightingDataSRV;
			PassParameters->SceneTexturesStruct = CreateSceneTextureUniformBuffer(SceneContext, FeatureLevel, ESceneTextureSetupMode::All, EUniformBufferUsage::UniformBuffer_SingleFrame);
			PassParameters->ReflectionStruct = CreateReflectionUniformBuffer(View, EUniformBufferUsage::UniformBuffer_SingleFrame);
			PassParameters->FogUniformParameters = CreateFogUniformBuffer(View, EUniformBufferUsage::UniformBuffer_SingleFrame);

			auto RayGenerationShader = View.ShaderMap->GetShader<FGlobalIlluminationNecaRGS>();
			ClearUnusedGraphResources(RayGenerationShader, PassParameters);

			FIntPoint RayTracingResolution = FIntPoint(GIRaysCount, ProbeBudgets[UpdateLodLevel].X * ProbeBudgets[UpdateLodLevel].Y * ProbeBudgets[UpdateLodLevel].Z);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("GlobalIlluminationRayTracing %dx%d", RayTracingResolution.X, RayTracingResolution.Y),
				PassParameters,
				ERDGPassFlags::Compute,
				[PassParameters, this, &View, RayGenerationShader, RayTracingResolution](FRHICommandList& RHICmdList)
			{
				FRayTracingShaderBindingsWriter GlobalResources;
				SetShaderParameters(GlobalResources, RayGenerationShader, *PassParameters);

				FRHIRayTracingScene* RayTracingSceneRHI = View.RayTracingScene.RayTracingSceneRHI;
				RHICmdList.RayTraceDispatch(View.RayTracingMaterialPipeline, RayGenerationShader.GetRayTracingShader(), RayTracingSceneRHI, GlobalResources, RayTracingResolution.X, RayTracingResolution.Y);
			});
		}

		// Gather Radiance & Depth
		{
			FRayTracingGlobalIlluminationNecaGatherCS::FParameters *PassParameters = GraphBuilder.AllocParameters<FRayTracingGlobalIlluminationNecaGatherCS::FParameters>();
			PassParameters->ProbeSideLength = ProbeSideLengthIrradiance;
			PassParameters->RaysCount = GIRaysCount;
			PassParameters->IsDepth = 0.0f;
			PassParameters->DepthSharpness = DepthSharpness;
			PassParameters->HysterisistPower = GRayTracingGlobalIlluminationHysterisistPower;
			PassParameters->MaxDistance = MaxDistance;
			PassParameters->Hysterisist = Hysterisist;
			PassParameters->RandomOrientation = RandomRotation;
			PassParameters->RawRadianceTextureSize = RawIrradianceTextureSize;
			PassParameters->UpdateStartPosition = FVector4((StartProbeId.X + StartProbeId.Z * ProbeCounts[lod].X) * (ProbeSideLengthIrradiance + 2), StartProbeId.Y * (ProbeSideLengthIrradiance + 2), 0, 0);
			PassParameters->UpdateProbeCount = FVector4(ProbeBudgets[UpdateLodLevel].X, ProbeBudgets[UpdateLodLevel].Y, ProbeBudgets[UpdateLodLevel].Z, 0);
			PassParameters->WorldGIProbeCount = FVector4(ProbeCounts[lod].X, ProbeCounts[lod].Y, ProbeCounts[lod].Z, 0);
			PassParameters->GIRadianceTexture = GIRadianceRawTexture;
			PassParameters->GIRadianceTextureSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			FRDGTextureRef GatherTargetTexturePrev = GraphBuilder.RegisterExternalTexture(GIRadianceGatherRT[lod]);
			PassParameters->GatherTargetTexturePrev = GatherTargetTexturePrev;
			FRDGTextureRef GatherTargetTexture = GraphBuilder.RegisterExternalTexture(GIRadianceGatherRTPrev[lod]);
			PassParameters->GatherTargetTexture = GraphBuilder.CreateUAV(GatherTargetTexture);

			FRayTracingGlobalIlluminationNecaGatherCS::FPermutationDomain PermutationVector;
			PermutationVector.Set<FRayTracingGlobalIlluminationNecaGatherCS::FRayTracingGlobalIlluminationNecaGatherPSTypeDim>(EGatherType::Irradiance);
			TShaderMapRef<FRayTracingGlobalIlluminationNecaGatherCS> ComputeShader(View.ShaderMap, PermutationVector);
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("GlobalIllumination Radiance Gather"),
				ComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(FIntVector(IrradianceConvolutionWidth[UpdateLodLevel], IrradianceConvolutionHeight[UpdateLodLevel], 1), FIntVector(FComputeShaderUtils::kGolden2DGroupSize, FComputeShaderUtils::kGolden2DGroupSize, 1)));
		}

		// Gather Radiance & Depth
		{
			FRayTracingGlobalIlluminationNecaGatherCS::FParameters *PassParameters = GraphBuilder.AllocParameters<FRayTracingGlobalIlluminationNecaGatherCS::FParameters>();
			PassParameters->ProbeSideLength = ProbeSideLengthDepth;
			PassParameters->RaysCount = GIRaysCount;
			PassParameters->IsDepth = 1.0f;
			PassParameters->DepthSharpness = DepthSharpness;
			PassParameters->MaxDistance = MaxDistance;
			PassParameters->Hysterisist = Hysterisist;
			PassParameters->RandomOrientation = RandomRotation;
			PassParameters->RawRadianceTextureSize = RawIrradianceTextureSize;

			PassParameters->UpdateStartPosition = FVector4((StartProbeId.X + StartProbeId.Z * ProbeCounts[lod].X) * (ProbeSideLengthDepth + 2), StartProbeId.Y * (ProbeSideLengthDepth + 2), 0, 0);
			PassParameters->UpdateProbeCount = FVector4(ProbeBudgets[UpdateLodLevel].X, ProbeBudgets[UpdateLodLevel].Y, ProbeBudgets[UpdateLodLevel].Z, 0);
			PassParameters->WorldGIProbeCount = FVector4(ProbeCounts[lod].X, ProbeCounts[lod].Y, ProbeCounts[lod].Z, 0);
			PassParameters->GIRadianceTexture = GIRadianceRawTexture;
			PassParameters->GIRadianceTextureSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			FRDGTextureRef GatherTargetTexturePrev = GraphBuilder.RegisterExternalTexture(GIDepthGatherRT[lod]);
			PassParameters->GatherTargetTexturePrev = GatherTargetTexturePrev;
			FRDGTextureRef GatherTargetTexture = GraphBuilder.RegisterExternalTexture(GIDepthGatherRTPrev[lod]);
			PassParameters->GatherTargetTexture = GraphBuilder.CreateUAV(GatherTargetTexture);

			FRayTracingGlobalIlluminationNecaGatherCS::FPermutationDomain PermutationVector;
			PermutationVector.Set<FRayTracingGlobalIlluminationNecaGatherCS::FRayTracingGlobalIlluminationNecaGatherPSTypeDim>(EGatherType::Depth);
			TShaderMapRef<FRayTracingGlobalIlluminationNecaGatherCS> ComputeShader(View.ShaderMap, PermutationVector);
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("GlobalIllumination Depth Gather"),
				ComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(FIntVector(DepthConvolutionWidth[UpdateLodLevel], DepthConvolutionHeight[UpdateLodLevel], 1), FIntVector(FComputeShaderUtils::kGolden2DGroupSize, FComputeShaderUtils::kGolden2DGroupSize, 1)));
		}
	}

	// Compositing
	{
		FPooledRenderTargetDesc Desc = SceneContext.GetSceneColor()->GetDesc();
		Desc.Format = PF_FloatRGBA;
		Desc.Flags &= ~(TexCreate_FastVRAM | TexCreate_Transient);
		GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, GlobalIlluminationRT, TEXT("RayTracingGlobalIllumination"));
	}

	{
		FRayTracingGlobalIlluminationNecaApplyPS::FParameters *PassParameters = GraphBuilder.AllocParameters<FRayTracingGlobalIlluminationNecaApplyPS::FParameters>();

		PassParameters->StartPosition = StartPosition;
		PassParameters->GIDebug = GRayTracingGlobalIllumiantionDebugMode;
		PassParameters->ShowCascade = GRayTracingGlobalIlluminationDebugShowCascade;

		for (int i = 0; i < 3; i++)
		{
			PassParameters->WorldGIProbeCount[i] = ProbeCounts[i];
			PassParameters->ProbeStep[i] = ProbeSteps[i];
			PassParameters->TextureSizeIrradiance[i] = FVector4(IrradianceWidth[i], IrradianceHeight[i], 1.0f / IrradianceWidth[i], 1.0f / IrradianceHeight[i]);
			PassParameters->TextureSizeDepth[i] = FVector4(DepthWidth[i], DepthHeight[i], 1.0f / DepthWidth[i], 1.0f / DepthHeight[i]);
		}

		PassParameters->RadianceTextureLod0 = GIRadianceGatherRTPrev[0].IsValid() ? GraphBuilder.RegisterExternalTexture(GIRadianceGatherRTPrev[0]) : GraphBuilder.RegisterExternalTexture(GSystemTextures.WhiteDummy);
		PassParameters->RadianceTextureLod1 = GIRadianceGatherRTPrev[1].IsValid() ? GraphBuilder.RegisterExternalTexture(GIRadianceGatherRTPrev[1]) : GraphBuilder.RegisterExternalTexture(GSystemTextures.WhiteDummy);
		PassParameters->RadianceTextureLod2 = GIRadianceGatherRTPrev[2].IsValid() ? GraphBuilder.RegisterExternalTexture(GIRadianceGatherRTPrev[2]) : GraphBuilder.RegisterExternalTexture(GSystemTextures.WhiteDummy);

		PassParameters->DepthTextureLod0 = GIDepthGatherRTPrev[0].IsValid() ? GraphBuilder.RegisterExternalTexture(GIDepthGatherRTPrev[0]) : GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
		PassParameters->DepthTextureLod1 = GIDepthGatherRTPrev[1].IsValid() ? GraphBuilder.RegisterExternalTexture(GIDepthGatherRTPrev[1]) : GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
		PassParameters->DepthTextureLod2 = GIDepthGatherRTPrev[2].IsValid() ? GraphBuilder.RegisterExternalTexture(GIDepthGatherRTPrev[2]) : GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);

		PassParameters->ProbeSideLengthIrradiance = ProbeSideLengthIrradiance;
		PassParameters->ProbeSideLengthDepth = ProbeSideLengthDepth;
		PassParameters->NormalBias = NormalBias;
		PassParameters->GIIntensity = View.FinalPostProcessSettings.RayTracingGINecaIntensity;
		PassParameters->RadianceSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->DepthSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;
		PassParameters->SceneTexturesStruct = CreateUniformBufferImmediate(SceneTextures, EUniformBufferUsage::UniformBuffer_SingleDraw);
		PassParameters->RenderTargets[0] = FRenderTargetBinding(GraphBuilder.RegisterExternalTexture(GlobalIlluminationRT), ERenderTargetLoadAction::ELoad);

		PassParameters->ForceLod = GRayTracingGlobalIlluminationDebugLodLevel;

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("GlobalIllumination Apply"),
			PassParameters,
			ERDGPassFlags::Raster,
			[this, &SceneContext, &View, &SceneTextures, PassParameters](FRHICommandListImmediate& RHICmdList)
		{
			TShaderMapRef<FPostProcessVS> VertexShader(View.ShaderMap);
			TShaderMapRef<FRayTracingGlobalIlluminationNecaApplyPS> PixelShader(View.ShaderMap);
			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

			// Additive blending
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_Zero>::GetRHI();
			//GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;
			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

			SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);

			RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

			DrawRectangle(
				RHICmdList,
				0, 0,
				View.ViewRect.Width(), View.ViewRect.Height(),
				View.ViewRect.Min.X, View.ViewRect.Min.Y,
				View.ViewRect.Width(), View.ViewRect.Height(),
				FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
				SceneContext.GetBufferSizeXY(),
				VertexShader
			);
		}
		);
	}
}
#else // !RHI_RAYTRACING
{
	check(0);
}
#endif

bool FDeferredShadingSceneRenderer::RenderRayTracingGlobalIllumination(
	FRDGBuilder& GraphBuilder, 
	FSceneTextureParameters& SceneTextures,
	FViewInfo& View,
	IScreenSpaceDenoiser::FAmbientOcclusionRayTracingConfig* OutRayTracingConfig,
	IScreenSpaceDenoiser::FDiffuseIndirectInputs* OutDenoiserInputs,
	TRefCountPtr<IPooledRenderTarget>& GlobalIlluminationRT)
#if RHI_RAYTRACING
{
	if (!View.ViewState) return false;

	int32 RayTracingGISamplesPerPixel = GetRayTracingGlobalIlluminationSamplesPerPixel(View);
	if (RayTracingGISamplesPerPixel <= 0) return false;

	OutRayTracingConfig->ResolutionFraction = 1.0;
	if (GRayTracingGlobalIlluminationDenoiser != 0)
	{
		OutRayTracingConfig->ResolutionFraction = FMath::Clamp(GRayTracingGlobalIlluminationScreenPercentage / 100.0, 0.25, 1.0);
	}

	OutRayTracingConfig->RayCountPerPixel = RayTracingGISamplesPerPixel;

	int32 UpscaleFactor = int32(1.0 / OutRayTracingConfig->ResolutionFraction);

	// Allocate input for the denoiser.
	{
		FRDGTextureDesc Desc = FRDGTextureDesc::Create2DDesc(
			SceneTextures.SceneDepthBuffer->Desc.Extent / UpscaleFactor,
			PF_FloatRGBA,
			FClearValueBinding::None,
			/* InFlags = */ TexCreate_None,
			/* InTargetableFlags = */ TexCreate_ShaderResource | TexCreate_RenderTargetable | TexCreate_UAV,
			/* bInForceSeparateTargetAndShaderResource = */ false);

		OutDenoiserInputs->Color = GraphBuilder.CreateTexture(Desc, TEXT("RayTracingDiffuseIndirect"));

		Desc.Format = PF_G16R16;
		OutDenoiserInputs->RayHitDistance = GraphBuilder.CreateTexture(Desc, TEXT("RayTracingDiffuseIndirectHitDistance"));
	}

	// Ray generation pass
	if (!IsNeteaseRTGIEnabled(View))
	{
		if (IsFinalGatherEnabled(View))
		{
			RenderRayTracingGlobalIlluminationFinalGather(GraphBuilder, SceneTextures, View, *OutRayTracingConfig, UpscaleFactor, OutDenoiserInputs);
		}
		else
		{
			RenderRayTracingGlobalIlluminationBruteForce(GraphBuilder, SceneTextures, View, *OutRayTracingConfig, UpscaleFactor, OutDenoiserInputs);
		}
	}
	else
	{
		RenderRayTracingNecaGlobalIllumination(
			GraphBuilder,
			View,
			GlobalIlluminationRT);
	}
	return true;
}
#else
{
	unimplemented();
	return false;
}
#endif // RHI_RAYTRACING

#if RHI_RAYTRACING
void CopyGatherPassParameters(
	const FRayTracingGlobalIlluminationCreateGatherPointsRGS::FParameters& PassParameters,
	FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FParameters* NewParameters
)
{
	NewParameters->GatherSamplesPerPixel = PassParameters.GatherSamplesPerPixel;
	NewParameters->SamplesPerPixel = PassParameters.SamplesPerPixel;
	NewParameters->SampleIndex = PassParameters.SampleIndex;
	NewParameters->MaxBounces = PassParameters.MaxBounces;
	NewParameters->UpscaleFactor = PassParameters.UpscaleFactor;
	NewParameters->RenderTileOffsetX = PassParameters.RenderTileOffsetX;
	NewParameters->RenderTileOffsetY = PassParameters.RenderTileOffsetY;
	NewParameters->MaxRayDistanceForGI = PassParameters.MaxRayDistanceForGI;
	NewParameters->MaxShadowDistance = PassParameters.MaxShadowDistance;
	NewParameters->NextEventEstimationSamples = PassParameters.NextEventEstimationSamples;
	NewParameters->DiffuseThreshold = PassParameters.DiffuseThreshold;
	NewParameters->MaxNormalBias = PassParameters.MaxNormalBias;
	NewParameters->EvalSkyLight = PassParameters.EvalSkyLight;
	NewParameters->UseRussianRoulette = PassParameters.UseRussianRoulette;

	NewParameters->TLAS = PassParameters.TLAS;
	NewParameters->ViewUniformBuffer = PassParameters.ViewUniformBuffer;

	NewParameters->HaltonIteration = PassParameters.HaltonIteration;
	NewParameters->HaltonPrimes = PassParameters.HaltonPrimes;
	NewParameters->BlueNoise = PassParameters.BlueNoise;

	NewParameters->LightParameters = PassParameters.LightParameters;
	NewParameters->SkyLight = PassParameters.SkyLight;

	NewParameters->SceneTextures = PassParameters.SceneTextures;
	NewParameters->SSProfilesTexture = PassParameters.SSProfilesTexture;
	NewParameters->TransmissionProfilesLinearSampler = PassParameters.TransmissionProfilesLinearSampler;

	NewParameters->GatherPointsResolution = PassParameters.GatherPointsResolution;
	NewParameters->TileAlignedResolution = PassParameters.TileAlignedResolution;
	NewParameters->SortTileSize = PassParameters.SortTileSize;

	NewParameters->RWGatherPointsBuffer = PassParameters.RWGatherPointsBuffer;
	NewParameters->MaterialBuffer = PassParameters.MaterialBuffer;
}
#endif // RHI_RAYTRACING

void FDeferredShadingSceneRenderer::RayTracingGlobalIlluminationCreateGatherPoints(
	FRDGBuilder& GraphBuilder,
	FSceneTextureParameters& SceneTextures,
	FViewInfo& View,
	int32 UpscaleFactor,
	FRDGBufferRef& GatherPointsBuffer,
	FIntVector& GatherPointsResolution
)
#if RHI_RAYTRACING
{	
	RDG_GPU_STAT_SCOPE(GraphBuilder, RayTracingGICreateGatherPoints);
	RDG_EVENT_SCOPE(GraphBuilder, "Ray Tracing GI: Create Gather Points");

	int32 GatherSamples = FMath::Min(GetRayTracingGlobalIlluminationSamplesPerPixel(View), MAXIMUM_GATHER_POINTS_PER_PIXEL);
	int32 SamplesPerPixel = 1;

	uint32 IterationCount = SamplesPerPixel;
	uint32 SequenceCount = 1;
	uint32 DimensionCount = 24;
	int32 FrameIndex = View.ViewState->FrameIndex % 1024;
	FHaltonSequenceIteration HaltonSequenceIteration(Scene->HaltonSequence, IterationCount, SequenceCount, DimensionCount, FrameIndex);

	FHaltonIteration HaltonIteration;
	InitializeHaltonSequenceIteration(HaltonSequenceIteration, HaltonIteration);

	FHaltonPrimes HaltonPrimes;
	InitializeHaltonPrimes(Scene->HaltonPrimesResource, HaltonPrimes);

	FBlueNoise BlueNoise;
	InitializeBlueNoise(BlueNoise);

	FPathTracingLightData LightParameters;
	SetupLightParameters(*Scene, View, &LightParameters);

	float MaxShadowDistance = 1.0e27;
	if (GRayTracingGlobalIlluminationMaxShadowDistance > 0.0)
	{
		MaxShadowDistance = GRayTracingGlobalIlluminationMaxShadowDistance;
	}
	else if (Scene->SkyLight)
	{
		// Adjust ray TMax so shadow rays do not hit the sky sphere 
		MaxShadowDistance = FMath::Max(0.0, 0.99 * Scene->SkyLight->SkyDistanceThreshold);
	}

	FSkyLightData SkyLightParameters;
	SetupGlobalIlluminationSkyLightParameters(*Scene, &SkyLightParameters);

	FRayTracingGlobalIlluminationCreateGatherPointsRGS::FParameters* PassParameters = GraphBuilder.AllocParameters<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FParameters>();
	PassParameters->SampleIndex = (FrameIndex * SamplesPerPixel) % GatherSamples;
	PassParameters->GatherSamplesPerPixel = GatherSamples;
	PassParameters->SamplesPerPixel = SamplesPerPixel;
	PassParameters->MaxBounces = 1;
	PassParameters->MaxNormalBias = GetRaytracingMaxNormalBias();
	PassParameters->MaxRayDistanceForGI = GRayTracingGlobalIlluminationMaxRayDistance;
	PassParameters->MaxShadowDistance = MaxShadowDistance;
	PassParameters->EvalSkyLight = GRayTracingGlobalIlluminationEvalSkyLight != 0;
	PassParameters->UseRussianRoulette = GRayTracingGlobalIlluminationUseRussianRoulette != 0;
	PassParameters->DiffuseThreshold = GRayTracingGlobalIlluminationDiffuseThreshold;
	PassParameters->NextEventEstimationSamples = GRayTracingGlobalIlluminationNextEventEstimationSamples;
	PassParameters->UpscaleFactor = UpscaleFactor;
	PassParameters->RenderTileOffsetX = 0;
	PassParameters->RenderTileOffsetY = 0;

	// Global
	PassParameters->TLAS = View.RayTracingScene.RayTracingSceneRHI->GetShaderResourceView();
	PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;

	// Sampling sequence
	PassParameters->HaltonIteration = CreateUniformBufferImmediate(HaltonIteration, EUniformBufferUsage::UniformBuffer_SingleDraw);
	PassParameters->HaltonPrimes = CreateUniformBufferImmediate(HaltonPrimes, EUniformBufferUsage::UniformBuffer_SingleDraw);
	PassParameters->BlueNoise = CreateUniformBufferImmediate(BlueNoise, EUniformBufferUsage::UniformBuffer_SingleDraw);

	// Light data
	PassParameters->LightParameters = CreateUniformBufferImmediate(LightParameters, EUniformBufferUsage::UniformBuffer_SingleDraw);
	PassParameters->SceneTextures = SceneTextures;
	PassParameters->SkyLight = CreateUniformBufferImmediate(SkyLightParameters, EUniformBufferUsage::UniformBuffer_SingleDraw);

	// Shading data
	TRefCountPtr<IPooledRenderTarget> SubsurfaceProfileRT((IPooledRenderTarget*)GetSubsufaceProfileTexture_RT(GraphBuilder.RHICmdList));
	if (!SubsurfaceProfileRT)
	{
		SubsurfaceProfileRT = GSystemTextures.BlackDummy;
	}
	PassParameters->SSProfilesTexture = GraphBuilder.RegisterExternalTexture(SubsurfaceProfileRT);
	PassParameters->TransmissionProfilesLinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

	// Output
	FIntPoint DispatchResolution = FIntPoint::DivideAndRoundUp(View.ViewRect.Size(), UpscaleFactor);
	FIntVector LocalGatherPointsResolution(DispatchResolution.X, DispatchResolution.Y, GatherSamples);
	if (GatherPointsResolution != LocalGatherPointsResolution)
	{
		GatherPointsResolution = LocalGatherPointsResolution;
		FRDGBufferDesc BufferDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FGatherPoint), GatherPointsResolution.X * GatherPointsResolution.Y * GatherPointsResolution.Z);
		GatherPointsBuffer = GraphBuilder.CreateBuffer(BufferDesc, TEXT("GatherPointsBuffer"), ERDGResourceFlags::MultiFrame);
	}
	else
	{
		GatherPointsBuffer = GraphBuilder.RegisterExternalBuffer(((FSceneViewState*)View.State)->GatherPointsBuffer, TEXT("GatherPointsBuffer"));
	}
	PassParameters->GatherPointsResolution = FIntPoint(GatherPointsResolution.X, GatherPointsResolution.Y);
	PassParameters->RWGatherPointsBuffer = GraphBuilder.CreateUAV(GatherPointsBuffer, EPixelFormat::PF_R32_UINT);

	// When deferred materials are used, two passes are invoked:
	// 1) Gather ray-hit data and sort by hit-shader ID
	// 2) Re-trace "short" ray and shade
	const bool bSortMaterials = CVarRayTracingGlobalIlluminationFinalGatherSortMaterials.GetValueOnRenderThread() != 0;
	if (!bSortMaterials)
	{
	FRayTracingGlobalIlluminationCreateGatherPointsRGS::FPermutationDomain PermutationVector;
	PermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FUseAttenuationTermDim>(true);
	PermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FEnableTwoSidedGeometryDim>(CVarRayTracingGlobalIlluminationEnableTwoSidedGeometry.GetValueOnRenderThread() != 0);
	TShaderMapRef<FRayTracingGlobalIlluminationCreateGatherPointsRGS> RayGenerationShader(GetGlobalShaderMap(FeatureLevel), PermutationVector);
	ClearUnusedGraphResources(RayGenerationShader, PassParameters);

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("GatherPoints %d%d", GatherPointsResolution.X, GatherPointsResolution.Y),
		PassParameters,
		ERDGPassFlags::Compute,
		[PassParameters, this, &View, RayGenerationShader, GatherPointsResolution](FRHICommandList& RHICmdList)
	{
		FRHIRayTracingScene* RayTracingSceneRHI = View.RayTracingScene.RayTracingSceneRHI;
		FRayTracingShaderBindingsWriter GlobalResources;
		SetShaderParameters(GlobalResources, RayGenerationShader, *PassParameters);
		RHICmdList.RayTraceDispatch(View.RayTracingMaterialPipeline, RayGenerationShader.GetRayTracingShader(), RayTracingSceneRHI, GlobalResources, GatherPointsResolution.X, GatherPointsResolution.Y);
	});
	}
	else
	{
		// Determines tile-size for sorted-deferred path
		const int32 SortTileSize = CVarRayTracingGlobalIlluminationFinalGatherSortTileSize.GetValueOnRenderThread();
		FIntPoint TileAlignedResolution = FIntPoint(GatherPointsResolution.X, GatherPointsResolution.Y);
		if (SortTileSize)
		{
			TileAlignedResolution = FIntPoint::DivideAndRoundUp(TileAlignedResolution, SortTileSize) * SortTileSize;
		}
		PassParameters->TileAlignedResolution = TileAlignedResolution;
		PassParameters->SortTileSize = SortTileSize;

		FRDGBufferRef DeferredMaterialBuffer = nullptr;
		const uint32 DeferredMaterialBufferNumElements = TileAlignedResolution.X * TileAlignedResolution.Y;

		// Gather pass
		{
			FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FParameters* GatherPassParameters = GraphBuilder.AllocParameters<FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FParameters>();
			CopyGatherPassParameters(*PassParameters, GatherPassParameters);

			FRDGBufferDesc Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FDeferredMaterialPayload), DeferredMaterialBufferNumElements);
			DeferredMaterialBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("RayTracingGlobalIlluminationMaterialBuffer"));
			GatherPassParameters->MaterialBuffer = GraphBuilder.CreateUAV(DeferredMaterialBuffer);

			FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FPermutationDomain PermutationVector;
			PermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FUseAttenuationTermDim>(true);
			PermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FEnableTwoSidedGeometryDim>(CVarRayTracingGlobalIlluminationEnableTwoSidedGeometry.GetValueOnRenderThread() != 0);
			PermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS::FDeferredMaterialMode>(EDeferredMaterialMode::Gather);
			TShaderMapRef<FRayTracingGlobalIlluminationCreateGatherPointsTraceRGS> RayGenerationShader(GetGlobalShaderMap(FeatureLevel), PermutationVector);

			ClearUnusedGraphResources(RayGenerationShader, GatherPassParameters);

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("GlobalIlluminationRayTracingGatherMaterials %dx%d", TileAlignedResolution.X, TileAlignedResolution.Y),
				GatherPassParameters,
				ERDGPassFlags::Compute,
				[GatherPassParameters, this, &View, RayGenerationShader, TileAlignedResolution](FRHICommandList& RHICmdList)
			{
				FRayTracingPipelineState* Pipeline = BindRayTracingDeferredMaterialGatherPipeline(RHICmdList, View, RayGenerationShader.GetRayTracingShader());

				FRayTracingShaderBindingsWriter GlobalResources;
				SetShaderParameters(GlobalResources, RayGenerationShader, *GatherPassParameters);

				FRHIRayTracingScene* RayTracingSceneRHI = View.RayTracingScene.RayTracingSceneRHI;
				RHICmdList.RayTraceDispatch(Pipeline, RayGenerationShader.GetRayTracingShader(), RayTracingSceneRHI, GlobalResources, TileAlignedResolution.X, TileAlignedResolution.Y);
			});
		}

		// Sort by hit-shader ID
		const uint32 SortSize = CVarRayTracingGlobalIlluminationFinalGatherSortSize.GetValueOnRenderThread();
		SortDeferredMaterials(GraphBuilder, View, SortSize, DeferredMaterialBufferNumElements, DeferredMaterialBuffer);

		// Shade pass
		{
			PassParameters->MaterialBuffer = GraphBuilder.CreateUAV(DeferredMaterialBuffer);

			FRayTracingGlobalIlluminationCreateGatherPointsRGS::FPermutationDomain PermutationVector;
			PermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FUseAttenuationTermDim>(true);
			PermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FEnableTwoSidedGeometryDim>(CVarRayTracingGlobalIlluminationEnableTwoSidedGeometry.GetValueOnRenderThread() != 0);
			PermutationVector.Set<FRayTracingGlobalIlluminationCreateGatherPointsRGS::FDeferredMaterialMode>(EDeferredMaterialMode::Shade);
			TShaderMapRef<FRayTracingGlobalIlluminationCreateGatherPointsRGS> RayGenerationShader(GetGlobalShaderMap(FeatureLevel), PermutationVector);
			ClearUnusedGraphResources(RayGenerationShader, PassParameters);

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("GlobalIlluminationRayTracingShadeMaterials %d", DeferredMaterialBufferNumElements),
				PassParameters,
				ERDGPassFlags::Compute,
				[PassParameters, this, &View, RayGenerationShader, DeferredMaterialBufferNumElements](FRHICommandList& RHICmdList)
			{
				FRHIRayTracingScene* RayTracingSceneRHI = View.RayTracingScene.RayTracingSceneRHI;
				FRayTracingShaderBindingsWriter GlobalResources;
				SetShaderParameters(GlobalResources, RayGenerationShader, *PassParameters);

				// Shading pass for sorted materials uses 1D dispatch over all elements in the material buffer.
				// This can be reduced to the number of output pixels if sorting pass guarantees that all invalid entries are moved to the end.
				RHICmdList.RayTraceDispatch(View.RayTracingMaterialPipeline, RayGenerationShader.GetRayTracingShader(), RayTracingSceneRHI, GlobalResources, DeferredMaterialBufferNumElements, 1);
			});
		}
	}

}
#else
{
	unimplemented();
}
#endif

void FDeferredShadingSceneRenderer::RenderRayTracingGlobalIlluminationFinalGather(
	FRDGBuilder& GraphBuilder,
	FSceneTextureParameters& SceneTextures,
	FViewInfo& View,
	const IScreenSpaceDenoiser::FAmbientOcclusionRayTracingConfig& RayTracingConfig,
	int32 UpscaleFactor,
	// Output
	IScreenSpaceDenoiser::FDiffuseIndirectInputs* OutDenoiserInputs)
#if RHI_RAYTRACING
{
	// Generate gather points
	FRDGBufferRef GatherPointsBuffer;
	FSceneViewState* SceneViewState = (FSceneViewState*)View.State;
	RayTracingGlobalIlluminationCreateGatherPoints(GraphBuilder, SceneTextures, View, UpscaleFactor, GatherPointsBuffer, SceneViewState->GatherPointsResolution);

	// Perform gather
	RDG_GPU_STAT_SCOPE(GraphBuilder, RayTracingGIFinalGather);
	RDG_EVENT_SCOPE(GraphBuilder, "Ray Tracing GI: Final Gather");

	FRayTracingGlobalIlluminationFinalGatherRGS::FParameters* PassParameters = GraphBuilder.AllocParameters<FRayTracingGlobalIlluminationFinalGatherRGS::FParameters>();
	int32 SamplesPerPixel = FMath::Min(GetRayTracingGlobalIlluminationSamplesPerPixel(View), MAXIMUM_GATHER_POINTS_PER_PIXEL);
	int32 SampleIndex = View.ViewState->FrameIndex % SamplesPerPixel;
	PassParameters->SampleIndex = SampleIndex;
	PassParameters->SamplesPerPixel = SamplesPerPixel;
	PassParameters->DiffuseThreshold = GRayTracingGlobalIlluminationDiffuseThreshold;
	PassParameters->MaxNormalBias = GetRaytracingMaxNormalBias();
	PassParameters->FinalGatherDistance = GRayTracingGlobalIlluminationFinalGatherDistance;
	PassParameters->UpscaleFactor = UpscaleFactor;
	PassParameters->RenderTileOffsetX = 0;
	PassParameters->RenderTileOffsetY = 0;

	// Scene data
	PassParameters->TLAS = View.RayTracingScene.RayTracingSceneRHI->GetShaderResourceView();
	PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;

	// Shading data
	PassParameters->SceneTextures = SceneTextures;
	TRefCountPtr<IPooledRenderTarget> SubsurfaceProfileRT((IPooledRenderTarget*)GetSubsufaceProfileTexture_RT(GraphBuilder.RHICmdList));
	if (!SubsurfaceProfileRT)
	{
		SubsurfaceProfileRT = GSystemTextures.BlackDummy;
	}
	PassParameters->SSProfilesTexture = GraphBuilder.RegisterExternalTexture(SubsurfaceProfileRT);
	PassParameters->TransmissionProfilesLinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

	// Gather points
	PassParameters->GatherPointsResolution = FIntPoint(SceneViewState->GatherPointsResolution.X, SceneViewState->GatherPointsResolution.Y);
	PassParameters->GatherPointsBuffer = GraphBuilder.CreateSRV(GatherPointsBuffer);

	// Output
	PassParameters->RWGlobalIlluminationUAV = GraphBuilder.CreateUAV(OutDenoiserInputs->Color);
	PassParameters->RWRayDistanceUAV = GraphBuilder.CreateUAV(OutDenoiserInputs->RayHitDistance);

	FRayTracingGlobalIlluminationFinalGatherRGS::FPermutationDomain PermutationVector;
	PermutationVector.Set<FRayTracingGlobalIlluminationFinalGatherRGS::FUseAttenuationTermDim>(true);
	PermutationVector.Set<FRayTracingGlobalIlluminationFinalGatherRGS::FEnableTwoSidedGeometryDim>(CVarRayTracingGlobalIlluminationEnableTwoSidedGeometry.GetValueOnRenderThread() != 0);
	TShaderMapRef<FRayTracingGlobalIlluminationFinalGatherRGS> RayGenerationShader(GetGlobalShaderMap(FeatureLevel), PermutationVector);
	ClearUnusedGraphResources(RayGenerationShader, PassParameters);

	FIntPoint RayTracingResolution = FIntPoint::DivideAndRoundUp(View.ViewRect.Size(), UpscaleFactor);
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("GlobalIlluminationRayTracing %dx%d", RayTracingResolution.X, RayTracingResolution.Y),
		PassParameters,
		ERDGPassFlags::Compute,
		[PassParameters, this, &View, RayGenerationShader, RayTracingResolution](FRHICommandList& RHICmdList)
	{
		FRHIRayTracingScene* RayTracingSceneRHI = View.RayTracingScene.RayTracingSceneRHI;

		FRayTracingShaderBindingsWriter GlobalResources;
		SetShaderParameters(GlobalResources, RayGenerationShader, *PassParameters);
		RHICmdList.RayTraceDispatch(View.RayTracingMaterialPipeline, RayGenerationShader.GetRayTracingShader(), RayTracingSceneRHI, GlobalResources, RayTracingResolution.X, RayTracingResolution.Y);
	});


	GraphBuilder.QueueBufferExtraction(GatherPointsBuffer, &SceneViewState->GatherPointsBuffer, FRDGResourceState::EAccess::Read, FRDGResourceState::EPipeline::Compute);
}
#else
{
	unimplemented();
}
#endif

void FDeferredShadingSceneRenderer::RenderRayTracingGlobalIlluminationBruteForce(
	FRDGBuilder& GraphBuilder,
	FSceneTextureParameters& SceneTextures,
	FViewInfo& View,
	const IScreenSpaceDenoiser::FAmbientOcclusionRayTracingConfig& RayTracingConfig,
	int32 UpscaleFactor,
	IScreenSpaceDenoiser::FDiffuseIndirectInputs* OutDenoiserInputs)
#if RHI_RAYTRACING
{
	RDG_GPU_STAT_SCOPE(GraphBuilder, RayTracingGIBruteForce);
	RDG_EVENT_SCOPE(GraphBuilder, "Ray Tracing GI: Brute Force");

	int32 RayTracingGISamplesPerPixel = GetRayTracingGlobalIlluminationSamplesPerPixel(View);
	uint32 IterationCount = RayTracingGISamplesPerPixel;
	uint32 SequenceCount = 1;
	uint32 DimensionCount = 24;
	FHaltonSequenceIteration HaltonSequenceIteration(Scene->HaltonSequence, IterationCount, SequenceCount, DimensionCount, View.ViewState->FrameIndex % 1024);

	FHaltonIteration HaltonIteration;
	InitializeHaltonSequenceIteration(HaltonSequenceIteration, HaltonIteration);

	FHaltonPrimes HaltonPrimes;
	InitializeHaltonPrimes(Scene->HaltonPrimesResource, HaltonPrimes);

	FBlueNoise BlueNoise;
	InitializeBlueNoise(BlueNoise);

	FPathTracingLightData LightParameters;
	SetupLightParameters(*Scene, View, &LightParameters);

	float MaxShadowDistance = 1.0e27;
	if (GRayTracingGlobalIlluminationMaxShadowDistance > 0.0)
	{
		MaxShadowDistance = GRayTracingGlobalIlluminationMaxShadowDistance;
	}
	else if (Scene->SkyLight)
	{
		// Adjust ray TMax so shadow rays do not hit the sky sphere 
		MaxShadowDistance = FMath::Max(0.0, 0.99 * Scene->SkyLight->SkyDistanceThreshold);
	}

	FSkyLightData SkyLightParameters;
	SetupGlobalIlluminationSkyLightParameters(*Scene, &SkyLightParameters);

	FGlobalIlluminationRGS::FParameters* PassParameters = GraphBuilder.AllocParameters<FGlobalIlluminationRGS::FParameters>();
	PassParameters->SamplesPerPixel = RayTracingGISamplesPerPixel;
	int32 CVarRayTracingGlobalIlluminationMaxBouncesValue = CVarRayTracingGlobalIlluminationMaxBounces.GetValueOnRenderThread();
	PassParameters->MaxBounces = CVarRayTracingGlobalIlluminationMaxBouncesValue > -1? CVarRayTracingGlobalIlluminationMaxBouncesValue : View.FinalPostProcessSettings.RayTracingGIMaxBounces;
	PassParameters->MaxNormalBias = GetRaytracingMaxNormalBias();
	float MaxRayDistanceForGI = GRayTracingGlobalIlluminationMaxRayDistance;
	if (MaxRayDistanceForGI == -1.0)
	{
		MaxRayDistanceForGI = View.FinalPostProcessSettings.AmbientOcclusionRadius;
	}
	PassParameters->MaxRayDistanceForGI = MaxRayDistanceForGI;
	PassParameters->MaxRayDistanceForAO = View.FinalPostProcessSettings.AmbientOcclusionRadius;
	PassParameters->MaxShadowDistance = MaxShadowDistance;
	PassParameters->UpscaleFactor = UpscaleFactor;
	PassParameters->EvalSkyLight = GRayTracingGlobalIlluminationEvalSkyLight != 0;
	PassParameters->UseRussianRoulette = GRayTracingGlobalIlluminationUseRussianRoulette != 0;
	PassParameters->DiffuseThreshold = GRayTracingGlobalIlluminationDiffuseThreshold;
	PassParameters->NextEventEstimationSamples = GRayTracingGlobalIlluminationNextEventEstimationSamples;
	PassParameters->TLAS = View.RayTracingScene.RayTracingSceneRHI->GetShaderResourceView();
	PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;
	PassParameters->HaltonIteration = CreateUniformBufferImmediate(HaltonIteration, EUniformBufferUsage::UniformBuffer_SingleDraw);
	PassParameters->HaltonPrimes = CreateUniformBufferImmediate(HaltonPrimes, EUniformBufferUsage::UniformBuffer_SingleDraw);
	PassParameters->BlueNoise = CreateUniformBufferImmediate(BlueNoise, EUniformBufferUsage::UniformBuffer_SingleDraw);
	PassParameters->LightParameters = CreateUniformBufferImmediate(LightParameters, EUniformBufferUsage::UniformBuffer_SingleDraw);
	PassParameters->SceneTextures = SceneTextures;
	PassParameters->SkyLight = CreateUniformBufferImmediate(SkyLightParameters, EUniformBufferUsage::UniformBuffer_SingleDraw);

	// TODO: should be converted to RDG
	TRefCountPtr<IPooledRenderTarget> SubsurfaceProfileRT((IPooledRenderTarget*) GetSubsufaceProfileTexture_RT(GraphBuilder.RHICmdList));
	if (!SubsurfaceProfileRT)
	{
		SubsurfaceProfileRT = GSystemTextures.BlackDummy;
	}
	PassParameters->SSProfilesTexture = GraphBuilder.RegisterExternalTexture(SubsurfaceProfileRT);
	PassParameters->TransmissionProfilesLinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->RWGlobalIlluminationUAV = GraphBuilder.CreateUAV(OutDenoiserInputs->Color);
	PassParameters->RWRayDistanceUAV = GraphBuilder.CreateUAV(OutDenoiserInputs->RayHitDistance);
	PassParameters->RenderTileOffsetX = 0;
	PassParameters->RenderTileOffsetY = 0;

	FGlobalIlluminationRGS::FPermutationDomain PermutationVector;
	PermutationVector.Set<FGlobalIlluminationRGS::FUseAttenuationTermDim>(true);
	PermutationVector.Set<FGlobalIlluminationRGS::FEnableTwoSidedGeometryDim>(CVarRayTracingGlobalIlluminationEnableTwoSidedGeometry.GetValueOnRenderThread() != 0);
	TShaderMapRef<FGlobalIlluminationRGS> RayGenerationShader(GetGlobalShaderMap(FeatureLevel), PermutationVector);
	ClearUnusedGraphResources(RayGenerationShader, PassParameters);

	FIntPoint RayTracingResolution = FIntPoint::DivideAndRoundUp(View.ViewRect.Size(), UpscaleFactor);

	if (GRayTracingGlobalIlluminationRenderTileSize <= 0)
	{
		GraphBuilder.AddPass(
			RDG_EVENT_NAME("GlobalIlluminationRayTracing %dx%d", RayTracingResolution.X, RayTracingResolution.Y),
			PassParameters,
			ERDGPassFlags::Compute,
			[PassParameters, this, &View, RayGenerationShader, RayTracingResolution](FRHICommandList& RHICmdList)
		{
			FRHIRayTracingScene* RayTracingSceneRHI = View.RayTracingScene.RayTracingSceneRHI;

			FRayTracingShaderBindingsWriter GlobalResources;
			SetShaderParameters(GlobalResources, RayGenerationShader, *PassParameters);
			RHICmdList.RayTraceDispatch(View.RayTracingMaterialPipeline, RayGenerationShader.GetRayTracingShader(), RayTracingSceneRHI, GlobalResources, RayTracingResolution.X, RayTracingResolution.Y);
		});
	}
	else
	{
		int32 RenderTileSize = FMath::Max(32, GRayTracingGlobalIlluminationRenderTileSize);
		int32 NumTilesX = FMath::DivideAndRoundUp(RayTracingResolution.X, RenderTileSize);
		int32 NumTilesY = FMath::DivideAndRoundUp(RayTracingResolution.Y, RenderTileSize);
		for (int32 Y = 0; Y < NumTilesY; ++Y)
		{
			for (int32 X = 0; X < NumTilesX; ++X)
			{
				FGlobalIlluminationRGS::FParameters* TilePassParameters = PassParameters;

				if (X > 0 || Y > 0)
				{
					TilePassParameters = GraphBuilder.AllocParameters<FGlobalIlluminationRGS::FParameters>();
					*TilePassParameters = *PassParameters;

					TilePassParameters->RenderTileOffsetX = X * RenderTileSize;
					TilePassParameters->RenderTileOffsetY = Y * RenderTileSize;
				}

				int32 DispatchSizeX = FMath::Min<int32>(RenderTileSize, RayTracingResolution.X - TilePassParameters->RenderTileOffsetX);
				int32 DispatchSizeY = FMath::Min<int32>(RenderTileSize, RayTracingResolution.Y - TilePassParameters->RenderTileOffsetY);

				GraphBuilder.AddPass(
					RDG_EVENT_NAME("GlobalIlluminationRayTracing %dx%d (tile %dx%d)", DispatchSizeX, DispatchSizeY, X, Y),
					TilePassParameters,
					ERDGPassFlags::Compute,
					[TilePassParameters, this, &View, RayGenerationShader, DispatchSizeX, DispatchSizeY](FRHICommandList& RHICmdList)
				{
					FRHIRayTracingScene* RayTracingSceneRHI = View.RayTracingScene.RayTracingSceneRHI;

					FRayTracingShaderBindingsWriter GlobalResources;
					SetShaderParameters(GlobalResources, RayGenerationShader, *TilePassParameters);
					RHICmdList.RayTraceDispatch(View.RayTracingMaterialPipeline, RayGenerationShader.GetRayTracingShader(), RayTracingSceneRHI, 
						GlobalResources, DispatchSizeX, DispatchSizeY);
					RHICmdList.SubmitCommandsHint();
				});
			}
		}
	}
}
#else
{
	unimplemented();
}
#endif

#if RHI_RAYTRACING
TGlobalResource< FSpriteIndexBuffer<1> > GVisualizeGIQuadIndexBuffer;

class FVisualizeNecaGIProbesVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FVisualizeNecaGIProbesVS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	/** Default constructor. */
	FVisualizeNecaGIProbesVS() {}

	/** Initialization constructor. */
	FVisualizeNecaGIProbesVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ProbeStartPosition.Bind(Initializer.ParameterMap, TEXT("ProbeStartPosition"));
		ProbeGridRadius.Bind(Initializer.ParameterMap, TEXT("ProbeGridRadius"));
		ProbeCount.Bind(Initializer.ParameterMap, TEXT("ProbeCount"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, int LodIndex)
	{
		FRHIVertexShader* ShaderRHI = RHICmdList.GetBoundVertexShader();
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, View.ViewUniformBuffer);

		FVector StartPosition = View.FinalPostProcessSettings.RayTracingGINecaStartPosition;
		FVector EndPosition = View.FinalPostProcessSettings.RayTracingGINecaStartPosition + 2 * View.FinalPostProcessSettings.RayTracingGINecaBounds;
		
		FIntVector ProbeCountsInt = FIntVector(ProbeCounts[LodIndex].X, ProbeCounts[LodIndex].Y, ProbeCounts[LodIndex].Z);

		SetShaderValue(RHICmdList, ShaderRHI, ProbeStartPosition, StartPosition);
		SetShaderValue(RHICmdList, ShaderRHI, ProbeGridRadius, (EndPosition - StartPosition) / (ProbeCounts[LodIndex] - 1));
		SetShaderValue(RHICmdList, ShaderRHI, ProbeCount, ProbeCountsInt);
	}

private:
	LAYOUT_FIELD(FShaderParameter, ProbeStartPosition);
	LAYOUT_FIELD(FShaderParameter, ProbeGridRadius);
	LAYOUT_FIELD(FShaderParameter, ProbeCount);
};

IMPLEMENT_SHADER_TYPE(, FVisualizeNecaGIProbesVS, TEXT("/Engine/Private/VisualizeNecaGIProbe.usf"), TEXT("VisualizeNecaGIProbeVS"), SF_Vertex);

class FVisualizeNecaGIProbesPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FVisualizeNecaGIProbesPS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	/** Default constructor. */
	FVisualizeNecaGIProbesPS() {}

	/** Initialization constructor. */
	FVisualizeNecaGIProbesPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ProbeCount.Bind(Initializer.ParameterMap, TEXT("ProbeCount"));
		ProbeSideLengthIrradiance.Bind(Initializer.ParameterMap, TEXT("ProbeSideLengthIrradiance"));
		TextureSizeIrradiance.Bind(Initializer.ParameterMap, TEXT("TextureSizeIrradiance"));
		RadianceTexture.Bind(Initializer.ParameterMap, TEXT("RadianceTexture"));
		RadianceSampler.Bind(Initializer.ParameterMap, TEXT("RadianceSampler"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, TRefCountPtr<IPooledRenderTarget>& GIRadianceGatherRT, int LodIndex)
	{
		FRHIPixelShader* ShaderRHI = RHICmdList.GetBoundPixelShader();
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, View.ViewUniformBuffer);

		FIntVector ProbeCountsInt = FIntVector(ProbeCounts[LodIndex].X, ProbeCounts[LodIndex].Y, ProbeCounts[LodIndex].Z);
		const int LengthIrradianceProbe = GlobalProbeSideLengthIrradiance;
		const int IrradianceWidth = (LengthIrradianceProbe + 2) * ProbeCounts[LodIndex].X * ProbeCounts[LodIndex].Z + 2;
		const int IrradianceHeight = (LengthIrradianceProbe + 2) * ProbeCounts[LodIndex].Y + 2;

		SetShaderValue(RHICmdList, ShaderRHI, ProbeCount, ProbeCountsInt);
		SetShaderValue(RHICmdList, ShaderRHI, ProbeSideLengthIrradiance, LengthIrradianceProbe);
		SetShaderValue(RHICmdList, ShaderRHI, TextureSizeIrradiance, FVector4(IrradianceWidth, IrradianceHeight, 1.0f / IrradianceWidth, 1.0f / IrradianceHeight));

		if (GIRadianceGatherRT)
			SetTextureParameter(RHICmdList, ShaderRHI, RadianceTexture, RadianceSampler,
				TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), GIRadianceGatherRT->GetRenderTargetItem().ShaderResourceTexture);
		else
			SetTextureParameter(RHICmdList, ShaderRHI, RadianceTexture, RadianceSampler,
				TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), GSystemTextures.BlackDummy->GetRenderTargetItem().ShaderResourceTexture);
	}

private:
	LAYOUT_FIELD(FShaderParameter, ProbeCount);
	LAYOUT_FIELD(FShaderParameter, ProbeSideLengthIrradiance);
	LAYOUT_FIELD(FShaderParameter, TextureSizeIrradiance);
	LAYOUT_FIELD(FShaderResourceParameter, RadianceTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RadianceSampler);
};

IMPLEMENT_SHADER_TYPE(, FVisualizeNecaGIProbesPS, TEXT("/Engine/Private/VisualizeNecaGIProbe.usf"), TEXT("VisualizeNecaGIProbesPS"), SF_Pixel);

void FDeferredShadingSceneRenderer::VisualizeRayTracingNecaGIProbes(
	FRHICommandListImmediate& RHICmdList,
	const FViewInfo& View,
	TRefCountPtr<IPooledRenderTarget> GIRadianceGatherRT[3],
	int LodIndex)
{
	if (LodIndex != GRayTracingGlobalIlluminationDebugProbeLodLevel)
	{
		return;
	}

	int32 CVarRayTracingGlobalIlluminationValue = CVarRayTracingGlobalIllumination.GetValueOnRenderThread();
	if (CVarRayTracingGlobalIlluminationValue == 0 || (CVarRayTracingGlobalIlluminationValue == -1 && !IsNeteaseRTGIEnabled(View)))
	{
		return;
	}

	if (ViewFamily.EngineShowFlags.VisualizeNecaGIProbes)
	{
		SCOPED_DRAW_EVENT(RHICmdList, VisualizeNecaGIProbes);

		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

		int32 NumRenderTargets = 1;

		FRHITexture* RenderTargets[1] =
		{
			SceneContext.GetSceneColorSurface()
		};

		FRHIRenderPassInfo RPInfo(NumRenderTargets, RenderTargets, ERenderTargetActions::Load_Store);
		RPInfo.DepthStencilRenderTarget.Action = EDepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil;
		RPInfo.DepthStencilRenderTarget.DepthStencilTarget = SceneContext.GetSceneDepthSurface();
		RPInfo.DepthStencilRenderTarget.ExclusiveDepthStencil = FExclusiveDepthStencil::DepthWrite_StencilWrite;

		RHICmdList.BeginRenderPass(RPInfo, TEXT("VisualizeNecaGIProbes"));
		{
			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

			RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<true, CF_DepthNearOrEqual>::GetRHI();
			GraphicsPSOInit.BlendState = TStaticBlendStateWriteMask<CW_RGBA>::GetRHI();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			TShaderMapRef<FVisualizeNecaGIProbesVS> VertexShader(View.ShaderMap);
			TShaderMapRef<FVisualizeNecaGIProbesPS> PixelShader(View.ShaderMap);

			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

			VertexShader->SetParameters(RHICmdList, View, LodIndex);
			PixelShader->SetParameters(RHICmdList, View, GIRadianceGatherRT[GRayTracingGlobalIlluminationDebugProbeLodLevel], LodIndex);
			
			int NumQuads = ProbeCounts[LodIndex].X * ProbeCounts[LodIndex].Y * ProbeCounts[LodIndex].Z;

			RHICmdList.DrawIndexedPrimitive(GVisualizeGIQuadIndexBuffer.IndexBufferRHI, 0, 0, 4 * 1, 0, 2 * 1, NumQuads);
		}
		RHICmdList.EndRenderPass();
	}
}

#endif // RHI_RAYTRACING

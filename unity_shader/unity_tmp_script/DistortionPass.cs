using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using System.Collections.Generic;

namespace UnityEngine.Experiemntal.Rendering.Universal
{
    /// <summary>
    /// Copy the given color buffer to the given destination color buffer.
    ///
    /// You can use this pass to copy a color buffer to the destination,
    /// so you can use it later in rendering. For example, you can copy
    /// the opaque texture to use it for distortion effects.
    /// </summary>
    internal class DistortionPass : ScriptableRenderPass
    {

        public Material blitMaterial = null;
        public int blitShaderPassIndex = 0;

        public FilterMode filterMode {get; set;}

        private RenderTargetIdentifier source { get; set;}
        private RenderTargetHandle destination { get; set;}

        List<ShaderTagId> m_ShaderTagIdList = new List<ShaderTagId>();
        ProfilingSampler m_ProfilingSampler;

        RenderTargetHandle m_TemporaryColorTexture;
        RenderTargetHandle m_DistortionTexture;
        string m_ProfilerTag;

        FilteringSettings m_FilteringSettings;

        public DistortionPass(RenderPassEvent renderPassEvent, Material blitMaterial, int blitShaderPassIndex, string tag) {
            this.renderPassEvent = renderPassEvent;
            this.blitMaterial = blitMaterial;
            this.blitShaderPassIndex = blitShaderPassIndex;
            m_ProfilerTag = tag;
            m_TemporaryColorTexture.Init("_TemporaryColorTexture");
            m_DistortionTexture.Init("_DistortionTexture");

            m_ShaderTagIdList.Add(new ShaderTagId("UniversalForward"));
            m_ShaderTagIdList.Add(new ShaderTagId("SRPDefaultUnlit"));

            LayerMask mask = LayerMask.GetMask("mask");
            m_FilteringSettings = new FilteringSettings(RenderQueueRange.all, mask);

            m_ProfilingSampler = new ProfilingSampler(tag+" profiling");
        }

        public void Setup(RenderTargetIdentifier source, RenderTargetHandle destination) {
            this.source = source;
            this.destination = destination;
        }

        public override void Configure(CommandBuffer cmd, RenderTextureDescriptor cameraTextureDescriptor)
        {
            // Create distortion texture
            int width = cameraTextureDescriptor.width;
            int height = cameraTextureDescriptor.height;

            RenderTextureDescriptor opaqueDesc = cameraTextureDescriptor;
            opaqueDesc.width = width;
            opaqueDesc.height = height;
            opaqueDesc.depthBufferBits = 0;
            opaqueDesc.msaaSamples = 1;

            cmd.GetTemporaryRT(m_DistortionTexture.id, opaqueDesc, filterMode);
            ConfigureTarget(m_DistortionTexture.Identifier());
            ConfigureClear(ClearFlag.Color, new Color(0.5f, 0.5f, 0.0f, 0.0f));
        }

        public override void Execute(ScriptableRenderContext context, ref RenderingData renderingData)
        {
            CommandBuffer cmd = CommandBufferPool.Get(m_ProfilerTag);

            //using (new ProfilingScope(cmd, m_ProfilingSampler))
            {
                // Global render pass data containing various settings.
                //context.ExecuteCommandBuffer(cmd);
                //cmd.Clear();

                Camera camera = renderingData.cameraData.camera;
                var drawSettings = CreateDrawingSettings(m_ShaderTagIdList, ref renderingData, SortingCriteria.BackToFront);
                var filterSettings = m_FilteringSettings;

#if UNITY_EDITOR
                // When rendering the preview camera, we want the layer mask to be forced to Everything
                if (renderingData.cameraData.isPreviewCamera)
                {
                    filterSettings.layerMask = -1;
                }
#endif
                RenderStateBlock stateBlock = new RenderStateBlock(RenderStateMask.Nothing);
                context.DrawRenderers(renderingData.cullResults, ref drawSettings, ref filterSettings, ref stateBlock);

                // Render objects that did not match any shader pass with error shader
                //RenderingUtils.RenderObjectsWithError(context, ref renderingData.cullResults, camera, filterSettings, SortingCriteria.None);
            }
            context.ExecuteCommandBuffer(cmd);
            cmd.Clear();
            //CommandBufferPool.Release(cmd);
            //return;

            ConfigureClear(ClearFlag.Color, Color.clear);
            cmd = CommandBufferPool.Get("distortion postprocess");
            RenderTextureDescriptor opaqueDesc = renderingData.cameraData.cameraTargetDescriptor;
            opaqueDesc.depthBufferBits = 0;
            opaqueDesc.msaaSamples = 1;
            cmd.GetTemporaryRT(m_TemporaryColorTexture.id, opaqueDesc, filterMode);

            //blit pass
            Blit(cmd, source, m_TemporaryColorTexture.Identifier());
            cmd.SetGlobalTexture("_distortion_tex", m_DistortionTexture.id);
            Blit(cmd, m_TemporaryColorTexture.Identifier(), source, blitMaterial);
            context.ExecuteCommandBuffer(cmd);
            cmd.Clear();
            CommandBufferPool.Release(cmd);
        }

        public override void FrameCleanup(CommandBuffer cmd)
        {
            base.FrameCleanup(cmd);
            if (destination == RenderTargetHandle.CameraTarget)
                cmd.ReleaseTemporaryRT(m_TemporaryColorTexture.id);
        }
    }
}
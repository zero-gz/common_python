Shader "my_shader/add_specular" {
    Properties {
    }
    
    CGINCLUDE
    #include "SeparableSubsurfaceScatterCommon.cginc"
    #pragma target 3.0
    ENDCG

    SubShader {
        ZTest Always
        ZWrite Off 
        Cull Off
        Stencil{
            Ref 5
            comp equal
            pass keep
        }
        Pass {
            Name "AddSpecular"
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #pragma multi_compile_fwdbase
            #pragma target 3.0
		    #pragma enable_d3d11_debug_symbols

            float4 frag(VertexOutput i) : SV_TARGET {
                float4 SceneColor = tex2D(_MainTex, i.uv);
				float4 specular_color = tex2D(add_specular_tex, i.uv);
				float SSSDepth = LinearEyeDepth(SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, i.uv)).r;

				float3 result = SceneColor.rgb + specular_color.rgb;
                return float4(result, SceneColor.a);
            }
            ENDCG
        } 
    }
}

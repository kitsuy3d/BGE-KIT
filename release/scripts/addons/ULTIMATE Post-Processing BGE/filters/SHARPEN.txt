#version 120
#pragma optionNV(fastmath on)
#pragma optionNV(fastprecision on)
#pragma optionNV(inline all)
#pragma optionNV(unroll all)
#pragma optionNV(ifcvt none)
#pragma optionNV(strict on)

uniform float Sharpness;

uniform sampler2D bgl_RenderedTexture;
uniform float bgl_RenderedTextureWidth;
uniform float bgl_RenderedTextureHeight;

void main(void)
{
  float width = 1.0 /  bgl_RenderedTextureWidth; 
  float height = 1.0 / bgl_RenderedTextureHeight;
       
  vec4 Total = vec4(0.0);     
       
  vec2 Scoord = gl_TexCoord[0].xy;
  vec4 Msample = texture2D(bgl_RenderedTexture, Scoord);
  
  float Hoffs = 1.5 * width;
  float Voffs = 1.5 * height;
  
  vec2 UV1 = gl_TexCoord[0].xy + vec2(Hoffs,Voffs);  
  Total += texture2D(bgl_RenderedTexture, UV1);
  
  vec2 UV2 = gl_TexCoord[0].xy + vec2(-Hoffs,Voffs);  
  Total += texture2D(bgl_RenderedTexture, UV2);
  
  vec2 UV3 = gl_TexCoord[0].xy + vec2(-Hoffs,-Voffs);  
  Total += texture2D(bgl_RenderedTexture, UV3);
  
  vec2 UV4 = gl_TexCoord[0].xy + vec2(Hoffs,-Voffs);  
  Total += texture2D(bgl_RenderedTexture, UV4);
  
  Total *= .15;
  
  
  Hoffs = 2.5 * width;
  Voffs = 2.5 * height;
  
  vec2 UV5 = gl_TexCoord[0].xy + vec2(Hoffs,0.0);  
  Total += texture2D(bgl_RenderedTexture, UV5) *.1;
  
  vec2 UV6 = gl_TexCoord[0].xy + vec2(0.0,Voffs);  
  Total += texture2D(bgl_RenderedTexture, UV6) *.1;
  
  vec2 UV7 = gl_TexCoord[0].xy + vec2(-Hoffs,0.0);  
  Total += texture2D(bgl_RenderedTexture, UV7) *.1;
  
  vec2 UV8 = gl_TexCoord[0].xy + vec2(0.0,-Voffs);  
  Total += texture2D(bgl_RenderedTexture, UV8) *.1;
  
    
   
  vec4 Final = (1 + Sharpness) * Msample - Total * Sharpness;
  
  
  
        
  gl_FragColor.xyz = Final.xyz;
  gl_FragColor.a = 1.0;
}
uniform sampler2D bgl_DepthTexture;  // Depth texture  
uniform sampler2D bgl_RenderedTexture; // Color texture 
uniform float bgl_RenderedTextureWidth;
uniform float bgl_RenderedTextureHeight;

float samples = 32.0;
float radius = 0.15;
float brightness = 0.8;

uniform vec2 camerarange = vec2(0.1, 100.0);
      
   float pw = radius/(bgl_RenderedTextureWidth); 
   float ph = radius/(bgl_RenderedTextureHeight);  

float rand(in vec2 coord) //generating random noise
{
	float noiseX = clamp(fract(sin(dot(coord ,vec2(12.9898,78.233))) * 43758.5453),0.0,1.0)*0.2+0.5;
	//float noiseX = ((fract(1.0-coord.s*(bgl_RenderedTextureWidth/2.0))*0.25)+(fract(coord.t*(bgl_RenderedTextureHeight/2.0))*0.9));
	return noiseX;
}

   float readDepth(in vec2 coord)  
   {  
     if (coord.x<0.0||coord.y<0.0) return 1.0;
      float nearZ = camerarange.x;  
      float farZ =camerarange.y;  
      float posZ = texture2D(bgl_DepthTexture, coord).x;   
      return (2.0 * nearZ) / (nearZ + farZ - posZ * (farZ - nearZ));  
   }   

   vec3 readColor(in vec2 coord)  
   {  
     return texture2D(bgl_RenderedTexture, coord).xyz;  
   } 

   float compareDepths(in float depth1, in float depth2)  
   {  
     float gauss = 0.0; 
     float diff = (depth1 - depth2)*100.0; //depth difference (0-100)
     float gdisplace = 0.2; //gauss bell center
     float garea = 3.0; //gauss bell width

     //reduce left bell width to avoid self-shadowing
     if (diff<gdisplace) garea = 0.2; 

     gauss = pow(2.7182,-2.0*(diff-gdisplace)*(diff-gdisplace)/(garea*garea));

     return max(0.2,gauss);  
   }  

   vec3 calAO(float depth,float dw, float dh, inout float ao)  
   {  
     float temp = 0.0;
     vec3 bleed = vec3(0.0,0.0,0.0);
     float coordw = gl_TexCoord[0].x + dw/depth;
     float coordh = gl_TexCoord[0].y + dh/depth;

     if (coordw  < 1.0 && coordw  > 0.0 && coordh < 1.0 && coordh  > 0.0){

     	vec2 coord = vec2(coordw , coordh);
     	temp = compareDepths(depth, readDepth(coord)); 
        bleed = readColor(coord); 
 
     }
     ao += temp;
     return temp*bleed;  
   }   
     
   void main(void)  
   {  
     //randomization texture:
     float random = rand(gl_TexCoord[0].st);
     //random = random*2.0-(1.0);

     //initialize stuff:
     float depth = readDepth(gl_TexCoord[0].st);
     vec3 gi = vec3(0.0,0.0,0.0);  
     float ao = 0.0;
  	
	float dx0 = pw;
  	float dy0 = ph;
  	float ang = 0.0;

for (int i = 0; i < int(samples); ++i)
  {
    float dzx =  float(dx0 * random);
    float dzy =  float(dx0 * random);

    float a = radians(ang);

    float dx = cos(a)*dzx-sin(a)*dzy;
    float dy = sin(a)*dzx+cos(a)*dzy;
    
    gi += calAO(depth,  dx, dy, ao);
    
    dx0 += pw;
    dy0 += ph;

    ang+=360.0/6.0;
  }  
     

     //final values, some adjusting:

	vec3 lumcoeff = vec3(0.299,0.587,0.114);
	vec3 color = vec3(readColor(gl_TexCoord[0].st));
	float lum = dot(color, lumcoeff);
	vec3 luminance = vec3(lum, lum, lum);
	vec3 white = vec3(1.0,1.0,1.0);
	vec3 black = vec3(0.0,0.0,0.0);
	vec3 treshold = vec3(0.9,0.9,0.9);
	
	vec3 finalAO = vec3(1.0-(ao/samples));
    vec3 finalGI = (gi/samples);
	
	vec3 mask = clamp(max(black,luminance-treshold)+max(black,luminance-treshold)+max(black,luminance-treshold),0.0,1.0);
	//gl_FragColor = vec4(color*mix(finalAO/3*2+7*finalGI,white,mask),1.0);
	//gl_FragColor = vec4(mix(finalAO+finalGI,white,mask),1.0);
	gl_FragColor = vec4(color*finalAO+finalGI*brightness,1.0);
	
}
#if __VERSION__ < 130
  #define in varying
  #define flat
  #define out varying
#endif

flat in vec4 insideFinalColor;
flat in vec4 outsideFinalColor;
out vec4 fragColor;

void main()
{
	gl_FragData[0] = gl_FrontFacing ? insideFinalColor : outsideFinalColor;
}

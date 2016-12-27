#version 420 core

#define TERRAIN 0
#define TREE 1
#define LEAVES 2
#define SKYBOX 3
#define CLOUD 4

smooth in vec4 colorsExport;

uniform uint object;

in vec4 frontAmbDiffExport, frontSpecExport, backAmbDiffExport, backSpecExport;
in vec2 texCoordsExport;

uniform sampler2D grassTex;



vec4 fieldTexColor;

out vec4 colorsOut;

void main(void)
{
	
	if(object == TREE) colorsOut = colorsExport;
	if(object == LEAVES) colorsOut = colorsExport;
	if(object == TERRAIN)
	{
		fieldTexColor = texture(grassTex, texCoordsExport);
		colorsOut = fieldTexColor * colorsExport;
	}
	if(object == SKYBOX) colorsOut = colorsExport;
	if(object == CLOUD) colorsOut = colorsExport;
	
}
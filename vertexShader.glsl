#version 420 core

#define TERRAIN 0
#define TREE 1
#define LEAVES 2
#define SKYBOX 3
#define CLOUD 4

layout(location=0) in vec4 terrainCoords;
layout(location=1) in vec3 terrainNormals;
layout(location=2) in vec2 terrainTexCoords;
layout(location=3) in vec4 squareCoords;
layout(location=4) in vec4 leafCoords;
layout(location=5) in vec4 skyBoxCoords;
layout(location=6) in vec4 cloudCoords;
layout(location=7) in vec4 cloudColors;

uniform mat4 projMat;
uniform mat4 modelViewMat;
uniform vec4 globAmb;
uniform mat3 normalMat;

vec4 coords;

uniform uint object;

out vec4 frontAmbDiffExport, frontSpecExport, backAmbDiffExport, backSpecExport;
out vec2 texCoordsExport;


struct Material
{
	vec4 ambRefl;
	vec4 difRefl;
	vec4 specRefl;
	vec4 emitCols;
	float shininess;
};

struct Light
{
 vec4 ambCols;
 vec4 difCols;
 vec4 specCols;
 vec4 coords;
};
uniform Light light0;
uniform Material terrainFandB;
vec3 normal;

smooth out vec4 colorsExport;

void main(void)
{
	if(object == TREE)
	{
		coords = squareCoords;
		colorsExport = vec4(0.8, 0.4, 0.1, 1.0);
	}
	if(object == LEAVES)
	{
		coords = leafCoords;
		colorsExport = vec4(0.4, 1.0, 0.4, 1.0);
	}
	if(object == TERRAIN)
	{
		coords = terrainCoords;

		//Diffuse
		normal = normalize(normalMat * terrainNormals);
		vec3 lightDirection = normalize(vec3(light0.coords));
		colorsExport = max(dot(normal, lightDirection), 0.0f) * (light0.difCols * terrainFandB.difRefl);

		texCoordsExport = terrainTexCoords;
	}
	if(object == SKYBOX)
	{
		coords = skyBoxCoords;
		colorsExport = vec4(0.5, 0.5, 1.0, 1.0);
	}
	if(object == CLOUD)
	{
		coords = cloudCoords;
		colorsExport = cloudColors;
	}
	gl_Position = projMat * modelViewMat * coords;
}
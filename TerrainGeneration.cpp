#include <iostream>
#include <fstream>
#include "getbmp.h"
#include "SkyBox.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/glext.h>
#pragma comment(lib, "glew32.lib") 

using namespace std;
using namespace glm;

// Size of the terrain
const int MAP_SIZE = 65;

const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 900;

static BitMapFile *image[1];
SkyBox sky(50.0f, MAP_SIZE);

struct Vertex
{
	vec4 coords;
	vec3 normals;
	vec2 texcoords;
	vec4 colors;
};

struct Vert
{
	vec4 coords;
	vec4 colors;
};

struct Matrix4x4
{
	float entries[16];
};

static mat4 projMat = mat4(1.0);

static const Matrix4x4 IDENTITY_MATRIX4x4 =
{
	{
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	}
};


struct Material
{
	vec4 ambRefl;
	vec4 difRefl;
	vec4 specRefl;
	vec4 emitCols;
	float shininess;
};

static const Material terrainFandB = 
{
	vec4(1.0, 1.0, 1.0 ,1.0),
	vec4(1.0, 1.0, 1.0 ,1.0),
	vec4(1.0, 1.0, 1.0 ,1.0),
	vec4(1.0, 1.0, 1.0 ,1.0),
	50.0f
};

struct Light
{
	vec4 ambCols;
	vec4 difCols;
	vec4 specCols;
	vec4 coords;
};

static const Light light0 =
{
	vec4(0.0, 0.0, 0.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(8.0, 4.0, 8.0, 0.0)
};



// Globals
//897
int seed = 897;

float cloud[MAP_SIZE][MAP_SIZE] = {};
float terrain[MAP_SIZE][MAP_SIZE] = {};
vector<Vertex> skyBoxVerts;
Vertex skyBoxVert[13];
vector<Vertex> leafVertices;
vector<Vertex> treeVertices;
vector<uint> branchIndexData;
vec3 terrainNormals[MAP_SIZE][MAP_SIZE] = {};


float cameraTheta = 0.0f;
float cameraPhi = 0.0f;
vec3 los = vec3(-0.258f, 0.0f, -1.0f);
float speed = 0.2;
vec3 eye = vec3(44.5, -0.95, 63.0);
vec3 up = vec3(0.0, 1.0, 0.0);

int numberOfTrees = 30;
int treeLevels = 10;

static const vec4 globAmb = vec4(0.2, 0.2, 0.2, 1.0);

static enum buffer { TERRAIN_VERTICES, SQUARE_VERTICES, LEAF_VERTS, SKYBOX_VERTS, CLOUD_VERTICES };
static enum object { TERRAIN, SQUARE, LEAF, SKYBOX, CLOUD };

static Vertex terrainVertices[MAP_SIZE*MAP_SIZE] = {};
static Vert cloudVertices[MAP_SIZE*MAP_SIZE] = {};
const int numStripsRequired = MAP_SIZE - 1;
const int verticesPerStrip = 2 * MAP_SIZE;

unsigned int terrainIndexData[numStripsRequired][verticesPerStrip];
unsigned int cloudIndexData[numStripsRequired][verticesPerStrip];
static mat3 normalMat = mat3(1.0);

static unsigned int
texture[1],
grassTexLoc,
programId,
vertexShaderId,
fragmentShaderId,
modelViewMatLoc,
normalMatLoc,
objectLoc,
projMatLoc,
buffer[5],
vao[5];


void shaderCompileTest(GLuint shader)
{
	GLint result = GL_FALSE;
	int logLength;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	std::vector<GLchar> vertShaderError((logLength > 1) ? logLength : 1);
	glGetShaderInfoLog(shader, logLength, NULL, &vertShaderError[0]);
	std::cout << &vertShaderError[0] << std::endl;
}

// Function to read text file, used to read shader files
char* readTextFile(char* aTextFile)
{
	FILE* filePointer = fopen(aTextFile, "rb");
	char* content = NULL;
	long numVal = 0;

	fseek(filePointer, 0L, SEEK_END);
	numVal = ftell(filePointer);
	fseek(filePointer, 0L, SEEK_SET);
	content = (char*)malloc((numVal + 1) * sizeof(char));
	fread(content, 1, numVal, filePointer);
	content[numVal] = '\0';
	fclose(filePointer);
	return content;
}


void BuildTree()
{
	int levels = treeLevels;
	float zRand = 0.3;
	float zConstraint = 1.2;
	for(int x = 0; x < numberOfTrees; x++)
	{
		vector<Vertex> temp;

		int baseX = rand() % MAP_SIZE -1;
		int baseZ = rand() % MAP_SIZE - 1;
		float baseY = terrain[baseX][baseZ];

		

		float baseAngle = 40.0;
		float angleConstraint = 30.0;
		float baseScaler = 0.8;
		float scalerConstraint = 0.2;
		
		int currentIndex;
		vec3 currentVert;

		temp.push_back({ { baseX, baseY, baseZ, 1.0 }, vec3(0.0, 0.0, 0.0), vec2(0.0, 0.0) });
		temp.push_back({{ baseX, baseY + 1, baseZ, 1.0 }, vec3(0.0, 0.0, 0.0), vec2(0.0, 0.0)});

		for (int i = 0; i < levels; i++)
		{
			for (int j = 0; j < pow(2, i); j++)
			{
				//Vert to build branches off
				currentIndex = pow(2, i) + j;
				currentVert = vec3(temp[currentIndex].coords[0], temp[currentIndex].coords[1], temp[currentIndex].coords[2]);

				vec3 parentVert;
				if (currentIndex % 2 == 0)
				{
					parentVert = vec3(temp[currentIndex / 2].coords[0], temp[currentIndex / 2].coords[1], 0.0);
				}
				else
				{
					parentVert = vec3(temp[(currentIndex - 1) / 2].coords[0], temp[(currentIndex - 1) / 2].coords[1], 0.0);
				}

				float angle;
				float scaler;

				//Scale trunk for next branch
				scaler = baseScaler + (-scalerConstraint + (float)(rand() / (float)(RAND_MAX / (scalerConstraint - -scalerConstraint))));
				float newX = (currentVert.x - parentVert.x) * scaler;
				float newY = (currentVert.y - parentVert.y) * scaler;
				float newZ = (currentVert.z - parentVert.z) * scaler;

				newZ = zRand * (-zConstraint + (float)(rand() / (float)(RAND_MAX / (zConstraint - -zConstraint))));

				//vec3 newVert = vec3((currentVert.x - parentVert.x) * scaler, (currentVert.y - parentVert.y) * scaler, 0);
				angle = baseAngle + (-angleConstraint + (float)(rand() / (float)(RAND_MAX / (angleConstraint - -angleConstraint))));

				float leftX = cos(radians(angle / 2)) * newX - sin(radians(angle / 2)) * newY;
				float leftY = sin(radians(angle / 2)) * newX + cos(radians(angle / 2)) * newY;

				vec3 leftVert = vec3(leftX, leftY, newZ) + currentVert;

				angle = baseAngle + (-angleConstraint + (float)(rand() / (float)(RAND_MAX / (angleConstraint - -angleConstraint))));

				float rightX = cos(radians(-angle / 2)) * newX - sin(radians(-angle / 2)) * newY;
				float rightY = sin(radians(-angle / 2)) * newX + cos(radians(-angle / 2)) * newY;

				newZ = zRand * (-zConstraint + (float)(rand() / (float)(RAND_MAX / (zConstraint - -zConstraint))));

				vec3 rightVert = vec3(rightX, rightY, newZ) + currentVert;



				temp.push_back({ { leftVert.x, leftVert.y, leftVert.z, 1.0 }, vec3(0.0, 0.0, 0.0), vec2(0.0,0.0) });
				temp.push_back({ { rightVert.x, rightVert.y, leftVert.z, 1.0 }, vec3(0.0, 0.0, 0.0), vec2(0.0,0.0)});

			}
		}

		for (int s = 0; s < temp.size(); s++)
		{
			treeVertices.push_back(temp[s]);
		}
	}

	int treeStepSize = treeVertices.size() / numberOfTrees;
	int currentTrunkIndex = 0;

	for(int t = 0; t < numberOfTrees; t++)
	{
		branchIndexData.push_back(currentTrunkIndex);
		branchIndexData.push_back(currentTrunkIndex + 1);
		
		for (int l = 1; l < treeStepSize - pow(2, levels); l++)
		{
			branchIndexData.push_back(l + currentTrunkIndex);
			branchIndexData.push_back((l * 2) + currentTrunkIndex);
			branchIndexData.push_back(l + currentTrunkIndex);
			branchIndexData.push_back(((l * 2) + 1) + currentTrunkIndex);
		}
		currentTrunkIndex = currentTrunkIndex + treeStepSize;
	}
		

	//Make Leaves
	currentTrunkIndex = 0;
	int numberOfLeaves = treeStepSize/2;

	for (int t = 0; t < numberOfTrees; t++)
	{
		for (int j = currentTrunkIndex; j < numberOfLeaves; j++)
		{
			vec4 newPoint1 = treeVertices[(t * treeStepSize) + numberOfLeaves + j].coords;
			vec4 newPoint2 = vec4(newPoint1.x + 0.2, newPoint1.y + 0.3, newPoint1.z, 1.0);
			vec4 newPoint3 = vec4(newPoint1.x - 0.2, newPoint1.y + 0.3, newPoint1.z, 1.0);
			vec4 newPoint4 = vec4(newPoint1.x, newPoint1.y + 0.3, newPoint1.z, 1.0);

			leafVertices.push_back({ newPoint1, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });
			leafVertices.push_back({ newPoint2, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });
			leafVertices.push_back({ newPoint3, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });
			leafVertices.push_back({ newPoint3, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });
			leafVertices.push_back({ newPoint2, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });
			leafVertices.push_back({ newPoint4, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });

		}

	}
	


	for (int i = treeVertices.size() - pow(2, levels); i < pow(2, levels + 1); i++)
	{
		vec4 newPoint1 = treeVertices[i].coords;
		vec4 newPoint2 = vec4(newPoint1.x + 0.2, newPoint1.y + 0.3, newPoint1.z, 1.0);
		vec4 newPoint3 = vec4(newPoint1.x - 0.2, newPoint1.y + 0.3, newPoint1.z, 1.0);
		vec4 newPoint4 = vec4(newPoint1.x, newPoint1.y + 0.3, newPoint1.z, 1.0);

		leafVertices.push_back({ newPoint1, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });
		leafVertices.push_back({ newPoint2, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });
		leafVertices.push_back({ newPoint3, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });
		leafVertices.push_back({ newPoint3, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });
		leafVertices.push_back({ newPoint2, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });
		leafVertices.push_back({ newPoint4, vec3(0.0, 1.0, 0.0), vec2(0.0,0.0) });

	}

}






void fillArrayTerrain(float (terrainArray)[][MAP_SIZE], float terrainLevel, float maxRand, float roughness)
{
	int stepSize = MAP_SIZE - 1;
	float randMax = maxRand;
	float H = roughness;
	float randNo;

	for (int x = 0; x < MAP_SIZE; x++)
	{
		for (int z = 0; z < MAP_SIZE; z++)
		{
			terrainArray[x][z] = 0;
		}
	}

	terrainArray[0][0] = -1 + (float)(rand() / (float)(RAND_MAX / (1 - -1)));
	terrainArray[stepSize][0] = -1 + (float)(rand() / (float)(RAND_MAX / (1 - -1)));
	terrainArray[0][stepSize] = -1 + (float)(rand() / (float)(RAND_MAX / (1 - -1)));
	terrainArray[stepSize][stepSize] = -1 + (float)(rand() / (float)(RAND_MAX / (1 - -1)));

	cout << terrain[0][0] << endl;

	while (stepSize > 1)
	{
		//Diamond Step
		for (int x = 0; x < MAP_SIZE - 1; x += stepSize)
		{
			for (int y = 0; y < MAP_SIZE - 1; y += stepSize)
			{
				float c1 = terrainArray[x][y];
				float c2 = terrainArray[x + stepSize][y];
				float c3 = terrainArray[x][y + stepSize];
				float c4 = terrainArray[x + stepSize][y + stepSize];
				float avg = (c1 + c2 + c3 + c4) / 4.0f;
				int mpx = x + stepSize / 2;
				int mpy = y + stepSize / 2;
				randNo = -randMax + (float)(rand() / (float)(RAND_MAX / (randMax - -randMax)));
				terrainArray[mpx][mpy] = avg + randNo;
				
			}
		}
		

		//Square Step
		for (int x = 0; x < MAP_SIZE - 1; x += stepSize)
		{
			for (int y = 0; y < MAP_SIZE - 1; y += stepSize)
			{
				float avg;
				//TopPoint
				if (y - (stepSize / 2) < 0)
				{
					avg = (terrainArray[x][y] + terrainArray[stepSize][y] + terrainArray[stepSize / 2][y + (stepSize / 2)]) / 3;
				}
				else
				{
					avg = (terrainArray[x][y] + terrainArray[stepSize][y] + terrainArray[stepSize / 2][y + (stepSize / 2)] + terrainArray[stepSize / 2][y - (stepSize / 2)]) / 4;
				}
				terrainArray[stepSize / 2][y] = avg + -randMax + (float)(rand() / (float)(RAND_MAX / (randMax - -randMax)));

				//Left Side
				if (x - (stepSize / 2) < 0)
				{
					avg = (terrainArray[x][y] + terrainArray[x][y + stepSize] + terrainArray[x + (stepSize / 2)][y + (stepSize / 2)]) / 3;
				}
				else
				{
					avg = (terrainArray[x][y] + terrainArray[x][y + stepSize] + terrainArray[x + (stepSize / 2)][y + (stepSize / 2)] + terrainArray[x - (stepSize / 2)][y + (stepSize / 2)]) / 4;
				}
				terrainArray[x][y + (stepSize / 2)] = avg + -randMax + (float)(rand() / (float)(RAND_MAX / (randMax - -randMax)));

				//Right Side
				if (x + stepSize + (stepSize / 2) > MAP_SIZE - 1)
				{
					avg = (terrainArray[x + (stepSize / 2)][y + (stepSize / 2)] + terrainArray[stepSize][y] + terrainArray[stepSize][stepSize]) / 3;
				}
				else
				{
					avg = (terrainArray[x + (stepSize / 2)][y + (stepSize / 2)] + terrainArray[x + stepSize][y] + terrainArray[x + stepSize][y + stepSize] + terrainArray[x + stepSize + (stepSize / 2)][y + (stepSize / 2)]) / 4;
				}
				terrainArray[x + stepSize][y + (stepSize / 2)] = avg + -randMax + (float)(rand() / (float)(RAND_MAX / (randMax - -randMax)));

				//Bottom Side
				if (y + stepSize + (stepSize / 2) > MAP_SIZE - 1)
				{
					avg = (terrainArray[x + (stepSize / 2)][y + (stepSize / 2)] + terrainArray[x][y + stepSize] + terrainArray[x + stepSize][y + stepSize]) / 3;
				}
				else
				{
					avg = (terrainArray[x + (stepSize / 2)][y + (stepSize / 2)] + terrainArray[x][y + stepSize] + terrainArray[x + stepSize][y + stepSize] + terrainArray[x + (stepSize / 2)][y + stepSize + (stepSize / 2)]) / 4;
				}
				terrainArray[x + (stepSize / 2)][y + stepSize] = avg + -randMax + (float)(rand() / (float)(RAND_MAX / (randMax - -randMax)));
			}
		}


		randMax = randMax * pow(2, -H);
		stepSize = stepSize / 2;
	}




	//QuadNormals

	

	for (int x = 0; x < MAP_SIZE; x++)
	{
		for (int y = 0; y < MAP_SIZE; y++)
		{
			terrainArray[x][y] = terrainArray[x][y] + terrainLevel;
		}
	}
}


void calcNormals(float(terrainArray)[][MAP_SIZE], vec3(normalArray)[][MAP_SIZE])
{
	vec3 quadNormals[MAP_SIZE - 1][MAP_SIZE - 1][2] = {};


	for (int x = 0; x < MAP_SIZE - 2; x++)
	{
		for (int y = 0; y < MAP_SIZE - 2; y++)
		{
			//Tri 0
			vec3 edge1 = vec3(x + 1, y, terrainArray[x + 1][y]) - vec3(x, y, terrainArray[x][y]);
			vec3 edge2 = vec3(x, y + 1, terrainArray[x][y + 1]) - vec3(x, y, terrainArray[x][y]);
			quadNormals[x][y][0] = cross(edge1, edge2);

			//Tri 1
			edge1 = vec3(x, y + 1, terrainArray[x][y + 1]) - vec3(x + 1, y + 1, terrainArray[x + 1][y + 1]);
			edge2 = vec3(x + 1, y, terrainArray[x + 1][y]) - vec3(x + 1, y + 1, terrainArray[x + 1][y + 1]);
			quadNormals[x][y][1] = cross(edge1, edge2);
		}
	}

	//Vert Normals

	for (int x = 0; x < MAP_SIZE - 1; x++)
	{
		for (int y = 0; y < MAP_SIZE - 1; y++)
		{
			//if top left on corner
			if (x == 0 && y == 0)
			{
				normalArray[x][y] = normalize(quadNormals[x][y][0]);
			}
			//Bottom left corner
			else if (x == 0 && y == MAP_SIZE - 1)
			{
				normalArray[x][y] = normalize(quadNormals[y - 1][x][0] + quadNormals[y - 1][x][1]);
			}
			//top right corner
			else if (x == MAP_SIZE - 1 && y == 0)
			{
				normalArray[x][y] = normalize(quadNormals[x - 1][y][0] + quadNormals[x - 1][y][1]);
			}
			//bottom right
			if (x == MAP_SIZE && y == MAP_SIZE)
			{
				normalArray[x][y] = normalize(quadNormals[x - 1][y - 1][1]);
			}
			//If on left edge
			else if (x == 0 && y != 0)
			{
				normalArray[x][y] = normalize(quadNormals[x][y - 1][0] + quadNormals[x][y - 1][1] + quadNormals[x][y][0]);
			}
			//If on top edge
			else if (y == 0 && x != 0)
			{
				normalArray[x][y] = normalize(quadNormals[x - 1][y][0] + quadNormals[x - 1][y][1] + quadNormals[x][y][0]);
			}
			//If on Right edge
			else if (x == MAP_SIZE - 1 && y != MAP_SIZE - 1)
			{
				normalArray[x][y] = normalize(quadNormals[x][y - 1][1] + quadNormals[x][y][0] + quadNormals[x][y][1]);
			}
			//If on bottom edge
			else if (y == MAP_SIZE - 1 && x != MAP_SIZE - 1)
			{
				normalArray[x][y] = normalize(quadNormals[x - 1][y - 1][1] + quadNormals[x][y - 1][0] + quadNormals[x][y - 1][1]);
			}
			else
			{
				normalArray[x][y] = normalize(quadNormals[x - 1][y - 1][1] + quadNormals[x][y - 1][0] + quadNormals[x][y - 1][1] + quadNormals[x - 1][y][0] + quadNormals[x - 1][y][1] + quadNormals[x][y][0]);
			}
		}
	}

}




// Initialization routine.
void setup(void)
{
	float cloudAlpha[MAP_SIZE][MAP_SIZE] = {};
	srand(seed);
	// Initialise terrain - set values in the height map to 0
	

	for (int x = 0; x < MAP_SIZE; x++)
	{
		for (int z = 0; z < MAP_SIZE; z++)
		{
			terrain[x][z] = 0;
		}
	}

	fillArrayTerrain(cloud, 18, 2, 0.5);
	fillArrayTerrain(terrain, 0, 8.0, 1.0);
	calcNormals(terrain, terrainNormals);
	


	float highestCloud = cloud[0][0];
	float lowestCloud = cloud[0][0];

	for (int z = 0; z < MAP_SIZE - 1; z++)
	{
		for (int x = 0; x < MAP_SIZE; x++)
		{
			if(cloud[x][z] > highestCloud) highestCloud = cloud[x][z];

			if(cloud[x][z] < lowestCloud) lowestCloud = cloud[x][z];
		}
	}

	for (int z = 0; z < MAP_SIZE - 1; z++)
	{
		for (int x = 0; x < MAP_SIZE; x++)
		{
			float opacity = (cloud[x][z] - lowestCloud) / (highestCloud - lowestCloud);
			cloudAlpha[x][z] = opacity;
		}
	}



	// TODO: Add your code here to calculate the height values of the terrain using the Diamond square algorithm
	

	//	terrainVertices

	// Intialise vertex array
	int i = 0;
	float fTextureS = float(MAP_SIZE) * 0.1f;
	float fTextureT = float(MAP_SIZE) * 0.1f;


	for (int z = 0; z < MAP_SIZE; z++)
	{
		for (int x = 0; x < MAP_SIZE; x++)
		{
			// Set the coords (1st 4 elements) and a default colour of black (2nd 4 elements) 
			terrainVertices[i] = { { (float)x, terrain[x][z], (float)z, 1.0 }, terrainNormals[x][z] };
			cloudVertices[i] = { { (float)x, cloud[x][z], (float)z, 1.0 } ,vec4(1.0, 1.0, 1.0, cloudAlpha[x][z]) }; //cloudAlpha[x][z]

			float fScaleC = float(x) / float(MAP_SIZE - 1);
			float fScaleR = float(z) / float(MAP_SIZE - 1);
			terrainVertices[i].texcoords = vec2(fTextureS*fScaleC,fTextureT*fScaleR);



			i++;
		}
	}

	// Now build the index data 
	i = 0;
	for (int z = 0; z < MAP_SIZE - 1; z++)
	{
		i = z * MAP_SIZE;
		for (int x = 0; x < MAP_SIZE * 2; x += 2)
		{
			terrainIndexData[z][x] = i;
			cloudIndexData[z][x] = i;
			i++;
		}
		for (int x = 1; x < MAP_SIZE * 2 + 1; x += 2)
		{
			terrainIndexData[z][x] = i;
			cloudIndexData[z][x] = i;
			i++;
		}
	}


	for (int j = 0; j < 13; j++)
	{
		skyBoxVert[j] = {sky.points[j], vec3(0.0,0.0,0.0), vec2(0.0,0.0) };
	}


	BuildTree();

	
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glEnable(GL_DEPTH_TEST);


	image[0] = getbmp("grass.bmp");

	glGenTextures(1, texture);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image[0]->sizeX, image[0]->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[0]->data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	grassTexLoc = glGetUniformLocation(programId, "grassTex");
	glUniform1i(grassTexLoc, 0);



	// Create shader program executable - read, compile and link shaders
	char* vertexShader = readTextFile("vertexShader.glsl");
	vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderId, 1, (const char**)&vertexShader, NULL);
	glCompileShader(vertexShaderId);
	std::cout << "Vertex" << endl;
	shaderCompileTest(vertexShaderId);

	char* fragmentShader = readTextFile("fragmentShader.glsl");
	fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderId, 1, (const char**)&fragmentShader, NULL);
	glCompileShader(fragmentShaderId);
	std::cout << "fragment" << endl;
	shaderCompileTest(fragmentShaderId);

	programId = glCreateProgram();
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);
	glLinkProgram(programId);
	glUseProgram(programId);
	///////////////////////////////////////

	// Create vertex array object (VAO) and vertex buffer object (VBO) and associate data with vertex shader.
	glGenVertexArrays(5, vao);
	glGenBuffers(5, buffer);


	glBindVertexArray(vao[TERRAIN]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer[TERRAIN_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(terrainVertices), terrainVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), (GLvoid*)sizeof(terrainVertices[0].coords));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), (GLvoid*)(sizeof(terrainVertices[0].coords) + sizeof(terrainVertices[0].normals)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(vao[SQUARE]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer[SQUARE_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, treeVertices.size() * sizeof(Vertex), treeVertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(treeVertices[0]), 0);
	glEnableVertexAttribArray(3);


	glBindVertexArray(vao[LEAF]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer[LEAF_VERTS]);
	glBufferData(GL_ARRAY_BUFFER, leafVertices.size() * sizeof(Vertex), leafVertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(leafVertices[0]), 0);
	glEnableVertexAttribArray(4);


	glBindVertexArray(vao[SKYBOX]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer[SKYBOX_VERTS]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyBoxVert), skyBoxVert, GL_STATIC_DRAW);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(skyBoxVerts[0]), 0);
	glEnableVertexAttribArray(5);

	glBindVertexArray(vao[CLOUD]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer[CLOUD_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cloudVertices), cloudVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(cloudVertices[0]), 0);
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(cloudVertices[0]), (GLvoid*)(sizeof(cloudVertices[0].coords)));
	glEnableVertexAttribArray(7);
	///////////////////////////////////////`


	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.ambRefl"), 1, &terrainFandB.ambRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.difRefl"), 1, &terrainFandB.difRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.specRefl"), 1, &terrainFandB.specRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.emitCols"), 1, &terrainFandB.emitCols[0]);
	glUniform1f(glGetUniformLocation(programId, "terrainFandB.shininess"), terrainFandB.shininess);
	glUniform4fv(glGetUniformLocation(programId, "globAmb"), 1, &globAmb[0]);

	glUniform4fv(glGetUniformLocation(programId, "light0.ambCols"), 1, &light0.ambCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.difCols"), 1, &light0.difCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.specCols"), 1, &light0.specCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.coords"), 1, &light0.coords[0]);


	
}

// Drawing routine.
void drawScene(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Obtain projection matrix uniform location and set value.
	projMatLoc = glGetUniformLocation(programId, "projMat");
	projMat = perspective(radians(60.0), (double)SCREEN_WIDTH / (double)SCREEN_HEIGHT, 0.1, 200.0);
	glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, value_ptr(projMat));

	///////////////////////////////////////

	// Obtain modelview matrix uniform location and set value.
	mat4 modelViewMat = mat4(1.0);
	// Move terrain into view - glm::translate replaces glTranslatef
	//modelViewMat = translate(modelViewMat, vec3(-16.5f, -2.5f, -20.0f)); // 5x5 grid
	modelViewMatLoc = glGetUniformLocation(programId, "modelViewMat");
	modelViewMat = lookAt(eye, eye+los, up);
	glUniformMatrix4fv(modelViewMatLoc, 1, GL_FALSE, value_ptr(modelViewMat));

	normalMatLoc = glGetUniformLocation(programId, "normalMat");
	// Calculate and update normal matrix, after any changes to the view matrix
	normalMat = transpose(inverse(mat3(modelViewMat)));
	glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, value_ptr(normalMat));

	///////////////////////////////////////

	objectLoc = glGetUniformLocation(programId, "object");

	
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// For each row - draw the triangle strip
	glUniform1ui(objectLoc, TERRAIN);
	glBindVertexArray(vao[TERRAIN]);
	for (int i = 0; i < MAP_SIZE - 1; i++)
	{
		glDrawElements(GL_TRIANGLE_STRIP, verticesPerStrip, GL_UNSIGNED_INT, terrainIndexData[i]);
	}

	glUniform1ui(objectLoc, SQUARE);
	glBindVertexArray(vao[SQUARE]);
	glDrawElements(GL_LINES, branchIndexData.size(), GL_UNSIGNED_INT, branchIndexData.data());

	glUniform1ui(objectLoc, LEAF);
	glBindVertexArray(vao[LEAF]);
	glDrawArrays(GL_TRIANGLES, 0, leafVertices.size());

	
	glUniform1ui(objectLoc, SKYBOX);
	glBindVertexArray(vao[SKYBOX]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 13);
	
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glUniform1ui(objectLoc, CLOUD);
	glBindVertexArray(vao[CLOUD]);
	for (int i = 0; i < MAP_SIZE - 1; i++)
	{
		glDrawElements(GL_TRIANGLE_STRIP, verticesPerStrip, GL_UNSIGNED_INT, cloudIndexData[i]);
	}

	glFlush();
}

// OpenGL window reshape routine.
void resize(int w, int h)
{
	glViewport(0, 0, w, h);
}

// Keyboard input processing routine.
void keyInput(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 119: //w
		cout << "Eye: " << eye.x << "," << eye.y << "," << eye.z << endl;
		cout << "LOS: " << los.x << "," << los.y << "," << los.z << endl;
		eye = eye + los * speed;
		glutPostRedisplay();
		break;
	case 97: //a
		cameraTheta -= 1;
		los.x = cos(radians(cameraPhi)) * sin(radians(cameraTheta));
		los.y = sin(radians(cameraPhi));
		los.z = cos(radians(cameraPhi)) * -cos(radians(cameraTheta));
		glutPostRedisplay();
		glutPostRedisplay();
		break;
	case 115: //s
		eye = eye - los * speed;
		glutPostRedisplay();
		break;
	case 100: //d
		cameraTheta += 1;
		los.x = cos(radians(cameraPhi)) * sin(radians(cameraTheta));
		los.y = sin(radians(cameraPhi));
		los.z = cos(radians(cameraPhi)) * -cos(radians(cameraTheta));
		glutPostRedisplay();
		break;
	case 114: //f
		cameraPhi += 1;
		los.x = cos(radians(cameraPhi)) * sin(radians(cameraTheta));
		los.y = sin(radians(cameraPhi));
		los.z = cos(radians(cameraPhi)) * -cos(radians(cameraTheta));
		glutPostRedisplay();
		break;
	case 116:
		eye.y += speed;
		glutPostRedisplay();
		break;
	case 103:
		eye.y -= speed;
		glutPostRedisplay();
		break;

	case 102: //r
		cameraPhi -= 1;
		los.x = cos(radians(cameraPhi)) * sin(radians(cameraTheta));
		los.y = sin(radians(cameraPhi));
		los.z = cos(radians(cameraPhi)) * -cos(radians(cameraTheta));
		glutPostRedisplay();
		break;


	case 27:
		exit(0);
		break;
	default:
		break;
	}
}

void printInfo()
{
	cout << "W and S to move Forwad/Backward" << endl;
	cout << "A and D to rotate Lef/Right" << endl;
	cout << "R and F to look up/down" << endl;
	cout << "T and G to move up/down" << endl;
}

// Main routine.
int main(int argc, char* argv[])
{
	printInfo();
	glutInit(&argc, argv);

	// Set the version of OpenGL (4.2)
	glutInitContextVersion(4, 2);
	// The core profile excludes all discarded features
	glutInitContextProfile(GLUT_CORE_PROFILE);
	// Forward compatibility excludes features marked for deprecation ensuring compatability with future versions
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("TerrainGeneration");

	// Set OpenGL to render in wireframe mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(resize);
	glutKeyboardFunc(keyInput);

	glewExperimental = GL_TRUE;
	glewInit();

	setup();

	glutMainLoop();
}

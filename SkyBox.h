#pragma once
#include <glm\glm.hpp>

using namespace glm;

class SkyBox
{
public:
	float height;
	vec4 points[13];

	SkyBox(float, int);

};
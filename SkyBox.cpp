#include "SkyBox.h"

SkyBox::SkyBox(float _height, int _mapSize)
{
	height = _height;
	
	points[0] = vec4(0.0, -_height, 0.0, 1.0);
	points[1] = vec4(0.0, _height, 0.0, 1.0);
	points[2] = vec4(_mapSize, -_height, 0.0, 1.0);
	points[3] = vec4(_mapSize, _height, 0, 1.0);
	points[4] = vec4(_mapSize, -_height, _mapSize, 1.0);
	points[5] = vec4(_mapSize, _height, _mapSize, 1.0);
	points[6] = vec4(0.0, -_height, _mapSize, 1.0);
	points[7] = vec4(0.0, _height, _mapSize, 1.0);
	points[8] = vec4(0.0, -_height, 0.0, 1.0);
	points[9] = vec4(0.0, _height, 0.0, 1.0);
	points[10] = vec4(0.0, _height, _mapSize, 1.0);
	points[11] = vec4(_mapSize, _height, 0, 1.0);
	points[12] = vec4(_mapSize, _height, _mapSize, 1.0);
	points[13] = vec4(0.0, _height, _mapSize, 1.0);
}


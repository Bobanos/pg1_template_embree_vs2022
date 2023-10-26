#ifndef SPHERICAL_MAP_H_
#define SPHERICAL_MAP_H_

#include <string>
#include <memory>

#define _USE_MATH_DEFINES
#include <math.h>

#include "structs.h"
#include "texture.h"

class SphericalMap
{
public:
	SphericalMap(const std::string& file_name);
	Color3f texel(const float x, const float y, const float z) const;
private:
	std::unique_ptr<Texture> texture_;
};

#endif

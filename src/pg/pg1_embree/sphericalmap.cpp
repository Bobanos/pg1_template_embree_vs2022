#include "stdafx.h"
#include "sphericalmap.h"

SphericalMap::SphericalMap(const std::string& file_name)
{
	// TODO 1 load spherical texture from file_name and store it in texture_
	texture_ = std::make_unique<Texture>(file_name.c_str());
}
Color3f SphericalMap::texel(const float x, const float y, const float z) const
{
	// TODO 2 compute (u, v) coordinates from direction (x, y, z) using spherical mapping
	const float u = (atan2f(y, x) + ((y < 0.0f) ? 2.0f * float(M_PI) : 0.0f)) / (2.0f * float(M_PI));
	const float v = acosf(z) / float(M_PI);


	// TODO 3 return bilinear interpolation of a texel at (u, v)
	return texture_->get_texel_interpolate(u, v);
}
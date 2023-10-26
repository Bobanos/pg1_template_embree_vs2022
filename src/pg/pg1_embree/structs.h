#pragma once


struct Vertex3f { float x, y, z; }; // a single vertex position structure matching certain format

using Normal3f = Vertex3f; // a single vertex normal structure matching certain format

struct Coord2f { float u, v; }; // texture coord structure

struct Triangle3ui { unsigned int v0, v1, v2; }; // indicies of a single triangle, the struct must match certain format, e.g. RTC_FORMAT_UINT3

struct RTC_ALIGN( 16 ) Color4f
{
	struct { float r{0}, g{ 0 }, b{ 0 }, a{ 1 }; }; // a = 1 means that the pixel is opaque

	float compress(const float u, const float gamma) {
		if (u <= 0.f) { return 0; }
		if (u >= 1.f) { return 1; }
		if (u <= 0.0031308f) { return 12.92f * u; }
		else{ 
			return 1.055f * powf(u,1.0f / gamma) - 0.055f;
		}
						
	}
	void compress(const float gamma = 2.4f) {
		r = compress(r, gamma);
		g = compress(g, gamma);
		b = compress(b, gamma);
	}
	float expand(const float u, const float gamma ) {
		if (u <= 0.0f) { return 0.0f; }
		if (u >= 1.0f) { return 1.0f; }
		if (u <= 0.04045f) { return u / 12.92f; }
		else {
			return powf((u + 0.055f) / 1.055f, gamma);
		}				
	}
	void expand(const float gamma = 2.4f) {
		r = expand(r, gamma);
		g = expand(g, gamma);
		b = expand(b, gamma);
	}
	
};

struct Color3f {
	struct { float r, g, b; };
	
	float compress(const float u, const float gamma) {
		if (u <= 0.f) { return 0; }
		if (u >= 1.f) { return 1; }
		if (u <= 0.0031308f) { return 12.92f * u; }
		else {
			return 1.055f * powf(u, 1.0f / gamma) - 0.055f;
		}

	}
	void compress(const float gamma = 2.4f) {
		r = compress(r, gamma);
		g = compress(g, gamma);
		b = compress(b, gamma);
	}
	float expand(const float u, const float gamma) {
		if (u <= 0.0f) { return 0.0f; }
		if (u >= 1.0f) { return 1.0f; }
		if (u <= 0.04045f) { return u / 12.92f; }
		else {
			return powf((u + 0.055f) / 1.055f, gamma);
		}
	}
	void expand(const float gamma = 2.4f) {
		r = expand(r, gamma);
		g = expand(g, gamma);
		b = expand(b, gamma);
	}
};

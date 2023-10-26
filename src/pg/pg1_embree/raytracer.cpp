#include "stdafx.h"
#include "raytracer.h"
#include "objloader.h"
#include "tutorials.h"

Raytracer::Raytracer(const int width, const int height,
	const float fov_y, const Vector3 view_from, const Vector3 view_at,
	const char* config) : SimpleGuiDX11(width, height)
{
	InitDeviceAndScene(config);

	camera_ = Camera(width, height, fov_y, view_from, view_at);
}

Raytracer::~Raytracer()
{
	ReleaseDeviceAndScene();
}

int Raytracer::InitDeviceAndScene(const char* config)
{
	device_ = rtcNewDevice(config);
	error_handler(nullptr, rtcGetDeviceError(device_), "Unable to create a new device.\n");
	rtcSetDeviceErrorFunction(device_, error_handler, nullptr);

	ssize_t triangle_supported = rtcGetDeviceProperty(device_, RTC_DEVICE_PROPERTY_TRIANGLE_GEOMETRY_SUPPORTED);

	// create a new scene bound to the specified device
	scene_ = rtcNewScene(device_);

	return S_OK;
}

int Raytracer::ReleaseDeviceAndScene()
{
	rtcReleaseScene(scene_);
	rtcReleaseDevice(device_);

	return S_OK;
}

void Raytracer::LoadScene(const std::string file_name)
{
	const int no_surfaces = LoadOBJ(file_name.c_str(), surfaces_, materials_);

	// surfaces loop
	for (auto surface : surfaces_)
	{
		RTCGeometry mesh = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);

		Vertex3f* vertices = (Vertex3f*)rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
			sizeof(Vertex3f), 3 * surface->no_triangles());

		Triangle3ui* triangles = (Triangle3ui*)rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
			sizeof(Triangle3ui), surface->no_triangles());

		rtcSetGeometryUserData(mesh, (void*)(surface->get_material()));

		rtcSetGeometryVertexAttributeCount(mesh, 2);

		Normal3f* normals = (Normal3f*)rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3,
			sizeof(Normal3f), 3 * surface->no_triangles());

		Coord2f* tex_coords = (Coord2f*)rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT2,
			sizeof(Coord2f), 3 * surface->no_triangles());

		// triangles loop
		for (int i = 0, k = 0; i < surface->no_triangles(); ++i)
		{
			Triangle& triangle = surface->get_triangle(i);

			// vertices loop
			for (int j = 0; j < 3; ++j, ++k)
			{
				const Vertex& vertex = triangle.vertex(j);

				vertices[k].x = vertex.position.x;
				vertices[k].y = vertex.position.y;
				vertices[k].z = vertex.position.z;

				normals[k].x = vertex.normal.x;
				normals[k].y = vertex.normal.y;
				normals[k].z = vertex.normal.z;

				tex_coords[k].u = vertex.texture_coords[0].u;
				tex_coords[k].v = vertex.texture_coords[0].v;
			} // end of vertices loop

			triangles[i].v0 = k - 3;
			triangles[i].v1 = k - 2;
			triangles[i].v2 = k - 1;
		} // end of triangles loop

		rtcCommitGeometry(mesh);
		unsigned int geom_id = rtcAttachGeometry(scene_, mesh);
		rtcReleaseGeometry(mesh);
	} // end of surfaces loop

	rtcCommitScene(scene_);
}

void Raytracer::LoadBackground(const std::string file_name)
{
	background_ = std::make_unique<SphericalMap>(file_name);
}


bool Raytracer::is_visible(const Vector3 x, const Vector3 y)
{
	Vector3 l = y - x;
	float dist = l.L2Norm();
	l *= 1.0 / dist;
	RTCRay ray;
	ray.org_x = x.x; // ray origin
	ray.org_y = x.y;
	ray.org_z = x.z;
	ray.tnear = 0.001f; // start of ray segment

	ray.dir_x = l.x; // ray direction
	ray.dir_y = l.y;
	ray.dir_z = l.z;
	ray.time = 0.0f; // time of this ray for motion blur

	ray.tfar = dist;// end of ray segment (set to hit distance)

	ray.mask = 0; // can be used to mask out some geometries for some rays
	ray.id = 0; // identify a ray inside a callback function
	ray.flags = 0; // reserved

	// setup a hit
	RTCHit hit;
	hit.geomID = RTC_INVALID_GEOMETRY_ID;
	hit.primID = RTC_INVALID_GEOMETRY_ID;
	hit.Ng_x = 0.0f; // geometry normal
	hit.Ng_y = 0.0f;
	hit.Ng_z = 0.0f;

	// merge ray and hit structures
	RTCRayHit ray_hit;
	ray_hit.ray = ray;
	ray_hit.hit = hit;

	// intersect ray with the scene
	RTCIntersectContext context;
	rtcInitIntersectContext(&context);
	rtcIntersect1(scene_, &context, &ray_hit);
	//rtcOccluded1(scene_, &context, &ray);

	if (ray_hit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		return false;
	}
	else
	{
		return true;
	}
}

RTCRay make_secondary(const Vector3 &origin, const Vector3 &dir) {

	RTCRay ray = RTCRay();
	ray.org_x = origin.x; // ray origin
	ray.org_y = origin.y;
	ray.org_z = origin.z;
	ray.tnear = 0.001f; // start of ray segment

	ray.dir_x = dir.x; // ray direction
	ray.dir_y = dir.y;
	ray.dir_z = dir.z;
	ray.time = 0.0f; // time of this ray for motion blur

	ray.tfar = FLT_MAX;// end of ray segment (set to hit distance)

	ray.mask = 0; // can be used to mask out some geometries for some rays
	ray.id = 0; // identify a ray inside a callback function
	ray.flags = 0; // reserved

	return ray;

}


float clamp(const float value) {
	return max(min(value,1.0f), 0.0f);
}

Vector3 Raytracer::TraceRay(RTCRay ray, const int depth, const int max_depth ) {
	if (depth >= max_depth) return Vector3();
	// merge ray and hit structures
	RTCRayHit ray_hit;

	ray_hit.hit = RTCHit();
	ray_hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
	ray_hit.hit.primID = RTC_INVALID_GEOMETRY_ID;

	ray_hit.hit.Ng_x = 0.0f; // geometry normal
	ray_hit.hit.Ng_y = 0.0f;
	ray_hit.hit.Ng_z = 0.0f;

	ray_hit.ray = ray;


	RTCIntersectContext context;
	rtcInitIntersectContext(&context);
	rtcIntersect1(scene_, &context, &ray_hit);

	if (ray_hit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		// we hit something
		RTCGeometry geometry = rtcGetGeometry(scene_, ray_hit.hit.geomID);
		Normal3f normal;
		//interpolovana normala
		rtcInterpolate0(geometry, ray_hit.hit.primID, ray_hit.hit.u, ray_hit.hit.v,
			RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, &normal.x, 3);

		Material* material = (Material*)rtcGetGeometryUserData(geometry);
		assert(material);




		Coord2f tex_coord;
		rtcInterpolate0(geometry, ray_hit.hit.primID, ray_hit.hit.u, ray_hit.hit.v,
			RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, &tex_coord.u, 2);

		Vector3 diffuse_color = Vector3{ material->diffuse.x, material->diffuse.y, material->diffuse.z };
		Texture* diffuse_texture = material->get_texture(Material::kDiffuseMapSlot);
		if (diffuse_texture)
		{
			Color3f diffuse_texel = diffuse_texture->get_texel_interpolate(tex_coord.u, 1 - tex_coord.v);
			diffuse_color.x = diffuse_texel.r;
			diffuse_color.y = diffuse_texel.g;
			diffuse_color.z = diffuse_texel.b;
		}

		Vector3 specular_color = Vector3{ material->specular.x, material->specular.y, material->specular.z };
		Texture* specular_texture = material->get_texture(Material::kSpecularMapSlot);
		if (specular_texture)
		{
			Color3f specular_texel = specular_texture->get_texel_interpolate(tex_coord.u, 1 - tex_coord.v);
			specular_color.x = specular_texel.r;
			specular_color.y = specular_texel.g;
			specular_color.z = specular_texel.b;
		}

		const Vector3 hitpoint{
			ray_hit.ray.org_x + ray_hit.ray.dir_x * ray_hit.ray.tfar ,
			ray_hit.ray.org_y + ray_hit.ray.dir_y * ray_hit.ray.tfar  ,
			ray_hit.ray.org_z + ray_hit.ray.dir_z * ray_hit.ray.tfar };

		Vector3 d{ ray_hit.ray.dir_x , ray_hit.ray.dir_y, ray_hit.ray.dir_z };
		//Vector3 d = Vector3(ray_hit.ray.dir_x, ray_hit.ray.dir_y, ray_hit.ray.dir_z);
		//d.Normalize();
		Vector3 n{ normal.x,normal.y, normal.z };

		if (n.DotProduct(d) > 0.0f) { n *= -1.0f; }

		Vector3 l = light_position - hitpoint;
		l.Normalize();


		Vector3 output_color;
		output_color += material->ambient;
		if (is_visible(hitpoint, light_position))
		{
			//output_color += diffuse_color * clamp(n.dotproduct(l));

			//vector3 l_r = 2 * (l.dotproduct(n) * n) - l;
			//l_r.normalize();

			//output_color += specular_color * powf(clamp(l_r.dotproduct(-d)), material->shininess) ;

			//vector3 l_d = d - 2 * (d.dotproduct(n) * n);
			//l_d.normalize();

			//rtcray secondary_ray = make_secondary(hitpoint, l_d);
			//vector3 l_i = traceray(secondary_ray, depth + 1);
			//output_color += specular_color * l_i * 0.25;

			// Calculate the diffuse component
			float diffuse_intensity = n.DotProduct(l);
			if (diffuse_intensity > 0.0f) {
				output_color += diffuse_color * clamp(diffuse_intensity);
			}

			// Calculate the specular component
			Vector3 l_r = 2 * (l.DotProduct(n) * n) - l;
			l_r.Normalize();

			float specular_intensity = l_r.DotProduct(-d);
			if (specular_intensity > 0.0f) {
				output_color += specular_color * powf(clamp(specular_intensity), material->shininess);
			}

			// Recursive ray for specular reflection
			Vector3 l_d = d - 2 * (d.DotProduct(n) * n);
			l_d.Normalize();

			RTCRay secondary_ray = make_secondary(hitpoint, l_d);
			Vector3 L_i = TraceRay(secondary_ray, depth + 1);
			output_color += specular_color * L_i * 0.25;
		}
		return output_color;
	}
	else {
		Color3f bg_color = background_->texel(ray.dir_x, ray.dir_y, ray.dir_z); // background texture
		//bg_color.expand();
		return Vector3(bg_color.r, bg_color.g, bg_color.b);
	}
}

Color4f Raytracer::get_pixel(const int x, const int y, const float t)
{
	auto ray = camera_.GenerateRay(x, y);

	return static_cast<Color4f>(TraceRay(ray));
}

int Raytracer::Ui()
{
	static float f = 0.0f;
	static int counter = 0;

	// Use a Begin/End pair to created a named window
	ImGui::Begin("Ray Tracer Params");
	ImGui::Text("Surfaces = %d", surfaces_.size());
	ImGui::Text("Materials = %d", materials_.size());
	ImGui::Separator();
	ImGui::Checkbox("Vsync", &vsync_);
	//ImGui::Checkbox( "Demo Window", &show_demo_window ); // Edit bools storing our window open/close state
	//ImGui::Checkbox( "Another Window", &show_another_window );

	//ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f    
	ImGui::SliderFloat3("Camera position", camera_position, -1000.0f, 1000.0f);
	ImGui::SliderFloat3("Light position", light_position, -1000.0f, 1000.0f);
	//ImGui::ColorEdit3( "clear color", ( float* )&clear_color ); // Edit 3 floats representing a color

	// Buttons return true when clicked (most widgets return true when edited/activated)
	if (ImGui::Button("Button"))
		counter++;
	ImGui::SameLine();
	ImGui::Text("counter = %d", counter);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	// 3. Show another simple window.
	/*if ( show_another_window )
	{
	ImGui::Begin( "Another Window", &show_another_window ); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
	ImGui::Text( "Hello from another window!" );
	if ( ImGui::Button( "Close Me" ) )
	show_another_window = false;
	ImGui::End();
	}*/

	return 0;
}
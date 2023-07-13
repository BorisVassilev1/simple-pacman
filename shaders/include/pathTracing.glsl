#ifdef GL_ES
precision highp float;
#endif

#define FLOAT_MAX 3.4028235e+38

struct Material {
	vec3  albedo;
	float specular_chance;

	vec3  emission;
	float ior;

	vec3  transparency_color;
	float refraction_chance;

	vec3  specular_color;
	float refraction_roughness;

	float specular_roughness;
	float metallic;
	int	  normal_map;
	float use_normal_map;

	int	  roughness_map;
	float use_roughness_map;
	int	  ao_map;
	float use_ao_map;

	int	  metallic_map;
	float use_metallic_map;
	int	  albedo_map;
	float use_albedo_map;

	int	  emission_map;
	float use_emission_map;
};	   // 96 bytes all

struct Sphere {
	vec3  position;
	float radius;	  // 16
	uint  matIdx;
};	   // 32

struct Ray {
	vec3 origin;
	vec3 direction;
};

struct RayHit {
	float dist;
	uint  type;
	vec2 uv;
	uint  objIdx;
	/*
		0: nothing
		1: ground plane
		2: sphere
		3: triangle
		4: box
	*/
};

struct HitInfo {
	vec3  pos;
	float dist;
	vec3  normal;
	vec2 texCoords;
	float isFrontFace;
	uint  matIdx;
};

struct Triangle {
	vec3 v0;
	uint matIdx;
	vec3 v1;
	vec3 v2;
	vec3 normal0;
	vec3 normal1;
	vec3 normal2;
	vec2 tc0;
	vec2 tc1;
	vec2 tc2;
	vec3 t0;
	vec3 t1;
	vec3 t2;
};

struct FastTriangle {
	vec3 v0;
	vec3 v1;
	vec3 v2;
};

struct Box {
	vec3 min;
	uint matIdx;
	vec3 max;
};	   // 32 bytes

struct AABB {
	vec3 min;
	vec3 max;
};

struct BVHNode {
	vec3 min;
	uint parent;
	vec3 max;
	uint right; // right child. Left child is next in the array

	uint objOffset; // offset for objects if it is a leaf
	uint objCount;
};

layout(std140, binding = 1) uniform Materials { Material materials[100]; };

layout(std430, binding = 2) readonly buffer Vertices { float vertices[]; };
layout(std430, binding = 3) readonly buffer Normals { float normals[]; };
layout(std430, binding = 8) readonly buffer TexCoords { float texCoords[]; };
layout(std430, binding = 9) readonly buffer Tangents { float tangents[]; };
layout(std430, binding = 4) readonly buffer Indices { int indices[]; };

layout(std430, binding = 5) readonly buffer BVH { BVHNode nodes[]; };
layout(std430, binding = 7) readonly buffer Primitives { uint primitives[]; };

layout(binding = 11) uniform samplerCube skybox;
layout(binding = 1) uniform sampler2D albedoMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform sampler2D heightMap;
layout(binding = 4) uniform sampler2D roughnessMap;
layout(binding = 5) uniform sampler2D aoMap;
layout(binding = 6) uniform sampler2D emissionMap;
layout(binding = 10) uniform sampler2D metallicMap;

uniform mat4  cameraMatrix;
uniform vec2  resolution;
uniform float fov;
uniform int	  samples_per_pixel = 1;

uniform uint random_seed;

uniform mat4 bvh_matrix;

float phi = 0.0001;

float PI	 = 3.14159;
float aspect = resolution.y / resolution.x;

int to1d(ivec2 position, ivec2 maximum) { return position.x + maximum.x * position.y; }

int to1d(ivec3 position, ivec3 maximum) {
	return (position.x + maximum.x * position.y) + maximum.y * maximum.x * position.z;
}

float angleBetween(vec3 v1, vec3 v2) {
	return acos(dot(normalize(v1), normalize(v2)));		/// 3.14159;
}

uint PCGHash(inout uint seed) {
	seed	  = seed * 747796405u + 2891336453u;
	uint word = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
	return (word >> 22u) ^ word;
}

float randomFloat(inout uint seed) { return float(PCGHash(seed)) / 4294967296.0; }

vec3 random_in_unit_sphere(inout uint seed) {
	float r1 = randomFloat(seed);
	float r2 = randomFloat(seed);
	float x	 = cos(2 * PI * r1) * 2 * sqrt(r2 * (1 - r2));
	float y	 = sin(2 * PI * r1) * 2 * sqrt(r2 * (1 - r2));
	float z	 = 1 - 2 * r2 + 0.0000001;
	return vec3(x, y, z);
}

vec3 cosWeightedHemissphereDir(vec3 normal, inout uint seed) {
	float z = randomFloat(seed) * 2.0 - 1.0;
	float a = randomFloat(seed) * 2.0 * PI;
	float r = sqrt(1.0 - z * z);
	float x = r * cos(a);
	float y = r * sin(a);

	// Convert unit vector in sphere to a cosine weighted vector in hemissphere
	return normalize(normal + vec3(x, y, z));
}

vec3 random_in_unit_disk(inout uint seed) {
	float r		= randomFloat(seed);
	float theta = randomFloat(seed) * 2 * PI;
	float x		= sqrt(r) * cos(theta);
	float y		= sqrt(r) * sin(theta);
	return vec3(x, y, 0.0);
}

float getSmallestPositive(float t1, float t2) {
	// Assumes at least one float > 0
	return t1 <= 0 ? t2 : t1;
}

void intersectBox(in Ray ray, in Box box, in uint objIdx, inout RayHit rec) {

	// Source:
	// https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525

	float t1 = -FLOAT_MAX;
	float t2 = FLOAT_MAX;

	vec3 t0s = (box.min - ray.origin) * (1.0 / ray.direction);
	vec3 t1s = (box.max - ray.origin) * (1.0 / ray.direction);

	vec3 tsmaller = min(t0s, t1s);
	vec3 tbigger  = max(t0s, t1s);

	t1 = max(t1, max(tsmaller.x, max(tsmaller.y, tsmaller.z)));
	t2 = min(t2, min(tbigger.x, min(tbigger.y, tbigger.z)));

	if (t1 <= t2) {
		if (t2 > phi && t2 < rec.dist) {
			rec.dist   = t2;
			rec.type   = 4;
			rec.objIdx = objIdx;
		}
		if (t1 > phi && t1 < rec.dist) {
			rec.dist   = t1;
			rec.type   = 4;
			rec.objIdx = objIdx;
		}
	}
}

void intersectSphere(in Ray r, in Sphere sphere, in uint objIdx, inout RayHit rec) {
	// source: https://antongerdelan.net/opengl/raycasting.html

	float t1 = FLOAT_MAX, t2 = FLOAT_MAX;
	vec3  sphereToRay = r.origin - sphere.position;
	float b			  = dot(r.direction, sphereToRay);
	float c			  = dot(sphereToRay, sphereToRay) - sphere.radius * sphere.radius;
	float disc		  = b * b - c;

	if (disc < 0.) return;

	float squareRoot = sqrt(disc);
	t1				 = -b - squareRoot;
	t2				 = -b + squareRoot;

	float t = getSmallestPositive(t1, t2);
	if (t > 0.001 && t < rec.dist) {
		rec.type   = 2;
		rec.dist   = t;
		rec.objIdx = objIdx;
	}
}

void intersectGroundPlane(in Ray ray, in float PlaneHeight, inout RayHit hit) {
	float t = (ray.origin.y - PlaneHeight) / -ray.direction.y;
	if (t > 0.001 && t < hit.dist) {
		vec3 pos = t * ray.direction + ray.origin;
		if (pos.x < 10 && pos.x > -10 && pos.z < 10 && pos.z > -10) {
			hit.type   = 1;
			hit.dist   = t;
			hit.objIdx = 0;		// none anyway
		}
	}
}

float intersect(in vec3 orig, in vec3 dir, in vec3 v0, in vec3 v1, in vec3 v2,
				out vec2 UV) {	   // graphicon.org/html/2012/conference/EN2%20-%20Graphics/gc2012Shumskiy.pdf
	vec3  e1	 = v1 - v0;
	vec3  e2	 = v2 - v0;
	vec3  normal = normalize(cross(e1, e2));
	float b		 = dot(normal, dir);
	vec3  w0	 = orig - v0;
	float a		 = -dot(normal, w0);
	float t		 = a / b;
	vec3  p		 = orig + t * dir;
	float uu, uv, vv, wu, wv, inverseD;
	uu		 = dot(e1, e1);
	uv		 = dot(e1, e2);
	vv		 = dot(e2, e2);
	vec3 w	 = p - v0;
	wu		 = dot(w, e1);
	wv		 = dot(w, e2);
	inverseD = uv * uv - uu * vv;
	inverseD = 1.0f / inverseD;
	float u	 = (uv * wv - vv * wu) * inverseD;
	if (u < 0.0 || u > 1.0) return -1.0f;
	float v = (uv * wu - uu * wv) * inverseD;
	if (v < 0.0 || (u + v) > 1.0) return -1.0f;

	UV = vec2(u, v);
	return t;
}

uniform Material geometry_material	 = Material(vec3(1.), .2, vec3(0.), 0.99, vec3(0.1), 0.0, vec3(1.), 0.0, 0.1, 0., 0,
												0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0);
uniform uint	 geometryMaterialIdx = 0;

bool intersectTriangle(in Ray ray, in vec3 v0, in vec3 v1, in vec3 v2, in uint objIdx, inout RayHit hit) {
	vec3 orig = ray.origin;
	vec3 dir  = ray.direction;
	vec2 uv;

	float t = intersect(orig, dir, v0, v1, v2, uv);

	if (t > phi && t < hit.dist) {
		hit.type = 3;
		hit.dist = t;
		hit.objIdx = objIdx;
		hit.uv = uv;

		// hit.normal = normalize((1. - uv.x - uv.y) * normal0 + uv.x * normal1 + uv.y * normal2);

		return true;
	}
	return false;
}

bool intersectAABB(in Ray ray, in vec3 _min, in vec3 _max) {
	// Source:
	// https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
	
	float t1 = -FLOAT_MAX;
	float t2 = FLOAT_MAX;

	vec3 t0s = (_min - ray.origin) * (1.0 / ray.direction);
	vec3 t1s = (_max - ray.origin) * (1.0 / ray.direction);

	vec3 tsmaller = min(t0s, t1s);
	vec3 tbigger  = max(t0s, t1s);

	t1 = max(t1, max(tsmaller.x, max(tsmaller.y, tsmaller.z)));
	t2 = min(t2, min(tbigger.x, min(tbigger.y, tbigger.z)));
	return t1 <= t2;
}

bool intersectAABB(in Ray ray, in AABB box) {
	return intersectAABB(ray, box.min, box.max);
}

bool intersectNode(in Ray ray, in BVHNode node) {
	return intersectAABB(ray, node.min, node.max);
}

bool intersectTriangle(in Ray ray, in FastTriangle t, in uint objIdx, inout RayHit hit) {
	return intersectTriangle(ray, t.v0, t.v1, t.v2, objIdx, hit);
}

float fresnelReflectAmount(float n1, float n2, vec3 normal, vec3 incident, float f0, float f90) {
	// Schlick aproximation
	float r0 = (n1 - n2) / (n1 + n2);
	r0 *= r0;
	float cosX = -dot(normal, incident);
	if (n1 > n2) {
		float n		= n1 / n2;
		float sinT2 = n * n * (1.0 - cosX * cosX);
		// Total internal reflection
		if (sinT2 > 1.0) return f90;
		cosX = sqrt(1.0 - sinT2);
	}
	float x	  = 1.0 - cosX;
	float ret = r0 + (1.0 - r0) * x * x * x * x * x;
	// ret = clamp(ret, 0.0, 1.0);

	// adjust reflect multiplier for object reflectivity
	return mix(f0, f90, ret);
}

vec3 get_sky_color(in Ray ray) {
	return textureLod(skybox, ray.direction, 2.).xyz;
}

const float PI	   = 3.14159265359;
const float TWO_PI = 2. * PI;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 color;
layout(location = 4) in vec3 tangent;

struct PointLight {
	vec3  position;
	vec3  color;
	float intensity;
};

struct DirectionalLight {
	vec3  direction;
	vec3  color;
	float intensity;
};

struct AmbientLight {
	vec3  color;
	float intensity;
};

struct Light {
	mat4  transform;
	vec3  color;
	float intensity;
	int	  type;
};

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

layout(std140, binding = 0) uniform Matrices {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 cameraWorldMatrix;
};

layout(std140, binding = 1) uniform Materials { Material materials[100]; };

layout(std140, binding = 2) uniform Lights {
	Light lights[100];
	uint  lightsCount;
};

uniform uint material_index = 0;
uniform float renderMode = 0;

uniform bool use_texture;
layout(binding = 1) uniform sampler2D albedoMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform sampler2D heightMap;
layout(binding = 4) uniform sampler2D roughnessMap;
layout(binding = 5) uniform sampler2D aoMap;
layout(binding = 6) uniform sampler2D emissionMap;
layout(binding = 10) uniform sampler2D metallicMap;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {	   // learnopengl
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a		 = roughness * roughness;
	float a2	 = a * a;
	float NdotH	 = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num	= a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom		= PI * denom * denom;

	return num / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num	= NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / max(denom, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2	= GeometrySchlickGGX(NdotV, roughness);
	float ggx1	= GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 calcLight(Light light, in vec3 position, in vec3 N, in vec2 texCoord, Material mat,
			   in vec3 albedo, in float roughness, in float metallic) {
	vec3 lightPosition = (light.transform * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	vec3 lightForward  = (light.transform * vec4(0.0, 0.0, 1.0, 0.0)).xyz;

	if (light.type == 0) return light.color * albedo * light.intensity;

	vec3 camPos = (cameraWorldMatrix * vec4(0, 0, 0, 1)).xyz;

	vec3 L = lightPosition - position;
	if (light.type == 1) L = -lightForward;

	float dist = length(L);
	L		   = normalize(L);

	vec3 V = normalize(camPos - position);

	vec3 H = normalize(V + L);

	float attennuation = 1. / (dist * dist);
	vec3  radiance	   = light.color * attennuation * light.intensity;

	vec3 F0 = vec3(0.04);
	F0		= mix(F0, albedo, metallic);
	vec3 F	= fresnelSchlick(max(dot(H, V), 0.0), F0);

	float NDF = DistributionGGX(N, H, roughness);
	float G	  = GeometrySmith(N, V, L, roughness);

	vec3  numerator	  = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	denominator		  = max(denominator, 0.0001);
	vec3 specular	  = numerator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	float NdotL = max(dot(N, L), 0.0);

	return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 calcAllLights(in vec3 position, in vec3 normal, in vec3 vertexNormal, in vec2 texCoord) {
	vec3  light		= vec3(0.0, 0.0, 0.0);
	vec3  diffuse	= texture(albedoMap, texCoord).xyz;
	float roughness = texture(roughnessMap, texCoord).y;
	vec3  emission	= texture(emissionMap, texCoord).xyz;
	float metallic	= texture(metallicMap, texCoord).z;
	float ao		= texture(aoMap, texCoord).x;

	Material mat = materials[material_index];

	vec3  calcAlbedo	= mix(1., ao, mat.use_ao_map) * mix(mat.albedo, diffuse, mat.use_albedo_map);
	float calcMetallic	= mix(mat.metallic, metallic, mat.use_metallic_map);
	vec3  calcEmission	= mix(mat.emission, mat.emission * emission, mat.use_emission_map);
	float calcRoughness = mix(mat.specular_roughness, mat.specular_roughness * roughness, mat.use_roughness_map) + 0.1;

	if(renderMode == 0) {
		for (int i = 0; i < lightsCount; i++) {
			light += calcLight(lights[i], position, normal, texCoord, mat, calcAlbedo, calcRoughness,
							calcMetallic);
		}
		light += calcEmission;
	}
	if(renderMode == 1) {
		light += calcRoughness;
	}
	if(renderMode == 2) {
		light += calcMetallic;
	}
	if(renderMode == 3) {
		light += calcAlbedo;
	}
	if(renderMode == 4) {
		light += ao;
	}
	if(renderMode == 5) {
		light += normal;
	}

	return light;
}

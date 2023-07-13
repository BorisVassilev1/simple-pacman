#version 460

in vec2 outTexCoord;

out vec4 fragColor;

layout(binding = 7) uniform sampler2D sampler_color;
layout(binding = 8) uniform sampler2D sampler_depth;
layout(binding = 9) uniform usampler2D sampler_stencil;

uniform bool doColorGrading	   = true;
uniform bool doGammaCorrection = true;

// ACES tone mapping curve fit to go from HDR to LDR
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x) {
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 LinearToInverseGamma(vec3 rgb, float gamma) {
	// rgb = clamp(rgb, 0.0, 1.0);
	return mix(pow(rgb, vec3(1.0 / gamma)) * 1.055 - 0.055, rgb * 12.92, vec3(lessThan(rgb, 0.0031308.xxx)));
}

void main() {
	vec4 color = texture(sampler_color, outTexCoord);

	if (doColorGrading) color.xyz = ACESFilm(color.xyz);
	if (doGammaCorrection) color.xyz = LinearToInverseGamma(color.xyz, 2.4);

	fragColor = color;
}
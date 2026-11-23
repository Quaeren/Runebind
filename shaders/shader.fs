#version 330 core
#define MAX_LIGHTS 16

struct Light {
	vec3 position;
	vec3 color;
	float radius;
	vec3 direction;
	float cutOff; float outerCutOff;
	int type;
};

uniform Light lights[MAX_LIGHTS];
uniform int lightCount;
uniform vec3 ambient;
uniform vec3 materialColor;
uniform bool useVertexColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 Color;
in vec2 TexCoord;

out vec4 FragColor;

float attenuation(float distance, float radius) {
	float x = clamp(1.0 - pow(distance / radius, 4.0), 0.0, 1.0);
	return (x * x) / (distance * distance + 1.0);
}

void main() {
	vec3 normal = normalize(Normal);
	vec3 albedo = useVertexColor ? Color : materialColor;
	vec3 result = ambient * albedo;

	for (int i = 0; i < lightCount; ++i) {
		vec3 lightPos = lights[i].position - FragPos;
		float distance = length(lightPos);
		if (distance >= lights[i].radius) { continue; }
		lightPos /= distance;

		float att = attenuation(distance, lights[i].radius);
		if (lights[i].type == 1) {
			float theta = dot(-lightPos, normalize(lights[i].direction));
			att *= clamp((theta - lights[i].cutOff) / (lights[i].cutOff - lights[i].outerCutOff), 0.0, 1.0);
		}
		vec3 fakeSun = normalize(vec3(0.3, 1.0, 0.2));
		result += albedo * 0.6 * max(dot(normal, fakeSun), 0.0);
		//result += albedo * lights[i].color * max(dot(normal, lightPos), 0.0) * att;
	}
	result = result / (result + 1.0);
	result = pow(result, vec3(1.0 / 2.2));
	FragColor = vec4(result, 1.0f);
}
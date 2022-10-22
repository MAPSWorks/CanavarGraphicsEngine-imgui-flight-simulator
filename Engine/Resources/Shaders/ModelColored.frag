#version 330 core

struct Sun
{
    vec3 direction;
    vec4 color;
    float ambient;
    float diffuse;
    float specular;
};

struct Model
{
    vec4 color;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};

struct Haze
{
    bool enabled;
    vec3 color;
    float density;
    float gradient;
};

struct PointLight
{
    vec4 color;
    vec3 position;
    float ambient;
    float diffuse;
    float specular;
    float constant;
    float linear;
    float quadratic;
};

uniform Haze haze;
uniform Sun sun;
uniform Model model;

uniform PointLight pointLights[8];
uniform int numberOfPointLights;

uniform vec3 cameraPos;

in vec4 fsPosition;
in vec3 fsNormal;

out vec4 outColor;

vec4 processSun(vec3 normal, vec3 viewDir)
{
    // Ambient
    float ambient = model.ambient * sun.ambient;

    // Diffuse
    float diffuse = max(dot(normal, sun.direction), 0.0) * model.diffuse * sun.diffuse;

    // Specular
    vec3 reflectDir = reflect(-sun.direction, normal);
    vec3 halfwayDir = normalize(sun.direction + viewDir);
    float specular = pow(max(dot(normal, halfwayDir), 0.0), model.shininess) * model.specular * sun.specular;

    return (ambient + diffuse + specular) * model.color * sun.color;
}


vec4 processPointLights(vec3 fragWorldPos, vec3 normal, vec3 viewDir)
{
    vec4 result = vec4(0);

    for (int i = 0; i < numberOfPointLights; i++)
    {
        // Ambient
        float ambient = pointLights[i].ambient * model.ambient;

        // Diffuse
        vec3 lightDir = normalize(pointLights[i].position - fragWorldPos);
        float diffuse =  max(dot(normal, lightDir), 0.0) * pointLights[i].diffuse * model.diffuse;

        // Specular
        vec3 reflectDir = reflect(-lightDir, normal);
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float specular = pow(max(dot(normal, halfwayDir), 0.0), model.shininess) * pointLights[i].specular * model.specular;

        // Attenuation
        float distance = length(pointLights[i].position - fragWorldPos);
        float attenuation = 1.0f / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * (distance * distance));

        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;

        result += (ambient + diffuse + specular) * pointLights[i].color;
    }

    return result;

}

vec4 processHaze(float distance, vec3 fragWorldPos, vec4 subjectColor)
{
    vec4 result = subjectColor;

    if(haze.enabled)
    {
        float factor = exp(-pow(distance * 0.00005f * haze.density, haze.gradient));
        factor = clamp(factor, 0.0f, 1.0f);
        result =  mix(vec4(haze.color, 1) * clamp(sun.direction.y, 0.0f, 1.0f), subjectColor, factor);
    }

    return result;
}

void main()
{
    // Common
    vec3 viewDir = normalize(cameraPos - fsPosition.xyz);
    float distance = length(cameraPos - fsPosition.xyz);

    vec4 result = vec4(0);
    result += processSun(fsNormal, viewDir);
    result += processPointLights(fsPosition.xyz, fsNormal, viewDir);
    outColor = processHaze(distance, fsPosition.xyz, result);
}

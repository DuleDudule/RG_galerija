#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct pointLight {
    vec3 position;
    vec3 color;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};
struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    float shininess;
};
struct SpotLight{
   float constant;
   float linear;
   float quadratic;

   float cutOff;
   float outerCutOff;

   vec3 position;
   vec3 direction;

   vec3 ambient;
   vec3 diffuse;
   vec3 specular;
};
const int NR_LIGHTS = 1;

uniform pointLight lights[NR_LIGHTS];
uniform Material material;
uniform SpotLight spotLight;
uniform DirLight dirLight;
uniform bool hdr;
uniform float exposure;

uniform vec3 viewPos;
uniform bool blinn;

// calculates the color when using a point light.
vec3 CalcPointLight(pointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,vec3 Diffuse,float Specular)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    }

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient * Diffuse;
    vec3 diffuse = light.diffuse * diff * Diffuse * light.color;
    vec3 specular = light.specular * spec * Specular;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir,vec3 Diffuse,float Specular)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    };

    // combine results
    vec3 ambient  = light.ambient  * Diffuse;
    vec3 diffuse  = light.diffuse  * diff * Diffuse;
    vec3 specular = light.specular * spec * Specular;
    return (ambient + diffuse + specular);
}


vec3 CalcSpotLight(SpotLight light, vec3 normal,vec3 viewDir,vec3 fragPos,vec3 Diffuse,float Specular){
    vec3 lightDir = normalize(light.position -fragPos);

    float diff = max(dot(normal, lightDir),0.0);
    //blinn?
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    }

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    float epsilon = light.cutOff - light.outerCutOff;
    float theta = dot(-lightDir,normalize(light.direction));
    float intensity = clamp((theta - light.outerCutOff) / epsilon,0.0,1.0);

    vec3 ambient = light.ambient * Diffuse;
    vec3 diffuse = light.diffuse * diff * Diffuse;
    vec3 specular = light.specular * spec * Specular;


    ambient *= attenuation;
    diffuse*= attenuation* intensity;
    specular *= attenuation * intensity;


    return (ambient + diffuse + specular);

}
void main()
{
    const float gamma = 2.2;
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0,0,0);
    result = CalcDirLight(dirLight, Normal, viewDir,Diffuse,Specular);
    for(int i = 0; i < NR_LIGHTS; ++i){
        result +=CalcPointLight(lights[i], Normal, FragPos, viewDir,Diffuse,Specular);
    }
    result += CalcSpotLight(spotLight,Normal,viewDir,FragPos,Diffuse,Specular);


    //tonemapping?
    if(hdr)
    {
        // reinhard
        // vec3 result = hdrColor / (hdrColor + vec3(1.0));
        // exposure
        result = vec3(1.0) - exp(-result * exposure);
        // also gamma correct while we're at it
        result = pow(result, vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
    }
    else
    {
        result = pow(result, vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
    }


//     // then calculate lighting as usual
//     vec3 lighting  = Diffuse * 0.1; // hard-coded ambient component
//     vec3 viewDir  = normalize(viewPos - FragPos);
//     for(int i = 0; i < NR_LIGHTS; ++i)
//     {
//         // diffuse
//         vec3 lightDir = normalize(lights[i].position - FragPos);
//         vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].color;
//         // specular
//         vec3 halfwayDir = normalize(lightDir + viewDir);
//         float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
//         vec3 specular = lights[i].color * spec * Specular;
//         // attenuation
//         float distance = length(lights[i].position - FragPos);
//         float attenuation = 1.0 / (1.0 + lights[i].linear * distance + lights[i].quadratic * distance * distance);
//         diffuse *= attenuation;
//         specular *= attenuation;
//         lighting += diffuse + specular;
//     }

}
#version 460 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
layout (location = 2) out vec4 Depth;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gMask;
float near = 0.1;
float far  = 100.0;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}
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
const int NR_LIGHTS_SIPKE = 96;
const int NR_LIGHTS_RAMOVI = 6;
const int NR_SPOT_LIGHTS = 26;

uniform pointLight lightsSipke[NR_LIGHTS_SIPKE];
uniform pointLight lightsRamovi[NR_LIGHTS_RAMOVI];
uniform Material material;
uniform SpotLight spotLight[NR_SPOT_LIGHTS];
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
//     vec3 viewDir = normalize(viewPos - FragPos);
    vec3 viewDir = normalize(FragPos - viewPos);
    vec3 result = vec3(0,0,0);
    result = CalcDirLight(dirLight, Normal, viewDir,Diffuse,Specular);

    vec3 maska = texture(gMask,TexCoords).rgb;
    if(maska == vec3(1.0,1.0,1.0)){
        for(int i = 0; i < NR_LIGHTS_SIPKE; ++i){
                result +=CalcPointLight(lightsSipke[i], Normal, FragPos, viewDir,Diffuse,Specular);
            }
                float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
                if(brightness > 1.0)
                    BrightColor = vec4(result, 1.0);
                else
                    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);


                FragColor = vec4(result, 1.0);
                float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
                Depth = vec4(vec3(depth), 1.0);
    }
    else{
        for(int i = 0; i < NR_LIGHTS_RAMOVI; ++i){
            result +=CalcPointLight(lightsRamovi[i], Normal, FragPos, viewDir,Diffuse,Specular);
        }
        for(int i= 0; i < NR_SPOT_LIGHTS; i++){
            result += CalcSpotLight(spotLight[i],Normal,viewDir,FragPos,Diffuse,Specular);
        }



            FragColor = vec4(result, 1.0);
            float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
            Depth = vec4(vec3(depth), 1.0);

    }

}
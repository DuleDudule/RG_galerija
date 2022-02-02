#version 460 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Material{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};
struct PointLight{
    float constant;
    float linear;
    float quadratic;

    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct DirLight{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
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
#define NR_POINT_LIGHTS 96
#define NR_SPOT_LIGHTS 26
uniform sampler2D tekstura;
uniform Material material;
uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight[NR_SPOT_LIGHTS];
float near = 0.1;
float far  = 100.0;
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}
vec3 CalcDirLight(DirLight light, vec3 normal,vec3 viewDir){
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir),0.0);

    vec3 reflectDir = reflect(-lightDir,normal);

    float spec = pow(max(dot(viewDir,reflectDir),0.0),material.shininess);

    vec3 ambient = light.ambient * vec3(texture(tekstura,TexCoords))*texture(tekstura, TexCoords).a;
    vec3 diffuse = light.diffuse * diff * vec3(texture(tekstura,TexCoords))*texture(tekstura, TexCoords).a;
    vec3 specular = light.specular * spec * vec3(texture(tekstura,TexCoords))*texture(tekstura, TexCoords).a;
    return (ambient + diffuse + specular);

}
vec3 CalcPointLight(PointLight light, vec3 normal,vec3 viewDir,vec3 fragPos){
    vec3 lightDir = normalize(light.position -fragPos);

    float diff = max(dot(normal, lightDir),0.0);

    vec3 reflectDir = reflect(-lightDir,normal);

    float spec = pow(max(dot(viewDir,reflectDir),0.0),material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light. quadratic * distance * distance);

    vec3 ambient = light.ambient * vec3(texture(tekstura,TexCoords))*texture(tekstura, TexCoords).a;
    vec3 diffuse = light.diffuse * diff * vec3(texture(tekstura,TexCoords))*texture(tekstura, TexCoords).a;
    vec3 specular = light.specular * spec * vec3(texture(tekstura,TexCoords))*texture(tekstura, TexCoords).a;

    ambient *= attenuation;
    diffuse*= attenuation;
    specular *= attenuation;


    return (ambient + diffuse + specular);

}
vec3 CalcSpotLight(SpotLight light, vec3 normal,vec3 viewDir,vec3 fragPos){
    vec3 lightDir = normalize(light.position -fragPos);

    float diff = max(dot(normal, lightDir),0.0);

    vec3 reflectDir = reflect(-lightDir,normal);

    float spec = pow(max(dot(viewDir,reflectDir),0.0),material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    float epsilon = light.cutOff - light.outerCutOff;
    float theta = dot(-lightDir,normalize(light.direction));
    float intensity = clamp((theta - light.outerCutOff) / epsilon,0.0,1.0);

    vec3 ambient = light.ambient * vec3(texture(tekstura,TexCoords))*texture(tekstura, TexCoords).a;
    vec3 diffuse = light.diffuse * diff * vec3(texture(tekstura,TexCoords))*texture(tekstura, TexCoords).a;
    vec3 specular = light.specular * spec * vec3(texture(tekstura,TexCoords))*texture(tekstura, TexCoords).a;


    ambient *= attenuation;
    diffuse*= attenuation* intensity;
    specular *= attenuation * intensity;


    return (ambient + diffuse + specular);

}


void main(){
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = CalcDirLight(dirLight,norm,viewDir);

    for(int i= 0; i < NR_POINT_LIGHTS; i++){
        result += CalcPointLight(pointLights[i],norm,viewDir,FragPos);
    }
    for(int i= 0; i < NR_SPOT_LIGHTS; i++){
        result += CalcSpotLight(spotLight[i],norm,viewDir,FragPos);
    }

    FragColor = vec4(result,texture(tekstura, TexCoords).a);
//        FragColor = vec4 (1.0,1.0,1.0,1.0);

}
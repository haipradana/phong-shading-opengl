#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main() {
    // Ambient lighting (ditingkatkan)
    float ambientStrength = 0.3; // Ditingkatkan dari 0.1
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting (dikurangi)
    float specularStrength = 0.3; // Dikurangi dari 0.5
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16); // Dikurangi dari 32
    vec3 specular = specularStrength * spec * lightColor;

    // Base color lebih terang
    vec3 objectColor = vec3(0.9, 0.9, 0.9); // Diubah dari 0.8

    // Combine all components dengan minimal lighting
    vec3 result = (ambient + diffuse + specular) * objectColor;
    result = max(result, ambient * objectColor); // Memastikan ada minimal lighting
    FragColor = vec4(result, 1.0);
}
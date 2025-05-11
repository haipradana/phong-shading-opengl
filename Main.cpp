#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>

// Define these before including tiny_obj_loader.h
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// Shader class tetap sama seperti sebelumnya
class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // Pastikan ifstream objects dapat throw exceptions
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            // Buka files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            // Baca file's buffer contents ke streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            // Tutup file handlers
            vShaderFile.close();
            fShaderFile.close();

            // Convert stream ke string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
            std::cout << "Vertex path: " << vertexPath << std::endl;
            std::cout << "Fragment path: " << fragmentPath << std::endl;
            return;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // Compile shaders
        unsigned int vertex, fragment;
        int success;
        char infoLog[512];

        // Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);

        // Print compile errors if any
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
            std::cout << "Vertex Shader Code:\n" << vShaderCode << std::endl;
            return;
        }

        // Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);

        // Print compile errors if any
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
            std::cout << "Fragment Shader Code:\n" << fShaderCode << std::endl;
            return;
        }

        // Shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        // Print linking errors if any
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            return;
        }

        // Delete shaders as they're linked into our program and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }


    void use() {
        glUseProgram(ID);
    }

    void setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
};

// Camera class
class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    float Yaw;
    float Pitch;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f)) {
        Position = position;
        Front = glm::vec3(0.0f, 0.0f, -1.0f);
        Up = glm::vec3(0.0f, 1.0f, 0.0f);
        Yaw = -90.0f;
        Pitch = 0.0f;
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }
};

// Model class yang baru menggunakan tinyobjloader
class Model {
public:
    unsigned int VAO, VBO;
    int vertexCount;
    std::string modelPath;

    Model(const char* path) : modelPath(path) {
        LoadModel();
    }

    void Draw() {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }

    ~Model() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

private:
    void LoadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str());

        if (!warn.empty()) {
            std::cout << "Warning when loading " << modelPath << ": " << warn << std::endl;
        }

        if (!err.empty()) {
            std::cout << "Error when loading " << modelPath << ": " << err << std::endl;
        }

        if (!ret) {
            std::cout << "Failed to load/parse .obj file: " << modelPath << std::endl;
            return;
        }

        std::vector<float> vertices;
        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++) {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                int fv = shapes[s].mesh.num_face_vertices[f];

                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++) {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    // vertex position
                    vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
                    vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
                    vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);

                    // normal
                    if (idx.normal_index >= 0) {
                        vertices.push_back(attrib.normals[3 * idx.normal_index + 0]);
                        vertices.push_back(attrib.normals[3 * idx.normal_index + 1]);
                        vertices.push_back(attrib.normals[3 * idx.normal_index + 2]);
                    }
                    else {
                        // default normal if not available
                        vertices.push_back(0.0f);
                        vertices.push_back(1.0f);
                        vertices.push_back(0.0f);
                    }
                }
                index_offset += fv;
            }
        }

        vertexCount = vertices.size() / 6; // 3 for pos, 3 for normal

        if (vertexCount == 0) {
            std::cout << "No vertices loaded from: " << modelPath << std::endl;
            return;
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        // vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);

        std::cout << "Successfully loaded " << modelPath << " with " << vertexCount << " vertices" << std::endl;
    }
};

// Global variables tetap sama
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 800.0f / 2.0f;
float lastY = 600.0f / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool isPerspective = true;

// Callback functions tetap sama
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera.Yaw += xoffset;
    camera.Pitch += yoffset;

    if (camera.Pitch > 89.0f)
        camera.Pitch = 89.0f;
    if (camera.Pitch < -89.0f)
        camera.Pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(camera.Yaw)) * cos(glm::radians(camera.Pitch));
    front.y = sin(glm::radians(camera.Pitch));
    front.z = sin(glm::radians(camera.Yaw)) * cos(glm::radians(camera.Pitch));
    camera.Front = glm::normalize(front);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.Position += cameraSpeed * camera.Front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.Position -= cameraSpeed * camera.Front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.Position -= glm::normalize(glm::cross(camera.Front, camera.Up)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.Position += glm::normalize(glm::cross(camera.Front, camera.Up)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        isPerspective = true;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        isPerspective = false;
}

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL 3D Scene", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Create shader
    Shader shader("vertex.vs", "fragment.fs");

    // Load models
    //Model cube("models/cube.obj");
    //Model model1("models/model1.obj");
    //Model model2("models/model2.obj");
    //Model model3("models/model3.obj");

    Model cube("kubus.obj");
    Model model1("teko_3D.obj");
    Model model2("cone.obj");
    Model model3("Donut.obj");

    // Check if models loaded successfully
    if (cube.vertexCount == 0 || model1.vertexCount == 0 ||
        model2.vertexCount == 0 || model3.vertexCount == 0) {
        std::cout << "Failed to load one or more models!" << std::endl;
        return -1;
    }

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Di dalam render loop, setelah shader.use()

// Set lighting properties
glm::vec3 lightPos(5.0f, 5.0f, 5.0f); // Posisi lampu diubah
shader.setVec3("lightPos", lightPos);
shader.setVec3("viewPos", camera.Position);
shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

// View/Projection transformations
glm::mat4 view = camera.GetViewMatrix();
glm::mat4 projection;
if (isPerspective) {
    projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
} else {
    projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
}
shader.setMat4("view", view);
shader.setMat4("projection", projection);

// Render kubus
glm::mat4 model = glm::mat4(1.0f);
model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0f)); // Lebih jauh ke kiri
model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(1.0f, 0.3f, 0.5f));
model = glm::scale(model, glm::vec3(0.8f)); // Ukuran dikecilkan
shader.setMat4("model", model);
cube.Draw();

// Render teko
model = glm::mat4(1.0f);
model = glm::translate(model, glm::vec3(3.0f, 0.0f, 0.0f)); // Lebih jauh ke kanan
model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.3f));
model = glm::scale(model, glm::vec3(0.007f)); // Sesuaikan scale untuk teko
shader.setMat4("model", model);
model1.Draw();

// Render cone
model = glm::mat4(1.0f);
model = glm::translate(model, glm::vec3(0.0f, 3.0f, 0.0f)); // Lebih tinggi
model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.3f, 0.5f, 1.0f));
model = glm::scale(model, glm::vec3(0.02f)); // Sesuaikan scale untuk cone
shader.setMat4("model", model);
model2.Draw();

// Render donut
model = glm::mat4(1.0f);
model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f)); // Lebih rendah
model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(1.0f, 1.0f, 0.0f));
model = glm::scale(model, glm::vec3(0.4f)); // Sesuaikan scale untuk donut
shader.setMat4("model", model);
model3.Draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
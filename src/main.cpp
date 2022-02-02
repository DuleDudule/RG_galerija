#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void renderQuad();


unsigned int loadTexture(char const * path, bool gammaCorrection);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool freeCamKeyPressed = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;

    float cutOff;
    float outerCutOff;
};
struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool hdr = true;
    bool bloom = true;
    bool blinn = true;


    float exposure = 0.422f;
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 frameLights = glm::vec3(4.0f);
    glm::vec3 dirLightAmbient = glm::vec3(0.05f,0.05f,0.05f);
    glm::vec3 dirLightDiffuse = glm::vec3(0.4f,0.4f,0.4f);
    glm::vec3 dirLightSpecular = glm::vec3(0.5f,0.5f,0.5f);
    glm::vec3 spotDir = glm::vec3(0.0f,1.0f,0.2f);
    float planeScaleX = 10.603f;
    float planeScaleY = 1.0f;
    float planeScaleZ = 1.835f;
    float planePosX = 8.987f;
    float planePosY = -3.228f;
    float planePosZ = -10.663f;
    glm::vec3 planePos = glm::vec3(5,planeScaleY,planePosZ);

    PointLight pointLight;
    SpotLight spotLight;
    ProgramState()
                :camera(glm::vec3(-26.5f,3.5f,-11.0f)){}
//            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
            << pointLight.linear << '\n'
            << pointLight.constant << '\n'
            << pointLight.quadratic << '\n'
            << exposure << '\n'
            << spotDir.x << '\n'
            << spotDir.y << '\n'
            << spotDir.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> pointLight.linear
           >> pointLight.quadratic
           >> pointLight.constant
           >>exposure
           >>spotDir.x
           >>spotDir.y
           >>spotDir.z;
    }
}

ProgramState *programState;


int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_CULL_FACE);

    // build and compile shaders
    // -------------------------
    Shader shaderGeometryPass("resources/shaders/8.1.g_buffer.vs", "resources/shaders/8.1.g_buffer.fs");
    Shader shaderGeometryPass2("resources/shaders/gBuffer2.vs", "resources/shaders/gBuffer2.fs");
    Shader shaderLightingPass("resources/shaders/8.1.deferred_shading.vs", "resources/shaders/8.1.deferred_shading.fs");
    Shader shaderBloomFinal("resources/shaders/7.bloom_final.vs", "resources/shaders/7.bloom_final.fs");
    Shader shaderBlur("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader transparentShader("resources/shaders/transparent.vs", "resources/shaders/transparent.fs");

    // load models
    // -----------
    Model tunel2("resources/objects/final/sipke2.obj");
    tunel2.SetShaderTextureNamePrefix("");
    Model ramovi2("resources/objects/final/ramovi2.obj");
    ramovi2.SetShaderTextureNamePrefix("");

    unsigned int transparentTexture = loadTexture("resources/textures/plocice3.png",false);


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 0.050f;
    pointLight.linear = 0.0f;
    pointLight.quadratic = 0.025f;

    SpotLight& spotLight = programState->spotLight;
    spotLight.direction = programState->spotDir;
    spotLight.position = glm::vec3(4.0f, 4.0, 0.0);
    spotLight.ambient = glm::vec3(0.0, 0.0, 0.0);
    spotLight.diffuse = glm::vec3(6.0, 6.0, 6.0);
    spotLight.specular = glm::vec3(1.0, 1.0, 1.0);

    spotLight.constant = 1.0f;
    spotLight.linear = 0.09f;
    spotLight.quadratic = 0.032f;

    spotLight.cutOff =glm::cos(glm::radians(6.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(8.0f));


    std::vector<glm::vec3> spotLightPositions ={glm::vec3(-13.129006,0.482098,-5.613296),
                                                glm::vec3(13.285252,0.231531,-16.278114),
                                                glm::vec3(10.160604,0.202667,-5.588177),
                                                glm::vec3(6.306561,0.432728,-16.453705),
                                                glm::vec3(2.221441,0.128679,-5.548817),
                                                glm::vec3(-2.007963,0.199252,-16.736084),
                                                glm::vec3(-5.503589,0.329211,-5.863959),
                                                glm::vec3(-9.701194,0.161961,-16.862968),
                                                glm::vec3(17.875425,0.042483,-5.448197),
                                                glm::vec3(21.437689,0.230818,-16.417316),
                                                glm::vec3(25.497709,0.187957,-5.209609),
                                                glm::vec3(28.816259,0.292503,-16.135221),
                                                glm::vec3(32.972527,0.241437,-5.065379),


                                                glm::vec3(-13.129006,0.482098,-5.613296),
                                                glm::vec3(13.285252,0.231531,-16.278114),
                                                glm::vec3(10.160604,0.202667,-5.588177),
                                                glm::vec3(6.306561,0.432728,-16.453705),
                                                glm::vec3(2.221441,0.128679,-5.548817),
                                                glm::vec3(-2.007963,0.199252,-16.736084),
                                                glm::vec3(-5.503589,0.329211,-5.863959),
                                                glm::vec3(-9.701194,0.161961,-16.862968),
                                                glm::vec3(17.875425,0.042483,-5.448197),
                                                glm::vec3(21.437689,0.230818,-16.417316),
                                                glm::vec3(25.497709,0.187957,-5.209609),
                                                glm::vec3(28.816259,0.292503,-16.135221),
                                                glm::vec3(32.972527,0.241437,-5.065379),//
    };
    std::vector<glm::vec3> spotLightDirections ={glm::vec3(-0.001684,0.262190,-0.965015),
                                                glm::vec3(0.010140,0.253758,0.967215),
                                                 glm::vec3(-0.005019,0.287361,-0.957809),
                                                 glm::vec3(-0.013537,0.241922,0.970201),
                                                 glm::vec3(-0.003342,0.289032,-0.957314),
                                                 glm::vec3(0.008452,0.246999,0.968979),
                                                 glm::vec3(0.001681,0.268920,-0.963161),
                                                 glm::vec3(0.001691,0.246999,0.969014),
                                                 glm::vec3(-0.010071,0.273960,-0.961689),
                                                 glm::vec3(0.059237,0.241922,0.968486),
                                                 glm::vec3(-0.001676,0.278992,-0.960292),
                                                 glm::vec3(-0.000001,0.260505,0.965473),
                                                 glm::vec3(0.001687,0.255446,-0.966822),

                                                 glm::vec3(2.44131e-06,-0.322265,-0.94665),
                                                 glm::vec3(0.0117649,-0.273959,0.961669),
                                                 glm::vec3(-0.00998139,-0.30237,-0.953138),
                                                 glm::vec3(-0.0100035,-0.299041,0.954188),
                                                 glm::vec3(0.00166725,-0.295708,-0.955277),
                                                 glm::vec3(0.00673757,-0.263873,0.964534),
                                                 glm::vec3(-4.15955e-08,-0.307357,-0.951594),
                                                 glm::vec3(0.00169211,-0.25207,0.967708),
                                                 glm::vec3(-0.0100858,-0.26892,-0.96311),
                                                 glm::vec3(0.0604872,-0.268921,0.961261),
                                                 glm::vec3(-0.00333986,-0.290703,-0.956808),
                                                 glm::vec3(-1.74803e-06,-0.299041,0.95424),
                                                 glm::vec3(0.00166904,-0.292372,-0.956303),//
    };

    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            -3.5f, -3.5f, -3.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            3.5f, -3.5f, -3.5f,  0.0f,  0.0f, -1.0f,  3.0f,  0.0f,
            3.5f,  3.5f, -3.5f,  0.0f,  0.0f, -1.0f,  3.0f,  3.0f,
            3.5f,  3.5f, -3.5f,  0.0f,  0.0f, -1.0f,  3.0f,  3.0f,
            -3.5f,  3.5f, -3.5f,  0.0f,  0.0f, -1.0f,  0.0f,  3.0f,
            -3.5f, -3.5f, -3.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

    };

    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
//    glBindVertexArray(0);

    //BAFFERI
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedoSpec, gDepth,gMask;
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
// normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
    //vizualizacija depth buffera
    glGenTextures(1, &gDepth);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gDepth, 0);

    //maska
    glGenTextures(1, &gMask);
    glBindTexture(GL_TEXTURE_2D, gMask);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gMask, 0);

    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 , GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(5, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[3];
    glGenTextures(3, colorBuffers);
    for (unsigned int i = 0; i < 3; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    unsigned int attachments2[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments2);

    unsigned int rboDepth2;
    glGenRenderbuffers(1, &rboDepth2);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth2);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth2);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    unsigned int pingpongDepthbuffers[2];
//    unsigned int rboDepth3[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    glGenTextures(2, pingpongDepthbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);


        glBindTexture(GL_TEXTURE_2D, pingpongDepthbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0,GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT  , GL_TEXTURE_2D, pingpongDepthbuffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }
    unsigned int hdrFBO1;
    glGenFramebuffers(1, &hdrFBO1);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO1);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers1;
    glGenTextures(1, &colorBuffers1);

        glBindTexture(GL_TEXTURE_2D, colorBuffers1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, colorBuffers1, 0);

    unsigned int attachments3[1] = { GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, attachments3);
    unsigned int rboDepth7;
    glGenRenderbuffers(1, &rboDepth7);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth7);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth7);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // lighting info
    // -------------
    const unsigned int NR_LIGHTS = 96;

    std::vector<glm::vec3> lightPositions ={glm::vec3(-18.219347,0.019841,-4.418480),
                                             glm::vec3(-18.219347,8.081969,-14.856816),
                                             glm::vec3(-18.219347,5.026847,-3.111567),
                                             glm::vec3(-18.219347,0.138651,-16.961456),
                                             glm::vec3(-18.219347,-7.838613,-14.839844),
                                             glm::vec3(-18.219347,-4.851382,-3.128541),
                                             glm::vec3(-14.826115,-4.925699,-3.381288),
                                             glm::vec3(-15.412453,-7.346662,-15.235477),
                                             glm::vec3(-15.503129,0.312667,-16.755184),
                                             glm::vec3(-15.503129,7.938774,-14.941512),
                                             glm::vec3(-14.993274,5.201172,-3.558238),
                                             glm::vec3(-15.031126,-0.070016,-4.551630),
                                             glm::vec3(-10.883092,-0.048775,-4.268446),
                                             glm::vec3(-10.784439,5.977470,-3.611219),
                                             glm::vec3(-10.324480,7.508895,-16.345179),
                                             glm::vec3(-10.364365,0.471079,-17.376165),
                                             glm::vec3(-10.309916,-7.370855,-16.077499),
                                             glm::vec3(-10.463161,-5.634110,-4.613414),
                                             glm::vec3(-6.949615,-5.789815,-4.839404),
                                             glm::vec3(-6.894283,0.058534,-4.513902),
                                             glm::vec3(-6.947968,6.773547,-4.601629),
                                             glm::vec3(-7.070244,7.238101,-16.018749),
                                             glm::vec3(-6.912966,0.406054,-17.335619),
                                             glm::vec3(-6.851453,-6.672943,-16.518084),
                                             glm::vec3(-3.111208,-6.488462,-16.959276),
                                             glm::vec3(-2.984980,0.545574,-17.353388),
                                             glm::vec3(-3.033709,6.923474,-17.769590),
                                             glm::vec3(-2.877670,6.834191,-4.944912),
                                             glm::vec3(-2.880383,-0.342947,-4.614349),
                                             glm::vec3(-3.063288,-6.884153,-4.756081),
                                             glm::vec3(0.725805,-6.791443,-5.457982),
                                             glm::vec3(0.546045,-6.121334,-17.347038),
                                             glm::vec3(0.566536,-0.001579,-17.141983),
                                             glm::vec3(0.598213,6.061696,-17.853161),
                                             glm::vec3(0.378008,7.194937,-5.114062),
                                             glm::vec3(0.516151,-0.212209,-4.037829),
                                             glm::vec3(4.202213,-0.294362,-4.389437),
                                             glm::vec3(4.289669,7.443382,-5.926639),
                                             glm::vec3(4.379492,5.825613,-17.974213),
                                             glm::vec3(4.422613,0.281853,-17.572206),
                                             glm::vec3(4.320147,-5.606761,-18.596729),
                                             glm::vec3(4.286295,-7.411984,-5.690137),
                                             glm::vec3(7.954132,-7.162360,-6.546725),
                                             glm::vec3(7.968596,-4.768166,-18.648403),
                                             glm::vec3(8.013432,-0.176811,-17.096205),
                                             glm::vec3(7.909918,5.422708,-18.605049),
                                             glm::vec3(7.692652,8.263905,-6.191995),
                                             glm::vec3(7.806843,-0.453552,-4.305387),
                                             glm::vec3(11.770861,-0.338058,-4.616235),
                                             glm::vec3(11.418758,7.884514,-6.651344),
                                             glm::vec3(11.681646,5.651884,-18.619614),
                                             glm::vec3(11.733187,-0.046442,-17.057770),
                                             glm::vec3(11.732687,-5.511294,-18.608459),
                                             glm::vec3(11.733997,-7.463484,-6.409688),
                                             glm::vec3(15.248487,-7.090653,-6.057655),
                                             glm::vec3(15.021112,-5.452492,-17.685188),
                                             glm::vec3(15.162653,-0.187076,-17.475836),
                                             glm::vec3(15.123453,6.161551,-18.018398),
                                             glm::vec3(15.204020,7.880557,-5.743414),
                                             glm::vec3(15.148302,-0.375172,-4.383356),
                                             glm::vec3(18.889162,-0.464615,-4.456898),
                                             glm::vec3(18.783464,-6.892170,-4.545051),
                                             glm::vec3(18.780680,-6.413173,-17.400761),
                                             glm::vec3(18.865232,-0.502893,-16.631338),
                                             glm::vec3(18.742838,6.178942,-17.350288),
                                             glm::vec3(18.600946,6.863485,-5.105618),
                                             glm::vec3(22.047590,6.540334,-4.818344),
                                             glm::vec3(22.353266,6.765969,-16.498960),
                                             glm::vec3(22.361963,-0.428131,-17.271051),
                                             glm::vec3(22.145308,-6.749078,-17.128759),
                                             glm::vec3(22.234533,-6.221894,-4.770278),
                                             glm::vec3(22.293509,0.274449,-4.481799),
                                             glm::vec3(25.780043,0.199960,-4.522364),
                                             glm::vec3(25.868715,-6.534802,-4.282954),
                                             glm::vec3(25.700392,-6.136224,-16.976793),
                                             glm::vec3(25.777571,-0.434856,-17.000488),
                                             glm::vec3(25.628008,6.709528,-16.814663),
                                             glm::vec3(25.684889,6.661357,-4.533993),
                                             glm::vec3(29.851908,6.541843,-4.229498),
                                             glm::vec3(29.528418,7.297765,-16.010218),
                                             glm::vec3(29.465765,-0.647562,-17.097174),
                                             glm::vec3(29.450788,-7.670951,-16.053154),
                                             glm::vec3(29.254168,-6.172198,-3.774960),
                                             glm::vec3(29.266287,-0.207689,-4.282393),
                                             glm::vec3(32.998901,-0.227530,-4.541747),
                                             glm::vec3(33.211159,-5.482950,-3.370150),
                                             glm::vec3(33.037880,-7.636845,-15.396164),
                                             glm::vec3(33.154816,-0.132163,-17.129303),
                                             glm::vec3(32.792183,7.605859,-14.887218),
                                             glm::vec3(33.013840,5.430369,-2.816965),
                                             glm::vec3(37.315979,5.549945,-2.907359),
                                             glm::vec3(37.237907,8.279658,-14.437741),
                                             glm::vec3(37.493950,-0.399421,-16.906458),
                                             glm::vec3(37.439026,-7.829729,-14.860568),
                                             glm::vec3(36.895416,-5.349567,-2.314456),
                                             glm::vec3(37.014977,0.177408,-3.963766),
                                            };
    std::vector<glm::vec3> lightColors;
    srand(13);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        float rColor = 0.0f; // between 0.5 and 1.0
        float gColor = 0.0f; // between 0.5 and 1.0
        float bColor = 0.0f; // between 0.5 and 1.0
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }
    std::vector<glm::vec3> lightPositions2={glm::vec3(28.796471,2.201056,-11.214249),
                                            glm::vec3(-9.926498,2.308139,-11.444411),
                                            glm::vec3(-1.413528,2.552329,-11.210718),
                                            glm::vec3(5.815273,2.439733,-11.318583),
                                            glm::vec3(12.929670,2.194789,-11.288651),
                                            glm::vec3(20.748600,2.234152,-11.268876),
    };




    // shader configuration
    // --------------------
    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedoSpec", 2);
    shaderLightingPass.setInt("gMask", 3);

    shaderBloomFinal.use();
    shaderBloomFinal.setInt("scene", 0);
    shaderBloomFinal.setInt("bloomBlur", 1);
    shaderBloomFinal.setInt("maska", 2);
    shaderBlur.use();
    shaderBlur.setInt("image", 0);
    transparentShader.use();
    transparentShader.setInt("tekstura", 0);



    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        //postavljanje boje svetla
        float redValue = (sin(currentFrame + (2.0f*3.14f)/3.0f)/2.0f) +0.5f;
        float greenValue = ((sin(currentFrame + (4.0f*3.14f))) / 2.0f) +0.5f;
        float blueValue = ((sin(currentFrame + (4.0f*3.14f)/3.0f)) / 2.0f) +0.5f;
        for(int i = 0 ; i< lightColors.size(); i++){
            lightColors[i] =(glm::vec3(redValue,greenValue,blueValue));
        }

        // render
        // ------

        // 1. geometry pass: render scene's geometry/color data into gbuffer
        // -----------------------------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        shaderGeometryPass.use();
        shaderGeometryPass.setMat4("projection", projection);
        shaderGeometryPass.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0,0,0));
        shaderGeometryPass.setMat4("model", model);

        glDepthFunc(GL_LEQUAL);
        tunel2.Draw(shaderGeometryPass);

        shaderGeometryPass2.use();
        shaderGeometryPass2.setMat4("projection", projection);
        shaderGeometryPass2.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(glm::vec3(0.0f)));
        shaderGeometryPass.setMat4("model", model);
        ramovi2.Draw(shaderGeometryPass2);
        glDepthFunc(GL_LESS);


        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderLightingPass.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gMask);

        // send light relevant uniforms
        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            shaderLightingPass.setVec3("lightsSipke[" + std::to_string(i) + "].position", lightPositions[i]);
            shaderLightingPass.setVec3("lightsSipke[" + std::to_string(i) + "].color", lightColors[i]);

            shaderLightingPass.setVec3("lightsSipke[" + std::to_string(i) + "].ambient", pointLight.ambient);
            shaderLightingPass.setVec3("lightsSipke[" + std::to_string(i) + "].diffuse", pointLight.diffuse);
            shaderLightingPass.setVec3("lightsSipke[" + std::to_string(i) + "].specular", pointLight.specular);
            shaderLightingPass.setFloat("lightsSipke[" + std::to_string(i) + "].constant", pointLight.constant);
            shaderLightingPass.setFloat("lightsSipke[" + std::to_string(i) + "].linear", pointLight.linear);
            shaderLightingPass.setFloat("lightsSipke[" + std::to_string(i) + "].quadratic", pointLight.quadratic);
        }
        for (unsigned int i = 0; i < lightPositions2.size(); i++)
        {
            shaderLightingPass.setVec3("lightsRamovi[" + std::to_string(i) + "].position", lightPositions2[i]);
            shaderLightingPass.setVec3("lightsRamovi[" + std::to_string(i) + "].color", glm::vec3(programState->frameLights));

            shaderLightingPass.setVec3("lightsRamovi[" + std::to_string(i) + "].ambient", pointLight.ambient);
            shaderLightingPass.setVec3("lightsRamovi[" + std::to_string(i) + "].diffuse", pointLight.diffuse);
            shaderLightingPass.setVec3("lightsRamovi[" + std::to_string(i) + "].specular", pointLight.specular);
            shaderLightingPass.setFloat("lightsRamovi[" + std::to_string(i) + "].constant", pointLight.constant);
            shaderLightingPass.setFloat("lightsRamovi[" + std::to_string(i) + "].linear", pointLight.linear);
            shaderLightingPass.setFloat("lightsRamovi[" + std::to_string(i) + "].quadratic", 0.1f);
        }
        for (unsigned int i = 0; i < spotLightPositions.size(); i++){
            //spotlight
            shaderLightingPass.setVec3("spotLight[" + std::to_string(i) + "].position", spotLightPositions[i]);

            shaderLightingPass.setVec3("spotLight[" + std::to_string(i) + "].direction", spotLightDirections[i]);
//            shaderLightingPass.setVec3("spotLight[" + std::to_string(i) + "].direction", programState->camera.Front);
            shaderLightingPass.setVec3("spotLight[" + std::to_string(i) + "].ambient", programState->spotLight.ambient);
            shaderLightingPass.setVec3("spotLight[" + std::to_string(i) + "].diffuse", programState->spotLight.diffuse);
            shaderLightingPass.setVec3("spotLight[" + std::to_string(i) + "].specular", programState->spotLight.specular);
            shaderLightingPass.setFloat("spotLight[" + std::to_string(i) + "].constant", programState->spotLight.constant);
            shaderLightingPass.setFloat("spotLight[" + std::to_string(i) + "].linear", programState->spotLight.linear);
            shaderLightingPass.setFloat("spotLight[" + std::to_string(i) + "].quadratic", programState->spotLight.quadratic);
            shaderLightingPass.setFloat("spotLight[" + std::to_string(i) + "].cutOff", programState->spotLight.cutOff);
            shaderLightingPass.setFloat("spotLight[" + std::to_string(i) + "].outerCutOff", programState->spotLight.outerCutOff);

        }
        shaderLightingPass.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        shaderLightingPass.setVec3("dirLight.ambient", programState->dirLightAmbient);
        shaderLightingPass.setVec3("dirLight.specular", programState->dirLightSpecular);
        shaderLightingPass.setVec3("dirLight.diffuse", programState->dirLightDiffuse);
        shaderLightingPass.setVec3("viewPos", programState->camera.Position);
        shaderLightingPass.setFloat("material.shininess", 32.0f);
        //directional light

        shaderLightingPass.setBool("blinn",programState->blinn);

        renderQuad();


        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // 2. blur bright fragments with two-pass Gaussian Blur
        // --------------------------------------------------
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        shaderBlur.use();

        glActiveTexture(GL_TEXTURE0);

        glDisable(GL_DEPTH_TEST);

        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);
            shaderBlur.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration){


                first_iteration = false;
            }
        }
        glEnable(GL_DEPTH_TEST);



        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderBloomFinal.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gMask);

        shaderBloomFinal.setInt("bloom", programState->bloom);
        shaderBloomFinal.setFloat("exposure", programState->exposure);
        renderQuad();

        std::cout << "bloom: " << (programState->bloom ? "on" : "off") << "|hdr: " << (programState->hdr ? "on" : "off") << "| exposure: " << programState->exposure<<::endl;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
        glBlitFramebuffer(
                0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST
        );

        transparentShader.use();

        glBindVertexArray(transparentVAO);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
//
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(programState->planePosX,programState->planePosY,programState->planePosZ));
        model = glm::scale(model, glm::vec3(programState->planeScaleX,1,1));
        model = glm::scale(model, glm::vec3(1,programState->planeScaleY,1));
        model = glm::scale(model, glm::vec3(1,1,programState->planeScaleZ));
        transparentShader.setMat4("projection", projection);
        transparentShader.setMat4("view", view);
        float angle = 90.0f;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.0f,0.0f ));
        transparentShader.setMat4("model", model);
        transparentShader.setBool("blinn", programState->blinn);

        transparentShader.setVec3("viewPos", programState->camera.Position);
        transparentShader.setFloat("material.shininess", 32.0f);
        //directional light
        transparentShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        transparentShader.setVec3("dirLight.ambient", programState->dirLightAmbient);
        transparentShader.setVec3("dirLight.diffuse", programState->dirLightDiffuse *5.0f);
        transparentShader.setVec3("dirLight.specular", programState->dirLightSpecular);

        glDepthFunc(GL_LEQUAL);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisable(GL_BLEND);


        if (programState->ImGuiEnabled) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();


            {
                static float f = 0.0f;
                ImGui::Begin("Hello window");
                ImGui::SliderFloat("Exposure slider", &programState->exposure, 0.0, 5.0);
                ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
                ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
                ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
                ImGui::End();
            }

            {
                ImGui::Begin("Camera info");
                const Camera &c = programState->camera;
                ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
                ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
                ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
                ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
                ImGui::End();
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
        programState->hdr = true;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
        programState->hdr = false;
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
        programState->bloom = true;
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
        programState->bloom = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        programState->blinn = true;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        programState->blinn = false;

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !freeCamKeyPressed) {
        programState->camera.freeCam = !programState->camera.freeCam;
        freeCamKeyPressed = true;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}



void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
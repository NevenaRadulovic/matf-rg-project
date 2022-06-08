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
unsigned int loadTexture(char const * path,bool gammaCorrection);
unsigned int loadCubeMap(vector<std::string> faces);
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse =true;


bool hdr = false;
bool hdrKeyPressed = false;
bool bloom = true;
bool bloomKeyPressed = false;

float exposure = 1.5f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
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
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

void setShader(Shader myShader, DirLight dirLight, PointLight pointLight, SpotLight spotLight, vector<glm::vec3> lightPos,bool hdr);


struct ProgramState {

    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    bool lightOn = false;
    bool fKeyPressed=false;
    DirLight dirLight;
    PointLight pointLight;
    SpotLight spotLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 4.0f, 5.0f)) {}

    void SaveToFile(std::string filename);
    void LoadFromFile(std::string filename);
    glm::vec3 tempPosition=glm::vec3(0.0f, 2.0f, 0.0f);
    float tempScale=1.0f;
    float tempRotation=0.0f;
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        << camera.Up.x<<'\n'
        << camera.Up.y<<'\n'
        << camera.Up.z<<'\n'
        << camera.Right.x<<'\n'
        << camera.Right.y<<'\n'
        << camera.Right.z<<'\n'
        << camera.WorldUp.x<<'\n'
        << camera.WorldUp.y<<'\n'
        << camera.WorldUp.z<<'\n'
        << lightOn << '\n'
        << fKeyPressed << '\n'
        << tempPosition.x << '\n'
        << tempPosition.y << '\n'
        << tempPosition.z << '\n'
        << tempScale << '\n'
        << tempRotation << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> camera.Up.x
           >> camera.Up.y
           >> camera.Up.z
           >> camera.Right.x
           >> camera.Right.y
           >> camera.Right.z
           >> camera.WorldUp.x
           >> camera.WorldUp.y
           >> camera.WorldUp.z
           >> lightOn
           >> fKeyPressed
           >> tempPosition.x
           >> tempPosition.y
           >> tempPosition.z
           >> tempScale
           >> tempRotation;
    }
}
ProgramState *programState;

void DrawImGui(ProgramState *programState);

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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Project", nullptr, nullptr);
    if (window == nullptr) {
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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    //light
    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3(-1.0f, -0.2f, 0.0f);
    dirLight.ambient = glm::vec3(0.02f,0.02f,0.04f );
    dirLight.diffuse = glm::vec3(0.4f, 0.4f, 0.6f);
    dirLight.specular = glm::vec3(0.7f, 0.7f, 0.7f);



    PointLight& pointLight = programState->pointLight;
    pointLight.ambient = glm::vec3(0.1,0.2,0.5);
    pointLight.diffuse = glm::vec3( 0.0f,0.2f,0.4f);
    pointLight.specular = glm::vec3(0.0f, 0.0f, 1.0f);
    pointLight.constant = 1.0f;
    pointLight.linear = 0.005f;
    pointLight.quadratic = 0.032f;


    SpotLight& spotLight = programState->spotLight;
    spotLight.position = programState->camera.Position;
    spotLight.direction = programState->camera.Front;
    spotLight.ambient = glm::vec3 (1.0f);
    spotLight.diffuse = glm::vec3 (0.9f);
    spotLight.specular = glm::vec3 (0.5f, 0.5f, 0.5f);
    spotLight.constant = 1.0f;
    spotLight.linear = 0.05f;
    spotLight.quadratic = 0.032f;
    spotLight.cutOff = glm::cos(glm::radians(10.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(13.0f));

    //Shaders
    Shader objShader("resources/shaders/object_shader.vs","resources/shaders/object_shader.fs");
    Shader skyboxShader("resources/shaders/skybox.vs","resources/shaders/skybox.fs");
    Shader waterShader("resources/shaders/water_blending.vs","resources/shaders/water_blending.fs");
    Shader discardShader("resources/shaders/discard_shader.vs","resources/shaders/discard_shader.fs");
    Shader blurShader("resources/shaders/blur.vs","resources/shaders/blur.fs");
    Shader bloomShader("resources/shaders/bloom_final.vs","resources/shaders/bloom_final.fs");



    //Models
    Model island("resources/objects/island/island.obj",true);
    island.SetShaderTextureNamePrefix("material.");

    Model crystal("resources/objects/crystals/crystal5.obj",true);
    crystal.SetShaderTextureNamePrefix("material.");

    Model tree("resources/objects/BlueTree/BlueTree.obj",true);
    tree.SetShaderTextureNamePrefix("material.");

    Model stomp("resources/objects/stomp/BlueStump.obj",true);
    stomp.SetShaderTextureNamePrefix("material.");

    Model lamp("resources/objects/lamp/lamp.obj",true);
    lamp.SetShaderTextureNamePrefix("material.");

    Model lightCrystal("resources/objects/crystals/crystal4.obj",true);
    lightCrystal.SetShaderTextureNamePrefix("material.");

    Model arch("resources/objects/Arch/stoneArch.obj",true);
    arch.SetShaderTextureNamePrefix("material.");

    Model platform("resources/objects/Platform/StonePlatform.obj",true);
    platform.SetShaderTextureNamePrefix("material.");

    Model stone("resources/objects/stone/stone.obj",true);
    stone.SetShaderTextureNamePrefix("material.");



    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    float transparentVertices[] = {
            // positions         //normals         // texture Coords (swapped y coordinates because texture is flipped upside down)
            25.0f,  0.0f,  25.0f, 0.0f, 1.0f, 0.0f, 40.0f, 0.0f,
            -25.0f,  0.0f,  25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -25.0f,  0.0f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 40.0f,

            25.0f,  0.0f,  25.0f, 0.0f, 1.0f, 0.0f, 40.0f, 0.0f,
            -25.0f,  0.0f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 40.0f,
            25.0f,  0.0f, -25.0f, 0.0f, 1.0f, 0.0f, 40.0f, 40.0f
    };

    float transparentVertices2[] = {
            // positions         // texture Coords
            0.5f, 0.5f,  0.0f,  1.0f,  1.0f,
            0.5f,  -0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  -0.5f,  0.0f,  0.0f,  0.0f,
            -0.5f, 0.5f,  0.0f,  0.0f,  1.0f

    };

    unsigned int transparent2Indices[] = {
            0, 1, 3,
            1, 2, 3
    };
    //transparent VAO for water
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

   //transparentVAO2 for grass and portal
    unsigned int transparentVAO2, transparentVBO2, transparentEBO2;
    glGenVertexArrays(1, &transparentVAO2);
    glGenBuffers(1, &transparentVBO2);
    glGenBuffers(1, &transparentEBO2);

    glBindVertexArray(transparentVAO2);

    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices2), transparentVertices2, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparentEBO2);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(transparent2Indices), transparent2Indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));


    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
  
  
    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // load textures
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/water_dark.png").c_str(),true);
    unsigned int grassTexture = loadTexture(FileSystem::getPath("resources/textures/grass2.png").c_str(),true);
    unsigned int portalTexture = loadTexture(FileSystem::getPath("resources/textures/portal2.png").c_str(),true);


    //BLOOM
    // configure (floating point) framebuffers

    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
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
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    // shader configuration
    blurShader.use();
    blurShader.setInt("image", 0);
    bloomShader.use();
    bloomShader.setInt("scene", 0);
    bloomShader.setInt("bloomBlur", 1);


    vector<glm::vec3> pointLightPositions = {
            glm::vec3(1.2f, 3.35f, -0.5f),
            glm::vec3(1.2f, 3.36f, 0.4f),
            glm::vec3(-1.64f, 4.43f+sin(glfwGetTime())*0.02, -0.35f),
            glm::vec3(-0.6f, 3.0f, -0.77f),
            glm::vec3(-0.1f, 2.8f, 0.87f)
    };




    glm::vec3 crystalsPositions[] = {
            glm::vec3(-0.1f, 3.02f, 0.87f),
            glm::vec3(-0.6f, 3.0f, -0.77f)

    };

    vector<glm::vec3> plants
            {
                    glm::vec3(1.55f, 3.22f, -0.42f),
                    glm::vec3(1.24f, 3.2f, 0.33f),
                    glm::vec3(-0.7f, 3.2f, 1.1f),
                    glm::vec3(-0.2f, 3.2f, -1.0f),
                    glm::vec3(-1.85f, 4.18f, -0.6f),
                    glm::vec3(0.1f, 3.2f, 0.9f),
                    glm::vec3(-0.73f, 3.2f, 0.1f)

            };

    vector<glm::vec3> waterSquares
            {
                    glm::vec3(-25.0f, 0.0f, -25.0f),
                    glm::vec3(-25.0f, 0.0f, 25.0f),
                    glm::vec3(25.0f, 0.0f, -25.0f),
                    glm::vec3(25.0f, 0.0f, 25.0f)
            };

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/Skybox/Front.png"),
                    FileSystem::getPath("resources/textures/Skybox/Back.png"),
                    FileSystem::getPath("resources/textures/Skybox/Up.png"),
                    FileSystem::getPath("resources/textures/Skybox/Down.png"),
                    FileSystem::getPath("resources/textures/Skybox/LeftRight.png"),
                    FileSystem::getPath("resources/textures/Skybox/LeftRight.png")
            };
    unsigned int cubeMapTexture = loadCubeMap(faces);


    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = (float)currentFrame - lastFrame;
        lastFrame = (float)currentFrame;

        // input
        processInput(window);
        setShader(objShader, dirLight, pointLight, spotLight, pointLightPositions,hdr);

        // render
        glClearColor(0.0f,0.0f,0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        //island
        objShader.use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.2f));	// it's a bit too big for our scene, so scale it down
        objShader.setMat4("model", model);
        island.Draw(objShader);

       //crystals
        for(auto &crystalsPosition: crystalsPositions){
            model = glm::mat4(1.0f);
            model = glm::translate(model, crystalsPosition); // translate it down so it's at the center of the scene
            model = glm::scale(model, glm::vec3(0.15f));	// it's a bit too big for our scene, so scale it down
            objShader.setMat4("model", model);
            crystal.Draw(objShader);
        }

        // tree
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.6f, 2.85f, 0.6f));
        model = glm::rotate(model,glm::radians(90.0f),glm::vec3(0.0f,1.0f,0.0f));
        model = glm::scale(model, glm::vec3(0.25f));	// it's a bit too big for our scene, so scale it down
        objShader.setMat4("model", model);
        tree.Draw(objShader);


        //arch
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.57f, 3.0f, -0.024f));
        model = glm::scale(model, glm::vec3(0.205f));	// it's a bit too big for our scene, so scale it down
        objShader.setMat4("model", model);
        arch.Draw(objShader);

        //platform
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.55f, 2.73f, -0.45f));
        model = glm::scale(model, glm::vec3(0.205f));	// it's a bit too big for our scene, so scale it down
        objShader.setMat4("model", model);
        platform.Draw(objShader);

        //stone1
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.5f, 2.93f, -1.55f));
        model = glm::rotate(model,glm::radians(20.0f),glm::vec3(0.0f,1.0f,0.0f));
        model = glm::scale(model, glm::vec3(0.14f));	// it's a bit too big for our scene, so scale it down
        objShader.setMat4("model", model);
        stone.Draw(objShader);

        //stone2
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.3f, 2.82f, 1.7f));
        model = glm::rotate(model,glm::radians(180.0f),glm::vec3(0.0f,1.0f,0.0f));
        model = glm::scale(model, glm::vec3(0.14f));	// it's a bit too big for our scene, so scale it down
        objShader.setMat4("model", model);
        stone.Draw(objShader);

        //stone3
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.05f, 4.0f, -0.35f));
        model = glm::rotate(model,glm::radians(90.0f),glm::vec3(0.0f,1.0f,0.0f));
        model = glm::scale(model, glm::vec3(0.06f));	// it's a bit too big for our scene, so scale it down
        objShader.setMat4("model", model);
        stone.Draw(objShader);

        //stomp1
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.55f, 2.9f, -0.2f));
        model = glm::scale(model, glm::vec3(0.11f));	// it's a bit too big for our scene, so scale it down
        objShader.setMat4("model", model);
        stomp.Draw(objShader);

        //stomp2
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.64f, 4.0f, -0.35f));
        model = glm::rotate(model,glm::radians(70.0f),glm::vec3(0.0f,1.0f,0.0f));
        model = glm::scale(model, glm::vec3(0.11f));	// it's a bit too big for our scene, so scale it down
        objShader.setMat4("model", model);
        stomp.Draw(objShader);

        //lamp1
        objShader.use();
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.2f, 3.07f, -0.5f));
        model = glm::rotate(model,glm::radians(-90.0f),glm::vec3(0.0f,1.0f,0.0f));
        model = glm::scale(model, glm::vec3(0.3f));
        objShader.setMat4("model", model);
        lamp.Draw(objShader);

        //lamp2
        objShader.use();
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.2f, 3.05f, 0.4f));
        model = glm::rotate(model,glm::radians(-90.0f),glm::vec3(0.0f,1.0f,0.0f));
        model = glm::scale(model, glm::vec3(0.3f));
        objShader.setMat4("model", model);
        lamp.Draw(objShader);


        //light crystal
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.64f, 4.45f+sin(glfwGetTime())*0.02, -0.35f));
        model = glm::scale(model, glm::vec3(0.05f));
        objShader.setMat4("model", model);
        lightCrystal.Draw(objShader);

        //plants
        discardShader.use();
        discardShader.setMat4("projection", projection);
        discardShader.setMat4("view", view);
        glBindVertexArray(transparentVAO2);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        for (unsigned int i = 0; i < plants.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, plants[i]);
            model = glm::rotate(model, (float)i*60.0f, glm::vec3(0.0, 0.1, 0.0));
            model = glm::scale(model, glm::vec3(0.4f));
            discardShader.setMat4("model", model);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        }

        //portal
        discardShader.use();
        discardShader.setMat4("projection", projection);
        discardShader.setMat4("view", view);
        glBindVertexArray(transparentVAO2);
        glBindTexture(GL_TEXTURE_2D, portalTexture);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.6f, 3.465f, -0.02f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 0.1, 0.0));
        model = glm::rotate(model, (float)(glfwGetTime()*0.05), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(0.745f));
        discardShader.setMat4("model", model);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glDisable(GL_CULL_FACE);

        //water rendering
        waterShader.use();
        waterShader.setVec3("viewPos", programState->camera.Position);
        waterShader.setMat4("projection", projection);
        waterShader.setMat4("view", view);
        waterShader.setFloat("currentFrame", currentFrame);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glBindVertexArray(transparentVAO);


        for (auto & waterSquare : waterSquares)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            model = glm::mat4(1.0f);
            model = glm::translate(model, waterSquare);
            waterShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDisable(GL_CULL_FACE);

        }

        //Skybox
        skyboxShader.use();
        skyboxShader.setInt("skybox", 0);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS); // set depth function back to default


        glBindFramebuffer(GL_FRAMEBUFFER, 0);

  //BLOOM
        // blur bright fragments with two-pass Gaussian Blur
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        blurShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloomShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        bloomShader.setInt("hdr", hdr);
        bloomShader.setInt("bloom", bloom);
        bloomShader.setFloat("exposure", exposure);
        renderQuad();

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
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &transparentVAO);
    glDeleteVertexArrays(1, &transparentVAO2);

    glDeleteBuffers(1, &skyboxVBO);
    glDeleteBuffers(1, &transparentVBO);
    glDeleteBuffers(1, &transparentVBO2);

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
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    //spot light
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !programState->fKeyPressed){
        programState->lightOn = !programState->lightOn;
        programState->fKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
    {
        programState->fKeyPressed = false;
    }

    //hdr
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !hdrKeyPressed)
    {
        hdr = !hdr;
        hdrKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        hdrKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }

    //exposure
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.02f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        exposure += 0.02f;
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
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos; // reversed since y-coordinates go from bottom to top

    lastX = (float)xpos;
    lastY = (float)ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::DragFloat3("Temp position", (float*)&programState->tempPosition, 0.01,-20.0, 20.0);
        ImGui::DragFloat("Temp scale", &programState->tempScale, 0.02, 0.02, 128.0);
        ImGui::DragFloat("Temp rotation", &programState->tempRotation, 0.5, 0.0, 360.0);


        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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


unsigned int loadCubeMap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void setShader(Shader myShader, DirLight dirLight, PointLight pointLight, SpotLight spotLight, vector<glm::vec3> lightPos,bool hdr){
    myShader.use();

    myShader.setVec3("viewPosition", programState->camera.Position);
    myShader.setFloat("material.shininess", 32.0f);

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                            (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = programState->camera.GetViewMatrix();
    myShader.setMat4("projection", projection);
    myShader.setMat4("view", view);

    //directional lights
    myShader.setVec3("dirLight.direction", dirLight.direction);
    myShader.setVec3("dirLight.ambient", dirLight.ambient);
    myShader.setVec3("dirLight.diffuse", dirLight.diffuse);
    myShader.setVec3("dirLight.specular", dirLight.specular);


    //point lights
    for(unsigned int i=2; i<=4; i++){
        myShader.setVec3("pointLights[" + std::to_string(i-2) + "].position", lightPos[i]);
        myShader.setVec3("pointLights[" + std::to_string(i - 2) + "].ambient", pointLight.ambient);
        myShader.setVec3("pointLights[" + std::to_string(i - 2) + "].diffuse", pointLight.diffuse);
        myShader.setVec3("pointLights[" + std::to_string(i - 2) + "].specular", pointLight.specular);
        myShader.setFloat("pointLights[" + std::to_string(i - 2) + "].constant", pointLight.constant);
        myShader.setFloat("pointLights[" + std::to_string(i - 2) + "].linear", pointLight.linear);
        myShader.setFloat("pointLights[" + std::to_string(i - 2) + "].quadratic", pointLight.quadratic);

    }
    
    //candles
    for(unsigned int i=0; i<=1; i++){
        myShader.setVec3("candles[" + std::to_string(i) + "].position", lightPos[i]);
        if(hdr){
            myShader.setVec3("candles[" + std::to_string(i) + "].ambient", glm::vec3(50.0f,50.0f,200.0f));
            myShader.setVec3("candles[" + std::to_string(i) + "].diffuse", glm::vec3(1.0));
            myShader.setVec3("candles[" + std::to_string(i) + "].specular", glm::vec3(1.5));
            myShader.setFloat("candles[" + std::to_string(i) + "].constant", 1.0f);
            myShader.setFloat("candles[" + std::to_string(i) + "].linear", 100.0f);
            myShader.setFloat("candles[" + std::to_string(i) + "].quadratic", 100.0f);
        }else {

            myShader.setVec3("candles[" + std::to_string(i) + "].diffuse", pointLight.diffuse);
            myShader.setVec3("candles[" + std::to_string(i) + "].specular", pointLight.specular);
            myShader.setFloat("candles[" + std::to_string(i) + "].constant", pointLight.constant);
            myShader.setFloat("candles[" + std::to_string(i) + "].linear", pointLight.linear);
            myShader.setFloat("candles[" + std::to_string(i) + "].quadratic", pointLight.quadratic);
            myShader.setVec3("candles[" + std::to_string(i) + "].ambient", pointLight.ambient);
        }
    }
    
    

    //spot light
    myShader.setInt("lightOn", programState->lightOn);
    myShader.setVec3("spotLight.position", programState->camera.Position);
    myShader.setVec3("spotLight.direction", programState->camera.Front);
    myShader.setVec3("spotLight.ambient", spotLight.ambient);
    myShader.setVec3("spotLight.diffuse", spotLight.diffuse);
    myShader.setVec3("spotLight.specular", spotLight.specular);
    myShader.setFloat("spotLight.constant", spotLight.constant);
    myShader.setFloat("spotLight.linear", spotLight.linear);
    myShader.setFloat("spotLight.quadratic", spotLight.quadratic);
    myShader.setFloat("spotLight.cutOff", spotLight.cutOff);
    myShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

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

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <learnopengl/camera.h>
#include <learnopengl/filesystem.h>
#include <learnopengl/model.h>
#include <learnopengl/shader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods);

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

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

struct ProgramState {
  glm::vec3 clearColor = glm::vec3(0.2);
  bool ImGuiEnabled = false;
  Camera camera;
  bool CameraMouseMovementUpdateEnabled = true;
  glm::vec3 backpackPosition = glm::vec3(0.0f);
  float backpackScale = 1.0f;
  PointLight pointLight;
  ProgramState() : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

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
      << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
  std::ifstream in(filename);
  if (in) {
    in >> clearColor.r >> clearColor.g >> clearColor.b >> ImGuiEnabled >>
        camera.Position.x >> camera.Position.y >> camera.Position.z >>
        camera.Front.x >> camera.Front.y >> camera.Front.z;
  }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

float rectangleVertices[] = {
    // Coords    // texCoords
    1.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,

    1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f};

float skyboxVertices[] = {-1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
                          1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
                          -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                          1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f};

unsigned int skyboxIndices[] = {1, 2, 6, 6, 5, 1, 0, 4, 7, 7, 3, 0,
                                4, 5, 6, 6, 7, 4, 0, 3, 2, 2, 1, 0,
                                0, 1, 5, 5, 4, 0, 3, 7, 6, 6, 2, 3};

auto main() -> int {
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
  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", nullptr, nullptr);
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
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // tell stb_image.h to flip loaded texture's on the y-axis (before loading
  // model).
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
  (void)io;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  // configure global opengl state
  // -----------------------------
  // Testiranje dubine
  glEnable(GL_DEPTH_TEST);
  // Stencil test
  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  // Face-culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CW);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // GAMMA KOREKCIJA
  double gamma = 0.4;
  glEnable(GL_FRAMEBUFFER_SRGB);
  // build and compile shaders
  // -------------------------
  Shader framebufferShader("resources/shaders/framebuffer.vs",
                           "resources/shaders/framebuffer.fs");
  Shader skyboxShader("resources/shaders/skybox.vs",
                      "resources/shaders/skybox.fs");
  Shader windowsShader("resources/shaders/windows.vs",
                       "resources/shaders/windows.fs");
  Shader cobraShader("resources/shaders/cobra.vs",
                     "resources/shaders/cobra.fs");
  Shader cobraOutlineShader("resources/shaders/cobra_outline.vs",
                            "resources/shaders/cobra_outline.fs");
  Shader rb1Shader("resources/shaders/building.vs",
                   "resources/shaders/building.fs");
  Shader rb2Shader("resources/shaders/building.vs",
                   "resources/shaders/building.fs");
  Shader rb3Shader("resources/shaders/building.vs",
                   "resources/shaders/building.fs");
  Shader rb4Shader("resources/shaders/building.vs",
                   "resources/shaders/building.fs");
  Shader roadShader("resources/shaders/road.vs", "resources/shaders/road.fs");
  // load models
  // -----------
  Model windowsModel("resources/objects/windows/scene.gltf");
  Model cobraModel("resources/objects/cobra/Shelby.obj");
  Model rb1Model("resources/objects/buildings/rb1.obj");
  Model rb2Model("resources/objects/buildings/rb2.obj");
  Model rb3Model("resources/objects/buildings/rb3.obj");
  Model rb4Model("resources/objects/buildings/rb4.obj");
  Model roadModel("resources/objects/road/road.obj");

  cobraModel.SetShaderTextureNamePrefix("material.");
  rb1Model.SetShaderTextureNamePrefix("material.");
  rb2Model.SetShaderTextureNamePrefix("material.");
  rb3Model.SetShaderTextureNamePrefix("material.");
  rb4Model.SetShaderTextureNamePrefix("material.");
  roadModel.SetShaderTextureNamePrefix("material.");

  // Inicijalne postavke skybox-a
  skyboxShader.use();
  skyboxShader.setInt("skybox", 0);

  // Create VAO, VBO, and EBO for the skybox
  unsigned int skyboxVAO, skyboxVBO, skyboxEBO;
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);
  glGenBuffers(1, &skyboxEBO);
  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                        (void *)nullptr);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // All the faces of the cubemap (make sure they are in this exact order)
  std::string facesCubemap[6] = {
      "resources/textures/skybox/rt.jpg", "resources/textures/skybox/lf.jpg",
      "resources/textures/skybox/up.jpg", "resources/textures/skybox/dn.jpg",
      "resources/textures/skybox/ft.jpg", "resources/textures/skybox/bk.jpg",
  };

  // Kreiraj cubemap teksturu
  unsigned int cubemapTexture;
  glGenTextures(1, &cubemapTexture);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  // glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  for (unsigned int i = 0; i < 6; i++) {
    int width, height, nrCh;
    unsigned char *data =
        stbi_load(facesCubemap[i].c_str(), &width, &height, &nrCh, 0);
    if (data) {
      stbi_set_flip_vertically_on_load(false);
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, width,
                   height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
    } else {
      std::cout << "Failed to load texture: " << facesCubemap[i] << std::endl;
      stbi_image_free(data);
    }
  }

  // Inicijalne postavke svetla
  PointLight &pointLight = programState->pointLight;
  pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
  pointLight.ambient = glm::vec3(0.2, 0.2, 0.2);
  pointLight.diffuse = glm::vec3(44.6, 44.6, 44.6);
  pointLight.specular = glm::vec3(445.0, 454.0, 454.0);

  pointLight.constant = 1.0f;
  pointLight.linear = 0.09f;
  pointLight.quadratic = 0.032f;

  // draw in wireframe
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // FRAMEBUFFER
  // Create Frame Buffer Object
  unsigned int FBO;
  glGenFramebuffers(1, &FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);

  // Create Framebuffer Texture
  unsigned int framebufferTexture;
  glGenTextures(1, &framebufferTexture);
  glBindTexture(GL_TEXTURE_2D, framebufferTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                  GL_CLAMP_TO_EDGE); // Prevents edge bleeding
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                  GL_CLAMP_TO_EDGE); // Prevents edge bleeding
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         framebufferTexture, 0);

  // Create Render Buffer Object
  unsigned int RBO;
  glGenRenderbuffers(1, &RBO);
  glBindRenderbuffer(GL_RENDERBUFFER, RBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH,
                        SCR_HEIGHT);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, RBO);

  // Prepare framebuffer rectangle VBO and VAO
  unsigned int rectVAO, rectVBO;
  glGenVertexArrays(1, &rectVAO);
  glGenBuffers(1, &rectVBO);
  glBindVertexArray(rectVAO);
  glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), &rectangleVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));

  // Error checking framebuffer
  auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "Framebuffer error: " << fboStatus << std::endl;

  // render loop
  // -----------
  framebufferShader.use();
  framebufferShader.setInt("screenTexture", 0);
  framebufferShader.setFloat("gamma", gamma);
  while (!glfwWindowShouldClose(window)) {
    // per-frame time logic
    // --------------------
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // input
    // -----
    processInput(window);

    glClearColor(pow(programState->clearColor.r, gamma),
                 pow(programState->clearColor.g, gamma),
                 pow(programState->clearColor.b, gamma), 1.0f);

    // Specify the color of the background

    // render
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    // SKYBOX [POCETAK]
    glDepthFunc(GL_LEQUAL);
    skyboxShader.use();
    glm::mat4 skyboxView = glm::mat4(1.0f);
    glm::mat4 skyboxProjection = glm::mat4(1.0f);
    skyboxView = glm::mat4(glm::mat3(
        glm::lookAt(programState->camera.Position,
                    programState->camera.Position + programState->camera.Front,
                    programState->camera.Up)));
    skyboxProjection = glm::perspective(
        glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 1000.0f);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "view"), 1,
                       GL_FALSE, glm::value_ptr(skyboxView));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "projection"), 1,
                       GL_FALSE, glm::value_ptr(skyboxProjection));

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);

    // SKYBOX [KRAJ]

    // KOBRA [POCETAK]
    // Namestanje svetla za shader kobre
    cobraShader.use();
    pointLight.position =
        glm::vec3(10.0 * cos(currentFrame), 10.0f, 70.0 * sin(currentFrame));
    cobraShader.setVec3("pointLight.position", pointLight.position);
    cobraShader.setVec3("pointLight.ambient", pointLight.ambient);
    cobraShader.setVec3("pointLight.diffuse", pointLight.diffuse);
    cobraShader.setVec3("pointLight.specular", pointLight.specular);
    cobraShader.setFloat("pointLight.constant", pointLight.constant);
    cobraShader.setFloat("pointLight.linear", pointLight.linear);
    cobraShader.setFloat("pointLight.quadratic", pointLight.quadratic);
    cobraShader.setVec3("viewPosition", programState->camera.Position);
    cobraShader.setFloat("material.shininess", 32.0f);

    // transformacije modela kobre
    glm::mat4 cobraProjection =
        glm::perspective(glm::radians(programState->camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 cobraView = programState->camera.GetViewMatrix();
    cobraShader.setMat4("projection", cobraProjection);
    cobraShader.setMat4("view", cobraView);

    // crtanje modela Shelby kobre
    glm::mat4 cobraTransform = glm::mat4(1.0f);
    cobraTransform = glm::translate(
        cobraTransform,
        programState->backpackPosition); // translate it down so it's at the
                                         // center of the scene
    cobraTransform = glm::scale(
        cobraTransform,
        glm::vec3(programState->backpackScale)); // it's a bit too big for our
                                                 // scene, so scale it down
    cobraShader.setMat4("model", cobraTransform);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    cobraModel.Draw(cobraShader);

    // TODO: Ukloniti stencil bafer

    // Stencil buffer

    // STENCIL MODE KOBRA

    // KOBRA [KRAJ]

    // zgrada 1 [POCETAK]
    rb1Shader.use();
    pointLight.position =
        glm::vec3(4.0 * cos(currentFrame), 15.0f, 15.0 * sin(currentFrame));
    rb1Shader.setVec3("pointLight.position", pointLight.position);
    rb1Shader.setVec3("pointLight.ambient", pointLight.ambient);
    rb1Shader.setVec3("pointLight.diffuse", pointLight.diffuse);
    rb1Shader.setVec3("pointLight.specular", pointLight.specular);
    rb1Shader.setFloat("pointLight.constant", pointLight.constant);
    rb1Shader.setFloat("pointLight.linear", pointLight.linear);
    rb1Shader.setFloat("pointLight.quadratic", pointLight.quadratic);
    rb1Shader.setVec3("viewPosition", programState->camera.Position);
    rb1Shader.setFloat("material.shininess", 32.0f);

    glm::mat4 rb1Projection =
        glm::perspective(glm::radians(programState->camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 rb1View = programState->camera.GetViewMatrix();
    rb1Shader.setMat4("projection", rb1Projection);
    rb1Shader.setMat4("view", rb1View);

    glm::mat4 rb1Transform = glm::mat4(1.0f);
    rb1Transform = glm::translate(
        rb1Transform,
        programState->backpackPosition +
            glm::vec3(
                25.0, 0.0,
                5.0)); // translate it down so it's at the center of the scene
    rb1Transform = glm::scale(
        rb1Transform,
        glm::vec3(programState->backpackScale)); // it's a bit too big for our
                                                 // scene, so scale it down
    rb1Shader.setMat4("model", rb1Transform);
    rb1Model.Draw(rb1Shader);
    // zgrada1 [KRAJ]

    // zgrada2 [POCETAK]
    rb2Shader.use();
    pointLight.position =
        glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
    rb2Shader.setVec3("pointLight.position", pointLight.position);
    rb2Shader.setVec3("pointLight.ambient", pointLight.ambient);
    rb2Shader.setVec3("pointLight.diffuse", pointLight.diffuse);
    rb2Shader.setVec3("pointLight.specular", pointLight.specular);
    rb2Shader.setFloat("pointLight.constant", pointLight.constant);
    rb2Shader.setFloat("pointLight.linear", pointLight.linear);
    rb2Shader.setFloat("pointLight.quadratic", pointLight.quadratic);
    rb2Shader.setVec3("viewPosition", programState->camera.Position);
    rb2Shader.setFloat("material.shininess", 32.0f);

    glm::mat4 rb2Projection =
        glm::perspective(glm::radians(programState->camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 rb2View = programState->camera.GetViewMatrix();
    rb2Shader.setMat4("projection", rb2Projection);
    rb2Shader.setMat4("view", rb2View);

    glm::mat4 rb2Transform = glm::mat4(1.0f);
    rb2Transform = glm::translate(
        rb2Transform,
        programState->backpackPosition +
            glm::vec3(
                -25.0, 0.0,
                5.0)); // translate it down so it's at the center of the scene
    rb2Transform = glm::scale(
        rb2Transform,
        glm::vec3(programState->backpackScale)); // it's a bit too big for our
                                                 // scene, so scale it down
    rb2Shader.setMat4("model", rb2Transform);
    rb2Model.Draw(rb2Shader);
    // zgrada2 [KRAJ]

    // zgrada3 [POCETAK]
    rb3Shader.use();
    pointLight.position =
        glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
    rb3Shader.setVec3("pointLight.position", pointLight.position);
    rb3Shader.setVec3("pointLight.ambient", pointLight.ambient);
    rb3Shader.setVec3("pointLight.diffuse", pointLight.diffuse);
    rb3Shader.setVec3("pointLight.specular", pointLight.specular);
    rb3Shader.setFloat("pointLight.constant", pointLight.constant);
    rb3Shader.setFloat("pointLight.linear", pointLight.linear);
    rb3Shader.setFloat("pointLight.quadratic", pointLight.quadratic);
    rb3Shader.setVec3("viewPosition", programState->camera.Position);
    rb3Shader.setFloat("material.shininess", 32.0f);

    glm::mat4 rb3Projection =
        glm::perspective(glm::radians(programState->camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 rb3View = programState->camera.GetViewMatrix();
    rb3Shader.setMat4("projection", rb3Projection);
    rb3Shader.setMat4("view", rb3View);

    for (int i = -200; i < 200; i += 30) {
      glm::mat4 rb3Transform = glm::mat4(1.0f);
      rb3Transform = glm::translate(
          rb3Transform,
          programState->backpackPosition +
              glm::vec3(17.0, 0.0, 5.0 + i)); // translate it down so it's at
                                              // the center of the scene
      rb3Transform = glm::scale(
          rb3Transform,
          glm::vec3(programState->backpackScale)); // it's a bit too big for our
                                                   // scene, so scale it down
      rb3Shader.setMat4("model", rb3Transform);
      rb3Model.Draw(rb3Shader);
    }

    for (int i = -200; i < 200; i += 30) {
      glm::mat4 rb3Transform = glm::mat4(1.0f);
      rb3Transform = glm::translate(
          rb3Transform,
          programState->backpackPosition +
              glm::vec3(-15.0, 0.0, 5.0 + i)); // translate it down so it's at
                                               // the center of the scene
      rb3Transform = glm::scale(
          rb3Transform,
          glm::vec3(programState->backpackScale)); // it's a bit too big for our
                                                   // scene, so scale it down
      rb3Shader.setMat4("model", rb3Transform);
      rb3Model.Draw(rb3Shader);
    }
    // zgrada3 [KRAJ]

    // zgrada4 [POCETAK]
    rb4Shader.use();
    pointLight.position =
        glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
    rb4Shader.setVec3("pointLight.position", pointLight.position);
    rb4Shader.setVec3("pointLight.ambient", pointLight.ambient);
    rb4Shader.setVec3("pointLight.diffuse", pointLight.diffuse);
    rb4Shader.setVec3("pointLight.specular", pointLight.specular);
    rb4Shader.setFloat("pointLight.constant", pointLight.constant);
    rb4Shader.setFloat("pointLight.linear", pointLight.linear);
    rb4Shader.setFloat("pointLight.quadratic", pointLight.quadratic);
    rb4Shader.setVec3("viewPosition", programState->camera.Position);
    rb4Shader.setFloat("material.shininess", 32.0f);

    glm::mat4 rb4Projection =
        glm::perspective(glm::radians(programState->camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 rb4View = programState->camera.GetViewMatrix();
    rb4Shader.setMat4("projection", rb4Projection);
    rb4Shader.setMat4("view", rb4View);

    glm::mat4 rb4Transform = glm::mat4(1.0f);
    rb4Transform = glm::translate(
        rb4Transform,
        programState->backpackPosition +
            glm::vec3(
                -85.0, 0.0,
                5.0)); // translate it down so it's at the center of the scene
    rb4Transform = glm::scale(
        rb4Transform,
        glm::vec3(programState->backpackScale)); // it's a bit too big for our
                                                 // scene, so scale it down
    rb4Shader.setMat4("model", rb4Transform);
    rb4Model.Draw(rb1Shader);
    // zgrada4 [KRAJ]

    // PUT [POCETAK]
    roadShader.use();
    pointLight.position =
        glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
    roadShader.setVec3("pointLight.position", pointLight.position);
    roadShader.setVec3("pointLight.ambient", pointLight.ambient);
    roadShader.setVec3("pointLight.diffuse", pointLight.diffuse);
    roadShader.setVec3("pointLight.specular", pointLight.specular);
    roadShader.setFloat("pointLight.constant", pointLight.constant);
    roadShader.setFloat("pointLight.linear", pointLight.linear);
    roadShader.setFloat("pointLight.quadratic", pointLight.quadratic);
    roadShader.setVec3("viewPosition", programState->camera.Position);
    roadShader.setFloat("material.shininess", 32.0f);

    glm::mat4 roadProjection =
        glm::perspective(glm::radians(programState->camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 roadView = programState->camera.GetViewMatrix();
    roadShader.setMat4("projection", roadProjection);
    roadShader.setMat4("view", roadView);

    for (int i = 0; i < 10; i++) {
      glm::mat4 roadTransform = glm::mat4(1.0f);

      roadTransform = glm::translate(
          roadTransform,
          programState->backpackPosition +
              glm::vec3(0.0, -1.4, -i * 41.5)); // translate it down so it's at
                                                // the center of the scene
      roadTransform = glm::scale(
          roadTransform,
          glm::vec3(programState->backpackScale *
                    5.0)); // it's a bit too big for our scene, so scale it down
      roadShader.setMat4("model", roadTransform);
      roadModel.Draw(roadShader);
    }

    for (int i = 0; i < 10; i++) {
      glm::mat4 roadTransform = glm::mat4(1.0f);

      roadTransform = glm::translate(
          roadTransform,
          programState->backpackPosition +
              glm::vec3(0.0, -1.4, i * 41.5)); // translate it down so it's at
                                               // the center of the scene
      roadTransform = glm::scale(
          roadTransform,
          glm::vec3(programState->backpackScale *
                    5.0)); // it's a bit too big for our scene, so scale it down
      roadShader.setMat4("model", roadTransform);
      roadModel.Draw(roadShader);
    }

    // WINDOWS
    windowsShader.use();
    glm::mat4 windowsProjection =
        glm::perspective(glm::radians(programState->camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 windowsView = programState->camera.GetViewMatrix();
    roadShader.setMat4("projection", windowsProjection);
    roadShader.setMat4("view", windowsView);

    glm::mat4 windowTransform = glm::mat4(1.0f);

    windowTransform = glm::translate(
        windowTransform,
        programState->backpackPosition +
            glm::vec3(
                0.0, 5.0,
                0.0)); // translate it down so it's at the center of the scene

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    for (int i = 0; i < 5; i++) {
      windowTransform = glm::mat4(1.0f);
      windowTransform =
          glm::translate(windowTransform, programState->backpackPosition +
                                              glm::vec3(0.0, 3.0, i * 4));
      windowTransform = glm::scale(
          windowTransform,
          glm::vec3(programState->backpackScale *
                    5.0)); // it's a bit too big for our scene, so scale it down
      windowTransform =
          glm::rotate(windowTransform, glm::radians(cos(currentFrame) * 180),
                      glm::vec3(0, 1, 0));
      windowsShader.setMat4("model", windowTransform);

      windowsModel.Draw(windowsShader);
    }
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    // WINDOWS

    // PUT [KRAJ]

    // POCETAK KOBRA [STENCIL]
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);
    cobraOutlineShader.use();
    cobraOutlineShader.setFloat("str", 0.08f);
    cobraOutlineShader.setMat4("projection", cobraProjection);
    cobraOutlineShader.setMat4("view", cobraView);
    cobraOutlineShader.setMat4("model", cobraTransform);
    cobraModel.Draw(cobraOutlineShader);
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glEnable(GL_DEPTH_TEST);
    // KRAJ KOBRA [STENCIL]

    // FRAMEBUFFER
    framebufferShader.use();
    glDisable(GL_CULL_FACE);
    // glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(
        GL_DEPTH_TEST); // prevents framebuffer rectangle from being discarded
    // Draw the framebuffer rectangle

    glBindVertexArray(rectVAO);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // GUI crtanje
    if (programState->ImGuiEnabled)
      DrawImGui(programState);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved
    // etc.)
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

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
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
}

// glfw: whenever the window size changed (by OS or user resize) this callback
// function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
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
  float yoffset =
      lastY - ypos; // reversed since y-coordinates go from bottom to top

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

void DrawImGui(ProgramState *programState) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  {
    static float f = 0.0f;
    ImGui::Begin("Hello window");
    ImGui::Text("Hello text");
    ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
    ImGui::ColorEdit3("Background color", (float *)&programState->clearColor);
    ImGui::DragFloat3("Backpack position",
                      (float *)&programState->backpackPosition);
    ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1,
                     4.0);

    ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant,
                     0.05, 0.0, 1.0);
    ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear,
                     0.05, 0.0, 1.0);
    ImGui::DragFloat("pointLight.quadratic",
                     &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
    ImGui::End();
  }

  {
    ImGui::Begin("Camera info");
    const Camera &c = programState->camera;
    ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y,
                c.Position.z);
    ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
    ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
    ImGui::Checkbox("Camera mouse update",
                    &programState->CameraMouseMovementUpdateEnabled);
    ImGui::End();
  }

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
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

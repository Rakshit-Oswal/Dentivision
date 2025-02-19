#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "objectLoader.cpp"
#include "camera.cpp"
#include "shaders.cpp"
#include <fstream>

#define COLOR 45/255.0f 

#include "dependencies/imgui.h"
#include "dependencies/imgui_impl_glfw.h"
#include "dependencies/imgui_impl_opengl3.h"
#include "dependencies/imgui_demo.cpp"

const int screenHeight = 600, screenWidth = 800;
float fov = 45.f, lastFrame = .0f, deltaTime = .0f, movementSpeed = 2.f, lastX = screenWidth / 2.0f, lastY = screenHeight / 2.0f;
bool firstMouse = true, wireframeMode = false, rotating = false;

std::vector<float> vertices, colors, textureCoords;
std::vector<unsigned int> indices;

glm::vec3 camPos = glm::vec3(0.0, 0.0, 15.0);
glm::vec3 camFront = glm::vec3(0.0, 0.0, -1.0);
glm::vec3 camUp = glm::vec3(0.0, 1.0, 0.0);
glm::mat4 model = glm::mat4(1.0);

void setBuffers() {
    GLuint VAO, VBO, EBO, CBO, TBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &CBO);
    glGenBuffers(1, &TBO);

    glBindVertexArray(VAO);

    // Position buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Color buffer
    glBindBuffer(GL_ARRAY_BUFFER, CBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // Texture buffer
    glBindBuffer(GL_ARRAY_BUFFER, TBO);
    glBufferData(GL_ARRAY_BUFFER, textureCoords.size() * sizeof(float), textureCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
    // Update camera aspect ratio
    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (camera) {
        camera->screenWidth = static_cast<float>(width);
        camera->screenHeight = static_cast<float>(height);
    }
}

GLFWwindow* InitWindow(){
    if (!glfwInit()){
        std::cout << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Dentivision", NULL,NULL);

    if(!window) std::cout<<"Window was not created";
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }
    return window;
}

void pollKeyboardInputs(GLFWwindow* window, Camera& camera){
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.move(Camera::Movement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.move(Camera::Movement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.move(Camera::Movement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.move(Camera::Movement::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.move(Camera::Movement::UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.move(Camera::Movement::DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void pollMouseInputs(GLFWwindow* window){
    GLFWcursor* crosshairCursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        glfwSetCursor(window, crosshairCursor);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CROSSHAIR_CURSOR);
    }
    else {
        glfwSetCursor(window, nullptr);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xOffset = xpos - lastX;
        float yOffset = lastY - ypos; // Reversed since y-coordinates range from bottom to top
        lastX = xpos;
        lastY = ypos;

        //camera->rotate(xOffset, yOffset);

        // Apply rotation to the model matrix
        // Rotate around Y-axis for horizontal mouse movement (xOffset)
        float sensitivity = 0.3f;
        model = glm::rotate(model, glm::radians(xOffset * sensitivity), glm::vec3(0.0f, 1.0f, 0.0f));
        // Rotate around X-axis for vertical mouse movement (yOffset)
        model = glm::rotate(model, glm::radians(-yOffset * sensitivity), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    else{
        firstMouse = true;
    }
}

GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load image using stb_image
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    return textureID;
}

int main(){
    GLFWwindow* window = InitWindow();  
    if(!window) return -1;
    
    glViewport(0, 0, screenWidth, screenHeight);
    loadObj("2022-09-18_mouth_sketchfab.obj", vertices, indices, colors, textureCoords);

    setBuffers();

    std::string vertexShader = readFile("shader.vert"), fragmentShader = readFile("shader.frag");
    GLuint shader = CreateShader(vertexShader, fragmentShader);

    //create an object of the camera class
    Camera camera(camPos, camFront, camUp, fov, screenWidth, screenHeight, 0.1f, 100.0f);
    glfwSetWindowUserPointer(window, &camera); // Pass the camera to the mouse callback
    glfwSetCursorPosCallback(window, mouse_callback);

    //-----imgui-----
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
    //---------------

    glEnable(GL_DEPTH_TEST);
    //blending used to allow transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //-------------
    model = glm::scale(model, glm::vec3(15.0));
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    GLuint textureID = loadTexture("textures/mouth_Base_color.png");
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float time = static_cast<float>(glfwGetTime()); 
        
        //Update
        pollKeyboardInputs(window, camera);
        pollMouseInputs(window);

        glm::mat4 view = camera.look();
        glm::mat4 projection = camera.project();
        if(rotating) 
            model = glm::rotate(model, (glm::radians(1.0f)), glm::vec3(0.0, 1.0, 0.0));

        if (wireframeMode)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Solid mode

        //Draw
        glClearColor(COLOR, COLOR, COLOR, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(90, 100), ImGuiCond_FirstUseEver);
        // GUI
        ImGui::Begin("Controls");
        ImGui::Checkbox("Wireframe Mode", &wireframeMode);
        ImGui::Checkbox("Rotating", &rotating);
        ImGui::End();

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glUseProgram(shader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shader, "texture1"), 0);

        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform1f(glGetUniformLocation(shader, "time"), time);
        glUniform3fv(glGetUniformLocation(shader, "camPos"), 1, glm::value_ptr(camPos));

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
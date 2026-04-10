#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// Simple vector operations
void normalize(float& x, float& y, float& z) {
    float len = sqrt(x*x + y*y + z*z);
    if (len > 0.001f) {
        x /= len;
        y /= len;
        z /= len;
    }
}

// Texture loading function
GLuint loadTexture(const char* filename) {
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) {
        std::cerr << "Failed to load texture: " << filename << " - " << IMG_GetError() << std::endl;
        return 0;
    }
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    SDL_FreeSurface(surface);
    return texture;
}

// Shader creation function
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Create vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }
    
    // Create fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }
    
    // Link program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

// Vertex structure for cube
struct Vertex {
    float x, y, z;
    float u, v;
};

// Cube vertices with texture coordinates
std::vector<Vertex> cubeVertices = {
    // Front face
    {-0.5f, -0.5f,  0.5f, 0.0f, 0.0f},
    { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f},
    { 0.5f,  0.5f,  0.5f, 1.0f, 1.0f},
    {-0.5f,  0.5f,  0.5f, 0.0f, 1.0f},
    
    // Back face
    {-0.5f, -0.5f, -0.5f, 1.0f, 0.0f},
    {-0.5f,  0.5f, -0.5f, 1.0f, 1.0f},
    { 0.5f,  0.5f, -0.5f, 0.0f, 1.0f},
    { 0.5f, -0.5f, -0.5f, 0.0f, 0.0f},
    
    // Left face
    {-0.5f, -0.5f, -0.5f, 0.0f, 0.0f},
    {-0.5f, -0.5f,  0.5f, 1.0f, 0.0f},
    {-0.5f,  0.5f,  0.5f, 1.0f, 1.0f},
    {-0.5f,  0.5f, -0.5f, 0.0f, 1.0f},
    
    // Right face
    { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f},
    { 0.5f,  0.5f, -0.5f, 1.0f, 1.0f},
    { 0.5f,  0.5f,  0.5f, 0.0f, 1.0f},
    { 0.5f, -0.5f,  0.5f, 0.0f, 0.0f},
    
    // Top face
    {-0.5f,  0.5f, -0.5f, 0.0f, 0.0f},
    {-0.5f,  0.5f,  0.5f, 0.0f, 1.0f},
    { 0.5f,  0.5f,  0.5f, 1.0f, 1.0f},
    { 0.5f,  0.5f, -0.5f, 1.0f, 0.0f},
    
    // Bottom face
    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f},
    { 0.5f, -0.5f, -0.5f, 1.0f, 1.0f},
    { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f},
    {-0.5f, -0.5f,  0.5f, 0.0f, 0.0f}
};

// Camera
struct Camera {
    float x, y, z;
    float yaw, pitch;
    float moveSpeed;
    float mouseSensitivity;
    
    Camera() : x(0), y(1.5f), z(3), yaw(-90), pitch(0), 
               moveSpeed(10.0f), mouseSensitivity(0.02f) {}
    
    void processMouse(float dx, float dy) {
        yaw += dx * mouseSensitivity;
        pitch -= dy * mouseSensitivity;
        if (pitch > 89) pitch = 89;
        if (pitch < -89) pitch = -89;
    }
    
    void processKeyboard(const Uint8* keys, float dt) {
        float frontX = cos((yaw) * 3.14159f / 180) * cos(pitch * 3.14159f / 180);
        float frontY = sin(pitch * 3.14159f / 180);
        float frontZ = sin((yaw) * 3.14159f / 180) * cos(pitch * 3.14159f / 180);
        
        float rightX = cos((yaw - 90) * 3.14159f / 180);
        float rightZ = sin((yaw - 90) * 3.14159f / 180);
        
        if (keys[SDL_SCANCODE_W]) {
            x += frontX * moveSpeed * dt;
            z += frontZ * moveSpeed * dt;
        }
        if (keys[SDL_SCANCODE_S]) {
            x -= frontX * moveSpeed * dt;
            z -= frontZ * moveSpeed * dt;
        }
        if (keys[SDL_SCANCODE_A]) {
            x -= rightX * moveSpeed * dt;
            z -= rightZ * moveSpeed * dt;
        }
        if (keys[SDL_SCANCODE_D]) {
            x += rightX * moveSpeed * dt;
            z += rightZ * moveSpeed * dt;
        }
    }
};

// Draw cube with texture
void drawCube(GLuint vbo, GLuint texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1); // texCoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    
    glDrawArrays(GL_QUADS, 0, cubeVertices.size());
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

const char* vertexShaderSource = R"(
attribute vec3 position;
attribute vec2 texCoord;
varying vec2 vTexCoord;
void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);
    vTexCoord = texCoord;
}
)";

const char* fragmentShaderSource = R"(
uniform sampler2D textureSampler;
varying vec2 vTexCoord;
void main() {
    gl_FragColor = texture2D(textureSampler, vTexCoord);
}
)";

// Draw grid
void drawGrid(int size, float spacing) {
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);
    
    float half = size * spacing * 0.5f;
    for (int i = 0; i <= size; ++i) {
        float pos = -half + i * spacing;
        
        glVertex3f(pos, 0, -half);
        glVertex3f(pos, 0,  half);
        
        glVertex3f(-half, 0, pos);
        glVertex3f( half, 0, pos);
    }
    
    glEnd();
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed" << std::endl;
        return 1;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    SDL_Window* window = SDL_CreateWindow(
        "3D OpenGL - StellaTrail",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1200, 800,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window creation failed" << std::endl;
        SDL_Quit();
        return 1;
    }
    
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context creation failed" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    SDL_GL_MakeCurrent(window, glContext);
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed" << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_SetSwapInterval(1);
    
    std::cout << "OpenGL initialized" << std::endl;
    
    // Create shader program
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);
    
    // Create VBO for cube
    GLuint cubeVBO;
    glGenBuffers(1, &cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(Vertex), cubeVertices.data(), GL_STATIC_DRAW);
    
    // Load texture
    GLuint cubeTexture = loadTexture("player.png");
    if (cubeTexture == 0) {
        std::cerr << "Failed to load player.png texture" << std::endl;
        // Continue without texture, or exit
    }
    
    // Set texture uniform
    glUniform1i(glGetUniformLocation(shaderProgram, "textureSampler"), 0);
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float fov = 45.0f;
    float aspect = 1200.0f / 800.0f;
    float near = 0.1f, far = 100.0f;
    float f = 1.0f / tanf(fov * 3.14159f / 360.0f);
    glFrustum(-near * aspect / f, near * aspect / f, -near / f, near / f, near, far);
    
    Camera camera;
    bool running = true;
    bool menuMode = false;
    SDL_Event event;
    Uint64 lastTime = SDL_GetPerformanceCounter();
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_ShowCursor(SDL_DISABLE);
    int prevMouseX = 0, prevMouseY = 0;
    
    while (running) {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float dt = (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
        lastTime = currentTime;
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                menuMode = !menuMode;
                if (menuMode) {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    SDL_ShowCursor(SDL_ENABLE);
                    std::cout << "Menu Mode - ESC to resume" << std::endl;
                } else {
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    SDL_ShowCursor(SDL_DISABLE);
                    std::cout << "Game Mode - ESC for menu" << std::endl;
                }
            }
            if (event.type == SDL_MOUSEMOTION && !menuMode) {
                int dx = event.motion.x - prevMouseX;
                int dy = event.motion.y - prevMouseY;
                prevMouseX = event.motion.x;
                prevMouseY = event.motion.y;
                camera.processMouse(dx, dy);
            }
        }
        
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        if (!menuMode) {
            camera.processKeyboard(keys, dt);
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        float frontX = cos(camera.yaw * 3.14159f / 180) * cos(camera.pitch * 3.14159f / 180);
        float frontY = sin(camera.pitch * 3.14159f / 180);
        float frontZ = sin(camera.yaw * 3.14159f / 180) * cos(camera.pitch * 3.14159f / 180);
        glm::vec3 eye(camera.x, camera.y, camera.z);
        glm::vec3 center(camera.x + frontX, camera.y + frontY, camera.z + frontZ);
        glm::vec3 up(0, 1, 0);
        glm::mat4 view = glm::lookAt(eye, center, up);
        glLoadMatrixf(&view[0][0]);
        
        drawGrid(20, 1.0f);
        
        glPushMatrix();
        glTranslatef(0, 0.5f, -3);
        drawCube(cubeVBO, cubeTexture);
        glPopMatrix();
        
        SDL_GL_SwapWindow(window);
    }
    
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}

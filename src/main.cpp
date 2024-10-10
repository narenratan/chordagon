/**
 *  CHORDAGON - a microtonal chord visualizer
 *
 *  Draws each note being played as a point on the pitch circle
 *      Angle on the circle is proportional to pitch in cents
 *      Notes an octave apart get the same angle
 *  Draws lines connecting all pairs of notes
 *      Each line represents an interval
 *      Each line is coloured based on the size of the corresponding interval
 *  Draws the pitch circle itself
 *      Slowly rotating
 *
 *  Uses OpenGL. See shaders.h for the shader source code.
 */

#include <iostream>
#include <thread>
#include <map>
#include <ranges>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <libremidi/libremidi.hpp>
#include <readerwriterqueue.h>
#include <libMTSClient.h>

#ifdef TEXTURE_FROM_FILE
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define TWOPI 6.283185307179586

// Queue used to receive midi messages
static moodycamel::ReaderWriterQueue<libremidi::message, 4096> midiMessageQueue(128);

// Scale factors to adjust for window aspect ratio
static float scaleX = 1.0;
static float scaleY = 1.0;

// Number of points to use when drawing the pitch circle
static constexpr int N = 1024;

// Maximum number of simultaneously displayable notes
static constexpr int maxNotes = 16;

// clang-format off
// Indices of all edges between maxNotes vertices
static unsigned int indices[] = {
    0, 1,
    0, 2,
    1, 2,
    0, 3,
    1, 3,
    2, 3,
    0, 4,
    1, 4,
    2, 4,
    3, 4,
    0, 5,
    1, 5,
    2, 5,
    3, 5,
    4, 5,
    0, 6,
    1, 6,
    2, 6,
    3, 6,
    4, 6,
    5, 6,
    0, 7,
    1, 7,
    2, 7,
    3, 7,
    4, 7,
    5, 7,
    6, 7,
    0, 8,
    1, 8,
    2, 8,
    3, 8,
    4, 8,
    5, 8,
    6, 8,
    7, 8,
    0, 9,
    1, 9,
    2, 9,
    3, 9,
    4, 9,
    5, 9,
    6, 9,
    7, 9,
    8, 9,
    0, 10,
    1, 10,
    2, 10,
    3, 10,
    4, 10,
    5, 10,
    6, 10,
    7, 10,
    8, 10,
    9, 10,
    0, 11,
    1, 11,
    2, 11,
    3, 11,
    4, 11,
    5, 11,
    6, 11,
    7, 11,
    8, 11,
    9, 11,
    10, 11,
    0, 12,
    1, 12,
    2, 12,
    3, 12,
    4, 12,
    5, 12,
    6, 12,
    7, 12,
    8, 12,
    9, 12,
    10, 12,
    11, 12,
    0, 13,
    1, 13,
    2, 13,
    3, 13,
    4, 13,
    5, 13,
    6, 13,
    7, 13,
    8, 13,
    9, 13,
    10, 13,
    11, 13,
    12, 13,
    0, 14,
    1, 14,
    2, 14,
    3, 14,
    4, 14,
    5, 14,
    6, 14,
    7, 14,
    8, 14,
    9, 14,
    10, 14,
    11, 14,
    12, 14,
    13, 14,
    0, 15,
    1, 15,
    2, 15,
    3, 15,
    4, 15,
    5, 15,
    6, 15,
    7, 15,
    8, 15,
    9, 15,
    10, 15,
    11, 15,
    12, 15,
    13, 15,
    14, 15
};
// clang-format on

// Set scale factors when window is resized
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    float aspect = (float)height / (float)width;
    scaleX = aspect <= 1.0 ? aspect : 1.0;
    scaleY = aspect <= 1.0 ? 1.0 : 1.0 / aspect;
    glViewport(0, 0, width, height);
}

// Close window if escape key is pressed
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

GLFWwindow *setupWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(600, 600, "Chordagon", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }

    glEnable(GL_MULTISAMPLE);

    return window;
}

// Set up all vertices needed
void setupVertices(unsigned int VAO[])
{
    unsigned int VBO, EBO;
    glGenVertexArrays(3, VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Set up vertex array object for edges
    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Set up vertex array object for points
    glBindVertexArray(VAO[1]);
    glBindVertexArray(0);

    // Set up vertex array object for circle
    constexpr int M = 3 * (N + 1);
    float circleVertices[2 * M] = {0.0f};
    float theta = 0.0f;
    for (int i = 0; i <= N; i++)
    {
        theta = i * TWOPI / N;
        float c = std::cos(theta);
        float s = std::sin(theta);
        float d = std::abs(0.01 * sin(60 * theta));
        circleVertices[3 * (2 * i)] = (0.8 + d) * c;
        circleVertices[3 * (2 * i) + 1] = (0.8 + d) * s;
        circleVertices[3 * (2 * i + 1)] = (0.8 - d) * c;
        circleVertices[3 * (2 * i + 1) + 1] = (0.8 - d) * s;
    }

    glBindVertexArray(VAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(circleVertices), circleVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Load texture for edge colors
unsigned int loadTexture()
{
    // By default use pre-generated texture from texture.h
    // If TEXTURE_FROM_FILE is used, the texture is loaded from an image file
#ifdef TEXTURE_FROM_FILE
    int width, height, nrChannels;
    unsigned char *data = stbi_load("images/rainbow.jpg", &width, &height, &nrChannels, 0);
#else
#include "texture.h"
#endif

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

#ifdef TEXTURE_FROM_FILE
    // Print header corresponding to loaded image to stdout
    // This can then be copied into texture.h to allow building without the image file
    std::cout << std::format("int width = {};\n", width);
    std::cout << std::format("int height = {};\n", height);
    std::cout << std::format("int nrChannels = {};\n", nrChannels);
    std::cout << "unsigned char data[] = {\n";
    int n = width * height * nrChannels;
    for (int i = 0; i < n; i++)
    {
        std::cout << std::format("    0x{:X}", data[i]) << (i != n - 1 ? ",\n" : "\n");
    }
    std::cout << "};\n";
    stbi_image_free(data);
#endif

    return texture;
}

void setupMIDI()
{
    // Each midi input puts its messages onto the midiMessageQueue
    auto my_callback = [](const libremidi::message &message) {
        midiMessageQueue.try_enqueue(message);
    };

    libremidi::observer obs;

    // Listen on all midi ports
    int i = 0;
    std::cout << "MIDI input ports:" << std::endl;
    for (const libremidi::input_port &port : obs.get_input_ports())
    {
        std::cout << i++ << ": " << port.port_name << std::endl;
        libremidi::midi_in *midi =
            new libremidi::midi_in{libremidi::input_configuration{.on_message = my_callback}};
        midi->open_port(port);
    }
    std::cout << std::endl;
}

/**
 * Compile a shader program from source text
 *
 * Requires code for vertex shader and fragment shader.
 * Can optionally provide code for a geometry shader.
 */
unsigned int compileShaderProgram(std::string vertexCode, std::string geometryCode,
                                  std::string fragmentCode)
{
    bool useGeometryShader = geometryCode.size() != 0;

    const char *vShaderCode = vertexCode.c_str();
    const char *gShaderCode = geometryCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();

    unsigned int vertex, geometry, fragment, ID;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    if (useGeometryShader)
    {
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &gShaderCode, NULL);
        glCompileShader(geometry);
        glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(geometry, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    if (useGeometryShader)
    {
        glAttachShader(ID, geometry);
    }
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertex);
    if (useGeometryShader)
    {
        glDeleteShader(geometry);
    }
    glDeleteShader(fragment);

    return ID;
};

// Overload for when not using a geometry shader
unsigned int compileShaderProgram(std::string vertexCode, std::string fragmentCode)
{
    return compileShaderProgram(vertexCode, "", fragmentCode);
}

// IDs of all shader programs used later on
struct ShaderPrograms
{
    unsigned int point;
    unsigned int line;
    unsigned int circle;
};

// Compile all shader programs
ShaderPrograms compileShaders()
{
    // Include shader source code strings
#include "shaders.h"

    unsigned int pointShaderProgram = compileShaderProgram(
        pointVertexShaderSource, pointGeometryShaderSource, pointFragmentShaderSource);
    unsigned int lineShaderProgram = compileShaderProgram(
        pointVertexShaderSource, lineGeometryShaderSource, lineFragmentShaderSource);
    unsigned int circleShaderProgram =
        compileShaderProgram(circleVertexShaderSource, circleFragmentShaderSource);
    return ShaderPrograms{pointShaderProgram, lineShaderProgram, circleShaderProgram};
}

// Update the noteAngles map based on midi messages received
void updateNoteAngles(MTSClient *c, std::map<char, float> &noteAngles)
{
    libremidi::message m;
    double freq = 0.0;
    char noteNumber = 0;
    char velocity = 0;

    while (midiMessageQueue.try_dequeue(m))
    {
        if (m.get_message_type() == libremidi::message_type::NOTE_ON)
        {
            noteNumber = m.bytes[1];
            velocity = m.bytes[2];
            if (velocity == 0)
            {
                // Treat velocity 0 note-on as note-off (some MIDI controllers behave like this)
                noteAngles.erase(noteNumber);
            }
            else if (noteAngles.size() < maxNotes)
            {
                // Map each new note to its angle in radians on the pitch circle
                // Angle is proportional to note cents
                freq = MTS_NoteToFrequency(c, noteNumber, 0);
                noteAngles[noteNumber] = TWOPI * log2(freq / 440.0);
            }
        }
        if (m.get_message_type() == libremidi::message_type::NOTE_OFF)
        {
            // Remove notes we get a note-off for
            noteAngles.erase(m.bytes[1]);
        }
    }
}

// Draw points for notes, edges for intervals, and the pitch circle
void draw(ShaderPrograms shaders, unsigned int VAO[], std::map<char, float> noteAngles)
{
    glClearColor(5.0f / 255.0f, 1.0f / 255.0f, 74.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    float timeValue = glfwGetTime();

    // Copy note angles into array to pass as a uniform to shaders
    float noteAnglesArr[maxNotes] = {0.0f};
    assert(noteAngles.size() <= maxNotes);
    int i = 0;
    for (const auto &v : std::views::values(noteAngles))
    {
        noteAnglesArr[i] = v;
        i++;
    }

    glBindVertexArray(VAO[2]);
    glUseProgram(shaders.circle);
    glUniform1f(glGetUniformLocation(shaders.circle, "scaleX"), scaleX);
    glUniform1f(glGetUniformLocation(shaders.circle, "scaleY"), scaleY);
    glUniform1f(glGetUniformLocation(shaders.circle, "time"), timeValue);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * N);

    glBindVertexArray(VAO[0]);
    glUseProgram(shaders.line);
    glUniform1f(glGetUniformLocation(shaders.line, "scaleX"), scaleX);
    glUniform1f(glGetUniformLocation(shaders.line, "scaleY"), scaleY);
    glUniform1fv(glGetUniformLocation(shaders.line, "noteAngles"), maxNotes, noteAnglesArr);
    glDrawElements(GL_LINES, (noteAngles.size() * (noteAngles.size() - 1)), GL_UNSIGNED_INT, 0);

    glBindVertexArray(VAO[1]);
    glUseProgram(shaders.point);
    glUniform1f(glGetUniformLocation(shaders.point, "scaleX"), scaleX);
    glUniform1f(glGetUniformLocation(shaders.point, "scaleY"), scaleY);
    glUniform1fv(glGetUniformLocation(shaders.point, "noteAngles"), maxNotes, noteAnglesArr);
    glDrawArrays(GL_POINTS, 0, noteAngles.size());
}

int main()
{
    GLFWwindow *window = setupWindow();

    unsigned int VAO[3];
    setupVertices(VAO);

    setupMIDI();

    ShaderPrograms shaders = compileShaders();

    unsigned int texture = loadTexture();
    glUniform1i(glGetUniformLocation(shaders.line, "rainbow"), texture);

    MTSClient *c = MTS_RegisterClient();

    std::map<char, float> noteAngles; // {{0, 0.0}, {1, 0.1}, {2, 3.14}, {3, 5.0}};

    std::cout << std::this_thread::get_id() << " Starting main loop" << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        updateNoteAngles(c, noteAngles);
        draw(shaders, VAO, noteAngles);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    MTS_DeregisterClient(c);
    glDeleteVertexArrays(3, VAO);
    glfwTerminate();
    return 0;
}

int WinMain() { return main(); }

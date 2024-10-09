// GLSL source code for all shaders used

std::string pointVertexShaderSource = R"(

#version 330 core

uniform float scaleX, scaleY;
uniform float noteAngles[16];

void main()
{
    gl_Position = vec4(scaleX * (0.8 * sin(noteAngles[gl_VertexID])),
                       scaleY * (0.8 * cos(noteAngles[gl_VertexID])), 1.0, 1.0);
}

)";

std::string pointGeometryShaderSource = R"(

#version 330 core

#define TWOPI 6.283185307179586
#define N 60
#define TWONPLUSONE 121

layout(points) in;
layout(triangle_strip, max_vertices = TWONPLUSONE) out;

uniform float scaleX, scaleY;

void main()
{
    int i;
    float r = 0.02;
    float theta0 = TWOPI / N;
    float theta = 0.0;
    for (i = 0; i <= N; i++)
    {
        theta = i * theta0;
        gl_Position =
            gl_in[0].gl_Position + vec4(scaleX * r * cos(theta), scaleY * r * sin(theta), 0.0, 0.0);
        EmitVertex();
        gl_Position = gl_in[0].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}

)";

std::string pointFragmentShaderSource = R"(

#version 330 core
out vec4 FragColor;

void main() { FragColor = vec4(1.0); }

)";

std::string circleVertexShaderSource = R"(

#version 330 core
layout(location = 0) in vec3 aPos;

uniform float scaleX, scaleY, time;

void main()
{
    float c = cos(0.01 * time);
    float s = sin(0.01 * time);
    gl_Position =
        vec4(scaleX * (aPos.x * c + aPos.y * s), scaleY * (-aPos.x * s + aPos.y * c), 1.0, 1.0);
}

)";

std::string circleFragmentShaderSource = R"(

#version 330 core
out vec4 FragColor;

void main() { FragColor = vec4(0.5, 0.5, 0.5, 1.0); }

)";

std::string lineGeometryShaderSource = R"(

#version 330 core
layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

out float color;

uniform float scaleX, scaleY;
uniform float noteAngles[16];

#define PI 3.141592653589793

void main()
{
    // clang-format off
    int indices[] = int[120*2](
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
    );
    // clang-format on

    float phi1 = noteAngles[indices[2 * gl_PrimitiveIDIn]];
    float phi2 = noteAngles[indices[2 * gl_PrimitiveIDIn + 1]];

    float x = mod(abs(phi2 - phi1) / PI, 2.0);
    color = x < 1 ? x : 2 - x;

    float theta = phi1 + (phi2 - phi1) / 2.0;
    float costheta = cos(theta);
    float sintheta = sin(theta);

    float r = 0.01;
    vec4 d = vec4(scaleX * r * sintheta, scaleY * r * costheta, 0.0, 0.0);
    gl_Position = gl_in[0].gl_Position + d;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position - d;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position + d;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position - d;
    EmitVertex();
    EndPrimitive();
}

)";

std::string lineFragmentShaderSource = R"(

#version 330 core
out vec4 FragColor;
in float color;

uniform sampler2D rainbow;

void main() { FragColor = texture(rainbow, vec2(0.5, 1.0 - color)); }

)";

#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aSize;
layout(location = 3) in float aAlpha;

uniform mat4 U_MVP; // model-view-projection matrix
out vec3 vertexColor;
out float vertexAlpha;

void main()
{
    gl_Position = U_MVP * vec4(aPos, 0.0, 1.0);
    //gl_Position = vec4((aPos.x / 500.0) - 1.0, (aPos.y / 250.0) - 1.0, 0.0, 1.0);
    gl_PointSize = aSize;

    vertexColor = aColor;
    vertexAlpha = aAlpha;
}

#version 330 core
in vec3 vertexColor;
in float vertexAlpha;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor, vertexAlpha);
}

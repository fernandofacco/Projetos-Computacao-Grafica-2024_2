#version 430
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texc;
layout (location = 3) in vec3 normal;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

//Variáveis que irão para o fragment shader
out vec3 finalColor;
out vec2 texCoord;
out vec3 scaledNormal;
out vec3 fragPos;

void main()
{
	//...pode ter mais linhas de código aqui!
	gl_Position = projection * view * model * vec4(position, 1.0);
	finalColor = color;
    texCoord = texc;
    fragPos = vec3(model * vec4(position, 1.0));
    scaledNormal = vec3(model * vec4(normal, 1.0));
}
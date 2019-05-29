#include <glad/glad.h>
#include <SDL2/SDL.h>
#include "glad.c"

#include <iostream>
#include <cmath>
const float pi = acos(-1);
const float R = 0.75;
const float nR = R * cos(0.4*pi) / cos(0.2*pi);

SDL_Window *window;
SDL_GLContext GLContext;

void OnResize()
{
	int width, height;
	SDL_GetWindowSize(window, &width, &height);
	glViewport(0, 0, width, height);
}


static const char * vertexShaderSource= R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
out vec3 ourColor;
uniform float offsetX;
void main()
{
	gl_Position = vec4(aPos.xy+offsetX,aPos.z, 1.0);
	ourColor = aColor;
})";

static const char* fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
in vec3 ourColor;
void main()
{
	FragColor = vec4(ourColor,1.0f);
})";
class Shader
{
public:
	unsigned int ID;
	Shader(const GLchar * vertexSource, const GLchar * fragmentSource)
	{
		unsigned vid, fid; //顶点着色器 vid,片段着色器 fid
		int success;
		char infoLog[512];
		
		//编译
		vid = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vid, 1, &vertexSource, NULL);
		glCompileShader(vid);
		//检测GLSL代码是否编译错误
		glGetShaderiv(vid, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vid, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		}

		fid = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fid, 1, &fragmentSource, NULL);
		glCompileShader(fid);

		glGetShaderiv(fid, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fid, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		}

		ID = glCreateProgram();
		glAttachShader(ID, vid);
		glAttachShader(ID, fid);
		glLinkProgram(ID);

		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(ID, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}

		glDeleteProgram(vid);
		glDeleteProgram(fid);
	}
	void use()
	{
		glUseProgram(ID);
	}
	void setFloat(const char *s, float val)
	{
		glUniform1f(glGetUniformLocation(ID, s), val);
	}
};
int main(int argc, char *argv[])
{
	// glfw: initialize and configure
	// ------------------------------
	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow("MyOpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 600, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	GLContext = SDL_GL_CreateContext(window);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
	}
	

	float vertices[] = {
		0.0f,0.0f,0.0f,							1.0f, 1.0f, 0.0f,
		R*sin(0.0*pi), R*cos(0.0*pi), 0.0f,		1.0f, 1.0f, 0.0f,
		nR*sin(0.2*pi), nR*cos(0.2*pi), 0.0f,	0.5f, 0.5f, 0.0f,
		R*sin(0.4*pi), R*cos(0.4*pi), 0.0f,		1.0f, 1.0f, 0.0f,
		nR*sin(0.6*pi), nR*cos(0.6*pi), 0.0f,	0.5f, 0.5f, 0.0f,
		R*sin(0.8*pi), R*cos(0.8*pi), 0.0f,		1.0f, 1.0f, 0.0f,
		nR*sin(1.0*pi), nR*cos(1.0*pi), 0.0f,	0.5f, 0.5f, 0.0f,
		R*sin(1.2*pi), R*cos(1.2*pi), 0.0f,		1.0f, 1.0f, 0.0f,
		nR*sin(1.4*pi), nR*cos(1.4*pi), 0.0f,	0.5f, 0.5f, 0.0f,
		R*sin(1.6*pi), R*cos(1.6*pi), 0.0f,		1.0f, 1.0f, 0.0f,
		nR*sin(1.8*pi), nR*cos(1.8*pi), 0.0f,	0.5f, 0.5f, 0.0f,

	};
	unsigned int indices[] = { // 注意索引从0开始! 
	0, 1, 2, // 第一个三角形
	0,1,10,
	0,3,2,
	0,3,4,
	0,5,4,
	0,5,6,
	0,7,6,
	0,7,8,
	0,9,8,
	0,9,10,
	};
	
	unsigned int VAO,VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &EBO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	Shader ShaderA(vertexShaderSource, fragmentShaderSource);
	ShaderA.use();
	
	SDL_Event event;
	bool is_open = true;
	while (is_open)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				is_open = false;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event) {
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_MAXIMIZED:
				case SDL_WINDOWEVENT_RESTORED:
				case SDL_WINDOWEVENT_MINIMIZED:
					OnResize();
					break;
				case SDL_WINDOWEVENT_CLOSE:
					is_open = false;
					break;
				}
				break;
			}
		}
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ShaderA.setFloat("offsetX", cos(SDL_GetTicks()/500.0)/2.0f);

		glDrawElements(GL_TRIANGLES, 30, GL_UNSIGNED_INT, 0);
		SDL_GL_SwapWindow(window);

	}
	return 0;
}
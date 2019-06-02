#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <cmath>
#include<LearningOpenGL/Camera.h>
const float pi = acos(-1);
const float R = 0.75;
const float nR = R * cos(0.4*pi) / cos(0.2*pi);
SDL_Window *window;
SDL_GLContext GLContext;
int GL_width = 600, GL_height = 600;
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
unsigned int deltaTime = 0;
unsigned int lastFrame = 0;
const unsigned int FPS = 1000 / 120;
void OnResize()
{
	SDL_GetWindowSize(window, &GL_width, &GL_height);
	glViewport(0, 0, GL_width, GL_height);
}


static const char * vertexShaderSource= R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;
out vec3 ourColor;
out vec2 TexCoord;
uniform mat4 model;
uniform mat4 proj;
uniform mat4 view;
void main()
{
	gl_Position = proj * view * model * vec4(aPos,1.0);
	ourColor = aColor;
	TexCoord = aTexCoord;
})";

static const char* fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
in vec3 ourColor;
in vec2 TexCoord;
uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;
uniform bool useTexture;
uniform bool setBackground;
uniform float offsetTrans;
void main()
{
	if (useTexture)
	FragColor = mix(texture(ourTexture1,TexCoord),texture(ourTexture2,TexCoord),offsetTrans) ;
	else
	FragColor = vec4(ourColor,1.0f);
	if (setBackground)
	FragColor = vec4(offsetTrans,offsetTrans,offsetTrans,1.0f);
})";
void InitTexture()
{
	unsigned int texture[2];
	glGenTextures(2, texture);
	int width, height, nrChannels;
	unsigned char *data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_set_flip_vertically_on_load(true);
	data = stbi_load("awesomeface.png", &width, &height, &nrChannels, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture[1]);


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);
}
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
	void setInt(const char *s, int val)
	{
		glUniform1i(glGetUniformLocation(ID, s), val);
	}
	void setBool(const char *s, bool val)
	{
		glUniform1i(glGetUniformLocation(ID, s), int(val));
	}
	void setMat4(const char *s, glm::mat4 val)
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, s), 1, GL_FALSE, glm::value_ptr(val));
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
	SDL_SetRelativeMouseMode(SDL_TRUE);

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
	}
	
	float vertices[] = {
		0.0f,0.0f,0.3f,							1.0f, 1.0f, 0.0f,	0.0f,0.0f,
		R*sin(0.0*pi), R*cos(0.0*pi), 0.0f,		1.0f, 1.0f, 0.0f,	1.0f,1.0f,
		nR*sin(0.2*pi), nR*cos(0.2*pi), 0.0f,	0.5f, 0.5f, 0.0f,	0.0f,1.0f,
		R*sin(0.4*pi), R*cos(0.4*pi), 0.0f,		1.0f, 1.0f, 0.0f,	1.0f,1.0f,
		nR*sin(0.6*pi), nR*cos(0.6*pi), 0.0f,	0.5f, 0.5f, 0.0f,	0.0f,1.0f,
		R*sin(0.8*pi), R*cos(0.8*pi), 0.0f,		1.0f, 1.0f, 0.0f,	1.0f,1.0f,
		nR*sin(1.0*pi), nR*cos(1.0*pi), 0.0f,	0.5f, 0.5f, 0.0f,	0.0f,1.0f,
		R*sin(1.2*pi), R*cos(1.2*pi), 0.0f,		1.0f, 1.0f, 0.0f,	1.0f,1.0f,
		nR*sin(1.4*pi), nR*cos(1.4*pi), 0.0f,	0.5f, 0.5f, 0.0f,	0.0f,1.0f,
		R*sin(1.6*pi), R*cos(1.6*pi), 0.0f,		1.0f, 1.0f, 0.0f,	1.0f,1.0f,
		nR*sin(1.8*pi), nR*cos(1.8*pi), 0.0f,	0.5f, 0.5f, 0.0f,	0.0f,1.0f,

	};
	unsigned int indices[] = 
	{ // 注意索引从0开始! 
	0,1,2, // 第一个三角形
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

	float vertices2[] = 
	{
	0.0f,0.0f,0.0f,							
	R*sin(0.0*pi), R*cos(0.0*pi), 0.0f,		
	nR*sin(0.2*pi), nR*cos(0.2*pi), 0.0f,	
	R*sin(0.4*pi), R*cos(0.4*pi), 0.0f,		
	nR*sin(0.6*pi), nR*cos(0.6*pi), 0.0f,	
	R*sin(0.8*pi), R*cos(0.8*pi), 0.0f,		
	nR*sin(1.0*pi), nR*cos(1.0*pi), 0.0f,	
	R*sin(1.2*pi), R*cos(1.2*pi), 0.0f,		
	nR*sin(1.4*pi), nR*cos(1.4*pi), 0.0f,	
	R*sin(1.6*pi), R*cos(1.6*pi), 0.0f,		
	nR*sin(1.8*pi), nR*cos(1.8*pi), 0.0f,	

	};
	unsigned int VAO[2], VBO[2], EBO;
	glGenVertexArrays(2, VAO);
	glGenBuffers(1, &EBO);
	glGenBuffers(2, VBO);


	//绑定
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindVertexArray(VAO[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
	glBindVertexArray(VAO[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	InitTexture();

	Shader ShaderA(vertexShaderSource, fragmentShaderSource);
	ShaderA.use();
	ShaderA.setInt("ourTexture1", 0);
	ShaderA.setInt("ourTexture2", 1);

	glEnable(GL_DEPTH_TEST);
	SDL_Event event;
	bool is_open = true;
	bool useTexture = true;
	float offsetTrans = 0.0f; //材质占比
	float RotateAnglePre_z = 0.0f; //绕原点旋转角度

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
			case SDL_KEYDOWN:
				switch (event.key.keysym.scancode) {
					//调节材质占比 （线性插值）
				case SDL_SCANCODE_F11:
				if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP)
					SDL_SetWindowFullscreen(window, 0);
				else
					SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP);
					break;
				case SDL_SCANCODE_I:
					offsetTrans = (offsetTrans + 0.01) > 1.0 ? 1.0 : (offsetTrans + 0.01);
					break;
				case SDL_SCANCODE_K:
					offsetTrans = (offsetTrans - 0.01) < 0.0 ? 0.0 : (offsetTrans - 0.01);
					break;
					// 绕中心旋转
				case SDL_SCANCODE_L:
					RotateAnglePre_z -= 0.01;
					break;
				case SDL_SCANCODE_J:
					RotateAnglePre_z += 0.01;
					break;
					//调节镜头位置
				case SDL_SCANCODE_D:
					camera.ProcessKeyboard(RIGHT,FPS);
					break;
				case SDL_SCANCODE_A:
					camera.ProcessKeyboard(LEFT,FPS);
					break;
				case SDL_SCANCODE_S:
					camera.ProcessKeyboard(BACKWARD,FPS);
					break;
				case SDL_SCANCODE_W:
					camera.ProcessKeyboard(FORWARD,FPS);
					break;

				case SDL_SCANCODE_SPACE:
					useTexture = !useTexture;
					break;
				case SDL_SCANCODE_ESCAPE:
					is_open = false;
					break;				
					}
				break;
			case SDL_MOUSEMOTION:
				camera.ProcessMouseMotion(event.motion.xrel, event.motion.yrel);
				break;
			}

		}
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		ShaderA.setFloat("offsetTrans", offsetTrans);
		glm::mat4 model(1.0f);
		glm::mat4 proj(1.0f);
		glm::mat4 view(1.0f);
		view = camera.GetViewMatrix();
		model = glm::rotate(model, RotateAnglePre_z * 2 * pi, glm::vec3(0.0f,0.0f,1.0f));
		proj = glm::perspective(glm::radians(45.0f), (float) GL_width/ (float)GL_height, 0.1f, 100.0f);
		ShaderA.setMat4("model", model);
		ShaderA.setMat4("proj", proj);
		ShaderA.setMat4("view", view);

		
		ShaderA.setBool("setBackground", 1);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(VAO[1]);
		glDrawElements(GL_TRIANGLES, 30, GL_UNSIGNED_INT, 0);
		

		ShaderA.setBool("setBackground", 0);
		ShaderA.setBool("useTexture", useTexture);
		glBindVertexArray(VAO[0]);
		glDrawElements(GL_TRIANGLES, 30, GL_UNSIGNED_INT, 0);


		SDL_GL_SwapWindow(window);

		unsigned int currentFrame = SDL_GetTicks();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
		if (deltaTime < FPS)
			SDL_Delay(FPS - deltaTime);

	}
	SDL_GL_DeleteContext(GLContext);
	return 0;
}
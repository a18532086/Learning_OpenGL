#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <LearningOpenGL/Shader.h>
#include <LearningOpenGL/Camera.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
SDL_Window *window;
SDL_GLContext GLContext;
int GL_width = 600, GL_height = 600;
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
unsigned int deltaTime = 0;
unsigned int lastFrame = 0;
const unsigned int FPS = 1000 / 60;
static const char *objvs = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 proj;
uniform mat4 view;

void main()
{
	FragPos = vec3(model * vec4(aPos,1.0));
	Normal = mat3(transpose(inverse(model))) * aNormal;
	TexCoords = aTexCoords;
	gl_Position = proj * view * model * vec4(aPos,1.0);
})";

static const char *objfs = R"(#version 330 core

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Light light;
uniform Material material;
out vec4 FragColor;

void main()
{
	// ambient
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;  
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;  
        
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
})";

static const char *lightvs = R"(#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 proj;
uniform mat4 view;
void main()
{
	gl_Position = proj * view * model * vec4(aPos,1.0);
})";

static const char *lightfs = R"(#version 330 core
out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0f);
})";

void InitTexture(const char* path,int ID)
{
	unsigned int texture;
	glGenTextures(1, &texture);
	int width, height, nrComponents;

	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);

	GLenum format;
	if (nrComponents == 1)
		format = GL_RED;
	else if (nrComponents == 3)
		format = GL_RGB;
	else if (nrComponents == 4)
		format = GL_RGBA;

	glActiveTexture(GL_TEXTURE0 + ID);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);
}

void OnResize()
{
	SDL_GetWindowSize(window, &GL_width, &GL_height);
	glViewport(0, 0, GL_width, GL_height);
}

int main(int argc,char* argv[])
{
    SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow("MyOpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 600, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	GLContext = SDL_GL_CreateContext(window);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_SetRelativeMouseMode(SDL_TRUE);

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };
	unsigned int VAO, VBO, lightVAO;
    glGenVertexArrays(1,&VAO);
    glGenVertexArrays(1,&lightVAO);
    glGenBuffers(1,&VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);
	SDL_Event event;
	bool is_open = true;
	glm::vec3 SRC_lightPos(1.2f, 1.0f, 2.0f);
	InitTexture("container2.png", 0);
	InitTexture("container2_specular.png", 1);

	Shader objShader(objvs, objfs), lightShader(lightvs, lightfs);
    objShader.use();
	objShader.setInt("material.diffuse",  0);
	objShader.setInt("material.specular", 1);
	objShader.setFloat("material.shininess", 64.0f);
	objShader.setVec3("light.ambient",  glm::vec3(0.1f, 0.1f, 0.1f));
	objShader.setVec3("light.diffuse",  glm::vec3(0.5f, 0.5f, 0.5f)); // 将光照调暗了一些以搭配场景
	objShader.setVec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));




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
				switch (event.window.event)
				{
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
				switch (event.key.keysym.scancode)
				{
					//调节材质占比 （线性插值）
				case SDL_SCANCODE_F11:
					if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP)
						SDL_SetWindowFullscreen(window, 0);
					else
						SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
					break;
					//调节镜头位置
				case SDL_SCANCODE_D:
					camera.ProcessKeyboard(RIGHT, FPS);
					break;
				case SDL_SCANCODE_A:
					camera.ProcessKeyboard(LEFT, FPS);
					break;
				case SDL_SCANCODE_S:
					camera.ProcessKeyboard(BACKWARD, FPS);
					break;
				case SDL_SCANCODE_W:
					camera.ProcessKeyboard(FORWARD, FPS);
					break;
				case SDL_SCANCODE_ESCAPE:
					is_open = false;
					break;
				}
				break;
			case SDL_MOUSEMOTION:
				camera.ProcessMouseMotion(event.motion.xrel, event.motion.yrel);
				break;
			case SDL_MOUSEWHEEL:
				camera.ProcessMouseScroll(event.wheel.y);
				break;
			}
		}

		//GL缓存清除
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//M、V、P
        objShader.use();
		glm::mat4 model(1.0f);
		glm::mat4 proj(1.0f);
		glm::mat4 view(1.0f);
		view = camera.GetViewMatrix();
		proj = glm::perspective(glm::radians(camera.Zoom), (float)GL_width / (float)GL_height, 0.1f, 100.0f);
		glm::vec3 lightPos = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(0.1f*SDL_GetTicks()), glm::vec3(0.0f, 1.0f, 0.0f))) * SRC_lightPos;
		objShader.setVec3("viewPos",camera.Position);
		objShader.setVec3("light.position", lightPos);
		objShader.setMat4("model", model);
		objShader.setMat4("proj", proj);
		objShader.setMat4("view", view);

        glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		lightShader.use();
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f));
		
		lightShader.setMat4("model", model);
        lightShader.setMat4("view", view);
        lightShader.setMat4("proj", proj);
        glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);


        unsigned int currentFrame = SDL_GetTicks();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		if (deltaTime < FPS)
			SDL_Delay(FPS - deltaTime);
        SDL_GL_SwapWindow(window);
	}
	SDL_GL_DeleteContext(GLContext);
    return 0;
}
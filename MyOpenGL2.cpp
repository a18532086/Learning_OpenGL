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

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
}; 

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct SpotLight {
    vec3 position;
	vec3 direction;

    float constant;
    float linear;
    float quadratic;
	float cutOff;
	float outerCutOff;

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

const int MAX_POINTLIGHT = 4;

uniform vec3 viewPos;
uniform Material material;
uniform DirLight dirLight;
uniform PointLight pointLights[MAX_POINTLIGHT];
uniform SpotLight spotLight;
out vec4 FragColor;

vec3 CalcDirlight(DirLight light,vec3 normal,vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);
	//漫反射
	float diff = max(dot(lightDir,normal),0.0f);
	//镜面反射
	vec3 reflectDir = reflect(-lightDir,normal);
	float spec = pow(max(dot(reflectDir,viewDir),0.0f),material.shininess);
	//计算
	vec3 ambient = light.ambient * texture(material.diffuse,TexCoords).rgb;
	vec3 diffuse = light.diffuse * diff * texture(material.diffuse,TexCoords).rgb;
	vec3 specular = light.specular * spec * texture(material.specular,TexCoords).rgb; 

	return ambient + diffuse + specular;
}

vec3 CalcPointlight(PointLight light,vec3 normal,vec3 FragPos,vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - FragPos);
	//漫反射
	float diff = max(dot(lightDir,normal),0.0f);
	//镜面反射
	vec3 reflectDir = reflect(-lightDir,normal);
	float spec = pow(max(dot(reflectDir,viewDir),0.0f),material.shininess);
	//计算
	vec3 ambient = light.ambient * texture(material.diffuse,TexCoords).rgb;
	vec3 diffuse = light.diffuse * diff * texture(material.diffuse,TexCoords).rgb;
	vec3 specular = light.specular * spec * texture(material.specular,TexCoords).rgb; 
	//光衰减
	float distance = length(light.position - FragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	return ambient + diffuse + specular;
}
vec3 CalcSpotlight(SpotLight light,vec3 normal,vec3 FragPos,vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - FragPos);
	//漫反射
	float diff = max(dot(lightDir,normal),0.0f);
	//镜面反射
	vec3 reflectDir = reflect(-lightDir,normal);
	float spec = pow(max(dot(reflectDir,viewDir),0.0f),material.shininess);
	//计算
	vec3 ambient = light.ambient * texture(material.diffuse,TexCoords).rgb;
	vec3 diffuse = light.diffuse * diff * texture(material.diffuse,TexCoords).rgb;
	vec3 specular = light.specular * spec * texture(material.specular,TexCoords).rgb; 
	//光衰减
	float distance = length(light.position - FragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	//聚光
	float theta = dot(-lightDir,normalize(light.direction));
	float intensity = clamp ((theta - light.outerCutOff) / (light.cutOff - light.outerCutOff),0.0,1.0);
	diffuse *= intensity;
	specular *= intensity;

	return ambient + diffuse + specular;
}
void main()
{ 	
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 norm = normalize(Normal);
	vec3 result;

	//定向光
	result += CalcDirlight(dirLight,norm,viewDir);

	//点光源
	for (int i = 0;i < MAX_POINTLIGHT;i++)
	result += CalcPointlight(pointLights[i],norm,FragPos,viewDir);

	//聚光
	result += CalcSpotlight(spotLight,norm,FragPos,viewDir);

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
	glm::vec3 specular(1.0f, 1.0f, 1.0f);
	glm::vec3 diffuse = 0.5f * specular;
	glm::vec3 ambient = 0.5f * diffuse;
	glm::vec3 cubePositions[] = {
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(2.0f, 5.0f, -15.0f),
		glm::vec3(-1.5f, -2.2f, -2.5f),
		glm::vec3(-3.8f, -2.0f, -12.3f),
		glm::vec3(2.4f, -0.4f, -3.5f),
		glm::vec3(-1.7f, 3.0f, -7.5f),
		glm::vec3(1.3f, -2.0f, -2.5f),
		glm::vec3(1.5f, 2.0f, -2.5f),
		glm::vec3(1.5f, 0.2f, -1.5f),
		glm::vec3(-1.3f, 1.0f, -1.5f)};
	glm::vec3 pointLightPositions[] = {
        glm::vec3( 0.7f,  0.2f,  2.0f),
        glm::vec3( 2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };
	InitTexture("container2.png", 0);
	InitTexture("container2_specular.png", 1);

	Shader objShader(objvs, objfs), lightShader(lightvs, lightfs);
    objShader.use();
	objShader.setInt("material.diffuse",  0);
	objShader.setInt("material.specular", 1);
	objShader.setFloat("material.shininess", 64.0f);
	// directional light
	objShader.setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
	objShader.setVec3("dirLight.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
	objShader.setVec3("dirLight.diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
	objShader.setVec3("dirLight.specular", glm::vec3(0.5f, 0.5f, 0.5f));
	// point light 1
	objShader.setVec3("pointLights[0].position", pointLightPositions[0]);
	objShader.setVec3("pointLights[0].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
	objShader.setVec3("pointLights[0].diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
	objShader.setVec3("pointLights[0].specular", glm::vec3(1.0f, 1.0f, 1.0f));
	objShader.setFloat("pointLights[0].constant", 1.0f);
	objShader.setFloat("pointLights[0].linear", 0.09);
	objShader.setFloat("pointLights[0].quadratic", 0.032);
	// point light 2
	objShader.setVec3("pointLights[1].position", pointLightPositions[1]);
	objShader.setVec3("pointLights[1].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
	objShader.setVec3("pointLights[1].diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
	objShader.setVec3("pointLights[1].specular", glm::vec3(1.0f, 1.0f, 1.0f));
	objShader.setFloat("pointLights[1].constant", 1.0f);
	objShader.setFloat("pointLights[1].linear", 0.09);
	objShader.setFloat("pointLights[1].quadratic", 0.032);
	// point light 3
	objShader.setVec3("pointLights[2].position", pointLightPositions[2]);
	objShader.setVec3("pointLights[2].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
	objShader.setVec3("pointLights[2].diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
	objShader.setVec3("pointLights[2].specular", glm::vec3(1.0f, 1.0f, 1.0f));
	objShader.setFloat("pointLights[2].constant", 1.0f);
	objShader.setFloat("pointLights[2].linear", 0.09);
	objShader.setFloat("pointLights[2].quadratic", 0.032);
	// point light 4
	objShader.setVec3("pointLights[3].position", pointLightPositions[3]);
	objShader.setVec3("pointLights[3].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
	objShader.setVec3("pointLights[3].diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
	objShader.setVec3("pointLights[3].specular", glm::vec3(1.0f, 1.0f, 1.0f));
	objShader.setFloat("pointLights[3].constant", 1.0f);
	objShader.setFloat("pointLights[3].linear", 0.09);
	objShader.setFloat("pointLights[3].quadratic", 0.032);
	objShader.setVec3("spotLight.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
	objShader.setVec3("spotLight.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
	objShader.setVec3("spotLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));
	objShader.setFloat("spotLight.constant", 1.0f);
	objShader.setFloat("spotLight.linear", 0.09);
	objShader.setFloat("spotLight.quadratic", 0.032);
	objShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
	objShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

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
		objShader.setVec3("spotLight.position",camera.Position);
		objShader.setVec3("spotLight.direction",camera.Front);
		objShader.setMat4("model", model);
		objShader.setMat4("proj", proj);
		objShader.setMat4("view", view);

        glBindVertexArray(VAO);
		for (unsigned int i = 0; i < 10; i++)
		{
			model = glm::mat4(1.0f);
			model = glm::translate(model, cubePositions[i]);
			float angle = 20.0f * i;
			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
			objShader.setMat4("model", model);

			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		lightShader.use();
		
		lightShader.setMat4("model", model);
        lightShader.setMat4("view", view);
        lightShader.setMat4("proj", proj);
        glBindVertexArray(lightVAO);
		for (unsigned int i = 0; i < 4; i++)
		{
			model = glm::mat4(1.0f);
			model = glm::translate(model, pointLightPositions[i]);
			model = glm::scale(model, glm::vec3(0.1f));
			lightShader.setMat4("model", model);

			glDrawArrays(GL_TRIANGLES, 0, 36);
		}


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
#ifndef SHADER_H
#define SHADER_H
#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
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
	void setVec3(const char *s, glm::vec3 val)
	{
		glUniform3fv(glGetUniformLocation(ID, s), 1, glm::value_ptr(val));
	}
	void setMat4(const char *s, glm::mat4 val)
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, s), 1, GL_FALSE, glm::value_ptr(val));
	}
};
#endif
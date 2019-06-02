#ifndef CAMERA_H
#define CAMERA_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<iostream>
enum Camera_Movement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};
class Camera
{
public:
	//向量
	glm::vec3 Position;
	glm::vec3 SRC_Front;
	glm::vec3 Front;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	glm::vec3 Up;
	//欧拉角
	float Yaw;
	float Pitch;
    float Speed;
    float MoveSpeed;
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = 0.0f, float pitch = 0.0f,float speed = 0.01f,float movespeed = 0.001f) : Position(position), SRC_Front(glm::vec3(0.0f, 0.0f, -1.0f)), Yaw(yaw), Pitch(pitch), WorldUp(up),Speed(speed),MoveSpeed(movespeed)
	{
		updateCamera();
	}
	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(Position, Position + Front, Up);
	}
    void ProcessKeyboard(Camera_Movement direction,unsigned int FPS)
    {
        float FSpeed = Speed * FPS;
        switch (direction)
        {
        case FORWARD:
            Position += FSpeed * Front ;
            break;
        case BACKWARD:
            Position -= FSpeed * Front;
            break;
        case RIGHT:
            Position += FSpeed * Right;
            break;
        case LEFT:
            Position -= FSpeed * Right;
            break;
        }
        //Position.y = 0.0f;
    }
    void ProcessMouseMotion(int offsetX,int offsetY)
    {
        Yaw -= MoveSpeed * offsetX;
        Pitch -= MoveSpeed * offsetY;
        if (Pitch >= glm::radians(89.0f))
            Pitch = glm::radians(89.0f);
        else if (Pitch <= glm::radians(-89.0f))
            Pitch = glm::radians(-89.0f);
        updateCamera();
    }

private:
	void updateCamera()
	{
		glm::mat4 trans(1.0f);
		trans = glm::rotate(trans, Yaw, glm::vec3(0.0f, 1.0f, 0.0f));
		trans = glm::rotate(trans, Pitch, glm::vec3(1.0f, 0.0f, 0.0f));
		Front = glm::mat3(trans) * SRC_Front;
		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}
};
#endif
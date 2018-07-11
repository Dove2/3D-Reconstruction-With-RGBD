#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor; // ��ɫ����������λ��ֵΪ 1
out vec3 outColor; // ��Ƭ����ɫ�����һ����ɫ


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
	
	gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
	outColor = vec3(-aPos.z / 20); // ��ourColor����Ϊ���ǴӶ�����������õ���������ɫ
	//outColor = aColor;
}
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 nu; // normal upper
layout (location = 2) in vec3 nl; // normal left
layout (location = 3) in vec3 nd; // normal down
layout (location = 4) in vec3 nr; // normal right
layout (location = 5) in vec3 aColor;
out vec3 outColor;
out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
	
	gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
	outColor = vec3(-aPos.z / 20); // 将ourColor设置为我们从顶点数据那里得到的输入颜色

	vec3 upLeft = cross(nu, nl);
	vec3 downRight = cross(nd, nr);

	normal = normalize(upLeft * downRight);

	//outColor = normalize(aNormal);
}

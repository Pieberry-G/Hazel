// ---------------------------
// - Hazel 3D -
// RendererMX Shader
// ---------------------------

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;
uniform mat4 u_ModelMatrix;

void main()
{
	//v_EntityID = a_EntityID;
	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

uniform int u_EntityID;

layout(location = 0) out int o_EntityID;

void main()
{
	o_EntityID = u_EntityID;
}
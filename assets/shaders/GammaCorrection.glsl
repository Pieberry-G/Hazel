// Gamma Correction Shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 0) out vec2 v_TexCoord;

void main()
{
	gl_Position = vec4(a_Position, 1.0);
	v_TexCoord = a_TexCoord;
}

#type fragment
#version 450 core

layout(location = 0) in vec2 v_TexCoord;

layout(location = 0) out vec4 o_Color;

uniform sampler2D u_Texture;

const float gamma = 2.2;

void main()
{
	vec4 textureColor = texture(u_Texture, v_TexCoord);
	textureColor.rgb = pow(textureColor.rgb, vec3(1.0 / gamma));
	o_Color = textureColor;
}
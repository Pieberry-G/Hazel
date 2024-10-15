#include "Hazel/Renderer/Renderer3D.h"

#include "Hazel/Core/ResourceManager.h"
#include "Hazel/Renderer/FrameBuffer.h"

#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Hazel {

	struct SphereVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoord;

		// Editor-only
		int EntityID;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;

		// Editor-only
		int EntityID;
	};

	struct Renderer3DData
	{
		static const uint32_t MaxVertices = 100000;
		static const uint32_t MaxIndices = 100000;

		glm::mat4 ViewProjection;
		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;

		// IBL
		Ref<Shader> IBL_BackgroundShader;

		// Sphere
		Ref<Shader> SphereShader;
		Ref<VertexArray> SphereVertexArray;
		Ref<VertexBuffer> SphereVertexBuffer;
		Ref<IndexBuffer> SphereIndexBuffer;
		uint32_t SphereIndexCount = 0;
		SphereVertex* SphereVertexBufferBase = nullptr;
		SphereVertex* SphereVertexBufferPtr = nullptr;
		uint32_t* SphereIndexBufferBase = nullptr;
		uint32_t* SphereIndexBufferPtr = nullptr;

		// Line
		Ref<Shader> LineShader;
		Ref<VertexArray> LineVertexArray;
		Ref<VertexBuffer> LineVertexBuffer;
		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;
		float LineWidth = 2.0f;

		PbrMaterialTexture PbrTex;

		Renderer3D::Statistics Stats;
	};

	static Renderer3DData s_DataR3D;

	void Renderer3D::Init()
	{
		// IBL
		s_DataR3D.IBL_BackgroundShader = Shader::Create("../../assets/shaders/IBL_Background.glsl");

		// Sphere
		s_DataR3D.SphereShader = Shader::Create("../../assets/shaders/Renderer3D_Sphere.glsl");
		s_DataR3D.SphereVertexBufferBase = new SphereVertex[s_DataR3D.MaxVertices];
		s_DataR3D.SphereIndexBufferBase = new uint32_t[s_DataR3D.MaxIndices];

		s_DataR3D.SphereShader->Bind();

		s_DataR3D.SphereShader->SetInt("irradianceMap", 0);
		s_DataR3D.SphereShader->SetInt("prefilterMap", 1);
		s_DataR3D.SphereShader->SetInt("brdfLUT", 2);

		s_DataR3D.SphereShader->SetInt("u_AlbedoMap", 3);
		s_DataR3D.SphereShader->SetInt("u_NormalMap", 4);
		s_DataR3D.SphereShader->SetInt("u_MetallicMap", 5);
		s_DataR3D.SphereShader->SetInt("u_RoughnessMap", 6);
		s_DataR3D.SphereShader->SetInt("u_AoMap", 7);

		// Lines
		s_DataR3D.LineShader = Shader::Create("../../assets/shaders/Renderer3D_Line.glsl");
		s_DataR3D.LineVertexBufferBase = new LineVertex[s_DataR3D.MaxVertices];
	}

	void Renderer3D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		glm::mat4 viewProj = camera.GetProjection() * glm::inverse(transform);

		s_DataR3D.SphereShader->Bind();
		s_DataR3D.SphereShader->SetMat4("u_ViewProjection", viewProj);

		s_DataR3D.LineShader->Bind();
		s_DataR3D.LineShader->SetMat4("u_ViewProjection", viewProj);

		StartBatch();
	}

	void Renderer3D::BeginScene(const EditorCamera& camera)
	{
		RenderCommand::DisableDepthTest();
		DrawIBLBackground(camera);
		RenderCommand::EnableDepthTest();

		glm::mat4 viewProj = camera.GetViewProjection();
		glm::vec3 camPos = camera.GetPosition();

		s_DataR3D.ViewMatrix = camera.GetViewMatrix();
		s_DataR3D.ProjectionMatrix = camera.GetProjection();
		s_DataR3D.ViewProjection = camera.GetViewProjection();

		s_DataR3D.SphereShader->Bind();
		s_DataR3D.SphereShader->SetMat4("u_ViewProjection", viewProj);
		s_DataR3D.SphereShader->SetFloat3("u_CamPos", camPos);

		s_DataR3D.LineShader->Bind();
		s_DataR3D.LineShader->SetMat4("u_ViewProjection", viewProj);
		s_DataR3D.LineShader->SetFloat3("u_CamPos", camPos);

		StartBatch();
	}

	void Renderer3D::EndScene()
	{
		Flush();
	}

	void Renderer3D::StartBatch()
	{
		s_DataR3D.SphereIndexCount = 0;
		s_DataR3D.SphereVertexBufferPtr = s_DataR3D.SphereVertexBufferBase;
		s_DataR3D.SphereIndexBufferPtr = s_DataR3D.SphereIndexBufferBase;

		s_DataR3D.LineVertexCount = 0;
		s_DataR3D.LineVertexBufferPtr = s_DataR3D.LineVertexBufferBase;
	}

	void Renderer3D::Flush()
	{
		if(s_DataR3D.SphereIndexCount)
		{
			// VAO
			s_DataR3D.SphereVertexArray = VertexArray::Create();
			// VBO
			uint32_t dataSize = (uint8_t*)s_DataR3D.SphereVertexBufferPtr - (uint8_t*)s_DataR3D.SphereVertexBufferBase;
			s_DataR3D.SphereVertexBuffer = VertexBuffer::Create(s_DataR3D.SphereVertexBufferBase, dataSize);
			s_DataR3D.SphereVertexBuffer->SetLayout({
				{ ShaderDataType::Float3, "a_Position"	},
				{ ShaderDataType::Float3, "a_Normal"	},
				{ ShaderDataType::Float2, "a_TexCoord"	},
				{ ShaderDataType::Int,	  "a_EntityID"	},
			});
			s_DataR3D.SphereVertexArray->AddVertexBuffer(s_DataR3D.SphereVertexBuffer);
			// IBO
			s_DataR3D.SphereIndexBuffer =  IndexBuffer::Create(s_DataR3D.SphereIndexBufferBase, s_DataR3D.SphereIndexCount);
			s_DataR3D.SphereVertexArray->SetIndexBuffer(s_DataR3D.SphereIndexBuffer);

			// Bind textures
			ResourceManager::Get()->GetCubeTexture("IrradianceMap")->Bind(0);
			ResourceManager::Get()->GetCubeTexture("PrefilterMap")->Bind(1);
			ResourceManager::Get()->Get2DTexture("BrdfLUTTexture")->Bind(2);

			if(s_DataR3D.PbrTex.AlbedoMap)
				s_DataR3D.PbrTex.AlbedoMap->Bind(3);
			if (s_DataR3D.PbrTex.NormalMap)
				s_DataR3D.PbrTex.NormalMap->Bind(4);
			if (s_DataR3D.PbrTex.MetallicMap)
				s_DataR3D.PbrTex.MetallicMap->Bind(5);
			if (s_DataR3D.PbrTex.RoughnessMap)
				s_DataR3D.PbrTex.RoughnessMap->Bind(6);
			if (s_DataR3D.PbrTex.AoMap)
				s_DataR3D.PbrTex.AoMap->Bind(7);

			s_DataR3D.SphereShader->Bind();
			RenderCommand::DrawIndexed(s_DataR3D.SphereVertexArray, s_DataR3D.SphereIndexCount);
			s_DataR3D.Stats.DrawCalls++;
		}

		if (s_DataR3D.LineVertexCount)
		{
			// VAO
			s_DataR3D.LineVertexArray = VertexArray::Create();
			// VBO
			uint32_t dataSize = (uint8_t*)s_DataR3D.LineVertexBufferPtr - (uint8_t*)s_DataR3D.LineVertexBufferBase;
			s_DataR3D.LineVertexBuffer = VertexBuffer::Create(s_DataR3D.LineVertexBufferBase, dataSize);
			s_DataR3D.LineVertexBuffer->SetLayout({
				{ ShaderDataType::Float3, "a_Position"	},
				{ ShaderDataType::Float4, "a_Color"		},
				{ ShaderDataType::Int,	  "a_EntityID"	},
			});
			s_DataR3D.LineVertexArray->AddVertexBuffer(s_DataR3D.LineVertexBuffer);

			s_DataR3D.LineShader->Bind();
			RenderCommand::SetLineWidth(s_DataR3D.LineWidth);
			RenderCommand::DrawLines(s_DataR3D.LineVertexArray, s_DataR3D.LineVertexCount);
			s_DataR3D.Stats.DrawCalls++;
		}
	}

	void Renderer3D::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Renderer3D::DrawSphere(const glm::vec3& position, float radius, const PbrMaterial& material, LightParams lightParams)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { radius, radius, radius });

		DrawSphere(transform, material, lightParams);
	}

	void Renderer3D::DrawSphere(const glm::vec3& position, float radius, PbrMaterialTexture pbrTexture, LightParams lightParams)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { radius, radius, radius });

		DrawSphere(transform, pbrTexture, lightParams);
	}

	void Renderer3D::DrawSphere(const glm::mat4& transform, const PbrMaterial& material, LightParams lightParams, int entityID)
	{
		const float textureIndex = 0.0f; // White Texture;

		const uint32_t xTotalSegments = 64;
		const uint32_t yTotalSegments = 64;
		constexpr float pi = glm::pi<float>();

		s_DataR3D.SphereShader->Bind();
		s_DataR3D.SphereShader->SetMat4("u_ModelMatrix", transform);
		s_DataR3D.SphereShader->SetMat4("u_NormalMatrix", glm::transpose(glm::inverse(glm::mat3(transform))));

		s_DataR3D.SphereShader->SetInt("u_UseTexture", 0);
		s_DataR3D.SphereShader->SetFloat3("u_Albedo", material.Albedo);
		s_DataR3D.SphereShader->SetFloat("u_Metallic", material.Metallic);
		s_DataR3D.SphereShader->SetFloat("u_Roughness", material.Roughness);
		s_DataR3D.SphereShader->SetFloat("u_Ao", material.Ao);

		int pointLightNum = lightParams.PointLightPositions.size();
		s_DataR3D.SphereShader->SetInt("u_PointLightNum", pointLightNum);
		if (pointLightNum > 0)
		{
			s_DataR3D.SphereShader->SetFloat3Array("u_PointLightPositions", glm::value_ptr(lightParams.PointLightPositions[0]), pointLightNum);
			s_DataR3D.SphereShader->SetFloat3Array("u_PointLightColors", glm::value_ptr(lightParams.PointLightColors[0]), pointLightNum);
		}

		for (uint32_t x = 0; x <= xTotalSegments; x++)
		{
			for (uint32_t y = 0; y <= yTotalSegments; y++)
			{
				float xSegment = (float)x / (float)xTotalSegments;
				float ySegment = (float)y / (float)yTotalSegments;
				float xPos = std::cos(xSegment * 2.0f * pi) * std::sin(ySegment * pi);
				float yPos = std::cos(ySegment * pi);
				float zPos = std::sin(xSegment * 2.0f * pi) * std::sin(ySegment * pi);

				s_DataR3D.SphereVertexBufferPtr->Position = glm::vec3(xPos, yPos, zPos);
				s_DataR3D.SphereVertexBufferPtr->Normal = glm::vec3(xPos, yPos, zPos);
				s_DataR3D.SphereVertexBufferPtr->TexCoord = glm::vec2(xSegment, ySegment);
				s_DataR3D.SphereVertexBufferPtr->EntityID = entityID;
				s_DataR3D.SphereVertexBufferPtr++;
			}
		}

		for (uint32_t y = 0; y < yTotalSegments; y++)
		{
			for (uint32_t x = 0; x < xTotalSegments; x++)
			{
				*s_DataR3D.SphereIndexBufferPtr++ = y * (xTotalSegments + 1) + x;
				*s_DataR3D.SphereIndexBufferPtr++ = (y + 1) * (xTotalSegments + 1) + x;
				*s_DataR3D.SphereIndexBufferPtr++ = (y + 1) * (xTotalSegments + 1) + x + 1;

				*s_DataR3D.SphereIndexBufferPtr++ = y * (xTotalSegments + 1) + x;
				*s_DataR3D.SphereIndexBufferPtr++ = (y + 1) * (xTotalSegments + 1) + x + 1;
				*s_DataR3D.SphereIndexBufferPtr++ = y * (xTotalSegments + 1) + x + 1;
			}
		}
		s_DataR3D.SphereIndexCount = s_DataR3D.SphereIndexBufferPtr - s_DataR3D.SphereIndexBufferBase;
		s_DataR3D.Stats.SphereCount++;
		NextBatch();
	}

	void Renderer3D::DrawSphere(const glm::mat4& transform, PbrMaterialTexture pbrTexture, LightParams lightParams, int entityID)
	{
		float textureIndex = 0.0f;

		const uint32_t xTotalSegments = 64;
		const uint32_t yTotalSegments = 64;
		constexpr float pi = glm::pi<float>();

		s_DataR3D.SphereShader->Bind();
		s_DataR3D.SphereShader->SetMat4("u_ModelMatrix", transform);
		s_DataR3D.SphereShader->SetMat4("u_NormalMatrix", glm::transpose(glm::inverse(glm::mat3(transform))));
		s_DataR3D.SphereShader->SetInt("u_UseTexture", 1);

		int pointLightNum = lightParams.PointLightPositions.size();
		s_DataR3D.SphereShader->SetInt("u_PointLightNum", pointLightNum);
		if (pointLightNum > 0)
		{
			s_DataR3D.SphereShader->SetFloat3Array("u_PointLightPositions", glm::value_ptr(lightParams.PointLightPositions[0]), pointLightNum);
			s_DataR3D.SphereShader->SetFloat3Array("u_PointLightColors", glm::value_ptr(lightParams.PointLightColors[0]), pointLightNum);
		}

		s_DataR3D.PbrTex = pbrTexture;

		for (uint32_t x = 0; x <= xTotalSegments; x++)
		{
			for (uint32_t y = 0; y <= yTotalSegments; y++)
			{
				float xSegment = (float)x / (float)xTotalSegments;
				float ySegment = (float)y / (float)yTotalSegments;
				float xPos = std::cos(xSegment * 2.0f * pi) * std::sin(ySegment * pi);
				float yPos = std::cos(ySegment * pi);
				float zPos = std::sin(xSegment * 2.0f * pi) * std::sin(ySegment * pi);

				s_DataR3D.SphereVertexBufferPtr->Position = glm::vec3(xPos, yPos, zPos);
				s_DataR3D.SphereVertexBufferPtr->Normal = glm::vec3(xPos, yPos, zPos);
				s_DataR3D.SphereVertexBufferPtr->TexCoord = glm::vec2(xSegment, ySegment);
				s_DataR3D.SphereVertexBufferPtr->EntityID = entityID;
				s_DataR3D.SphereVertexBufferPtr++;
			}
		}

		for (uint32_t y = 0; y < yTotalSegments; y++)
		{
			for (uint32_t x = 0; x < xTotalSegments; x++)
			{
				*s_DataR3D.SphereIndexBufferPtr++ = y * (xTotalSegments + 1) + x;
				*s_DataR3D.SphereIndexBufferPtr++ = (y + 1) * (xTotalSegments + 1) + x;
				*s_DataR3D.SphereIndexBufferPtr++ = (y + 1) * (xTotalSegments + 1) + x + 1;

				*s_DataR3D.SphereIndexBufferPtr++ = y * (xTotalSegments + 1) + x;
				*s_DataR3D.SphereIndexBufferPtr++ = (y + 1) * (xTotalSegments + 1) + x + 1;
				*s_DataR3D.SphereIndexBufferPtr++ = y * (xTotalSegments + 1) + x + 1;
			}
		}
		s_DataR3D.SphereIndexCount = s_DataR3D.SphereIndexBufferPtr - s_DataR3D.SphereIndexBufferBase;
		s_DataR3D.Stats.SphereCount++;
		NextBatch();
	}

	void Renderer3D::DrawSphere(const glm::mat4& transform, SphereRendererComponent& src, LightParams lightParams, int entityID)
	{
		if (src.MaterialTexture.isComplete())
			DrawSphere(transform, src.MaterialTexture, lightParams, entityID);
		else
			DrawSphere(transform, src.Material, lightParams, entityID);
	}

	void Renderer3D::DrawLines(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID)
	{
		s_DataR3D.LineVertexBufferPtr->Position = p0;
		s_DataR3D.LineVertexBufferPtr->Color = color;
		s_DataR3D.LineVertexBufferPtr->EntityID = entityID;
		s_DataR3D.LineVertexBufferPtr++;

		s_DataR3D.LineVertexBufferPtr->Position = p1;
		s_DataR3D.LineVertexBufferPtr->Color = color;
		s_DataR3D.LineVertexBufferPtr->EntityID = entityID;
		s_DataR3D.LineVertexBufferPtr++;

		s_DataR3D.LineVertexCount += 2;
	}

	float Renderer3D::GetLineWidth()
	{
		return s_DataR3D.LineWidth;
	}

	void Renderer3D::SetLineWidth(float width)
	{
		s_DataR3D.LineWidth = width;
	}

	static void RenderCube()
	{
		// initialize (if necessary)
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};

		// VAO
		Ref<VertexArray> cubeVAO = VertexArray::Create();
		// VBO
		Ref<VertexBuffer> cubeVBO = VertexBuffer::Create(vertices, sizeof(vertices));
		cubeVBO->SetLayout({
			{ ShaderDataType::Float3, "aPos"	},
			{ ShaderDataType::Float3, "aNormal"	},
			{ ShaderDataType::Float2, "aTexCoord"	},
			});
		cubeVAO->AddVertexBuffer(cubeVBO);

		// render Cube
		RenderCommand::DrawArrays(cubeVAO, 36);
	}

	void Renderer3D::DrawIBLBackground(const EditorCamera& camera)
	{
		s_DataR3D.IBL_BackgroundShader->Bind();
		s_DataR3D.IBL_BackgroundShader->SetMat4("projection", camera.GetProjection());
		s_DataR3D.IBL_BackgroundShader->SetMat4("view", camera.GetViewMatrix());
		s_DataR3D.IBL_BackgroundShader->SetInt("environmentMap", 0);
		ResourceManager::Get()->GetCubeTexture("EnvCubeMap")->Bind();
		RenderCube();
	}

	void Renderer3D::DrawGroundPlane(int rows, int cols, float spacing)
	{
		glm::vec4 color(0.8f);
		for (int i = 0; i <= rows; i++)
		{
			glm::vec3 p0 = { (i - rows / 2.0f) * spacing, 0.0f, -(cols / 2.0f) * spacing };
			glm::vec3 p1 = { (i - rows / 2.0f) * spacing, 0.0f, (cols / 2.0f) * spacing };
			DrawLines(p0, p1, color);
		}
		for (int j = 0; j <= cols; j++)
		{
			glm::vec3 p0 = { -(rows / 2.0f) * spacing, 0.0f, (j - cols / 2.0f) * spacing };
			glm::vec3 p1 = { (rows / 2.0f) * spacing, 0.0f, (j - cols / 2.0f) * spacing };
			DrawLines(p0, p1, color);
		}
	}

	void Renderer3D::ResetStats()
	{
		memset(&s_DataR3D.Stats, 0, sizeof(Statistics));
	}

	Renderer3D::Statistics Renderer3D::GetStats()
	{
		return s_DataR3D.Stats;
	}
}
#include "Hazel/Renderer/Renderer2D.h"

#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Hazel {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;

		// Editor-only
		int EntityID;
	};

	struct CircleVertex
	{
		glm::vec3 WorldPosition;
		glm::vec3 LocalPosition;
		glm::vec4 Color;
		float Thickness;
		float Fade;

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

	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> QuadShader;
		Ref<Texture> WhiteTexture;

		Ref<VertexArray> CircleVertexArray;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<Shader> CircleShader;

		Ref<VertexArray> LineVertexArray;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<Shader> LineShader;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		uint32_t CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPtr = nullptr;

		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;

		float LineWidth = 2.0f;

		std::array<Ref<Texture>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec4 QuadVertexPositions[4];

		Renderer2D::Statistics Stats;
	};

	static Renderer2DData s_DataR2D;

	void Renderer2D::Init()
	{
		// Quad
		s_DataR2D.QuadVertexArray = VertexArray::Create();

		s_DataR2D.QuadVertexBuffer = VertexBuffer::Create(s_DataR2D.MaxVertices * sizeof(QuadVertex));
		s_DataR2D.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position"	},
			{ ShaderDataType::Float4, "a_Color"		    },
			{ ShaderDataType::Float2, "a_TexCoord"	    },
			{ ShaderDataType::Float,  "a_TexIndex"	    },
			{ ShaderDataType::Float,  "a_TilingFactor"  },
			{ ShaderDataType::Int,	  "a_EntityID"	    },
			});
		s_DataR2D.QuadVertexArray->AddVertexBuffer(s_DataR2D.QuadVertexBuffer);

		s_DataR2D.QuadVertexBufferBase = new QuadVertex[s_DataR2D.MaxVertices];

		uint32_t* quadIndices = new uint32_t[s_DataR2D.MaxIndices];
		uint32_t offet = 0;
		for (uint32_t i = 0; i < s_DataR2D.MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offet + 0;
			quadIndices[i + 1] = offet + 1;
			quadIndices[i + 2] = offet + 2;
			quadIndices[i + 3] = offet + 2;
			quadIndices[i + 4] = offet + 3;
			quadIndices[i + 5] = offet + 0;

			offet += 4;
		}
		Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_DataR2D.MaxIndices);
		s_DataR2D.QuadVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		// Circles
		s_DataR2D.CircleVertexArray = VertexArray::Create();

		s_DataR2D.CircleVertexBuffer = VertexBuffer::Create(s_DataR2D.MaxVertices * sizeof(CircleVertex));
		s_DataR2D.CircleVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_WorldPosition"	},
			{ ShaderDataType::Float3, "a_LocalPosition"	},
			{ ShaderDataType::Float4, "a_Color"		},
			{ ShaderDataType::Float,  "a_Thickness"	},
			{ ShaderDataType::Float,  "a_Fade"		},
			{ ShaderDataType::Int,	  "a_EntityID"	},
			});
		s_DataR2D.CircleVertexArray->AddVertexBuffer(s_DataR2D.CircleVertexBuffer);
		s_DataR2D.CircleVertexArray->SetIndexBuffer(quadIB); // Use quad IB
		s_DataR2D.CircleVertexBufferBase = new CircleVertex[s_DataR2D.MaxVertices];

		// Lines
		s_DataR2D.LineVertexArray = VertexArray::Create();

		s_DataR2D.LineVertexBuffer = VertexBuffer::Create(s_DataR2D.MaxVertices * sizeof(LineVertex));
		s_DataR2D.LineVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position"	},
			{ ShaderDataType::Float4, "a_Color"	},
			{ ShaderDataType::Int,	  "a_EntityID"	},
			});
		s_DataR2D.LineVertexArray->AddVertexBuffer(s_DataR2D.LineVertexBuffer);
		s_DataR2D.LineVertexBufferBase = new LineVertex[s_DataR2D.MaxVertices];


		s_DataR2D.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_DataR2D.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		s_DataR2D.QuadShader = Shader::Create("../../assets/shaders/Renderer2D_Quad.glsl");
		s_DataR2D.CircleShader = Shader::Create("../../assets/shaders/Renderer2D_Circle.glsl");
		s_DataR2D.LineShader = Shader::Create("../../assets/shaders/Renderer2D_Line.glsl");

		int32_t samplers[s_DataR2D.MaxTextureSlots];
		for (uint32_t i = 0; i < 32; i++)
			samplers[i] = i;

		s_DataR2D.QuadShader->Bind();
		s_DataR2D.QuadShader->SetIntArray("u_Textures", samplers, s_DataR2D.MaxTextureSlots);

		// Set first texture slot to 0
		s_DataR2D.TextureSlots[0] = s_DataR2D.WhiteTexture;

		s_DataR2D.QuadVertexPositions[0] = { -0.5, -0.5, 0.0f, 1.0f };
		s_DataR2D.QuadVertexPositions[1] = { 0.5, -0.5, 0.0f, 1.0f };
		s_DataR2D.QuadVertexPositions[2] = { 0.5,  0.5, 0.0f, 1.0f };
		s_DataR2D.QuadVertexPositions[3] = { -0.5,  0.5, 0.0f, 1.0f };
	}

	void Renderer2D::Shutdown()
	{

	}

	void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		glm::mat4 viewProj = camera.GetProjection() * glm::inverse(transform);

		s_DataR2D.QuadShader->Bind();
		s_DataR2D.QuadShader->SetMat4("u_ViewProjection", viewProj);

		s_DataR2D.CircleShader->Bind();
		s_DataR2D.CircleShader->SetMat4("u_ViewProjection", viewProj);

		s_DataR2D.LineShader->Bind();
		s_DataR2D.LineShader->SetMat4("u_ViewProjection", viewProj);

		StartBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		glm::mat4 viewProj = camera.GetViewProjection();

		s_DataR2D.QuadShader->Bind();
		s_DataR2D.QuadShader->SetMat4("u_ViewProjection", viewProj);

		s_DataR2D.CircleShader->Bind();
		s_DataR2D.CircleShader->SetMat4("u_ViewProjection", viewProj);

		s_DataR2D.LineShader->Bind();
		s_DataR2D.LineShader->SetMat4("u_ViewProjection", viewProj);

		StartBatch();
	}

	void Renderer2D::EndScene()
	{
		Flush();
	}

	void Renderer2D::StartBatch()
	{
		s_DataR2D.QuadIndexCount = 0;
		s_DataR2D.QuadVertexBufferPtr = s_DataR2D.QuadVertexBufferBase;

		s_DataR2D.CircleIndexCount = 0;
		s_DataR2D.CircleVertexBufferPtr = s_DataR2D.CircleVertexBufferBase;

		s_DataR2D.LineVertexCount = 0;
		s_DataR2D.LineVertexBufferPtr = s_DataR2D.LineVertexBufferBase;

		s_DataR2D.TextureSlotIndex = 1;
	}

	void Renderer2D::Flush()
	{
		if (s_DataR2D.QuadIndexCount)
		{
			uint32_t dataSize = (uint8_t*)s_DataR2D.QuadVertexBufferPtr - (uint8_t*)s_DataR2D.QuadVertexBufferBase;
			s_DataR2D.QuadVertexBuffer->SetData(s_DataR2D.QuadVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_DataR2D.TextureSlotIndex; i++)
				s_DataR2D.TextureSlots[i]->Bind(i);

			s_DataR2D.QuadShader->Bind();
			RenderCommand::DrawIndexed(s_DataR2D.QuadVertexArray, s_DataR2D.QuadIndexCount);
			s_DataR2D.Stats.DrawCalls++;
		}

		if (s_DataR2D.CircleIndexCount)
		{
			uint32_t dataSize = (uint8_t*)s_DataR2D.CircleVertexBufferPtr - (uint8_t*)s_DataR2D.CircleVertexBufferBase;
			s_DataR2D.CircleVertexBuffer->SetData(s_DataR2D.CircleVertexBufferBase, dataSize);

			s_DataR2D.CircleShader->Bind();
			RenderCommand::DrawIndexed(s_DataR2D.CircleVertexArray, s_DataR2D.CircleIndexCount);
			s_DataR2D.Stats.DrawCalls++;
		}

		if (s_DataR2D.LineVertexCount)
		{
			uint32_t dataSize = (uint8_t*)s_DataR2D.LineVertexBufferPtr - (uint8_t*)s_DataR2D.LineVertexBufferBase;
			s_DataR2D.LineVertexBuffer->SetData(s_DataR2D.LineVertexBufferBase, dataSize);

			s_DataR2D.LineShader->Bind();
			RenderCommand::SetLineWidth(s_DataR2D.LineWidth);
			RenderCommand::DrawLines(s_DataR2D.LineVertexArray, s_DataR2D.LineVertexCount);
			s_DataR2D.Stats.DrawCalls++;
		}
	}

	void Renderer2D::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D> texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D> texture, float tilingFactor, const glm::vec4& tintColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		constexpr float textureIndex = 0.0f; // White Texture;
		constexpr float tilingFactor = 1.0f;

		if (s_DataR2D.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_DataR2D.QuadVertexBufferPtr->Position = transform * s_DataR2D.QuadVertexPositions[i];
			s_DataR2D.QuadVertexBufferPtr->Color = color;
			s_DataR2D.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_DataR2D.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_DataR2D.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_DataR2D.QuadVertexBufferPtr->EntityID = entityID;
			s_DataR2D.QuadVertexBufferPtr++;
		}

		s_DataR2D.QuadIndexCount += 6;

		s_DataR2D.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D> texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		float textureIndex = 0.0f;

		if (s_DataR2D.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (uint32_t i = 0; i < s_DataR2D.TextureSlotIndex; i++)
		{
			if (*s_DataR2D.TextureSlots[i] == *texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_DataR2D.TextureSlotIndex;
			s_DataR2D.TextureSlots[textureIndex] = texture;
			s_DataR2D.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_DataR2D.QuadVertexBufferPtr->Position = transform * s_DataR2D.QuadVertexPositions[i];
			s_DataR2D.QuadVertexBufferPtr->Color = tintColor;
			s_DataR2D.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_DataR2D.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_DataR2D.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_DataR2D.QuadVertexBufferPtr->EntityID = entityID;
			s_DataR2D.QuadVertexBufferPtr++;
		}

		s_DataR2D.QuadIndexCount += 6;

		s_DataR2D.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		constexpr float textureIndex = 0.0f; // White Texture;
		constexpr float tilingFactor = 1.0f;

		if (s_DataR2D.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_DataR2D.QuadVertexBufferPtr->Position = transform * s_DataR2D.QuadVertexPositions[i];
			s_DataR2D.QuadVertexBufferPtr->Color = color;
			s_DataR2D.QuadVertexBufferPtr->TexCoord = textureCoords[0];
			s_DataR2D.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_DataR2D.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_DataR2D.QuadVertexBufferPtr++;
		}

		s_DataR2D.QuadIndexCount += 6;

		s_DataR2D.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D> texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D> texture, float tilingFactor, const glm::vec4& tintColor)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		float textureIndex = 0.0f;

		if (s_DataR2D.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (uint32_t i = 0; i < s_DataR2D.TextureSlotIndex; i++)
		{
			if (*s_DataR2D.TextureSlots[i] == *texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_DataR2D.TextureSlotIndex;
			s_DataR2D.TextureSlots[textureIndex] = texture;
			s_DataR2D.TextureSlotIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_DataR2D.QuadVertexBufferPtr->Position = transform * s_DataR2D.QuadVertexPositions[i];
			s_DataR2D.QuadVertexBufferPtr->Color = tintColor;
			s_DataR2D.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_DataR2D.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_DataR2D.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_DataR2D.QuadVertexBufferPtr++;
		}

		s_DataR2D.QuadIndexCount += 6;

		s_DataR2D.Stats.QuadCount++;
	}

	void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness, float fade, int entityID)
	{
		// TODO: implement for circle
		//if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
		//	NextBatch();

		for (size_t i = 0; i < 4; i++)
		{
			s_DataR2D.CircleVertexBufferPtr->WorldPosition = transform * s_DataR2D.QuadVertexPositions[i];
			s_DataR2D.CircleVertexBufferPtr->LocalPosition = s_DataR2D.QuadVertexPositions[i] * 2.0f;
			s_DataR2D.CircleVertexBufferPtr->Color = color;
			s_DataR2D.CircleVertexBufferPtr->Thickness = thickness;
			s_DataR2D.CircleVertexBufferPtr->Fade = fade;
			s_DataR2D.CircleVertexBufferPtr->EntityID = entityID;
			s_DataR2D.CircleVertexBufferPtr++;
		}

		s_DataR2D.CircleIndexCount += 6;

		s_DataR2D.Stats.QuadCount++;
	}

	float Renderer2D::GetLineWidth()
	{
		return s_DataR2D.LineWidth;
	}

	void Renderer2D::SetLineWidth(float width)
	{
		s_DataR2D.LineWidth = width;
	}

	void Renderer2D::DrawLines(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID)
	{
		s_DataR2D.LineVertexBufferPtr->Position = p0;
		s_DataR2D.LineVertexBufferPtr->Color = color;
		s_DataR2D.LineVertexBufferPtr->EntityID = entityID;
		s_DataR2D.LineVertexBufferPtr++;

		s_DataR2D.LineVertexBufferPtr->Position = p1;
		s_DataR2D.LineVertexBufferPtr->Color = color;
		s_DataR2D.LineVertexBufferPtr->EntityID = entityID;
		s_DataR2D.LineVertexBufferPtr++;

		s_DataR2D.LineVertexCount += 2;
	}

	void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID)
	{
		glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
		glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

		DrawLines(p0, p1, color);
		DrawLines(p1, p2, color);
		DrawLines(p2, p3, color);
		DrawLines(p3, p0, color);
	}

	void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		glm::vec3 lineVertices[4];
		for (size_t i = 0; i < 4; i++)
			lineVertices[i] = transform * s_DataR2D.QuadVertexPositions[i];

		DrawLines(lineVertices[0], lineVertices[1], color);
		DrawLines(lineVertices[1], lineVertices[2], color);
		DrawLines(lineVertices[2], lineVertices[3], color);
		DrawLines(lineVertices[3], lineVertices[0], color);
	}

	void Renderer2D::DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID)
	{
		if (src.Texture)
			DrawQuad(transform, src.Texture, src.TilingFactor, src.Color, entityID);
		else
			DrawQuad(transform, src.Color, entityID);
	}

	void Renderer2D::ResetStats()
	{
		memset(&s_DataR2D.Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_DataR2D.Stats;
	}

}
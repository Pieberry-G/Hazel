#pragma once

#include "OrthographicCamera.h"

#include "Texture.h"

namespace Hazel {

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(OrthographicCamera& camera);
		static void EndScene();

		// Primitives
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture> texture);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture> texture);
	};
}
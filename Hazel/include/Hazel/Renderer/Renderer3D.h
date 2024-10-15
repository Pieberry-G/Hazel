#pragma once

#include "Hazel/Renderer/Material.h"
#include "Hazel/Renderer/Camera.h"
#include "Hazel/Renderer/EditorCamera.h"

#include "Hazel/Scene/Components.h"

namespace Hazel {

	struct LightParams
	{
		std::vector<glm::vec3> PointLightPositions;
		std::vector<glm::vec3> PointLightColors;
		glm::vec3 DirectionalLightDirection;
		glm::vec3 DirectionalLightColor;
		//IBLSettings iblSettings;            // IBL 设置  
		// 其他渲染参数，如材质列表、模型列表等  
	};

	class Renderer3D
	{
	public:
		static void Init();

		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void EndScene();
		static void StartBatch();
		static void Flush();

		// Primitives
		static void DrawSphere(const glm::vec3& position, float radius, const PbrMaterial& material, LightParams lightParams);
		static void DrawSphere(const glm::vec3& position, float radius,  PbrMaterialTexture pbrTexture, LightParams lightParams);
		
		static void DrawSphere(const glm::mat4& transform, const PbrMaterial& material, LightParams lightParams, int entityID = -1);
		static void DrawSphere(const glm::mat4& transform, PbrMaterialTexture pbrTexture, LightParams lightParams, int entityID = -1);
		
		static void DrawSphere(const glm::mat4& transform, SphereRendererComponent& src, LightParams lightParams, int entityID);

		static void DrawLines(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);
		static float GetLineWidth();
		static void SetLineWidth(float width);

		static void DrawIBLBackground(const EditorCamera& camera);
		static void DrawGroundPlane(int rows, int cols, float spacing = 1.0f);

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t SphereCount = 0;
		};
		static void ResetStats();
		static Statistics GetStats();
	private:
		static void NextBatch();
	};
}
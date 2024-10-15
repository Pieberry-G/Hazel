#pragma once

#include "Hazel/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Hazel {

	struct PbrMaterial
	{
		glm::vec3 Albedo = glm::vec3(1.0f);
		float Metallic = 0.0f;
		float Roughness = 1.0f;
		float Ao = 1.0f;

		PbrMaterial() = default;

		PbrMaterial(const glm::vec3& albedo, float metallic, float roughness, float ao)
			: Albedo(albedo), Metallic(metallic), Roughness(roughness), Ao(ao) {}
	};

	struct PbrMaterialTexture
	{
		Ref<Texture2D> AlbedoMap;
		Ref<Texture2D> NormalMap;
		Ref<Texture2D> MetallicMap;
		Ref<Texture2D> RoughnessMap;
		Ref<Texture2D> AoMap;

		PbrMaterialTexture() = default;

		PbrMaterialTexture(const std::string& pbrTexturePath, PbrTexImage& pbrTexImage = PbrTexImage())
		{
			AlbedoMap = Texture2D::Create(pbrTexturePath + "/albedo.png", pbrTexImage.images[0]);
			NormalMap = Texture2D::Create(pbrTexturePath + "/normal.png", pbrTexImage.images[1]);
			MetallicMap = Texture2D::Create(pbrTexturePath + "/metallic.png", pbrTexImage.images[2]);
			RoughnessMap = Texture2D::Create(pbrTexturePath + "/roughness.png", pbrTexImage.images[3]);
			AoMap = Texture2D::Create(pbrTexturePath + "/ao1.png", pbrTexImage.images[4]);
		}

		bool isComplete()
		{
			return AlbedoMap && NormalMap && MetallicMap && RoughnessMap && AoMap;
		}
	};
}
#include "Hazel/Core/ResourceManager.h"

namespace Hazel {

    ResourceManager* ResourceManager::s_Instance = nullptr;

	Ref<Texture2D> ResourceManager::Get2DTexture(const std::string& name)
	{
		auto it = m_2DTextures.find(name);
		if (it != m_2DTextures.end())
			return it->second;
		else
		{
			HZ_CORE_ERROR("2D texture resource not found!");
			return Ref<Texture2D>();
		}
	}

	ResourceManager::ResourceManager()
	{
		Preload2DTexResources();
	}

	void ResourceManager::Preload2DTexResources()
	{
		std::string texturePath = "../../assets/textures/";

		std::vector<std::string> texNames = { "Checkerboard", "ChernoLogo" };
		for (uint32_t i = 0; i < texNames.size(); i++)
			m_2DTextures[texNames[i]] = Texture2D::Create(texturePath + texNames[i] + ".png");
	}

}
#pragma once

#include "Hazel/Renderer/Texture.h"

namespace Hazel {

	class ResourceManager
	{
	public:
		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;

		static ResourceManager* Get() {
			if (s_Instance == nullptr) {
				s_Instance = new ResourceManager();
			}
			return s_Instance;
		}

		Ref<Texture2D> Get2DTexture(const std::string& name);
	private:
		ResourceManager();
		void Preload2DTexResources();

	private:
		std::unordered_map<std::string, Ref<Texture2D>> m_2DTextures;

	private:
		static ResourceManager* s_Instance;
	};

}
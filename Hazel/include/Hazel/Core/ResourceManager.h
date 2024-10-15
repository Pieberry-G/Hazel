#pragma once

#include "Hazel/Renderer/Material.h"

#include <future>

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

		PbrMaterialTexture GetPbrTexture(const std::string& name);
		Ref<Texture2D> Get2DTexture(const std::string& name);
		Ref<TextureCube> GetCubeTexture(const std::string& name);
	private:
		ResourceManager();
		void PreloadPbrTexResources();
		void Preload2DTexResources();
		void PrecomputeIBLTextures();

	private:
		std::unordered_map<std::string, PbrMaterialTexture> m_PbrTextures;

		std::unordered_map<std::string, Ref<Texture2D>> m_2DTextures;
		std::unordered_map<std::string, Ref<TextureCube>> m_CubeTextures;

		std::vector<std::future<void>> m_Futures;
	private:
		static ResourceManager* s_Instance;
	};

}
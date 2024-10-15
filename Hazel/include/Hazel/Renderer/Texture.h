#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/FrameBuffer.h"

#include <stb_image.h>

namespace Hazel {

	class StbImage
	{
	public:
		void LoadImage(const std::string& imagePath)
		{
			stbi_set_flip_vertically_on_load(1);
			Data = stbi_load(imagePath.c_str(), &Width, &Height, &Channels, 0);

			if (!Data)
				HZ_CORE_ERROR("Failed to load image!");
		}

		bool GetData(stbi_uc*& data, int& width, int& height, int& channels) const
		{
			if (!Data) return false;

			data = Data;
			width = Width;
			height = Height;
			channels = Channels;
		}

		void FreeImage()
		{
			stbi_image_free(Data);
			Data = nullptr;
		}
	private:
		stbi_uc* Data = nullptr;
		int Width, Height, Channels;
	};

	struct PbrTexImage
	{
		// 0:AlbedoMap  1:NormalMap  2:MetallicMap  3:RoughnessMap  4:AoMap
		StbImage images[5];
	};

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual void SetData(void* data, uint32_t size, uint32_t textureIndex = 0) = 0;
		virtual void SetDataFromFrameBuffer(const Ref<FrameBuffer>& frameBuffer, uint32_t textureIndex = 0, int level = 0) = 0;

		virtual void GenerateMipmaps() const = 0;
		
		virtual void Bind(uint32_t slot = 0, uint32_t textureIndex = 0) const = 0;

		virtual bool IsLoaded() const = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(uint32_t width, uint32_t height);
		static Ref<Texture2D> Create(const std::string& path, StbImage& stbImage = StbImage());
		static Ref<Texture2D> Create(const Ref<FrameBuffer>& frameBuffer);
		static Ref<Texture2D> CreateHdr(const std::string& hdrPath);
	};

	class TextureCube : public Texture
	{
	public:
		static Ref<TextureCube> Create(uint32_t width, uint32_t height);
	};

}
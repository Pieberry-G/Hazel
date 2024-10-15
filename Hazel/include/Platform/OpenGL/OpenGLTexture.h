#pragma once

#include "Hazel/Renderer/Texture.h"

#include <glad/glad.h>

namespace Hazel {

	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(uint32_t width, uint32_t height);
		OpenGLTexture2D(const std::string& path, StbImage& stbImage);
		OpenGLTexture2D(const Ref<FrameBuffer>& frameBuffer);
		OpenGLTexture2D(const std::string& hdrPath);
		virtual ~OpenGLTexture2D();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual void SetData(void* data, uint32_t size, uint32_t textureIndex = 0) override;
		virtual void SetDataFromFrameBuffer(const Ref<FrameBuffer>& frameBuffer, uint32_t textureIndex, int level) override;
	
		virtual void GenerateMipmaps() const override;

		virtual void Bind(uint32_t slot = 0, uint32_t textureIndex = 0) const override;

		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual bool operator==(const Texture& other) const override
		{
			return m_RendererID == ((OpenGLTexture2D&)other).m_RendererID;
		}
	private:
		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;
	};

	class OpenGLTextureCube : public TextureCube
	{
	public:
		OpenGLTextureCube(uint32_t width, uint32_t height);
		virtual ~OpenGLTextureCube();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual void SetData(void* data, uint32_t size, uint32_t textureIndex = 0) override;
		virtual void SetDataFromFrameBuffer(const Ref<FrameBuffer>& frameBuffer, uint32_t textureIndex, int level) override;

		virtual void GenerateMipmaps() const override;

		virtual void Bind(uint32_t slot = 0, uint32_t textureIndex = 0) const override;

		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual bool operator==(const Texture& other) const override
		{
			return m_RendererID == ((OpenGLTextureCube&)other).m_RendererID;
		}
	private:
		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;
	};
}
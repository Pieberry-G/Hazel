#include "Platform/OpenGL/OpenGLTexture.h"

#include <stb_image.h>

namespace Hazel {

	////////////////////////////////////////////////////////////////////////////
	// Texture2D ///////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
		m_InternalFormat = GL_RGBA8;
		m_DataFormat = GL_RGBA;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path, StbImage& stbImage)
		: m_Path(path)
	{
		stbi_uc* data;
		int width, height, channels;
		bool isPreloaded = stbImage.GetData(data, width, height, channels);

		if (!isPreloaded)
		{
			stbi_set_flip_vertically_on_load(1);
			data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		}

		if (data)
		{
			m_IsLoaded = true;

			m_Width = width;
			m_Height = height;

			GLenum internalFormat = 0, dataFormat = 0;
			if (channels == 4)
			{
				internalFormat = GL_RGBA8;
				dataFormat = GL_RGBA;
			}
			else if (channels == 3)
			{
				internalFormat = GL_RGB8;
				dataFormat = GL_RGB;
			}
			else if (channels == 1)
			{
				internalFormat = GL_R8;
				dataFormat = GL_RED;
			}

			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			HZ_CORE_ASSERT(internalFormat && dataFormat, "Format not supported!");

			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

			if (!isPreloaded)
				stbi_image_free(data);
		}
	}

	OpenGLTexture2D::OpenGLTexture2D(const Ref<FrameBuffer>& frameBuffer)
	{
		m_Width = frameBuffer->GetSpecification().Width;
		m_Height = frameBuffer->GetSpecification().Height;

		m_InternalFormat = GL_RGB16F;
		m_DataFormat = GL_RGB;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		frameBuffer->Bind();
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_Width, m_Height);
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& hdrPath)
		: m_Path(hdrPath)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1); 
		float* data = stbi_loadf(hdrPath.c_str(), &width, &height, &channels, 0);

		if (data)
		{
			m_IsLoaded = true;

			m_Width = width;
			m_Height = height;

			GLenum internalFormat = 0, dataFormat = 0;
			if (channels == 4)
			{
				internalFormat = GL_RGBA16F;
				dataFormat = GL_RGBA;
			}
			else if (channels == 3)
			{
				internalFormat = GL_RGB16F;
				dataFormat = GL_RGB;
			}

			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			HZ_CORE_ASSERT(internalFormat && dataFormat, "Format not supported!");

			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_FLOAT, data);

			stbi_image_free(data);
		}
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::SetData(void* data, uint32_t size, uint32_t textureIndex)
	{
		uint32_t bpc = m_DataFormat == GL_RGBA ? 4 : 3;
		HZ_CORE_ASSERT(size == m_Width * m_Height * bpc, "Data must be entire texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
	}

	void OpenGLTexture2D::SetDataFromFrameBuffer(const Ref<FrameBuffer>& frameBuffer, uint32_t textureIndex, int level)
	{
		frameBuffer->Bind();
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glCopyTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, 0, 0, m_Width, m_Height);
	}

	void OpenGLTexture2D::GenerateMipmaps() const
	{
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	void OpenGLTexture2D::Bind(uint32_t slot, uint32_t textureIndex) const
	{
		glBindTextureUnit(slot, m_RendererID);
	}


	////////////////////////////////////////////////////////////////////////////
	// TextureCube /////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	OpenGLTextureCube::OpenGLTextureCube(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
		m_InternalFormat = GL_RGB16F;
		m_DataFormat = GL_RGB;

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_R, GL_REPEAT);

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}

	OpenGLTextureCube::~OpenGLTextureCube()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTextureCube::SetData(void* data, uint32_t size, uint32_t textureIndex)
	{
		HZ_CORE_ASSERT(size == m_Width * m_Height * 3, "Data must be entire texture!");
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, data);
	}

	void OpenGLTextureCube::SetDataFromFrameBuffer(const Ref<FrameBuffer>& frameBuffer, uint32_t textureIndex, int level)
	{
		frameBuffer->Bind();
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
		glCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, level, 0, 0, 0, 0, m_Width, m_Height);
	}

	void OpenGLTextureCube::GenerateMipmaps() const
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}

	void OpenGLTextureCube::Bind(uint32_t slot, uint32_t textureIndex) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
		glActiveTexture(GL_TEXTURE0);
	}
}
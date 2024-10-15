#include "Hazel/Core/ResourceManager.h"

#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/Framebuffer.h"
#include "Hazel/Renderer/RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Hazel {

    ResourceManager* ResourceManager::s_Instance = nullptr;

	static void LoadPbrTexImage(const std::string imagePath, PbrTexImage* pbrTexImage)
	{
		std::vector<std::string> fileName = { "/albedo.png", "/normal.png", "/metallic.png", "/roughness.png", "/ao.png" };

		for (uint32_t i = 0; i < 5; i++)
		{
			StbImage& image = pbrTexImage->images[i];
			image.LoadImage(imagePath + fileName[i]);
		}
	}

	static void FreePbrTexImage(std::vector<PbrTexImage>& pbrTexImages)
	{
		for (uint32_t i = 0; i < pbrTexImages.size(); i++)
		{
			for (uint32_t i = 0; i < 5; i++)
			{
				StbImage& image = pbrTexImages[i].images[i];
				image.FreeImage();
			}
		}
	}

	PbrMaterialTexture ResourceManager::GetPbrTexture(const std::string& name)
	{
		auto it = m_PbrTextures.find(name);
		if (it != m_PbrTextures.end())
			return it->second;
		else
		{
			HZ_CORE_ERROR("Pbr texture resource not found!");
			return PbrMaterialTexture();
		}
	}

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

	Ref<TextureCube> ResourceManager::GetCubeTexture(const std::string& name)
	{
		auto it = m_CubeTextures.find(name);
		if (it != m_CubeTextures.end())
			return it->second;
		else
		{
			HZ_CORE_ERROR("Cube texture resource not found!");
			return Ref<TextureCube>();
		}
	}

	static void RenderCube();
	static void RenderQuad();

	ResourceManager::ResourceManager()
	{
		PreloadPbrTexResources();
		Preload2DTexResources();
		PrecomputeIBLTextures();
	}

	void ResourceManager::PreloadPbrTexResources()
	{
		std::string pbrTexturePath = "../../assets/textures/pbr/";

		std::vector<std::string> mtrlNames = { "rusted_iron", "gold", "grass", "plastic", "wall" };
		std::vector<PbrTexImage> pbrTexImages(mtrlNames.size());

#define ASYNC 1
#if ASYNC
		for (uint32_t i = 0; i < mtrlNames.size(); i++)
		{
			std::string imagePath = pbrTexturePath + mtrlNames[i];
			m_Futures.push_back(std::async(std::launch::async, LoadPbrTexImage, imagePath, &pbrTexImages[i]));
		}
		for (auto& future : m_Futures)
			future.get();
		m_Futures.clear();
		for (uint32_t i = 0; i < mtrlNames.size(); i++)
			m_PbrTextures[mtrlNames[i]] = PbrMaterialTexture(pbrTexturePath + mtrlNames[i], pbrTexImages[i]);
		m_Futures.push_back(std::async(std::launch::async, FreePbrTexImage, pbrTexImages));
#else
		for (uint32_t i = 0; i < mtrlNames.size(); i++)
			m_PbrTextures[mtrlNames[i]] = PbrMaterialTexture(pbrTexturePath + mtrlNames[i]);
#endif
	}

	void ResourceManager::Preload2DTexResources()
	{
		std::string texturePath = "../../assets/textures/";

		std::vector<std::string> texNames = { "Checkerboard", "ChernoLogo" };
		for (uint32_t i = 0; i < texNames.size(); i++)
			m_2DTextures[texNames[i]] = Texture2D::Create(texturePath + texNames[i] + ".png");

		std::vector<std::string> hdrTexNames = { "christmas_photo_studio_03_8k" };
		for (uint32_t i = 0; i < hdrTexNames.size(); i++)
			m_2DTextures[hdrTexNames[i]] = Texture2D::CreateHdr(texturePath + "hdr/" + hdrTexNames[i] + ".hdr");
	}

	void ResourceManager::PrecomputeIBLTextures()
	{
		// IBL
		Ref<Shader> IBL_EquirectangularToCubemapShader = Shader::Create("../../assets/shaders/IBL_EquirectangularToCubemap.glsl");
		Ref<Shader> IBL_IrradianceConvolutionShader = Shader::Create("../../assets/shaders/IBL_IrradianceConvolution.glsl");
		Ref<Shader> IBL_PrefilterShader = Shader::Create("../../assets/shaders/IBL_Prefilter.glsl");
		Ref<Shader> IBL_BrdfShader = Shader::Create("../../assets/shaders/IBL_Brdf.glsl");
		// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
		// ----------------------------------------------------------------------------------------------
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] =
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

		// pbr: setup cubemap to render to and attach to framebuffer
		// ---------------------------------------------------------
		Ref<TextureCube> envCubeMap = TextureCube::Create(512, 512);
		m_CubeTextures["EnvCubeMap"] = envCubeMap;
		// pbr: convert HDR equirectangular environment map to cubemap equivalent
		// ----------------------------------------------------------------------
		IBL_EquirectangularToCubemapShader->Bind();
		IBL_EquirectangularToCubemapShader->SetInt("equirectangularMap", 1);
		IBL_EquirectangularToCubemapShader->SetMat4("projection", captureProjection);
		m_2DTextures["christmas_photo_studio_03_8k"]->Bind(1);
		Ref<FrameBuffer> captureFBO;
		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGB16F, FramebufferTextureFormat::Depth };
		fbSpec.Width = 512;
		fbSpec.Height = 512;
		captureFBO = FrameBuffer::Create(fbSpec);
		captureFBO->Bind();
		for (uint32_t i = 0; i < 6; i++)
		{
			IBL_EquirectangularToCubemapShader->SetMat4("view", captureViews[i]);
			RenderCommand::Clear();
			RenderCube();
			envCubeMap->SetDataFromFrameBuffer(captureFBO, i);
		}
		captureFBO->Unbind();
		// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
		envCubeMap->GenerateMipmaps();

		// pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
		// --------------------------------------------------------------------------------
		Ref<TextureCube> irradianceMap = TextureCube::Create(32, 32);
		m_CubeTextures["IrradianceMap"] = irradianceMap;
		IBL_IrradianceConvolutionShader->Bind();
		IBL_IrradianceConvolutionShader->SetInt("environmentMap", 1);
		IBL_IrradianceConvolutionShader->SetMat4("projection", captureProjection);
		envCubeMap->Bind(1);
		captureFBO->Resize(32, 32);
		captureFBO->Bind();
		for (uint32_t i = 0; i < 6; i++)
		{
			IBL_IrradianceConvolutionShader->SetMat4("view", captureViews[i]);
			RenderCommand::Clear();
			RenderCube();
			irradianceMap->SetDataFromFrameBuffer(captureFBO, i);
		}
		captureFBO->Unbind();

		// pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
		// --------------------------------------------------------------------------------
		Ref<TextureCube> prefilterMap = TextureCube::Create(128, 128);
		m_CubeTextures["PrefilterMap"] = prefilterMap;
		IBL_PrefilterShader->Bind();
		IBL_PrefilterShader->SetInt("environmentMap", 1);
		IBL_PrefilterShader->SetMat4("projection", captureProjection);
		envCubeMap->Bind(1);
		uint32_t maxMipLevels = 5;
		for (uint32_t mip = 0; mip < maxMipLevels; mip++)
		{
			// reisze framebuffer according to mip-level size.
			uint32_t mipWidth = static_cast<uint32_t>(128 * std::pow(0.5, mip));
			uint32_t mipHeight = static_cast<uint32_t>(128 * std::pow(0.5, mip));
			captureFBO->Resize(mipWidth, mipHeight);
			captureFBO->Bind();
			float roughness = (float)mip / (float)(maxMipLevels - 1);
			IBL_PrefilterShader->SetFloat("roughness", roughness);
			for (uint32_t i = 0; i < 6; i++)
			{
				IBL_PrefilterShader->SetMat4("view", captureViews[i]);
				RenderCommand::Clear();
				RenderCube();
				prefilterMap->SetDataFromFrameBuffer(captureFBO, i, mip);
			}
		}
		captureFBO->Unbind();

		// pbr: generate a 2D LUT from the BRDF equations used.
		// ----------------------------------------------------
		IBL_BrdfShader->Bind();
		captureFBO->Resize(512, 512);
		captureFBO->Bind();
		RenderCommand::Clear();
		RenderQuad();
		Ref<Texture2D> brdfLUTTexture = Texture2D::Create(captureFBO);
		m_2DTextures["BrdfLUTTexture"] = brdfLUTTexture;
		captureFBO->Unbind();
	}

	static void RenderCube()
	{
		// initialize (if necessary)
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};

		// VAO
		Ref<VertexArray> cubeVAO = VertexArray::Create();
		// VBO
		Ref<VertexBuffer> cubeVBO = VertexBuffer::Create(vertices, sizeof(vertices));
		cubeVBO->SetLayout({
			{ ShaderDataType::Float3, "aPos"	},
			{ ShaderDataType::Float3, "aNormal"	},
			{ ShaderDataType::Float2, "aTexCoord"	},
		});
		cubeVAO->AddVertexBuffer(cubeVBO);

		// render Cube
		RenderCommand::DrawArrays(cubeVAO, 36);
	}

	static void RenderQuad()
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,

			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};

		// VAO
		Ref<VertexArray> quadVAO = VertexArray::Create();
		// VBO
		Ref<VertexBuffer> quadVBO = VertexBuffer::Create(quadVertices, sizeof(quadVertices));
		quadVBO->SetLayout({
			{ ShaderDataType::Float3, "aPos"	},
			{ ShaderDataType::Float2, "aTexCoord"	},
		});
		quadVAO->AddVertexBuffer(quadVBO);

		// render Cube
		RenderCommand::DrawArrays(quadVAO, 6);
	}

}
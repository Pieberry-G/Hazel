#include "Hazel/Scene/Scene.h"

#include "Hazel/Core/ResourceManager.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/ScriptableEntity.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Renderer/Renderer3D.h"
#include "Hazel/Renderer/RenderCommand.h"

namespace Hazel {

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	template<typename Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		auto view = src.view<Component>();
		for (auto e : view)
		{
			UUID uuid = src.get<IDComponent>(e).ID;
			HZ_CORE_ASSERT(enttMap.find(uuid) != enttMap.end());
			entt::entity dstEnttID = enttMap.at(uuid);

			auto& component = src.get<Component>(e);
			dst.emplace_or_replace<Component>(dstEnttID, component);
		}
	}

	template<typename Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		if (src.HasComponent<Component>())
		{
			dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
		}
	}

	Ref<Scene> Scene::Copy(Ref<Scene> other)
	{
		Ref<Scene> newScene = CreateRef<Scene>();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto e : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
			const std::string& name = srcSceneRegistry.get<TagComponent>(e).Tag;
			Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
			enttMap[uuid] = (entt::entity)newEntity;
		}

		// Copy components (except IDComponent and TagComponent)
		CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<SpriteRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<SphereRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<PointLightComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<DirectionalLightComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>(name);
		tag.Tag = name.empty() ? "Entity" : tag.Tag;
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnRuntimeStart()
	{
	}

	void Scene::OnRuntimeStop()
	{
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		// Update scripts
		{
			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				//TODO: Move to Scene::OnScenePlay
				if (!nsc.Instance)
				{
					nsc.Instance = nsc.InstantiateScript();
					nsc.Instance->m_Entity = Entity{ entity, this };
					nsc.Instance->OnCreate();
				}

				nsc.Instance->OnUpdate(ts);
			});
		}

		// Render 3D
		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			for (auto entity : view)
			{
				auto& [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);
				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = transform.GetTransform();
					break;
				}
			}
		}

		if(mainCamera)
		{
			Renderer3D::BeginScene(*mainCamera, cameraTransform);

			LightParams lightParams = GetLightParams();

			// Draw sphere
			{
				auto view = m_Registry.view<TransformComponent, SphereRendererComponent>();
				for (auto entity : view)
				{
					auto [transform, sphere] = view.get<TransformComponent, SphereRendererComponent>(entity);
					Renderer3D::DrawSphere(transform.GetTransform(), sphere, lightParams, (int)entity);
				}
			}
/*
			// Draw sprites
			{
				auto group = m_Registry.view<TransformComponent, SpriteRendererComponent>();
				for (auto entity : group)
				{
					auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
					Renderer3D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
				}
			}
*/
			Renderer3D::EndScene();
		}
	}

	void Scene::OnUpdateEditor(Timestep ts, EditorCamera& camera)
	{
		Renderer3D::BeginScene(camera);

		LightParams lightParams = GetLightParams();

		// Draw sphere
		{
			auto view = m_Registry.view<TransformComponent, SphereRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, sphere] = view.get<TransformComponent, SphereRendererComponent>(entity);
				Renderer3D::DrawSphere(transform.GetTransform(), sphere, lightParams, (int)entity);
			}
		}

		Renderer3D::DrawGroundPlane(15, 15, 1.0f);
/*
		// Draw sprite
		{
			auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);
				Renderer3D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
			}
		}
*/
		//Renderer2D::BeginScene(camera);
		//Renderer2D::DrawQuad(glm::vec3(0.0f), glm::vec3(1.0f), ResourceManager::Get()->Get2DTexture("IBL"));
		//Renderer2D::EndScene();

		Renderer3D::EndScene();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	void Scene::DuplicateEntity(Entity entity)
	{
		Entity newEntity = CreateEntity(entity.GetName());

		CopyComponentIfExists<TransformComponent>(newEntity, entity);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity, entity);
		CopyComponentIfExists<SphereRendererComponent>(newEntity, entity);
		CopyComponentIfExists<PointLightComponent>(newEntity, entity);
		CopyComponentIfExists<DirectionalLightComponent>(newEntity, entity);
		CopyComponentIfExists<CameraComponent>(newEntity, entity);
		CopyComponentIfExists<NativeScriptComponent>(newEntity, entity);
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& camera = view.get<CameraComponent>(entity);
			if (camera.Primary)
				return Entity{ entity, this };
		}
		return {};
	}

	LightParams Scene::GetLightParams()
	{
		LightParams lightParams;

		auto pointLightView = m_Registry.view<TransformComponent, PointLightComponent>();
		for (auto entity : pointLightView)
		{
			auto [transform, pointLight] = pointLightView.get<TransformComponent, PointLightComponent>(entity);
			lightParams.PointLightPositions.push_back(transform.Translation);
			lightParams.PointLightColors.push_back(pointLight.Color);
		}
		auto directionalLightView = m_Registry.view<TransformComponent, DirectionalLightComponent>();
		for (auto entity : directionalLightView)
		{
			auto [transform, directionalLight] = directionalLightView.get<TransformComponent, DirectionalLightComponent>(entity);
			lightParams.DirectionalLightDirection = directionalLight.Direction;
			lightParams.DirectionalLightColor = directionalLight.Color;
		}
		return lightParams;
	}

	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(false);
	}

	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SphereRendererComponent>(Entity entity, SphereRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<PointLightComponent>(Entity entity, PointLightComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<DirectionalLightComponent>(Entity entity, DirectionalLightComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}
}
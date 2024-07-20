#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Scene/Scene.h"

namespace Hazel {
	
	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContex(Ref<Scene> context);

		void OnImGuiRender();
	private:
		Ref<Scene> m_Context;
	};

}
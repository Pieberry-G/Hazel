#include "SceneHierarchyPanel.h"

#include "imgui.h"

#include "Hazel/Scene/Components.h"

namespace Hazel {

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContex(context);
	}

	void SceneHierarchyPanel::SetContex(Ref<Scene> context)
	{
		m_Context = context;
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");

		m_Context->m_Registry.view<TagComponent>().each([&](auto entityID, auto& tc)
		{
			ImGui::Text("%s", tc.Tag.c_str());
		});

		ImGui::End();
	}
}
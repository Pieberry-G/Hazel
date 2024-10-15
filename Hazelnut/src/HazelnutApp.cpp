#include <Hazel.h>
#include <Hazel/Core/EntryPoint.h>

#include "EditorLayer.h"
#include "EditorLayer3D.h"

namespace Hazel {

	class HazelnutApp : public Application
	{
	public:
		HazelnutApp()
		{
			//PushLayer(new EditorLayer());
			PushLayer(new EditorLayer3D());
		}

		~HazelnutApp()
		{

		}
	};

	Hazel::Application* CreateApplication()
	{
		return new HazelnutApp();
	}

}

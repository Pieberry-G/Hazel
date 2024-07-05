#include <Hazel.h>
#include <Hazel/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Hazel {

	class HazelnutApp : public Application
	{
	public:
		HazelnutApp()
		{
			//PushLayer(new ExampleLayer());
			PushLayer(new EditorLayer());
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

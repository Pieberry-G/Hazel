#pragma once

#include "Hazel/Core.h"
#include "Hazel/Window.h"
#include "Hazel/LayerStack.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"

#include "Hazel/Imgui/ImGuiLayer.h"

#include "Hazel/renderer/Shader.h"
#include "Hazel/Renderer/Buffer.h"

namespace Hazel {
	
	class HAZEL_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		bool OnWindowClose(WindowCloseEvent& e);
		void OnEvent(Event& e);

		void PushLayer(Layer* Layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

		inline static Application& Get() { return *s_Instance; }
	private:
		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		LayerStack m_LayerStack;

		unsigned int m_VertexArray;
		std::unique_ptr<Shader> m_shader;
		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
	private:
		static Application* s_Instance;
	};

	// To be defined in CLIENT
	Application* CreateApplication();

}
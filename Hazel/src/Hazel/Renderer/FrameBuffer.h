#pragma once

#include "Hazel/Core/Base.h"

namespace Hazel {

	struct FramebufferSpecification
	{
		uint32_t Width, Height;
		// FramebufferFormat Format =
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class FrameBuffer
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual uint32_t GetColorAttachmentRendererID() const  = 0;
		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<FrameBuffer> Create(const FramebufferSpecification& spec);
	};

}
#pragma once
#include "../../CommonUtilities/win/WinAPI.h"
#include <dxgi1_6.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;
#include "../Context.h"
#include "../NonCopyable.h"
#include "OverlayConfig.h"
#include <chrono>
#include <optional>

namespace GfxLayer::Extension
{
	void CheckResult(HRESULT result, const char* pMessage);

	class OverlayRenderer : public NonCopyable
	{
	public:
		OverlayRenderer(const OverlayConfig& cfg, IDXGISwapChain3* pSwapChain);
		virtual ~OverlayRenderer() = default;

		// called from a hook whenever the host is resizing its render targets
		virtual void			Resize(unsigned bufferCount, unsigned width, unsigned height) = 0;

		void					NewFrame();
		RECT					GetScissorRect() const;
		IDXGISwapChain3*		GetSwapChain() const;

	protected:
		virtual void			Render(bool renderBar, bool useRainbow) = 0;
		// this is triggered both by Resize and by UpdateConfig
		virtual void			UpdateViewport(const OverlayConfig& cfg) = 0;
		// called from Render when it is detected that config has changed (via IPC action)
		virtual void			UpdateConfig(const OverlayConfig& cfg) = 0;
		// get frame number from start of flash -> used for rainbow
		size_t GetFlashFrameIndex() const;

	private:
		using clock = std::chrono::high_resolution_clock;
		OverlayConfig			m_currentConfig;
		ComPtr<IDXGISwapChain3>	m_pSwapChain;
		unsigned m_width;
		unsigned m_height;
		std::optional<clock::time_point> m_flashStartTime;
		bool m_clickHoldoff = false;
		size_t m_flashFrameIndex = 0;
	};
}
#include "OverlayRenderer.h"
#include <cassert>
#include "../Logging.h"
#include "OverlayConfigPack.h"

namespace GfxLayer::Extension 
{
	using namespace std::literals;

	void CheckResult(HRESULT result, const char* pMessage)
	{
		if (FAILED(result))
		{
			LOGE << "Error: " << pMessage << " (0x" << std::hex << result << ")";
			assert(0);
		}
	}

	OverlayRenderer::OverlayRenderer(const OverlayConfig& cfg, IDXGISwapChain3* pSwapChain):
		m_currentConfig{ cfg },
		m_pSwapChain(pSwapChain)
	{}

	RECT OverlayRenderer::GetScissorRect() const
	{
		const float rectWidth = m_width * m_currentConfig.BarSize;
		RECT scissor;
		scissor.left = LONG((m_width - rectWidth) * m_currentConfig.BarRightShift);
		scissor.top = 0;
		scissor.right = LONG(scissor.left + rectWidth);
		scissor.bottom = m_height;
		return scissor;
	}

	IDXGISwapChain3* OverlayRenderer::GetSwapChain() const
	{
		return m_pSwapChain.Get();
	}

	void OverlayRenderer::Resize(unsigned bufferCount, unsigned width, unsigned height)
	{
		m_width = width;
		m_height = height;
		UpdateViewport(m_currentConfig);
	}

	void OverlayRenderer::NewFrame()
	{
		auto& pack = OverlayConfigPack::Get();
		if (pack.IsDirty()) {
			m_currentConfig = pack.Read();
			UpdateConfig(m_currentConfig);
		}

		bool flashStartedThisFrame = false;
		// we only check mouse state if we're not currently in a flash
		if (!m_flashStartTime) {
			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
				// if lmb is down, start a flash as long as we're not in holdoff
				if (!m_clickHoldoff) {
					m_flashStartTime = clock::now();
					flashStartedThisFrame = true;
					m_clickHoldoff = true;
				}
			}
			else {
				// if lmb up (and not in flash) remove the holdoff so another flash could begin
				m_clickHoldoff = false;
			}
		}
		// if we have a flash start we might need to draw flash
		if (m_flashStartTime) {
			// draw flash if initiated this frame OR within flash duration
			if (flashStartedThisFrame || (clock::now() - *m_flashStartTime < 400ms)) {
				Render(true);
			}
			else {
				// reset flash time if duration lapsed
				m_flashStartTime.reset();
			}
		}
		// if we're not in flash, we might need to draw background
		if (!m_flashStartTime && m_currentConfig.RenderBackground) {
			Render(false);
		}
	}
}


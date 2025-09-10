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

	ScissorRects OverlayRenderer::GetScissorRects() const
	{
		const float wfg = m_width * m_currentConfig.BarSize;
		const float wbg = m_width * m_currentConfig.BackgroundSize;
		const float w = std::max(wfg, wbg);
		const float c = (m_width - w) * m_currentConfig.BarRightShift + 0.5f * w;

		ScissorRects r{};
		r.fg = { LONG(c - 0.5f * wfg), 0, LONG(c - 0.5f * wfg + wfg), LONG(m_height) };
		r.bg = { LONG(c - 0.5f * wbg), 0, LONG(c - 0.5f * wbg + wbg), LONG(m_height) };
		return r;
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
					m_flashFrameIndex = 0;
				}
			}
			else {
				// if lmb up (and not in flash) remove the holdoff so another flash could begin
				m_clickHoldoff = false;
			}
		}
		else {
			// increment frame index for rainbow effect
			m_flashFrameIndex++;
		}
		// if we have a flash start we might need to draw flash
		if (m_flashStartTime) {
			// draw flash if initiated this frame OR within flash duration
			const std::chrono::duration<float> flashDuration{ m_currentConfig.FlashDuration };
			if (flashStartedThisFrame || (clock::now() - *m_flashStartTime < flashDuration)) {
				Render(true, m_currentConfig.UseRainbow, m_currentConfig.RenderBackground);
			}
			else {
				// reset flash time if duration lapsed
				m_flashStartTime.reset();
			}
		}
		// if we're not in flash, we might need to draw background
		if (!m_flashStartTime && m_currentConfig.RenderBackground) {
			Render(false, m_currentConfig.UseRainbow, true);
		}
	}

	size_t OverlayRenderer::GetRainbowIndex() const
	{
		return m_flashFrameIndex % m_rainbowColors.size();
	}
}


#pragma once

namespace GfxLayer::Extension
{
	struct OverlayConfig
	{
		float BarSize = 0.2f;
		float BarRightShift = 0.5f;
		float BarColor[4] = { 1.f, 1.f, 1.f, 1.f };
		bool  RenderBackground = false;
		float BackgroundColor[4] = { 0.f, 0.f, 0.f, 1.f };
	};
}
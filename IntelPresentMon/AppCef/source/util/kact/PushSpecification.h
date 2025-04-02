#pragma once
#include "../../../Interprocess/source/act/ActionHelper.h"
#include "KernelExecutionContext.h"
#include "../KernelWrapper.h"
#include "../MakeOverlaySpec.h"
#include <format>
#include <Core/source/kernel/Kernel.h>
#include <variant>
#include <cereal/types/variant.hpp>
#include <cereal/types/array.hpp>
#include "../V8Transfer.h"

#define ACT_NAME PushSpecification
#define ACT_EXEC_CTX KernelExecutionContext
#define ACT_TYPE AsyncActionBase_
#define ACT_NS ::p2c::client::util::kact

namespace p2c::client::util::kact
{
	using namespace ::pmon::ipc::act;
    
    namespace push_spec_impl
    {
        struct Metric
        {
            PM_METRIC metricId;
            uint32_t arrayIndex;
            uint32_t deviceId;
            PM_STAT statId;
            PM_UNIT desiredUnitId;

            template<class A> void serialize(A& ar) {
                ar(metricId, arrayIndex, deviceId, statId, desiredUnitId);
            }
        };

        struct WidgetMetric
        {
            Metric metric;
            gfx::Color lineColor;
            gfx::Color fillColor;
            gfx::lay::AxisAffinity axisAffinity;

            template<class A> void serialize(A& ar) {
                ar(metric, lineColor, fillColor, axisAffinity);
            }
        };

        // Graph widget type.
        struct Graph {
            std::vector<WidgetMetric> metrics;

            uint32_t height;
            uint32_t vDivs;
            uint32_t hDivs;
            bool showBottomAxis;
            struct GraphType {
                std::string name;
                std::array<int, 2> range;
                std::array<int, 2> rangeRight;
                uint32_t binCount;
                std::array<int, 2> countRange;
                bool autoLeft;
                bool autoRight;
                bool autoCount;

                template<class A> void serialize(A& ar) {
                    ar(name, range, rangeRight, binCount, countRange, autoLeft, autoRight, autoCount);
                }
            } graphType;
            gfx::Color gridColor;
            gfx::Color dividerColor;
            gfx::Color backgroundColor;
            gfx::Color borderColor;
            gfx::Color textColor;
            float textSize;

            template<class A> void serialize(A& ar) {
                ar(metrics, height, vDivs, hDivs, showBottomAxis, graphType,
                    gridColor, dividerColor, backgroundColor, borderColor, textColor, textSize);
            }
        };

        struct Readout {
            std::vector<WidgetMetric> metrics;

            bool showLabel;
            float fontSize;
            gfx::Color fontColor;
            gfx::Color backgroundColor;

            template<class A> void serialize(A& ar) {
                ar(metrics, showLabel, fontSize, fontColor, backgroundColor);
            }
        };

        using Widget = std::variant<Graph, Readout>;

        struct Params {
            std::optional<uint32_t> pid;

            struct Preferences {
                std::string capturePath;
                uint32_t captureDelay;
                bool enableCaptureDelay;
                uint32_t captureDuration;
                bool enableCaptureDuration;
                bool hideDuringCapture;
                bool hideAlways;
                bool independentWindow;
                uint32_t metricPollRate;
                uint32_t overlayDrawRate;
                uint32_t telemetrySamplingPeriodMs;
                uint32_t etwFlushPeriod;
                bool manualEtwFlush;
                uint32_t metricsOffset;
                uint32_t metricsWindow;
                kern::OverlaySpec::OverlayPosition overlayPosition;
                float timeRange;
                float overlayMargin;
                float overlayBorder;
                float overlayPadding;
                float graphMargin;
                float graphBorder;
                float graphPadding;
                gfx::Color overlayBorderColor;
                gfx::Color overlayBackgroundColor;

                struct GraphFont {
                    std::string name;
                    float axisSize;
                    template<class A> void serialize(A& ar) {
                        ar(name, axisSize);
                    }
                } graphFont;

                uint32_t overlayWidth;
                bool upscale;
                bool generateStats;
                bool enableTargetBlocklist;
                bool enableAutotargetting;
                float upscaleFactor;
                std::optional<int> adapterId; // Uncertain: may be a different type in your system.
                bool enableFlashInjection;
                float flashInjectionSize;
                gfx::Color flashInjectionColor;
                bool flashInjectionBackgroundEnable;
                gfx::Color flashInjectionBackgroundColor;
                float flashInjectionRightShift;

                template<class A> void serialize(A& ar) {
                    ar(capturePath, captureDelay, enableCaptureDelay,
                        captureDuration, enableCaptureDuration, hideDuringCapture, hideAlways,
                        independentWindow, metricPollRate, overlayDrawRate, telemetrySamplingPeriodMs,
                        etwFlushPeriod, manualEtwFlush, metricsOffset, metricsWindow, overlayPosition,
                        timeRange, overlayMargin, overlayBorder, overlayPadding, graphMargin,
                        graphBorder, graphPadding, overlayBorderColor, overlayBackgroundColor,
                        graphFont, overlayWidth, upscale, generateStats, enableTargetBlocklist,
                        enableAutotargetting, upscaleFactor, adapterId, enableFlashInjection, flashInjectionSize,
                        flashInjectionColor, flashInjectionBackgroundEnable, flashInjectionBackgroundColor,
                        flashInjectionRightShift);
                }
            } preferences;

            // Widgets stored as a variant: either Graph or Readout.
            std::vector<Widget> widgets;

            template<class A> void serialize(A& ar) {
                ar(pid, preferences, widgets);
            }
        };
    }

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
        using Params = push_spec_impl::Params;
		struct Response {
			template<class A> void serialize(A& ar) {
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
        static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
        {
            (*ctx.ppKernel)->UpdateInjection(
                in.preferences.enableFlashInjection,
                in.pid,
                in.preferences.flashInjectionBackgroundEnable,
                in.preferences.flashInjectionColor,
                in.preferences.flashInjectionBackgroundColor,
                in.preferences.flashInjectionSize,
                in.preferences.flashInjectionRightShift
            );
            if (!in.pid) {
                (*ctx.ppKernel)->ClearOverlay();
            }
            else {
                (*ctx.ppKernel)->PushSpec(MakeOverlaySpec(in));
            }
            return {};
        }
	};



#ifdef PM_ASYNC_ACTION_REGISTRATION_
	ACTION_REG();
#endif
}

namespace cereal
{
    template<class Archive>
    void serialize(Archive& archive, p2c::gfx::Color& s)
    {
        archive(s.r, s.g, s.b, s.a);
    }
}

namespace p2c::client::util
{
    template<>
    struct CustomV8Conversion<kact::push_spec_impl::Widget>
    {
        static void FromV8(CefV8Value& v8, kact::push_spec_impl::Widget& out)
        {
            kern::WidgetType type;
            ::p2c::client::util::FromV8(*v8.GetValue("widgetType"), type);
            if (type == kern::WidgetType::Graph) {
                using Xfer = kact::push_spec_impl::Graph;
                out.emplace<Xfer>();
                ::p2c::client::util::FromV8(v8, std::get<Xfer>(out));
            }
            else if (type == kern::WidgetType::Readout) {
                using Xfer = kact::push_spec_impl::Readout;
                out.emplace<Xfer>();
                ::p2c::client::util::FromV8(v8, std::get<Xfer>(out));
            }
            else {
                pmlog_error("Unknown widget type").pmwatch(int(type));
            }
        }
    };

    template<>
    struct CustomV8Conversion<gfx::Color>
    {
        static void FromV8(CefV8Value& v8, gfx::Color& out)
        {
            out.r = float(v8.GetValue("r")->GetIntValue()) / 255.f;
            out.g = float(v8.GetValue("g")->GetIntValue()) / 255.f;
            out.b = float(v8.GetValue("b")->GetIntValue()) / 255.f;
            out.a = float(v8.GetValue("a")->GetDoubleValue());
        }
    };
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE
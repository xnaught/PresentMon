// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Overlay.h"
#include <Core/source/gfx/layout/GraphElement.h>
#include <Core/source/gfx/layout/FlexElement.h>
#include <Core/source/gfx/layout/TextElement.h>
#include <Core/source/gfx/layout/ReadoutElement.h>
#include <Core/source/pmon/PresentMon.h>
#include <Core/source/pmon/RawFrameDataWriter.h>
#include <Core/source/pmon/metric/DynamicPollingMetric.h>
#include <Core/source/pmon/Timekeeper.h>
#include <Core/source/gfx/layout/style/StyleProcessor.h>
#include <Core/source/infra/util/rn/ToVector.h>
#include <Core/source/win/StandardWindow.h>
#include <Core/source/win/OverlayWindow.h>
#include <Core/source/infra/opt/Options.h>
#include <ranges>
#include <set>
#include <chrono>
#include <format>
#include <thread>
#include <array>
#include <cassert>
#include "TargetLostException.h"


namespace p2c::kern
{
    using namespace gfx;
    using namespace lay;


    // free functions used by Overlay
    namespace
    {
        auto MakeDocument_(
            gfx::Graphics& gfx,
            const OverlaySpec& spec,
            std::map<size_t, GraphDataPack>& graphPacks,
            std::vector<TextDataPack>& textPacks,
            std::shared_ptr<TextElement>& captureIndicator)
        {
            auto pRoot = FlexElement::Make({}, { "doc" });
            std::shared_ptr<gfx::lay::Element> pReadoutContainer;
            size_t iNextReadout = 0;

            for (const auto& w : spec.widgets) {
                try {
                    if (auto pGraphSpec = std::get_if<GraphSpec>(&w)) {
                        auto packsData = pGraphSpec->metrics |
                            std::views::transform([&](const GraphMetricSpec& metric) {
                                auto& pack = graphPacks.at(metric.index);
                                return std::make_shared<GraphLinePack>(GraphLinePack{
                                    .data = pack.pData,
                                    .axisAffinity = metric.axisAffinity,
                                    .label = pack.GetFullName(),
                                    .units = pack.pMetric->GetUnits(),
                                });
                            }) |
                            infra::util::rn::ToVector();
                        pRoot->AddChild(GraphElement::Make(
                            pGraphSpec->type, std::move(packsData), { pGraphSpec->tag }
                        ));
                        pReadoutContainer.reset();
                    }
                    else if (auto pReadoutSpec = std::get_if<ReadoutSpec>(&w)) {
                        if (!pReadoutContainer) {
                            pReadoutContainer = gfx::lay::FlexElement::Make({}, { "readout-container" });
                            pRoot->AddChild(pReadoutContainer);
                        }
                        auto& pack = textPacks[iNextReadout++];
                        pReadoutContainer->AddChild(gfx::lay::ReadoutElement::Make(
                            pack.pMetric->GetMetricClassName() == L"Text", pack.GetFullName(),
                            pack.pMetric->GetUnits(), &pack.text, { pReadoutSpec->tag } )
                        );
                    }
                    else {
                        throw std::runtime_error{ "Bad widget variant" };
                    }
                }
                catch (...) {
                    p2clog.warn(L"Failed building a widget into document").commit();
                }
            }

            // capture state indicator
            pRoot->AddChild(FlexElement::Make(
                {
                    TextElement::Make(L"Capture Status:", {"label"}),
                    captureIndicator = TextElement::Make(L"-------------------", {"value"}),
                },
                { "cap" }
            ));

            // trigger layout calculation of entire document
            pRoot->FinalizeAsRoot(DimensionsSpec{ (float)spec.overlayWidth }, spec.sheets, gfx);

            return pRoot;
        }
    }



    // Overlay member functions

    Overlay::Overlay(
        win::Process proc_,
        std::shared_ptr<OverlaySpec> pSpec_,
        pmon::PresentMon* pm_,
        std::map<size_t, GraphDataPack> graphPacks_,
        std::optional<gfx::Vec2I> pos_)
        :
        Overlay{ proc_, pSpec_, pm_, std::move(graphPacks_), std::move(pos_),
            std::make_unique<pmon::CachingQuery>(pm_->GetTracker(), pSpec_->averagingWindowSize, pSpec_->metricsOffset)}
    {}

    Overlay::Overlay(
        win::Process proc_,
        std::shared_ptr<OverlaySpec> pSpec_,
        pmon::PresentMon* pm_,
        std::map<size_t, GraphDataPack> graphPacks_,
        std::optional<gfx::Vec2I> pos_,
        std::unique_ptr<pmon::CachingQuery> pQuery_)
        :
        proc{ std::move(proc_) },
        pm{ pm_ },
        pSpec{ std::move(pSpec_) },
        pQuery{ std::move(pQuery_) },
        graphPacks{ std::move(graphPacks_) },
        hProcess{ OpenProcess(SYNCHRONIZE, TRUE, proc.pid) },
        moveHandlerToken{ win::EventHookManager::AddHandler(std::make_shared<WindowMoveHandler>(proc, this)) },
        activateHandlerToken{ win::EventHookManager::AddHandler(std::make_shared<WindowActivateHandler>(proc, this)) },
        targetRect{ win::GetWindowClientRect(proc.hWnd) },
        position{ pSpec->overlayPosition },
        upscaleFactor{ pSpec->upscale ? pSpec->upscaleFactor : 1.f },
        graphicsDimensions{ pSpec->overlayWidth, 240 },
        windowDimensions{ Dimensions{ graphicsDimensions } * upscaleFactor },
        pWindow{ MakeWindow_(pos_) },
        gfx{ pWindow->GetHandle(), graphicsDimensions, upscaleFactor, infra::opt::get().allowTearing },
        samplingPeriodMs{ pSpec->samplingPeriodMs },
        samplesPerFrame{ pSpec->samplesPerFrame },
        hideDuringCapture{ pSpec->hideDuringCapture },
        hideAlways{ pSpec->hideAlways },
        samplingWaiter{ float(pSpec->samplingPeriodMs) / 1'000.f }
    {
        UpdateDataSets_();
        pRoot = MakeDocument_(gfx, *pSpec, graphPacks, textPacks, pCaptureIndicatorText);
        UpdateCaptureStatusText_();
        AdjustOverlaySituation_(position);
        pm->StartTracking(proc.pid);
    }

    Overlay::~Overlay()
    {
        if (pm) {
            try { pm->StopTracking(); }
            catch (...) {}
        }
    }

    void Overlay::UpdateDataSets_()
    {
        // set of new graph metrics after update, used to prune the GraphDataPacks
        std::set<size_t> newSet;
        // clear text packs every update because they have no buffer of samples that need to be carried over
        textPacks.clear();
        // clearing query (which owns all realized dynamic metrics) and activeMetricsMap (has non-owning pointers to all metrics)
        // we might want to carry over metrics that are common between before and after update, but it's difficult to compare
        // here due to differences not only in metric index, but also targetted device
        pQuery->Reset();
        activeMetricsMap.clear();
        // helper to resolve metric type and add to dynamic query if necessary
        const auto ResolveMetric = [this](size_t metricIndex) {
            auto pRepoMetric = pm->GetMetricByIndex(metricIndex);
            if (auto pDynamicMetric = dynamic_cast<pmon::met::DynamicPollingMetric*>(pRepoMetric)) {
                // TODO: instead of hardcoding 1 in here as the default device id, derive it based
                // on the enumerated gpu devices
                const auto activeGpuId = pm->GetSelectedAdapter().value_or(1);
                auto pUniqueRealized = pDynamicMetric->RealizeMetric(pm->GetIntrospectionRoot(), pQuery.get(), activeGpuId);
                pmon::met::Metric* pRawRealized = pUniqueRealized.get();
                pQuery->AddDynamicMetric(std::move(pUniqueRealized));
                return pRawRealized;
            }
            else {
                // use metric from repository directly if not a DynamicPollingMetric
                return pRepoMetric;
            }
        };
        // registering all requested metrics into a map
        for (auto& w : pSpec->widgets) {
            if (auto pGraphSpec = std::get_if<GraphSpec>(&w)) {
                for (const auto metric : pGraphSpec->metrics) {
                    if (!activeMetricsMap.contains(metric.index)) {
                        activeMetricsMap[metric.index] = ResolveMetric(metric.index);
                    }
                }
            }
            else if (auto pReadoutSpec = std::get_if<ReadoutSpec>(&w)) {
                if (!activeMetricsMap.contains(pReadoutSpec->metricIndex)) {
                    activeMetricsMap[pReadoutSpec->metricIndex] = ResolveMetric(pReadoutSpec->metricIndex);
                }
            }
        }
        // compiling the dynamic query
        pQuery->Finalize(pm->GetSession());
        // populating packs with metrics
        for (auto& w : pSpec->widgets) {
            try {
                if (auto pGraphSpec = std::get_if<GraphSpec>(&w)) {
                    for (const auto metric : pGraphSpec->metrics) {
                        auto pMetric = activeMetricsMap[metric.index];
                        // TODO: silent fail this by inserting "empty" data pack and logging 
                        assert(pMetric);
                        auto [i, inserted] = graphPacks.emplace(metric.index,
                            GraphDataPack{ pMetric, pSpec->graphDataWindowSize }
                        );
                        // if pack already existed, resize it
                        if (!inserted) {
                            i->second.pData->Resize(pSpec->graphDataWindowSize);
                        }
                        newSet.emplace(metric.index);
                    }
                }
                else if (auto pReadoutSpec = std::get_if<ReadoutSpec>(&w)) {
                    auto pMetric = activeMetricsMap[pReadoutSpec->metricIndex];
                    // TODO: silent fail this by inserting "empty" data pack and logging 
                    assert(pMetric);
                    textPacks.emplace_back(std::wstring{}, pMetric);
                }
                else {
                    throw std::runtime_error{ "Bad widget variant" };
                }
            }
            catch (...) {
                p2clog.warn(L"Failed updating dataset with widget").commit();
            }
        }
        // prune data packs that are now defunct
        std::erase_if(graphPacks, [&newSet](const auto& p) { return !newSet.contains(p.first); });
        // all previous realized metrics were wiped out, so we need to update pointers in data packs
        for (auto&&[idx, pack] : graphPacks) {
            pack.pMetric = activeMetricsMap[idx];
        }
    }

    std::unique_ptr<win::KernelWindow> Overlay::MakeWindow_(std::optional<Vec2I> pos_)
    {
        UpdateTargetFullscreenStatus();
        std::unique_ptr<win::KernelWindow> pWindow;
        if (pSpec->independentKernelWindow) {
            // try and get the position of the control (CEF) window for calculating independent metrics window pos
            // TODO: connect this more assuredly to the cef control window
            // tried CefBrowserHost::GetWindowHandle, but it causes crashes for some unknown reason
            // should make this at least less brittle with respect to window classname / title
            Vec2I iPos{ CW_USEDEFAULT, CW_USEDEFAULT };
            if (auto hWndControl = FindWindowA("BrowserWindowClass", "Intel PresentMon")) {
                RECT controlRect{};
                if (GetWindowRect(hWndControl, &controlRect)) {
                    iPos = { controlRect.left + 25, controlRect.top + 25 };
                }
                else {
                    p2clog.warn(L"failed to get rect of control window").hr().commit();
                }
            }
            else {
                p2clog.warn(L"failed to find control window").commit();
            }
            // make the metrics window
            pWindow = std::make_unique<win::StandardWindow>(
                iPos.x, iPos.y,
                windowDimensions,
                L"PresentMon Data Display"
            );
        }
        else {
            const auto pos = CalculateOverlayPosition_();
            pWindow = std::make_unique<win::OverlayWindow>(
                targetFullscreen,
                pos.x, pos.y,
                windowDimensions,
                L"P2C#OVERLAY"
            );
            // place overlay just above target in the z-order
            pWindow->Reorder(proc.hWnd);
        }
        return pWindow;
    }

    void Overlay::RebuildDocument(std::shared_ptr<OverlaySpec> pSpec_)
    {
        pSpec = std::move(pSpec_);
        UpdateDataSets_();
        pRoot = MakeDocument_(gfx, *pSpec, graphPacks, textPacks, pCaptureIndicatorText);
        UpdateCaptureStatusText_();
        samplingPeriodMs = pSpec->samplingPeriodMs;
        samplingWaiter.SetInterval(pSpec->samplingPeriodMs / 1'000.f);
        samplesPerFrame = pSpec->samplesPerFrame;
        hideDuringCapture = pSpec->hideDuringCapture;
        hideAlways = pSpec->hideAlways;
        AdjustOverlaySituation_(pSpec->overlayPosition);
        if (IsHidden_())
        {
            pWindow->Hide();
        }
        else
        {
            pWindow->Show();
            pWindow->Reorder(proc.hWnd);
        }
    }

    void Overlay::AdjustOverlaySituation_(OverlaySpec::OverlayPosition position_)
    {
        if (const DimensionsI newDims = pRoot->GetElementDims(); newDims != graphicsDimensions)
        {
            graphicsDimensions = newDims;
            windowDimensions = Dimensions{ graphicsDimensions } * upscaleFactor;
            pWindow->Resize(windowDimensions);
            if (!pWindow->Standard()) {
                pWindow->Move(CalculateOverlayPosition_());
            }
            gfx.Resize(graphicsDimensions);
            position = position_;
        }
        else if (position != position_)
        {
            position = position_;
            if (!pWindow->Standard()) {
                pWindow->Move(CalculateOverlayPosition_());
            }
        }
    }

    void Overlay::UpdateGraphData_(double timestamp)
    {
        if (!IsTargetLive())
        {
            throw TargetLostException{};
        }

        for (auto& [i, p] : graphPacks)
        {
            p.Populate(timestamp);
        }

        for (auto& p : textPacks)
        {
            p.Populate(timestamp);
        }
    }

    void Overlay::UpdateTargetRect(const RectI& newRect)
    {
        if (pWindow->Standard()) {
            // if we are a independent overlay (which is a standard) window, don't move when target moves
            return;
        }

        if (targetRect != newRect) {
            targetRect = newRect;
            pWindow->Move(CalculateOverlayPosition_());
        }
        // hide window during move, record timepoint to determine when to show again
        pWindow->Hide();
        lastMoveTime = std::chrono::high_resolution_clock::now();
    }

    void Overlay::UpdateTargetOrder(bool topmost)
    {
        if (pWindow->Standard()) {
            // if we are a independent overlay (which is a standard) window, don't do reordering
            return;
        }

        if (pWindow->Fullscreen()) {
            // if we are a fullscreen attached overlay, always stay on top
            // TODO: examine how topmost is set on SOTTR with logging to improve this fullscreen kludge
            pWindow->SetTopmost();
        }
        else {
            if (topmost) {
                // this function called with topmost=true when a window activation happens
                // and the activated window is the target, so we need topmost to get on top
                // of it
                pWindow->SetTopmost();
            }
            else {
                // if other window was activated, we need to stop being topmost and insert
                // ourself just above the target in the window order
                pWindow->ClearTopmost();
                pWindow->Reorder(proc.hWnd);
            }
        }
    }

    Vec2I Overlay::CalculateOverlayPosition_() const
    {
        if (position == OverlaySpec::OverlayPosition::Center)
        {
            p2clog.note(L"center overlay position unimplimented").commit();
        }

        int x, y;
        if (position == OverlaySpec::OverlayPosition::TopLeft || position == OverlaySpec::OverlayPosition::BottomLeft)
        {
            x = targetRect.left;
        }
        else
        {
            x = targetRect.right - windowDimensions.width;
        }
        if (position == OverlaySpec::OverlayPosition::TopLeft || position == OverlaySpec::OverlayPosition::TopRight)
        {
            y = targetRect.top;
        }
        else
        {
            y = targetRect.bottom - windowDimensions.height;
        }

        return { x, y };
    }

    bool Overlay::IsHidden_() const
    {
        return (pWriter && hideDuringCapture) || hideAlways;
    }

    void Overlay::Render_()
    {
        // update window contents
        gfx.BeginFrame();
        pRoot->Draw(gfx);
        gfx.EndFrame();
    }

    void Overlay::UpdateCaptureStatusText_()
    {
        if (pWriter) {
            pCaptureIndicatorText->SetText(L"In Progress");
        }
        else {
            pCaptureIndicatorText->SetText(L"Standing By");
        }
    }

    void Overlay::InitiateClose()
    {
        pWindow->Close();
    }

    void Overlay::RunTick()
    {
        using namespace std::chrono_literals;
        // polling data sources multiple times per drawn overlay frame
        if (IsHidden_())
        {
            // hardcode this slow overlay tick for now during capture
            std::this_thread::sleep_for(100ms);
            pmon::Timekeeper::LockNow();
        }
        else
        {
            for (int i = 0; i < samplesPerFrame; i++)
            {
                samplingWaiter.Wait();
                pmon::Timekeeper::LockNow();
                UpdateGraphData_(pmon::Timekeeper::GetLockedNow());
            }
        }

        if (pWriter) {
            pWriter->Process();
        }

        // handle hide during move logic (show if time elapsed and now otherwise hidden)
        if (lastMoveTime) {
            const auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration<float>(now - *lastMoveTime).count() >= 0.1f) {
                lastMoveTime = {};
                if (!IsHidden_()) {
                    pWindow->Show();
                }
            }
        }

        if (!IsHidden_())
        {
            Render_();
        }
    }

    void Overlay::SetCaptureState(bool active, std::wstring path, std::wstring name)
    {
        p2clog.info(std::format(L"Capture set to {}", active)).commit();

        if (active && !pWriter)
        {
            const std::chrono::zoned_time now{ std::chrono::current_zone(), std::chrono::system_clock::now() };
            auto fullPath = std::format(L"{0}{1}-{3}-{2:%y}{2:%m}{2:%d}-{2:%H}{2:%M}{2:%OS}.csv", path, name, now, proc.name);
            // create optional path for stats file
            auto fullStatsPath = [&]() -> std::optional<std::wstring> {
                if (pSpec->generateStats) {
                    return std::format(L"{0}{1}-{3}-{2:%y}{2:%m}{2:%d}-{2:%H}{2:%M}{2:%OS}-stats.csv", path, name, now, proc.name);
                }
                else {
                    return std::nullopt;
                }
            }();
            pWriter = { pm->MakeRawFrameDataWriter(std::move(fullPath), std::move(fullStatsPath), proc.pid, proc.name) };
        }
        else if (!active && pWriter)
        {
            pWriter.reset();
        }

        // handle window visibility
        if (IsHidden_())
        {
            pWindow->Hide();
        }
        else
        {
            pWindow->Show();
            pWindow->Reorder(proc.hWnd);
        }

        // update indicator on overlay
        UpdateCaptureStatusText_();
    }

    bool Overlay::IsTargetLive() const
    {
        const auto ret = WaitForSingleObject(hProcess, 0);
        if (ret == WAIT_FAILED)
        {
            p2clog.hr().commit();
        }
        return ret != WAIT_OBJECT_0;
    }

    bool Overlay::IsStandardWindow() const
    {
        return pWindow->Standard();
    }

    const win::Process& Overlay::GetProcess() const
    {
        return proc;
    }

    void Overlay::UpdateTargetFullscreenStatus()
    {
        if (const auto hMon = MonitorFromWindow(proc.hWnd, MONITOR_DEFAULTTONULL)) {
            MONITORINFOEXW monInfo{};
            monInfo.cbSize = (DWORD)sizeof(MONITORINFOEXW);
            if (GetMonitorInfoW(hMon, &monInfo) != 0) {
                const auto monRect = win::RectToRectI(monInfo.rcMonitor);
                targetFullscreen = targetRect == monRect;
            }
            else {
                p2clog.warn(L"didn't get monitor info").commit();
            }
        }
        else {
            p2clog.warn(L"didn't get monitor from window").commit();
        }
    }

    bool Overlay::NeedsFullscreenReboot() const
    {
        return !pWindow->Standard() && (targetFullscreen != pWindow->Fullscreen());
    }

    const OverlaySpec& Overlay::GetSpec() const
    {
        return *pSpec;
    }

    std::unique_ptr<Overlay> Overlay::SacrificeClone(std::optional<HWND> hWnd_, std::shared_ptr<OverlaySpec> pSpec_)
    {
        p2clog.info(L"doing SacrificeClone").commit();

        std::optional<Vec2I> pos;
        if (pWindow->Standard()) {
            pos = pWindow->GetPosition();
        }

        if (!pSpec_) {
            pSpec_ = std::move(pSpec);
        }

        proc.hWnd = hWnd_.value_or(proc.hWnd);

        auto pNewOverlay = std::make_unique<Overlay>(
            proc,
            std::move(pSpec_),
            pm,
            std::move(graphPacks),
            pos,
            std::move(pQuery)
        );
        // clear pm so that stream isn't closed when this overlay dies
        pm = nullptr;
        pNewOverlay->pWriter = std::move(pWriter);
        pNewOverlay->UpdateCaptureStatusText_();
        return pNewOverlay;
    }
    std::unique_ptr<Overlay> Overlay::RetargetPidClone(win::Process proc_)
    {
        p2clog.info(L"doing RetargetPidClone").commit();

        std::optional<Vec2I> pos;
        if (pWindow->Standard()) {
            pos = pWindow->GetPosition();
        }

        pm->StopTracking();
        auto pNewOverlay = std::make_unique<Overlay>(
            proc_,
            std::move(pSpec),
            pm,
            std::map<size_t, GraphDataPack>{},
            pos
        );
        // clear pm so that stream isn't closed when this overlay dies
        // (because we manually close it above)
        pm = nullptr;
        return pNewOverlay;
    }

    const gfx::RectI& Overlay::GetTargetRect() const
    {
        return targetRect;
    }
}
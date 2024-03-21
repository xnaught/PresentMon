#pragma once
#include <fstream>
#include <format>

int MetricListSample(pmapi::Session& session)
{
    std::ofstream out{ "metrics.md" };

    // header
    out << "| Metric | Description | Compatible Query Types |\n"
        << "| - | - |:-:|\n";

    // Loop through ALL PresentMon metrics
    for (auto metric : session.GetIntrospectionRoot()->GetMetrics()) {
        // name and description
        out << std::format("|{}|{}|", metric.Introspect().GetName(), metric.Introspect().GetDescription());
        // query compatibility codes
        switch (metric.GetType()) {
        case PM_METRIC_TYPE_DYNAMIC: out << "D|\n"; break;
        case PM_METRIC_TYPE_DYNAMIC_FRAME: out << "DF|\n"; break;
        case PM_METRIC_TYPE_FRAME_EVENT: out << "F|\n"; break;
        case PM_METRIC_TYPE_STATIC: out << "DS|\n"; break;
        default: out << "|\n";
        }
    }

    return 0;
}

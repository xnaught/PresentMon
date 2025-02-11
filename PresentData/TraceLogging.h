#pragma once
#include <windows.h>
#include <stdexcept>

#define INITGUID // Ensure that EventTraceGuid is defined.
#include <evntrace.h>
#undef INITGUID

#include <tdh.h>
#include <vector>
#include <string>
#include <optional>

class TraceLoggingContext
{
public:
    explicit TraceLoggingContext() :
        mpEventRecord(nullptr),
        mpEventData(nullptr),
        mpEventDataEnd(nullptr),
        mPointerSize(0),
        mCurrentEventValid(false),
        mpTraceEventInfo(nullptr),
        mEventTimeStamp{ 0 } {
    }

    bool DecodeTraceLoggingEventRecord(EVENT_RECORD* pEventRecord);
    std::optional<std::wstring> GetEventName();

    // Helper: dependent_false for static_assert in templates.
    template<typename T>
    struct dependent_false : std::false_type {};

    // Templated member function to retrieve a property value of type T by name.
    template<typename T>
    T GetNumericPropertyValue(const std::wstring& propertyName) const
    {
        unsigned propertyCount = this->mpTraceEventInfo->PropertyCount;

        // Start at the beginning of the raw data.
        const BYTE* pb = this->mpEventData;

        // Iterate over each property.
        for (unsigned i = 0; i < propertyCount; ++i)
        {
            const EVENT_PROPERTY_INFO& epi = this->mpTraceEventInfo->EventPropertyInfoArray[i];

            // Only support scalar properties: count must be 1 and not flagged as a struct or parameterized.
            if (epi.count != 1 || (epi.Flags & (PropertyStruct | PropertyParamCount)) != 0)
            {
                size_t skip = 0;
                try {
                    skip = GetSkipSize(epi.nonStructType.InType);
                }
                catch (std::runtime_error) {
                    throw std::runtime_error("Invalid skip size");
                }
                pb += skip;
                continue;
            }

            // Obtain the property name.
            auto currentName = epi.NameOffset ? this->GetTraceEventInfoString(epi.NameOffset) : std::nullopt;

            // Get the size of the data for this property.
            size_t skip = 0;
            try {
                skip = GetSkipSize(epi.nonStructType.InType);
            }
            catch (std::runtime_error) {
                throw std::runtime_error("Invalid skip size");
            }

            // If this is not the property we are looking for, skip its data.
            if ((currentName.has_value() == false) || (currentName.value() != propertyName))
            {
                pb += skip;
                continue;
            }

            // Found the matching property.
            if (pb + skip > mpEventDataEnd)
            {
                throw std::runtime_error("Insufficient data for property");
            }

            // Check that the property's underlying type matches the expected type T.
            if constexpr (std::is_same_v<T, int8_t>)
            {
                if (epi.nonStructType.InType != TDH_INTYPE_INT8)
                    throw std::runtime_error("Type mismatch for property: expected int8_t");
                return *reinterpret_cast<const int8_t*>(pb);
            }
            else if constexpr (std::is_same_v<T, uint8_t>)
            {
                if (epi.nonStructType.InType != TDH_INTYPE_UINT8)
                    throw std::runtime_error("Type mismatch for property: expected uint8_t");
                return *reinterpret_cast<const uint8_t*>(pb);
            }
            else if constexpr (std::is_same_v<T, int16_t>)
            {
                if (epi.nonStructType.InType != TDH_INTYPE_INT16)
                    throw std::runtime_error("Type mismatch for property: expected int16_t");
                return *reinterpret_cast<const int16_t*>(pb);
            }
            else if constexpr (std::is_same_v<T, uint16_t>)
            {
                if (epi.nonStructType.InType != TDH_INTYPE_UINT16)
                    throw std::runtime_error("Type mismatch for property: expected uint16_t");
                return *reinterpret_cast<const uint16_t*>(pb);
            }
            else if constexpr (std::is_same_v<T, int32_t>)
            {
                if (epi.nonStructType.InType != TDH_INTYPE_INT32)
                    throw std::runtime_error("Type mismatch for property: expected int32_t");
                return *reinterpret_cast<const int32_t*>(pb);
            }
            else if constexpr (std::is_same_v<T, uint32_t>)
            {
                if (epi.nonStructType.InType != TDH_INTYPE_UINT32 &&
                    epi.nonStructType.InType != TDH_INTYPE_HEXINT32)
                {
                    throw std::runtime_error("Type mismatch for property: expected uint32_t");
                }
                return *reinterpret_cast<const uint32_t*>(pb);
            }
            else if constexpr (std::is_same_v<T, int64_t>)
            {
                if (epi.nonStructType.InType != TDH_INTYPE_INT64)
                    throw std::runtime_error("Type mismatch for property: expected int64_t");
                return *reinterpret_cast<const int64_t*>(pb);
            }
            else if constexpr (std::is_same_v<T, uint64_t>)
            {
                if (epi.nonStructType.InType != TDH_INTYPE_UINT64 &&
                    epi.nonStructType.InType != TDH_INTYPE_HEXINT64)
                {
                    throw std::runtime_error("Type mismatch for property: expected uint32_t");
                }
                return *reinterpret_cast<const uint64_t*>(pb);
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                if (epi.nonStructType.InType != TDH_INTYPE_FLOAT)
                    throw std::runtime_error("Type mismatch for property: expected float");
                return *reinterpret_cast<const float*>(pb);
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                if (epi.nonStructType.InType != TDH_INTYPE_DOUBLE)
                    throw std::runtime_error("Type mismatch for property: expected double");
                return *reinterpret_cast<const double*>(pb);
            }
            else
            {
                static_assert(dependent_false<T>::value, "Unsupported type in GetNumericPropertyValue");
            }
        }

        // If we reach here, the property was not found.
        throw std::runtime_error("Property not found");
    }
private:

    bool SetupTraceLoggingInfoBuffer();
    std::optional<std::wstring> GetTraceEventInfoString(unsigned offset) const;
    
    size_t GetSkipSize(unsigned tdhType) const;

    TDH_CONTEXT  mTdhContext[1] = {};
    EVENT_RECORD* mpEventRecord;                   // Event currently being processed
    BYTE const* mpEventData;                     // Position of the next byte of event data to be consumed.
    BYTE const* mpEventDataEnd;                  // Position of the end of the event data.
    BYTE mPointerSize;
    bool mCurrentEventValid;

    LARGE_INTEGER       mEventTimeStamp;
    std::vector<BYTE>   mTraceEventInfoBuffer;        // Buffer for TRACE_EVENT_INFO data.
    TRACE_EVENT_INFO const* mpTraceEventInfo;
};
#include "../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include <initguid.h>
#include <evntcons.h>
#include <cguid.h>
#include <atlbase.h>
#include <comdef.h>
#include <objbase.h>
#include <relogger.h>
#include <iostream>
#include <optional>
#include <format>
#include "CliOptions.h"

DEFINE_GUID(IID_ITraceRelogger, 0xF754AD43, 0x3BCC, 0x4286, 0x80, 0x09, 0x9C, 0x5D, 0xA2, 0x14, 0xE8, 0x4E); // {F754AD43-3BCC-4286-8009-9C5DA214E84E}
DEFINE_GUID(IID_ITraceEventCallback, 0x3ED25501, 0x593F, 0x43E9, 0x8F, 0x38, 0x3A, 0xB4, 0x6F, 0x5A, 0x4A, 0x52); // {3ED25501-593F-43E9-8F38-3AB46F5A4A52}

enum class Mode
{
    Analysis,
    Trim,
};

class EventCallback : public ITraceEventCallback
{
private:
    // COM support
    DWORD ref_count_ = 0;
    // operation mode
    Mode mode_;
    // trim region
    std::optional<std::pair<uint64_t, uint64_t>> trimRange_;
    // analysis stats
    int eventCount_ = 0;
    std::optional<uint64_t> firstTimestamp_;
    uint64_t lastTimestamp_ = 0;

public:
    EventCallback(bool infoOnly)
        :
        mode_{ infoOnly ? Mode::Analysis : Mode::Trim }
    {}
    STDMETHODIMP QueryInterface(const IID& iid, void** pObj)
    {
        if (iid == IID_IUnknown) {
            *pObj = dynamic_cast<IUnknown*>(this);
        }
        else if (iid == IID_ITraceEventCallback) {
            *pObj = dynamic_cast<ITraceEventCallback*>(this);
        }
        else {
            *pObj = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef(void)
    {
        return InterlockedIncrement(&ref_count_);
    }
    STDMETHODIMP_(ULONG) Release()
    {
        ULONG ucount = InterlockedDecrement(&ref_count_);
        if (ucount == 0) {
            delete this;
        }
        return ucount;
    }
    HRESULT STDMETHODCALLTYPE OnBeginProcessTrace(ITraceEvent* pHeaderEvent, ITraceRelogger* pRelogger)
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnEvent(ITraceEvent* pEvent, ITraceRelogger* pRelogger)
    {
        EVENT_RECORD* pEvt = nullptr;
        pEvent->GetEventRecord(&pEvt);
        const auto ts = (uint64_t)pEvt->EventHeader.TimeStamp.QuadPart;
        if (!firstTimestamp_) {
            firstTimestamp_ = ts;
        }
        lastTimestamp_ = ts;
        eventCount_++;
        if (mode_ == Mode::Trim) {
            if (trimRange_) {
                if (ts < trimRange_->first || ts > trimRange_->second) {
                    return S_OK;
                }
            }
            pRelogger->Inject(pEvent);
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnFinalizeProcessTrace(ITraceRelogger* pRelogger)
    {
        return S_OK;
    }
    int GetEventCount() const
    {
        return eventCount_;
    }
    std::pair<uint64_t, uint64_t> GetTimestampRange() const
    {
        return { firstTimestamp_.value_or(0), lastTimestamp_};
    }
    void SetTrimRange(std::pair<uint64_t, uint64_t> range)
    {
        trimRange_ = range;
    }
};

class TempFile
{
public:
    operator const CComBSTR& () const
    {
        return name_;
    }
    ~TempFile()
    {
        std::filesystem::remove((const wchar_t*)name_);
    }
private:
    CComBSTR name_ = "null-log.etl.tmp";
};


int main(int argc, const char** argv)
{
    // parse command line, return with error code from CLI11 if running as app
    if (auto e = clio::Options::Init(argc, argv)) {
        return *e;
    }
    auto& opt = clio::Options::Get();

    std::locale::global(std::locale("en_US.UTF-8"));

    if (auto hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); FAILED(hr)) {
        std::cout << "Failed to init COM" << std::endl;
        return -1;
    }

    ITraceRelogger* pRelogger = nullptr;
    if (auto hr = CoCreateInstance(CLSID_TraceRelogger, 0, CLSCTX_INPROC_SERVER, IID_ITraceRelogger, reinterpret_cast<void**>(&pRelogger));
        FAILED(hr)) {
        std::cout << "Failed to create trace relogger instance" << std::endl;
        return -1;
    }

    TRACEHANDLE relogTraceHandle;
    const CComBSTR inputEtlName = (*opt.inputFile).c_str();
    if (auto hr = pRelogger->AddLogfileTraceStream(inputEtlName, nullptr, &relogTraceHandle); FAILED(hr)) {
        std::cout << "Failed to add logfile: " << *opt.inputFile << std::endl;
    }

    // we must output to etl file no matter what
    // create a dummy temp etl if user did not specify output
    CComBSTR outputEtlName;
    std::optional<TempFile> temp;
    if (opt.outputFile) {
        outputEtlName = (*opt.outputFile).c_str();
    }
    else {
        temp.emplace();
        outputEtlName = *temp;
    }
    if (auto hr = pRelogger->SetOutputFilename(outputEtlName); FAILED(hr)) {
        std::cout << "Failed to set output file: " << *opt.outputFile << std::endl;
    }

    EventCallback* pCallbackProcessor = new EventCallback(!opt.outputFile);
    if (auto hr = pRelogger->RegisterCallback(pCallbackProcessor); FAILED(hr)) {
        std::cout << "Failed to register callback" << std::endl;
    }

    if (opt.trimRange) {
        pCallbackProcessor->SetTrimRange(*opt.trimRange);
    }

    if (auto hr = pRelogger->ProcessTrace(); FAILED(hr)) {
        std::cout << "Failed to process trace" << std::endl;
    }

    std::cout << "Processed " << pCallbackProcessor->GetEventCount() << std::endl;
    const auto tsr = pCallbackProcessor->GetTimestampRange();
    std::cout << std::format("Timestamp range {:L}-{:L} (duration: {:L})", tsr.first, tsr.second, tsr.second - tsr.first) << std::endl;

    return 0;
}

namespace pmon::util::log
{
    std::shared_ptr<class IChannel> GetDefaultChannel() noexcept
    {
        return {};
    }
}
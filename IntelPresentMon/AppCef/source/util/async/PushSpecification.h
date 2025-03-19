// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include <Core/source/kernel/Kernel.h>
#include "../CefValues.h"
#include "../MakeOverlaySpec.h"

namespace p2c::client::util::async
{
    namespace
    {
        gfx::Color ColorFromV8(const CefRefPtr<CefValue>& rgba)
        {
            return gfx::Color::FromBytes(
                Traverse(rgba)["r"],
                Traverse(rgba)["g"],
                Traverse(rgba)["b"]
            ).WithAlpha(Traverse(rgba)["a"]);
        }
    }

    class PushSpecification : public AsyncEndpoint
    {
    public:
        static constexpr std::string GetKey() { return "pushSpecification"; }
        PushSpecification() : AsyncEndpoint{ AsyncEndpoint::Environment::KernelTask } {}
        // {very-complicated-spec-object} => null
        Result ExecuteOnKernelTask(uint64_t uid, CefRefPtr<CefValue> pArgObj, kern::Kernel& kernel) const override
        {
            auto args = Traverse(pArgObj);
            kernel.UpdateInjection(
                args["preferences"]["enableFlashInjection"],
                args["pid"].AsOptional<uint32_t>(),
                args["preferences"]["flashInjectionBackgroundEnable"],
                ColorFromV8(args["preferences"]["flashInjectionColor"]),
                ColorFromV8(args["preferences"]["flashInjectionBackgroundColor"]),
                args["preferences"]["flashInjectionSize"],
                args["preferences"]["flashInjectionRightShift"]);
            if (args["pid"].IsNull()) {
                kernel.ClearOverlay();
            }
            else {
                kernel.PushSpec(MakeOverlaySpec(std::move(pArgObj)));
            }
            return Result{ true, CefValueNull() };
        }
    };
}
#pragma once
#include "../Core/source/infra/opt/Framework.h"
#include <string>
#include <vector>

namespace pmon::ipc::mock::opt
{
	namespace impl
	{
		using namespace p2c::infra::opt::impl;
		struct OptionsStruct : public OptionStructBase_
		{
			// add options and switches here to augment the CLI
			Flag testF{ "--test-f", "Output test string from test function f() in ipc lib" };
			Flag basicMessage{ "--basic-message", "Do minimal IPC test transferring string across shared memory", [this](CLI::Option* p) {
				p->excludes(testF.opt()); } };

		protected:
			// edit application name and description here
			std::string GetName() const override
			{
				return "Interprocess Mock";
			}
			std::string GetDescription() const override
			{
				return "Application to mock interprocess communication endpoints for testing";
			}
		};
	}

	inline void init(int argc, char** argv) { impl::App<impl::OptionsStruct>::Init(argc, argv); }
	inline auto& get() { return impl::App<impl::OptionsStruct>::GetOptions(); }
}
#include "../Interprocess/source/act/AsyncActionCollection.h"
#include "AsyncActions.h"
// #include "NamedPipeServer.h" for Context

namespace pmon::svc::acts
{
	namespace {
		using namespace pmon::ipc;
		static struct ActionRegistrator {
			ActionRegistrator() {
				auto& actions = act::AsyncActionCollection<ServiceExecutionContext>::Get();
				actions.AddAction<OpenSessionAction>();
			}
		} registrator_;
	}
}
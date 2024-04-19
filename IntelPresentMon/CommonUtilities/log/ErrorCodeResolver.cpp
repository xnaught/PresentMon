#include "ErrorCodeResolver.h"


namespace pmon::util::log
{
	void ErrorCodeResolver::AddProvider(std::unique_ptr<IErrorCodeProvider> pProvider)
	{
		providerMap_[pProvider->GetTargetType()] = std::move(pProvider);
	}

	ErrorCodeResolver::Strings ErrorCodeResolver::Resolve(std::type_index typeIdx, const ErrorCode& c) const noexcept
	{
		try {
			if (auto i = providerMap_.find(typeIdx); i != providerMap_.end()) {
				return i->second->Resolve(c);
			}
		}
		catch (...) {}
		return {};
	}
}
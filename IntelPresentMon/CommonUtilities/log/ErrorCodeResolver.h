#pragma once
#include <unordered_map>
#include <typeindex>
#include <memory>
#include "IErrorCodeResolver.h"
#include "IErrorCodeProvider.h"

namespace pmon::util::log
{
	class ErrorCodeResolver : public IErrorCodeResolver
	{
	public:
		void AddProvider(std::unique_ptr<IErrorCodeProvider>);
		Strings Resolve(std::type_index typeIdx, const ErrorCode&) const noexcept override;
	private:
		std::unordered_map<std::type_index, std::unique_ptr<IErrorCodeProvider>> providerMap_;
	};
}
#include "CliFramework.h"
#include "../str/String.h"
#include <Windows.h>

namespace pmon::util::cli
{
	OptionsContainer::OptionsContainer(const char* description, const char* name) : app_{ description, name } {}
	std::string OptionsContainer::GetName() const
	{
		return app_.get_name();
	}
	void OptionsContainer::Finalize()
	{
		app_.parse(str::ToNarrow(GetCommandLineW()), true);
		finalized_ = true;
	}
	int OptionsContainer::Exit(const CLI::ParseError& e)
	{
		return app_.exit(e);
	}

	Flag::Flag(OptionsContainer* pParent, std::string names, std::string description)
	{
		pOption_ = pParent->app_.add_flag(std::move(names), data_, std::move(description));
	}
	bool Flag::operator*() const
	{
		return bool(*this);
	}
	Flag::operator bool() const
	{
		return (bool)*pOption_;
	}
	bool Flag::operator!() const
	{
		return !bool(*this);
	}
}
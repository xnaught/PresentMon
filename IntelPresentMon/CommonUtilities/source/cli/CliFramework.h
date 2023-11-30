#pragma once

#pragma warning(push)
#pragma warning(disable : 26827)
#include <CLI/CLI.hpp>
#pragma warning(pop)

#include <optional>
#include <cassert>

namespace pmon::util::cli
{
	class OptionsContainer
	{
		template<typename T> friend class Option;
		friend class Flag;
	public:
		OptionsContainer(const char* description, const char* name);
		void Finalize();
		int Exit(const CLI::ParseError& e);
	protected:
		bool finalized_ = false;
		CLI::App app_;
	};

	template<class T>
	class OptionsBase : public OptionsContainer
	{
	public:
		OptionsBase() : OptionsContainer{ T::description, T::name } {}
		static const T& Get()
		{
			auto& opts = Get_();
			assert(opts.finalized_);
			return opts;
		}
		static std::optional<int> Init()
		{
			auto& opts = Get_();
			try {
				opts.Finalize();
				return {};
			}
			catch (const CLI::ParseError& e) {
				return opts.Exit(e);
			}
		}
	public:
		static T& Get_()
		{
			static T opts;
			return opts;
		}
	};

	template<typename T>
	class Option
	{
	public:
		Option(OptionsContainer* pParent, std::string names, const T& defaultValue, std::string description)
			:
			data_{ defaultValue }
		{
			pOption_ = pParent->app_.add_option(std::move(names), data_, std::move(description));
		}
		const T& operator*() const
		{
			return data_;
		}
		operator bool() const
		{
			return (bool)*pOption_;
		}
		bool operator!() const
		{
			return !bool(*this);
		}
	private:
		T data_;
		CLI::Option* pOption_ = nullptr;
	};

	class Flag
	{
	public:
		Flag(OptionsContainer* pParent, std::string names, std::string description);
		bool operator*() const;
		operator bool() const;
		bool operator!() const;
	private:
		bool data_ = false;
		CLI::Option* pOption_ = nullptr;
	};
}
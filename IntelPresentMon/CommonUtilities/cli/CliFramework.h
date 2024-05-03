#pragma once

#pragma warning(push)
#pragma warning(disable : 26827)
#include <CLI/CLI.hpp>
#pragma warning(pop)

#include <optional>
#include <cassert>
#include <string>
#include <span>

namespace pmon::util::cli
{
	class OptionsContainer
	{
		template<typename T> friend class Option;
		friend class Flag;
	public:
		OptionsContainer(const char* description, const char* name);
		std::string GetName() const;
	protected:
		// types
		class ConvertedNarrowOptions_
		{
		public:
			ConvertedNarrowOptions_(int argc, const wchar_t* const* wargv);
			~ConvertedNarrowOptions_();
			const char* const* GetRawPointerArray() const;

			ConvertedNarrowOptions_(const ConvertedNarrowOptions_&) = delete;
			ConvertedNarrowOptions_& operator=(const ConvertedNarrowOptions_&) = delete;
			ConvertedNarrowOptions_(ConvertedNarrowOptions_&&) = delete;
			ConvertedNarrowOptions_& operator=(ConvertedNarrowOptions_&&) = delete;

		private:
			std::vector<char*> stringPointerArray;
		};
		// functions
		void Finalize_(int argc, const char* const* argv);
		int Exit_(const CLI::ParseError& e);
		// data
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
		static std::optional<int> Init(int argc, const wchar_t* const* wargv)
		{
			ConvertedNarrowOptions_ narrowArgs{ argc, wargv };
			return Init(argc, narrowArgs.GetRawPointerArray());
		}
		static std::optional<int> Init(int argc, const char* const* argv)
		{
			auto& opts = Get_();
			try {
				opts.Finalize_(argc, argv);
				return {};
			}
			catch (const CLI::ParseError& e) {
				return opts.Exit_(e);
			}
		}
		static bool IsInitialized()
		{
			return Get_().finalized_;
		}
	private:
		static T& Get_()
		{
			static T opts;
			return opts;
		}
	};

	template<typename T>
	class Option
	{
		friend struct RuleBase_;
	public:
		template<class U>
		Option(OptionsContainer* pParent, std::string names, const T& defaultValue, std::string description, U&& customizer)
			:
			Option{ pParent, std::move(names), defaultValue, std::move(description) }
		{
			if constexpr (std::is_base_of_v<CLI::Validator, std::decay_t<U>> ) {
				pOption_->check(std::forward<U>(customizer));
			}
			else {
				customizer(pOption_);
			}
		}
		Option(OptionsContainer* pParent, std::string names, const T& defaultValue, std::string description)
			:
			data_{ defaultValue }
		{
			pOption_ = pParent->app_.add_option(std::move(names), data_, std::move(description));
		}
		Option(const Option&) = delete;
		Option& operator=(const Option&) = delete;
		Option(Option&&) = delete;
		Option& operator=(Option&&) = delete;
		~Option() = default;
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
		std::optional<T> AsOptional() const
		{
			if (*this) {
				return **this;
			}
			return {};
		}
	private:
		T data_;
		CLI::Option* pOption_ = nullptr;
	};

	class Flag
	{
		friend struct RuleBase_;
	public:
		Flag(OptionsContainer* pParent, std::string names, std::string description);
		bool operator*() const;
		operator bool() const;
		bool operator!() const;
	private:
		bool data_ = false;
		CLI::Option* pOption_ = nullptr;
	};

	struct RuleBase_
	{
	protected:
		template<class T>
		CLI::Option* GetOption(const T& el) const { return el.pOption_; }
	};

	class MutualExclusion : RuleBase_
	{
	public:
		template<class...T>
		MutualExclusion(const T&...elements)
		{
			Exclude(elements...);
		}
	private:
		template<class T, class...Rest>
		void Exclude(const T& pivot, const Rest&...rest) const
		{
			if constexpr (sizeof...(rest) > 0) {
				GetOption(pivot)->excludes(GetOption(rest)...);
				Exclude(rest...);
			}
		}
	};

	class MutualInclusion : RuleBase_
	{
	public:
		template<class...T>
		MutualInclusion(const T&...elements)
		{
			Include(elements...);
		}
	private:
		template<class T, class...Rest>
		void Include(const T& pivot, const Rest&...rest) const
		{
			if constexpr (sizeof...(rest) > 0) {
				GetOption(pivot)->needs(GetOption(rest)...);
				Include(rest...);
			}
		}
	};
}
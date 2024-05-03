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
	// option elements inherit from this so we can enumerate options at runtime
	// and iterate over them (for child process forwarding etc.)
	// TODO: check how this interacts with std::vector etc. containers
	class OptionsElement_
	{
		friend struct RuleBase_;
		friend class OptionsContainer;
	public:
		virtual ~OptionsElement_() = default;
		virtual operator bool() const = 0;
	protected:
		std::function<std::string(std::string)> GetCaptureCallback_();
		void SetName_(std::string name);
		void EnableQuote_();
	private:
		std::string name_;
		std::string raw_;
		bool forwarding_ = true;
		bool needsQuote_ = false;
	};

	class OptionsContainer
	{
		template<typename T> friend class Option;
		friend class Flag;
	public:
		OptionsContainer(const char* description, const char* name);
		std::string GetName() const;
		std::vector<std::pair<std::string, std::string>> GetForwardedOptions() const;
	private:
		void RegisterElement_(OptionsElement_* pElement);
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
		std::vector<OptionsElement_*> elementPtrs_;
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
	class Option : public OptionsElement_
	{
		friend struct RuleBase_;
	public:
		template<class U>
		Option(OptionsContainer* pParent, std::string names, const T& defaultValue, std::string description, U&& customizer)
			:
			data_{ defaultValue }
		{
			// create the option
			pOption_ = pParent->app_.add_option(std::move(names), data_, std::move(description));
			if constexpr (std::is_base_of_v<CLI::Validator, std::decay_t<U>> ) {
				if (customizer.get_modifying()) {
					pOption_->transform(std::forward<U>(customizer));
				}
				else {
					pOption_->check(std::forward<U>(customizer));
				}
			}
			else {
				customizer(pOption_);
			}
			// capture main name for the option (used when forwarding)
			SetName_(pOption_->get_name());
			// surround option value in quotes when forwarding string options
			if constexpr (std::same_as<std::string, T>) {
				EnableQuote_();
			}
			// capture the raw input string
			pOption_->transform(GetCaptureCallback_());
			// register this element with the container dynamically
			pParent->RegisterElement_(this);
		}
		Option(OptionsContainer* pParent, std::string names, const T& defaultValue, std::string description)
			:
			data_{ defaultValue }
		{
			// create the option
			pOption_ = pParent->app_.add_option(std::move(names), data_, std::move(description));
			// capture main name for the option (used when forwarding)
			SetName_(pOption_->get_name());
			// surround option value in quotes when forwarding string options
			if constexpr (std::same_as<std::string, T>) {
				EnableQuote_();
			}
			// capture the raw input string
			pOption_->transform(GetCaptureCallback_());
			// register this element with the container dynamically
			pParent->RegisterElement_(this);
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
		operator bool() const final
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

	class Flag : public OptionsElement_
	{
		friend struct RuleBase_;
	public:
		Flag(OptionsContainer* pParent, std::string names, std::string description);
		bool operator*() const;
		operator bool() const final;
		bool operator!() const;
	private:
		bool data_ = false;
		CLI::Option* pOption_ = nullptr;
	};

	struct RuleBase_
	{
	protected:
		// TODO: could move pOption_ into OptionsElement_, would obviate this template
		template<class T>
		CLI::Option* GetOption_(T& el) const { return el.pOption_; }
		void SetForwarding_(OptionsElement_& el, bool forwarding = false) { el.forwarding_ = forwarding; }
	};

	class MutualExclusion : RuleBase_
	{
	public:
		template<class...T>
		MutualExclusion(T&...elements)
		{
			Exclude(elements...);
		}
	private:
		template<class T, class...Rest>
		void Exclude(T& pivot, const Rest&...rest) const
		{
			if constexpr (sizeof...(rest) > 0) {
				GetOption_(pivot)->excludes(GetOption_(rest)...);
				Exclude(rest...);
			}
		}
	};

	class MutualInclusion : RuleBase_
	{
	public:
		template<class...T>
		MutualInclusion(T&...elements)
		{
			Include(elements...);
		}
	private:
		template<class T, class...Rest>
		void Include(T& pivot, const Rest&...rest) const
		{
			if constexpr (sizeof...(rest) > 0) {
				GetOption_(pivot)->needs(GetOption_(rest)...);
				Include(rest...);
			}
		}
	};

	class NoForward : RuleBase_
	{
	public:
		template<class...T>
		NoForward(T&...elements)
		{
			(..., SetForwarding_(elements));
		}
	};
}
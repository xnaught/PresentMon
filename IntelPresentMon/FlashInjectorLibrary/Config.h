#pragma once

#include "NonCopyable.h"

#include <unordered_map>
#include <string>

#define GFXL_OPT_LOG_FILE				"LogFile"
#define GFXL_OPT_WAIT_FOR_USER_INPUT	"WaitForUserInput"
#define GFXL_OPT_ENABLE_DEBUG_LAYER		"EnableDebugLayer"
#define GFXL_OPT_OUTPUT_DIR				"OutputDir"

namespace GfxLayer
{
	using OptionMap = std::unordered_map<std::string, std::string>;
	class Options
	{
	public:
		Options()  = default;
		~Options() = default;

		void		Add(const std::string& name, const std::string& value = "");

		bool		GetFlag(const std::string& name) const;
		std::string GetString(const std::string& name) const;
		float		GetFloat(const std::string& name) const;

		void		Print() const;

	private:
		OptionMap	m_Map;
	};

	class ConfigParser: public NonCopyable
	{
	public:
		ConfigParser();
		~ConfigParser();

		std::string		GetConfigFile();
		void			SetConfigFile(const std::string& configFile);
		void			Parse();
		const Options&	GetOptions() const;

	private:
		std::string		m_ConfigFile;
		Options			m_Options;
	};
}
#pragma once

namespace pmon::gid
{
	inline constexpr const char* defaultControlPipeName = R"(\\.\pipe\presentmonsvcnamedpipe)";
	inline constexpr const char* defaultIntrospectionNsmName = R"(Global\pm2_bip_shm)";
	inline constexpr const char* defaultLogPipeBaseName = "pm2-svc-log";
}
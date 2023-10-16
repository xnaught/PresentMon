#pragma once

namespace pmid
{
	class Middleware
	{
	public:
		virtual ~Middleware() = default;
		virtual void Speak(char* buffer) const = 0;
	};
}
#pragma once
#include "../../../CommonUtilities/win/WinAPI.h"
#include <string_view>
#include <cereal/archives/binary.hpp>
#include <boost/asio.hpp>
#include <ranges>

namespace pmon::ipc::act
{
	namespace as = boost::asio;

	// used to encode both request and response packets
	template<class H, class P>
	void EncodeTransmissionPacket(const H& header, const P& payload, as::streambuf& buf)
	{
		assert(buf.size() == 0);
		std::ostream os{ &buf };
		cereal::BinaryOutputArchive ar{ os };
		// first we directly write bytes for the size of the body as a placeholder until we know how many are serialized
		const uint32_t placeholderSize = 'TEMP';
		os.write(reinterpret_cast<const char*>(&placeholderSize), sizeof(placeholderSize));
		// record how many bytes used for the serialization of the size
		const auto sizeSize = buf.size();
		// serialize the packet body
		ar(header, payload);
		// calculate size of body
		const auto payloadSize = uint32_t(buf.size() - sizeSize);
		// replace the placeholder with the actual body size
		auto bufSeq = buf.data();
		const auto iSize = const_cast<char*>(as::buffer_cast<const char*>(bufSeq));
		auto replacement = std::string_view{ reinterpret_cast<const char*>(&payloadSize), sizeof(payloadSize) };
		std::ranges::copy(replacement, iSize);
	}
}
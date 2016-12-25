// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef HAT_CORE_HEADERONLY_MODE
#include "utils.hpp"
#endif
#include <sstream>
#include <iomanip>
#ifndef HAT_CORE_HEADERONLY_MODE
#define LINKAGE_RESTRICTION 
#else
#define LINKAGE_RESTRICTION inline
#endif

namespace hat {
namespace core {

LINKAGE_RESTRICTION std::istream & getLineFromFile(std::istream & filestream, std::string & target)
{
	std::getline(filestream, target);
	if ((target.size() > 0) && (target.back() == '\r')) {
		target = target.substr(0, target.size() - 1);
	}
	return filestream;
}


LINKAGE_RESTRICTION std::string clearUTF8_byteOrderMark(std::string const & firstLineOfFile)
{	
	static const std::string UTF_8_BOM{char(0xEF), char(0xBB), char(0xBF)};
	if (firstLineOfFile.find(UTF_8_BOM) == 0) {
		return firstLineOfFile.substr(UTF_8_BOM.size(), firstLineOfFile.size());
	}
	return firstLineOfFile;
}

LINKAGE_RESTRICTION std::string escapeRawUTF8_forJson(std::string const & stringToProcess)
{
	using namespace std::string_literals;
	std::stringstream result;

	static char SINGLE_BYTE_CODEPOINT_MASK = char(0b10000000);
	static char MULTI_BYTE_CODEPOINT_MASK =  char(0b11000000);


	//UTF-8 characters do not need escaping for the TAU client. The escaped unicode is more portable (if I will decide to support other encodings, it will make the generated json not rely on the underlying encoding)
	static const auto DONT_ESCAPE_UTF_8_SYMBOLS = bool{true};
	
	for (size_t i = 0; i < stringToProcess.size(); ++i) {
		char current = stringToProcess[i];
		// Find out the bytecount of the pending character in UTF-8 encoding:
		auto isSingleByte = (SINGLE_BYTE_CODEPOINT_MASK & current) == 0;
		if (isSingleByte || DONT_ESCAPE_UTF_8_SYMBOLS) { //simple character, will put it as-is (escape only the '"' symbol, because it will screw up the json parsing)
			if ((current =='"') || (current =='\\')) {
				result << '\\';
			}
			result << current;
		} else {
			//TODO: maybe rewrite this code (use the standard multi-byte character conversions - http://en.cppreference.com/w/cpp/string/multibyte)
			size_t extraBytes = 0;
			char firstByteMask = char(0b00111111); //0x3f // this mask will represent the significant bits in the first byte of the multi-byte sequence
			{
				char currentTmp = current;
				while ((currentTmp & MULTI_BYTE_CODEPOINT_MASK) == MULTI_BYTE_CODEPOINT_MASK) {
					++extraBytes;
					currentTmp = currentTmp << 1;
					firstByteMask = firstByteMask >> 1; // Clear the significant bits mask of the first byte one by one
				}
			}
			long int resultUnicodeCodepoint = firstByteMask & current;
			const int ADDITIONAL_BYTES_MASK = 0b00111111;//0x3f // this mask represents the significant bits in the all the additional bytes of the multi-byte sequence
			for (size_t j = 1; j <= extraBytes; ++j) {
				// Clear the first two bits of the current character, add the 6 left bits it to the end of the accumulated value:
				if ((i+j) >= stringToProcess.size()) {
					throw std::runtime_error("Unicode parsing error: premature end of the string detected."); //TODO: add more details here
				}
				resultUnicodeCodepoint = (resultUnicodeCodepoint << 6) | (stringToProcess[i+j] & ADDITIONAL_BYTES_MASK);
			}
			i += extraBytes;
			if (resultUnicodeCodepoint <= 0xFFFF) { // the codepoint can be presented as '\u****'
				result << "\\u" << std::setw(4) << std::setfill('0') << std::hex << resultUnicodeCodepoint;
			} else {
				throw std::runtime_error("can't escape the unicode codepoint above 0xFFFF. The functionality not implemented.");
			}
			// There is a way to encode any unicode codepoint, but it does not work in android:
			//result << "\\u{" << std::hex << resultUnicodeCodepoint << '}';
		}
		
	}
	auto toReturn = result.str();
	return toReturn;
}

} //namespace core
} //namespace hat

#undef LINKAGE_RESTRICTION
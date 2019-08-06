
#pragma once

#include <string>

namespace CodeConvert
{
	std::string		ShiftJISfromUTF16(const std::wstring& utf16);
	std::wstring	UTF16fromShiftJIS(const std::string& sjis);

}	// namespace

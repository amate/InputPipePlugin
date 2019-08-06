

#include "CodeConvert.h"
#include <Windows.h>

namespace CodeConvert
{

	std::string ShiftJISfromUTF16(const std::wstring & utf16)
	{
		int requireBytes = ::WideCharToMultiByte(CP_ACP, 0, utf16.c_str(), utf16.length(), nullptr, 0, NULL, NULL);
		if (requireBytes > 0) {
			std::string sjis;
			sjis.resize(requireBytes);
			int ret = ::WideCharToMultiByte(CP_ACP, 0, utf16.c_str(), utf16.length(), (LPSTR)sjis.data(), requireBytes, NULL, NULL);
			if (ret > 0) {
				return sjis;
			}
		}
		return std::string();
	}

	std::wstring	UTF16fromShiftJIS(const std::string& sjis)
	{
		int requireChars = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, sjis.c_str(), sjis.length(), nullptr, 0);
		if (requireChars > 0) {
			std::wstring utf16;
			utf16.resize(requireChars);
			int ret = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, sjis.c_str(), sjis.length(), (LPWSTR)utf16.data(), requireChars);
			if (ret > 0) {
				return utf16;
			}
		}
		return std::wstring();
	}







}	// namespace CodeConvert
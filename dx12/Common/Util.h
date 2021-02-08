#pragma once

#include <Windows.h>
#include <string>
#include <assert.h>
#include <exception>
#include <comdef.h>

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNumber) : ErrorCode(hr), FunctionName(functionName), FileName(fileName), LineNumber(lineNumber)
	{};
	std::wstring ToString() const;
	HRESULT ErrorCode;
	std::wstring FunctionName;
	std::wstring FileName;
	int LineNumber;
};

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x) \
{ \
HRESULT hr__ = (x); \
std::wstring wfn = AnsiToWString(__FILE__); \
if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn,__LINE__); } \
}
#endif
/***************************************************************************
* Copyright (C) 2017, Deping Chen, cdp97531@sina.com
*
* All rights reserved.
* For permission requests, write to the publisher.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
***************************************************************************/
#pragma once
#include <stdexcept>
#include <string>
#include "ImpExpMacro.h"

void RMSG_API SetDebugInfo(const char* fileName);
void RMSG_API EnableDebugInfo(bool value);

// The following functions/classes are just for internal use, so they are not exported.
void __cdecl PrintDebugInfo(const char* szMsg, ...);
void __cdecl PrintDebugInfo(const char* fileName, int lineNo, const char* funcName, const char* szMsg, ...);
#define PRINT_DEBUG_INFO(msg, ...) \
	PrintDebugInfo(__FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__)
#define ASSERT_DEBUG_INFO(boolExp)														\
	if (!(boolExp)) {																	\
		PrintDebugInfo(__FILE__, __LINE__, __FUNCTION__, "Assertion failed:" #boolExp);	\
		throw std::logic_error("Program error!");																	\
	}
#define THROW_INVALID_DXF()													\
	PrintDebugInfo(__FILE__, __LINE__, __FUNCTION__, "Invalid DXF file.");	\
	throw std::runtime_error("Invalid DXF file.")																\


class TraceFunction
{
public:
	TraceFunction(const char* fileName, const char* funcName);
	~TraceFunction();

private:
	const char* m_FileName;
	const char* m_FuncName;
};

#define TRACE_FUNCTION(arguments, ...)										\
PrintDebugInfo(__FILE__, __LINE__, __FUNCTION__, arguments, __VA_ARGS__);	\
TraceFunction func##__LINE__(__FILE__, __FUNCTION__);

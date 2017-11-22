/***************************************************************************
* Copyright (C) 2017, Deping Chen, cdp97531@sina.com
*
* All rights reserved.
* For permission requests, write to the publisher.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
***************************************************************************/
#include "utility.h"
#include <stdarg.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <thread>
#include <sstream>
#include <mutex>
#include <thread>

std::unique_ptr<std::ofstream> g_DebugInfo;
std::string g_DebugInfoFile;
bool g_EnableDebugInfo = true;
std::mutex g_DebugInfoMutex;

void SetDebugInfo(const char* fileName)
{
	std::lock_guard<std::mutex> lock(g_DebugInfoMutex);
	if (!g_DebugInfo)
	{
		g_DebugInfo = std::make_unique<std::ofstream>();
	}
	if (g_DebugInfo->is_open())
		g_DebugInfo->close();
	g_DebugInfoFile = fileName;
}

void EnableDebugInfo(bool value)
{
	g_EnableDebugInfo = value;
}

std::ofstream* GetDebugInfoStream()
{
	std::lock_guard<std::mutex> lock(g_DebugInfoMutex);
	if (g_DebugInfoFile.empty())
	{
		g_DebugInfoFile = "RemoteMessageDebug.log";
	}
	if (g_EnableDebugInfo)
	{
		if (!g_DebugInfo)
		{
			g_DebugInfo = std::make_unique<std::ofstream>();
		}
		if (!g_DebugInfo->is_open())
			g_DebugInfo->open(g_DebugInfoFile);

		return g_DebugInfo.get();
	}
	return nullptr;
}

void __cdecl PrintDebugInfo(const char* szMsg, ...)
{
	auto debugInfo = GetDebugInfoStream();
	if (!debugInfo || !debugInfo->is_open())
		return;
	char buffer[1024];
	va_list args;
	va_start(args, szMsg);
	int len = vsnprintf(buffer, sizeof(buffer) - 1, szMsg, args);
	va_end(args);
	buffer[sizeof(buffer) - 1] = '\0';
	if (len != -1)
	{
		buffer[len] = '\0';
	}

	std::lock_guard<std::mutex> lock(g_DebugInfoMutex);
	std::this_thread::get_id()._To_text(*debugInfo);
	*debugInfo << ':' << buffer << std::endl;
}

void __cdecl PrintDebugInfo(const char* fileName, int lineNo, const char* funcName, const char* szMsg, ...)
{
	auto debugInfo = GetDebugInfoStream();
	if (!debugInfo || !debugInfo->is_open())
		return;
	char buffer[1024];
	va_list args;
	va_start(args, szMsg);
	int len = vsnprintf(buffer, sizeof(buffer) - 1, szMsg, args);
	va_end(args);
	buffer[sizeof(buffer) - 1] = '\0';
	if (len != -1)
	{
		buffer[len] = '\0';
	}
	const char* sep = strrchr(fileName, '\\');
	if (sep)
		fileName = sep + 1;

	std::lock_guard<std::mutex> lock(g_DebugInfoMutex);
	std::this_thread::get_id()._To_text(*debugInfo);
	*debugInfo << ':' << fileName << ':' << lineNo << '@' << funcName << ' ' << buffer << std::endl;
}

TraceFunction::TraceFunction(const char* fileName, const char* funcName)
	: m_FileName(fileName)
	, m_FuncName(funcName)
{
}

TraceFunction::~TraceFunction()
{
	PrintDebugInfo(m_FileName, 0, m_FuncName, "Exit");
}

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

#include <stdint.h>
#include <functional>
#include <vector>
#include "ImpExpMacro.h"

namespace RMsg
{

using MsgCategory = uint16_t;
using MsgClass = uint16_t;
using MsgResult = uint32_t;

class RMSG_API Message
{
public:
	Message();
	// Message can't be copied, because there can't be more than one Message instance for the same (m_SessionId, m_MsgId, m_IsReply)
	Message(const Message&) = delete;
	~Message();
	void CopyHeader(const Message&);
	void SerializeToVector(std::vector<char>& header);
	uint32_t ParseFromVector(const std::vector<char>& header);
	enum { KEEP_ALIVE_MSG = 0 };
	uint32_t m_SessionId;
	union
	{
		uint32_t m_MsgIdEx;
		struct
		{
			// m_MsgId == 0 is used for keepalive communication.
			uint32_t m_MsgId : 31;
			uint32_t m_IsReply : 1;
		};
	};
	// m_MsgCat and m_MsgCls are valid when m_IsReply is 0
	MsgCategory m_MsgCat;
	MsgClass m_MsgCls;
	MsgResult m_MsgRst;
	std::vector<char> m_Payload;
	mutable Message* m_pNext;
	static const size_t s_SizeOfHeader = sizeof(uint32_t)/*m_MsgIdEx*/ + sizeof(MsgCategory)/*m_MsgCat*/ + sizeof(MsgClass)/*m_MsgCls*/ + sizeof(MsgResult)/*m_MsgRst*/ + sizeof(uint32_t)/*payload size*/;
};

}

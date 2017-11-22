/***************************************************************************
* Copyright (C) 2017, Deping Chen, cdp97531@sina.com
*
* All rights reserved.
* For permission requests, write to the publisher.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
***************************************************************************/
#include "Message.h"
#include "utility.h"

Message::Message()
	: m_SessionId(0)
	, m_MsgIdEx(KEEP_ALIVE_MSG)
	, m_MsgCat(0)
	, m_MsgCls(0)
	, m_MsgRst(0)
	, m_pNext(nullptr)
{
}


Message::~Message()
{
}

void Message::CopyHeader(const Message & src)
{
	m_MsgIdEx = src.m_MsgIdEx;
	m_MsgCat = src.m_MsgCat;
	m_MsgCls = src.m_MsgCls;
	m_MsgRst = src.m_MsgRst;
}

void Message::SerializeToVector(std::vector<char>& header)
{
	ASSERT_DEBUG_INFO(header.size() == s_SizeOfHeader);
	char* buffer = header.data();
	memcpy(buffer, &m_MsgIdEx, sizeof(m_MsgIdEx));
	buffer += sizeof(m_MsgIdEx);
	memcpy(buffer, &m_MsgCat, sizeof(m_MsgCat));
	buffer += sizeof(m_MsgCat);
	memcpy(buffer, &m_MsgCls, sizeof(m_MsgCls));
	buffer += sizeof(m_MsgCls);
	memcpy(buffer, &m_MsgRst, sizeof(m_MsgRst));
	buffer += sizeof(m_MsgRst);
	// Assume payload size is less than 4G.
	uint32_t payloadSize = uint32_t(m_Payload.size());
	memcpy(buffer, &payloadSize, sizeof(payloadSize));
	buffer += sizeof(payloadSize);
}

uint32_t Message::ParseFromVector(const std::vector<char>& header)
{
	ASSERT_DEBUG_INFO(header.size() == s_SizeOfHeader);
	const char* buffer = header.data();
	memcpy(&m_MsgIdEx, buffer, sizeof(m_MsgIdEx));
	buffer += sizeof(m_MsgIdEx);
	memcpy(&m_MsgCat, buffer, sizeof(m_MsgCat));
	buffer += sizeof(m_MsgCat);
	memcpy(&m_MsgCls, buffer, sizeof(m_MsgCls));
	buffer += sizeof(m_MsgCls);
	memcpy(&m_MsgRst, buffer, sizeof(m_MsgRst));
	buffer += sizeof(m_MsgRst);
	uint32_t payloadSize;
	memcpy(&payloadSize, buffer, sizeof(payloadSize));
	buffer += sizeof(payloadSize);
	return payloadSize;
}

void AppendList(Message*& pList1, Message* pList2)
{
	if (!pList1)
	{
		pList1 = pList2;
		return;
	}
	if (!pList2)
	{
		return;
	}
	Message* curr = pList1;
	while (curr->m_pNext)
	{
		curr = curr->m_pNext;
	}
	curr->m_pNext = pList2;
}

Message* ReverseList(Message* pMsg)
{
	Message* prev = nullptr;
	Message* curr = pMsg;
	while (curr)
	{
		// keep next
		Message* next = curr->m_pNext;
		// revert
		curr->m_pNext = prev;
		// advance
		prev = curr;
		curr = next;
	}
	// here, curr is nullptr
	return prev;
}


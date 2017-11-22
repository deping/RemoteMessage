/***************************************************************************
* Copyright (C) 2017, Deping Chen, cdp97531@sina.com
*
* All rights reserved.
* For permission requests, write to the publisher.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
***************************************************************************/
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include "Session.h"
#include "utility.h"

std::atomic<uint32_t> Session::m_CurSessionId = 0;

Session::Session()
	: m_SessionId(++m_CurSessionId)
	, m_CurMsgId(0)
	, m_WriteMsgQueue(nullptr)
	, m_pToBeWritten(nullptr)
	, m_pBeingWritten(nullptr)
	, m_pBeingRead(nullptr)
	, m_DisconnectId(0)
	, m_socket(m_ioservice)
{
	m_ReadHeader.resize(Message::s_SizeOfHeader);
	m_WriteHeader.resize(Message::s_SizeOfHeader);
}

Session::~Session()
{
	Reset();
}

uint32_t Session::NextMsgId()
{
	const uint32_t MaxMsgId = 0x7FFFFFFF;
	uint32_t msgId = ++m_CurMsgId;
	// Skip 0
	if ((msgId & MaxMsgId) == 0)
	{
		msgId = ++m_CurMsgId;
	}
	// Make sure that MsgId is less than 0x80000000.
	return msgId & MaxMsgId;
}

// private method
Message * Session::NewMessage()
{
	Message* pMsg = new Message();
	pMsg->m_SessionId = m_SessionId;
	return pMsg;
}

void Session::HandleDisconnect(const boost::system::error_code &ec)
{
	const int code = ec.value();
	switch (code)
	{
	case boost::asio::error::eof:
	case boost::asio::error::connection_reset:
	case boost::asio::error::connection_aborted:
	case boost::asio::error::connection_refused:
	case boost::asio::error::bad_descriptor:
		{
			std::map<uint32_t, OnDisconnect> onDisconnects;
			{
				// Release lock ASAP.
				std::lock_guard<std::mutex> lock(m_DisconnectMutex);
				onDisconnects = m_OnDisconnects;
			}
			for (const auto& onDisconnect : onDisconnects)
			{
				onDisconnect.second();
			}
		}
		break;
	default:
		break;
	}
}

Message* Session::NewRequestMessage(MsgCategory msgCat, MsgClass msgCls)
{
	auto pMsg = new Message();
	pMsg->m_SessionId = m_SessionId;
	pMsg->m_MsgIdEx = NextMsgId();
	pMsg->m_MsgCat = msgCat;
	pMsg->m_MsgCls = msgCls;
	return pMsg;
}

Message* Session::NewReplyMessage(const Message * pMsg)
{
	auto pReply = new Message();
	pReply->m_SessionId = m_SessionId;
	pReply->m_MsgIdEx = pMsg->m_MsgIdEx;
	pReply->m_IsReply = 1;
	pReply->m_MsgCat = pMsg->m_MsgCat;
	pReply->m_MsgCls = pMsg->m_MsgCls;
	return pReply;
}

void Session::DeleteMessage(const Message* pMsg)
{
	delete pMsg;
}

void Session::RegisterMessageHandler(MsgCategory msgCat, MsgClass msgCls, const MessageHander& handler)
{
	std::lock_guard<std::mutex> lock(m_MessageMapMutex);
	m_MessageMap[msgCat][msgCls] = handler;
}

void Session::EnqueueNotice(Message* pMsg)
{
	ASSERT_DEBUG_INFO(pMsg && pMsg->m_SessionId == m_SessionId && !pMsg->m_pNext);
	Message* pHead = m_WriteMsgQueue.load(std::memory_order_relaxed);
	do
	{
		pMsg->m_pNext = pHead;
	} while (!m_WriteMsgQueue.compare_exchange_weak(pHead, pMsg, std::memory_order_release, std::memory_order_relaxed));
	PRINT_DEBUG_INFO("Message(Id=0x%X, Category=%d, Class=%d) is enqueued.", pMsg->m_MsgIdEx, pMsg->m_MsgCat, pMsg->m_MsgCls);

	TryToAsyncWriteMessage();
}

void Session::EnqueueRequest(Message* pMsg, const MessageHander& callback)
{
	ASSERT_DEBUG_INFO(pMsg && callback);
	{
		std::lock_guard<std::recursive_mutex> lock(m_WriteMsgMutex);
		m_Callbacks[pMsg->m_MsgId] = callback;
	}
	EnqueueNotice(pMsg);
}

void Session::CancelTicket(uint32_t msgId)
{
	// Get message list to be sent.
	Message* pHead = m_WriteMsgQueue.load(std::memory_order_relaxed);
	do
	{
	} while (pHead && !m_WriteMsgQueue.compare_exchange_weak(pHead, nullptr, std::memory_order_release, std::memory_order_relaxed));

	// We can't make sure the writing order is the same as message queue.
	// We just make sure messages is written one after another.

	std::lock_guard<std::recursive_mutex> lock(m_WriteMsgMutex);
	if (pHead)
	{
		pHead = ReverseList(pHead);
		AppendList(m_pToBeWritten, pHead);
	}

	// Remove and delete message which ID is msgId
	// Can't delete m_pBeingWritten because async_write need it.
	Message* pPrev = nullptr;
	Message* pMsg = m_pToBeWritten;
	while (pMsg)
	{
		if (pMsg->m_MsgId == msgId)
		{
			if (pPrev)
				pPrev->m_pNext = pMsg->m_pNext;
			else
				m_pToBeWritten = pMsg->m_pNext;
			DeleteMessage(pMsg);
		}
		pPrev = pMsg;
		pMsg = pMsg->m_pNext;
	}

	// Cancel callback if there is one
	m_Callbacks.erase(msgId);
}

void Session::Disconnect()
{
	// Any asynchronous send, receive or connect operations will be cancelled immediately,
	// and will complete with the boost::asio::error::operation_aborted error.
	if (m_socket.is_open())
	{
		m_socket.close();
	}
}

void Session::Run()
{
	// stopped condition: either through an explicit call to stop(), or due to running out of work.
	// async_read work is linked, so if there is no error, this will run forever.
	while (!m_ioservice.stopped())
	{
		m_ioservice.run_one();
	}
}

void Session::Stop()
{
	m_ioservice.stop();
}

uint32_t Session::RegisterDisconnect(OnDisconnect handler)
{
	if (!handler)
		return 0;
	std::lock_guard<std::mutex> lock(m_DisconnectMutex);
	m_OnDisconnects[++m_DisconnectId] = handler;
	return m_DisconnectId;
}

void Session::UnregisterDisconnect(uint32_t connectionId)
{
	std::lock_guard<std::mutex> lock(m_DisconnectMutex);
	m_OnDisconnects.erase(connectionId);
}

void Session::TryToAsyncWriteMessage()
{
	// Get message list to be sent.
	Message* pHead = m_WriteMsgQueue.load(std::memory_order_relaxed);
	do
	{
	} while (pHead && !m_WriteMsgQueue.compare_exchange_weak(pHead, nullptr, std::memory_order_release, std::memory_order_relaxed));

	// We can't make sure the writing order is the same as message queue.
	// We just make sure messages is written one after another.

	std::lock_guard<std::recursive_mutex> lock(m_WriteMsgMutex);
	if (pHead)
	{
		pHead = ReverseList(pHead);
		AppendList(m_pToBeWritten, pHead);
	}

	// Only no message is being written and some messages are waiting for writing, start async_write.
	if (!m_pBeingWritten && m_pToBeWritten)
	{
		m_pBeingWritten.reset(m_pToBeWritten);
		m_pToBeWritten = m_pToBeWritten->m_pNext;
		std::vector<boost::asio::const_buffer> buffers;
		m_pBeingWritten->SerializeToVector(m_WriteHeader);
		buffers.push_back(boost::asio::buffer(m_WriteHeader));
		buffers.push_back(boost::asio::buffer(m_pBeingWritten->m_Payload));

		boost::asio::async_write(m_socket, buffers,
			[this](const boost::system::error_code &ec, std::size_t bytes_transferred)
		{
			AsyncWriteHandler(ec, bytes_transferred);
		});
	}
}

void Session::Connect(const char* server, int port)
{
	boost::asio::ip::tcp::endpoint tcp(boost::asio::ip::address::from_string(server), port);
	m_socket.async_connect(tcp, [this](const boost::system::error_code &ec)
	{
		AsyncConnectHandler(ec);
	});
}

void Session::AsyncAcceptHandler(const boost::system::error_code &ec)
{
	if (ec)
	{
		PRINT_DEBUG_INFO(ec.message().c_str());
		return;
	}
	else
	{
		PRINT_DEBUG_INFO("socket connected.");
	}

	boost::asio::async_read(m_socket, boost::asio::buffer(m_ReadHeader), [this](const boost::system::error_code &ec, std::size_t bytes_transferred)
	{
		AsyncReadHeaderHandler(ec, bytes_transferred);
	});
}

void Session::AsyncWriteHandler(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
	HandleDisconnect(ec);
	if (ec)
	{
		PRINT_DEBUG_INFO(ec.message().c_str());
	}
	else
	{
		PRINT_DEBUG_INFO("Message(IdEx=0x%X, Category=%d, Class=%d) is written.", m_pBeingWritten->m_MsgIdEx, m_pBeingWritten->m_MsgCat, m_pBeingWritten->m_MsgCls);
	}

	{
		// Release lock ASAP
		std::lock_guard<std::recursive_mutex> lock(m_WriteMsgMutex);
		m_pBeingWritten.reset(nullptr);
	}

	TryToAsyncWriteMessage();
}

void Session::AsyncReadHeaderHandler(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
	HandleDisconnect(ec);
	if (ec)
	{
		PRINT_DEBUG_INFO(ec.message().c_str());
		return;
	}

	ASSERT_DEBUG_INFO(m_pBeingRead == nullptr);
	m_pBeingRead.reset(NewMessage());
	uint32_t payloadSize = m_pBeingRead->ParseFromVector(m_ReadHeader);
	if (payloadSize == 0)
	{
		ProcessMessageAndAsyncReadNext();
	}
	else
	{
		m_pBeingRead->m_Payload.resize(payloadSize);
		boost::asio::async_read(m_socket, boost::asio::buffer(m_pBeingRead->m_Payload), [this](const boost::system::error_code &ec, std::size_t bytes_transferred)
		{
			AsyncReadPayloadHandler(ec, bytes_transferred);
		});
	}
}

void Session::AsyncReadPayloadHandler(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
	HandleDisconnect(ec);
	ASSERT_DEBUG_INFO(m_pBeingRead);
	if (ec)
	{
		PRINT_DEBUG_INFO(ec.message().c_str());
		m_pBeingRead.reset(nullptr);
		return;
	}

	ProcessMessageAndAsyncReadNext();
}

void Session::Reset()
{
	Message* pHead = m_WriteMsgQueue.load(std::memory_order_relaxed);
	do
	{
	} while (pHead && !m_WriteMsgQueue.compare_exchange_weak(pHead, nullptr, std::memory_order_release, std::memory_order_relaxed));

	std::lock_guard<std::recursive_mutex> lock(m_WriteMsgMutex);
	if (pHead)
	{
		pHead = ReverseList(pHead);
		AppendList(m_pToBeWritten, pHead);
	}

	Message* pMsg;
	while (pMsg = m_pToBeWritten)
	{
		m_pToBeWritten = m_pToBeWritten->m_pNext;
		delete pMsg;
	}
}

Session::MessageHander Session::FindMessageHandler(MsgCategory msgCat, MsgClass msgCls)
{
	std::lock_guard<std::mutex> lock(m_MessageMapMutex);
	auto catIt = m_MessageMap.find(msgCat);
	if (catIt != m_MessageMap.end())
	{
		auto clsIt = catIt->second.find(msgCls);
		if (clsIt != catIt->second.end())
		{
			return clsIt->second;
		}
	}
	return nullptr;
}

Session::MessageHander Session::FindAndRemoveCallback(uint32_t msgId)
{
	std::lock_guard<std::recursive_mutex> lock(m_WriteMsgMutex);
	auto it = m_Callbacks.find(msgId);
	if (it != m_Callbacks.end())
	{
		auto handler = it->second;
		m_Callbacks.erase(it);
		return handler;
	}
	return nullptr;
}

void Session::ProcessMessageAndAsyncReadNext()
{
	PRINT_DEBUG_INFO("Message(IdEx=0x%X, Category=%d, Class=%d) is received.", m_pBeingRead->m_MsgIdEx, m_pBeingRead->m_MsgCat, m_pBeingRead->m_MsgCls);
	MessageHander handler;
	if (m_pBeingRead->m_IsReply)
	{
		handler = FindAndRemoveCallback(m_pBeingRead->m_MsgId);
	}
	else
	{
		handler = FindMessageHandler(m_pBeingRead->m_MsgCat, m_pBeingRead->m_MsgCls);
	}
	if (handler)
	{
		// hanlder take ownership of m_pBeingRead
		handler(std::move(m_pBeingRead));
	}
	else
	{
		PRINT_DEBUG_INFO("No handler for message(IdEx=0x%X, Category=%d, Class=%d).", m_pBeingRead->m_MsgIdEx, m_pBeingRead->m_MsgCat, m_pBeingRead->m_MsgCls);
	}
	m_pBeingRead.reset(nullptr);
	boost::asio::async_read(m_socket, boost::asio::buffer(m_ReadHeader), [this](const boost::system::error_code &ec, std::size_t bytes_transferred)
	{
		AsyncReadHeaderHandler(ec, bytes_transferred);
	});
}

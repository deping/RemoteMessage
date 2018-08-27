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

#include "Message.h"
#include <memory>
#include <list>
#include <atomic>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <mutex>
#pragma warning(disable:4800) // forcing value to bool 'true' or 'false' (performance warning)
#include <google/protobuf/message_lite.h>

namespace RMsg
{

template<typename T, typename TPbMsg>
using TMessageHandler = void(T::*)(const Message&, const std::shared_ptr<TPbMsg>&);

class RMSG_API Session
{
public:
	using MessageHander = std::function<void(std::unique_ptr<const Message>)>;
	using OnDisconnect = std::function<void()>;
	using OnConnect = OnDisconnect;

	template <typename T, typename TPbMsg>
	static typename Session::MessageHander CreateMessageHandler(TMessageHandler<T, TPbMsg> pMemFunc, T* pThis)
	{
		return [pMemFunc, pThis](std::unique_ptr<const Message> pMsg) {
			auto pPbMsg = std::make_shared<TPbMsg>();
			pPbMsg->ParseFromArray(pMsg->m_Payload.data(), int(pMsg->m_Payload.size()));
			Message msg;
			msg.CopyHeader(*pMsg.get());
			(pThis->*pMemFunc)(msg, pPbMsg);
		};
	}

	Session();
	// Session can't be copied.
	Session(const Session&) = delete;
	~Session();

	void Connect(const char* server, int port);
	void Disconnect();
	// Run is called in the message thread which calls read/writer handlers.
	void Run();
	void RunForever();
	// This can be called in another thread to stop handler processing.
	void Stop();

	uint32_t RegisterDisconnect(OnDisconnect handler);
	void UnregisterDisconnect(uint32_t connectionId);
	uint32_t RegisterConnect(OnConnect handler);
	void UnregisterConnect(uint32_t connectionId);

	uint32_t NextMsgId();
	void EnqueueNotice(Message* pMsg);
	void EnqueueRequest(Message* pMsg, const MessageHander& callback);
	template<typename TPbMsg>
	Message* ConvertPbRequest(const TPbMsg& pbMsg)
	{
		const auto& defTypeInfo = TPbMsg::TypeInfo::default_instance();
		Message* pMsg = NewRequestMessage(defTypeInfo.category(), defTypeInfo.method());
		pMsg->m_Payload.resize(pbMsg.ByteSizeLong());
		pbMsg.SerializeToArray(pMsg->m_Payload.data(), pMsg->m_Payload.size());
		return pMsg;
	}
	template<typename TPbMsg>
	Message* ConvertPbReply(const TPbMsg& pbMsg, const Message& request)
	{
		Message* pMsg = NewReplyMessage(&request);
		pMsg->m_Payload.resize(pbMsg.ByteSizeLong());
		pbMsg.SerializeToArray(pMsg->m_Payload.data(), pMsg->m_Payload.size());
		return pMsg;
	}
	template<typename TPbMsg>
	void EnqueuePbNotice(const TPbMsg& pbMsg)
	{
		Message* pMsg = ConvertPbRequest(pbMsg);
		EnqueueNotice(pMsg);
	}
	template<typename TPbMsg>
	void EnqueuePbReply(const TPbMsg& pbMsg, const Message& request)
	{
		Message* pMsg = ConvertPbReply(pbMsg, request);
		EnqueueNotice(pMsg);
	}
	template<typename T, typename TPbMsgRequest, typename TPbMsgReply>
	void EnqueuePbRequest(const TPbMsgRequest& pbMsg, TMessageHandler<T, TPbMsgReply> pMemFunc, T* pThis)
	{
		Message* pMsg = ConvertPbRequest(pbMsg);
		auto handler = CreateMessageHandler(pMemFunc, pThis);
		EnqueueRequest(pMsg, handler);
	}
	void CancelTicket(uint32_t msgId);
	// When you want to post a message, you must use NewRequestMessage/NewReplyMessage to create it.
	// Message can be created from different thread, but message belongs to Session.
	// Message which created by some Session must be posted by this session.
	Message* NewRequestMessage(MsgCategory msgCat, MsgClass msgCls);
	Message* NewReplyMessage(const Message* pMsg);
	// User programmer must use DeleteMessage to delete the return value of NewRequestMessage/NewReplyMessage
	// to avoid bug of new object in one CRT heap and delete object in another CRT heap.
	static void DeleteMessage(const Message* pMsg);

	void RegisterMessageHandler(MsgCategory msgCat, MsgClass msgCls, const MessageHander& handler);
	template <typename T, typename TPbMsg>
	void RegisterMessageHandler(TMessageHandler<T, TPbMsg> pMemFunc, T* pThis)
	{
		Session::MessageHander handler = CreateMessageHandler<T, TPbMsg>(pMemFunc, pThis);
		const auto& defTypeInfo = TPbMsg::TypeInfo::default_instance();
		RegisterMessageHandler(defTypeInfo.category(), defTypeInfo.method(), handler);
	}

private:
	friend class Server;
	uint32_t m_SessionId;
	std::atomic<uint32_t> m_CurMsgId;
	std::atomic<Message*> m_WriteMsgQueue;

	// Serialize the access to m_pToBeWritten and m_pMsgBeingWritten
	std::recursive_mutex m_WriteMsgMutex;
	Message* m_pToBeWritten;
	std::unique_ptr<Message> m_pBeingWritten;
	std::unique_ptr<Message> m_pBeingRead;
	std::vector<char> m_WriteHeader;
	std::vector<char> m_ReadHeader;

	std::mutex m_MessageMapMutex;
	std::map<MsgCategory, std::map<MsgClass, MessageHander>> m_MessageMap;
	std::map<uint32_t, MessageHander> m_Callbacks;

	std::mutex m_DisconnectMutex;
	std::map<uint32_t, OnDisconnect> m_OnDisconnects;
	uint32_t m_DisconnectId;
	std::map<uint32_t, OnConnect> m_OnConnects;
	uint32_t m_ConnectId;

	boost::asio::io_service m_ioservice;
	boost::asio::ip::tcp::socket m_socket;

	static std::atomic<uint32_t> m_CurSessionId;

	void AsyncAcceptHandler(const boost::system::error_code &ec);
#define AsyncConnectHandler AsyncAcceptHandler
	void AsyncWriteHandler(const boost::system::error_code &ec, std::size_t bytes_transferred);
	void AsyncReadHeaderHandler(const boost::system::error_code &ec, std::size_t bytes_transferred);
	void AsyncReadPayloadHandler(const boost::system::error_code &ec, std::size_t bytes_transferred);
	void TryToAsyncWriteMessage();
	void Reset();
	MessageHander FindMessageHandler(MsgCategory msgCat, MsgClass msgCls);
	MessageHander FindAndRemoveCallback(uint32_t msgId);
	void ProcessMessageAndAsyncReadNext();
	Message* NewMessage();
	void HandleDisconnect(const boost::system::error_code &ec);
	void HandleConnect();
};

}

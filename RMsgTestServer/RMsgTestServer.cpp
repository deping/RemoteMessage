/***************************************************************************
* Copyright (C) 2017, Deping Chen, cdp97531@sina.com
*
* All rights reserved.
* For permission requests, write to the publisher.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
***************************************************************************/
#include <iostream>
#include "Session.h"
#include "Server.h"
#include "PbSample.pb.h"
#include "utility.h"

using namespace RMsg;

class Worker
{
	Session* m_pSession;

public:
	Worker(Session* pSession)
		: m_pSession(pSession)
	{
	}

	void Process(const Message& msg, const std::shared_ptr<PbAddRequest>& pPbRequest)
	{
		std::cout << "PbAddRequest received." << std::endl;
		PbAddReply reply;
		reply.set_sum(pPbRequest->num1() + pPbRequest->num2());
		m_pSession->EnqueuePbReply(reply, msg);
	}

	void Process(const Message& msg, const std::shared_ptr<PbGreet>& pPbRequest)
	{
		std::cout << "PbGreet received." << std::endl;
		PbGreetReply reply;
		reply.set_text("I am fine!");
		m_pSession->EnqueuePbReply(reply, msg);
	}

	void ProcessFinish(const Message& msg, const std::shared_ptr<PbFinish>& pPbRequest)
	{
		std::cout << "PbFinish received." << std::endl;
		m_pSession->Disconnect();
	}

	static void OnDisconnect()
	{
		std::cout << "Socket disconnected." << std::endl;
	}
};

int main(int argc, char* argv[])
{
	// Disable to improve performance, Enable to debug communication error.
	EnableDebugInfo(true);
	Session s;
	Server ser;
	ser.Listen(9093, s);
	s.RegisterDisconnect(&Worker::OnDisconnect);
	Worker w(&s);
	s.RegisterMessageHandler<Worker, PbAddRequest>(&Worker::Process, &w);
	s.RegisterMessageHandler<Worker, PbGreet>(&Worker::Process, &w);
	s.RegisterMessageHandler(&Worker::ProcessFinish, &w);
	s.Run();
	return 0;
}
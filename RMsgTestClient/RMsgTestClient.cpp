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
#include "PbSample.pb.h"
#include "ProtobufMessageHandler.h"
#include "utility.h"

class Manager
{
	Session* m_pSession;

public:
	Manager(Session* pSession)
		: m_pSession(pSession)
	{
	}

	void GetSum(const Message& msg, const std::shared_ptr<PbAddReply>& pPbReply)
	{
		std::cout << pPbReply->sum() << std::endl;
	}

	void Greet(const Message& msg, const std::shared_ptr<PbGreetReply>& pPbReply)
	{
		std::cout << pPbReply->text() << std::endl;
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
	s.Connect("127.0.0.1", 9093);
	s.RegisterDisconnect(&Manager::OnDisconnect);
	Manager m(&s);
	PbGreet greet;
	greet.set_text("Are you OK?");
	s.EnqueuePbRequest(greet, &Manager::Greet, &m);
	PbAddRequest add;
	add.set_num1(1);
	add.set_num2(2);
	s.EnqueuePbRequest(add, &Manager::GetSum, &m);
	PbFinish finish;
	s.EnqueuePbNotice(finish);
	// s.Run should be in worker thread to avoid blocking the UI thread.
	s.Run();
    return 0;
}
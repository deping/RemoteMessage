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
#include "Session.h"

class RMSG_API Server
{
public:
	Server();
	~Server();
	// Listen will block until connection sets up.
	bool Listen(int port, Session& s);
	// This can be called in another thread to stop listening.
	void StopListen();

private:
	boost::asio::io_service m_ioservice;
	boost::asio::ip::tcp::acceptor m_acceptor;

	void Run();
};


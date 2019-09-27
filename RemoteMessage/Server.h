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

namespace RMsg
{

class RMSG_API Server
{
public:
	Server();
	~Server();
	// Listen will block until connection sets up.
    // if port is 0, get actual port by port().
	bool Listen(int port, Session& s);
	// This can be called in another thread to stop listening.
	void StopListen();
    int port() const { return m_port; }
    bool listened() const { return m_listened; }

private:
	boost::asio::io_service m_ioservice;
	boost::asio::ip::tcp::acceptor m_acceptor;
    int m_port;
    bool m_listened;
};

}

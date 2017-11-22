/***************************************************************************
* Copyright (C) 2017, Deping Chen, cdp97531@sina.com
*
* All rights reserved.
* For permission requests, write to the publisher.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
***************************************************************************/
#include "Server.h"
#include "utility.h"

#define CHECK_ERROR(statement, ret)				\
	statement;									\
	if (ec)										\
	{											\
		PRINT_DEBUG_INFO(ec.message().c_str());	\
		return ret;								\
	}

namespace RMsg
{

Server::Server()
	: m_acceptor(m_ioservice)
{
}


Server::~Server()
{
}

void Server::Run()
{
	// stopped condition: either through an explicit call to stop(), or due to running out of work.
	// async_read work is linked, so if there is no error, this will run forever.
	m_ioservice.reset();
	while (!m_ioservice.stopped())
	{
		m_ioservice.run_one();
	}
}

bool Server::Listen(int port, Session& s)
{
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
	m_acceptor.open(endpoint.protocol());
	boost::system::error_code ec;
	CHECK_ERROR(m_acceptor.bind(endpoint, ec), false);
	CHECK_ERROR(m_acceptor.listen(boost::asio::socket_base::max_connections, ec), false);
	m_acceptor.async_accept(s.m_socket, [&s](const boost::system::error_code &ec)
	{
		s.AsyncAcceptHandler(ec);
	});
	Run();
	return true;
}

void Server::StopListen()
{
	m_ioservice.stop();
}

}

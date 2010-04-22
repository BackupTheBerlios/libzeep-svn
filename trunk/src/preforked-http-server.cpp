//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/config.hpp"

#if SOAP_SERVER_HAS_PREFORK

#include <iostream>
#include <sstream>
#include <locale>

#include "zeep/http/preforked-server.hpp"
#include "zeep/http/connection.hpp"
#include "zeep/exception.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <sys/wait.h>

using namespace std;

namespace zeep { namespace http {

preforked_server_base::~preforked_server_base()
{
	m_io_service.stop();
	delete m_constructor;
}

void preforked_server_base::run()
{
	try
	{
		// create a socket pair to pass the file descriptors through
		int sockfd[2];
		int err = socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);
		if (err < 0)
			throw exception("Error creating socket pair: %s", strerror(errno));
	
		// fork
		m_pid = fork();
		if (m_pid < 0)
			throw exception("Error forking worker application: %s", strerror(errno));
	
		if (m_pid == 0)	// child process
		{
			close(sockfd[0]);
			
			// remove the blocks on the signal handlers
			sigset_t wait_mask;
			sigemptyset(&wait_mask);
			pthread_sigmask(SIG_SETMASK, &wait_mask, 0);
	
			// Time to construct the Server object
			auto_ptr<server> srvr(m_constructor->construct(m_address, m_port, -m_nr_of_threads));
			
			// run the server as a worker
			boost::thread t(boost::bind(&server::run, srvr.get()));

			// now start the processing loop passing on file descriptors read
			// from the parent process
			try
			{
				for (;;)
				{
					boost::shared_ptr<connection> conn(new connection(srvr->get_io_service(), *srvr));
					
					if (not read_socket_from_parent(sockfd[1], conn->get_socket()))
						break;
					
					conn->start();
				}
			}
			catch (std::exception& e)
			{
				std::cerr << "Exception caught: " << e.what() << std::endl;
				exit(1);
			}
			
			srvr->stop();
			t.join();
			
			exit(0);
		}
	
		// bind the address here
		boost::asio::ip::tcp::resolver resolver(m_io_service);
		boost::asio::ip::tcp::resolver::query
			query(m_address, boost::lexical_cast<string>(m_port));
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	
		m_acceptor.open(endpoint.protocol());
		m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		m_acceptor.bind(endpoint);
		m_acceptor.listen();
		m_acceptor.async_accept(m_socket,
			boost::bind(&preforked_server_base::handle_accept, this, boost::asio::placeholders::error));
		
		// start a thread to listen to the socket
		m_fd = sockfd[0];
		close(sockfd[1]);
	
		// keep the server at work until we call stop
		boost::asio::io_service::work work(m_io_service);
	
		boost::thread thread(
			boost::bind(&boost::asio::io_service::run, &m_io_service));
	
		thread.join();
	
		if (m_fd >= 0)
			close(m_fd);
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception caught in running server: " << e.what() << std::endl;
	}
}

//void preforked_server_base::run_worker(int fd, int nr_of_threads)
//{
//	// keep the server at work until we call stop
//	boost::asio::io_service::work work(m_io_service);
//	
//	for (int i = 0; i < nr_of_threads; ++i)
//	{
//		m_threads.create_thread(
//			boost::bind(&boost::asio::io_service::run, &m_io_service));
//	}
//
//	try
//	{
//		for (;;)
//		{
//			boost::shared_ptr<connection> conn(new connection(m_io_service, *this));
//			
//			if (not read_socket_from_parent(fd, conn->get_socket()))
//				break;
//			
//			conn->start();
//		}
//	}
//	catch (std::exception& e)
//	{
//		cerr << e.what() << endl;
//	}
//	
//	stop();
//
//	m_threads.join_all();
//}

bool preforked_server_base::read_socket_from_parent(int fd_socket, boost::asio::ip::tcp::socket& socket)
{
	typedef boost::asio::ip::tcp::socket::native_type native_type;

#if __APPLE__
	// macos is special...
	assert(CMSG_SPACE(sizeof(int)) == 16);
#endif

	struct msghdr	msg;
	union {
	  struct cmsghdr	cm;
#if __APPLE__
	  char				control[16];
#else
	  char				control[CMSG_SPACE(sizeof(int))];
#endif
	} control_un;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_name = nil;
	msg.msg_namelen = 0;

	boost::asio::ip::tcp::socket::endpoint_type peer_endpoint;

	struct iovec iov[1];
	iov[0].iov_base = peer_endpoint.data();
	iov[0].iov_len = peer_endpoint.capacity();
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	bool result = false;
	int n = recvmsg(fd_socket, &msg, 0);
	if (n >= 0)
	{
		peer_endpoint.resize(n);
	
		struct cmsghdr* cmptr CMSG_FIRSTHDR(&msg);
		if (cmptr != nil and cmptr->cmsg_len == CMSG_LEN(sizeof(int)))
		{
			if (cmptr->cmsg_level != SOL_SOCKET)
			 	cerr << "control level != SOL_SOCKET" << endl;
			else if (cmptr->cmsg_type != SCM_RIGHTS)
				cerr << "control type != SCM_RIGHTS";
			else
			{
				int fd = *(reinterpret_cast<native_type*>(CMSG_DATA(cmptr)));
				if (fd >= 0)
				{
					socket.assign(peer_endpoint.protocol(), fd);
					result = true;
				}
			}
		}
		else
			cerr << "No file descriptor was passed" << endl;
	}
	
	return result;
}

void preforked_server_base::write_socket_to_worker(int fd_socket, boost::asio::ip::tcp::socket& socket)
{
	typedef boost::asio::ip::tcp::socket::native_type native_type;
	
	struct msghdr msg;
	union {
		struct cmsghdr	cm;
#if __APPLE__
	  char				control[16];
#else
	  char				control[CMSG_SPACE(sizeof(native_type))];
#endif
//		char			control[CMSG_SPACE(sizeof(native_type))];
	} control_un;
	
	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	
	struct cmsghdr* cmptr = CMSG_FIRSTHDR(&msg);
	cmptr->cmsg_len = CMSG_LEN(sizeof(int));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_RIGHTS;
	*(reinterpret_cast<native_type*>(CMSG_DATA(cmptr))) = socket.native();
	
	msg.msg_name = nil;
	msg.msg_namelen = 0;
	
	struct iovec iov[1];
	iov[0].iov_base = socket.remote_endpoint().data();
	iov[0].iov_len = socket.remote_endpoint().size();
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	
	int err = sendmsg(fd_socket, &msg, 0);
	if (err < 0)
		throw exception("error passing filedescriptor: %s", strerror(errno));
}

void preforked_server_base::stop()
{
	m_io_service.stop();
	
	// kill the worker
	kill(m_pid, SIGTERM);
	
	// and wait until it died to avoid zombies
	int status;
	waitpid(m_pid, &status, 0);
	
	// close the socket to the worker
	close(m_fd);
	m_fd = -1;
}

void preforked_server_base::handle_accept(const boost::system::error_code& ec)
{
	if (not ec)
	{
		write_socket_to_worker(m_fd, m_socket);
		m_socket.close();
		m_acceptor.async_accept(m_socket,
			boost::bind(&preforked_server_base::handle_accept, this, boost::asio::placeholders::error));
	}
}

}
}

#endif

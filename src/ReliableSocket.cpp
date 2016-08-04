//
// Created by jyjia on 2016/7/12.
//


#include <thread>
#include <iostream>
#include <csignal>
#include <cstring>
#include "ReliableSocket.h"
#include "package/PackageConnectionCreate.h"
#include "package/PackageConnectionClose.h"
#include "package/PackageConnectionCreateR.h"
#include "package/PackageAck.h"

#define MAX(a, b) ((a) > (b))? (a) : (b)

#ifdef TARGET_LINUX
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX(a, b) ((a) > (b))? (a) : (b)

#endif // TARGET_LINUX

#ifdef TARGET_WIN32
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <ws2tcpip.h>
#endif // TARGET_WIN32

#ifdef TARGET_LINUX

#define set_socket_timeout(sock, ms) \
	{ \
		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &ms, sizeof(ms)) < 0) { \
		LOG(ERROR) << "SOCKET INITIALIZATION: set timeout fail"; \
	} \
	}
#define set_time_ms(tv, t) tv.tv_sec = (t) / 1000;tv.tv_usec = ((t) % 1000) * 1000


#endif // TARGET_LINUX

#ifdef TARGET_WIN32

//typedef int ssize_t;
#define msleep(s) Sleep(s)
#define set_socket_timeout(sock, ms) \
	{ \
		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&ms, sizeof(ms)) != 0) { \
		LOG(ERROR) << "SOCKET INITIALIZATION:set timeout fail"; \
	} \
	}
#define set_time_ms(tv, t) tv = (t)

#endif // TARGET_WIN32



#define min(a, b) ((a) < (b))? (a) : (b)

#define get_us(tv_a, tv_b) ((tv_a.tv_sec - tv_b.tv_sec) * 1000000 + (tv_a.tv_usec - tv_b.tv_usec))

ReliableSocket::ReliableSocket(int down_speed, int up_speed) {
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    download_speed = down_speed;
    upload_speed = up_speed;
    socket_type = RegularSocket;
    worker_running = false;
}

int ReliableSocket::bind_addr(const struct sockaddr *bind_addr) {
    memcpy(local_addr, bind_addr, remote_len);
    int r = bind(sock, bind_addr, sizeof(*bind_addr));
    if (r == 0)
        socket_type = BindedSocket;
    if (r == 0)
        LOG(INFO) << "Bind socket to address successfully\n";
    else
        LOG(INFO) << "Bind socket to address fail\n";
    return r;
}

int ReliableSocket::connect_addr(struct sockaddr *server_addr) {
	time_struct_ms s0, s1;
	timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
    remote_len = sizeof(*server_addr);
    remote_addr = new sockaddr();
    connected_addr = new sockaddr();
    memcpy(remote_addr, server_addr, remote_len);
    memcpy(connected_addr, remote_addr, remote_len);

    PackageConnectionCreate create = PackageConnectionCreate(download_speed);
    LOG(INFO) << "Sending request to create connection";
	get_time_ms(s0);
    sendto(sock, create.get_buf(), create.length(), 0, remote_addr, remote_len);
    PackageConnectionCreateR create_r = PackageConnectionCreateR();
    int count = 0;
    int r;
	fd_set read_set;
	while (count < 5) {
		FD_ZERO(&read_set);
		FD_SET(sock, &read_set);
        tv.tv_sec = 1;
		select(sock + 1, &read_set, NULL, NULL, &tv);
		if (FD_ISSET(sock, &read_set)) {
			r = recvfrom(sock, create_r.get_buf(), MAX_MTU, 0, NULL, NULL);
			if (r > 0)
				break;
		}
		LOG(ERROR) << "error connecting to server, retry\n";
		sendto(sock, create.get_buf(), create.length(), 0, remote_addr, remote_len);
		count++;
	}
    /* check if recvfrom timeout */
    if(count >= 5)
        return -1;
    send_speed = min(create_r.create_r_format->download_speed, upload_speed);
    get_time_ms(s1);
    long rtt = get_ms(s1, s0);
    set_rto(rtt);
    struct sockaddr_in* addr = (struct sockaddr_in*)connected_addr;
    addr->sin_port = htons(create_r.create_r_format->port);
    LOG(INFO) << "connection established to port " << create_r.create_r_format->port << "\n";

	/* create a local socket to get exit requests */
	local_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sockaddr_in *local_in = (sockaddr_in*)local_addr;
	inet_pton(AF_INET, "127.0.0.1", &local_in->sin_addr);
	bool binded = false;
	local_in->sin_family = AF_INET;
	for (int i = 2000; i < 2010; i++) {
		local_in->sin_port = htons(i);
		if (bind(local_sock, local_addr, remote_len) == 0) {
			binded = true;
			break;
		}
	}
	if (!binded) {
		LOG(ERROR) << "error binding the control port 2000-2010\n";
		return -1;
	}

    worker_running = true;
    start_thread_sender();
    start_thread_receiver();
    start_thread_heart_beat();
    start_thread_ackhandler();
    return 0;
}

ReliableSocket::~ReliableSocket() {
	send_pool.de_init();
    delete remote_addr;
    delete connected_addr;
}

void ReliableSocket::set_rto(long rtt) {
    rto = 1.5*rtt + 20;
    LOG(INFO) << "delay: " << rtt << "ms";
}

int ReliableSocket::close_addr() {
    PackageConnectionClose close_p = PackageConnectionClose();
    /* Does not need to ensure that the connection is really closed,
     * server will close them in a short time anyway */
    sendto(sock, close_p.get_buf(), close_p.length(), 0, connected_addr, remote_len);
    LOG(INFO) << "Sending request to close connection\n";
    worker_running = false;
    stop_all_thread();
    closesocket(sock);
    if (socket_type == AcceptableSocket) {
        //close all client socket
    }
    return 0;
}

void ReliableSocket::start_thread_sender() {
    /* Main Sender */
    std::thread([this]() {
		exit_sender = false;
        PackageData *package_send;
        PackageAckObject ack_object;
        int list_package_size = 1400 * 80;
        int list_package_ms = (double)list_package_size / send_speed / 1000 * 8;
        int list_package_count;
        time_struct_ms s0, s1;
        long time_pass = 0;
        while (worker_running) {
            list_package_count = 0;
            get_time_ms(s0);
            /* get from queue */
            while (list_package_count < list_package_size) {
                send_mutex.lock();
                while (send_pool.size() == 0) {
                    if (!worker_running) {
                        send_mutex.unlock();
                        LOG(INFO) << "sender thread exit";
						exit_sender = true;
                        return;
                    }
                    send_cv.wait(&send_mutex);
                }
                package_send = send_pool.pop();
                send_mutex.unlock();

                /* update una and send */
                package_send->data_format->una = recv_ack_id;
                list_package_count += package_send->length();
                sendto(sock, package_send->get_buf(), package_send->length(), 0,
                       connected_addr, remote_len);
//                std::cout << "Send package " << package_send->data_format->package_id << "\n";
                ack_object.data = package_send;
                get_time_ms(ack_object.time_send);

                /* add to ack watching list */
                ack_mutex.lock();
                ack_pool.push(ack_object);
                ack_mutex.unlock();
                ack_cv.signal();
                list_package_count += package_send->length();
            }
            get_time_ms(s1);
            time_pass = get_ms(s1, s0);
//            usleep(200);
            if (time_pass < list_package_ms) {
                msleep(list_package_ms - time_pass);
            }
        }
		exit_sender = true;
    }).detach();

    /* Resender */
    std::thread([this](){
		exit_resender = false;
        PackageData *package_send;
        PackageAckObject ack_object;
        int list_package_size = 1400 * 1;
        int list_package_us = list_package_size / send_speed * 8 * 4;
        int list_package_count;
        time_struct_ms s0, s1;
        long time_pass = 0;
        while (worker_running) {
            list_package_count = 0;
            get_time_ms(s0);
            /* get from queue */
            resend_mutex.lock();
            while (resend_pool.size() == 0){
                if (!worker_running) {
                    resend_mutex.unlock();
                    LOG(INFO) << "sender thread exit\n";
					exit_resender = true;
                    return;
                }
                resend_cv.wait(&resend_mutex);
            }
            package_send = resend_pool.front();
            resend_pool.pop();
            resend_mutex.unlock();

            package_send->data_format->una = recv_ack_id;
            list_package_count += package_send->length();
            sendto(sock, package_send->get_buf(), package_send->length(), 0,
                   connected_addr, remote_len);
//            std::cout << "Send package " << package_send->data_format->package_id << "\n";
            ack_object.data = package_send;
            get_time_ms(ack_object.time_send);

            /* add to ack watching list */
            list_package_us =  package_send->length() / send_speed * 8;
            ack_mutex.lock();
            ack_pool.push(ack_object);
            ack_mutex.unlock();
            ack_cv.signal();
            get_time_ms(s1);
            //time_pass = get_us(s1, s0);
            if (time_pass < list_package_us) {
                //usleep(list_package_us - time_pass);
            }
//            usleep(100);
        }
		exit_resender = true;
    }).detach();

}

void ReliableSocket::start_thread_ackhandler() {
    std::thread([this](){
		exit_ack_handler = false;
        PackageAckObject ack_obj;
		time_struct_ms tv;
        long time_spend;
        bool resend = false;
        while(worker_running) {
            /* get from queue */
            ack_mutex.lock();
            while (ack_pool.size() == 0) {
                if (!worker_running) {
                    ack_mutex.unlock();
                    LOG(INFO) << "ackhandler thread exit";
					exit_ack_handler = true;
                    return;
                }
                ack_cv.wait(&ack_mutex);
            }
            ack_obj = ack_pool.front();
            ack_pool.pop();
            ack_mutex.unlock();

            resend = false;
            /* while una of the receiver is less than package id */
            while (send_ack_id < ack_obj.data->data_format->package_id) {
                ack_recv_mutex.lock();
                if (ack_recv_pool.find(ack_obj.data->data_format->package_id) != 0) {
                    ack_recv_mutex.unlock();
                    break;
                }
                ack_recv_mutex.unlock();

                get_time_ms(tv);
                time_spend = get_ms(tv, ack_obj.time_send);
                if (time_spend < rto) {
                    msleep(5);
                } else {
                    /* connection timeout, add to resend queue */
//                  std::cout << "Ack timeout, resend package " << ack_obj.data->data_format->package_id << "\n";
                    resend_mutex.lock();
                    resend_pool.push(ack_obj.data);
                    resend_mutex.unlock();
                    resend_cv.signal();
                    resend = true;
                    break;
                }
            }
            if (!resend) {
                send_pool.erase(ack_obj.data->data_format->package_id);
            }
        }
        LOG(INFO) << "ackhandler thread exit\n";
		exit_ack_handler = true;
    }).detach();
}

void ReliableSocket::start_thread_heart_beat() {
    std::thread([this](){
		exit_heart_beat = false;
        int count = 0;
        time_struct_ms s0, s1;
        sleep(2);
        count = 0;
        bool miss = false;
        while (worker_running) {
            ping_mutex.lock();
            if (latest_ping != nullptr) {
				count = 0;
                delete latest_ping;
                latest_ping = nullptr;
            }
            ping_mutex.unlock();
            PackagePing ping = PackagePing();
            LOG(INFO) << "Sending Ping Package";
            get_time_ms(s0);
            miss = false;
            sendto(sock, ping.get_buf(), ping.length(),
                   0, connected_addr, remote_len);

            ping_mutex.lock();
            if (latest_ping == nullptr) {
                ping_cv.wait_time(&ping_mutex, rto);
            }
            //timeout

            if (latest_ping == nullptr) {
                count++;
                miss = true;
            } else {
                delete latest_ping;
                latest_ping = nullptr;
				count = 0;
            }
            ping_mutex.unlock();
            if (count >= 10) {
                /* Connection Close */
                stop_all_thread();
                sleep(2);
                closesocket(sock);
                LOG(INFO) << "heart beat thread exit";
				exit_heart_beat = true;
                return;
            }
            if (!miss) {
                get_time_ms(s1);
                set_rto(get_ms(s1, s0));
            }
			if (!worker_running) {
				break;
			}
            sleep(20);
        }
		LOG(INFO) << "heart beat thread exit";
		exit_heart_beat = true;
    }).detach();
}

void ReliableSocket::start_thread_receiver() {
    std::thread([this]() {
		exit_receiver = false;
        PackageBase package = PackageBase();
        int r;
		fd_set read_set;
		timeval tv;
        while (worker_running) {
			FD_ZERO(&read_set);
			FD_SET(sock, &read_set);
			if (socket_type == RegularSocket || socket_type == BindedSocket) {
				FD_SET(local_sock, &read_set);
			}
			tv.tv_sec = 20;
			tv.tv_usec = 0;
			select(sock + 1, &read_set, NULL, NULL, &tv);
			if ((socket_type == RegularSocket || socket_type == BindedSocket) && 
				FD_ISSET(local_sock, &read_set)) {
				LOG(INFO) << "exit msg recv\n";
				break;
			}
			if (FD_ISSET(sock, &read_set)) {
				r = recvfrom(sock, package.get_buf(), MAX_MTU, 0, connected_addr, &remote_len);
			}
			else {
				continue;
			}
            //r = recvfrom(sock, package.get_buf(), MAX_MTU, 0, connected_addr, &remote_len);
            if (r > 0) {
                package.set_len(r);
                process_package(&package);
            }
        }
        LOG(INFO) << "receiver thread exit";
		exit_receiver = true;
    }).detach();
}

int ReliableSocket::send_package(unsigned char *buf, int size) {
    if (!worker_running)
        return -1;
    //LOG(INFO) << "Send Package, Length " << size;
    PackageData *package = new PackageData(buf, size);
    send_mutex.lock();
    package->data_format->package_id = send_id;
    send_pool.push(package, &send_mutex);
    send_id++;
    send_mutex.unlock();
    send_cv.signal();
    return 0;
}

void ReliableSocket::process_package(PackageBase *package) {
    PackagePing *recv_ping;
    PackageAck *ack;
    PackageData *data;
    PackageAck *new_ack;
    PackageType type = package->get_type();
    uint64_t recv_package_id;
    uint64_t recv_una;

    uint64_t ack_id;
    uint64_t ack_una;

    switch (type) {
        case Ping:
            recv_ping = new PackagePing(package);
            if (!recv_ping->ping_format->ping_back) {
                LOG(INFO) << "PACKAGE PROCESSING:Recv ping";
                recv_ping->ping_format->ping_back = true;
                sendto(sock, recv_ping->get_buf(), recv_ping->length(), 0, connected_addr, remote_len);
                delete recv_ping;
            } else {
				LOG(INFO) << "PACKAGE PROCESSING:Recv ping back";
                ping_mutex.lock();
                latest_ping = recv_ping;
                ping_mutex.unlock();
                ping_cv.signal();
            }
            break;
        case Data:
            data = new PackageData(package);
            recv_package_id = data->data_format->package_id;
            recv_una = data->data_format->una;
//            std::cout << "Receive Data Package " << data->data_format->package_id << "\n";
            recv_mutex.lock();
            /* Duplicated data package
             * Drop Package */
            if (!recv_pool.find(data->data_format->package_id) &&
                    recv_ack_id < recv_package_id) {
				recv_pool.push_back(data, recv_package_id);
            }
            while (recv_pool.find(recv_ack_id + 1)) {
                recv_ack_id++;
            }
            recv_mutex.unlock();
            recv_cv.signal();
//            std::cout << "Send ack " << recv_package_id << " una " << recv_ack_id << "\n";
            ack = new PackageAck(recv_package_id, recv_ack_id);
            sendto(sock, ack->get_buf(), ack->length(), 0, connected_addr, remote_len);

            /* Delete unneccessary ack */
            ack_recv_mutex.lock();
			send_ack_id = recv_una;
            ack_recv_mutex.unlock();
            delete ack;
            break;
        case Ack:
            new_ack = new PackageAck(package);
            ack_id = new_ack->ack_format->package_id;
            ack_una = new_ack->ack_format->una;
            delete new_ack;
//            std::cout << "Receive ack " << ack_id << " una "
//                      << ack_una << " resend " << resend_now << "\n";
            /* if una number is illegal. */
            if (ack_una > send_id) {
                ack_una = 0;
            }
            ack_recv_mutex.lock();
            /* If ack has already received, or ack number is illegal. */
            if (ack_id <= send_ack_id || ack_id > send_id) {
                ack_recv_mutex.unlock();
                return;
            }
            if (ack_recv_pool.find(ack_id) != 0) {
                ack_recv_mutex.unlock();
                return;
            }
            ack_recv_pool.insert(ack_id);
			send_ack_id = ack_una;
            while (ack_recv_pool.find(send_ack_id + 1) != 0) {
                send_ack_id++;
            }
            ack_recv_mutex.unlock();


            /* fast resend */
            if (resend_now < send_ack_id)
                resend_now = send_ack_id;
            if (ack_id > resend_now + 2) {
                uint64_t resend_id = resend_now + 1;
                while (resend_id < ack_id) {
                    ack_recv_mutex.lock();
                    bool found = ack_recv_pool.find(resend_id) != 0;
                    ack_recv_mutex.unlock();
                    if (!found) {
//                        std::cout << "Resend package " << resend_id << "\n";
                        resend_mutex.lock();
                        resend_pool.push(send_pool[resend_id]);
                        resend_mutex.unlock();
                        resend_cv.signal();
                    }
                    resend_id++;
                }
                resend_now = ack_id;
            }
            break;
        case ConnectionClose:
            LOG(INFO) << "PACKAGE PROCESSING:connection close\n";
            worker_running = false;
            stop_all_thread();
            closesocket(sock);
            break;
        default:
            LOG(ERROR) << "PACKAGE PROCESSING:Unknown package, dropping\n";
            break;
    }
}

int ReliableSocket::recv_package(unsigned char *buf) {
    PackageData *return_data;
    recv_mutex.lock();
	while (recv_pool.size() == 0) {
		if (!worker_running) {
			recv_mutex.unlock();
			return -1;
		}
		recv_cv.wait(&recv_mutex);
	}
	return_data = recv_pool.pop_front();
    recv_mutex.unlock();
    memcpy(buf, return_data->get_data(), return_data->data_length());
    int len = return_data->data_length();
    delete return_data;
    //LOG(INFO) << "Recv package, Length " << len;
    return len;
}

ReliableSocket* ReliableSocket::accept_connection() {
    uint64_t unique_id;
    if (socket_type != AcceptableSocket)
        return nullptr;
    PackageConnectionCreate create = PackageConnectionCreate();
    PackageConnectionCreateR create_r = PackageConnectionCreateR(50001, 100);
	ClientSocketInfo info;
    ReliableSocket *new_socket;

	while (true) {
		fd_set read_set;
		FD_ZERO(&read_set);
		FD_SET(sock, &read_set);
		int max_fd = sock;
		for (std::list<ClientSocketInfo>::iterator itr = client_socket_pool.begin(); 
			itr != client_socket_pool.end(); itr++) {
			FD_SET(itr->sock, &read_set);
			max_fd = MAX(itr->sock, max_fd);
		}
		select(max_fd + 1, &read_set, NULL, NULL, NULL);
		if (FD_ISSET(sock, &read_set)) {
			recvfrom(sock, create.get_buf(), MAX_MTU, 0, connected_addr, &remote_len);
			if (create.get_type() != ConnectionCreate) {
				LOG(INFO) << "not connection create package, droping";
			}
			else {
				bool old_request = false;
				unique_id = ((sockaddr_in*)connected_addr)->sin_port;
				unique_id = unique_id << 32 | ((sockaddr_in*)connected_addr)->sin_addr.s_addr;
				int sub_send_speed = min(create.create_format->download_speed, upload_speed);
				for (std::list<ClientSocketInfo>::iterator itr = client_socket_pool.begin(); 
					itr != client_socket_pool.end(); itr++) {
					if (unique_id == itr->client_id) {
						create_r.create_r_format->download_speed = info.speed;
						create_r.create_r_format->port = ntohl(((sockaddr_in *)&info.local_addr)->sin_port);
						sendto(sock, create_r.get_buf(), create_r.length(), 0, connected_addr, remote_len);
						LOG(INFO) << "duplicated request";
						old_request = true;
						break;
					}
				}
				if (!old_request) {
					LOG(INFO) << "new request";
					struct sockaddr new_addr;
					memcpy(&new_addr, local_addr, remote_len);
					int po = get_free_port();
					((sockaddr_in *)&new_addr)->sin_port = htons(po);
					int sock_cr = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
					while (bind(sock_cr, &new_addr, remote_len) != 0) {
						po = get_free_port();
						//        LOG(INFO) << "trying to bind client to port " << po << "\n";
						((sockaddr_in *)&new_addr)->sin_port = htons(po);
					}
					LOG(INFO) << "new connection is binded to port " << po;
					create_r.create_r_format->port = po;
					create_r.create_r_format->download_speed = sub_send_speed;
					sendto(sock, create_r.get_buf(), create_r.length(), 0, connected_addr, remote_len);
					info.client_id = unique_id;
					info.connected_addr = *connected_addr;
					info.local_addr = new_addr;
					info.sock = sock_cr;
					info.speed = sub_send_speed;
					client_socket_pool.push_back(info);
				}
			}
		}
		if (client_socket_pool.size() != 0) {
			for (std::list<ClientSocketInfo>::iterator itr = client_socket_pool.begin(); itr != client_socket_pool.end(); itr++) {
				if (FD_ISSET(itr->sock, &read_set)) {
					client_socket_pool.erase(itr);
					return new ReliableSocket(itr->speed, itr->sock,
						&itr->connected_addr, &itr->local_addr, 200);
				}
			}
		}
	}
	return nullptr;
}

ReliableSocket::ReliableSocket(int speed_send, int sock_r, sockaddr *co_addr, sockaddr *local, long rt) {
    send_speed = speed_send;
    set_rto(rt);
    socket_type = ServerSocket;
    sock = sock_r;
    memcpy(local_addr, local, remote_len);
    memcpy(remote_addr, co_addr, remote_len);
    memcpy(connected_addr, co_addr, remote_len);
    worker_running = true;
    start_thread_sender();
    start_thread_ackhandler();
    start_thread_receiver();
    start_thread_heart_beat();
}

int ReliableSocket::get_free_port() {
    int po = free_port_now + free_port_start;
    free_port_now = (free_port_now + 1) % 100;
    return po;
}

int ReliableSocket::listen_addr() {
    socket_type = AcceptableSocket;
    return 0;
}

int ReliableSocket::bind_addr(std::string host, int port) {
    sockaddr addr;
    socklen_t len;
    get_ipinfo(host, port, &addr, &len);
    return bind_addr(&addr);
}

int ReliableSocket::connect_addr(std::string host, int port) {
    sockaddr addr;
    socklen_t len;
    get_ipinfo(host, port, &addr, &len);
    return connect_addr(&addr);
}

int ReliableSocket::get_ipinfo(std::string host_s, int port, sockaddr *addr, socklen_t *addrlen) {
    struct addrinfo hints;
    struct addrinfo *res;
    int sock, r, flags;
    const char *host = host_s.c_str();
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
	hints.ai_family = AF_INET;
    if (0 != (r = getaddrinfo(host, NULL, &hints, &res))) {
        LOG(ERROR) << "SOCKET INITIALIZATION:getaddrinfo, error";
        return -1;
    }

    if (res->ai_family == AF_INET)
        ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(port);
    else if (res->ai_family == AF_INET6)
        ((struct sockaddr_in6 *)res->ai_addr)->sin6_port = htons(port);
    else {
        LOG(ERROR) << "SOCKET INITIALIZATION:unknown ai_family " << res->ai_family;
        freeaddrinfo(res);
        return -1;
    }
    memcpy(addr, res->ai_addr, res->ai_addrlen);
    *addrlen = res->ai_addrlen;
    return 0;
}

void ReliableSocket::stop_all_thread() {
    worker_running = false;
    send_cv.signal();
    resend_cv.signal();
    ack_cv.signal();
    recv_cv.signal();
    int sock_cl = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    char buf[5];
	sendto(sock_cl, buf, 1, 0, local_addr, remote_len);
	sendto(sock_cl, buf, 1, 0, local_addr, remote_len);
	while (!exit_sender || !exit_receiver || !exit_ack_handler ||
		!exit_resender || !exit_heart_beat) {
		sleep(1);
	}
}

bool ReliableSocket::is_alive() {return worker_running; }
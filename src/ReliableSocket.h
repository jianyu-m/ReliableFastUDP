//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_RELIABLESOCKET_H
#define RELIABLEFASTUDP_RELIABLESOCKET_H

#include <string>
#include <queue>
#include <map>
#include <list>
#include "package/PackageData.h"
#include "pthread_wrapper/ConditionVariable.h"
#include "package/PackagePing.h"

#ifdef TARGET_LINUX
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <queue>

#endif // LINUX

#ifdef TARGET_LINUX
typedef struct timeval time_struct_ms;
#define get_time_ms(t) gettimeofday(&t, 0)
#define get_ms(tv_a, tv_b) ((tv_a.tv_sec - tv_b.tv_sec) * 1000 + (tv_a.tv_usec - tv_b.tv_usec) / 1000)
#define closesocket(sock) close(sock)
#define msleep(s) usleep((s) * 1000)
#endif // TARGET_LINUX

#ifdef TARGET_WIN32
typedef DWORD time_struct_ms;
typedef int socklen_t;
#define get_time_ms(t) t = timeGetTime()
#define get_ms(tv_a, tv_b) ((tv_a) - (tv_b))
#define sleep(s) Sleep((s)*1000)
#define msleep(s) Sleep(s)
#endif // TARGET_WIN32

typedef enum SocketType{
    RegularSocket,
    BindedSocket,
    ServerSocket,
    AcceptableSocket
}SocketType;

typedef struct PackageAckObject {
	PackageData *data;
    time_struct_ms time_send;
}PackageAckObject;

namespace netstruct {

	class IdSet {
	private:
		uint64_t *arr;
		long size;
	public:
		IdSet(long arr_size) {
			arr = new uint64_t[arr_size];
			size = arr_size;
		}
		void insert(uint64_t a) {
			arr[a % size] = a;
		}
//		uint64_t operator[](uint64_t index) {
//			return arr[index % size];
//		}
		uint64_t find(uint64_t index) {
			if (arr[index % size] != index) {
				return 0;
			}
			return arr[index % size];
		}
		~IdSet() {
			delete[] arr;
		}
	};
	class PackageMap {
	private:
		PackageData **data_pointer_list;
		long capacity;
		uint64_t *arr_id;
		long head = 1;
		long tail = 1;
		long len = 0;
	public:
		PackageMap(int arr_size) {
			data_pointer_list = new PackageData*[arr_size];
			arr_id = new uint64_t[arr_size];
			for (int i = 0; i < arr_size; i++) {
				data_pointer_list[i] = nullptr;
				arr_id[i] = 0;
			}
			capacity = arr_size;
		}
		PackageData* find(uint64_t index) {
			if (arr_id[index % capacity] == index) {
				return data_pointer_list[index % capacity];
			}
			return nullptr;
		}
		PackageData*& operator[](uint64_t index) {
			return data_pointer_list[index % capacity];
		}
		void set(uint64_t index, PackageData *data) {
			arr_id[index % capacity] = index;
			data_pointer_list[index % capacity] = data;
		}
		//does not delete content
		void erase(uint64_t index) {
			if (arr_id[index % capacity] == index) {
				arr_id[index % capacity] = 0;
//				data_pointer_list[index % capacity] = nullptr;
			}
		}
		void push(PackageData *data, pthread::Mutex *mutex) {
			uint64_t index = data->data_format->package_id % capacity;
			while (len >= capacity || arr_id[index] != 0) {
				mutex->unlock();
				msleep(2);
				mutex->lock();
			}
			if (data_pointer_list[tail] != nullptr) {
				delete data_pointer_list[tail];
			}
			data_pointer_list[tail] = data;
			arr_id[tail] = data->data_format->package_id;
			tail = (tail + 1) % capacity;
			len++;
		}
		PackageData* pop() {
			if (len <= 0)
				return nullptr;
			PackageData *return_data = data_pointer_list[head];
			head = (head + 1) % capacity;
			len--;
			return return_data;
		}
		long size() { return len; }
		void de_init() {
			for (int i = 0; i < capacity; i++) {
				if (data_pointer_list[i]) {
					delete data_pointer_list[i];
				}
			}
			delete[] arr_id;
		}
	};
	class PackageListMap {
	private:
		uint64_t *arr;
		long len = 0;
		long capacity;
		PackageData **data_list;
		long start = 0;
		long end = 0;
	public:
		PackageListMap(long __capacity) {
			capacity = __capacity;
			data_list = new PackageData*[capacity];
			arr = new uint64_t[capacity];
			for (long i = 0; i < capacity; i++) {
				arr[i] = 0;
				data_list[i] = nullptr;
			}
		}
		//TODO if the cache is full
		void push_back(PackageData *data, uint64_t id) {
			arr[id % capacity] = id;
			data_list[end] = data;
			end = (end + 1) % capacity;
			len++;
		}
		PackageData* pop_front() {
			PackageData* return_data;
			if (len != 0) {
				return_data = data_list[start];
				start = (start + 1) % capacity;
				len--;
				return return_data;
			}
			return nullptr;
		}
		bool find(uint64_t id) {
			if (arr[id % capacity] == id) {
				return true;
			}
			else {
				return false;
			}
		}
		long size() { return len; }
	};
}

typedef struct ClientSocketInfo {
	uint64_t client_id;
	int sock;
	int speed;
	struct sockaddr connected_addr;
	struct sockaddr local_addr;
} ClientSocketInfo;

class ReliableSocket {
protected:
    SocketType socket_type;
    int sock;
	//Only used in client side
	int local_sock;
    int download_speed;
    int upload_speed;
    int send_speed;
    bool worker_running = false;
	std::list<ClientSocketInfo> client_socket_pool;

    struct sockaddr *remote_addr = new sockaddr();
    struct sockaddr *connected_addr = new sockaddr();
    struct sockaddr *local_addr = new sockaddr();
    socklen_t remote_len = sizeof(*remote_addr);

    long rto;
    void set_rto(long rtt);

    uint64_t send_id = 1;
    pthread::Mutex send_mutex;
    pthread::ConditionVariable send_cv;
    netstruct::PackageMap send_pool = netstruct::PackageMap(100000);
    pthread::Mutex resend_mutex;
    pthread::ConditionVariable resend_cv;
    std::queue<PackageData*> resend_pool;
    uint64_t resend_now = 0;

    uint64_t recv_ack_id = 0;
    uint64_t send_ack_id = 0;
    pthread::Mutex ack_mutex;
    pthread::ConditionVariable ack_cv;
    std::queue<PackageAckObject> ack_pool;

    pthread::Mutex ack_recv_mutex;
    netstruct::IdSet ack_recv_pool = netstruct::IdSet(100000);

    pthread::Mutex ping_mutex;
    pthread::ConditionVariable ping_cv;
    PackagePing *latest_ping = nullptr;

    uint64_t recv_id = 1;
    pthread::Mutex recv_mutex;
    pthread::ConditionVariable recv_cv;
    netstruct::PackageListMap recv_pool = netstruct::PackageListMap(100000);

    void start_thread_sender();
    void start_thread_ackhandler();
    void start_thread_heart_beat();
    void start_thread_receiver();
    void process_package(PackageBase *package);

    void stop_all_thread();

    const int free_port_start = 50020;
    int free_port_now = 0;
    int get_free_port();

	bool exit_sender = true;
	bool exit_resender = true;
	bool exit_ack_handler = true;
	bool exit_heart_beat = true;
	bool exit_receiver = true;
    /* Return a connected socket */
    ReliableSocket(int speed_send, int sock, sockaddr *connected_addr, sockaddr *local_addr, long rt);
    int get_ipinfo(std::string host, int port, sockaddr *addr, socklen_t *socklen);
public:
    static int mtu;
    ReliableSocket(int download_speed, int upload_speed);

    ~ReliableSocket();
    int recv_package(unsigned char *buf);
    int send_package(unsigned char *buf, int size);
    int bind_addr(const struct sockaddr *bind_addr);
    int bind_addr(std::string host, int port);
    int connect_addr(std::string host, int port);
    int connect_addr(struct sockaddr *server_addr);
    int close_addr();
    int listen_addr();
    ReliableSocket* accept_connection();
    bool is_alive();
};


#endif //RELIABLEFASTUDP_RELIABLESOCKET_H

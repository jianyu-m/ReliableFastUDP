//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_PACKAGE_H
#define RELIABLEFASTUDP_PACKAGE_H
typedef enum PackageType {
    ConnectionCreate,
    ConnectionCreateR,
    ConnectionClose,
    ConnectionCloseR,
    Ping,
    Data,
    Ack
} PackageType;

namespace netstruct {
	class AckQueue {
	private:
		uint64_t *arr;
		long len = 0, start = 0, end = 0;
		long capacity;
	public:
		AckQueue(long __capacity) {
			capacity = __capacity;
			arr = new uint64_t[capacity];
		}
		AckQueue(const AckQueue& queue) {
			arr = queue.arr;
			len = queue.len;
			start = queue.start;
			end = queue.end;
			capacity = queue.capacity;
		}
		void push(uint64_t id) {
			arr[end] = id;
			end = (end + 1) % capacity;
			len++;
		}
		uint64_t pop() {
			if (len <= 0)
				return 0;
			uint64_t return_val = arr[start];
			start = (start + 1) % capacity;
			len--;
			return return_val;
		}
		void rebase() {
			len = 0;
			start = end;
		}
		void de_init() {
			delete[] arr;
		}
		long size() { return len; }
	};
}

#endif //RELIABLEFASTUDP_PACKAGE_H

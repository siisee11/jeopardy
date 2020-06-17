#include <iostream>
#include <thread>
#include <deque>
#include <mutex>
#include <chrono>
#include <condition_variable>

using std::deque;
std::mutex cout_mu;
std::condition_variable cond;

class Buffer
{
public:
    void add(int num) {
        while (true) {
            std::unique_lock<std::mutex> locker(mu);
            cond.wait(locker, [this](){return buffer_.size() < size_;});
            buffer_.push_back(num);
            locker.unlock();
            cond.notify_all();
            return;
        }
    }
    int remove() {
        while (true)
        {
            std::unique_lock<std::mutex> locker(mu);
            cond.wait(locker, [this](){return buffer_.size() > 0;});
            int back = buffer_.back();
            buffer_.pop_back();
            locker.unlock();
            cond.notify_all();
            return back;
        }
    }
    Buffer() {}
private:
   // Add them as member variables here
    std::mutex mu;
    std::condition_variable cond;

   // Your normal variables here
    deque<int> buffer_;
    const unsigned int size_ = 10;
};

class Producer
{
    Buffer &buffer_;

public:
    Producer(Buffer& buffer)
		: buffer_(buffer)
    {}

	void run() {
		while (true) {
			int num = std::rand() % 100;
			buffer_.add(num);
			{
				std::unique_lock<std::mutex> lock(cout_mu);
				std::cout << "Produced: " << num << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
    }
};

class Consumer
{
    Buffer &buffer_;

public:
    Consumer(Buffer& buffer)
		: buffer_(buffer)
	{}
    void run() {
        while (true) {
            int num = buffer_.remove();
			{
				std::unique_lock<std::mutex> lock(cout_mu);
				std::cout << "Consumed: " << num << std::endl;
			}
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
};

int main() {
    Buffer b;
    Producer p(b);
    Consumer c(b);

    std::thread producer_thread(&Producer::run, &p);
    std::thread consumer_thread(&Consumer::run, &c);

    producer_thread.join();
    consumer_thread.join();
    getchar();
    return 0;
}

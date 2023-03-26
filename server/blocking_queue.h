#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include "../common/messages.h"

// Blocking queue for shared pointers of server messages, used by client connections and server.
class BlockingMessageQueue {
private:
    std::mutex mutex;
    std::condition_variable condition_variable;
    std::queue<std::shared_ptr<ServerMessage>> queue;
    bool client_connection_closed = false;

public:
    BlockingMessageQueue() = default;
    explicit BlockingMessageQueue(std::queue<std::shared_ptr<ServerMessage>> queue) :
            queue(std::move(queue)) {}

    void push(const std::shared_ptr<ServerMessage> &message) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(message);
        }
        condition_variable.notify_one();
    }

    std::shared_ptr<ServerMessage> pop() {
        std::unique_lock<std::mutex> lock(mutex);
        condition_variable.wait(lock, [&]{ return !queue.empty() || client_connection_closed; });
        if (client_connection_closed) {
            throw std::invalid_argument("Connection closed cleanly by peer");
        }
        std::shared_ptr<ServerMessage> message(std::move(queue.front()));
        queue.pop();
        return message;
    }

    void close_client_connection() {
        {
            std::lock_guard lock(mutex);
            client_connection_closed = true;
        }
        condition_variable.notify_one();
    }
};

#endif //BLOCKING_QUEUE_H

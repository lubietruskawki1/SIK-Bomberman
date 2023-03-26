#ifndef BUFFER_H
#define BUFFER_H

#include <boost/asio.hpp>
#include <iostream>
#include "types.h"

namespace as = boost::asio;

// Buffer sizes.
constexpr size_t UDP_BUFFER_SIZE = 65507;
constexpr size_t TCP_BUFFER_SIZE = 4096;

class Buffer {
protected:
    char *buffer;
    size_t size;
    size_t index = 0; // Pointer.
    size_t message_length = 0;

    explicit Buffer(size_t buffer_size) : size(buffer_size) {
        buffer = new char[buffer_size];
    }

    // Resets value of variables 'index' and 'message_length'. Called after sending a message
    // and before receiving one if needed.
    void reset() {
        index = 0;
        message_length = 0;
    }

    // Receives a message.
    virtual void receive_message([[maybe_unused]] size_t value_size) {}

    // Ensures that at least 'value_size' bytes is written in buffer.
    virtual void fill([[maybe_unused]] size_t value_size) {}

    // Ensures that at least 'value_size' bytes is free in buffer.
    virtual void free([[maybe_unused]] size_t value_size) {}

public:
    // Functions reading from buffer a sequence of 'size' bytes and storing it in a variable
    // 'value'. If needed they convert read value to host byte order. They increase the value
    // of the variable 'index' accordingly.
    void get(uint8_t &value, size_t value_size);
    void get(uint16_t &value, size_t value_size);
    void get(uint32_t &value, size_t value_size);
    [[maybe_unused]] void get(uint64_t &value, size_t value_size);

    // Function reading from buffer a sequence of 'size' bytes and storing it in a variable
    // 'value' of type string. Increases the value of the variable 'index' accordingly.
    void get(std::string &value);

    // Functions inserting into buffer a sequence of 'size' bytes with the value of the
    // variable 'value'. If needed they convert read value to network byte order. They
    // increase the value of the variable 'index' accordingly.
    void insert(uint8_t value, size_t value_size);
    void insert(uint16_t value, size_t value_size);
    void insert(uint32_t value, size_t value_size);
    [[maybe_unused]] void insert(uint64_t value, size_t value_size);

    // Function inserting into buffer a sequence of 'size' bytes with the value of the
    // variable 'value' of type string. Increases the value of the variable 'index'
    // accordingly.
    void insert(const std::string &value);

    // Sends a message.
    virtual void send_message() {}

    // Returns the length of a message currently kept in buffer.
    [[nodiscard]] size_t get_message_length() const  {
        return message_length;
    }

    virtual ~Buffer() {
        delete[] buffer;
    }
};

class BufferUDP : public Buffer {
private:
    as::ip::udp::socket &socket;
    as::ip::udp::endpoint endpoint;

    void receive_message([[maybe_unused]] size_t value_size) override {
        // Message fits in a UDP datagram, so load the whole message immediately
        // (the argument is a dummy).
        reset();
        message_length = socket.receive(as::buffer(buffer, size));
    }

public:
    BufferUDP(as::ip::udp::socket &socket, as::ip::udp::endpoint endpoint) :
            Buffer(UDP_BUFFER_SIZE), socket(socket), endpoint(std::move(endpoint)) {}

    void receive_message() {
        receive_message(size);
    }

    void send_message() override {
        socket.send_to(as::buffer(buffer, index), endpoint);
        reset();
    }
};

class BufferTCP : public Buffer {
private:
    as::ip::tcp::socket &socket;

    void receive_message(size_t value_size) override {
        boost::system::error_code error;
        as::read(socket, as::buffer(buffer + index, value_size), error);
        if (error == as::error::eof) {
            throw std::invalid_argument("Connection closed cleanly by peer");
        } else if (error) {
            throw boost::system::system_error(error);
        }
        message_length += value_size;
    }

    void fill(size_t value_size) override {
        // With TCP, load to buffer exactly 'value_size' bytes. If there aren't at least
        // this many free bytes in buffer reset values of variables 'index' and
        // 'message_length' (as buffer will be overwritten).
        if (index + value_size > size) {
            reset();
        }
         receive_message(value_size);
    }

    void free(size_t value_size) override {
        // If given variable won't fit in buffer send the datagram created so far.
        if (index + value_size > size) {
            send_message();
        }
    }

public:
    explicit BufferTCP(as::ip::tcp::socket &socket) :
            Buffer(TCP_BUFFER_SIZE), socket(socket) {}

    void send_message() override {
        as::write(socket, as::buffer(buffer, index));
        reset();
    }
};

#endif //BUFFER_H
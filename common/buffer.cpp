#include "buffer.h"

void Buffer::get(uint8_t &value, size_t value_size) {
    fill(value_size);
    memcpy(&value, buffer + index, value_size);
    index += value_size;
}

void Buffer::get(uint16_t &value, size_t value_size) {
    fill(value_size);
    uint16_t value_network_byte_order;
    memcpy(&value_network_byte_order, buffer + index, value_size);
    index += value_size;
    value = ntohs(value_network_byte_order);
}

void Buffer::get(uint32_t &value, size_t value_size) {
    fill(value_size);
    uint32_t value_network_byte_order;
    memcpy(&value_network_byte_order, buffer + index, value_size);
    index += value_size;
    value = ntohl(value_network_byte_order);
}

[[maybe_unused]] [[maybe_unused]] void Buffer::get(uint64_t &value, size_t value_size) {
    fill(value_size);
    uint64_t value_network_byte_order;
    memcpy(&value_network_byte_order, buffer + index, value_size);
    index += value_size;
    value = be64toh(value_network_byte_order);
}

void Buffer::get(std::string &value) {
    string_length_t value_size;
    get(value_size, STRING_LENGTH_SIZE);
    fill(value_size);
    value.resize(value_size);
    memcpy(&value[0], buffer + index, value_size);
    index += value_size;
}

void Buffer::insert(uint8_t value, size_t value_size) {
    free(value_size);
    memcpy(buffer + index, &value, value_size);
    index += value_size;
    message_length += value_size;
}

void Buffer::insert(uint16_t value, size_t value_size) {
    free(value_size);
    uint16_t value_network_byte_order = htons(value);
    memcpy(buffer + index, &value_network_byte_order, value_size);
    index += value_size;
    message_length += value_size;
}

void Buffer::insert(uint32_t value, size_t value_size) {
    free(value_size);
    uint32_t value_network_byte_order = htonl(value);
    memcpy(buffer + index, &value_network_byte_order, value_size);
    index += value_size;
    message_length += value_size;
}

[[maybe_unused]] void Buffer::insert(uint64_t value, size_t value_size) {
    free(value_size);
    uint64_t value_network_byte_order = htobe64(value);
    memcpy(buffer + index, &value_network_byte_order, value_size);
    index += value_size;
    message_length += value_size;
}

void Buffer::insert(const std::string &value) {
    auto value_size = (string_length_t) value.length();
    insert(value_size, STRING_LENGTH_SIZE);
    free(value_size);
    memcpy(buffer + index, &value[0], value_size);
    index += value_size;
    message_length += value_size;
}
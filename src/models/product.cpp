#include "product.hpp"
#include <stdexcept>

Product::Product(std::int32_t id, const std::string& name, const Money& price, std::int32_t stock)
    : id_(id), name_(name), price_(price), stock_(stock) {
    if (stock < 0) {
        throw std::invalid_argument("Stock cannot be negative");
    }
}

std::int32_t Product::GetId() const {
    return id_;
}

const std::string& Product::GetName() const {
    return name_;
}

Money Product::GetPrice() const {
    return price_;
}

std::int32_t Product::GetStock() const {
    return stock_;
}

bool Product::ReserveStock(std::int32_t quantity) {
    if (quantity <= 0) {
        throw std::invalid_argument("Quantity must be positive");
    }
    if (stock_ < quantity) {
        return false;
    }
    stock_ -= quantity;
    return true;
}

void Product::ReleaseStock(std::int32_t quantity) {
    if (quantity <= 0) {
        throw std::invalid_argument("Quantity must be positive");
    }
    stock_ += quantity;
}

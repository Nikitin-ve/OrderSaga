#include "user.hpp"
#include <stdexcept>

User::User(std::int32_t id, const std::string& name, const Money& balance)
    : id_(id), name_(name), balance_(balance) {}

std::int32_t User::GetId() const {
    return id_;
}

const std::string& User::GetName() const {
    return name_;
}

Money User::GetBalance() const {
    return balance_;
}

void User::DeductBalance(const Money& amount) {
    if (!amount.IsPositive()) {
        throw std::invalid_argument("Deduct amount must be positive");
    }
    if (balance_ < amount) {
        throw std::runtime_error("Insufficient balance");
    }
    balance_ -= amount;
}

void User::AddBalance(const Money& amount) {
    if (!amount.IsPositive()) {
        throw std::invalid_argument("Add amount must be positive");
    }
    balance_ += amount;
}

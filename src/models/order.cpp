#include "order.hpp"

Order::Order(std::int32_t id, std::int32_t user_id)
    : id_(id), user_id_(user_id), status_(OrderStatus::kPending),
      discount_percent_(0.0) {
    created_at_ = std::time(nullptr);
}

std::int32_t Order::GetId() const {
    return id_;
}

std::int32_t Order::GetUserId() const {
    return user_id_;
}

OrderStatus Order::GetStatus() const {
    return status_;
}

const std::vector<OrderItem>& Order::GetItems() const {
    return items_;
}

Money Order::GetTotalPrice() const {
    return total_price_;
}

double Order::GetDiscountPercent() const {
    return discount_percent_;
}

Money Order::GetFinalPrice() const {
    return final_price_;
}

std::time_t Order::GetCreatedAt() const {
    return created_at_;
}

const std::string& Order::GetFailureReason() const {
    return failure_reason_;
}

std::string Order::GetStatusString() const {
    switch (status_) {
        case OrderStatus::kPending: return "PENDING";
        case OrderStatus::kProcessing: return "PROCESSING";
        case OrderStatus::kConfirmed: return "CONFIRMED";
        case OrderStatus::kFailed: return "FAILED";
        case OrderStatus::kCancelled: return "CANCELLED";
        case OrderStatus::kCompensating: return "COMPENSATING";
        case OrderStatus::kCompensated: return "COMPENSATED";
        default: return "UNKNOWN";
    }
}

void Order::SetStatus(OrderStatus status) {
    status_ = status;
}

void Order::SetDiscountPercent(double discount) {
    discount_percent_ = discount;
    RecalculateFinalPrice();
}

void Order::SetFailureReason(const std::string& reason) {
    failure_reason_ = reason;
}

void Order::AddItem(const OrderItem& item) {
    items_.push_back(item);
    total_price_ += item.GetTotalPrice();
    RecalculateFinalPrice();
}

void Order::ClearItems() {
    items_.clear();
    total_price_ = Money();
    final_price_ = Money();
}

void Order::RecalculateFinalPrice() {
    final_price_ = total_price_.ApplyDiscount(discount_percent_);
}

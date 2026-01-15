#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <ctime>
#include "money.hpp"

/**
 * @brief Статус заказа
 */
enum OrderStatus {
    kPending,
    kProcessing,
    kConfirmed,
    kFailed,
    kCancelled,
    kCompensating,
    kCompensated
};

/**
 * @brief Элемент заказа (товар и количество)
 */
struct OrderItem {
    std::int32_t product_id;
    std::int32_t quantity;
    Money unit_price;  // Цена за единицу товара на момент заказа
    
    OrderItem(std::int32_t prod_id, std::int32_t qty, const Money& price)
        : product_id(prod_id), quantity(qty), unit_price(price) {}
    
    // Итоговая стоимость позиции
    Money GetTotalPrice() const {
        return unit_price * quantity;
    }
};

/**
 * @brief Модель заказа
 */
class Order {
public:
    Order(std::int32_t id, std::int32_t user_id);
    
    // Геттеры
    std::int32_t GetId() const;
    std::int32_t GetUserId() const;
    OrderStatus GetStatus() const;
    const std::vector<OrderItem>& GetItems() const;
    Money GetTotalPrice() const;
    double GetDiscountPercent() const;
    Money GetFinalPrice() const;
    std::string GetStatusString() const;
    std::time_t GetCreatedAt() const;
    const std::string& GetFailureReason() const;
    
    // Сеттеры
    void SetStatus(OrderStatus status);
    void SetDiscountPercent(double discount);
    void SetFailureReason(const std::string& reason);
    
    // Операции с товарами
    void AddItem(const OrderItem& item);
    void ClearItems();
    
private:
    std::int32_t id_;
    std::int32_t user_id_;
    OrderStatus status_;
    std::vector<OrderItem> items_;
    Money total_price_;
    double discount_percent_;
    Money final_price_;
    std::time_t created_at_;
    std::string failure_reason_;
    
    void RecalculateFinalPrice();
};

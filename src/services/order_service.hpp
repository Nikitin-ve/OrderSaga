#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include "../models/order.hpp"
#include "../common/types.hpp"
#include "discount_service.hpp"
#include "billing_service.hpp"
#include "inventory_service.hpp"
#include "notification_service.hpp"

// Forward declaration
class OrderSagaOrchestrator;

/**
 * @brief Результат создания заказа
 */
struct CreateOrderResult {
    bool success;
    std::string message;
    std::int32_t order_id;
    
    CreateOrderResult(bool s, const std::string& msg, std::int32_t id = -1) 
        : success(s), message(msg), order_id(id) {}
    
    static CreateOrderResult Ok(std::int32_t id, const std::string& msg = "Заказ создан") {
        return CreateOrderResult(true, msg, id);
    }
    
    static CreateOrderResult Fail(const std::string& msg) {
        return CreateOrderResult(false, msg, -1);
    }
};

/**
 * @brief Сервис управления заказами
 */
class OrderService {
public:
    OrderService(BillingService& billing, 
                InventoryService& inventory,
                DiscountService& discount,
                NotificationService& notification);
    
    ~OrderService();
    
    // Создание заказа - возвращает ID созданного заказа или ошибку
    CreateOrderResult CreateOrder(std::int32_t user_id, 
                                  const std::vector<std::pair<std::int32_t, std::int32_t>>& items);
    
    ServiceResult ProcessOrder(std::int32_t order_id);
    
    ServiceResult CancelOrder(std::int32_t order_id);
    
    // Геттеры
    std::optional<Order> GetOrder(std::int32_t order_id) const;
    std::vector<Order> GetUserOrders(std::int32_t user_id) const;
    std::vector<Order> GetAllOrders() const;
    const std::vector<std::string>& GetOperationLog() const;
    
    void ClearOperationLog();
    
private:
    std::unordered_map<std::int32_t, Order> orders_;
    std::int32_t next_order_id_;
    
    BillingService& billing_;
    InventoryService& inventory_;
    DiscountService& discount_;
    NotificationService& notification_;
    
    std::unique_ptr<OrderSagaOrchestrator> saga_orchestrator_;
    
    mutable std::vector<std::string> operation_log_;
    
    void Log(const std::string& message) const;
};

#include "order_service.hpp"
#include "../saga/order_saga_orchestrator.hpp"
#include <sstream>

OrderService::OrderService(BillingService& billing,
                          InventoryService& inventory,
                          DiscountService& discount,
                          NotificationService& notification)
    : next_order_id_(1),
      billing_(billing),
      inventory_(inventory),
      discount_(discount),
      notification_(notification) {
    
    saga_orchestrator_ = std::make_unique<OrderSagaOrchestrator>(
        billing_, inventory_, discount_, notification_);
    
    Log("OrderService инициализирован");
}

OrderService::~OrderService() = default;

void OrderService::Log(const std::string& message) const {
    operation_log_.push_back("[OrderService] " + message);
}

CreateOrderResult OrderService::CreateOrder(std::int32_t user_id,
                                            const std::vector<std::pair<std::int32_t, std::int32_t>>& items) {
    if (items.empty()) {
        Log("ОШИБКА: Попытка создать пустой заказ");
        return CreateOrderResult::Fail("Заказ не может быть пустым");
    }
    
    auto user_opt = billing_.GetUser(user_id);
    if (!user_opt) {
        Log("ОШИБКА: Пользователь " + std::to_string(user_id) + " не найден");
        return CreateOrderResult::Fail("Пользователь не найден");
    }
    
    std::int32_t order_id = next_order_id_++;
    Order order(order_id, user_id);
    
    for (const auto& [product_id, quantity] : items) {
        auto product_opt = inventory_.GetProduct(product_id);
        if (!product_opt) {
            Log("ОШИБКА: Товар " + std::to_string(product_id) + " не найден");
            return CreateOrderResult::Fail("Товар " + std::to_string(product_id) + " не найден");
        }
        
        OrderItem item(product_id, quantity, product_opt->GetPrice());
        order.AddItem(item);
    }
    
    Money order_amount = order.GetTotalPrice();
    double discount = discount_.CalculateOrderDiscount(user_id, order_amount);
    order.SetDiscountPercent(discount);
    
    orders_.insert({order_id, order});
    
    std::stringstream ss;
    ss << "Создан заказ #" << order_id << " для пользователя " << user_opt->GetName()
       << " (сумма: " << order.GetTotalPrice().ToString() << " руб., скидка: " << discount 
       << "%, итого: " << order.GetFinalPrice().ToString() << " руб.)";
    Log(ss.str());
    
    return CreateOrderResult::Ok(order_id, "Заказ создан");
}

ServiceResult OrderService::ProcessOrder(std::int32_t order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        Log("ОШИБКА: Заказ " + std::to_string(order_id) + " не найден");
        return ServiceResult::Fail("Заказ не найден");
    }
    
    Order& order = it->second;
    
    if (order.GetStatus() != OrderStatus::kPending) {
        Log("ОШИБКА: Заказ " + std::to_string(order_id) + " не в статусе PENDING");
        return ServiceResult::Fail("Заказ не может быть обработан в текущем статусе");
    }
    
    Log("Запуск Saga для обработки заказа #" + std::to_string(order_id));
    
    bool success = saga_orchestrator_->ExecuteSaga(order);
    
    if (success) {
        Log("Заказ #" + std::to_string(order_id) + " успешно обработан");
        return ServiceResult::Ok("Заказ успешно обработан");
    } else {
        std::stringstream ss;
        ss << "Заказ #" << order_id << " не обработан: " << order.GetFailureReason();
        Log(ss.str());
        return ServiceResult::Fail(order.GetFailureReason());
    }
}

ServiceResult OrderService::CancelOrder(std::int32_t order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        Log("ОШИБКА: Заказ " + std::to_string(order_id) + " не найден");
        return ServiceResult::Fail("Заказ не найден");
    }
    
    Order& order = it->second;
    
    if (order.GetStatus() == OrderStatus::kConfirmed) {
        saga_orchestrator_->CompensateSaga(order);
        order.SetStatus(OrderStatus::kCancelled);
        order.SetFailureReason("Отменён пользователем");
        Log("Заказ #" + std::to_string(order_id) + " отменён с компенсацией");
        return ServiceResult::Ok("Заказ отменён, средства возвращены");
    } else if (order.GetStatus() == OrderStatus::kPending) {
        order.SetStatus(OrderStatus::kCancelled);
        order.SetFailureReason("Отменён пользователем");
        Log("Заказ #" + std::to_string(order_id) + " отменён");
        return ServiceResult::Ok("Заказ отменён");
    } else {
        Log("ОШИБКА: Заказ " + std::to_string(order_id) + " не может быть отменён в статусе " + order.GetStatusString());
        return ServiceResult::Fail("Заказ не может быть отменён в текущем статусе");
    }
}

std::optional<Order> OrderService::GetOrder(std::int32_t order_id) const {
    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<Order> OrderService::GetUserOrders(std::int32_t user_id) const {
    std::vector<Order> result;
    for (const auto& [id, order] : orders_) {
        if (order.GetUserId() == user_id) {
            result.push_back(order);
        }
    }
    return result;
}

std::vector<Order> OrderService::GetAllOrders() const {
    std::vector<Order> result;
    for (const auto& [id, order] : orders_) {
        result.push_back(order);
    }
    return result;
}

const std::vector<std::string>& OrderService::GetOperationLog() const {
    return operation_log_;
}

void OrderService::ClearOperationLog() {
    operation_log_.clear();
}

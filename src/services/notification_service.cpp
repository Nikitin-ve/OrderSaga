#include "notification_service.hpp"
#include <sstream>

NotificationService::NotificationService() : next_notification_id_(1), simulate_failure_(false) {
    Log("NotificationService инициализирован");
}

void NotificationService::Log(const std::string& message) const {
    operation_log_.push_back("[NotificationService] " + message);
}

ServiceResult NotificationService::CheckFailure(const std::string& operation) const {
    if (simulate_failure_) {
        Log("ОШИБКА: Симуляция сбоя при " + operation);
        return ServiceResult::Fail("Сервис уведомлений недоступен");
    }
    return ServiceResult::Ok();
}

ServiceResult NotificationService::SendNotification(std::int32_t user_id, std::int32_t order_id,
                                                    NotificationType type, 
                                                    const std::string& message) {
    auto check = CheckFailure("отправке уведомления");
    if (!check.success) return check;
    
    Notification notification(next_notification_id_++, user_id, order_id, type, message);
    notification.sent = true;  // В реальности здесь была бы отправка через внешний сервис
    notifications_.push_back(notification);
    
    std::stringstream ss;
    ss << "Отправлено уведомление [" << notification.GetTypeString() << "] пользователю " << user_id 
       << " о заказе " << order_id << ": " << message;
    Log(ss.str());
    
    return ServiceResult::Ok("Уведомление отправлено");
}

ServiceResult NotificationService::NotifyOrderCreated(std::int32_t user_id, std::int32_t order_id) {
    return SendNotification(user_id, order_id, NotificationType::kOrderCreated, 
        "Ваш заказ #" + std::to_string(order_id) + " создан и обрабатывается");
}

ServiceResult NotificationService::NotifyOrderConfirmed(std::int32_t user_id, std::int32_t order_id) {
    return SendNotification(user_id, order_id, NotificationType::kOrderConfirmed, 
        "Ваш заказ #" + std::to_string(order_id) + " подтверждён и оплачен");
}

ServiceResult NotificationService::NotifyOrderFailed(std::int32_t user_id, std::int32_t order_id, 
                                                     const std::string& reason) {
    return SendNotification(user_id, order_id, NotificationType::kOrderFailed, 
        "Заказ #" + std::to_string(order_id) + " не может быть выполнен: " + reason);
}

ServiceResult NotificationService::NotifyOrderCancelled(std::int32_t user_id, std::int32_t order_id,
                                                        const std::string& reason) {
    return SendNotification(user_id, order_id, NotificationType::kOrderCancelled, 
        "Заказ #" + std::to_string(order_id) + " отменён: " + reason);
}

ServiceResult NotificationService::NotifyPaymentProcessed(std::int32_t user_id, std::int32_t order_id) {
    return SendNotification(user_id, order_id, NotificationType::kPaymentProcessed, 
        "Оплата за заказ #" + std::to_string(order_id) + " успешно обработана");
}

ServiceResult NotificationService::NotifyRefundProcessed(std::int32_t user_id, std::int32_t order_id) {
    return SendNotification(user_id, order_id, NotificationType::kRefundProcessed, 
        "Средства за заказ #" + std::to_string(order_id) + " возвращены на баланс");
}

std::vector<Notification> NotificationService::GetNotificationsForUser(std::int32_t user_id) const {
    std::vector<Notification> result;
    for (const auto& n : notifications_) {
        if (n.user_id == user_id) {
            result.push_back(n);
        }
    }
    return result;
}

std::vector<Notification> NotificationService::GetNotificationsForOrder(std::int32_t order_id) const {
    std::vector<Notification> result;
    for (const auto& n : notifications_) {
        if (n.order_id == order_id) {
            result.push_back(n);
        }
    }
    return result;
}

void NotificationService::SetSimulateFailure(bool fail) {
    simulate_failure_ = fail;
    if (fail) {
        Log("ВНИМАНИЕ: Включена симуляция сбоя");
    } else {
        Log("Симуляция сбоя отключена");
    }
}

bool NotificationService::IsSimulatingFailure() const {
    return simulate_failure_;
}

const std::vector<std::string>& NotificationService::GetOperationLog() const {
    return operation_log_;
}

void NotificationService::ClearOperationLog() {
    operation_log_.clear();
}

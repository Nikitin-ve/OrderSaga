#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "../common/types.hpp"

/**
 * @brief Уведомление
 */
struct Notification {
    std::int32_t notification_id;
    std::int32_t user_id;
    std::int32_t order_id;
    NotificationType type;
    std::string message;
    bool sent;
    
    Notification(std::int32_t n_id, std::int32_t u_id, std::int32_t o_id, 
                 NotificationType t, const std::string& msg)
        : notification_id(n_id), user_id(u_id), order_id(o_id), 
          type(t), message(msg), sent(false) {}
    
    std::string GetTypeString() const { return NotificationTypeToString(type); }
};

/**
 * @brief Сервис уведомлений
 * 
 * Симулирует отправку уведомлений (в реальности просто логирует)
 */
class NotificationService {
public:
    NotificationService();
    
    // Геттеры (доступны даже при симуляции сбоя)
    std::vector<Notification> GetNotificationsForUser(std::int32_t user_id) const;
    std::vector<Notification> GetNotificationsForOrder(std::int32_t order_id) const;
    const std::vector<std::string>& GetOperationLog() const;
    bool IsSimulatingFailure() const;
    
    // Методы отправки уведомлений (недоступны при симуляции сбоя)
    ServiceResult NotifyOrderCreated(std::int32_t user_id, std::int32_t order_id);
    ServiceResult NotifyOrderConfirmed(std::int32_t user_id, std::int32_t order_id);
    ServiceResult NotifyOrderFailed(std::int32_t user_id, std::int32_t order_id, const std::string& reason);
    ServiceResult NotifyOrderCancelled(std::int32_t user_id, std::int32_t order_id, const std::string& reason);
    ServiceResult NotifyPaymentProcessed(std::int32_t user_id, std::int32_t order_id);
    ServiceResult NotifyRefundProcessed(std::int32_t user_id, std::int32_t order_id);
    
    void SetSimulateFailure(bool fail);
    void ClearOperationLog();
    
private:
    std::vector<Notification> notifications_;
    std::int32_t next_notification_id_;
    bool simulate_failure_;
    mutable std::vector<std::string> operation_log_;
    
    void Log(const std::string& message) const;
    ServiceResult CheckFailure(const std::string& operation) const;
    ServiceResult SendNotification(std::int32_t user_id, std::int32_t order_id, 
                                   NotificationType type, const std::string& message);
};

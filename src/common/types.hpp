#pragma once

#include <cstdint>
#include <string>

/**
 * @brief Результат операции сервиса
 */
struct ServiceResult {
    bool success;
    std::string message;
    
    ServiceResult(bool s, const std::string& msg) : success(s), message(msg) {}
    
    static ServiceResult Ok(const std::string& msg = "OK") {
        return ServiceResult(true, msg);
    }
    
    static ServiceResult Fail(const std::string& msg) {
        return ServiceResult(false, msg);
    }
};

/**
 * @brief Типы уведомлений
 */
enum class NotificationType {
    kOrderCreated,
    kOrderConfirmed,
    kOrderFailed,
    kOrderCancelled,
    kPaymentProcessed,
    kRefundProcessed
};

/**
 * @brief Статус платежа
 */
enum class PaymentStatus {
    kPending,
    kConfirmed,
    kRefunded,
    kFailed
};

/**
 * @brief Получить строковое представление типа уведомления
 */
inline std::string NotificationTypeToString(NotificationType type) {
    switch (type) {
        case NotificationType::kOrderCreated: return "ORDER_CREATED";
        case NotificationType::kOrderConfirmed: return "ORDER_CONFIRMED";
        case NotificationType::kOrderFailed: return "ORDER_FAILED";
        case NotificationType::kOrderCancelled: return "ORDER_CANCELLED";
        case NotificationType::kPaymentProcessed: return "PAYMENT_PROCESSED";
        case NotificationType::kRefundProcessed: return "REFUND_PROCESSED";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Получить строковое представление статуса платежа
 */
inline std::string PaymentStatusToString(PaymentStatus status) {
    switch (status) {
        case PaymentStatus::kPending: return "pending";
        case PaymentStatus::kConfirmed: return "confirmed";
        case PaymentStatus::kRefunded: return "refunded";
        case PaymentStatus::kFailed: return "failed";
        default: return "unknown";
    }
}


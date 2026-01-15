#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include "../models/user.hpp"
#include "../common/types.hpp"

/**
 * @brief Информация о платеже
 */
struct Payment {
    std::int32_t payment_id;
    std::int32_t order_id;
    std::int32_t user_id;
    Money amount;
    PaymentStatus status;
    
    Payment(std::int32_t p_id, std::int32_t o_id, std::int32_t u_id, const Money& amt)
        : payment_id(p_id), order_id(o_id), user_id(u_id), amount(amt), 
          status(PaymentStatus::kPending) {}
    
    bool IsConfirmed() const { return status == PaymentStatus::kConfirmed; }
    std::string GetStatusString() const { return PaymentStatusToString(status); }
};

/**
 * @brief Сервис биллинга (оплаты)
 */
class BillingService {
public:
    BillingService();
    
    // Геттеры (доступны даже при симуляции сбоя)
    std::optional<User> GetUser(std::int32_t user_id) const;
    std::optional<Money> GetBalance(std::int32_t user_id) const;
    bool CanPay(std::int32_t user_id, const Money& amount) const;
    std::optional<Payment> GetPayment(std::int32_t order_id) const;
    std::vector<User> GetAllUsers() const;
    const std::vector<std::string>& GetOperationLog() const;
    bool IsSimulatingFailure() const;
    
    // Модифицирующие методы (недоступны при симуляции сбоя)
    ServiceResult AddUser(const User& user);
    ServiceResult ProcessPayment(std::int32_t order_id, std::int32_t user_id, const Money& amount);
    ServiceResult ConfirmPayment(std::int32_t order_id);
    ServiceResult RefundPayment(std::int32_t order_id);
    ServiceResult Deposit(std::int32_t user_id, const Money& amount);
    
    void SetSimulateFailure(bool fail);
    void ClearOperationLog();
    
private:
    std::unordered_map<std::int32_t, User> users_;
    std::unordered_map<std::int32_t, Payment> payments_;
    std::int32_t next_payment_id_;
    bool simulate_failure_;
    mutable std::vector<std::string> operation_log_;
    
    void Log(const std::string& message) const;
    ServiceResult CheckFailure(const std::string& operation) const;
};

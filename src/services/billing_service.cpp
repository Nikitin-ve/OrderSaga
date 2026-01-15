#include "billing_service.hpp"
#include <sstream>

BillingService::BillingService() : next_payment_id_(1), simulate_failure_(false) {
    Log("BillingService инициализирован");
}

void BillingService::Log(const std::string& message) const {
    operation_log_.push_back("[BillingService] " + message);
}

ServiceResult BillingService::CheckFailure(const std::string& operation) const {
    if (simulate_failure_) {
        Log("ОШИБКА: Симуляция сбоя при " + operation);
        return ServiceResult::Fail("Сервис оплаты недоступен");
    }
    return ServiceResult::Ok();
}

ServiceResult BillingService::AddUser(const User& user) {
    auto check = CheckFailure("добавлении пользователя");
    if (!check.success) return check;
    
    if (users_.find(user.GetId()) != users_.end()) {
        Log("ПРЕДУПРЕЖДЕНИЕ: Пользователь " + std::to_string(user.GetId()) + " уже существует, обновление");
    }
    
    users_[user.GetId()] = user;
    
    std::stringstream ss;
    ss << "Добавлен пользователь: " << user.GetName() 
       << " (ID: " << user.GetId() << ", баланс: " << user.GetBalance().ToString() << " руб.)";
    Log(ss.str());
    
    return ServiceResult::Ok("Пользователь добавлен");
}

std::optional<User> BillingService::GetUser(std::int32_t user_id) const {
    auto it = users_.find(user_id);
    if (it != users_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<Money> BillingService::GetBalance(std::int32_t user_id) const {
    auto it = users_.find(user_id);
    if (it != users_.end()) {
        return it->second.GetBalance();
    }
    return std::nullopt;
}

bool BillingService::CanPay(std::int32_t user_id, const Money& amount) const {
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    return it->second.GetBalance() >= amount;
}

ServiceResult BillingService::ProcessPayment(std::int32_t order_id, std::int32_t user_id, const Money& amount) {
    auto check = CheckFailure("обработке платежа");
    if (!check.success) return check;
    
    if (payments_.find(order_id) != payments_.end()) {
        Log("ПРЕДУПРЕЖДЕНИЕ: Платёж для заказа " + std::to_string(order_id) + " уже существует");
        return ServiceResult::Fail("Платёж уже существует");
    }
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        Log("ОШИБКА: Пользователь " + std::to_string(user_id) + " не найден");
        return ServiceResult::Fail("Пользователь не найден");
    }
    
    if (!amount.IsPositive()) {
        Log("ОШИБКА: Некорректная сумма: " + amount.ToString());
        return ServiceResult::Fail("Некорректная сумма платежа");
    }
    
    if (it->second.GetBalance() < amount) {
        std::stringstream ss;
        ss << "ОШИБКА: Недостаточно средств у пользователя " << it->second.GetName()
           << " (требуется: " << amount.ToString() << ", доступно: " << it->second.GetBalance().ToString() << ")";
        Log(ss.str());
        return ServiceResult::Fail("Недостаточно средств на балансе");
    }
    
    it->second.DeductBalance(amount);
    
    Payment payment(next_payment_id_++, order_id, user_id, amount);
    payments_.insert({order_id, payment});
    
    std::stringstream ss;
    ss << "Платёж обработан: " << amount.ToString() << " руб. от пользователя " 
       << it->second.GetName() << " за заказ " << order_id;
    Log(ss.str());
    
    return ServiceResult::Ok("Платёж обработан");
}

ServiceResult BillingService::ConfirmPayment(std::int32_t order_id) {
    auto check = CheckFailure("подтверждении платежа");
    if (!check.success) return check;
    
    auto it = payments_.find(order_id);
    if (it == payments_.end()) {
        Log("ПРЕДУПРЕЖДЕНИЕ: Платёж для заказа " + std::to_string(order_id) + " не найден");
        return ServiceResult::Ok("Платёж не найден");
    }
    
    it->second.status = PaymentStatus::kConfirmed;
    
    Log("Платёж для заказа " + std::to_string(order_id) + " подтверждён");
    return ServiceResult::Ok("Платёж подтверждён");
}

ServiceResult BillingService::RefundPayment(std::int32_t order_id) {
    // Компенсация не зависит от симуляции сбоя
    auto it = payments_.find(order_id);
    if (it == payments_.end()) {
        Log("ПРЕДУПРЕЖДЕНИЕ: Платёж для заказа " + std::to_string(order_id) + " не найден (компенсация)");
        return ServiceResult::Ok("Платёж не найден для возврата");
    }
    
    auto user_it = users_.find(it->second.user_id);
    if (user_it != users_.end()) {
        user_it->second.AddBalance(it->second.amount);
        
        std::stringstream ss;
        ss << "Возврат средств: " << it->second.amount.ToString() << " руб. пользователю " 
           << user_it->second.GetName() << " за заказ " << order_id << " (компенсация)";
        Log(ss.str());
    }
    
    it->second.status = PaymentStatus::kRefunded;
    payments_.erase(it);
    
    return ServiceResult::Ok("Средства возвращены");
}

std::optional<Payment> BillingService::GetPayment(std::int32_t order_id) const {
    auto it = payments_.find(order_id);
    if (it != payments_.end()) {
        return it->second;
    }
    return std::nullopt;
}

ServiceResult BillingService::Deposit(std::int32_t user_id, const Money& amount) {
    auto check = CheckFailure("пополнении баланса");
    if (!check.success) return check;
    
    if (!amount.IsPositive()) {
        return ServiceResult::Fail("Некорректная сумма пополнения");
    }
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return ServiceResult::Fail("Пользователь не найден");
    }
    
    it->second.AddBalance(amount);
    
    std::stringstream ss;
    ss << "Баланс пополнен: +" << amount.ToString() << " руб. пользователю " 
       << it->second.GetName() << " (итого: " << it->second.GetBalance().ToString() << ")";
    Log(ss.str());
    
    return ServiceResult::Ok("Баланс пополнен");
}

void BillingService::SetSimulateFailure(bool fail) {
    simulate_failure_ = fail;
    if (fail) {
        Log("ВНИМАНИЕ: Включена симуляция сбоя");
    } else {
        Log("Симуляция сбоя отключена");
    }
}

bool BillingService::IsSimulatingFailure() const {
    return simulate_failure_;
}

const std::vector<std::string>& BillingService::GetOperationLog() const {
    return operation_log_;
}

void BillingService::ClearOperationLog() {
    operation_log_.clear();
}

std::vector<User> BillingService::GetAllUsers() const {
    std::vector<User> result;
    for (const auto& [id, user] : users_) {
        result.push_back(user);
    }
    return result;
}

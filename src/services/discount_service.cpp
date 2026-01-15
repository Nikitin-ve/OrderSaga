#include "discount_service.hpp"
#include <algorithm>
#include <sstream>

DiscountService::DiscountService() : simulate_failure_(false) {
    Log("DiscountService инициализирован");
}

void DiscountService::Log(const std::string& message) const {
    operation_log_.push_back("[DiscountService] " + message);
}

ServiceResult DiscountService::CheckFailure(const std::string& operation) const {
    if (simulate_failure_) {
        Log("ОШИБКА: Симуляция сбоя при " + operation);
        return ServiceResult::Fail("Сервис скидок недоступен");
    }
    return ServiceResult::Ok();
}

DiscountInfo DiscountService::GetUserDiscount(std::int32_t user_id) const {
    auto it = user_discounts_.find(user_id);
    if (it != user_discounts_.end()) {
        return it->second;
    }
    return DiscountInfo(user_id, 0.0, false, 0);
}

double DiscountService::CalculateOrderDiscount(std::int32_t user_id, const Money& order_amount) const {
    DiscountInfo info = GetUserDiscount(user_id);
    
    double total_discount = info.discount_percent;
    
    if (info.is_vip) {
        total_discount += kVipBonus;
    }
    
    std::int32_t order_bonus_multiplier = info.orders_count / 10;
    total_discount += order_bonus_multiplier * kOrderBonusPer10;
    
    if (order_amount.GetKopecks() >= kLargeOrderThresholdKopecks) {
        total_discount += kLargeOrderBonus;
    }
    
    total_discount = std::min(total_discount, kMaxDiscount);
    
    return total_discount;
}

ServiceResult DiscountService::SetUserDiscount(std::int32_t user_id, double discount_percent) {
    auto check = CheckFailure("установке скидки");
    if (!check.success) return check;
    
    if (discount_percent < 0 || discount_percent > 100) {
        Log("ОШИБКА: Некорректный процент скидки: " + std::to_string(discount_percent));
        return ServiceResult::Fail("Процент скидки должен быть от 0 до 100");
    }
    
    auto it = user_discounts_.find(user_id);
    if (it == user_discounts_.end()) {
        user_discounts_[user_id] = DiscountInfo(user_id, discount_percent, false, 0);
    } else {
        it->second.discount_percent = discount_percent;
    }
    
    Log("Установлена скидка " + std::to_string(discount_percent) + "% для пользователя " + std::to_string(user_id));
    return ServiceResult::Ok("Скидка установлена");
}

ServiceResult DiscountService::SetVipStatus(std::int32_t user_id, bool is_vip) {
    auto check = CheckFailure("установке VIP-статуса");
    if (!check.success) return check;
    
    auto it = user_discounts_.find(user_id);
    if (it == user_discounts_.end()) {
        user_discounts_[user_id] = DiscountInfo(user_id, 0.0, is_vip, 0);
    } else {
        it->second.is_vip = is_vip;
    }
    
    std::string status = is_vip ? "установлен" : "снят";
    Log("VIP-статус " + status + " для пользователя " + std::to_string(user_id));
    return ServiceResult::Ok("VIP-статус " + status);
}

ServiceResult DiscountService::IncrementOrderCount(std::int32_t user_id) {
    auto check = CheckFailure("увеличении счётчика заказов");
    if (!check.success) return check;
    
    auto it = user_discounts_.find(user_id);
    if (it == user_discounts_.end()) {
        user_discounts_[user_id] = DiscountInfo(user_id, 0.0, false, 1);
    } else {
        it->second.orders_count++;
    }
    
    Log("Увеличен счётчик заказов для пользователя " + std::to_string(user_id));
    return ServiceResult::Ok("Счётчик заказов увеличен");
}

ServiceResult DiscountService::DecrementOrderCount(std::int32_t user_id) {
    // Компенсация не зависит от симуляции сбоя
    auto it = user_discounts_.find(user_id);
    if (it != user_discounts_.end() && it->second.orders_count > 0) {
        it->second.orders_count--;
        Log("Уменьшен счётчик заказов для пользователя " + std::to_string(user_id) + " (компенсация)");
    }
    return ServiceResult::Ok("Счётчик заказов уменьшен");
}

ServiceResult DiscountService::ApplyDiscount(std::int32_t user_id, std::int32_t order_id, const Money& order_amount) {
    auto check = CheckFailure("применении скидки к заказу " + std::to_string(order_id));
    if (!check.success) return check;
    
    if (applied_discounts_.find(order_id) != applied_discounts_.end()) {
        Log("ПРЕДУПРЕЖДЕНИЕ: Скидка уже применена к заказу " + std::to_string(order_id));
        return ServiceResult::Ok("Скидка уже применена");
    }
    
    double discount = CalculateOrderDiscount(user_id, order_amount);
    applied_discounts_[order_id] = discount;
    
    std::stringstream ss;
    ss << "Применена скидка " << discount << "% к заказу " << order_id 
       << " для пользователя " << user_id;
    Log(ss.str());
    
    return ServiceResult::Ok("Скидка " + std::to_string(discount) + "% применена");
}

ServiceResult DiscountService::CancelDiscount(std::int32_t user_id, std::int32_t order_id) {
    // Компенсация не зависит от симуляции сбоя
    auto it = applied_discounts_.find(order_id);
    if (it != applied_discounts_.end()) {
        double discount = it->second;
        applied_discounts_.erase(it);
        
        std::stringstream ss;
        ss << "Отменена скидка " << discount << "% для заказа " << order_id 
           << " пользователя " << user_id << " (компенсация)";
        Log(ss.str());
    }
    return ServiceResult::Ok("Скидка отменена");
}

double DiscountService::GetAppliedDiscount(std::int32_t order_id) const {
    auto it = applied_discounts_.find(order_id);
    if (it != applied_discounts_.end()) {
        return it->second;
    }
    return 0.0;
}

void DiscountService::SetSimulateFailure(bool fail) {
    simulate_failure_ = fail;
    if (fail) {
        Log("ВНИМАНИЕ: Включена симуляция сбоя");
    } else {
        Log("Симуляция сбоя отключена");
    }
}

bool DiscountService::IsSimulatingFailure() const {
    return simulate_failure_;
}

const std::vector<std::string>& DiscountService::GetOperationLog() const {
    return operation_log_;
}

void DiscountService::ClearOperationLog() {
    operation_log_.clear();
}

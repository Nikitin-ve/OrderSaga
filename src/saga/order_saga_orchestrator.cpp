#include "order_saga_orchestrator.hpp"

OrderSagaOrchestrator::OrderSagaOrchestrator(BillingService& billing,
                                             InventoryService& inventory,
                                             DiscountService& discount,
                                             NotificationService& notification)
    : billing_(billing), inventory_(inventory), 
      discount_(discount), notification_(notification) {
    Log("OrderSagaOrchestrator инициализирован");
}

void OrderSagaOrchestrator::Log(const std::string& message) const {
    execution_log_.push_back("[Saga] " + message);
}

std::vector<SagaStep> OrderSagaOrchestrator::BuildSagaSteps(Order& order) {
    std::vector<SagaStep> steps;
    std::int32_t order_id = order.GetId();
    std::int32_t user_id = order.GetUserId();
    
    // Шаг 1: Уведомление о создании заказа
    // Компенсация: нет (информационное уведомление)
    steps.emplace_back(
        "Уведомление о создании заказа",
        [this, user_id, order_id]() {
            return notification_.NotifyOrderCreated(user_id, order_id);
        },
        []() {}
    );
    
    // Шаг 2: Резервирование товаров на складе
    // Компенсация: освобождение резерва (выполняется в ReleaseReservation)
    for (const auto& item : order.GetItems()) {
        std::int32_t product_id = item.product_id;
        std::int32_t quantity = item.quantity;
        
        steps.emplace_back(
            "Резервирование товара " + std::to_string(product_id),
            [this, order_id, product_id, quantity]() {
                return inventory_.ReserveProduct(order_id, product_id, quantity);
            },
            [this, order_id, product_id]() {
                inventory_.ReleaseReservation(order_id, product_id);
            }
        );
    }
    
    // Шаг 3: Применение скидки
    // Компенсация: отмена применённой скидки
    Money order_amount = order.GetTotalPrice();
    steps.emplace_back(
        "Применение скидки",
        [this, user_id, order_id, order_amount]() {
            return discount_.ApplyDiscount(user_id, order_id, order_amount);
        },
        [this, user_id, order_id]() {
            discount_.CancelDiscount(user_id, order_id);
        }
    );
    
    // Шаг 4: Обработка платежа
    // Компенсация: возврат средств (выполняется в RefundPayment)
    Money final_price = order.GetFinalPrice();
    steps.emplace_back(
        "Обработка платежа",
        [this, order_id, user_id, final_price]() {
            return billing_.ProcessPayment(order_id, user_id, final_price);
        },
        [this, order_id]() {
            billing_.RefundPayment(order_id);
        }
    );
    
    // Шаг 5: Подтверждение платежа
    // Компенсация: нет (возврат средств выполняется на шаге 4)
    steps.emplace_back(
        "Подтверждение платежа",
        [this, order_id]() {
            return billing_.ConfirmPayment(order_id);
        },
        []() {}  // Возврат средств выполнен в компенсации шага 4 (RefundPayment)
    );
    
    // Шаг 6: Подтверждение резервирований
    // Компенсация: нет (освобождение резервов выполняется на шаге 2)
    for (const auto& item : order.GetItems()) {
        std::int32_t product_id = item.product_id;
        
        steps.emplace_back(
            "Подтверждение резервирования товара " + std::to_string(product_id),
            [this, order_id, product_id]() {
                return inventory_.ConfirmReservation(order_id, product_id);
            },
            []() {}  // Освобождение резервов выполнено в компенсации шага 2 (ReleaseReservation)
        );
    }
    
    // Шаг 7: Увеличение счётчика заказов
    // Компенсация: уменьшение счётчика
    steps.emplace_back(
        "Обновление статистики заказов",
        [this, user_id]() {
            return discount_.IncrementOrderCount(user_id);
        },
        [this, user_id]() {
            discount_.DecrementOrderCount(user_id);
        }
    );
    
    // Шаг 8: Уведомление об успешном заказе
    // Компенсация: нет (информационное уведомление)
    steps.emplace_back(
        "Уведомление о подтверждении заказа",
        [this, user_id, order_id]() {
            return notification_.NotifyOrderConfirmed(user_id, order_id);
        },
        []() {}
    );
    
    return steps;
}

bool OrderSagaOrchestrator::ExecuteSaga(Order& order) {
    std::int32_t order_id = order.GetId();
    
    Log("=== Начало выполнения Saga для заказа #" + std::to_string(order_id) + " ===");
    
    order.SetStatus(OrderStatus::kProcessing);
    
    std::vector<SagaStep> steps = BuildSagaSteps(order);
    
    for (std::size_t i = 0; i < steps.size(); ++i) {
        SagaStep& step = steps[i];
        
        Log("Шаг " + std::to_string(i + 1) + "/" + std::to_string(steps.size()) + 
            ": " + step.name);
        
        ServiceResult result = step.execute();
        step.executed = true;
        
        if (result.success) {
            step.succeeded = true;
            Log("  ✓ Успешно: " + result.message);
        } else {
            step.succeeded = false;
            Log("  ✗ Ошибка: " + result.message);
            
            order.SetFailureReason(result.message);
            order.SetStatus(OrderStatus::kCompensating);
            
            Log("--- Запуск компенсации ---");
            RunCompensation(steps, static_cast<std::int32_t>(i));
            
            order.SetStatus(OrderStatus::kFailed);
            
            // Уведомление о неудаче
            notification_.NotifyOrderFailed(order.GetUserId(), order_id, result.message);
            
            Log("=== Saga завершена с ошибкой ===");
            return false;
        }
    }
    
    order.SetStatus(OrderStatus::kConfirmed);
    Log("=== Saga успешно завершена ===");
    
    return true;
}

void OrderSagaOrchestrator::RunCompensation(std::vector<SagaStep>& steps, std::int32_t from_step) {
    for (std::int32_t i = from_step - 1; i >= 0; --i) {
        SagaStep& step = steps[static_cast<std::size_t>(i)];
        
        if (step.executed && step.succeeded) {
            Log("Компенсация шага: " + step.name);
            step.compensate();
        }
    }
}

void OrderSagaOrchestrator::CompensateSaga(Order& order) {
    std::int32_t order_id = order.GetId();
    std::int32_t user_id = order.GetUserId();
    
    Log("=== Принудительная компенсация заказа #" + std::to_string(order_id) + " ===");
    
    order.SetStatus(OrderStatus::kCompensating);
    
    // Возврат средств
    billing_.RefundPayment(order_id);
    
    // Освобождение всех резервирований
    inventory_.ReleaseAllReservations(order_id);
    
    // Отмена скидки
    discount_.CancelDiscount(user_id, order_id);
    
    // Уменьшение счётчика заказов
    discount_.DecrementOrderCount(user_id);
    
    // Уведомление
    notification_.NotifyRefundProcessed(user_id, order_id);
    
    order.SetStatus(OrderStatus::kCompensated);
    
    Log("=== Компенсация завершена ===");
}

const std::vector<std::string>& OrderSagaOrchestrator::GetExecutionLog() const {
    return execution_log_;
}

void OrderSagaOrchestrator::ClearExecutionLog() {
    execution_log_.clear();
}

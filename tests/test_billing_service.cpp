#include "test_framework.hpp"
#include "../src/services/billing_service.hpp"

TEST(BillingService_AddUser) {
    BillingService service;
    
    User user(1, "Иван", Money::FromRubles(10000));
    ServiceResult result = service.AddUser(user);
    
    ASSERT_TRUE(result.success);
    
    auto retrieved = service.GetUser(1);
    ASSERT_TRUE(retrieved.has_value());
    ASSERT_EQ("Иван", retrieved->GetName());
    ASSERT_EQ(1000000, retrieved->GetBalance().GetKopecks()); // 10000 рублей
}

TEST(BillingService_GetUserNotFound) {
    BillingService service;
    
    auto user = service.GetUser(999);
    ASSERT_FALSE(user.has_value());
}

TEST(BillingService_CanPay) {
    BillingService service;
    
    service.AddUser(User(1, "Тест", Money::FromRubles(5000)));
    
    ASSERT_TRUE(service.CanPay(1, Money::FromRubles(5000)));
    ASSERT_TRUE(service.CanPay(1, Money::FromRubles(3000)));
    ASSERT_FALSE(service.CanPay(1, Money::FromRubles(5001)));
    ASSERT_FALSE(service.CanPay(999, Money::FromRubles(100)));
}

TEST(BillingService_ProcessPayment) {
    BillingService service;
    
    service.AddUser(User(1, "Покупатель", Money::FromRubles(10000)));
    
    ServiceResult result = service.ProcessPayment(100, 1, Money::FromRubles(3000));
    ASSERT_TRUE(result.success);
    
    auto balance = service.GetBalance(1);
    ASSERT_TRUE(balance.has_value());
    ASSERT_EQ(700000, balance->GetKopecks()); // 7000 рублей
    
    auto payment = service.GetPayment(100);
    ASSERT_TRUE(payment.has_value());
    ASSERT_EQ(100, payment->order_id);
    ASSERT_EQ(1, payment->user_id);
    ASSERT_EQ(300000, payment->amount.GetKopecks()); // 3000 рублей
    ASSERT_TRUE(payment->status == PaymentStatus::kPending);
}

TEST(BillingService_InsufficientFunds) {
    BillingService service;
    
    service.AddUser(User(1, "Бедный", Money::FromRubles(1000)));
    
    ServiceResult result = service.ProcessPayment(100, 1, Money::FromRubles(5000));
    ASSERT_FALSE(result.success);
    
    auto balance = service.GetBalance(1);
    ASSERT_TRUE(balance.has_value());
    ASSERT_EQ(100000, balance->GetKopecks()); // 1000 рублей - не изменился
}

TEST(BillingService_ConfirmPayment) {
    BillingService service;
    
    service.AddUser(User(1, "Тест", Money::FromRubles(10000)));
    service.ProcessPayment(100, 1, Money::FromRubles(2000));
    
    ServiceResult result = service.ConfirmPayment(100);
    ASSERT_TRUE(result.success);
    
    auto payment = service.GetPayment(100);
    ASSERT_TRUE(payment.has_value());
    ASSERT_TRUE(payment->IsConfirmed());
    ASSERT_TRUE(payment->status == PaymentStatus::kConfirmed);
}

TEST(BillingService_RefundPayment) {
    BillingService service;
    
    service.AddUser(User(1, "Тест", Money::FromRubles(10000)));
    service.ProcessPayment(100, 1, Money::FromRubles(3000));
    
    auto balance = service.GetBalance(1);
    ASSERT_EQ(700000, balance->GetKopecks()); // 7000 рублей
    
    ServiceResult result = service.RefundPayment(100);
    ASSERT_TRUE(result.success);
    
    balance = service.GetBalance(1);
    ASSERT_EQ(1000000, balance->GetKopecks()); // 10000 рублей - вернулось
    
    auto payment = service.GetPayment(100);
    ASSERT_FALSE(payment.has_value()); // Платёж удалён
}

TEST(BillingService_Deposit) {
    BillingService service;
    
    service.AddUser(User(1, "Тест", Money::FromRubles(1000)));
    
    ServiceResult result = service.Deposit(1, Money::FromRubles(5000));
    ASSERT_TRUE(result.success);
    
    auto balance = service.GetBalance(1);
    ASSERT_EQ(600000, balance->GetKopecks()); // 6000 рублей
}

TEST(BillingService_SimulateFailure) {
    BillingService service;
    
    service.AddUser(User(1, "Тест", Money::FromRubles(10000)));
    
    service.SetSimulateFailure(true);
    ASSERT_TRUE(service.IsSimulatingFailure());
    
    ServiceResult result = service.ProcessPayment(100, 1, Money::FromRubles(1000));
    ASSERT_FALSE(result.success);
    
    // Баланс не должен измениться
    auto balance = service.GetBalance(1);
    ASSERT_EQ(1000000, balance->GetKopecks());
    
    // Геттеры должны работать
    auto user = service.GetUser(1);
    ASSERT_TRUE(user.has_value());
    
    service.SetSimulateFailure(false);
    result = service.ProcessPayment(100, 1, Money::FromRubles(1000));
    ASSERT_TRUE(result.success);
}

TEST(BillingService_DuplicatePayment) {
    BillingService service;
    
    service.AddUser(User(1, "Тест", Money::FromRubles(10000)));
    
    ServiceResult result1 = service.ProcessPayment(100, 1, Money::FromRubles(1000));
    ASSERT_TRUE(result1.success);
    
    ServiceResult result2 = service.ProcessPayment(100, 1, Money::FromRubles(1000));
    ASSERT_FALSE(result2.success);
}

TEST(BillingService_RefundNonexistent) {
    BillingService service;
    
    ServiceResult result = service.RefundPayment(999);
    ASSERT_TRUE(result.success); // Не ошибка - просто нечего возвращать
}

TEST(BillingService_GetAllUsers) {
    BillingService service;
    
    service.AddUser(User(1, "Пользователь 1", Money::FromRubles(1000)));
    service.AddUser(User(2, "Пользователь 2", Money::FromRubles(2000)));
    
    auto users = service.GetAllUsers();
    ASSERT_EQ(2, static_cast<std::int32_t>(users.size()));
    
    bool found_user1 = false, found_user2 = false;
    for (const auto& user : users) {
        if (user.GetId() == 1 && user.GetName() == "Пользователь 1") {
            found_user1 = true;
            ASSERT_EQ(100000, user.GetBalance().GetKopecks());
        }
        if (user.GetId() == 2 && user.GetName() == "Пользователь 2") {
            found_user2 = true;
            ASSERT_EQ(200000, user.GetBalance().GetKopecks());
        }
    }
    ASSERT_TRUE(found_user1);
    ASSERT_TRUE(found_user2);
}

TEST(BillingService_KopecksPrecision) {
    BillingService service;
    
    // Тест на точность в копейках
    service.AddUser(User(1, "Тест", Money::FromRublesAndKopecks(100, 99)));
    
    auto balance = service.GetBalance(1);
    ASSERT_EQ(10099, balance->GetKopecks()); // 100.99 рублей
    
    service.ProcessPayment(100, 1, Money::FromRublesAndKopecks(50, 50));
    
    balance = service.GetBalance(1);
    ASSERT_EQ(5049, balance->GetKopecks()); // 50.49 рублей
}

TEST(BillingService_GetBalanceNotFound) {
    BillingService service;
    
    auto balance = service.GetBalance(999);
    ASSERT_FALSE(balance.has_value());
}

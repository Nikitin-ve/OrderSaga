#include "test_framework.hpp"
#include "../src/services/discount_service.hpp"

TEST(DiscountService_Creation) {
    DiscountService service;
    
    DiscountInfo info = service.GetUserDiscount(999);
    ASSERT_EQ(999, info.user_id);
    ASSERT_NEAR(0.0, info.discount_percent, 0.00001);
    ASSERT_FALSE(info.is_vip);
    ASSERT_EQ(0, info.orders_count);
}

TEST(DiscountService_SetUserDiscount) {
    DiscountService service;
    
    ServiceResult result = service.SetUserDiscount(1, 10.0);
    ASSERT_TRUE(result.success);
    
    DiscountInfo info = service.GetUserDiscount(1);
    ASSERT_NEAR(10.0, info.discount_percent, 0.00001);
}

TEST(DiscountService_InvalidDiscount) {
    DiscountService service;
    
    ServiceResult result = service.SetUserDiscount(1, 150.0);
    ASSERT_FALSE(result.success);
    
    result = service.SetUserDiscount(1, -10.0);
    ASSERT_FALSE(result.success);
}

TEST(DiscountService_VipBonus) {
    DiscountService service;
    
    service.SetUserDiscount(1, 5.0);
    service.SetVipStatus(1, true);
    
    double discount = service.CalculateOrderDiscount(1, Money::FromRubles(1000));
    ASSERT_NEAR(10.0, discount, 0.00001);
}

TEST(DiscountService_OrderCountBonus) {
    DiscountService service;
    
    for (std::int32_t i = 0; i < 25; ++i) {
        service.IncrementOrderCount(1);
    }
    
    DiscountInfo info = service.GetUserDiscount(1);
    ASSERT_EQ(25, info.orders_count);
    
    double discount = service.CalculateOrderDiscount(1, Money::FromRubles(1000));
    ASSERT_NEAR(4.0, discount, 0.00001);
}

TEST(DiscountService_LargeOrderBonus) {
    DiscountService service;
    
    // Большой заказ от 10000 рублей даёт бонус 3%
    double discount = service.CalculateOrderDiscount(1, Money::FromRubles(15000));
    ASSERT_NEAR(3.0, discount, 0.00001);
    
    // Заказ меньше 10000 не даёт бонус
    discount = service.CalculateOrderDiscount(1, Money::FromRubles(5000));
    ASSERT_NEAR(0.0, discount, 0.00001);
}

TEST(DiscountService_MaxDiscountCap) {
    DiscountService service;
    
    service.SetUserDiscount(1, 25.0);
    service.SetVipStatus(1, true);
    
    for (std::int32_t i = 0; i < 50; ++i) {
        service.IncrementOrderCount(1);
    }
    
    double discount = service.CalculateOrderDiscount(1, Money::FromRubles(1000));
    ASSERT_NEAR(30.0, discount, 0.00001);
}

TEST(DiscountService_ApplyAndCancelDiscount) {
    DiscountService service;
    
    service.SetUserDiscount(1, 15.0);
    
    ServiceResult result = service.ApplyDiscount(1, 100, Money::FromRubles(5000));
    ASSERT_TRUE(result.success);
    
    double applied_discount = service.GetAppliedDiscount(100);
    ASSERT_NEAR(15.0, applied_discount, 0.00001);
    
    result = service.CancelDiscount(1, 100);
    ASSERT_TRUE(result.success);
    
    applied_discount = service.GetAppliedDiscount(100);
    ASSERT_NEAR(0.0, applied_discount, 0.00001);
}

TEST(DiscountService_ApplyDiscountWithLargeOrder) {
    DiscountService service;
    
    service.SetUserDiscount(1, 10.0);
    
    // Большой заказ должен учитывать бонус за сумму
    ServiceResult result = service.ApplyDiscount(1, 100, Money::FromRubles(15000));
    ASSERT_TRUE(result.success);
    
    double applied_discount = service.GetAppliedDiscount(100);
    ASSERT_NEAR(13.0, applied_discount, 0.00001);  // 10% + 3% за большой заказ
}

TEST(DiscountService_SimulateFailure) {
    DiscountService service;
    
    service.SetSimulateFailure(true);
    ASSERT_TRUE(service.IsSimulatingFailure());
    
    ServiceResult result = service.SetUserDiscount(1, 10.0);
    ASSERT_FALSE(result.success);
    
    result = service.ApplyDiscount(1, 100, Money::FromRubles(1000));
    ASSERT_FALSE(result.success);
    
    // Геттеры должны работать даже при сбое
    DiscountInfo info = service.GetUserDiscount(1);
    ASSERT_EQ(1, info.user_id);
    
    service.SetSimulateFailure(false);
    result = service.SetUserDiscount(1, 10.0);
    ASSERT_TRUE(result.success);
}

TEST(DiscountService_DecrementOrderCount) {
    DiscountService service;
    
    service.IncrementOrderCount(1);
    service.IncrementOrderCount(1);
    
    DiscountInfo info = service.GetUserDiscount(1);
    ASSERT_EQ(2, info.orders_count);
    
    service.DecrementOrderCount(1);
    
    info = service.GetUserDiscount(1);
    ASSERT_EQ(1, info.orders_count);
    
    // Не должен уходить в минус
    service.DecrementOrderCount(1);
    service.DecrementOrderCount(1);
    
    info = service.GetUserDiscount(1);
    ASSERT_EQ(0, info.orders_count);
}

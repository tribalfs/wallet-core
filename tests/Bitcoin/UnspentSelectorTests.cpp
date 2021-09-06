// Copyright © 2017-2020 Trust Wallet.
//
// This file is part of Trust. The full Trust copyright notice, including
// terms governing use, modification, and redistribution, is contained in the
// file LICENSE at the root of the source code distribution tree.

#include <TrustWalletCore/TWCoinType.h>

#include "TxComparisonHelper.h"
#include "Bitcoin/OutPoint.h"
#include "Bitcoin/Script.h"
#include "Bitcoin/UnspentSelector.h"
#include "proto/Bitcoin.pb.h"

#include <gtest/gtest.h>
#include <sstream>

using namespace TW;
using namespace TW::Bitcoin;


std::vector<uint64_t> utxoAmounts(const UTXOs& utxos) {
    std::vector<uint64_t> res;
    for (const auto& utxo: utxos) {
        res.push_back(utxo.amount);
    }
    return res;
}

std::vector<UTXO> unrollIndices(const std::vector<UTXO>& utxos, const std::vector<size_t>& indices) {
    std::vector<UTXO> res;
    for(auto i: indices) {
        res.push_back(utxos[i]);
    }
    return res;
}

TEST(BitcoinUnspentSelector, SelectUnspents1) {
    auto utxos = buildTestUTXOs({4000, 2000, 6000, 1000, 11000, 12000});

    std::cout << "TTT1 " << utxoAmounts(utxos).size() << "\n";
    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(5000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {11000}));
}

TEST(BitcoinUnspentSelector, SelectUnspents2) {
    auto utxos = buildTestUTXOs({4000, 2000, 6000, 1000, 50000, 120000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(10000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {50000}));
}

TEST(BitcoinUnspentSelector, SelectUnspents3) {
    auto utxos = buildTestUTXOs({4000, 2000, 5000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(6000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {4000, 5000}));
}

TEST(BitcoinUnspentSelector, SelectUnspents4) {
    auto utxos = buildTestUTXOs({40000, 30000, 30000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(50000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {30000, 40000}));
}

TEST(BitcoinUnspentSelector, SelectUnspents5) {
    auto utxos = buildTestUTXOs({1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(28000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {6000, 7000, 8000, 9000}));
}

TEST(BitcoinUnspentSelector, SelectUnspentsInsufficient) {
    auto utxos = buildTestUTXOs({4000, 4000, 4000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(15000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));
}

TEST(BitcoinUnspentSelector, SelectCustomCase) {
    auto utxos = buildTestUTXOs({794121, 2289357});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(2287189, 61);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {794121, 2289357}));
}

TEST(BitcoinUnspentSelector, SelectNegativeNoUTXOs) {
    auto utxos = buildTestUTXOs({});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(100000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));
}

TEST(BitcoinUnspentSelector, SelectNegativeTarget0) {
    auto utxos = buildTestUTXOs({100'000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(0, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));
}

TEST(BitcoinUnspentSelector, SelectOneTypical) {
    auto utxos = buildTestUTXOs({100'000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(50'000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {100'000}));
}

TEST(BitcoinUnspentSelector, SelectOneInsufficient) {
    auto utxos = buildTestUTXOs({100'000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(200'000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));
}

TEST(BitcoinUnspentSelector, SelectOneInsufficientEqual) {
    auto utxos = buildTestUTXOs({100'000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(100'000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));
}

TEST(BitcoinUnspentSelector, SelectOneInsufficientHigher) {
    auto utxos = buildTestUTXOs({100'000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(99'900, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));
}

TEST(BitcoinUnspentSelector, SelectOneFitsExactly) {
    auto utxos = buildTestUTXOs({100'000});

    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    auto expectedFee = 174;
    auto indices = selector.select(100'000 - expectedFee, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {100'000}));

    EXPECT_EQ(feeCalculator.calculate(1, 2, 1), expectedFee);
    EXPECT_EQ(feeCalculator.calculate(1, 1, 1), 143);

    // 1 sat more and does not fit any more
    indices = selector.select(100'000 - expectedFee + 1, 1);
    selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));
}

TEST(BitcoinUnspentSelector, SelectOneFitsExactlyHighfee) {
    auto utxos = buildTestUTXOs({100'000});

    const auto byteFee = 10;
    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    auto expectedFee = 1740;
    auto indices = selector.select(100'000 - expectedFee, byteFee);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {100'000}));

    EXPECT_EQ(feeCalculator.calculate(1, 2, byteFee), expectedFee);
    EXPECT_EQ(feeCalculator.calculate(1, 1, byteFee), 1430);

    // 1 sat more and does not fit any more
    indices = selector.select(100'000 - expectedFee + 1, byteFee);
    selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));
}

TEST(BitcoinUnspentSelector, SelectThreeNoDust) {
    auto utxos = buildTestUTXOs({100'000, 70'000, 75'000});

    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    auto indices = selector.select(100'000 - 174 - 10, 1);
    auto selected = unrollIndices(utxos, indices);

    // 100'000 would fit with dust; instead two UTXOs are selected not to leave dust
    EXPECT_TRUE(verifySelectedUTXOs(selected, {75'000, 100'000}));
    
    EXPECT_EQ(feeCalculator.calculate(1, 2, 1), 174);

    const auto dustLimit = 102;
    // Now 100'000 fits with no dust
    indices = selector.select(100'000 - 174 - dustLimit, 1);
    selected = unrollIndices(utxos, indices);
    EXPECT_TRUE(verifySelectedUTXOs(selected, {100'000}));

    // One more and we are over dust limit
    indices = selector.select(100'000 - 174 - dustLimit + 1, 1);
    selected = unrollIndices(utxos, indices);
    EXPECT_TRUE(verifySelectedUTXOs(selected, {75'000, 100'000}));
}

TEST(BitcoinUnspentSelector, SelectTwoFirstEnough) {
    auto utxos = buildTestUTXOs({20'000, 80'000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(15'000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {20'000}));
}

TEST(BitcoinUnspentSelector, SelectTwoSecondEnough) {
    auto utxos = buildTestUTXOs({20'000, 80'000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(70'000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {80'000}));
}

TEST(BitcoinUnspentSelector, SelectTwoBoth) {
    auto utxos = buildTestUTXOs({20'000, 80'000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(90'000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {20'000, 80'000}));
}

TEST(BitcoinUnspentSelector, SelectTwoFirstEnoughButSecond) {
    auto utxos = buildTestUTXOs({20'000, 22'000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(18'000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {22'000}));
}

TEST(BitcoinUnspentSelector, SelectTenThree) {
    auto utxos = buildTestUTXOs({1'000, 2'000, 100'000, 3'000, 4'000, 5,000, 125'000, 6'000, 150'000, 7'000});

    auto selector = UnspentSelector(utxoAmounts(utxos));
    auto indices = selector.select(300'000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {100'000, 125'000, 150'000}));
}

TEST(BitcoinUnspentSelector, SelectTenThreeExact) {
    auto utxos = buildTestUTXOs({1'000, 2'000, 100'000, 3'000, 4'000, 5,000, 125'000, 6'000, 150'000, 7'000});

    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    const auto dustLimit = 102;
    auto indices = selector.select(375'000 - 376 - dustLimit, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {100'000, 125'000, 150'000}));

    ASSERT_EQ(feeCalculator.calculate(3, 2, 1), 376);

    // one more, and it's too much
    indices = selector.select(375'000 - 376 - dustLimit + 1, 1);
    selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {7'000, 100'000, 125'000, 150'000}));
}

TEST(BitcoinUnspentSelector, SelectMaxAmountOne) {
    auto utxos = buildTestUTXOs({10189534});

    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    auto indices = selector.selectMaxAmount(1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {10189534}));

    EXPECT_EQ(feeCalculator.calculate(1, 2, 1), 174);
}

TEST(BitcoinUnspentSelector, SelectAllAvail) {
    auto utxos = buildTestUTXOs({10189534});

    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    auto indices = selector.select(10189534 - 226, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {10189534}));

    EXPECT_EQ(feeCalculator.calculate(1, 2, 1), 174);
}

TEST(BitcoinUnspentSelector, SelectMaxAmount5of5) {
    auto utxos = buildTestUTXOs({400, 500, 600, 800, 1000});

    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    auto byteFee = 1;
    auto indices = selector.selectMaxAmount(byteFee);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {400, 500, 600, 800, 1000}));

    EXPECT_EQ(feeCalculator.calculateSingleInput(byteFee), 102);
    EXPECT_EQ(feeCalculator.calculate(5, 1, byteFee), 548);
}

TEST(BitcoinUnspentSelector, SelectMaxAmount4of5) {
    auto utxos = buildTestUTXOs({400, 500, 600, 800, 1000});

    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    auto byteFee = 4;
    auto indices = selector.selectMaxAmount(byteFee);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {500, 600, 800, 1000}));

    EXPECT_EQ(feeCalculator.calculateSingleInput(byteFee), 408);
    EXPECT_EQ(feeCalculator.calculate(4, 1, byteFee), 1784);
}

TEST(BitcoinUnspentSelector, SelectMaxAmount1of5) {
    auto utxos = buildTestUTXOs({400, 500, 600, 800, 1000});

    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    auto byteFee = 8;
    auto indices = selector.selectMaxAmount(byteFee);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {1000}));

    EXPECT_EQ(feeCalculator.calculateSingleInput(byteFee), 816);
    EXPECT_EQ(feeCalculator.calculate(1, 1, byteFee), 1144);
}

TEST(BitcoinUnspentSelector, SelectMaxAmountNone) {
    auto utxos = buildTestUTXOs({400, 500, 600, 800, 1000});

    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    auto byteFee = 10;
    auto indices = selector.selectMaxAmount(byteFee);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));

    EXPECT_EQ(feeCalculator.calculateSingleInput(byteFee), 1020);
}

TEST(BitcoinUnspentSelector, SelectMaxAmountNoUTXOs) {
    auto utxos = buildTestUTXOs({});

    auto& feeCalculator = getFeeCalculator(TWCoinTypeBitcoin);
    auto selector = UnspentSelector(utxoAmounts(utxos), feeCalculator);
    auto indices = selector.selectMaxAmount(1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));
}

TEST(BitcoinUnspentSelector, SelectZcashUnspents) {
    auto utxos = buildTestUTXOs({100000, 2592, 73774});

    auto selector = UnspentSelector(utxoAmounts(utxos), getFeeCalculator(TWCoinTypeZcash));
    auto indices = selector.select(10000, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {73774}));
}

TEST(BitcoinUnspentSelector, SelectGroestlUnspents) {
    auto utxos = buildTestUTXOs({499971976});

    auto selector = UnspentSelector(utxoAmounts(utxos), getFeeCalculator(TWCoinTypeZcash));
    auto indices = selector.select(499951976, 1, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {499971976}));
}

TEST(BitcoinUnspentSelector, SelectZcashMaxAmount) {
    auto utxos = buildTestUTXOs({100000, 2592, 73774});

    auto selector = UnspentSelector(utxoAmounts(utxos), getFeeCalculator(TWCoinTypeZcash));
    auto indices = selector.selectMaxAmount(1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {100000, 2592, 73774}));
}

TEST(BitcoinUnspentSelector, SelectZcashMaxUnspents2) {
    auto utxos = buildTestUTXOs({100000, 2592, 73774});

    auto selector = UnspentSelector(utxoAmounts(utxos), getFeeCalculator(TWCoinTypeZcash));
    auto indices = selector.select(176366 - 6, 1);
    auto selected = unrollIndices(utxos, indices);

    EXPECT_TRUE(verifySelectedUTXOs(selected, {}));
}

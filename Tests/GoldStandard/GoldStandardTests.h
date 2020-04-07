#pragma once
struct GoldStandardTestsApi {
    SetupTest GoldStandardTestsSetup;
    VoidTest TrivialFlipTest;
};
const GoldStandardTestsApi& getapi();

// Note: add new test cases below as well
PRESENTMON_TEST_CLASS_BEGIN_WITH_SETUP(GoldStandardTests, GoldStandardTestsSetup)
PRESENTMON_TEST(GoldStandardTests, TrivialFlipTest) // Trivial D3D12 Flip Mode test case.
PRESENTMON_TEST_CLASS_END()
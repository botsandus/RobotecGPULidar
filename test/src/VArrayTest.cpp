#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <VArray.hpp>
#include <VArrayProxy.hpp>
#include <VArrayTyped.hpp>

using namespace ::testing;

class VArrayTypedTests :public ::testing::TestWithParam<int>
{
protected:
    int testDataSize;
};

INSTANTIATE_TEST_SUITE_P(
    VArrayTypedTests,
    VArrayTypedTests,
    ::testing::Values(
         0, 1, 5
        ));

TEST_P(VArrayTypedTests, Base)
{
    int testDataSize = GetParam();
    int* data = new int[testDataSize];
    for (int i = 0; i < testDataSize; ++i)
    {
        data[i] = i;
    }

    // ----------------------------------------- Untyped VArray ground truth tests. -------------------------------
    VArray::Ptr arrayUntyped = VArray::create<int>(0);

    arrayUntyped->setData(data, testDataSize);
    EXPECT_TRUE(arrayUntyped->getElemCount() == testDataSize);
    EXPECT_TRUE(arrayUntyped->getElemSize() == sizeof(int));

    const void* rawDataOfVArray = arrayUntyped->getReadPtr(MemLoc::Host);
    for (int i = 0; i < arrayUntyped->getElemCount(); ++i)
    {
        EXPECT_TRUE(reinterpret_cast<const int*>(rawDataOfVArray)[i] == i);
    }
    //--------------------------------------------------------------------------------------------------------------

    // ---------------------------------------------- Create new VArrayTyped. --------------------------------------
    VArrayTyped<int>::Ptr arrayTyped = VArrayTyped<int>::create(0);

    arrayTyped->setData(data, testDataSize);
    EXPECT_TRUE(arrayTyped->getElemCount() == testDataSize);
    EXPECT_TRUE(arrayTyped->getElemSize() == sizeof(int));

    const int* typedDataOfVArrayTyped = arrayTyped->getReadPtr(MemLoc::Host);
    for (int i = 0; i < arrayTyped->getElemCount(); ++i)
    {
        EXPECT_TRUE(typedDataOfVArrayTyped[i] == i);
    }
    //--------------------------------------------------------------------------------------------------------------

    // ------------------------------------ Create VArrayTyped from existing VArray --------------------------------
    VArrayTyped<int>::Ptr arrayTypedFromUntyped = VArrayTyped<int>::create(arrayUntyped);

    EXPECT_TRUE(arrayTypedFromUntyped->getElemCount() == arrayUntyped->getElemCount());
    EXPECT_TRUE(arrayTypedFromUntyped->getElemSize() == arrayUntyped->getElemSize());
    EXPECT_TRUE(arrayTypedFromUntyped->getElemCapacity() == arrayUntyped->getElemCapacity());

    const int* typedDataOfVArrayTypedFromVArray = arrayTypedFromUntyped->getReadPtr(MemLoc::Host);

    for (int i = 0; i < arrayUntyped->getElemCount(); ++i)
    {
        EXPECT_TRUE(typedDataOfVArrayTypedFromVArray[i] == reinterpret_cast<const int*>(typedDataOfVArrayTyped)[i]);
    }
    //--------------------------------------------------------------------------------------------------------------
}

TEST(VArrayTyped, Smoke)
{
    VArrayTyped<int>::Ptr array = VArrayTyped<int>::create(1);
    int value = 4;

    {
        // Goal:
        // CPU: ||
        // GPU: |4|
        int* devicePtr = array->getWritePtr(MemLoc::Device);
        EXPECT_TRUE(devicePtr != nullptr);
        CHECK_CUDA(cudaMemcpy(devicePtr, &value, sizeof(int), cudaMemcpyDefault));
    }
    {
        // CPU: |4|
        // GPU: |4|
        const int* hostPtr = reinterpret_cast<const int*>(array->getReadPtr(MemLoc::Host));
        EXPECT_TRUE(hostPtr != nullptr);
        EXPECT_EQ(hostPtr[0], value);
    }
    {
        // CPU: |4|3|0|
        // GPU: |4|
        array->resize(3);
        EXPECT_EQ(array->getCount(), 3);
        (*array)[1] = 3;
        // Last element should have been zero-initialized
    }
    {
        // CPU: |4|3|0|
        // GPU: |4|3|0|
        int values[3];
        const int* devicePtr = array->getReadPtr(MemLoc::Device);
        CHECK_CUDA(cudaMemcpy(values, devicePtr, 3 * sizeof(int), cudaMemcpyDefault));
        EXPECT_EQ(values[0], 4);
        EXPECT_EQ(values[1], 3);
        EXPECT_EQ(values[2], 0);
    }
}

TEST(VArrayTyped, SmokeInsert)
{
    // Default location is Device
    VArrayTyped<int>::Ptr array = VArrayTyped<int>::create(1);

    {
        // Goal:
        // CPU: ||
        // GPU: |1|2|3|4|5|
        array->resize(3);
        int value = 1;
        array->insertData(&value, 1, 0);
        value = 2;
        array->insertData(&value, 1, 1);
        value = 3;
        array->insertData(&value, 1, 2);
        // Next inserts should resize array with preserveData
        value = 4;
        array->insertData(&value, 1, 3);
        value = 5;
        array->insertData(&value, 1, 4);
    }
    {
        // CPU: |1|2|3|4|5|
        // GPU: |1|2|3|4|5|
        const int* hostPtr = reinterpret_cast<const int*>(array->getReadPtr(MemLoc::Host));
        EXPECT_TRUE(hostPtr != nullptr);
        EXPECT_EQ(hostPtr[0], 1);
        EXPECT_EQ(hostPtr[1], 2);
        EXPECT_EQ(hostPtr[2], 3);
        EXPECT_EQ(hostPtr[3], 4);
        EXPECT_EQ(hostPtr[4], 5);
    }
}

TEST(VArray, Smoke)
{
    VArrayProxy<int>::Ptr array = VArrayProxy<int>::create(1);
    int value = 4;

    {
        // Goal:
        // CPU: ||
        // GPU: |4|
        int* devicePtr = array->getWritePtr(MemLoc::Device);
        EXPECT_TRUE(devicePtr != nullptr);
        CHECK_CUDA(cudaMemcpy(devicePtr, &value, sizeof(int), cudaMemcpyDefault));
    }
    {
        // CPU: |4|
        // GPU: |4|
        const int* hostPtr = reinterpret_cast<const int*>(array->getReadPtr(MemLoc::Host));
        EXPECT_TRUE(hostPtr != nullptr);
        EXPECT_EQ(hostPtr[0], value);
    }
    {
        // CPU: |4|3|0|
        // GPU: |4|
        array->resize(3);
        EXPECT_EQ(array->getCount(), 3);
        (*array)[1] = 3;
        // Last element should have been zero-initialized
    }
    {
        // CPU: |4|3|0|
        // GPU: |4|3|0|
        int values[3];
        const int* devicePtr = array->getReadPtr(MemLoc::Device);
        CHECK_CUDA(cudaMemcpy(values, devicePtr, 3 * sizeof(int), cudaMemcpyDefault));
        EXPECT_EQ(values[0], 4);
        EXPECT_EQ(values[1], 3);
        EXPECT_EQ(values[2], 0);
    }
}

TEST(VArray, SmokeInsert)
{
    // Default location is Device
    VArrayProxy<int>::Ptr array = VArrayProxy<int>::create(1);

    {
        // Goal:
        // CPU: ||
        // GPU: |1|2|3|4|5|
        array->resize(3);
        int value = 1;
        array->insertData(&value, 1, 0);
        value = 2;
        array->insertData(&value, 1, 1);
        value = 3;
        array->insertData(&value, 1, 2);
        // Next inserts should resize array with preserveData
        value = 4;
        array->insertData(&value, 1, 3);
        value = 5;
        array->insertData(&value, 1, 4);
    }
    {
        // CPU: |1|2|3|4|5|
        // GPU: |1|2|3|4|5|
        const int* hostPtr = reinterpret_cast<const int*>(array->getReadPtr(MemLoc::Host));
        EXPECT_TRUE(hostPtr != nullptr);
        EXPECT_EQ(hostPtr[0], 1);
        EXPECT_EQ(hostPtr[1], 2);
        EXPECT_EQ(hostPtr[2], 3);
        EXPECT_EQ(hostPtr[3], 4);
        EXPECT_EQ(hostPtr[4], 5);
    }
}

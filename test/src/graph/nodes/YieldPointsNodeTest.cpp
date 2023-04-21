#include <graph/Node.hpp>
#include <gtest/gtest.h>
#include <utils.hpp>
#include <RGLFields.hpp>

std::vector<rgl_field_t> fields = {
    XYZ_F32,
    IS_HIT_I32,
    RAY_IDX_U32,
    INTENSITY_F32,
    RING_ID_U16,
    AZIMUTH_F32,
    DISTANCE_F32,
    RETURN_TYPE_U8,
    TIME_STAMP_F64,
    PADDING_8,
    PADDING_16,
    PADDING_32 };

// All subsets of set
template <typename T>
std::vector< std::vector<T> > getAllSubsets(std::vector<T> set)
{
    std::vector< std::vector<T> > subset;
    std::vector<T> empty;
    subset.push_back(empty);

    for (int i = 0; i < set.size(); i++)
    {
        std::vector< std::vector<T> > subsetTemp = subset;

        for (int j = 0; j < subsetTemp.size(); j++)
            subsetTemp[j].push_back(set[i]);

        for (int j = 0; j < subsetTemp.size(); j++)
            subset.push_back(subsetTemp[j]);
    }
    return subset;
}

class YieldPointsNodeTest : public RGLTestWithParam<std::vector<rgl_field_t>>{
protected:

};

INSTANTIATE_TEST_SUITE_P(
    YieldPointsNodeTests, YieldPointsNodeTest,
    testing::ValuesIn(getAllSubsets(fields)));

TEST_P(YieldPointsNodeTest, blabla)
{
    rgl_node_t yieldNode = nullptr;
    std::vector<rgl_field_t> fieldsParam = GetParam();
    if(fieldsParam.empty()) {
        return;
    }

    EXPECT_RGL_SUCCESS(rgl_node_points_yield(&yieldNode, fieldsParam.data(), fieldsParam.size()));
    ASSERT_THAT(yieldNode, testing::NotNull());

    // If (*yieldNode) != nullptr
    EXPECT_RGL_SUCCESS(rgl_node_points_yield(&yieldNode, fieldsParam.data(), fieldsParam.size()));
}

TEST_F(YieldPointsNodeTest, invalid_arguments)
{
    rgl_node_t yieldNode = nullptr;

    EXPECT_RGL_INVALID_ARGUMENT(rgl_node_points_yield(nullptr, nullptr, 0), "node != nullptr");

    EXPECT_RGL_INVALID_ARGUMENT(rgl_node_points_yield(&yieldNode, nullptr, 0), "fields != nullptr");

    yieldNode = nullptr;
    EXPECT_RGL_INVALID_ARGUMENT(rgl_node_points_yield(&yieldNode, fields.data(), 0), "field_count > 0");
}

TEST_F(YieldPointsNodeTest, valid_arguments)
{
    rgl_node_t yieldNode = nullptr;

    EXPECT_RGL_SUCCESS(rgl_node_points_yield(&yieldNode, fields.data(), fields.size()));
    ASSERT_THAT(yieldNode, testing::NotNull());

    // If (*yieldNode) != nullptr
    EXPECT_RGL_SUCCESS(rgl_node_points_yield(&yieldNode, fields.data(), fields.size()));
}

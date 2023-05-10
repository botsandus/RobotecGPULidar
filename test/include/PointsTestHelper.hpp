#include <utils.hpp>
#include <random>

static std::random_device randomDevice;
static auto randomSeed = randomDevice();
static std::mt19937 randomGenerator {randomSeed};

struct RGLPointsNodeTestHelper {
    
protected:
    rgl_node_t usePointsNode = nullptr;

    std::vector<rgl_field_t> pointFields = {
        XYZ_F32,
        IS_HIT_I32,
        INTENSITY_F32
    };

    std::vector<rgl_field_t> pointFieldsWithoutIsHit = {
        XYZ_F32,
        INTENSITY_F32
    };

    struct TestPointStruct {
        Field<XYZ_F32>::type xyz;
        Field<IS_HIT_I32>::type isHit;
        Field<INTENSITY_F32>::type intensity;
    };

    struct TestPointStructWithoutIsHit {
        Field<XYZ_F32>::type xyz;
        Field<INTENSITY_F32>::type intensity;
    };

    std::vector<TestPointStruct> inPoints;

    enum class HitPointDensity {
        HALF_HIT = 0,
        ALL_NON_HIT,
        ALL_HIT,
        RANDOM
    };

    int32_t randomNonHitCount = 0;

    std::vector<TestPointStruct> generateTestPointsArray(
            int count,
            rgl_mat3x4f transform = identityTestTransform,
            HitPointDensity hitPointDensity = HitPointDensity::HALF_HIT
            )
    {
        std::vector<TestPointStruct> points;
        randomNonHitCount = 0;

        for (int i = 0; i < count; ++i) {
            auto currentPoint = TestPointStruct {
                .xyz = { i, i + 1, i + 2 },
                .isHit = determineIsHit(i, hitPointDensity),
                .intensity = 100 };
            currentPoint.xyz = Mat3x4f::fromRGL(transform) * currentPoint.xyz;
            points.emplace_back(currentPoint);
        }
        return points;
    }

    //TODO(nebraszka): To be changed so as not to duplicate the code
    std::vector<TestPointStructWithoutIsHit> generateTestPointsArrayWithoutIsHit(
        int count,
        rgl_mat3x4f transform = identityTestTransform
    )
    {
        std::vector<TestPointStructWithoutIsHit> points;

        for (int i = 0; i < count; ++i) {
            auto currentPoint = TestPointStructWithoutIsHit {
                .xyz = { i, i + 1, i + 2 },
                .intensity = 100 };
            currentPoint.xyz = Mat3x4f::fromRGL(transform) * currentPoint.xyz;
            points.emplace_back(currentPoint);
        }
        return points;
    }

    void createTestUsePointsNode(
            int pointsCount,
            rgl_mat3x4f transform = identityTestTransform,
            HitPointDensity hitPointDensity = HitPointDensity::HALF_HIT)
    {
        inPoints = generateTestPointsArray(pointsCount, transform, hitPointDensity);

        EXPECT_RGL_SUCCESS(rgl_node_points_from_array(&usePointsNode, inPoints.data(), inPoints.size(), pointFields.data(), pointFields.size()));
        ASSERT_THAT(usePointsNode, testing::NotNull());
    }

    std::vector<TestPointStruct> separateHitPoints() {
        std::vector<TestPointStruct> hitPoints;
        for(auto point: inPoints) {
            if(point.isHit == 1) {
                hitPoints.push_back(point);
            }
        }
        return hitPoints;
    }

    rgl_node_t simulateEmptyPointCloudInputNode() {
        createTestUsePointsNode(1, identityTestTransform, HitPointDensity::ALL_NON_HIT);
        rgl_node_t compactNode = nullptr;
        EXPECT_RGL_SUCCESS(rgl_node_points_compact(&compactNode));
        EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(usePointsNode, compactNode));

        return compactNode;
    }

private:
    int32_t determineIsHit(int index, HitPointDensity hitPointDensity){
        switch(hitPointDensity){
            case HitPointDensity::HALF_HIT:
                return index % 2;
            case HitPointDensity::ALL_NON_HIT:
                return 0;
            case HitPointDensity::ALL_HIT:
                return 1;
            case HitPointDensity::RANDOM:
            {
                int isHit = randomGenerator() % 2;
                if(!isHit) {
                    randomNonHitCount++;
                }
                return isHit;
            }
            default:
                return 0;
        }
    }
};
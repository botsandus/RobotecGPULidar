#include <utils.hpp>
#include <scenes.hpp>
#include <RGLFields.hpp>

#define CUBE_FROM_CENTER_DISTANCE 1.0f
#define RAYTRACE_DEPTH 100

class DistanceFieldTest : public RGLTest {
protected:
    rgl_node_t useRaysNode, raytraceNode, gaussianNoiseNode, yieldPointsNode, compactPointsNode;
    rgl_mat3x4f rayTf;
    int32_t distanceFieldCount, distanceFieldSize;
    std::vector<::Field<DISTANCE_F32>::type> distanceFieldData;

    DistanceFieldTest()
    {
        useRaysNode = nullptr;
        raytraceNode = nullptr;
        gaussianNoiseNode = nullptr;
        yieldPointsNode = nullptr;
        compactPointsNode = nullptr;

        rayTf = Mat3x4f::identity().toRGL();
    }

    void prepareSceneWithCube(Vec3f cubeTranslation)
    {
        rgl_mesh_t cubeMesh = makeCubeMesh();
        rgl_entity_t cubeEntity = nullptr;
	    EXPECT_RGL_SUCCESS(rgl_entity_create(&cubeEntity, nullptr, cubeMesh));
        rgl_mat3x4f entityTf = Mat3x4f::translation(cubeTranslation).toRGL();
        EXPECT_RGL_SUCCESS(rgl_entity_set_pose(cubeEntity, &entityTf));
    }

    void connectNodes(bool withGaussianNoiseNode)
    {
        ASSERT_RGL_SUCCESS(rgl_graph_node_add_child(useRaysNode, raytraceNode));
        ASSERT_RGL_SUCCESS(rgl_graph_node_add_child(raytraceNode, compactPointsNode));
        if(withGaussianNoiseNode) {
            ASSERT_RGL_SUCCESS(rgl_graph_node_add_child(compactPointsNode, gaussianNoiseNode));
            ASSERT_RGL_SUCCESS(rgl_graph_node_add_child(gaussianNoiseNode, yieldPointsNode));
        } else {
            ASSERT_RGL_SUCCESS(rgl_graph_node_add_child(compactPointsNode, yieldPointsNode));
        }
    }

    void prepareNodesForRaytracingAndGettingDistance() {
        ASSERT_RGL_SUCCESS(rgl_node_rays_from_mat3x4f(&useRaysNode, &rayTf, sizeof(rayTf)));
        ASSERT_RGL_SUCCESS(rgl_node_raytrace(&raytraceNode, nullptr, RAYTRACE_DEPTH));
        std::vector<rgl_field_t> fields = {DISTANCE_F32};
        ASSERT_RGL_SUCCESS(rgl_node_points_yield(&yieldPointsNode, fields.data(), fields.size()));
        ASSERT_RGL_SUCCESS(rgl_node_points_compact(&compactPointsNode));
    }

    void disconnectNodes(bool withGaussianNoiseNode) {
        ASSERT_RGL_SUCCESS(rgl_graph_node_remove_child(useRaysNode, raytraceNode));
        ASSERT_RGL_SUCCESS(rgl_graph_node_remove_child(raytraceNode, compactPointsNode));
        if(withGaussianNoiseNode) {
            ASSERT_RGL_SUCCESS(rgl_graph_node_remove_child(compactPointsNode, gaussianNoiseNode));
            ASSERT_RGL_SUCCESS(rgl_graph_node_remove_child(gaussianNoiseNode, yieldPointsNode));
        } else {
            ASSERT_RGL_SUCCESS(rgl_graph_node_remove_child(compactPointsNode, yieldPointsNode));
        }
    }

    void getResults() {
        ASSERT_RGL_SUCCESS(rgl_graph_get_result_size(yieldPointsNode, DISTANCE_F32, &distanceFieldCount, &distanceFieldSize));
        distanceFieldData.resize(distanceFieldCount);
	    ASSERT_RGL_SUCCESS(rgl_graph_get_result_data(yieldPointsNode, DISTANCE_F32, distanceFieldData.data()));
    }

};

TEST_F(DistanceFieldTest, simple_use_case_usage_example)
{
    float cubeZAxisTranslation = 5.0f;
    Vec3f cubeTranslation = {0.0f, 0.0f, cubeZAxisTranslation};
    prepareSceneWithCube(cubeTranslation);

    prepareNodesForRaytracingAndGettingDistance();

    connectNodes(false);

	ASSERT_RGL_SUCCESS(rgl_graph_run(yieldPointsNode));

    getResults();

    EXPECT_EQ(distanceFieldData.at(0), cubeZAxisTranslation - CUBE_FROM_CENTER_DISTANCE);
}

TEST_F(DistanceFieldTest, should_compute_distance_from_ray_beginning)
{
    float cubeZAxisTranslation = 10.0f;
    Vec3f cubeTranslation = {0.0f, 0.0f, cubeZAxisTranslation};
    prepareSceneWithCube(cubeTranslation);

    prepareNodesForRaytracingAndGettingDistance();

    connectNodes(false);

	ASSERT_RGL_SUCCESS(rgl_graph_run(yieldPointsNode));

    getResults();

	ASSERT_RGL_SUCCESS(rgl_graph_get_result_data(yieldPointsNode, DISTANCE_F32, distanceFieldData.data()));

    float distance = distanceFieldData.at(0);

    // Translate ray position and compute the distance again
    float rayZAxisTranslation = 5.0f;
    rayTf = Mat3x4f::translation(0.0f, 0.0f, rayZAxisTranslation).toRGL();
    ASSERT_RGL_SUCCESS(rgl_node_rays_from_mat3x4f(&useRaysNode, &rayTf, sizeof(rayTf)));
    ASSERT_RGL_SUCCESS(rgl_graph_run(yieldPointsNode));

    getResults();
    
    float distanceAfterRayTranslation = distanceFieldData.at(0);

    // The distance should change if it is computed from the beginning of the ray 
    // and not from the beginning of its coordinate system
    EXPECT_EQ(distance - distanceAfterRayTranslation, rayZAxisTranslation);

}

TEST_F(DistanceFieldTest, should_change_distance_when_gaussian_distance_noise_considered_mean)
{
    float cubeZAxisTranslation = 10.0f;
    Vec3f cubeTranslation = {0.0f, 0.0f, cubeZAxisTranslation};
    prepareSceneWithCube(cubeTranslation);

    prepareNodesForRaytracingAndGettingDistance();

    connectNodes(false);

	ASSERT_RGL_SUCCESS(rgl_graph_run(yieldPointsNode));

	getResults();

	EXPECT_RGL_SUCCESS(rgl_graph_get_result_data(yieldPointsNode, DISTANCE_F32, distanceFieldData.data()));

    float distance = distanceFieldData.at(0);
	
    float mean = 17.23f;
    ASSERT_RGL_SUCCESS(rgl_node_gaussian_noise_distance(&gaussianNoiseNode, mean, 0.0f, 0.0f));
    ASSERT_RGL_SUCCESS(rgl_node_raytrace(&raytraceNode, nullptr, RAYTRACE_DEPTH));

    disconnectNodes(false);
    connectNodes(true);
    ASSERT_RGL_SUCCESS(rgl_graph_run(yieldPointsNode));
    getResults();

    float distanceAfterNoise = distanceFieldData.at(0);

    EXPECT_EQ(distanceAfterNoise - distance, mean);
}

TEST_F(DistanceFieldTest, should_change_distance_when_gaussian_distance_noise_considered_mean_with_std_dev)
{
    float cubeZAxisTranslation = 10.0f;
    Vec3f cubeTranslation = {0.0f, 0.0f, cubeZAxisTranslation};
    prepareSceneWithCube(cubeTranslation);

    prepareNodesForRaytracingAndGettingDistance();

    connectNodes(false);

	ASSERT_RGL_SUCCESS(rgl_graph_run(yieldPointsNode));

	getResults();

    float distance = distanceFieldData.at(0);
	
    float mean = 17.23f;
    ASSERT_RGL_SUCCESS(rgl_node_gaussian_noise_distance(&gaussianNoiseNode, 0.0f, 100.0f, 0.0f));
    ASSERT_RGL_SUCCESS(rgl_node_raytrace(&raytraceNode, nullptr, RAYTRACE_DEPTH));

    disconnectNodes(false);
    connectNodes(true);
    ASSERT_RGL_SUCCESS(rgl_graph_run(yieldPointsNode));
    getResults();

    float distanceAfterNoise = distanceFieldData.at(0);

    std::cerr << distanceAfterNoise - distance << std::endl;
}

/* Gaussian Noise Test plan:
    - jak mean wpływa na odległość, jak std_dev, jak std_dev_2  
    - jak mogę sprawdzać działanie std_dev??? sigma, 2sigma,
*/ 

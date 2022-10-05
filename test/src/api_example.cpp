#include <gtest/gtest.h>
#include <rgl/api/experimental.h>

#define RGL_CHECK(call)                  \
do {                                     \
	rgl_status_t status = call;          \
	if (status != RGL_SUCCESS) {         \
		const char* msg;                 \
		rgl_get_last_error_string(&msg); \
		FAIL() << msg;                   \
	}                                    \
} while(0)


rgl_vec3f cube_vertices[] = {
	{-1, -1, -1},
	{1, -1, -1},
	{1, 1, -1},
	{-1, 1, -1},
	{-1, -1, 1},
	{1, -1, 1},
	{1, 1, 1},
	{-1, 1, 1}
};

constexpr size_t cube_vertices_length = sizeof(cube_vertices) / sizeof(cube_vertices[0]);

rgl_vec3i cube_indices[] = {
	{0, 1, 3},
	{3, 1, 2},
	{1, 5, 2},
	{2, 5, 6},
	{5, 4, 6},
	{6, 4, 7},
	{4, 0, 7},
	{7, 0, 3},
	{3, 2, 7},
	{7, 2, 6},
	{4, 5, 0},
	{0, 5, 1},
};
constexpr size_t cube_indices_length = sizeof(cube_indices) / sizeof(cube_indices[0]);


TEST(API, Readme_Example)
{
	// Load mesh from a file
	rgl_mesh_t cube_mesh = 0;
	RGL_CHECK(rgl_mesh_create(&cube_mesh, cube_vertices, cube_vertices_length, cube_indices, cube_indices_length));

	// Put an entity on the default scene
	rgl_entity_t cube_entity = 0;
	RGL_CHECK(rgl_entity_create(&cube_entity, NULL, cube_mesh));

	// Set position of the cube entity to (0, 0, 5)
	rgl_mat3x4f entity_tf = {
		.value = {
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 5}
		}
	};
	RGL_CHECK(rgl_entity_set_pose(cube_entity, &entity_tf));

	// Create a description of lidar that sends 1 ray
	// By default, lidar will have infinite ray range
	// and will be placed in (0, 0, 0), facing positive Z
	rgl_lidar_t lidar = 0;
	rgl_mat3x4f ray_tf[] = {
    {.value = {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    }},
    {.value = {
    {1, 0, 0, 0},
    {0, 1, 0, 0.1},
    {0, 0, 1, 0},
    }},
    {.value = {
    {1, 0, 0, 0},
    {0, 1, 0, 0.2},
    {0, 0, 1, 0},
    }},
    {.value = {
    {1, 0, 0, 0},
    {0, 1, 0, 0.3},
    {0, 0, 1, 0},
    }},
	};
    int frameSizes[] = {1,2,1};
    int frameCount = 3;
	// RGL_CHECK(rgl_lidar_create(&lidar, &ray_tf, 1));
	RGL_CHECK(rgl_lidar_create_with_frames(&lidar, ray_tf, 1, frameSizes, frameCount));

    for (int i = 0; i < frameCount*2; ++i) {
        // Start raytracing on the default scene
        RGL_CHECK(rgl_lidar_raytrace_async(NULL, lidar));

        // Wait for raytracing (if needed) and collect results
        int hitpoint_count = 0;
        rgl_vec3f results[1] = {0};
        RGL_CHECK(rgl_lidar_get_output_size(lidar, &hitpoint_count));
        RGL_CHECK(rgl_lidar_get_output_data(lidar, RGL_FORMAT_XYZ, results));

        printf("Got %d hitpoint(s)\n", hitpoint_count);
        for (int i = 0; i < hitpoint_count; ++i) {
            printf("- (%.2f, %.2f, %.2f)\n", results[i].value[0], results[i].value[1], results[i].value[2]);
        }
        RGL_CHECK(rgl_lidar_next_frame(lidar));

        // ASSERT_EQ(hitpoint_count, frameSizes[i]);
        // ASSERT_FLOAT_EQ(results[0].value[2], 4.0f);

    }
}
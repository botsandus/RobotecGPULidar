// Copyright 2022 Robotec.AI
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

// Hack to complete compilation on Windows. In runtime it is never used.
#ifdef _WIN32
	#include <io.h>
	#define PROT_READ 1
	#define MAP_PRIVATE 1
	#define MAP_FAILED nullptr
	static int munmap(void* addr, size_t length) { return -1; }
	static void* mmap(void* start, size_t length, int prot, int flags, int fd, size_t offset) { return nullptr; }
#else
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif // _WIN32

#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <string> 
#include <optional>
#include <unordered_map>
#include <map>

#include <spdlog/fmt/bundled/format.h>
#include <yaml-cpp/yaml.h>

#include <Logger.hpp>
#include <RGLExceptions.hpp>
#include <rgl/api/core.h>

#define BIN_EXTENSION ".bin"
#define YAML_EXTENSION ".yaml"
#define RGL_VERSION "rgl_version"

#ifdef _WIN32
#define TAPE_HOOK(...)
#else
#define TAPE_HOOK(...)                                              \
do if (tapeRecord.has_value()) {                                    \
	tapeRecord->recordApiCall(__func__ __VA_OPT__(, ) __VA_ARGS__); \
} while (0)
#endif // _WIN32

#define TAPE_ARRAY(data, count) std::make_pair(data, count)
#define FWRITE(source, elemSize, elemCount, file)                          \
do if (fwrite(source, elemSize, elemCount, file) != elemCount) {           \
    throw RecordError(fmt::format("Failed to write data to binary file")); \
} while(0)

class TapeRecord
{
	YAML::Node yamlRoot; // Represents the whole yaml file
	YAML::Node yamlRecording; // The sequence of API calls

	FILE* fileBin;
	std::ofstream fileYaml;

	size_t currentBinOffset = 0;

	static void recordRGLVersion(YAML::Node& node);

	template<typename T>
	size_t writeToBin(const T* source, size_t elemCount)
	{
		size_t elemSize = sizeof(T);
		uint8_t remainder = (elemSize * elemCount) % 16;
		uint8_t bytesToAdd = (16 - remainder) % 16;

		FWRITE(source, elemSize, elemCount, fileBin);
		if (remainder != 0) {
			uint8_t zeros[16];
			FWRITE(zeros, sizeof(uint8_t), bytesToAdd, fileBin);
		}

		size_t outBinOffest = currentBinOffset;
		currentBinOffset += elemSize * elemCount + bytesToAdd;
		return outBinOffest;
	}

	template<typename T, typename... Args>
	void recordApiArguments(YAML::Node& node, int currentArgIdx, T currentArg, Args... remainingArgs)
	{
		node[currentArgIdx] = valueToYaml(currentArg);

		if constexpr(sizeof...(remainingArgs) > 0) {
			recordApiArguments(node, ++currentArgIdx, remainingArgs...);
			return;
		}
	}

	//// value to yaml converters
	template<typename T>
	T valueToYaml(T value) { return value; }

	uintptr_t valueToYaml(rgl_node_t value) { return (uintptr_t) value; }
	uintptr_t valueToYaml(rgl_node_t* value) { return (uintptr_t) *value; }

	uintptr_t valueToYaml(rgl_entity_t value) { return (uintptr_t) value; }
	uintptr_t valueToYaml(rgl_entity_t* value) { return (uintptr_t) *value; }

	uintptr_t valueToYaml(rgl_mesh_t value) { return (uintptr_t) value; }
	uintptr_t valueToYaml(rgl_mesh_t* value) { return (uintptr_t) *value; }

	uintptr_t valueToYaml(rgl_scene_t value) { return (uintptr_t) value; }
	uintptr_t valueToYaml(rgl_scene_t* value) { return (uintptr_t) *value; }

	uintptr_t valueToYaml(void* value) { return (uintptr_t) value; }

	int valueToYaml(int32_t* value) { return *value; }
	int valueToYaml(rgl_field_t value) { return (int)value; }
	int valueToYaml(rgl_log_level_t value) { return (int)value; }

	size_t valueToYaml(const rgl_mat3x4f* value) { return writeToBin(value, 1); }

	// TAPE_ARRAY
	template<typename T, typename N>
	size_t valueToYaml(std::pair<T, N> value) { return writeToBin(value.first, value.second); }

public:
	explicit TapeRecord(const std::filesystem::path& path);

	~TapeRecord();

	template<typename... Args>
	void recordApiCall(std::string fnName, Args... args)
	{
		YAML::Node apiCallNode;
		apiCallNode["name"] = fnName;
		if constexpr(sizeof...(args) > 0) {
			recordApiArguments(apiCallNode, 0, args...);
		}
		yamlRecording.push_back(apiCallNode);
	}
};

class TapePlay
{
	YAML::Node yamlRoot;
	uint8_t* fileMmap{};
	size_t mmapSize{};

	std::unordered_map<size_t, rgl_mesh_t> tapeMeshes;
	std::unordered_map<size_t, rgl_entity_t> tapeEntities;
	std::unordered_map<size_t, rgl_node_t> tapeNodes;

	std::map<std::string, std::function<void(const YAML::Node&)>> tapeFunctions;

	void tape_get_version_info(const YAML::Node& yamlNode);
	void tape_configure_logging(const YAML::Node& yamlNode);
	void tape_cleanup(const YAML::Node& yamlNode);
	void tape_mesh_create(const YAML::Node& yamlNode);
	void tape_mesh_destroy(const YAML::Node& yamlNode);
	void tape_mesh_update_vertices(const YAML::Node& yamlNode);
	void tape_entity_create(const YAML::Node& yamlNode);
	void tape_entity_destroy(const YAML::Node& yamlNode);
	void tape_entity_set_pose(const YAML::Node& yamlNode);
	void tape_entity_set_laser_retro(const YAML::Node& yamlNode);
	void tape_graph_run(const YAML::Node& yamlNode);
	void tape_graph_destroy(const YAML::Node& yamlNode);
	void tape_graph_get_result_size(const YAML::Node& yamlNode);
	void tape_graph_get_result_data(const YAML::Node& yamlNode);
	void tape_graph_node_set_active(const YAML::Node& yamlNode);
	void tape_graph_node_add_child(const YAML::Node& yamlNode);
	void tape_graph_node_remove_child(const YAML::Node& yamlNode);
	void tape_node_rays_from_mat3x4f(const YAML::Node& yamlNode);
	void tape_node_rays_set_ring_ids(const YAML::Node& yamlNode);
	void tape_node_rays_transform(const YAML::Node& yamlNode);
	void tape_node_points_transform(const YAML::Node& yamlNode);
	void tape_node_raytrace(const YAML::Node& yamlNode);
	void tape_node_points_format(const YAML::Node& yamlNode);
	void tape_node_points_yield(const YAML::Node& yamlNode);
	void tape_node_points_compact(const YAML::Node& yamlNode);
	void tape_node_points_downsample(const YAML::Node& yamlNode);
	void tape_node_points_write_pcd_file(const YAML::Node& yamlNode);
	void tape_node_points_visualize(const YAML::Node& yamlNode);

	void mmapInit(const char* path);

public:

	explicit TapePlay(const char* path);
	~TapePlay();
};

extern std::optional<TapeRecord> tapeRecord;
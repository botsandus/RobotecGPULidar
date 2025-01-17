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

#include <scene/Scene.hpp>
#include <scene/Mesh.hpp>
#include <APIObject.hpp>
#include <utility>
#include <math/Mat3x4f.hpp>

constexpr float DEFAULT_LASER_RETRO = 100.0;

struct Entity : APIObject<Entity>
{
	Entity(std::shared_ptr<Mesh> mesh, std::optional<std::string> name=std::nullopt);

	// TODO(prybicki): low-prio optimization: do not rebuild whole IAS if only transform changed
	void setTransform(Mat3x4f newTransform);
	OptixInstance getIAS(int idx);
	void setLaserRetro(float retro);
	const float getLaserRetro() { return laser_retro;}
	std::shared_ptr<Mesh> mesh;
	std::weak_ptr<Scene> scene;
private:
	Mat3x4f transform;
	float laser_retro;
	std::optional<std::string> humanReadableName;
	friend struct APIObject<Entity>;
	friend struct Scene;
};

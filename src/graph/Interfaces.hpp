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

#include <rgl/api/core.h>
#include <math/Mat3x4f.hpp>
#include <VArray.hpp>
#include <VArrayProxy.hpp>
#include <RGLFields.hpp>
#include <gpu/GPUFieldDesc.hpp>
#include <memory/ConcreteArrays.hpp>

/**
 * Note: this interface may be removed in the future (rays will become a part of the point cloud)
 */
struct IRaysNode
{
	using Ptr = std::shared_ptr<IRaysNode>;
	using ConstPtr = std::shared_ptr<const IRaysNode>;

	virtual std::size_t getRayCount() const = 0;
	virtual DeviceAsyncArray<Mat3x4f>::ConstPtr getRays() const = 0;

	// RingIds are client-specific ray property.
	// They are optional, since such concept is not applicable to some lidar implementations.
	virtual std::optional<std::size_t> getRingIdsCount() const = 0;
	virtual std::optional<DeviceAsyncArray<int>::ConstPtr> getRingIds() const = 0;

	// Helper method to e.g. correctly apply gaussian noise, accounting for previous transformations.
	// TODO: It's worth reconsidering to handle this on graph construction level, e.g. forbid transforms before noise.
	virtual Mat3x4f getCumulativeRayTransfrom() const { return Mat3x4f::identity(); }
};

// TODO: I*SingleInput convenience interfaces rely on correct behavior of subclass, i.e. getting input pointer.
// TODO: This could be handled more automatically. Example approach below. If it looks OK, please remove this TODO on review.
struct IRaysNodeSingleInput : IRaysNode
{
	using Ptr = std::shared_ptr<IRaysNodeSingleInput>;

	size_t getRayCount() const override { return getInput()->getRayCount(); }
	virtual DeviceAsyncArray<Mat3x4f>::ConstPtr getRays() const override { return getInput()->getRays(); };

	std::optional<size_t> getRingIdsCount() const override { return getInput()->getRingIdsCount(); }
	virtual std::optional<DeviceAsyncArray<int>::ConstPtr> getRingIds() const override { return getInput()->getRingIds(); }

	virtual Mat3x4f getCumulativeRayTransfrom() const override { return getInput()->getCumulativeRayTransfrom(); }

protected:
	IRaysNode::Ptr getInput() const
	{
		if (input == nullptr) {
			input = loadInputNode();
		}
		return input;
	}
	virtual IRaysNode::Ptr loadInputNode() const = 0;
protected:
	mutable IRaysNode::Ptr input;
};

// TODO(prybicki): getFieldData* may act lazily, so they take stream as a parameter to do the lazy evaluation.
// TODO(prybicki): This requires synchronizing with the potentially different stream provided by the schedule(...)
// TODO(prybicki): This situation is bug-prone, requiring greater mental effort when implementing nodes.
// TODO(prybicki): It might be better to remove stream as a parameter and assume that all pipeline nodes are using common stream.
struct IPointsNode
{
	using Ptr = std::shared_ptr<IPointsNode>;

	// Node requirements
	virtual std::vector<rgl_field_t> getRequiredFieldList() const { return {}; };

	// Point cloud description
	virtual bool isDense() const = 0;
	virtual bool hasField(rgl_field_t field) const = 0;
	virtual std::size_t getWidth() const = 0;
	virtual std::size_t getHeight() const = 0;
	virtual std::size_t getPointCount() const { return getWidth() * getHeight(); }

	virtual Mat3x4f getLookAtOriginTransform() const { return Mat3x4f::identity(); }

	// Data getters
	virtual VArray::ConstPtr getFieldData(rgl_field_t field, cudaStream_t stream) const = 0;
	virtual std::size_t getFieldPointSize(rgl_field_t field) const { return getFieldSize(field); }

	template<rgl_field_t field>
	typename VArrayProxy<typename Field<field>::type>::ConstPtr
	getFieldDataTyped(cudaStream_t stream)
	{ return getFieldData(field, stream)->template getTypedProxy<typename Field<field>::type>(); }
};

struct IPointsNodeSingleInput : IPointsNode
{
	using Ptr = std::shared_ptr<IPointsNodeSingleInput>;

	// Point cloud description
	bool isDense() const override { return input->isDense(); }
	size_t getWidth() const override { return input->getWidth(); }
	size_t getHeight() const override { return input->getHeight(); }

	Mat3x4f getLookAtOriginTransform() const override { return input->getLookAtOriginTransform(); }

	// Data getters
	bool hasField(rgl_field_t field) const { return input->hasField(field); }
	VArray::ConstPtr getFieldData(rgl_field_t field, cudaStream_t stream) const override
	{ return input->getFieldData(field, stream); }

protected:
	IPointsNode::Ptr input;
};

struct IPointsNodeMultiInput : IPointsNode
{
	using Ptr = std::shared_ptr<IPointsNodeMultiInput>;

	// Unable to calcuate origin from multiple inputs.
	Mat3x4f getLookAtOriginTransform() const override { return Mat3x4f::identity(); }

protected:
	std::vector<IPointsNode::Ptr> pointInputs;
};

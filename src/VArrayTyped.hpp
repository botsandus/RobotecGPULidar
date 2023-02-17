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
#include <VArray.hpp>
#include <cuda.h>
/*
 * TODO
 */
template <typename T>
struct VArrayTyped : public VArray, std::enable_shared_from_this<VArrayTyped<T>>
{
public:
    using Ptr = std::shared_ptr<VArrayTyped<T>>;
    using ConstPtr = std::shared_ptr<const VArrayTyped<T>>;

   // template <typename... Args>
    static VArrayTyped<T>::Ptr create(std::size_t initialSize)
    {
        return VArrayTyped<T>::Ptr(new VArrayTyped<T>(initialSize));
    }

    static VArrayTyped<T>::Ptr create(VArray::Ptr src = nullptr)
    {
        if (src != nullptr)
        {
            if (typeid(T) != src->typeInfo)
            {
                auto msg = fmt::format("VArray type mismatch: {} requested as {}", name(src->typeInfo), name(typeid(T)));
                throw std::invalid_argument(msg);
            }
            return std::reinterpret_pointer_cast<VArrayTyped<T>>(src);
        } else
        {
            return VArrayTyped<T>::Ptr(new VArrayTyped<T>(0));
        }
    }

    //TODO mrozikp Think how to merge it with non const method!
    static VArrayTyped<T>::ConstPtr create(VArray::ConstPtr src)
    {
        if (src != nullptr)
        {
            if (typeid(T) != src->typeInfo)
            {
                auto msg = fmt::format("VArray type mismatch: {} requested as {}", name(src->typeInfo), name(typeid(T)));
                throw std::invalid_argument(msg);
            }
            return std::reinterpret_pointer_cast<const VArrayTyped<T>>(src);
        } else
        {
            return VArrayTyped<T>::ConstPtr(new VArrayTyped<T>(0));
        }
    }

     VArray untyped()
     {
        return static_cast<VArray>(*this);
     };
    VArray untyped() const
    {
        return static_cast<const VArray>(*this);
        //return std::dynamic_pointer_cast<const VArray>(shared_from_this());
    };

    void getData(T* typedData, std::size_t count)
    {
        VArray::getData((void*)typedData, count);
        // TODO think how it should work rly
        //void* rawData;
        //VArray::getData(rawData, count);
        //typedData = std::dynamic_pointer_cast<T>(rawData);
    }

    void setData(const T* rawData, std::size_t count)
    {
        // TODO mrozikp check type before internal setdata
        VArray::setData(rawData, count);
    }

    std::size_t getCount() const { return VArray::getElemCount(); }
    std::size_t getBytesInUse() const { return VArray::getElemCount() * sizeof(T); }

    T* getWritePtr(MemLoc location) { return reinterpret_cast<T*>(VArray::getWritePtr(location)); }
    const T* getReadPtr(MemLoc location) const { return reinterpret_cast<const T*>(VArray::getReadPtr(location)); }

    CUdeviceptr getCUdeviceptr() { return reinterpret_cast<CUdeviceptr>(VArray::getWritePtr(MemLoc::Device)); }

    // TODO(prybicki): remove these in favor of ...(location)
    T* getHostPtr() { return dynamic_cast<T*>(VArray::getWritePtr(MemLoc::Host)); }
    const T* getHostPtr() const { return dynamic_cast<const T*>(VArray::getReadPtr(MemLoc::Device)); }

    T* getDevicePtr() { return reinterpret_cast<T*>(VArray::getWritePtr(MemLoc::Device)); }
    const T* getDevicePtr() const { return reinterpret_cast<const T*>(VArray::getReadPtr(MemLoc::Device)); }

    T& operator[](int idx) { return (reinterpret_cast<T*>(VArray::getWritePtr(MemLoc::Host)))[idx]; }
    const T& operator[](int idx) const { return (reinterpret_cast<const T*>(VArray::getReadPtr(MemLoc::Host)))[idx]; }

private:
    VArrayTyped(std::size_t initialSize)
        : VArray(typeid(T), sizeof(T), initialSize)
    {
    }

//    VArrayTyped(const std::type_info& type, std::size_t sizeOfType, std::size_t initialSize)
//        : VArray(type, sizeOfType, initialSize)
//    {
//    }
};
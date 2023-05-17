#include <tuple>
#include <vector>
#include <functional>
#include <random>
#include <iostream>

template<typename... Fields>
struct Point {
    std::tuple<Fields...> fields;

    Point(Fields... fields) : fields(std::make_tuple(fields...)) {}
};

template<typename... Fields>    
class PointGenerator {
public:
    using PointType = Point<Fields...>;

    template<typename... Fillers>
    static std::vector<PointType> generatePoints(size_t count, Fillers... fillers) {
        std::vector<PointType> points;
        points.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            points.emplace_back(fillField<Fields>(fillers, i)...);
        }

        return points;
    }

private:
    template<typename Field, typename Filler>
    static Field fillField(Filler filler, size_t i) {
        Field field;
        filler(field, i);
        return field;
    }
};

struct RGLPointsHelper{

    using xyzType = Field<XYZ_F32>::type;
    using isHitType = Field<IS_HIT_I32>::type;
    using intensityType = Field<INTENSITY_F32>::type;

    std::unordered_map<std::type_index, std::function<void(void*, size_t)>> fillers;

    RGLPointsHelper() {
        setXYZFiller([](xyzType &xyz, size_t i) { xyz = {0.0f, 0.0f, 0.0f}; });
        setIsHitFiller([](isHitType &isHit, size_t i) { isHit = 1; });
        setIntensityFiller([](intensityType &intensity, size_t i) { intensity = 0.1f; });
    }

    void setXYZFiller(std::function<void(xyzType&, size_t)> filler) {
        fillers[typeid(xyzType)] = [filler](void* field, size_t i) { 
            filler(*static_cast<xyzType*>(field), i); 
        };
    }

    void setIsHitFiller(std::function<void(isHitType&, size_t)> filler) {
        fillers[typeid(isHitType)] = [filler](void* field, size_t i) { 
            filler(*static_cast<isHitType*>(field), i); 
        };
    }

    void setIntensityFiller(std::function<void(intensityType&, size_t)> filler) {
        fillers[typeid(intensityType)] = [filler](void* field, size_t i) { 
            filler(*static_cast<intensityType*>(field), i); 
        };
    }

    template<typename... Fields>
    auto generatePoints(size_t count) {
        auto points = PointGenerator<Fields...>::generatePoints(
            count, 
            [this](Fields& field, size_t i) { fillers[typeid(Fields)](&field, i); }...);

        return points;
    }
};

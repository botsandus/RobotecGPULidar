#include <vector>
#include <numeric>

struct RGLRingIdsTestHelper {

    static std::vector<int> generateRingIds(int numRingIds)
    {
        std::vector<int> points(numRingIds);
        std::iota(points.begin(), points.end(), 0);
        return points;
    }

};
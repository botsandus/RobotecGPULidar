#pragma once

#include <type_traits>
#include <numeric>
#include <iterator>
#include <array>
#include <cmath>

#include <macros/cuda.hpp>
#include <macros/iteration.hpp>

template<int dim, typename T>
struct Vector
{
	using V = Vector<dim, T>;
	// static_assert(dim >= 2); // TODO: Is dim == 1 OK?
	// *** *** *** CONSTRUCTORS *** *** *** //

	// Zero constructor
	HD Vector() : Vector(static_cast<T>(0)) {}

	// Uniform constructor
	HD Vector(T scalar) {
		for (auto&& v : row) {
			v = scalar;
		}
	}

	Vector(const std::array<T, dim>& values)
	{ std::copy(values.begin(), values.end(), row); }

	// List constructor
	template<typename... Args>
	HD Vector(Args... args) : row{static_cast<T>(args)...}
	{ static_assert(sizeof...(Args) == dim); }

	// Type cast constructor
	template<typename U>
	HD Vector(const Vector<dim, U>& other) {
		for (int i = 0; i < dim; ++i) {
			row[i] = static_cast<T>(other.row[i]);
		}
	}

	// Dimension cast constructor
	template<int lowerDim>
	HD Vector(const Vector<lowerDim, T>& other, T fillValue)
	{
		static_assert(lowerDim < dim);
		for (int i = 0; i < lowerDim; ++i) {
			row[i] = other[i];
		}
		for (int i = lowerDim; i < dim; ++i) {
			row[i] = fillValue;
		}
	}

	// *** *** *** ACCESSORS *** *** *** //

	FORWARD_ITERATION(row, HD)
	FORWARD_INDEX_OPERATOR(row, HD)

#define NAMED_GETTER(name, index) \
HD T name() { static_assert(dim > index); return row[index]; } \
HD const T name() const { static_assert(dim > index); return row[index]; }

NAMED_GETTER(x, 0)
NAMED_GETTER(y, 1)
NAMED_GETTER(z, 2)
#undef NAMED_GETTER

// *** *** *** PIECEWISE OPERATORS (VECTOR + SCALAR) *** *** *** //

#define PIECEWISE_OPERATOR(OP, OPEQ)                                        \
HD V& operator OPEQ(const V& rhs) {                                         \
for (int i = 0; i < dim; ++i) {                                 \
row[i] OPEQ rhs[i];                                             \
}                                                                   \
return *this;                                                       \
}                                                                           \
HD friend V operator OP(V lhs, const V& rhs) { 	lhs OPEQ rhs; return lhs; } \
// HD V& operator OPEQ(T rhs) { return (*this) OPEQ V {rhs}; }                 \
	// HD friend V operator OP(T lhs, V rhs) {	rhs OPEQ lhs; return rhs; }         \
	// TODO: scalar operators above are a bit questionable.

	PIECEWISE_OPERATOR(+, +=)
	PIECEWISE_OPERATOR(-, -=)
	PIECEWISE_OPERATOR(*, *=)
	PIECEWISE_OPERATOR(/, /=)
#undef PIECEWISE_OPERATOR

HD T lengthSquared() const {
		auto sum = static_cast<T>(0);
		for (int i = 0; i < dim; ++i) {
			sum += row[i] * row[i];
		}
		return sum;
	}
	HD T length() const { return std::sqrt(lengthSquared()); }

	HD V half() const { return *this / V {static_cast<T>(2)}; }

	HD T min() const {
		T value = std::numeric_limits<T>::max();
		for (int i = 0; i < dim; ++i) {
			value = row[i] < value ? row[i] : value;
		}
		return value;
	}

	HD T product() const {
		T value = static_cast<T>(1);
		for (auto&& v : *this) {
			value *= v;
		}
		return value;
	}

private:
	T row[dim];
};

#ifndef __CUDACC__
#include <spdlog/fmt/fmt.h>
template<int dim, typename T>
struct fmt::formatter<Vector<dim, T>>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& ctx) {
		return ctx.begin();
	}

	template<typename FormatContext>
	auto format(Vector<dim, T> const& vec, FormatContext& ctx) {

		fmt::format_to(ctx.out(), "(");
		for (int i = 0; i < dim; ++i) {
			fmt::format_to(ctx.out(), "{}", vec[i]);
			if (i < dim - 1) {
				fmt::format_to(ctx.out(), ", ");
			}
		}
		return fmt::format_to(ctx.out(), ")");
	}
};
#endif // __CUDACC__

using Vec2f = Vector<2, float>;
using Vec3f = Vector<3, float>;
using Vec4f = Vector<4, float>;

using Vec2i = Vector<2, int>;
using Vec3i = Vector<3, int>;
using Vec4i = Vector<4, int>;

static_assert(std::is_trivially_copyable<Vec2f>::value);
static_assert(std::is_trivially_copyable<Vec3f>::value);
static_assert(std::is_trivially_copyable<Vec4f>::value);

static_assert(std::is_trivially_copyable<Vec2i>::value);
static_assert(std::is_trivially_copyable<Vec3i>::value);
static_assert(std::is_trivially_copyable<Vec4i>::value);
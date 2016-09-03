#include <algorithm>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "lodepng.h"
#include "nanoflann.hpp"

// point cloud for nanoflann's kd-tree, for fast nearest neighbour approximation
template <typename T>
struct PointCloud {
	struct Point {
		T x, y;
		unsigned char r, g, b, a;
	};
	std::vector<Point> points;
	inline size_t kdtree_get_point_count() const {
		return points.size();
	}
	inline T kdtree_distance(const T * p1, const size_t p2index, size_t) const {
		const T dx = p1[0] - points[p2index].x;
		const T dy = p1[1] - points[p2index].y;
		return dx*dx + dy*dy;
	}
	inline T kdtree_get_pt(const size_t index, int dimension) const {
		switch (dimension) {
		case 0:
			return points[index].x;
			break;
		case 1:
		default:
			return points[index].y;
		}
	}
	template <class BBOX>
	bool kdtree_get_bbox(BBOX & bb) const {
		return false;
	}
};

int main(int argc, char * argv[]) {

	if (argc < 9) {
		std::cerr << "Too few arguments. Usage: warp7p <input.png> <k1> <k2> <p1> <p2> <k3> <cx> <cy>" << std::endl;
		return -1;
	}

	auto pixels = std::vector<unsigned char>{};
	auto width = 0u;
	auto height = 0u;
	auto error = 0u;
	if (std::string(argv[1]) == "-") {
		std::cerr << "Reading from standard input is not supported." << std::endl;
		return -1;
	} else {
		error = lodepng::decode(pixels, width, height, argv[1]);
	}
	if (error) {
		std::cerr << "Error decoding PNG at \"" << argv[1] << "\" - " << error << " (" << lodepng_error_text(error) << ")." << std::endl;
		return -1;
	}

	// make the destination image twice the size as source image for extra space
	auto destPixels = std::vector<unsigned char>(width * 2 * height * 2 * 4);
	auto dWidth  = static_cast<double>(width);
	auto dHeight = static_cast<double>(height);

	double k1, k2, p1, p2, k3, cx, cy;
	try {
		k1 = std::stod(argv[2]);
		k2 = std::stod(argv[3]);
		p1 = std::stod(argv[4]);
		p2 = std::stod(argv[5]);
		k3 = std::stod(argv[6]);
		cx = std::stod(argv[7]);
		cy = std::stod(argv[8]);
	} catch (...) {
		std::cerr << "Invalid warp argument provided." << std::endl;
		return -1;
	}

	using pixel_index_t = decltype(pixels)::size_type;

	auto destPoints = PointCloud<double>{};
	destPoints.points.reserve(width * 2 * height * 2);

	for (pixel_index_t y = 0; y < height; ++y) {
		for (pixel_index_t x = 0; x < width; ++x) {
			// scale coordinates down to [0.0, 1.0) and offset by (cx, cy)
			auto xp = x / dWidth - cx;
			auto yp = y / dHeight - cy;
			// r^2 parameter (distance from centre squared)
			auto r2 = xp * xp + yp * yp;
			// trapezoidal distortion parameters
			auto Tx = 2*p1*xp*yp + p2 * (r2 + 2*xp*xp);
			auto Ty = p1 * (r2 + 2*yp*yp) + 2*p2*xp*yp;
			// radial distortion coefficient
			auto radialCoefficient = 1 + r2*k1 + r2*r2*k2 + r2*r2*r2*k3;
			// coordinates after applying radial and trapezoidal effects
			auto x_c = xp * radialCoefficient + Tx;
			auto y_c = yp * radialCoefficient + Ty;
			// convert back to actual pixel coordinates with bounds checking
			if (x_c < -1.0 || y_c < -1.0) {
				continue;
			}
			auto x_dest = static_cast<pixel_index_t>((x_c + 1.0) * width);
			auto y_dest = static_cast<pixel_index_t>((y_c + 1.0) * height);
			if (x_dest > width * 2 || y_dest > height * 2) {
				continue;
			}
			// add new adjusted point to point cloud
			auto iSrc = (x + y * width) * 4;
			auto newPoint = decltype(destPoints)::Point{};
			newPoint.x = x_dest;
			newPoint.y = y_dest;
			newPoint.r = pixels[iSrc + 0];
			newPoint.g = pixels[iSrc + 1];
			newPoint.b = pixels[iSrc + 2];
			newPoint.a = pixels[iSrc + 3];
			destPoints.points.emplace_back(newPoint);
		}
	}

	// use nanoflann to create a kd-tree for approximate nearest neighbour interpolation on point cloud
	using my_kd_tree_t =
		nanoflann::KDTreeSingleIndexAdaptor<
			nanoflann::L2_Simple_Adaptor<double, PointCloud<double>>,
			PointCloud<double>,
			2
		>;

	my_kd_tree_t index{ 2, destPoints, nanoflann::KDTreeSingleIndexAdaptorParams(10) };
	index.buildIndex();

	for (pixel_index_t y = 0; y < height * 2; ++y) {
		for (pixel_index_t x = 0; x < width * 2; ++x) {
			const auto iDest = (x + y * width * 2) * 4;
			size_t retIndex;
			double distanceSquared;
			const double queryPoint[2] = { static_cast<double>(x), static_cast<double>(y) };
			index.knnSearch(&queryPoint[0], 1, &retIndex, &distanceSquared);
			destPixels[iDest + 0] = destPoints.points[retIndex].r;
			destPixels[iDest + 1] = destPoints.points[retIndex].g;
			destPixels[iDest + 2] = destPoints.points[retIndex].b;
			destPixels[iDest + 3] = destPoints.points[retIndex].a;
		}
	}

	// save the result png
	auto png = std::vector<unsigned char>{};
	error = lodepng::encode(png, destPixels, width * 2, height * 2);
	if (error) {
		std::cerr << "Error encoding result PNG - " << error << " (" << lodepng_error_text(error) << ")." << std::endl;
		return -1;
	} else {
		lodepng::save_file(png, "out.png");
	}

	return 0;

}

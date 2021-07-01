#include <algorithm>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include <lodepng/lodepng.h>
#include "nanoflann.hpp"

// point cloud for nanoflann's kd-tree, for fast nearest neighbour approximation
template <typename T>
struct PointCloud
{
    struct Point
    {
        T x, y;
        unsigned char r, g, b, a;
    };
    std::vector<Point> points;
    inline size_t kdtree_get_point_count() const
    {
        return points.size();
    }
    inline T kdtree_distance(const T * p1, const size_t p2index, size_t) const
    {
        const T dx = p1[0] - points[p2index].x;
        const T dy = p1[1] - points[p2index].y;
        return dx * dx + dy * dy;
    }
    inline T kdtree_get_pt(const size_t index, int dimension) const
    {
        switch (dimension)
        {
        case 0:
            return points[index].x;
            break;
        case 1:
        default:
            return points[index].y;
        }
    }
    template <class BBOX>
    bool kdtree_get_bbox(BBOX & bb) const
    {
        return false;
    }
};

int main(int argc, char * argv[])
{
    unsigned int width = 0u, height = 0u;
    unsigned int width2, height2;
    unsigned int xline, iSrc;
    float xp, yp, xp2, yp2, xyp, r2, Tx, Ty, radialCoefficient;
    float x_c, y_c, mcx, mcy;

    if (argc < 3)
    {
        std::cerr << "Too few arguments. Usage: " << argv[0] << " <input.png> <output.png> [k1=0.0] [k2=0.0] [p1=0.0] [p2=0.0] [k3=0.0] [cx=0.5] [cy=0.5] [mc=1.0]" << std::endl;
        return -1;
    }

    auto pixels = std::vector<unsigned char> {};
    auto error = 0u;
    if (std::string(argv[1]) == "-")
    {
        std::cerr << "Reading from standard input is not supported." << std::endl;
        return -1;
    }
    else
    {
        error = lodepng::decode(pixels, width, height, argv[1]);
    }
    if (error)
    {
        std::cerr << "Error decoding PNG at \"" << argv[1] << "\" - " << error << " (" << lodepng_error_text(error) << ")." << std::endl;
        return -1;
    }

    // make the destination image twice the size as source image for extra space
    float k1, k2, p1, p2, k3, cx, cy, mc;
    try
    {
        k1 = (argc < 4) ? 0.0f : std::stod(argv[3]);
        k2 = (argc < 5) ? 0.0f : std::stod(argv[4]);
        p1 = (argc < 6) ? 0.0f : std::stod(argv[5]);
        p2 = (argc < 7) ? 0.0f : std::stod(argv[6]);
        k3 = (argc < 8) ? 0.0f : std::stod(argv[7]);
        cx = (argc < 9) ? 0.5f : std::stod(argv[8]);
        cy = (argc < 10) ? 0.5f : std::stod(argv[9]);
        mc = (argc < 11) ? 1.0f : std::stod(argv[10]);
    }
    catch (...)
    {
        std::cerr << "Invalid warp argument provided." << std::endl;
        return -1;
    }

    width2 = width * mc;
    height2 = height * mc;
    mcx = cx * mc;
    mcy = cy * mc;
    auto destPixels = std::vector<unsigned char>(width2 * height2 * 4);
    auto dWidth  = static_cast<double>(width);
    auto dHeight = static_cast<double>(height);

    using pixel_index_t = decltype(pixels)::size_type;

    auto destPoints = PointCloud<double> {};
    destPoints.points.reserve(width2 * height2);
    
    pixel_index_t y, x;

    for (y = 0; y < height; y++)
    {
        xline = y * width;
        for (x = 0; x < width; x++)
        {
            // scale coordinates down to [0.0, 1.0) and offset by (cx, cy)
            xp = x / dWidth - cx;
            yp = y / dHeight - cy;
            xp2 = xp * xp;
            yp2 = yp * yp;
            xyp = xp * yp;
            // r^2 parameter (distance from centre squared)
            r2 = xp2 + yp2;
            // trapezoidal distortion parameters
            Tx = 2.0f * p1 * xyp + p2 * (r2 + xp2 + xp2);
            Ty = p1 * (r2 + yp2 + yp2) + 2.0f * p2 * xyp;
            // radial distortion coefficient
            radialCoefficient = 1.0f + r2 * (k1 + r2 * (k2 + r2 * k3));
            // coordinates after applying radial and trapezoidal effects
            x_c = xp * radialCoefficient + Tx;
            y_c = yp * radialCoefficient + Ty;
            // convert back to actual pixel coordinates with bounds checking
            x_c += mcx;
            y_c += mcy;
            if (x_c < 0.0f || y_c < 0.0f)
                continue;
            auto x_dest = static_cast<pixel_index_t>(x_c * width);
            auto y_dest = static_cast<pixel_index_t>(y_c * height);
            if (x_dest > width2 || y_dest > height2)
                continue;
            // add new adjusted point to point cloud
            iSrc = (x + xline) * 4;
            auto newPoint = decltype(destPoints)::Point {};
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

    my_kd_tree_t index { 2, destPoints, nanoflann::KDTreeSingleIndexAdaptorParams(10) };
    index.buildIndex();

    for (y = 0; y < height2; y++)
    {
        xline = y * width2;
        for (x = 0; x < width2; x++)
        {
            const unsigned int iDest = (x + xline) * 4;
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
    auto png = std::vector<unsigned char> {};
    error = lodepng::encode(png, destPixels, width2, height2);
    if (error)
    {
        std::cerr << "Error encoding result PNG - " << error << " (" << lodepng_error_text(error) << ")." << std::endl;
        return -1;
    }
    else
    {
        lodepng::save_file(png, argv[2]);
    }

    return 0;

}

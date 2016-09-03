warp7p
======

Simple utility to apply 7 parameter distortion (k1, k2, p1, p2, k3, cx, cy) to an image as per the equations [here](http://docs.opencv.org/2.4/doc/tutorials/calib3d/camera_calibration/camera_calibration.html). Made for the sake of learning and to demonstrate the effects of the individual parameters for a university project on camera calibration. Uses nearest neighbour interpolation.

Requires that `lodepng.h` and `lodepng.cpp` from [LodePNG](http://lodev.org/lodepng/) (for PNG input/output) and `nanoflann.hpp` from [nanoflann](https://github.com/jlblancoc/nanoflann) (for fast approximate nearest neighbour interpolation with a KD-tree) are present in the source directory, alongside `main.cpp`.

Usage: `warp7p <input.png> <k1> <k2> <p1> <p2> <k3> <cx> <cy>`

License: GPLv2
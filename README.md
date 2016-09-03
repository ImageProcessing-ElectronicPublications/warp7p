warp7p
======

Simple utility to apply 7 parameter distortion (k1, k2, p1, p2, k3, cx, cy) to an image as per the equations [here](http://docs.opencv.org/2.4/doc/tutorials/calib3d/camera_calibration/camera_calibration.html). Made for the sake of learning and to demonstrate the effects of the individual parameters for a university project on camera calibration. Uses nearest neighbour interpolation.

Requires that `lodepng.h` and `lodepng.cpp` from [LodePNG](http://lodev.org/lodepng/) (for PNG input/output) and `nanoflann.hpp` from [nanoflann](https://github.com/jlblancoc/nanoflann) (for fast approximate nearest neighbour interpolation with a KD-tree) are present in the source directory, alongside `main.cpp`.

Usage: `warp7p <input.png> <k1> <k2> <p1> <p2> <k3> <cx> <cy>`

Example: `warp7p test.png -0.4 0 -0.05 0.1 0 0.5 0.5`  
Where test.png is:  
![test.png](https://cloud.githubusercontent.com/assets/6970423/18223433/7cf6f74e-71f6-11e6-8868-c0c6a2ba5f7f.png)  
Produces the resulting out.png:  
![out.png](https://cloud.githubusercontent.com/assets/6970423/18223434/84009176-71f6-11e6-8fd7-1df6da586fff.png)

The effects of individual parameters:

k1 (positive)  
![k1.png](https://cloud.githubusercontent.com/assets/6970423/18223448/f15a7728-71f6-11e6-8c19-5982f25a0ea5.png)  
k1 (negative)  
![k1n.png](https://cloud.githubusercontent.com/assets/6970423/18223451/fbc161a4-71f6-11e6-9be0-1c6465de5dc9.png)  
k2  
![k2.png](https://cloud.githubusercontent.com/assets/6970423/18223449/f45e4b02-71f6-11e6-9bfc-8932b34713f6.png)  
k3  
![k3.png](https://cloud.githubusercontent.com/assets/6970423/18223450/f864e30a-71f6-11e6-9eec-65a2b283d1fe.png)  
p1  
![p1.png](https://cloud.githubusercontent.com/assets/6970423/18223452/fda37296-71f6-11e6-953a-434b19d029d4.png)  
p2  
![p2.png](https://cloud.githubusercontent.com/assets/6970423/18223453/fed96cec-71f6-11e6-9889-4f230427e2eb.png)  

License: GPLv2

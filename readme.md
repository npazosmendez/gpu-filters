# Filters

**Edge Detection** with Canny's algorithm

**Line Detection** with Hough's transform

**Region Filling** via exemplar-based image inpainting

**Dense Optical Flow** with Lucas-Kanade


# Dependencies
* ### OpenCL 1.2 with C++ bindings
Filters are implemented using OpenCL 1.2. To download needed headers and libraries (linux) for compilation run:

```shell
sudo apt install ocl-icd-libopencl1
sudo apt install opencl-headers
sudo apt install clinfo
```

We've also used the C++ bindings, which are included in the repo.

A device that supports OpenCL is needed to run the program. Drivers installation varies. These are some devices in which the program ran and how to install their drivers:

**NVIDIA GeForce 940m**

```shell
sudo add-apt-repository ppa:graphics-drivers/ppa
sudo apt-get update
sudo apt-get install nvidia-355 nvidia-prime
sudo reboot
```


* ### OpenCV 2
Miscellaneous. For I/O purposes mainly. **Central algorithms are not implemented using this library.**

To get eveything needed to compile run:

```shell
sudo apt-get install libopencv-dev
```

# What is left
- Better line drawing algorithm
- Lucas-Kanade C version
- Lucas-Kanade OpenCL version
- Hough line-detection OpenCL version
- Inpainting OpenCL version
- Handle boundaries (image borders) in all algorithms
- Optimize all OpenCL code
- Test / validate current implementations
- Create a friendly UI

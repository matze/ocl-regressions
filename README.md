## General information

This small utility checks if an installed OpenCL platform is working according
to the specifications in a multi GPU environment.

Right now it checks if
- platforms and devices can be initialized
- programs can be build
- kernels can be created from these programs

The latter is currently _not_ working with NVIDIA driver 302.06.03 and a multi
GPU setup with a GTX 680 and a GTX 590 in the same OpenCL context.


### Building

An OpenCL installation should be present, if so you can build with

    $ ./waf configure
    $ ./waf

and run the suite over a range of GPUs with

    $ ./build/check --first=0 --last=4

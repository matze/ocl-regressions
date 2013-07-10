## General information

This small utility checks if an installed OpenCL platform is working according
to the specifications in a multi GPU environment.

Right now it checks if

- platforms and devices can be initialized
- programs can be built
- programs can be built for different devices
- kernels can be created and started from these programs

The latter is currently _not_ working with NVIDIA driver 302.06.03 and a multi
GPU setup with a GTX 680 and a GTX 590 in the same OpenCL context.


### Building

You must have GLib 2.0 installed (package libglib2.0-dev on Debian-based
systems) and one of the many OpenCL SDKs. If the requirements are satisfied you
can build with

    $ make

and run the suite over a range of GPUs with

    $ ./check --first=0 --last=4

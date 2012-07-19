#ifndef OCL_H
#define OCL_H

#include <CL/cl.h>
#include <glib.h>

#define OCL_ALL_DEVICES     0xFFFF

typedef struct _OCL {
    cl_context           context;
    cl_uint              num_all_devices;
    cl_device_id        *all_devices;
    cl_uint              num_devices;
    cl_device_id        *devices;
    cl_command_queue    *cmd_queues;
} OCL;

OCL         *ocl_new    (guint           device_from,
                         guint           device_to,
                         int            *ocl_errcode);
void         ocl_free   (OCL            *ocl);
cl_program   ocl_program_new_from_source 
                        (OCL *ocl, 
                         const gchar    *source, 
                         const gchar    *options,
                         int            *ocl_errcode);
cl_program   ocl_program_new_from_file
                        (OCL            *ocl, 
                         const gchar    *filename, 
                         const gchar    *options,
                         int            *ocl_errcode);
const gchar *ocl_error  (int             ocl_errcode);

#endif

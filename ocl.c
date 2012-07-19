#include <stdio.h>
#include "ocl.h"

#define check_and_return_ocl_errcode(code) \
    if (code != CL_SUCCESS) { g_warning ("OpenCL error at %s:%i\n", __FILE__, __LINE__); \
        *ocl_errcode = code; return NULL; }

static const gchar *error_msgs[];

const gchar *
ocl_error (int ocl_errcode)
{
    if (ocl_errcode >= -14)
        return error_msgs[-ocl_errcode];

    if (ocl_errcode <= -30)
        return error_msgs[-ocl_errcode-15];

    return NULL;
}

static gchar *
ocl_read_program (const gchar *filename)
{
    FILE *fp = fopen (filename, "r");
    if (fp == NULL)
        return NULL;

    fseek (fp, 0, SEEK_END);
    const size_t length = ftell (fp);
    rewind (fp);

    gchar *buffer = (gchar *) g_malloc0(length+1);
    if (buffer == NULL) {
        fclose (fp);
        return NULL;
    }

    size_t buffer_length = fread (buffer, 1, length, fp);
    fclose (fp);
    if (buffer_length != length) {
        g_free (buffer);
        return NULL;
    }

    return buffer;
}

cl_program
ocl_program_new_from_file (OCL *ocl, const gchar *filename, const gchar *options, int *ocl_errcode)
{
    cl_program program;
    gchar *buffer;

    g_return_val_if_fail (ocl != NULL && filename != NULL && ocl_errcode != NULL, NULL);
    
    buffer = ocl_read_program (filename);

    if (buffer == NULL) 
        return NULL;

    program = ocl_program_new_from_source (ocl, buffer, options, ocl_errcode);
    g_free (buffer);
    return program;
}

cl_program
ocl_program_new_from_source (OCL *ocl, const gchar *source, const gchar *options, int *ocl_errcode)
{
    cl_program program;
    int errcode;

    g_return_val_if_fail (ocl != NULL && source != NULL && ocl_errcode != NULL, NULL);

    program = clCreateProgramWithSource (ocl->context, 1, (const char **) &source, NULL, &errcode);
    check_and_return_ocl_errcode (errcode);
    errcode = clBuildProgram (program, ocl->num_devices, ocl->devices, options, NULL, NULL);

    if (errcode != CL_SUCCESS) {
        const int LOG_SIZE = 4096;
        gchar* log = (gchar *) g_malloc0(LOG_SIZE * sizeof (char));

        clGetProgramBuildInfo (program, ocl->devices[0], CL_PROGRAM_BUILD_LOG, LOG_SIZE, (void*) log, NULL);
        g_print ("\n=== Build log for ===%s\n\n", log);
        g_free (log);
        return NULL;
    }

    return program;
}

static cl_platform_id
get_platform (gchar *prefered_platform)
{
    gchar result[256];
    cl_platform_id *platforms = NULL;
    cl_uint num_platforms = 0;
    cl_platform_id platform = NULL;

    cl_int errcode = clGetPlatformIDs (0, NULL, &num_platforms);
    if (errcode != CL_SUCCESS)
        return NULL;

    platforms = (cl_platform_id *) g_malloc0(num_platforms * sizeof (cl_platform_id));
    errcode = clGetPlatformIDs (num_platforms, platforms, NULL);

    if (errcode == CL_SUCCESS)
        platform = platforms[0];

    for (int i = 0; i < num_platforms; i++) {
        clGetPlatformInfo (platforms[i], CL_PLATFORM_VENDOR, 256, result, NULL);
        if (g_strstr_len (result, -1, prefered_platform) != NULL) {
            platform = platforms[i];
            break; 
        }
    }

    g_free (platforms);
    return platform;
}

OCL *
ocl_new (guint from, guint to, int *ocl_errcode)
{
    OCL *ocl;
    int errcode;
    const size_t len = 256;
    char string_buffer[len];

    g_return_val_if_fail (ocl_errcode != NULL, NULL);
    g_return_val_if_fail (from <= to, NULL);

    ocl = g_malloc0(sizeof (OCL));
    cl_platform_id platform = get_platform ("");

    if (platform == NULL)
        return NULL;

    /* Get all device types */
    errcode = clGetDeviceIDs (platform, CL_DEVICE_TYPE_ALL, 0, NULL, &ocl->num_all_devices);
    check_and_return_ocl_errcode (errcode);

    ocl->all_devices = g_malloc0 (ocl->num_all_devices * sizeof (cl_device_id));
    errcode = clGetDeviceIDs (platform, CL_DEVICE_TYPE_ALL, ocl->num_all_devices, ocl->all_devices, NULL);
    check_and_return_ocl_errcode (errcode);

    /* Create context spanning devices within range from:to */
    to = MIN (to, ocl->num_all_devices - 1);
    ocl->num_devices = to - from + 1;
    ocl->devices = g_malloc0 (ocl->num_devices * sizeof (cl_device_id));

    for (guint i = from; i < to + 1; i++)
        ocl->devices[i - from] = ocl->all_devices[i];

    ocl->context = clCreateContext (NULL, ocl->num_devices, ocl->devices, NULL, NULL, &errcode);
    check_and_return_ocl_errcode (errcode);
    ocl->cmd_queues = g_malloc0(ocl->num_devices * sizeof (cl_command_queue));
    cl_command_queue_properties queue_properties = CL_QUEUE_PROFILING_ENABLE;

    /* Print platform info */
    errcode = clGetPlatformInfo (platform, CL_PLATFORM_VERSION, len, string_buffer, NULL);
    check_and_return_ocl_errcode (errcode);
    g_print ("# Platform: %s\n", string_buffer);

    for (int i = 0; i < ocl->num_devices; i++) {
        clGetDeviceInfo (ocl->devices[i], CL_DEVICE_NAME, len, string_buffer, NULL);
        g_print ("# Device %i: %s\n", i, string_buffer);

        ocl->cmd_queues[i] = clCreateCommandQueue (ocl->context, ocl->devices[i], queue_properties, &errcode);
        check_and_return_ocl_errcode (errcode);
    }

    return ocl;
}

void 
ocl_free (OCL *ocl)
{
    for (int i = 0; i < ocl->num_devices; i++)
        clReleaseCommandQueue (ocl->cmd_queues[i]);

    clReleaseContext (ocl->context);

    g_free (ocl->all_devices);
    g_free (ocl->devices);
    g_free (ocl->cmd_queues);
    g_free (ocl);
}

static const gchar *error_msgs[] = {
    "CL_SUCCESS",
    "CL_DEVICE_NOT_FOUND",
    "CL_DEVICE_NOT_AVAILABLE",
    "CL_COMPILER_NOT_AVAILABLE",
    "CL_MEM_OBJECT_ALLOCATION_FAILURE",
    "CL_OUT_OF_RESOURCES",
    "CL_OUT_OF_HOST_MEMORY",
    "CL_PROFILING_INFO_NOT_AVAILABLE",
    "CL_MEM_COPY_OVERLAP",
    "CL_IMAGE_FORMAT_MISMATCH",
    "CL_IMAGE_FORMAT_NOT_SUPPORTED",
    "CL_BUILD_PROGRAM_FAILURE",
    "CL_MAP_FAILURE",
    "CL_MISALIGNED_SUB_BUFFER_OFFSET",
    "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST",

    /* next IDs start at 30! */
    "CL_INVALID_VALUE",
    "CL_INVALID_DEVICE_TYPE",
    "CL_INVALID_PLATFORM",
    "CL_INVALID_DEVICE",
    "CL_INVALID_CONTEXT",
    "CL_INVALID_QUEUE_PROPERTIES",
    "CL_INVALID_COMMAND_QUEUE",
    "CL_INVALID_HOST_PTR",
    "CL_INVALID_MEM_OBJECT",
    "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
    "CL_INVALID_IMAGE_SIZE",
    "CL_INVALID_SAMPLER",
    "CL_INVALID_BINARY",
    "CL_INVALID_BUILD_OPTIONS",
    "CL_INVALID_PROGRAM",
    "CL_INVALID_PROGRAM_EXECUTABLE",
    "CL_INVALID_KERNEL_NAME",
    "CL_INVALID_KERNEL_DEFINITION",
    "CL_INVALID_KERNEL",
    "CL_INVALID_ARG_INDEX",
    "CL_INVALID_ARG_VALUE",
    "CL_INVALID_ARG_SIZE",
    "CL_INVALID_KERNEL_ARGS",
    "CL_INVALID_WORK_DIMENSION",
    "CL_INVALID_WORK_GROUP_SIZE",
    "CL_INVALID_WORK_ITEM_SIZE",
    "CL_INVALID_GLOBAL_OFFSET",
    "CL_INVALID_EVENT_WAIT_LIST",
    "CL_INVALID_EVENT",
    "CL_INVALID_OPERATION",
    "CL_INVALID_GL_OBJECT",
    "CL_INVALID_BUFFER_SIZE",
    "CL_INVALID_MIP_LEVEL",
    "CL_INVALID_GLOBAL_WORK_SIZE"
};


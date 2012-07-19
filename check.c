
#include <stdarg.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "ocl.h"

static void
check_kernel (cl_program program, const gchar *name, int *errcode)
{
    cl_kernel kernel;

    kernel = clCreateKernel (program, name, errcode);
    
    if (kernel != NULL)
        clReleaseKernel (kernel);  
}

static void
print_check (const gchar *format, int errcode, ...)
{
    va_list args;

    va_start (args, errcode);
    g_vprintf (format, args);
    va_end (args);

    if (errcode == CL_SUCCESS) {
        g_print (": OK\n");
    }
    else
        g_print (": Error: %s\n", ocl_error (errcode));
}

int main(int argc, char *argv[])
{
    OCL *ocl;
    GOptionContext *context;
    GError         *error   = NULL;
    int             errcode = CL_SUCCESS;
    cl_program      program;

    static gint first_device = 0;
    static gint last_device  = OCL_ALL_DEVICES;

    static GOptionEntry entries[] =
    {
        { "first", 'f', 0, G_OPTION_ARG_INT, &first_device, "First device to use", "N" },
        { "last",  'l', 0, G_OPTION_ARG_INT, &last_device,  "Last device to use",  "M" },
        { NULL }
    };

    static const gchar *kernels[] =
    {
        "assign",
        "two_const_params",
        "three_const_params",
        "four_const_params",
        NULL
    };

    context = g_option_context_new ("");
    g_option_context_add_main_entries (context, entries, NULL);

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("Option parsing failed: %s\n", error->message); 
        return 1;
    }
    
    ocl = ocl_new (first_device, last_device, &errcode);
    print_check ("Initialization", errcode);

    program = ocl_program_new_from_file (ocl, "programs.cl", "", &errcode);
    print_check ("Creating backproject program", errcode);

    for (guint i = 0; kernels[i] != NULL; i++) {
        check_kernel (program, kernels[i], &errcode);
        print_check ("Creating kernel `%s`", errcode, kernels[i]);
    }

    ocl_free (ocl);
    return 0;
}

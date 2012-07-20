
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

static GList *
read_kernel_names (const gchar *filename)
{
    GList   *names = NULL;
    FILE    *fp;
    gchar   *line;
    GRegex  *regex;
    GMatchInfo *match_info;

    regex = g_regex_new ("__kernel void ([_A-Za-z][_A-Za-z0-9]*)", 0, 0, NULL);
    line = g_malloc0 (1024);
    fp = fopen (filename, "r");

    while (fgets (line, 1024, fp) != NULL) {
        if (g_regex_match (regex, line, 0, &match_info)) {
            gchar *kernel_name = g_match_info_fetch (match_info, 1);
            names = g_list_append (names, kernel_name);
        }
    }

    fclose (fp);
    g_free (line);


    g_regex_unref (regex);
    return names;
}

int main(int argc, char *argv[])
{
    OCL *ocl;
    GOptionContext *context;
    GError         *error   = NULL;
    GList          *kernels;
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

    context = g_option_context_new ("");
    g_option_context_add_main_entries (context, entries, NULL);

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("Option parsing failed: %s\n", error->message); 
        return 1;
    }

    /* Check bug with mixed GPUs and more than two __constant parameters */
    kernels = read_kernel_names ("programs.cl");
    
    ocl = ocl_new (first_device, last_device, &errcode);
    print_check ("Initialization", errcode);

    program = ocl_program_new_from_file (ocl, "programs.cl", "", &errcode);
    print_check ("Creating `programs.cl`", errcode);

    for (GList *it = g_list_first (kernels); it != NULL; it = g_list_next (it)) {
        gchar *name = (gchar *) it->data;

        check_kernel (program, name, &errcode);
        print_check ("Creating kernel `%s`", errcode, name);
    }

    clReleaseProgram (program);

    /* Check that two different kernel programs can be built if the arguments
     * stay the same */
    if (ocl->num_devices > 1) {
        cl_kernel  kernel;

        static const char *source = "\
#ifdef FIRST\n \
   __kernel void foo(__global float *arg) {}\n\
   __kernel void bar(__global float *arg) {}\n\
#else\n \
   __kernel void foo(__global float *arg) {}\n\
   __kernel void bar(__constant float *arg) {}\n\
#endif"; 

        program = clCreateProgramWithSource (ocl->context, 1, (const char **) &source, NULL, &errcode);

        print_check ("Creating program `kernel-definition`", errcode);

        errcode = clBuildProgram (program, 1, &ocl->devices[0], "-D FIRST", NULL, NULL);
        print_check ("Build program `kernel-definition` for GPU 1", errcode);

        errcode = clBuildProgram (program, 1, &ocl->devices[1], "", NULL, NULL);
        print_check ("Build program `kernel-definition` for GPU 2", errcode);

        kernel = clCreateKernel (program, "foo", &errcode);
        print_check ("Created kernel `foo` with same signature", errcode);
        clReleaseKernel (kernel);

        kernel = clCreateKernel (program, "bar", &errcode);
        print_check ("Created kernel `bar` with different signature [expect CL_INVALID_KERNEL_DEFINITION]", errcode);
    }

    ocl_free (ocl);
    g_list_foreach (kernels, (GFunc) g_free, NULL);

    return 0;
}

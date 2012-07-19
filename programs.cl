__kernel void assign(__global float *input, __global float *output)
{
    const int idx = get_global_id (0); 
    input[idx] = output[idx]; 
}

__kernel void two_const_params(__constant float *c_param_1,
                               __constant float *c_param_2)
{
}

__kernel void three_const_params(__constant float *c_param_1,
                                 __constant float *c_param_2,
                                 __constant float *c_param_3)
{
}

__kernel void four_const_params(__constant float *c_param_1,
                                __constant float *c_param_2,
                                __constant float *c_param_3,
                                __constant float *c_param_4)
{
}

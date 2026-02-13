/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2012 Victor Oliveira (victormatheus@gmail.com)
 * Copyright 2026 Ondřej Míchal <harrymichal@seznam.cz>
 */

#ifndef __GEGL_CL_INIT_H__
#define __GEGL_CL_INIT_H__

#include "gegl-cl-types.h"
#include "CL/cl_function_types.h"

G_BEGIN_DECLS

const char *      gegl_cl_errstring(cl_int err);

gboolean          gegl_cl_init (void);

void              gegl_cl_cleanup (void);

gboolean          gegl_cl_is_accelerated (void);

gboolean          gegl_cl_has_gl_sharing (void);

void              gegl_cl_disable (void);

void              gegl_cl_hard_disable (void);

cl_platform_id    gegl_cl_get_platform (void);

cl_device_id      gegl_cl_get_device (void);

cl_context        gegl_cl_get_context (void);

cl_command_queue  gegl_cl_get_command_queue (void);

cl_ulong          gegl_cl_get_local_mem_size (void);

size_t            gegl_cl_get_iter_width (void);

size_t            gegl_cl_get_iter_height (void);

void              gegl_cl_set_profiling (gboolean enable);

void              gegl_cl_set_default_device_type (cl_device_type default_device_type);

gboolean          gegl_cl_has_extension (const char *extension_name);

typedef struct
{
  cl_program  program;
  cl_kernel  *kernel;
  size_t     *work_group_size;
} GeglClRunData;

GeglClRunData *   gegl_cl_compile_and_build (const char *program_source,
                                             const char *kernel_name[]);

extern gboolean _gegl_cl_is_accelerated;

#define gegl_cl_is_accelerated()   _gegl_cl_is_accelerated

#define GEGL_CL_CHUNK_SIZE 1024 * 1024

#ifdef __GEGL_CL_INIT_MAIN__

#define _GEGL_CL_WRAP_FUNCTION(cl_func) cl_func##_fn gegl_##cl_func = NULL;

#else

#define _GEGL_CL_WRAP_FUNCTION(cl_func) extern cl_func##_fn gegl_##cl_func;

#endif

/* Query Platform Info */
_GEGL_CL_WRAP_FUNCTION (clGetPlatformIDs);
_GEGL_CL_WRAP_FUNCTION (clGetPlatformInfo);

/* Query Devices */
_GEGL_CL_WRAP_FUNCTION (clGetDeviceIDs);
_GEGL_CL_WRAP_FUNCTION (clGetDeviceInfo);

/* Partition a Device */
_GEGL_CL_WRAP_FUNCTION (clCreateSubDevices);
_GEGL_CL_WRAP_FUNCTION (clReleaseDevice);
_GEGL_CL_WRAP_FUNCTION (clRetainDevice);

/* Contexts */
_GEGL_CL_WRAP_FUNCTION (clCreateContext);
_GEGL_CL_WRAP_FUNCTION (clCreateContextFromType);
_GEGL_CL_WRAP_FUNCTION (clGetContextInfo);
_GEGL_CL_WRAP_FUNCTION (clReleaseContext);
_GEGL_CL_WRAP_FUNCTION (clRetainContext);
_GEGL_CL_WRAP_FUNCTION (clSetContextDestructorCallback);

/* Command-Queues */
_GEGL_CL_WRAP_FUNCTION (clCreateCommandQueueWithProperties);
_GEGL_CL_WRAP_FUNCTION (clGetCommandQueueInfo);
_GEGL_CL_WRAP_FUNCTION (clReleaseCommandQueue);
_GEGL_CL_WRAP_FUNCTION (clRetainCommandQueue);
_GEGL_CL_WRAP_FUNCTION (clSetDefaultDeviceCommandQueue);

/* Buffer Objects */
_GEGL_CL_WRAP_FUNCTION (clCreateBuffer);
_GEGL_CL_WRAP_FUNCTION (clCreateBufferWithProperties);
_GEGL_CL_WRAP_FUNCTION (clCreateSubBuffer);
_GEGL_CL_WRAP_FUNCTION (clEnqueueReadBuffer);
_GEGL_CL_WRAP_FUNCTION (clEnqueueWriteBuffer);
_GEGL_CL_WRAP_FUNCTION (clEnqueueReadBufferRect);
_GEGL_CL_WRAP_FUNCTION (clEnqueueWriteBufferRect);
_GEGL_CL_WRAP_FUNCTION (clEnqueueCopyBuffer);
_GEGL_CL_WRAP_FUNCTION (clEnqueueCopyBufferRect);
_GEGL_CL_WRAP_FUNCTION (clEnqueueFillBuffer);
_GEGL_CL_WRAP_FUNCTION (clEnqueueMapBuffer);

/* Image Objects */
_GEGL_CL_WRAP_FUNCTION (clCreateImage);
_GEGL_CL_WRAP_FUNCTION (clCreateImageWithProperties);
_GEGL_CL_WRAP_FUNCTION (clEnqueueReadImage);
_GEGL_CL_WRAP_FUNCTION (clEnqueueWriteImage);
_GEGL_CL_WRAP_FUNCTION (clEnqueueCopyImage);
_GEGL_CL_WRAP_FUNCTION (clEnqueueCopyImageToBuffer);
_GEGL_CL_WRAP_FUNCTION (clEnqueueCopyBufferToImage);
_GEGL_CL_WRAP_FUNCTION (clEnqueueFillImage);
_GEGL_CL_WRAP_FUNCTION (clEnqueueMapImage);
_GEGL_CL_WRAP_FUNCTION (clGetImageInfo);
_GEGL_CL_WRAP_FUNCTION (clGetSupportedImageFormats);

/* Memory Objects */
_GEGL_CL_WRAP_FUNCTION (clEnqueueUnmapMemObject);
_GEGL_CL_WRAP_FUNCTION (clEnqueueMigrateMemObjects);
_GEGL_CL_WRAP_FUNCTION (clGetMemObjectInfo);
_GEGL_CL_WRAP_FUNCTION (clRetainMemObject);
_GEGL_CL_WRAP_FUNCTION (clReleaseMemObject);
_GEGL_CL_WRAP_FUNCTION (clSetMemObjectDestructorCallback);

/* Sampler Objects */
_GEGL_CL_WRAP_FUNCTION (clCreateSamplerWithProperties);
_GEGL_CL_WRAP_FUNCTION (clReleaseSampler);
_GEGL_CL_WRAP_FUNCTION (clRetainSampler);
_GEGL_CL_WRAP_FUNCTION (clGetSamplerInfo);

/* Program Objects */
_GEGL_CL_WRAP_FUNCTION (clBuildProgram);
_GEGL_CL_WRAP_FUNCTION (clCompileProgram);
_GEGL_CL_WRAP_FUNCTION (clCreateProgramWithSource);
_GEGL_CL_WRAP_FUNCTION (clCreateProgramWithBinary);
_GEGL_CL_WRAP_FUNCTION (clCreateProgramWithBuiltInKernels);
_GEGL_CL_WRAP_FUNCTION (clCreateProgramWithIL);
_GEGL_CL_WRAP_FUNCTION (clGetProgramBuildInfo);
_GEGL_CL_WRAP_FUNCTION (clGetProgramInfo);
_GEGL_CL_WRAP_FUNCTION (clLinkProgram);
_GEGL_CL_WRAP_FUNCTION (clReleaseProgram);
_GEGL_CL_WRAP_FUNCTION (clRetainProgram);
_GEGL_CL_WRAP_FUNCTION (clSetProgramSpecializationConstant);
_GEGL_CL_WRAP_FUNCTION (clUnloadPlatformCompiler);

/* Kernel Objects */
_GEGL_CL_WRAP_FUNCTION (clCloneKernel);
_GEGL_CL_WRAP_FUNCTION (clCreateKernel);
_GEGL_CL_WRAP_FUNCTION (clCreateKernelsInProgram);
_GEGL_CL_WRAP_FUNCTION (clGetKernelInfo);
_GEGL_CL_WRAP_FUNCTION (clGetKernelArgInfo);
_GEGL_CL_WRAP_FUNCTION (clGetKernelSubGroupInfo);
_GEGL_CL_WRAP_FUNCTION (clGetKernelWorkGroupInfo);
_GEGL_CL_WRAP_FUNCTION (clReleaseKernel);
_GEGL_CL_WRAP_FUNCTION (clRetainKernel);
_GEGL_CL_WRAP_FUNCTION (clSetKernelArg);
_GEGL_CL_WRAP_FUNCTION (clSetKernelArgSVMPointer);
_GEGL_CL_WRAP_FUNCTION (clSetKernelExecInfo);

/* Executing Kernels */
_GEGL_CL_WRAP_FUNCTION (clEnqueueNDRangeKernel);
_GEGL_CL_WRAP_FUNCTION (clEnqueueNativeKernel);

/* Event Objects */
_GEGL_CL_WRAP_FUNCTION (clCreateUserEvent);
_GEGL_CL_WRAP_FUNCTION (clGetEventInfo);
_GEGL_CL_WRAP_FUNCTION (clReleaseEvent);
_GEGL_CL_WRAP_FUNCTION (clRetainEvent);
_GEGL_CL_WRAP_FUNCTION (clSetEventCallback);
_GEGL_CL_WRAP_FUNCTION (clSetUserEventStatus);
_GEGL_CL_WRAP_FUNCTION (clWaitForEvents);

/* Markers, Barriers, and Waiting */
_GEGL_CL_WRAP_FUNCTION (clEnqueueBarrierWithWaitList);
_GEGL_CL_WRAP_FUNCTION (clEnqueueMarkerWithWaitList);

/* Profiling Operations on Memory Objects and Kernels */
_GEGL_CL_WRAP_FUNCTION (clGetDeviceAndHostTimer);
_GEGL_CL_WRAP_FUNCTION (clGetEventProfilingInfo);
_GEGL_CL_WRAP_FUNCTION (clGetHostTimer);

/* Flush and Finish */
_GEGL_CL_WRAP_FUNCTION (clFlush);
_GEGL_CL_WRAP_FUNCTION (clFinish);

/* Pipes */
_GEGL_CL_WRAP_FUNCTION (clCreatePipe);
_GEGL_CL_WRAP_FUNCTION (clGetPipeInfo);

/* Shared Virtual Memory (SVM) */
_GEGL_CL_WRAP_FUNCTION (clSVMAlloc);
_GEGL_CL_WRAP_FUNCTION (clSVMFree);
_GEGL_CL_WRAP_FUNCTION (clEnqueueSVMFree);
_GEGL_CL_WRAP_FUNCTION (clEnqueueSVMMap);
_GEGL_CL_WRAP_FUNCTION (clEnqueueSVMMemcpy);
_GEGL_CL_WRAP_FUNCTION (clEnqueueSVMMemFill);
_GEGL_CL_WRAP_FUNCTION (clEnqueueSVMMigrateMem);
_GEGL_CL_WRAP_FUNCTION (clEnqueueSVMUnmap);

/* Optional Extensions */
_GEGL_CL_WRAP_FUNCTION (clGetExtensionFunctionAddressForPlatform);

/* cl_khr_gl_sharing */
_GEGL_CL_WRAP_FUNCTION (clGetGLContextInfoKHR);
_GEGL_CL_WRAP_FUNCTION (clCreateFromGLBuffer);
_GEGL_CL_WRAP_FUNCTION (clCreateFromGLTexture);
_GEGL_CL_WRAP_FUNCTION (clCreateFromGLRenderbuffer);
_GEGL_CL_WRAP_FUNCTION (clGetGLObjectInfo);
_GEGL_CL_WRAP_FUNCTION (clGetGLTextureInfo);
_GEGL_CL_WRAP_FUNCTION (clEnqueueAcquireGLObjects);
_GEGL_CL_WRAP_FUNCTION (clEnqueueReleaseGLObjects);

G_END_DECLS

#endif  /* __GEGL_CL_INIT_H__ */

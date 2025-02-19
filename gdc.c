/*
 *   Copyright 2013 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <IONmem.h>
#include <uapi/linux/dma-buf.h>
#include <uapi/linux/dma-heap.h>
#include "gdc_api.h"

#define   FILE_NAME_GDC      "/dev/gdc"
#define   FILE_NAME_AML_GDC  "/dev/amlgdc"

int gdc_create_ctx(struct gdc_usr_ctx_s *ctx)
{
	int ion_fd = -1, heap_fd = -1;
	char *dev_name = (ctx->dev_type == ARM_GDC) ? FILE_NAME_GDC :
						      FILE_NAME_AML_GDC;

	ctx->gdc_client = open(dev_name, O_RDWR | O_SYNC);

	if (ctx->gdc_client < 0) {
		E_GDC("gdc open failed error=%d, %s",
			errno, strerror(errno));
		return -1;
	}

	heap_fd = dmabuf_heap_open(GENERIC_HEAP);
	if (heap_fd < 0) {
		ion_fd = ion_mem_init();
		if (ion_fd < 0) {
			E_GDC("%s %d, ion and dma_heap open failed\n", __func__, __LINE__);
		}
	}

	ctx->dma_heap_fd = heap_fd;
	ctx->ion_fd = ion_fd;

	return 0;
}

int gdc_destroy_ctx(struct gdc_usr_ctx_s *ctx)
{
	int i;

	if (ctx->c_buff != NULL) {
		munmap(ctx->c_buff, ctx->c_len);
		ctx->c_buff = NULL;
		ctx->c_len = 0;
	}
	if (ctx->gs_ex.config_buffer.shared_fd > 0) {
		D_GDC("close config_fd\n");
		close(ctx->gs_ex.config_buffer.shared_fd);
	}

	for (i = 0; i < ctx->plane_number; i++) {
		if (ctx->i_buff[i] != NULL) {
			munmap(ctx->i_buff[i], ctx->i_len[i]);
			ctx->i_buff[i] = NULL;
			ctx->i_len[i] = 0;
		}

		if (i == 0) {
			if (ctx->gs_ex.input_buffer.shared_fd > 0) {
				D_GDC("close in_fd=%d\n",
					ctx->gs_ex.input_buffer.shared_fd);
				close(ctx->gs_ex.input_buffer.shared_fd);
			}
		} else if (i == 1) {
			if (ctx->gs_ex.input_buffer.uv_base_fd > 0) {
				D_GDC("close in_fd=%d\n",
					ctx->gs_ex.input_buffer.uv_base_fd);
				close(ctx->gs_ex.input_buffer.uv_base_fd);
			}
		} else if (i == 2) {
			if (ctx->gs_ex.input_buffer.v_base_fd > 0) {
				D_GDC("close in_fd=%d\n",
					ctx->gs_ex.input_buffer.v_base_fd);
				close(ctx->gs_ex.input_buffer.v_base_fd);
			}
		}

		if (ctx->o_buff[i] != NULL) {
			munmap(ctx->o_buff[i], ctx->o_len[i]);
			ctx->o_buff[i] = NULL;
			ctx->o_len[i] = 0;
		}
		if (i == 0) {
			if (ctx->gs_ex.output_buffer.shared_fd > 0) {
				D_GDC("close out_fd=%d\n",
					ctx->gs_ex.output_buffer.shared_fd);
				close(ctx->gs_ex.output_buffer.shared_fd);
			}
		} else if (i == 1) {
			if (ctx->gs_ex.output_buffer.uv_base_fd > 0) {
				D_GDC("close out_fd=%d\n",
					ctx->gs_ex.output_buffer.uv_base_fd);
				close(ctx->gs_ex.output_buffer.uv_base_fd);
			}
		} else if (i == 2) {
			if (ctx->gs_ex.output_buffer.v_base_fd > 0) {
				D_GDC("close out_fd=%d\n",
					ctx->gs_ex.output_buffer.v_base_fd);
				close(ctx->gs_ex.output_buffer.v_base_fd);
			}
		}
	}
	if (ctx->gdc_client >= 0) {
		close(ctx->gdc_client);
		ctx->gdc_client = -1;
	}

	if (ctx->ion_fd >= 0) {
		ion_mem_exit(ctx->ion_fd);
		ctx->ion_fd = -1;
	}

	if (ctx->dma_heap_fd >= 0) {
		dmabuf_heap_close(ctx->dma_heap_fd);
		ctx->dma_heap_fd = -1;
	}

	return 0;
}

static int _gdc_alloc_dma_buffer(int fd, unsigned int dir,
						unsigned int len)
{
	int ret = -1;
	struct gdc_dmabuf_req_s buf_cfg;

	memset(&buf_cfg, 0, sizeof(buf_cfg));

	buf_cfg.dma_dir = dir;
	buf_cfg.len = len;

	ret = ioctl(fd, GDC_REQUEST_DMA_BUFF, &buf_cfg);
	if (ret < 0) {
		E_GDC("GDC_REQUEST_DMA_BUFF %s failed: %s, fd=%d\n", __func__,
			strerror(ret), fd);
		return ret;
	}
	return buf_cfg.index;
}

static int _gdc_get_dma_buffer_fd(int fd, int index)
{
	struct gdc_dmabuf_exp_s ex_buf;

	ex_buf.index = index;
	ex_buf.flags = O_RDWR;
	ex_buf.fd = -1;

	if (ioctl(fd, GDC_EXP_DMA_BUFF, &ex_buf)) {
		E_GDC("failed get dma buf fd\n");
		return -1;
	}
	D_GDC("dma buffer export, fd=%d\n", ex_buf.fd);

	return ex_buf.fd;
}

static int _gdc_free_dma_buffer(int fd, int index)
{
	if (ioctl(fd, GDC_FREE_DMA_BUFF, &index))  {
		E_GDC("failed free dma buf fd\n");
		return -1;
	}
	return 0;
}

static int set_buf_fd(gdc_alloc_buffer_t *buf, gdc_buffer_info_t *buf_info,
				int buf_fd, int plane_id)
{
	int ret = 0;

	D_GDC("buf_fd=%d, plane_id=%d, buf->format=%d\n", buf_fd,
				plane_id, buf->format);
	switch (buf->format & FORMAT_TYPE_MASK) {
	case NV12:
		if (buf->plane_number == 1) {
			buf_info->shared_fd = buf_fd;
		} else {
			if (plane_id == 0)
				buf_info->y_base_fd = buf_fd;
			else if(plane_id == 1)
				buf_info->uv_base_fd = buf_fd;
			else {
				E_GDC("plane id(%d) error\n", plane_id);
				ret = -1;
			}
		}
		break;
	case YV12:
		if (buf->plane_number == 1) {
			buf_info->shared_fd = buf_fd;
		} else {
			if (plane_id == 0)
				buf_info->y_base_fd = buf_fd;
			else if (plane_id == 1)
				buf_info->u_base_fd = buf_fd;
			else if (plane_id == 2)
				buf_info->v_base_fd = buf_fd;
			else {
				E_GDC("plane id(%d) error\n", plane_id);
				ret = -1;
			}
		}
		break;
	case Y_GREY:
		if (buf->plane_number == 1) {
			buf_info->shared_fd = buf_fd;
		} else {
			E_GDC("plane id(%d) error\n", plane_id);
			ret = -1;
		}
		break;
	case YUV444_P:
	case RGB444_P:
		if (buf->plane_number == 1) {
			buf_info->shared_fd = buf_fd;
		} else {
			if (plane_id == 0)
			buf_info->y_base_fd = buf_fd;
			else if (plane_id == 1)
				buf_info->u_base_fd = buf_fd;
			else if (plane_id == 2)
				buf_info->v_base_fd = buf_fd;
			else {
				E_GDC("plane id(%d) error\n", plane_id);
				ret = -1;
			}
		}
		break;
	default:
		return -1;
		break;
	}
	D_GDC("buf_info->shared_fd=%d\n",buf_info->shared_fd);
	D_GDC("buf_info->y_base_fd=%d\n",buf_info->y_base_fd);
	D_GDC("buf_info->uv_base_fd=%d\n",buf_info->uv_base_fd);
	return ret;
}

int dmabuf_heap_open(char *name)
{
	int ret, fd;
	char buf[256];

	ret = snprintf(buf, 256, "%s/%s", DEVPATH, name);
	if (ret < 0) {
		E_GDC("snprintf failed!\n");
		return ret;
	}

	fd = open(buf, O_RDWR);
	/* if errno == 0, which means heap_codecmm open success
	 * else if errno == 2, which means there is no heap_codecmm, using ion replace
	 */
	if (errno != 0 && errno != 2)
		E_GDC("%s %d open %s failed %s\n", __func__, __LINE__, buf , strerror(errno));
	return fd;
}

static int dmabuf_heap_alloc_fdflags(int fd, size_t len, unsigned int fd_flags,
				     unsigned int heap_flags, int *dmabuf_fd)
{
	struct dma_heap_allocation_data data = {
		.len = len,
		.fd = 0,
		.fd_flags = fd_flags,
		.heap_flags = heap_flags,
	};
	int ret;

	if (!dmabuf_fd)
		return -EINVAL;

	ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &data);
	if (ret < 0)
		return ret;
	*dmabuf_fd = (int)data.fd;
	return ret;
}

int dmabuf_heap_alloc(int fd, size_t len, unsigned int flags,
			     int *dmabuf_fd)
{
	return dmabuf_heap_alloc_fdflags(fd, len, O_RDWR | O_CLOEXEC, flags,
					 dmabuf_fd);
}

void dmabuf_sync(int fd, int start_stop)
{
	struct dma_buf_sync sync = {
		.flags = start_stop | DMA_BUF_SYNC_RW,
	};
	int ret;

	ret = ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
	if (ret)
		E_GDC("sync failed %d\n", errno);
}

void dmabuf_heap_release(int dmabuf_fd)
{
	if (dmabuf_fd >= 0)
		close(dmabuf_fd);
}

void  dmabuf_heap_close(int heap_fd)
{
	if (heap_fd >= 0)
		close(heap_fd);
}

/* gdc: allocate a block of memory */
int gdc_alloc_mem(struct gdc_usr_ctx_s *ctx, uint32_t len, uint32_t type)
{
	int dir = 0, index = -1, buf_fd = -1;
	struct gdc_dmabuf_req_s buf_cfg;

	memset(&buf_cfg, 0, sizeof(buf_cfg));
	if (type == OUTPUT_BUFF_TYPE)
		dir = DMA_FROM_DEVICE;
	else
		dir = DMA_TO_DEVICE;

	buf_cfg.len = len;
	buf_cfg.dma_dir = dir;

	index = _gdc_alloc_dma_buffer(ctx->gdc_client, dir,
					buf_cfg.len);
	if (index < 0)
		return -1;

	/* get dma fd*/
	buf_fd = _gdc_get_dma_buffer_fd(ctx->gdc_client,
						index);
	if (buf_fd < 0) {
		E_GDC("%s: alloc failed\n", __func__);
		return -1;
	}
	D_GDC("%s: dma_fd=%d\n", buf_fd, __func__);
	/* after alloc, dmabuffer free can be called, it just dec refcount */
	_gdc_free_dma_buffer(ctx->gdc_client, index);

	return buf_fd;
}

/* gdc: free a block of memory */
void gdc_release_mem(int shared_fd)
{
	if (shared_fd > 0)
		close(shared_fd);
}

/* gdc: sync cache for a block of memory */
void gdc_sync_for_device_mem(struct gdc_usr_ctx_s *ctx, int shared_fd)
{
	int ret = -1;

	if (shared_fd > 0) {
		ret = ioctl(ctx->gdc_client, GDC_SYNC_DEVICE, &shared_fd);
		if (ret < 0) {
			E_GDC("%s ioctl failed\n", __func__);
			return;
		}
	}
}

/* gdc: invalid cache for a block of memory */
void gdc_sync_for_cpu_mem(struct gdc_usr_ctx_s *ctx, int shared_fd)
{
	int ret = -1;

	if (shared_fd > 0) {
		ret = ioctl(ctx->gdc_client, GDC_SYNC_CPU, &shared_fd);
		if (ret < 0) {
			E_GDC("%s ioctl failed\n", __func__);
			return;
		}
	}
}


/* gdc or ion: alloc memory */
int gdc_alloc_buffer (struct gdc_usr_ctx_s *ctx, uint32_t type,
			struct gdc_alloc_buffer_s *buf, bool cache_flag)
{
	int dir = 0, index = -1, buf_fd[MAX_PLANE] = {-1};
	struct gdc_dmabuf_req_s buf_cfg;
	struct gdc_settings_ex *gs_ex;
	int i;

	gs_ex = &(ctx->gs_ex);
	memset(&buf_cfg, 0, sizeof(buf_cfg));
	if (type == OUTPUT_BUFF_TYPE)
		dir = DMA_FROM_DEVICE;
	else
		dir = DMA_TO_DEVICE;

	for (i = 0; i < buf->plane_number; i++) {
		buf_cfg.len = buf->len[i];
		buf_cfg.dma_dir = dir;

		if (ctx->mem_type == AML_GDC_MEM_ION) {
			int ret = -1;
			if (ctx->ion_fd >= 0) {
				IONMEM_AllocParams ion_alloc_params;
				ret = ion_mem_alloc(ctx->ion_fd, buf->len[i], &ion_alloc_params,
							cache_flag);
				if (ret < 0) {
					E_GDC("%s,%d,Not enough memory\n",__func__,
						__LINE__);
					return -1;
				}
				buf_fd[i] = ion_alloc_params.mImageFd;
				D_GDC("gdc_alloc_buffer: ion_fd=%d\n", buf_fd[i]);
			} else if (ctx->dma_heap_fd >= 0) {
				ret = dmabuf_heap_alloc(ctx->dma_heap_fd, buf->len[i], 0, &buf_fd[i]);
				if (ret < 0) {
					E_GDC("%s,%d,Not enough memory\n",__func__, __LINE__);
					return -1;
				}
				D_GDC("gdc_alloc_buffer: dma_heap_fd=%d\n", buf_fd[i]);
			}
		} else if (ctx->mem_type == AML_GDC_MEM_DMABUF) {
			index = _gdc_alloc_dma_buffer(ctx->gdc_client, dir,
							buf_cfg.len);
			if (index < 0)
				return -1;

			/* get  dma fd*/
			buf_fd[i] = _gdc_get_dma_buffer_fd(ctx->gdc_client,
								index);
			if (buf_fd[i] < 0)
				return -1;
			D_GDC("gdc_alloc_buffer: dma_fd=%d\n", buf_fd[i]);
			/* after alloc, dmabuffer free can be called, it just dec refcount */
			_gdc_free_dma_buffer(ctx->gdc_client, index);
		} else {
			E_GDC("gdc_alloc_buffer: wrong mem_type\n");
			return -1;
		}

		switch (type) {
		case INPUT_BUFF_TYPE:
			ctx->i_len[i] = buf->len[i];
			ctx->i_buff[i] = mmap(NULL, buf->len[i],
				PROT_READ | PROT_WRITE,
				MAP_SHARED, buf_fd[i], 0);
			if (ctx->i_buff[i] == MAP_FAILED) {
				ctx->i_buff[i] = NULL;
				E_GDC("Failed to alloc i_buff:%s\n",
					strerror(errno));
			}
			set_buf_fd(buf, &(gs_ex->input_buffer) ,buf_fd[i], i);
			gs_ex->input_buffer.plane_number = buf->plane_number;
			gs_ex->input_buffer.mem_alloc_type = ctx->mem_type;
			break;
		case OUTPUT_BUFF_TYPE:
			ctx->o_len[i] = buf->len[i];
			ctx->o_buff[i] = mmap(NULL, buf->len[i],
				PROT_READ | PROT_WRITE,
				MAP_SHARED, buf_fd[i], 0);
			if (ctx->o_buff[i] == MAP_FAILED) {
				ctx->o_buff[i] = NULL;
				E_GDC("Failed to alloc o_buff:%s\n",
						strerror(errno));
			}
			set_buf_fd(buf, &(gs_ex->output_buffer),buf_fd[i], i);
			gs_ex->output_buffer.plane_number = buf->plane_number;
			gs_ex->output_buffer.mem_alloc_type = ctx->mem_type;
			break;
		case CONFIG_BUFF_TYPE:
			if (i > 0)
				break;
			ctx->c_len = buf->len[i];
			ctx->c_buff = mmap(NULL, buf->len[i],
				PROT_READ | PROT_WRITE,
				MAP_SHARED, buf_fd[i], 0);
			if (ctx->c_buff == MAP_FAILED) {
				ctx->c_buff = NULL;
				E_GDC("Failed to alloc c_buff:%s\n",
						strerror(errno));
			}
			gs_ex->config_buffer.shared_fd = buf_fd[i];
			gs_ex->config_buffer.plane_number = 1;
			gs_ex->config_buffer.mem_alloc_type = ctx->mem_type;
			break;
		default:
			E_GDC("Error no such buff type\n");
			break;
		}
	}
	return 0;
}

int gdc_process(struct gdc_usr_ctx_s *ctx)
{
	int ret = -1;
	struct gdc_settings_ex *gs_ex = &ctx->gs_ex;

	ret = ioctl(ctx->gdc_client, GDC_PROCESS_EX, gs_ex);
	if (ret < 0) {
		E_GDC("GDC_RUN ioctl failed\n");
		return ret;
	}

	return 0;
}

int gdc_process_with_builtin_fw(struct gdc_usr_ctx_s *ctx)
{
	int ret = -1;
	struct gdc_settings_with_fw *gs_with_fw = &ctx->gs_with_fw;

	ret = ioctl(ctx->gdc_client, GDC_PROCESS_WITH_FW, gs_with_fw);
	if (ret < 0) {
		E_GDC("GDC_PROCESS_WITH_FW ioctl failed\n");
		return ret;
	}

	return 0;
}

/* gdc or ion: sync cache */
int gdc_sync_for_device(struct gdc_usr_ctx_s *ctx)
{
	int ret = -1, i;
	int shared_fd[MAX_PLANE];
	int plane_number = 0;
	struct gdc_settings_ex *gs_ex = &ctx->gs_ex;

	if (gs_ex->input_buffer.mem_alloc_type == AML_GDC_MEM_DMABUF) {
		plane_number = gs_ex->input_buffer.plane_number;
		for (i = 0; i < plane_number; i++) {
			if (i == 0)
				shared_fd[i] = gs_ex->input_buffer.shared_fd;
			else if (i == 1)
				shared_fd[i] = gs_ex->input_buffer.uv_base_fd;
			else if (i == 2)
				shared_fd[i] = gs_ex->input_buffer.v_base_fd;
			ret = ioctl(ctx->gdc_client, GDC_SYNC_DEVICE,
					&shared_fd[i]);
			if (ret < 0) {
				E_GDC("gdc_sync_for_device ioctl failed\n");
				return ret;
			}
		}
	}
	if (!ctx->custom_fw &&
		gs_ex->config_buffer.mem_alloc_type == AML_GDC_MEM_DMABUF) {
		shared_fd[0] = gs_ex->config_buffer.shared_fd;
		ret = ioctl(ctx->gdc_client, GDC_SYNC_DEVICE,
				&shared_fd[0]);
		if (ret < 0) {
			E_GDC("gdc_sync_for_device ioctl failed\n");
			return ret;
		}
	}

	return 0;
}

/* gdc or ion: invalid cache */
int gdc_sync_for_cpu(struct gdc_usr_ctx_s *ctx)
{
	int ret = -1, i;
	int shared_fd[MAX_PLANE];
	int plane_number = 0;
	struct gdc_settings_ex *gs_ex = &ctx->gs_ex;

	if (gs_ex->output_buffer.mem_alloc_type == AML_GDC_MEM_DMABUF) {
		plane_number = gs_ex->output_buffer.plane_number;
		for (i = 0; i < plane_number; i++) {
			if (i == 0)
				shared_fd[i] = gs_ex->output_buffer.shared_fd;
			else if (i == 1)
				shared_fd[i] = gs_ex->output_buffer.uv_base_fd;
			else if (i == 2)
				shared_fd[i] = gs_ex->output_buffer.v_base_fd;
			ret = ioctl(ctx->gdc_client, GDC_SYNC_CPU,
					&shared_fd[i]);
			if (ret < 0) {
				E_GDC("gdc_sync_for_cpu ioctl failed\n");
				return ret;
			}
		}
	} else if (ctx->ion_fd >= 0) {
		plane_number = gs_ex->output_buffer.plane_number;
		for (i = 0; i < plane_number; i++) {
			if (i == 0)
				ion_mem_invalid_cache(ctx->ion_fd,
						gs_ex->output_buffer.shared_fd);
			else if (i == 1)
				ion_mem_invalid_cache(ctx->ion_fd,
						gs_ex->output_buffer.uv_base_fd);
			else if (i == 2)
				ion_mem_invalid_cache(ctx->ion_fd,
						gs_ex->output_buffer.v_base_fd);
		}
	}

	return 0;
}

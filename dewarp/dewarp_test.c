/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <gdc_api.h>
#include <IONmem.h>
#include "dewarp_api.h"

struct dewarp_params dewarp_params;
static int process_circle = 1;
static int in_bit_width;
static int out_bit_width;
char in_file[256];
char out_file[256];
char dump_fw_file[256];
char static_fw_file[256];

char meshin_file[WIN_MAX][256];
float *meshin_data_table[WIN_MAX];

static inline unsigned long myclock()
{
	struct timeval tv;

	gettimeofday (&tv, NULL);

	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static void print_usage(void)
{
	printf ("Usage: dewarp_test [options]\n\n");
	printf ("Options:\n\n");
	printf ("  -help                                                                                                                                                   \n");
	printf ("  -win_num <Num>                                                                                                                                          \n");
	printf ("  -tile_x_step <Num>                                                                                                                                      \n");
	printf ("  -tile_y_step <Num>                                                                                                                                      \n");
	printf ("  -input_size <WxH>                                                                                                                                       \n");
	printf ("  -input_offset <X_Y>                                                                                                                                     \n");
	printf ("  -fov <Num>                                                                                                                                              \n");
	printf ("  -color_mode <0:YUV420_PLANAR 1:YUV420_SEMIPLANAR 2:YONLY>                                                                                               \n");
	printf ("  -in_bit_width  <0:8bit 1:10bit 2:12bit 3:16bit>                                                                                                         \n");
	printf ("  -out_bit_width <0:8bit 1:10bit 2:12bit 3:16bit>                                                                                                         \n");
	printf ("  -output_size <WxH>                                                                                                                                      \n");
	printf ("  -proj1 <ProjMode_Pan_Tilt_Rotation_Zoom_StrengthH_StrengthV>                                                                                            \n");
	printf ("  -proj2 <ProjMode_Pan_Tilt_Rotation_Zoom_StrengthH_StrengthV>                                                                                            \n");
	printf ("  -proj3 <ProjMode_Pan_Tilt_Rotation_Zoom_StrengthH_StrengthV>                                                                                            \n");
	printf ("  -proj4 <ProjMode_Pan_Tilt_Rotation_Zoom_StrengthH_StrengthV>                                                                                            \n");
	printf ("  -clb1 <Fx_Fy_Cx_Cy_K1_K2_K3_P1_P2_K4_K5_K6>                                                                                                             \n");
	printf ("  -clb2 <Fx_Fy_Cx_Cy_K1_K2_K3_P1_P2_K4_K5_K6>                                                                                                             \n");
	printf ("  -clb3 <Fx_Fy_Cx_Cy_K1_K2_K3_P1_P2_K4_K5_K6>                                                                                                             \n");
	printf ("  -clb4 <Fx_Fy_Cx_Cy_K1_K2_K3_P1_P2_K4_K5_K6>                                                                                                             \n");
	printf ("  -meshin1 <Xstart_Ystart_Xlen_Ylen_Xstep_Ystep>                                                                                                          \n");
	printf ("  -meshin2 <Xstart_Ystart_Xlen_Ylen_Xstep_Ystep>                                                                                                          \n");
	printf ("  -meshin3 <Xstart_Ystart_Xlen_Ylen_Xstep_Ystep>                                                                                                          \n");
	printf ("  -meshin4 <Xstart_Ystart_Xlen_Ylen_Xstep_Ystep>                                                                                                          \n");
	printf ("  -meshin1_file <MeshinDataFileName>                                                                                                                      \n");
	printf ("  -meshin2_file <MeshinDataFileName>                                                                                                                      \n");
	printf ("  -meshin3_file <MeshinDataFileName>                                                                                                                      \n");
	printf ("  -meshin4_file <MeshinDataFileName>                                                                                                                      \n");
	printf ("  -dptz_param SrcU_SrcV_OutWW_OutHH_SrbUW_SrbVW_SrbWW_SrbHH_ScWW_ScHH_ZoomP_ZoomQ_PixelAspectRatioX_PixelAspectRatioY_Fx_Fy_UC_VC_K1_K2_K3_K4_K5_K6_P1_P2 \n");
	printf ("  -win1 <WinStartX_WinEndX_WinStartY_WinEndY_ImgStartX_ImgEndX_ImgStartY_ImgEndY>                                                                         \n");
	printf ("  -win2 <WinStartX_WinEndX_WinStartY_WinEndY_ImgStartX_ImgEndX_ImgStartY_ImgEndY>                                                                         \n");
	printf ("  -win3 <WinStartX_WinEndX_WinStartY_WinEndY_ImgStartX_ImgEndX_ImgStartY_ImgEndY>                                                                         \n");
	printf ("  -win4 <WinStartX_WinEndX_WinStartY_WinEndY_ImgStartX_ImgEndX_ImgStartY_ImgEndY>                                                                         \n");
	printf ("  -prm_mode <0:use proj_param, 1:use clb_param 2:use meshin 3:use dptz_param>                                                                             \n");
	printf ("  -circle <Num>                                                                                                                                           \n");
	printf ("  -in_file  <ImageName>                                                                                                                                   \n");
	printf ("  -out_file <ImageName>                                                                                                                                   \n");
	printf ("  -static_fw_file <FirmwareName>                                                                                                                          \n");
	printf ("  -dump_fw_file <FirmwareName>                                                                                                                            \n");
	printf ("\n");
}

static int parse_command_line(int argc, char *argv[])
{
	int i, param_cnt = 0;
	struct input_param *in      = &dewarp_params.input_param;
	struct output_param *out    = &dewarp_params.output_param;
	struct proj_param *proj     = &dewarp_params.proj_param[0];
	struct win_param *win       = &dewarp_params.win_param[0];
	struct clb_param *clb       = &dewarp_params.clb_param[0];
	struct meshin_param *meshin = &dewarp_params.meshin_param[0];
	struct dptz_param * dptz_param = &dewarp_params.dptz_param;

	dewarp_params.tile_x_step = 8; /* default x step */
	dewarp_params.tile_y_step = 8; /* default y step */
	/* parse command line */
	for (i = 1; i < argc; i++) {
		if (strncmp (argv[i], "-", 1) == 0) {
			if (strcmp (argv[i] + 1, "help") == 0) {
				print_usage();
				return -1;
			} else if (strcmp (argv[i] + 1, "win_num") == 0 && ++i < argc &&
				sscanf (argv[i], "%d", &dewarp_params.win_num) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "win_num") == 0 && ++i < argc &&
				sscanf (argv[i], "%d", &dewarp_params.win_num) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "input_size") == 0 && ++i < argc &&
				sscanf (argv[i], "%dx%d", &in->width, &in->height) == 2) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "input_offset") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d", &in->offset_x, &in->offset_y) == 2) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "fov") == 0 && ++i < argc &&
				sscanf (argv[i], "%d", &in->fov) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "color_mode") == 0 && ++i < argc &&
				sscanf (argv[i], "%d", &dewarp_params.color_mode) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "in_bit_width") == 0 && ++i < argc &&
				sscanf (argv[i], "%d", &in_bit_width) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "out_bit_width") == 0 && ++i < argc &&
				sscanf (argv[i], "%d", &out_bit_width) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "output_size") == 0 && ++i < argc &&
				sscanf (argv[i], "%dx%d", &out->width, &out->height) == 2) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "proj1") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%f_%f_%f", &proj[0].projection_mode, &proj[0].pan, &proj[0].tilt,
					&proj[0].rotation, &proj[0].zoom, &proj[0].strength_hor, &proj[0].strength_ver) == 7) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "proj2") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%f_%f_%f", &proj[1].projection_mode, &proj[1].pan, &proj[1].tilt,
					&proj[1].rotation, &proj[1].zoom, &proj[1].strength_hor, &proj[1].strength_ver) == 7) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "proj3") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%f_%f_%f", &proj[2].projection_mode, &proj[2].pan, &proj[2].tilt,
					&proj[2].rotation, &proj[2].zoom, &proj[2].strength_hor, &proj[2].strength_ver) == 7) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "proj4") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%f_%f_%f", &proj[3].projection_mode, &proj[3].pan, &proj[3].tilt,
					&proj[3].rotation, &proj[3].zoom, &proj[3].strength_hor, &proj[3].strength_ver) == 7) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "clb1") == 0 && ++i < argc &&
				sscanf (argv[i], "%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf", &clb[0].fx, &clb[0].fy, &clb[0].cx, &clb[0].cy,
				&clb[0].k1, &clb[0].k2, &clb[0].k3, &clb[0].p1, &clb[0].p2, &clb[0].k4, &clb[0].k5, &clb[0].k6) == 12) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "clb2") == 0 && ++i < argc &&
				sscanf (argv[i], "%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf", &clb[1].fx, &clb[1].fy, &clb[1].cx, &clb[1].cy,
				&clb[1].k1, &clb[1].k2, &clb[1].k3, &clb[1].p1, &clb[1].p2, &clb[1].k4, &clb[1].k5, &clb[1].k6) == 12) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "clb3") == 0 && ++i < argc &&
				sscanf (argv[i], "%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf", &clb[2].fx, &clb[2].fy, &clb[2].cx, &clb[2].cy,
				&clb[2].k1, &clb[2].k2, &clb[2].k3, &clb[2].p1, &clb[2].p2, &clb[2].k4, &clb[2].k5, &clb[2].k6) == 12) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "clb4") == 0 && ++i < argc &&
				sscanf (argv[i], "%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf_%lf", &clb[3].fx, &clb[3].fy, &clb[3].cx, &clb[3].cy,
				&clb[3].k1, &clb[3].k2, &clb[3].k3, &clb[3].p1, &clb[3].p2, &clb[3].k4, &clb[3].k5, &clb[3].k6) == 12) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "meshin1") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%d_%d", &meshin[0].x_start, &meshin[0].y_start, &meshin[0].x_len, &meshin[0].y_len,
					&meshin[0].x_step, &meshin[0].y_step) == 6) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "meshin2") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%d_%d", &meshin[1].x_start, &meshin[1].y_start, &meshin[1].x_len, &meshin[1].y_len,
					&meshin[1].x_step, &meshin[1].y_step) == 6) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "meshin3") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%d_%d", &meshin[2].x_start, &meshin[2].y_start, &meshin[2].x_len, &meshin[2].y_len,
					&meshin[2].x_step, &meshin[2].y_step) == 6) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "meshin4") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%d_%d", &meshin[3].x_start, &meshin[3].y_start, &meshin[3].x_len, &meshin[3].y_len,
					&meshin[3].x_step, &meshin[3].y_step) == 6) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "meshin1_file") == 0 && ++i < argc &&
				sscanf (argv[i], "%s", meshin_file[0]) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "meshin2_file") == 0 && ++i < argc &&
				sscanf (argv[i], "%s", meshin_file[1]) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "meshin3_file") == 0 && ++i < argc &&
				sscanf (argv[i], "%s", meshin_file[2]) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "meshin4_file") == 0 && ++i < argc &&
				sscanf (argv[i], "%s", meshin_file[3]) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "dptz_param") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%f_%f_%f_%f_%f_%f_%f_%f_%f_%f_%f_%f_%f_%f_%f_%f",
					&dptz_param->src_u,  &dptz_param->src_v, &dptz_param->out_ww, &dptz_param->out_hh,
					&dptz_param->srB_uW, &dptz_param->srB_vW,&dptz_param->srB_ww, &dptz_param->srB_hh,
					&dptz_param->sc_ww,  &dptz_param->sc_hh, &dptz_param->zoom_p, &dptz_param->zoom_q,
					&dptz_param->pixel_aspect_ratio_x, &dptz_param->pixel_aspect_ratio_y,
					&dptz_param->fx,&dptz_param->fy, &dptz_param->uC,&dptz_param->vC,
					&dptz_param->k1,&dptz_param->k2,&dptz_param->k3,&dptz_param->k4,
					&dptz_param->k5,&dptz_param->k6,&dptz_param->p1,&dptz_param->p2) == 26) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "win1") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%d_%d_%d_%d", &win[0].win_start_x, &win[0].win_end_x, &win[0].win_start_y, &win[0].win_end_y,
					&win[0].img_start_x, &win[0].img_end_x, &win[0].img_start_y, &win[0].img_end_y) == 8) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "win2") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%d_%d_%d_%d", &win[1].win_start_x, &win[1].win_end_x, &win[1].win_start_y, &win[1].win_end_y,
					&win[1].img_start_x, &win[1].img_end_x, &win[1].img_start_y, &win[1].img_end_y) == 8) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "win3") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%d_%d_%d_%d", &win[2].win_start_x, &win[2].win_end_x, &win[2].win_start_y, &win[2].win_end_y,
					&win[2].img_start_x, &win[2].img_end_x, &win[2].img_start_y, &win[2].img_end_y) == 8) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "win4") == 0 && ++i < argc &&
				sscanf (argv[i], "%d_%d_%d_%d_%d_%d_%d_%d", &win[3].win_start_x, &win[3].win_end_x, &win[3].win_start_y, &win[3].win_end_y,
					&win[3].img_start_x, &win[3].img_end_x, &win[3].img_start_y, &win[3].img_end_y) == 8) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "prm_mode") == 0 && ++i < argc &&
				sscanf (argv[i], "%d", &dewarp_params.prm_mode) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "tile_x_step") == 0 && ++i < argc &&
				sscanf (argv[i], "%d", &dewarp_params.tile_x_step) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "tile_y_step") == 0 && ++i < argc &&
				sscanf (argv[i], "%d", &dewarp_params.tile_y_step) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "circle") == 0 && ++i < argc &&
				sscanf (argv[i], "%d", &process_circle) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "in_file") == 0 && ++i < argc &&
				sscanf (argv[i], "%s", in_file) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "out_file") == 0 && ++i < argc &&
				sscanf (argv[i], "%s", out_file) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "static_fw_file") == 0 && ++i < argc &&
				sscanf (argv[i], "%s", static_fw_file) == 1) {
				param_cnt++;
				continue;
			} else if (strcmp (argv[i] + 1, "dump_fw_file") == 0 && ++i < argc &&
				sscanf (argv[i], "%s", dump_fw_file) == 1) {
				param_cnt++;
				continue;
			}
		}
	}

	if (param_cnt == 0) {
		printf("no valid param found\n");
		return -1;
	}

	printf("########## dewarp params ##########\n");
	printf("process_circle:%d win_num:%d color_mode:%d (0:YUV420_PLANAR 1:YUV420_SEMIPLANAR 2:YONLY)\n", process_circle, dewarp_params.win_num, dewarp_params.color_mode);
	printf("   tile_x_step:%d tile_y_step:%d\n", dewarp_params.tile_x_step, dewarp_params.tile_y_step);
	printf("   input_param: width(%5d) height(%5d) offset_x(%5d) offset_y(%5d) fov(%5d)\n", in->width, in->height, in->offset_x, in->offset_y, in->fov);
	printf("     out_param: width(%5d) height(%5d)\n", out->width, out->height);

	printf("    proj_param:");
	for (i = 0; i < dewarp_params.win_num; i++) {
		if (i != 0)
			printf("              :");
		printf("projection_mode(%5d) pan(%5d) tilt(%5d) rotation(%5d) zoom(%5f) strength_hor(%5f) strength_ver(%5f)\n",
			proj[i].projection_mode, proj[i].pan, proj[i].tilt,
			proj[i].rotation, proj[i].zoom, proj[i].strength_hor, proj[i].strength_ver);
	}

	printf("     clb_param:");
	for (i = 0; i < dewarp_params.win_num; i++) {
		if (i != 0)
			printf("              :");
		printf("fx(%5lf) fy(%5lf) cx(%5lf) cy(%5lf) k1(%5lf) k2(%5lf) k3(%5lf) p1(%5lf) p2(%5lf) k4(%5lf) k5(%5lf) k6(%5lf)\n",
			clb[i].fx, clb[i].fy, clb[i].cx, clb[i].cy, clb[i].k1, clb[i].k2, clb[i].k3,
			clb[i].p1, clb[i].p2, clb[i].k4, clb[i].k5, clb[i].k6);
	}

	printf("     meshin_param:");
	for (i = 0; i < dewarp_params.win_num; i++) {
		if (i != 0)
			printf("              :");
		printf("xstart(%5d) ystart(%5d) xlen(%5d) ylen(%5d) xstep(%5d) ystep(%5d) meshin_data_file(%s)\n",
			meshin[i].x_start, meshin[i].y_start, meshin[i].x_len, meshin[i].y_len,
			meshin[i].x_step, meshin[i].y_step, meshin_file[i]);
	}

	printf("       dptz_param:");
	printf("src_u(%5d)_src_v(%5d)_out_ww(%5d)_out_hh(%5d)_srB_uW(%5d)_srB_vW(%5d)_srB_ww(%5d)_srB_hh(%5d)_sc_ww(%5d)_sc_hh(%5d)\n",
		dptz_param->src_u,  dptz_param->src_v, dptz_param->out_ww, dptz_param->out_hh,
		dptz_param->srB_uW, dptz_param->srB_vW,dptz_param->srB_ww, dptz_param->srB_hh,
		dptz_param->sc_ww,  dptz_param->sc_hh);
	printf("                 :");
	printf("zoom_p(%5f)_zoom_q(%5f)_pixel_aspect_ratio_x(%5f)_pixel_aspect_ratio_y(%5f)\n",
		dptz_param->zoom_p, dptz_param->zoom_q,
		dptz_param->pixel_aspect_ratio_x, dptz_param->pixel_aspect_ratio_y);
	printf("                 :");
	printf("fx(%5f)_fy(%5f)_uC(%5f)_vC(%5f)_k1(%5f)_k2(%5f)_k3(%5f)_k4(%5f)_k5(%5f)_k6(%5f)_p1(%5f)_p2(%5f)\n",
		dptz_param->fx,dptz_param->fy, dptz_param->uC,dptz_param->vC,
		dptz_param->k1,dptz_param->k2,dptz_param->k3,dptz_param->k4,
		dptz_param->k5,dptz_param->k6,dptz_param->p1,dptz_param->p2);

	printf("     win_param:");
	for (i = 0; i < dewarp_params.win_num; i++) {
		if (i != 0)
			printf("           :");
		printf("win_start_x(%5d) win_end_x(%5d) win_start_y(%5d) win_end_y(%5d) img_start_x(%5d) img_end_x(%5d) img_start_y(%5d) img_end_y(%5d)\n",
			win[i].win_start_x, win[i].win_end_x, win[i].win_start_y, win[i].win_end_y,
			win[i].img_start_x, win[i].img_end_x, win[i].img_start_y, win[i].img_end_y);
	}
	printf("       in_file:%s out_file:%s in_bit_width:%d out_bit_width:%d\n", in_file, out_file, in_bit_width, out_bit_width);
	if (strlen(static_fw_file))
		printf("       static_fw_file:%s\n", static_fw_file);
	printf("########################################\n");
	printf("\n");
	printf("\n");

	return 0;
}

static int ion_alloc_mem(unsigned int ion_dev_fd, unsigned int alloc_bytes, int cache);
static int get_file_size(char *file_name);
static int aml_read_file(int shared_fd, const char* file_name, int read_bytes);
static int aml_write_file(int shared_fd, const char* file_name, int write_bytes);
static void ion_release_mem(int shared_fd);
static int dewarp_to_libgdc_format(int dewarp_format, int in_bit_width,
				   int out_bit_width);

int main(int argc, char** argv)
{
	int i, j;
	unsigned long stime;
	struct input_param *in   = &dewarp_params.input_param;
	struct output_param *out = &dewarp_params.output_param;

	struct gdc_usr_ctx_s ctx;
	int i_y_stride = 0;
	int i_c_stride = 0;
	int o_y_stride = 0;
	int o_c_stride = 0;
	int i_width  = 0;
	int i_height = 0;
	int o_width  = 0;
	int o_height = 0;
	int format_all;
	int format;
	int in_bit_width_flag = 0;
	int out_bit_width_flag = 0;
	int in_bit_width_val = 8;
	int out_bit_width_val = 8;
	int plane_number = 1;
	struct gdc_settings_ex *gdc_gs = NULL;
	int ret = -1;
	unsigned int i_len = 0;
	unsigned int o_len = 0;
	int in_shared_fd  = -1;
	int out_shared_fd = -1;
	int firmware_shared_fd = -1;
	int fw_len = 300 * 1024;           /* 300KB, just for test */
	int fw_bytes = 0;
	int *fw_buffer = NULL;

	ret = parse_command_line(argc, argv);
	if (ret < 0)
		return -1;

	/* for meshin_mode, give meshin_data_table to dewarp_params */
	if (dewarp_params.prm_mode == 2) {
		float tmp;

		for (i = 0; i < dewarp_params.win_num; i++) {
			FILE *fp_meshin_file = NULL;
			int data_num = dewarp_params.meshin_param[i].x_len *
					    dewarp_params.meshin_param[i].y_len * 2;

			/* read mesh data from txt, just for test*/
			fp_meshin_file = fopen(meshin_file[i],"r");
			if (fp_meshin_file == NULL) {
				printf("mesh table input file(%s) open failed!\n",
					  meshin_file[i]);
				return -1;
			}

			meshin_data_table[i] = (float *)calloc(data_num, sizeof(float));
			for (j = 0; j < data_num; j++) {
				if (fscanf(fp_meshin_file, "%f", &tmp) < 0)
					printf("read mesh error, j:%x\n", j);
				meshin_data_table[i][j]= tmp;
			}
			fclose(fp_meshin_file);

			/* give it to dewarp_params */
			dewarp_params.meshin_param[i].meshin_data_table = meshin_data_table[i];
		}
	}


	ctx.custom_fw = 0;                 /* not use builtin fw */
	ctx.mem_type = AML_GDC_MEM_ION;    /* use ION memory to test */
	ctx.plane_number = plane_number;   /* data in one continuous mem block */
	ctx.dev_type = AML_GDC;            /* dewarp */

	format_all = dewarp_to_libgdc_format(dewarp_params.color_mode,
					     in_bit_width,
					     out_bit_width);

	i_width  = in->width;
	i_height = in->height;
	o_width  = out->width;
	o_height = out->height;

	format = format_all & FORMAT_TYPE_MASK;
	in_bit_width_flag = format_all & FORMAT_IN_BITW_MASK;
	out_bit_width_flag = format_all & FORMAT_OUT_BITW_MASK;

	switch (in_bit_width_flag) {
	case IN_BITW_8:
		in_bit_width_val = 8;
		break;
	case IN_BITW_10:
		in_bit_width_val = 10;
		break;
	case IN_BITW_12:
		in_bit_width_val = 12;
		break;
	case IN_BITW_16:
		in_bit_width_val = 16;
		break;
	default:
		printf("%s, format (0x%x) in_bitw is wrong\n",
			__func__, format);
	}

	switch (out_bit_width_flag) {
	case OUT_BITW_8:
		out_bit_width_val = 8;
		break;
	case OUT_BITW_10:
		out_bit_width_val = 10;
		break;
	case OUT_BITW_12:
		out_bit_width_val = 12;
		break;
	case OUT_BITW_16:
		out_bit_width_val = 16;
		break;
	default:
		printf("%s, format (0x%x) out_bitw is wrong\n",
			__func__, format);
	}

	if (format == NV12) {
		i_y_stride = AXI_WORD_ALIGN(AXI_BYTE_ALIGN(i_width * in_bit_width_val) / 8);
		o_y_stride = AXI_WORD_ALIGN(AXI_BYTE_ALIGN(o_width * out_bit_width_val) / 8);
		i_c_stride = i_y_stride;
		o_c_stride = o_y_stride;
	} else if (format == YV12) {
		i_c_stride = AXI_WORD_ALIGN(AXI_BYTE_ALIGN(i_width * in_bit_width_val / 2) / 8);
		o_c_stride = AXI_WORD_ALIGN(AXI_BYTE_ALIGN(o_width * out_bit_width_val / 2) / 8);
		i_y_stride = i_c_stride * 2;
		o_y_stride = o_c_stride * 2;
	} else if (format == Y_GREY) {
		i_y_stride = AXI_WORD_ALIGN(AXI_BYTE_ALIGN(i_width * in_bit_width_val) / 8);
		o_y_stride = AXI_WORD_ALIGN(AXI_BYTE_ALIGN(o_width * out_bit_width_val) / 8);
		i_c_stride = 0;
		o_c_stride = 0;
	} else {
		E_GDC("Error unknow format 0x%x\n", format);
		return ret;
	}

	gdc_gs = &ctx.gs_ex;

	gdc_gs->gdc_config.input_width = i_width;
	gdc_gs->gdc_config.input_height = i_height;
	gdc_gs->gdc_config.input_y_stride = i_y_stride;
	gdc_gs->gdc_config.input_c_stride = i_c_stride;
	gdc_gs->gdc_config.output_width = o_width;
	gdc_gs->gdc_config.output_height = o_height;
	gdc_gs->gdc_config.output_y_stride = o_y_stride;
	gdc_gs->gdc_config.output_c_stride = o_c_stride;
	gdc_gs->gdc_config.format = format_all;
	gdc_gs->magic = sizeof(*gdc_gs);

	ret = gdc_create_ctx(&ctx);
	if (ret < 0)
		goto release_meshin_buf;

	if (format == NV12 || format == YV12)
		i_len = i_y_stride * i_height * 3 / 2;
	else if (format == Y_GREY)
		i_len = i_y_stride * i_height;

	if (format == NV12 || format == YV12)
		o_len = o_y_stride * o_height * 3 / 2;
	else if (format == Y_GREY)
		o_len = o_y_stride * o_height;

	/* firmware buffer */
	ret = ion_alloc_mem(ctx.ion_fd, fw_len /*bytes*/, 0);
	if (ret < 0)
		goto destroy_ctx;
	firmware_shared_fd = ret;

	/* input image buffer */
	ret = ion_alloc_mem(ctx.ion_fd, i_len /*bytes*/, 0);
	if (ret < 0)
		goto release_fw_buf;
	in_shared_fd = ret;

	/* output image buffer */
	ret = ion_alloc_mem(ctx.ion_fd, o_len /*bytes*/, 0);
	if (ret < 0)
		goto release_in_buf;
	out_shared_fd = ret;

	gdc_gs->config_buffer.plane_number = plane_number;
	gdc_gs->config_buffer.mem_alloc_type = ctx.mem_type;
	gdc_gs->config_buffer.shared_fd = firmware_shared_fd;

	gdc_gs->input_buffer.plane_number = plane_number;
	gdc_gs->input_buffer.mem_alloc_type = ctx.mem_type;
	gdc_gs->input_buffer.shared_fd = in_shared_fd;

	gdc_gs->output_buffer.plane_number = plane_number;
	gdc_gs->output_buffer.mem_alloc_type = ctx.mem_type;
	gdc_gs->output_buffer.shared_fd = out_shared_fd;

	ret = aml_read_file(in_shared_fd, in_file, i_len);
	if (ret < 0)
		goto release_out_buf;

	fw_buffer = (int *)mmap(NULL, fw_len,
			PROT_READ | PROT_WRITE, MAP_SHARED, firmware_shared_fd, 0);
	if (!fw_buffer) {
		printf("%s,%d,mmap failed,Not enough memory\n",__func__, __LINE__);
		goto release_out_buf;
	}

	/* use static or dynamic config */
	if (strlen(static_fw_file)) {
		fw_bytes = get_file_size(static_fw_file);
		ret = aml_read_file(firmware_shared_fd, static_fw_file, fw_bytes);
		if (ret < 0)
			goto release_out_buf;
	} else {
		stime = myclock();
		for (i = 0; i< process_circle; i++)
			ret = dewarp_gen_config(&dewarp_params, fw_buffer);

		printf("fw generation time=%ld ms, total FW bytes:%d\n", myclock() - stime, ret);
		fw_bytes = ret;
	}

	stime = myclock();
	for (i = 0; i< process_circle; i++) {
		ret = gdc_process(&ctx);
		if (ret < 0) {
			E_GDC("ioctl failed\n");
			goto release_out_buf;
		}
	}
	printf("driver(HW) time=%ld ms\n", myclock() - stime);

	munmap(fw_buffer, fw_len);

	if (out_file[0] != 0) {
		printf("write out file\n");
		aml_write_file(out_shared_fd, out_file, o_len);
	}
	if (dump_fw_file[0] != 0) {
		printf("write firmware file\n");
		aml_write_file(firmware_shared_fd, dump_fw_file, fw_bytes);
	}

release_out_buf:
	ion_release_mem(out_shared_fd);
release_in_buf:
	ion_release_mem(in_shared_fd);
release_fw_buf:
	ion_release_mem(firmware_shared_fd);
destroy_ctx:
	gdc_destroy_ctx(&ctx);
release_meshin_buf:
	if (dewarp_params.prm_mode == 2) {
		for (i = 0; i < dewarp_params.win_num; i++)
		free(meshin_data_table[i]);
	}

	return ret;
}

static int ion_alloc_mem(unsigned int ion_dev_fd, unsigned int alloc_bytes, int cache)
{
	int ret = -1;
	IONMEM_AllocParams ion_alloc_param;

	if (!ion_dev_fd || !alloc_bytes) {
		printf("ion alloc failed\n");
		return -1;
	}
	cache = cache ? true : false;
	ret = ion_mem_alloc(ion_dev_fd, alloc_bytes, &ion_alloc_param, cache);
	if (ret < 0) {
		printf("%s,%d,Not enough memory\n",__func__, __LINE__);
		return -1;
	}

	return ion_alloc_param.mImageFd;
}

static int get_file_size(char *file_name)
{
	int f_size = -1;
	FILE *fp = NULL;

	if (file_name == NULL) {
		E_GDC("Error file name\n");
		return f_size;
	}

	fp = fopen(file_name, "rb");
	if (fp == NULL) {
		E_GDC("Error open file %s\n", file_name);
		return f_size;
	}

	fseek(fp, 0, SEEK_END);

	f_size = ftell(fp);

	fclose(fp);

	D_GDC("%s: size %d\n", file_name, f_size);

	return f_size;
}

static int aml_read_file(int shared_fd, const char* file_name, int read_bytes)
{
	int fd = -1;
	int read_num = 0;
	char *vaddr = NULL;

	if (shared_fd < 0 || !file_name || !read_bytes) {
		printf("wrong params, read faild");
		return -1;
	}
	vaddr = (char*)mmap(NULL, read_bytes,
			PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
	if (!vaddr) {
		printf("%s,%d,mmap failed,Not enough memory\n",__func__, __LINE__);
		return -1;
	}

	fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		printf("read source file:%s open error\n", file_name);
		return -1;
	}

	read_num = read(fd, vaddr, read_bytes);
	if (read_num <= 0) {
		printf("read file read_num=%d error\n",read_num);
		close(fd);
		return -1;
	}

	close(fd);
	munmap(vaddr, read_bytes);

	return 0;
}

static int aml_write_file(int shared_fd, const char* file_name, int write_bytes)
{
	int fd = -1;
	int write_num = 0;
	char *vaddr = NULL;

	if (shared_fd < 0 || !file_name || !write_bytes) {
		printf("wrong params, read faild");
		return -1;
	}

	vaddr = (char*)mmap(NULL, write_bytes,
			PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
	if (!vaddr) {
		printf("%s,%d,mmap failed,Not enough memory\n",__func__, __LINE__);
		return -1;
	}

	fd = open(file_name, O_RDWR | O_CREAT, 0660);
	if (fd < 0) {
		printf("write file:%s open error\n", file_name);
		return -1;
	}

	write_num = write(fd, vaddr, write_bytes);
	if (write_num <= 0) {
		printf("write file write_num=%d error\n", write_num);
		close(fd);
	}

	printf("save image 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		((char *)vaddr)[0], ((char *)vaddr)[1], ((char *)vaddr)[2], ((char *)vaddr)[3],
		((char *)vaddr)[4], ((char *)vaddr)[5], ((char *)vaddr)[6], ((char *)vaddr)[7]);
	close(fd);
	munmap(vaddr, write_bytes);

	return 0;
}

static void ion_release_mem(int shared_fd)
{
	close(shared_fd);
}

static int dewarp_to_libgdc_format(int dewarp_format, int in_bit_width,
				   int out_bit_width)
{
	int ret = 0;

	switch (dewarp_format) {
	case YUV420_PLANAR:
		ret = YV12;
		break;
	case YUV420_SEMIPLANAR:
		ret = NV12;
		break;
	case YONLY:
		ret = Y_GREY;
		break;
	default:
		printf("format(%d) is wrong\n", dewarp_format);
		break;
	}

	switch (in_bit_width) {
	case 0:
		ret |= IN_BITW_8;
		break;
	case 1:
		ret |= IN_BITW_10;
		break;
	case 2:
		ret |= IN_BITW_12;
		break;
	case 3:
		ret |= IN_BITW_16;
		break;
	default:
		printf("int bit_width(%d) is wrong\n", in_bit_width);
		break;
	}

	switch (out_bit_width) {
	case 0:
		ret |= OUT_BITW_8;
		break;
	case 1:
		ret |= OUT_BITW_10;
		break;
	case 2:
		ret |= OUT_BITW_12;
		break;
	case 3:
		ret |= OUT_BITW_16;
		break;
	default:
		printf("out bit_width(%d) is wrong\n", out_bit_width);
		break;
	}

	return ret;
}


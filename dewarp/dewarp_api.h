/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef DEWARP_API_H_
#define DEWARP_API_H_

#if defined (__cplusplus)
extern "C" {
#endif

#define WIN_MAX 4

struct input_param {
    int width;
    int height;
    int offset_x;
    int offset_y;
    int fov;
};

struct output_param {
    int width;
    int height;
};

struct proj_param {
    int projection_mode;
    int pan;
    int tilt;
    int rotation;
    float zoom;
    float strength_hor;
    float strength_ver;
};

struct win_param {
    int win_start_x;
    int win_end_x;
    int win_start_y;
    int win_end_y;
    int img_start_x;
    int img_end_x;
    int img_start_y;
    int img_end_y;
    int mesh_x_len;
    int mesh_y_len;
};

struct clb_param {
    double fx;
    double fy;
    double cx;
    double cy;
    double k1;
    double k2;
    double k3;
    double p1;
    double p2;
    double k4;
    double k5;
    double k6;
};

struct meshin_param {
    int x_start;
    int y_start;
    int x_len;
    int y_len;
    int x_step;
    int y_step;
    float *meshin_data_table;
};

struct dptz_param {
    int   src_u,  src_v;                    /* Dewarped. Includes zoom factor */
    int   out_ww, out_hh;                   /* Dewarped. Includes zoom factor */
    int   srB_uW, srB_vW;                   /* Warped. Includes borders */
    int   srB_ww, srB_hh;                   /* Warped. Includes borders. pre-zoom */
    int   sc_ww,  sc_hh;                    /* Warped. Includes borders. post-zoom (actual outputimage buffer) */
    float zoom_p, zoom_q;                   /* zoom factor.  src_width/out_width  src_height/out_height */
    float pixel_aspect_ratio_x, pixel_aspect_ratio_y;

    /* these are static calibration parameters: */
    float fx,fy;                            /* sensor GDC parameters (intrinsic focal length in pixels) */
    float uC,vC;                            /* sensor GDC parameters (intrinsic image center in pixels) */
    float k1,k2,k3,k4,k5,k6,p1,p2;          /* sensor GDC parameters; */
};

struct proc_param {
    int intrp_mode;
    int replace_0;
    int replace_1;
    int replace_2;
    int edge_0;
    int edge_1;
    int edge_2;
};

struct dewarp_params {
    int win_num;
    struct input_param input_param;
    int color_mode;
    struct output_param output_param;
    struct proj_param proj_param[WIN_MAX];
    struct clb_param clb_param[WIN_MAX];
    struct meshin_param meshin_param[WIN_MAX];
    struct dptz_param dptz_param;
    struct win_param win_param[WIN_MAX];
    int prm_mode; /* 0. use proj_param, 1.use clb_param, 2. meshin mode 3. use dptz_param */
    int tile_x_step;
    int tile_y_step;
    struct proc_param proc_param;
};

typedef enum _dw_proj_mode_ {
    PROJ_MODE_EQUISOLID = 0,
    PROJ_MODE_EQUIDISTANCE,
    PROJ_MODE_STEREOGRAPHIC,
    PROJ_MODE_ORTHOGONAL,
    PROJ_MODE_LINEAR
} dw_proj_mode_t;

typedef enum _data_mode_ {
    YUV420_PLANAR = 0,
    YUV420_SEMIPLANAR,
    YONLY,
    DEWARP_COLOR_MODE_MAX
} data_mode_t;

/* Description  : generate FW data to fw_buffer
 * Params       : dewarp_params and fw_buffer vaddr
 * Returen      : success -- total bytes of FW
 *                   fail -- minus
 */
int dewarp_gen_config(struct dewarp_params *dewarp_params, int *fw_buffer);

#if defined (__cplusplus)
}
#endif

#endif

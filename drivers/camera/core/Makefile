ccflags-y += -I$(srctree)/drivers/camera/common
ccflags-y += -I$(srctree)/drivers/camera/interface
ccflags-y += -I$(srctree)/drivers/camera/sensor
ccflags-y += -I$(srctree)/drivers/camera/sensor/csi2/sprd/receiver_r3p0
ccflags-y += -I$(srctree)/drivers/camera/flash/flash_drv
ccflags-y += -I$(srctree)/drivers/camera/core/isp2.6/inc
ccflags-y += -I$(srctree)/drivers/staging/android/ion
ccflags-y += -I$(srctree)/drivers/camera/core/isp2.6/adpt/sharkl5pro/inc
ccflags-y += -I$(srctree)/drivers/camera/core/isp2.6/test/
ccflags-y += -DPROJ_SHARKL5PRO

obj-$(CONFIG_SPRD_CAMERA_CORE) := sprd_camera.o
sprd_camera-objs := isp2.6/src/cam_block.o isp2.6/src/cam_buf.o isp2.6/src/cam_core.o isp2.6/src/cam_debugger.o isp2.6/src/cam_ioctl.o isp2.6/src/cam_queue.o isp2.6/src/cam_scaler.o isp2.6/src/cam_trusty.o
sprd_camera-objs += isp2.6/src/dcam_core.o isp2.6/src/dcam_drv.o isp2.6/src/dcam_path.o isp2.6/src/flash_core.o isp2.6/src/isp_3dnr.o isp2.6/src/isp_cfg.o isp2.6/src/isp_core.o isp2.6/src/isp_drv.o isp2.6/src/isp_fmcu.o
sprd_camera-objs += isp2.6/src/isp_ltm.o isp2.6/src/isp_path.o isp2.6/src/isp_slice.o
sprd_camera-objs += isp2.6/adpt/sharkl5pro/block/blk_comm.o isp2.6/adpt/sharkl5pro/block/dcam_k_3dnr_me.o isp2.6/adpt/sharkl5pro/block/dcam_k_aem.o isp2.6/adpt/sharkl5pro/block/dcam_k_afm.o
sprd_camera-objs += isp2.6/adpt/sharkl5pro/block/dcam_k_anti_flicker.o isp2.6/adpt/sharkl5pro/block/dcam_k_awbc.o isp2.6/adpt/sharkl5pro/block/dcam_k_bayerhist.o isp2.6/adpt/sharkl5pro/block/dcam_k_blc.o
sprd_camera-objs += isp2.6/adpt/sharkl5pro/block/dcam_k_bpc.o isp2.6/adpt/sharkl5pro/block/dcam_k_gain.o isp2.6/adpt/sharkl5pro/block/dcam_k_lsc.o isp2.6/adpt/sharkl5pro/block/dcam_k_lscm.o
sprd_camera-objs += isp2.6/adpt/sharkl5pro/block/dcam_k_pdaf.o isp2.6/adpt/sharkl5pro/block/dcam_k_raw_gtm.o isp2.6/adpt/sharkl5pro/block/isp_k_3dnr.o isp2.6/adpt/sharkl5pro/block/isp_k_bchs.o
sprd_camera-objs += isp2.6/adpt/sharkl5pro/block/isp_k_cce.o isp2.6/adpt/sharkl5pro/block/isp_k_cdn.o isp2.6/adpt/sharkl5pro/block/isp_k_cfa.o isp2.6/adpt/sharkl5pro/block/isp_k_cmc10.o
sprd_camera-objs += isp2.6/adpt/sharkl5pro/block/isp_k_edge.o isp2.6/adpt/sharkl5pro/block/isp_k_gamma.o isp2.6/adpt/sharkl5pro/block/isp_k_grgb.o isp2.6/adpt/sharkl5pro/block/isp_k_hist2.o
sprd_camera-objs += isp2.6/adpt/sharkl5pro/block/isp_k_hsv.o isp2.6/adpt/sharkl5pro/block/isp_k_iircnr.o isp2.6/adpt/sharkl5pro/block/isp_k_ltm.o isp2.6/adpt/sharkl5pro/block/isp_k_nlm.o
sprd_camera-objs += isp2.6/adpt/sharkl5pro/block/isp_k_post_cdn.o isp2.6/adpt/sharkl5pro/block/isp_k_pre_cdn.o isp2.6/adpt/sharkl5pro/block/isp_k_pstrz.o isp2.6/adpt/sharkl5pro/block/isp_k_uvd.o
sprd_camera-objs += isp2.6/adpt/sharkl5pro/block/isp_k_ygamma.o isp2.6/adpt/sharkl5pro/block/isp_k_ynr.o isp2.6/adpt/sharkl5pro/block/isp_k_yrandom.o isp2.6/adpt/sharkl5pro/block/isp_k_yuv_noisefilter.o
sprd_camera-objs += isp2.6/adpt/sharkl5pro/cam_hw_if.o isp2.6/adpt/sharkl5pro/dcam_hw.o isp2.6/adpt/sharkl5pro/dcam_int.o isp2.6/adpt/sharkl5pro/isp_hw.o isp2.6/adpt/sharkl5pro/isp_int.o
sprd_camera-objs += isp2.6/test/cam_test.o isp2.6/test/dcam_test.o isp2.6/test/isp_test.o
ifeq ($(CONFIG_COMPAT),y)
sprd_camera-objs += isp2.6/compat/compat_cam_drv.o
endif

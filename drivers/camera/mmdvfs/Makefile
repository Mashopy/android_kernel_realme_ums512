ccflags-y += -I$(srctree)/drivers/camera/mmdvfs/r1p0/mm_dvfs_devfreq
ccflags-y += -I$(srctree)/drivers/camera/mmdvfs/r1p0/mmsys_comm
ccflags-y += -I$(srctree)/drivers/camera/mmdvfs/r1p0/dvfs_driver/dvfs_driver_common
ccflags-y += -I$(srctree)/drivers/camera/mmdvfs/r1p0/dvfs_driver/dvfs_reg_param/sharkl5pro
ccflags-y += -I$(srctree)/drivers/camera/common
ccflags-y += -I$(srctree)/drivers/devfreq

ifeq ($(TEST_ON_HAPS),true)
ccflags-y += -DTEST_ON_HAPS
endif

obj-$(CONFIG_SPRD_CAMERA_MMDVFS) += mmdvfs.o
mmdvfs-objs := r1p0/mm_dvfs_devfreq/cpp_dvfs_governor.o r1p0/mm_dvfs_devfreq/dcam_axi_dvfs_governor.o r1p0/mm_dvfs_devfreq/dcam_if_dvfs_governor.o r1p0/mm_dvfs_devfreq/fd_dvfs_governor.o
mmdvfs-objs += r1p0/mm_dvfs_devfreq/isp_dvfs_governor.o r1p0/mm_dvfs_devfreq/jpg_dvfs_governor.o r1p0/mm_dvfs_devfreq/mmsys_dvfs_devfreq.o r1p0/mm_dvfs_devfreq/mmsys_dvfs_governor.o r1p0/mm_dvfs_devfreq/mtx_dvfs_governor.o
mmdvfs-objs += r1p0/mmsys_comm/mm_devfreq_common.o r1p0/mmsys_comm/mm_dvfs_queue.o r1p0/mmsys_comm/mmsys_dvfs_comm.o
mmdvfs-objs += r1p0/dvfs_driver/dvfs_driver_common/cpp_dvfs.o r1p0/dvfs_driver/dvfs_driver_common/dcam_axi_dvfs.o r1p0/dvfs_driver/dvfs_driver_common/dcam_if_dvfs.o r1p0/dvfs_driver/dvfs_driver_common/fd_dvfs.o
mmdvfs-objs += r1p0/dvfs_driver/dvfs_driver_common/isp_dvfs.o r1p0/dvfs_driver/dvfs_driver_common/jpg_dvfs.o r1p0/dvfs_driver/dvfs_driver_common/mmsys_dvfs.o r1p0/dvfs_driver/dvfs_driver_common/mtx_dvfs.o
mmdvfs-objs += r1p0/dvfs_driver/dvfs_reg_param/sharkl5pro/mm_dvfs_table.o
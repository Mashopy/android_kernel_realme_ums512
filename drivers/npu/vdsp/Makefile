ccflags-y += -DDEBUG
ccflags-y += -DKERNEL_VERSION_414
ccflags-y += -I$(srctree)/drivers/staging/android/ion
ccflags-y += -I$(srctree)/drivers/devfreq/apsys/
ccflags-y += -I$(srctree)/drivers/devfreq/apsys/vdsp
ccflags-y += -I$(srctree)/drivers/npu/vdsp/common
ccflags-y += -I$(srctree)/drivers/npu/vdsp/Cadence/xrp

ifeq ($(CONFIG_COMPAT),y)
ccflags-y += -DCONFIG_COMPAT
endif

ccflags-y += -DUNSUPPORT_FACEID

ifeq ($(TEST_ON_HAPS),true)
ccflags-y += -DTEST_ON_HAPS
endif

obj-$(CONFIG_NPU_VDSP) += sprd_vdsp.o
sprd_vdsp-objs := Cadence/xrp/common.o Cadence/xrp/pi_library_load.o
sprd_vdsp-objs += Cadence/xrp/vdsp_dump.o Cadence/xrp/vdsp_dvfs.o Cadence/xrp/vdsp_hw.o Cadence/xrp/vdsp_ipi_drv.o
sprd_vdsp-objs += Cadence/xrp/vdsp_log.o Cadence/xrp/vdsp_smem.o Cadence/xrp/vdsp_trusty.o
sprd_vdsp-objs += Cadence/xrp/xrp_faceid.o Cadence/xrp/xrp_firmware.o Cadence/xrp/xrp_library_loader.o
sprd_vdsp-objs += Cadence/xrp/xvp_main.o

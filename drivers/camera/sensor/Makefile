ccflags-y += -I$(srctree)/drivers/camera/common
ccflags-y += -I$(srctree)/drivers/camera/interface
ccflags-y += -I$(srctree)/drivers/camera/sensor/csi2/sprd/receiver_r3p0

obj-$(CONFIG_SPRD_CAMERA_SENSOR) += sprd_sensor.o
sprd_sensor-objs := sprd_sensor_core.o sprd_sensor_drv.o
sprd_sensor-objs += csi2/sprd/receiver_r3p0/csi_api.o csi2/sprd/receiver_r3p0/csi_driver.o
ifeq ($(CONFIG_COMPAT),y)
sprd_sensor-objs += compat_sensor_drv.o
endif

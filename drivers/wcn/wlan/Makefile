ccflags-y += -DSC2355_FTR -DSC2355_TX_FILTER -DSC2355_QOS_MAP
ccflags-y += -DIBSS_SUPPORT -DIBSS_RSN_SUPPORT
ccflags-y += -DNAN_SUPPORT
ccflags-y += -DRTT_SUPPORT
ccflags-y += -DSC2355_RX_NO_LOOP
ccflags-y += -DACS_SUPPORT -DSC2355_HW_CSUM
ccflags-y += -DWMMAC_WFA_CERTIFICATION
ccflags-y += -DCOMPAT_SAMPILE_CODE
ccflags-y += -DRND_MAC_SUPPORT
ccflags-y += -DSOFTAP_HOOK
ccflags-y += -DATCMD_ASSERT
ccflags-y += -DTCPACK_DELAY_SUPPORT
ifeq ($(CONFIG_WCN_WLAN_DEBUG),y)
ccflags-y += -DCONFIG_SPRD_WCN_DEBUG
endif
ccflags-y += -DSPLIT_STACK

obj-$(CONFIG_WCN_WLAN) += sprdwl_ng.o
sprdwl_ng-objs := sc2355/api_version.o sc2355/cfg80211.o sc2355/cmdevt.o sc2355/core_sc2355.o
sprdwl_ng-objs += sc2355/debug.o sc2355/defrag.o sc2355/ibss.o sc2355/if_sc2355.o
sprdwl_ng-objs += sc2355/main.o sc2355/mm.o sc2355/msg.o sc2355/nan.o
sprdwl_ng-objs += sc2355/npi.o sc2355/qos.o sc2355/reorder.o sc2355/rf_marlin3.o
sprdwl_ng-objs += sc2355/rnd_mac_addr.o sc2355/rtt.o sc2355/rx_msg_sc2355.o sc2355/softap_hook.o
sprdwl_ng-objs += sc2355/tcp_ack.o sc2355/tracer.o sc2355/tx_msg_sc2355.o sc2355/txrx.o
sprdwl_ng-objs += sc2355/vendor.o sc2355/work.o

/*
 * Copyright (C) 2020-2021 UNISOC Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <sprd_mm.h>
#include <video/sprd_mmsys_pw_domain.h>

#include "cpp_common.h"
#include "cpp_reg.h"
#include "sprd_cpp.h"
#include "sprd_img.h"
#include "dma_drv.h"
#include "rot_drv.h"
#include "scale_drv.h"
#include "cpp_core.h"


#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "CPP_CORE: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

#define CPP_DEVICE_NAME             "sprd_cpp"
#define ROT_TIMEOUT                 5000
#define SCALE_TIMEOUT               5000
#define DMA_TIMEOUT                 5000
#define CPP_IRQ_LINE_MASK           CPP_PATH_DONE
#define CPP_MMU_IRQ_LINE_MASK       CPP_MMU_ERROR_INT
#define ROT_DRV_DEBUG 0

unsigned long g_cpp_base;

enum cpp_irq_id {
	CPP_SCALE_DONE = 0,
	CPP_ROT_DONE,
	CPP_DMA_DONE,
	CPP_IRQ_NUMBER
};

struct dmaif_device {
	atomic_t count;
	struct mutex dma_mutex;
	struct completion done_com;
	struct dma_drv_private drv_priv;
};

struct rotif_device {
	atomic_t count;
	struct mutex rot_mutex;
	struct completion done_com;
	struct rot_drv_private drv_priv;
};

struct scif_device {
	atomic_t count;
	struct mutex sc_mutex;
	struct completion done_com;
	struct scale_drv_private drv_priv;
};

typedef void (*cpp_isr_func) (void *);

struct cpp_device {
	atomic_t users;
	atomic_t users_clk;
	spinlock_t hw_lock;
	struct mutex lock;
	spinlock_t slock;

	struct rotif_device *rotif;
	struct scif_device *scif;
	struct dmaif_device *dmaif;
	struct miscdevice md;

	unsigned int irq;
	cpp_isr_func isr_func[CPP_IRQ_NUMBER];
	void *isr_data[CPP_IRQ_NUMBER];

	void __iomem *io_base;

	struct platform_device *pdev;

	struct clk *cpp_clk;
	struct clk *cpp_clk_parent;
	struct clk *cpp_clk_default;
	struct clk *cpp_eb;

	struct regmap *mm_ahb_gpr;
	uint32_t awrqos;
};

typedef void (*cpp_isr) (struct cpp_device *dev);

static void sprd_cppcore_scale_done(struct cpp_device *dev)
{
	cpp_isr_func user_func = dev->isr_func[CPP_SCALE_DONE];
	void *priv = dev->isr_data[CPP_SCALE_DONE];

	if (user_func)
		(*user_func) (priv);
}

static void sprd_cppcore_rot_done(struct cpp_device *dev)
{
	cpp_isr_func user_func = dev->isr_func[CPP_ROT_DONE];
	void *priv = dev->isr_data[CPP_ROT_DONE];

	if (user_func)
		(*user_func) (priv);
}

static void sprd_cppcore_dma_done(struct cpp_device *dev)
{
	cpp_isr_func user_func = dev->isr_func[CPP_DMA_DONE];
	void *priv = dev->isr_data[CPP_DMA_DONE];

	if (user_func)
		(*user_func) (priv);
}

static const cpp_isr cpp_isr_list[CPP_IRQ_NUMBER] = {
	sprd_cppcore_scale_done,
	sprd_cppcore_rot_done,
	sprd_cppcore_dma_done,
};

#ifndef TEST_ON_HAPS
static void sprd_cppcore_qos_set(
		uint32_t qos)
{
	CPP_REG_MWR(CPP_AXIM_CHN_SET,
			CPP_AXIM_CHN_SET_QOS_MASK,
			qos);
}
#endif

static void sprd_cppcore_module_reset(struct cpp_device *dev)
{
	if (!dev) {
		pr_err("fail to get valid input ptr\n");
		return;
	}

	regmap_update_bits(dev->mm_ahb_gpr, MM_AHB_RESET,
			CPP_PATH_RESET_MASK,
			(unsigned int)CPP_PATH_RESET_MASK);
	udelay(2);
	regmap_update_bits(dev->mm_ahb_gpr, MM_AHB_RESET,
			CPP_PATH_RESET_MASK,
			~(unsigned int)CPP_PATH_RESET_MASK);
}

static void sprd_cppcore_scale_reset(
	struct cpp_device *dev)
{
	if (!dev) {
		pr_err("fail to get valid input ptr\n");
		return;
	}

	regmap_update_bits(dev->mm_ahb_gpr, (unsigned int)MM_AHB_RESET,
			(unsigned int)CPP_PATH0_AHB_RESET_BIT,
			(unsigned int)CPP_PATH0_AHB_RESET_BIT);
	udelay(2);
	regmap_update_bits(dev->mm_ahb_gpr, (unsigned int)MM_AHB_RESET,
			(unsigned int)CPP_PATH0_AHB_RESET_BIT,
			~(unsigned int)CPP_PATH0_AHB_RESET_BIT);
}

static void sprd_cppcore_rot_reset(struct cpp_device *dev)
{
	if (!dev) {
		pr_err("fail to get valid input ptr\n");
		return;
	}

	regmap_update_bits(dev->mm_ahb_gpr, MM_AHB_RESET,
			CPP_PATH1_AHB_RESET_BIT,
			(unsigned int)CPP_PATH1_AHB_RESET_BIT);
	udelay(2);
	regmap_update_bits(dev->mm_ahb_gpr, MM_AHB_RESET,
			CPP_PATH1_AHB_RESET_BIT,
			~(unsigned int)CPP_PATH1_AHB_RESET_BIT);
}

static void sprd_cppcore_dma_reset(struct cpp_device *dev)
{
	if (!dev) {
		pr_err("fail to get valid input ptr\n");
		return;
	}

	regmap_update_bits(dev->mm_ahb_gpr, MM_AHB_RESET,
			CPP_DMA_AHB_RESET_BIT,
			(unsigned int)CPP_DMA_AHB_RESET_BIT);
	udelay(2);
	regmap_update_bits(dev->mm_ahb_gpr, MM_AHB_RESET,
			CPP_DMA_AHB_RESET_BIT,
			~(unsigned int)CPP_DMA_AHB_RESET_BIT);
}

static void sprd_cppcore_iommu_reg_trace(struct cpp_device *dev)
{
	unsigned long addr = 0;

	if (!dev) {
		pr_err("fail to get valid input ptr\n");
		return;
	}

	pr_err("fail to CPP IOMMU INT ERROR:register list\n");
	for (addr = 0x200; addr <= 0x264 ; addr += 16) {
		pr_err("0x%lx: 0x%x 0x%x 0x%x 0x%x\n", addr,
			CPP_REG_RD(addr), CPP_REG_RD(addr + 4),
			CPP_REG_RD(addr + 8), CPP_REG_RD(addr + 12));
	}
}

static int sprd_cppcore_iommu_err_pre_proc(struct cpp_device *dev)
{
	if (!dev) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	sprd_cppcore_iommu_reg_trace(dev);

	return 0;
}

static irqreturn_t sprd_cppcore_isr_root(int irq, void *priv)
{
	int i = 0;
	unsigned int path_irq_line = 0;
	unsigned int status = 0;
	unsigned int mmu_irq_line = 0;
	unsigned long flag = 0;
	struct cpp_device *dev = NULL;

	if (!priv) {
		pr_err("fail to get valid input ptr\n");
		return -EINVAL;
	}

	dev = (struct cpp_device *)priv;
	status = CPP_REG_RD(CPP_INT_STS);
	mmu_irq_line = status & CPP_MMU_IRQ_LINE_MASK;
	if (unlikely(mmu_irq_line != 0)) {
		pr_err("fail to run iommu, int 0x%x\n", mmu_irq_line);
		if (sprd_cppcore_iommu_err_pre_proc(dev))
			return IRQ_HANDLED;
	}
	path_irq_line = status & CPP_IRQ_LINE_MASK;
	if (unlikely(path_irq_line == 0))
		return IRQ_NONE;
	CPP_REG_WR(CPP_INT_CLR, status);

	spin_lock_irqsave(&dev->slock, flag);
	for (i = 0; i < CPP_IRQ_NUMBER; i++) {
		if (path_irq_line & (1 << (unsigned int)i)) {
			if (cpp_isr_list[i])
				cpp_isr_list[i](dev);
		}
		path_irq_line &= ~(unsigned int)(1 << (unsigned int)i);
		if (!path_irq_line)
			break;
	}
	spin_unlock_irqrestore(&dev->slock, flag);

	return IRQ_HANDLED;
}

static int sprd_cppcore_sc_reg_trace(struct scale_drv_private *p)
{
#ifdef SCALE_DRV_DEBUG
	unsigned long addr = 0;

	if (!p) {
		pr_err("fail to get valid input ptr\n");
		return -EINVAL;
	}

	CPP_TRACE("CPP:Scaler Register list\n");
	for (addr = CPP_PATH_EB; addr <= CPP_PATH0_BP_YUV_REGULATE_2;
		addr += 16) {
		CPP_TRACE("0x%lx: 0x%8x 0x%8x 0x%8x 0x%8x\n", addr,
			CPP_REG_RD(addr), CPP_REG_RD(addr + 4),
			CPP_REG_RD(addr + 8), CPP_REG_RD(addr + 12));
	}
#endif

	return 0;
}

static int sprd_cppcore_rot_reg_trace(struct rot_drv_private *p)
{
#ifdef ROT_DRV_DEBUG
	unsigned long addr = 0;

	if (!p) {
		pr_err("fail to get valid input ptr\n");
		return -EINVAL;
	}

	CPP_TRACE("CPP:Rotation Register list");
	for (addr = CPP_ROTATION_SRC_ADDR; addr <= CPP_ROTATION_PATH_CFG;
		addr += 16) {
		CPP_TRACE("0x%lx: 0x%8x 0x%8x 0x%8x 0x%8x\n", addr,
			CPP_REG_RD(addr), CPP_REG_RD(addr + 4),
			CPP_REG_RD(addr + 8), CPP_REG_RD(addr + 12));
	}
#endif

	return 0;
}

static int sprd_cppcore_dma_reg_trace(struct dma_drv_private *p)
{
#ifdef DMA_DRV_DEBUG
	unsigned long addr = 0;

	if (!p) {
		pr_err("fail to get valid input ptr\n");
		return -EINVAL;
	}

	CPP_TRACE("CPP:Dma Register list");
	for (addr = CPP_DMA_SRC_ADDR; addr <= CPP_DMA_CFG; addr += 16) {
		CPP_TRACE("0x%lx: 0x%8x 0x%8x 0x%8x 0x%8x\n", addr,
			CPP_REG_RD(addr), CPP_REG_RD(addr + 4),
			CPP_REG_RD(addr + 8), CPP_REG_RD(addr + 12));
	}
#endif

	return 0;
}

static int sprd_cppcore_module_enable(struct cpp_device *dev)
{
	int ret = 0;

	if (!dev) {
		pr_err("fail to get valid input ptr\n");
		return -1;
	}
	mutex_lock(&dev->lock);
#ifndef TEST_ON_HAPS
	ret = clk_prepare_enable(dev->cpp_eb);
	if (ret) {
		pr_err("fail to enable cpp eb\n");
		goto fail;
	}

	ret = clk_set_parent(dev->cpp_clk, dev->cpp_clk_parent);
	if (ret) {
		pr_err("fail to set cpp clk\n");
		clk_disable_unprepare(dev->cpp_eb);
		goto fail;
	}

	ret = clk_prepare_enable(dev->cpp_clk);
	if (ret) {
		pr_err("fail to enable cpp clk\n");
		clk_disable_unprepare(dev->cpp_eb);
		goto fail;
	}
#endif
	sprd_cppcore_module_reset(dev);
#ifndef TEST_ON_HAPS
	sprd_cppcore_qos_set(dev->awrqos);
#endif

	CPP_REG_AWR(CPP_MMU_EN, (0xfffffffe));
	CPP_REG_OWR(MMU_PPN_RANGE1, (0xfff));
	CPP_REG_OWR(MMU_PPN_RANGE2, (0xfff));
#ifndef TEST_ON_HAPS
fail:
#endif
	mutex_unlock(&dev->lock);

	return ret;
}

static void sprd_cppcore_module_disable(struct cpp_device *dev)
{
	if (!dev) {
		pr_err("fail to get valid input ptr\n");
		return;
	}
	mutex_lock(&dev->lock);
#ifndef TEST_ON_HAPS
	clk_set_parent(dev->cpp_clk, dev->cpp_clk_default);
	clk_disable_unprepare(dev->cpp_clk);
	clk_disable_unprepare(dev->cpp_eb);
	sprd_cam_domain_disable();
	sprd_cam_pw_off();
#endif
	mutex_unlock(&dev->lock);
}

static void sprd_cppcore_register_isr(struct cpp_device *dev,
	enum cpp_irq_id id, cpp_isr_func user_func, void *priv)
{
	unsigned long flag = 0;

	if (!dev) {
		pr_err("fail to get valid input ptr\n");
		return;
	}

	if (id < CPP_IRQ_NUMBER) {
		spin_lock_irqsave(&dev->slock, flag);
		dev->isr_func[id] = user_func;
		dev->isr_data[id] = priv;
		if (user_func)
			CPP_REG_MWR(CPP_INT_MASK, (1 << id), ~(1 << id));
		else
			CPP_REG_MWR(CPP_INT_MASK, (1 << id), (1 << id));
		spin_unlock_irqrestore(&dev->slock, flag);
	} else {
		pr_err("fail to get valid cpp isr irq\n");
	}
}

static void sprd_cppcore_rot_isr(void *priv)
{
	struct rotif_device *rotif = (struct rotif_device *)priv;

	if (!rotif) {
		pr_err("fail to get valid input ptr\n");
		return;
	}

	complete(&rotif->done_com);
}

static void sprd_cppcore_scale_isr(void *priv)
{
	struct scif_device *scif = (struct scif_device *)priv;

	if (!scif) {
		pr_err("fail to get valid input ptr\n");
		return;
	}

	complete(&scif->done_com);
}

static void sprd_cppcore_dma_isr(void *priv)
{
	struct dmaif_device *dmaif = (struct dmaif_device *)priv;

	if (!dmaif) {
		pr_err("fail to get valid input ptr\n");
		return;
	}

	complete(&dmaif->done_com);
}

int sprd_cpp_core_get_sg_table(struct cpp_iommu_info *pfinfo)
{
	int i = 0, ret = 0;

	for (i = 0; i < 2; i++) {
		if (pfinfo->mfd[i] > 0) {
			ret = sprd_ion_get_buffer(pfinfo->mfd[i], NULL,
				&pfinfo->buf[i], &pfinfo->size[i]);
			if (ret) {
				pr_err("fail to get sg table\n");
				return -EFAULT;
			}
		}
	}

	return 0;
}

int sprd_cpp_core_get_addr(struct cpp_iommu_info *pfinfo)
{
	int i = 0, ret = 0;
	struct sprd_iommu_map_data iommu_data;

	memset(&iommu_data, 0x00, sizeof(iommu_data));

	for (i = 0; i < 2; i++) {
		if (pfinfo->size[i] <= 0)
			continue;

		if (sprd_iommu_attach_device(pfinfo->dev) == 0) {
			memset(&iommu_data, 0x00, sizeof(iommu_data));
			iommu_data.buf = pfinfo->buf[i];
			iommu_data.iova_size = pfinfo->size[i];
			iommu_data.ch_type = SPRD_IOMMU_FM_CH_RW;
			iommu_data.sg_offset = pfinfo->offset[i];

			ret = sprd_iommu_map(pfinfo->dev, &iommu_data);
			if (ret) {
				pr_err("fail to get iommu kaddr %d\n", i);
				return -EFAULT;
			}

			pfinfo->iova[i] = iommu_data.iova_addr
					+ pfinfo->offset[i];
		} else {
			ret = sprd_ion_get_phys_addr(pfinfo->mfd[i], NULL,
				&pfinfo->iova[i], &pfinfo->size[i]);
			if (ret) {
				pr_err("fail to get iommu phy addr\n");
				pr_err("index:%d mfd:0x%x\n",
					i, pfinfo->mfd[i]);
				return -EFAULT;
			}
			pfinfo->iova[i] += pfinfo->offset[i];
		}
	}

	return 0;
}

int sprd_cpp_core_free_addr(struct cpp_iommu_info *pfinfo)
{
	int i, ret;
	struct sprd_iommu_unmap_data iommu_data;

	memset(&iommu_data, 0x00, sizeof(iommu_data));

	for (i = 0; i < 2; i++) {
		if (pfinfo->size[i] <= 0)
			continue;

		if (sprd_iommu_attach_device(pfinfo->dev) == 0) {
			iommu_data.iova_addr = pfinfo->iova[i]
					- pfinfo->offset[i];
			iommu_data.iova_size = pfinfo->size[i];
			iommu_data.ch_type = SPRD_IOMMU_FM_CH_RW;
			iommu_data.buf = NULL;

			ret = sprd_iommu_unmap(pfinfo->dev, &iommu_data);
			if (ret) {
				pr_err("failed to free iommu %d\n", i);
				return -EFAULT;
			}
		}
	}

	return 0;
}

#define CPP_IOCTL
#include "cpp_ioctl.c"
#undef CPP_IOCTL

/* Need modify ref to the DCAM driver. */
static long sprd_cppcore_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct cpp_device *dev = NULL;
	cpp_io_func io_ctrl;

	if (!file) {
		pr_err("fail to get valid input ptr\n");
		return -EINVAL;
	}

	dev = file->private_data;
	if (!dev) {
		pr_err("fail to get cpp device\n");
		return -EFAULT;
	}
	io_ctrl = sprd_cppcore_ioctl_get_fun(cmd);
	if (io_ctrl != NULL) {
		ret = io_ctrl(dev, arg);
		if (ret) {
			pr_err("fail to cmd %d\n", _IOC_NR(cmd));
			goto exit;
		}
	} else {
		pr_debug("fail to get valid cmd 0x%x 0x%x %s\n", cmd,
			sprd_cppcore_ioctl_get_val(cmd),
			sprd_cppcore_ioctl_get_str(cmd));
	}

exit:
	return ret;
}

static int sprd_cppcore_open(struct inode *node, struct file *file)
{
	int ret = 0;
	struct cpp_device *dev = NULL;
	struct miscdevice *md = NULL;
	struct rotif_device *rotif = NULL;
	struct scif_device *scif = NULL;
	struct dmaif_device *dmaif = NULL;

	CPP_TRACE("start open cpp\n");

	if (!node || !file) {
		pr_err("fail to get valid input ptr\n");
		return -EINVAL;
	}
	CPP_TRACE("start get miscdevice\n");
	md = (struct miscdevice *)file->private_data;
	if (!md) {
		pr_err("fail to get md device\n");
		return -EFAULT;
	}
	dev = md->this_device->platform_data;

	file->private_data = (void *)dev;
	if (atomic_inc_return(&dev->users) != 1) {
		CPP_TRACE("cpp device node has been opened %d\n",
			atomic_read(&dev->users));
		return 0;
	}
#ifndef TEST_ON_HAPS
	ret = sprd_cam_pw_on();
	if (ret) {
		pr_err("%s fail to power on cpp\n", __func__);
		goto fail;
	}
	sprd_cam_domain_eb();
#endif
	ret = sprd_cppcore_module_enable(dev);
	if (ret) {
		ret = -EFAULT;
		pr_err("fail to enable cpp module\n");
		goto enable_fail;
	}

	rotif = vzalloc(sizeof(*rotif));
	if (unlikely(!rotif)) {
		ret = -EFAULT;
		pr_err("fail to vzalloc rotif\n");
		goto vzalloc_fail;
	}
	rotif->drv_priv.io_base = dev->io_base;
	rotif->drv_priv.priv = (void *)rotif;
	dev->rotif = rotif;
	rotif->drv_priv.hw_lock = &dev->hw_lock;
	sprd_cppcore_register_isr(dev, CPP_ROT_DONE,
		sprd_cppcore_rot_isr, (void *)rotif);
	init_completion(&rotif->done_com);
	mutex_init(&rotif->rot_mutex);

	scif = vzalloc(sizeof(*scif));
	if (unlikely(!scif)) {
		ret = -EFAULT;
		pr_err("fail to vzalloc scif\n");
		goto vzalloc_fail;
	}
	scif->drv_priv.io_base = dev->io_base;
	scif->drv_priv.pdev = dev->pdev;
	scif->drv_priv.hw_lock = &dev->hw_lock;
	scif->drv_priv.priv = (void *)scif;
	dev->scif = scif;
	init_completion(&scif->done_com);
	mutex_init(&scif->sc_mutex);
	sprd_cppcore_register_isr(dev, CPP_SCALE_DONE,
		sprd_cppcore_scale_isr, (void *)scif);

	dmaif = vzalloc(sizeof(*dmaif));
	if (unlikely(!dmaif)) {
		ret = -EFAULT;
		pr_err("fail to vzalloc dmaif\n");
		goto vzalloc_fail;
	}
	dmaif->drv_priv.io_base = dev->io_base;
	dmaif->drv_priv.pdev = dev->pdev;
	dmaif->drv_priv.hw_lock = &dev->hw_lock;
	dmaif->drv_priv.priv = (void *)dmaif;
	dev->dmaif = dmaif;
	init_completion(&dmaif->done_com);
	mutex_init(&dmaif->dma_mutex);
	spin_lock_init(&dev->hw_lock);
	sprd_cppcore_register_isr(dev, CPP_DMA_DONE,
		sprd_cppcore_dma_isr, (void *)dmaif);

	ret = devm_request_irq(&dev->pdev->dev, dev->irq,
		sprd_cppcore_isr_root, IRQF_SHARED, "CPP", (void *)dev);
	if (ret < 0) {
		pr_err("fail to install IRQ %d\n", ret);
		goto vzalloc_fail;
	}

	CPP_TRACE("open sprd_cpp success\n");

	return ret;

vzalloc_fail:
	if (scif) {
		vfree(scif);
		dev->scif = NULL;
	}
	if (rotif) {
		vfree(rotif);
		dev->rotif = NULL;
	}
	if (dmaif) {
		vfree(dmaif);
		dev->dmaif = NULL;
	}
	sprd_cppcore_module_disable(dev);
enable_fail:
	sprd_cam_domain_disable();
	ret = sprd_cam_pw_off();
	if (ret) {
		pr_err("%s: failed to camera power off\n", __func__);
		return ret;
	}
#ifndef TEST_ON_HAPS
fail:
#endif
	if (atomic_dec_return(&dev->users) != 0)
		CPP_TRACE("others is using cpp device\n");
	file->private_data = NULL;

	return ret;
}

static int sprd_cppcore_release(struct inode *node,
	struct file *file)
{
	int ret = 0;
	struct cpp_device *dev = NULL;

	if (!node || !file) {
		pr_err("fail to get valid input ptr\n");
		return -EINVAL;
	}

	dev = file->private_data;
	if (dev == NULL) {
		pr_err("fail to close cpp device\n");
		return -EFAULT;
	}

	if (atomic_dec_return(&dev->users) != 0) {
		return ret;
	}
	if (dev->rotif) {
		vfree(dev->rotif);
		dev->rotif = NULL;
	}
	if (dev->scif) {
		vfree(dev->scif);
		dev->scif = NULL;
	}

	if (dev->dmaif) {
		vfree(dev->dmaif);
		dev->dmaif = NULL;
	}

	devm_free_irq(&dev->pdev->dev, dev->irq, (void *)dev);
	sprd_cppcore_module_disable(dev);

	file->private_data = NULL;
	CPP_TRACE("cpp release success\n");

	return ret;
}

static const struct file_operations cpp_fops = {
	.open = sprd_cppcore_open,
	.unlocked_ioctl = sprd_cppcore_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sprd_cppcore_ioctl,
#endif
	.release = sprd_cppcore_release,
};

static int sprd_cppcore_probe(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int irq = 0;
	struct cpp_device *dev = NULL;
	void __iomem *reg_base = NULL;
	struct regmap *mm_ahb_gpr = NULL;
#ifndef TEST_ON_HAPS
	struct device_node *qos_node = NULL;
#endif

	if (!pdev) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		pr_err("fail to alloc cpp dev memory\n");
		ret = -ENOMEM;
		goto fail;
	}

	dev->md.minor = MISC_DYNAMIC_MINOR;
	dev->md.name = CPP_DEVICE_NAME;
	dev->md.fops = &cpp_fops;
	dev->md.parent = NULL;
	ret = misc_register(&dev->md);
	if (ret) {
		pr_err("fail to register misc devices\n");
		return ret;
	}
	dev->pdev = pdev;
	dev->md.this_device->of_node = pdev->dev.of_node;
	dev->md.this_device->platform_data = (void *)dev;

	mutex_init(&dev->lock);
	spin_lock_init(&dev->slock);
	atomic_set(&dev->users, 0);

	CPP_TRACE("sprd cpp probe pdev name %s\n", pdev->name);

#ifndef TEST_ON_HAPS
	dev->cpp_eb = devm_clk_get(&pdev->dev, "cpp_eb");
	if (IS_ERR_OR_NULL(dev->cpp_eb)) {
		ret =  PTR_ERR(dev->cpp_eb);
		goto misc_fail;
	}

	dev->cpp_clk = devm_clk_get(&pdev->dev, "cpp_clk");
	if (IS_ERR_OR_NULL(dev->cpp_clk)) {
		ret = PTR_ERR(dev->cpp_clk);
		goto misc_fail;
	}

	dev->cpp_clk_parent = devm_clk_get(&pdev->dev, "cpp_clk_parent");
	if (IS_ERR_OR_NULL(dev->cpp_clk_parent)) {
		ret = PTR_ERR(dev->cpp_clk_parent);
		goto misc_fail;
	}

	dev->cpp_clk_default = clk_get_parent(dev->cpp_clk);
	if (IS_ERR_OR_NULL(dev->cpp_clk_default)) {
		ret = PTR_ERR(dev->cpp_clk_default);
		goto misc_fail;
	}
#endif

	mm_ahb_gpr = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
			"sprd,cam-ahb-syscon");
	if (IS_ERR_OR_NULL(mm_ahb_gpr)) {
		pr_err("fail to get mm_ahb_gpr\n");
		ret = PTR_ERR(mm_ahb_gpr);
		goto misc_fail;
	}
	dev->mm_ahb_gpr = mm_ahb_gpr;

	reg_base = of_iomap(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(reg_base)) {
		pr_err("fail to get cpp base\n");
		ret = PTR_ERR(reg_base);
		goto misc_fail;
	}
	dev->io_base = reg_base;
	g_cpp_base = (unsigned long)reg_base;
	irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (irq <= 0) {
		pr_err("fail to get cpp irq %d\n", irq);
		ret = -EINVAL;
		goto misc_fail;
	}
	dev->irq = irq;
#ifndef TEST_ON_HAPS
	qos_node = of_parse_phandle(pdev->dev.of_node, "cpp_qos", 0);
	if (qos_node) {
		uint8_t val;

		if (of_property_read_u8(qos_node, "awrqos", &val)) {
			pr_err("fail to get cpp qos.\n");
			val = 0x1;
		}
	dev->awrqos = (uint32_t)val;
	} else {
		dev->awrqos = 0x1;
	}
#endif
	CPP_TRACE("cpp probe OK\n");

	return 0;

misc_fail:
	pr_err("fai to probe cpp module\n");
	misc_deregister(&dev->md);
fail:

	return ret;
}

static int sprd_cppcore_remove(struct platform_device *pdev)
{
	struct cpp_device *dev = platform_get_drvdata(pdev);

	misc_deregister(&dev->md);

	return 0;
}

static const struct of_device_id of_match_table[] = {
	{ .compatible = "sprd,cpp", },
	{},
};

static struct platform_driver sprd_cpp_driver = {
	.probe = sprd_cppcore_probe,
	.remove = sprd_cppcore_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = CPP_DEVICE_NAME,
		.of_match_table = of_match_ptr(of_match_table),
	},
};

module_platform_driver(sprd_cpp_driver);

MODULE_DESCRIPTION("R6P0 Lite CPP Driver");
MODULE_AUTHOR("Multimedia_Camera@unisoc");
MODULE_LICENSE("GPL");

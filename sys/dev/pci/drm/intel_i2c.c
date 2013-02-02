#include "drmP.h"
#include "drm.h"
#include "i915_drv.h"
#include "i915_reg.h"

#include <dev/i2c/i2cvar.h>

int	gmbus_i2c_acquire_bus(void *, int);
void	gmbus_i2c_release_bus(void *, int);
int	gmbus_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
	    void *buf, size_t, int);
void	i915_i2c_probe(struct inteldrm_softc *);

int
gmbus_i2c_acquire_bus(void *cookie, int flags)
{
	struct gmbus_port *gp = cookie;
	struct inteldrm_softc *dev_priv = gp->dev_priv;

	I915_WRITE(dev_priv->gpio_mmio_base + GMBUS0,
	    GMBUS_RATE_100KHZ | gp->port);

	return (0);
}

void
gmbus_i2c_release_bus(void *cookie, int flags)
{
	struct gmbus_port *gp = cookie;
	struct inteldrm_softc *dev_priv = gp->dev_priv;

	I915_WRITE(dev_priv->gpio_mmio_base + GMBUS0, 0);
}

int
gmbus_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct gmbus_port *gp = cookie;
	struct inteldrm_softc *dev_priv = gp->dev_priv;
	uint32_t reg, st, val;
	int reg_offset = dev_priv->gpio_mmio_base;
	uint8_t *b;
	int i, retries;

	if (cmdlen > 1 || I2C_OP_WRITE_P(op))
		return (EOPNOTSUPP);

	reg = 0;
	if (cmdlen > 0)
		reg |= GMBUS_CYCLE_INDEX;
	if (len > 0)
		reg |= GMBUS_CYCLE_WAIT;
	if (I2C_OP_STOP_P(op))
		reg |= GMBUS_CYCLE_STOP;
	if (I2C_OP_READ_P(op))
		reg |= GMBUS_SLAVE_READ;
	reg |= (addr << GMBUS_SLAVE_ADDR_SHIFT);
	b = (void *)cmdbuf;
	if (cmdlen > 0)
		reg |= (b[0] << GMBUS_SLAVE_INDEX_SHIFT);
	reg |= (len << GMBUS_BYTE_COUNT_SHIFT);
	I915_WRITE(GMBUS1 + reg_offset, reg | GMBUS_SW_RDY);

	if (I2C_OP_READ_P(op)) {
		b = buf;
		while (len > 0) {
			for (retries = 50; retries > 0; retries--) {
				st = I915_READ(GMBUS2 + reg_offset);
				if (st & (GMBUS_SATOER | GMBUS_HW_RDY))
					break;
				DELAY(1000);
			}
			if (st & GMBUS_SATOER)
				return (ENXIO);
			if ((st & GMBUS_HW_RDY) == 0)
				return (ETIMEDOUT);

			val = I915_READ(GMBUS3 + reg_offset);
			for (i = 0; i < 4 && len > 0; i++, len--) {
				*b++ = val & 0xff;
				val >>= 8;
			}
		}
	}

	for (retries = 10; retries > 0; retries--) {
		st = I915_READ(GMBUS2 + reg_offset);
		if ((st & GMBUS_ACTIVE) == 0)
			break;
		DELAY(1000);
	}
	if (st & GMBUS_ACTIVE)
		return (ETIMEDOUT);

	return (0);
}

void
i915_i2c_probe(struct inteldrm_softc *dev_priv)
{
	struct drm_device *dev = (struct drm_device *)dev_priv->drmdev;
	struct gmbus_port gp;
	struct i2c_controller ic;
	uint8_t buf[128];
	uint8_t cmd;
	int err, i;

	if (HAS_PCH_SPLIT(dev))
		dev_priv->gpio_mmio_base = PCH_GPIOA - GPIOA;
	else
		dev_priv->gpio_mmio_base = 0;

	gp.dev_priv = dev_priv;
	gp.port = GMBUS_PORT_PANEL;

	ic.ic_cookie = &gp;
	ic.ic_acquire_bus = gmbus_i2c_acquire_bus;
	ic.ic_release_bus = gmbus_i2c_release_bus;
	ic.ic_exec = gmbus_i2c_exec;

	bzero(buf, sizeof(buf));
	iic_acquire_bus(&ic, 0);
	cmd = 0;
	err = iic_exec(&ic, I2C_OP_READ_WITH_STOP, 0x50, &cmd, 1, buf, 128, 0);
	if (err)
		printf("err %d\n", err);
	iic_release_bus(&ic, 0);
	for (i = 0; i < sizeof(buf); i++)
		printf(" 0x%02x", buf[i]);
	printf("\n");
}

int
intel_setup_gmbus(struct inteldrm_softc *dev_priv)
{
	struct drm_device *dev = (struct drm_device *)dev_priv->drmdev;

	if (HAS_PCH_SPLIT(dev))
		dev_priv->gpio_mmio_base = PCH_GPIOA - GPIOA;
	else
		dev_priv->gpio_mmio_base = 0;

	dev_priv->gp.dev_priv = dev_priv;
	dev_priv->gp.port = GMBUS_PORT_PANEL;

	dev_priv->ddc.ic_cookie = &dev_priv->gp;
	dev_priv->ddc.ic_acquire_bus = gmbus_i2c_acquire_bus;
	dev_priv->ddc.ic_release_bus = gmbus_i2c_release_bus;
	dev_priv->ddc.ic_exec = gmbus_i2c_exec;

	return (0);
}

void
intel_gmbus_set_port(struct inteldrm_softc *dev_priv, int port)
{
	dev_priv->gp.port = port;	
}

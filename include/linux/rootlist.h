
#ifndef __LINUX_ROOTLIST_H
#define __LINUX_ROOTLIST_H

#include <linux/ioctl.h>

enum root_e {
    nBLUETOOTH = _IO('R', 0),
    nLCD,
    nSYSTEM,
    nSENSOR,
    nMMI,
};

enum attr_e {
    tTYPE = _IO('A', 0),
    tCHIP,
    tNAME,
    tVERSION,
    tDPI,
    tPPI,
    tEXTERIOR,
    tPATH,
    tSIZE,
    tPORT,
    tITEM,
};

enum addition_e {
    tSINGLE   = _IO('0', 0),
    tMULTIPLE,
};

typedef enum root_e root_key;
typedef enum attr_e attr_key;
typedef enum addition_e add_key;

#ifdef CONFIG_ROOTLIST
extern int hwlist_create_attr(root_key root, attr_key attr, add_key add, const char *value);
#else
#define hwlist_create_attr(a,b,c,d) 0
#endif

/*   bluetooth    */
static inline int hwlist_bluetooth_chip(const char *value)
{
    return hwlist_create_attr(nBLUETOOTH, tCHIP, tSINGLE, value);
}

static inline int hwlist_bluetooth_port(const char *value)
{
    return hwlist_create_attr(nBLUETOOTH, tPORT, tSINGLE, value);
}

/*   lcd   */
static inline int hwlist_lcd_chip(const char *value)
{
    return hwlist_create_attr(nLCD, tCHIP, tSINGLE, value);
}

static inline int hwlist_lcd_name(const char *value)
{
    return hwlist_create_attr(nLCD, tNAME, tSINGLE, value);
}

static inline int hwlist_lcd_size(const char *value)
{
    return hwlist_create_attr(nLCD, tSIZE, tSINGLE, value);
}

static inline int hwlist_lcd_dpi(const char *value)
{
    return hwlist_create_attr(nLCD, tDPI, tSINGLE, value);
}

static inline int hwlist_lcd_ppi(const char *value)
{
    return hwlist_create_attr(nLCD, tPPI, tSINGLE, value);
}

static inline int hwlist_lcd_exterior(const char *value)
{
    return hwlist_create_attr(nLCD, tEXTERIOR, tSINGLE, value);
}

/*   sensor   */
static inline int hwlist_sensor_chip(const char *value)
{
    return hwlist_create_attr(nSENSOR, tCHIP, tSINGLE, value);
}

static inline int hwlist_sensor_type(const char *value)
{
    return hwlist_create_attr(nSENSOR, tTYPE, tSINGLE, value);
}

/*   mmi   */
static inline int hwlist_mmi_item(const char *value)
{
    return hwlist_create_attr(nMMI, tITEM, tMULTIPLE, value);
}
#endif


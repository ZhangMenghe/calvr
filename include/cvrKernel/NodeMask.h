/**
 * @file NodeMask.h
 */
#ifndef CALVR_NODE_MASKS_H
#define CALVR_NODE_MASKS_H

namespace cvr
{

/**
 * @addtogroup kernel
 * @{
 */

/**
 * @brief Custom scenegraph node mask bits used by CalVR
 */
enum CVRNodeMask
{
    INTERSECT_MASK = 0x0002,
    CULL_MASK = 0x0400,
    CULL_MASK_LEFT = 0x0800,
    CULL_MASK_RIGHT = 0x1000,
    CULL_MASK_EYE_3 = 0x10000,
    CULL_MASK_EYE_4 = 0x20000,
    CULL_MASK_EYE_5 = 0x40000,
    CULL_MASK_EYE_6 = 0x80000,
    CULL_MASK_EYE_7 = 0x100000,
    CULL_MASK_EYE_8 = 0x200000,
    DISABLE_FIRST_CULL = 0x1000000,
    FIRST_CULL_STATUS = 0x2000000,
    CULL_ENABLE = 0x4000000
};

/**
 * @}
 */

}

#endif

 /******************************************************************************
 *  Copyright 2015 Qualcomm Technologies International, Ltd.
 *****************************************************************************/
/*! \file csr_types.h
 *  \brief CSRmesh library data types
 *
 *   This file contains the different data types used in CSRmesh library
 *
 *   NOTE: This library includes the Mesh Transport Layer, Mesh Control
 *   Layer and Mesh Association Layer functionality.
 */
 /*****************************************************************************/

#ifndef __CSR_TYPES_H__
#define __CSR_TYPES_H__

#if defined(CSR101x) || defined(CSR101x_A05) || defined(CSR102x) || defined(CSR102x_A05)
/*! \brief  CSR Mesh Macro to enable On Chip */
#if !defined(CSR_MESH_ON_CHIP)
#define CSR_MESH_ON_CHIP                             (1)
#endif /* !CSR_MESH_ON_CHIP */
/*! \brief  CSR Mesh Macro used for declaring large local variable in stack space  */
#define CSR_MESH_THREAD_LOCAL(var_type, var_name) static var_type var_name
#include <types.h>
#else
#define CSR_MESH_ON_CHIP                             (0)
#define CSR_MESH_THREAD_LOCAL(var_type, var_name) var_type var_name
#if defined(QCA401x)
#include "qcom_common.h"
#else
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#endif /* QCA401x */
#endif

/*! \addtogroup CSRmesh
 * @{
 */
 
#ifdef __cplusplus
extern "C" {
#endif

#undef TRUE
#undef FALSE

#define FALSE (0)
#define TRUE  (!FALSE)


#if (CSR_MESH_ON_CHIP != 1)

#ifndef QCA401x

/* Unsigned fixed width types */
typedef uint8_t CsrUint8;
typedef uint16_t CsrUint16;
typedef uint32_t CsrUint24;
typedef uint32_t CsrUint32;

/* Signed fixed width types */
typedef int8_t CsrInt8;
typedef int16_t CsrInt16;
typedef int32_t CsrInt32;

#else

typedef unsigned char CsrUint8;
typedef unsigned short CsrUint16;
typedef unsigned int CsrUint24;
typedef unsigned int CsrUint32;

typedef char CsrInt8;
typedef short CsrInt16;
typedef int CsrInt32;

#endif

#define min(x, y)    (((x) < (y)) ? (x) : (y))

#else

/* Unsigned fixed width types */
typedef uint8 CsrUint8;
typedef uint16 CsrUint16;
typedef uint24 CsrUint24;
typedef uint32 CsrUint32;

/* Signed fixed width types */
typedef int8 CsrInt8;
typedef int16 CsrInt16;
typedef int32 CsrInt32;

#endif

/* String types */
typedef char CsrCharString;

#if (CSR_MESH_ON_CHIP == 1)
typedef uint16 CsrSize;
/* Boolean */
typedef bool CsrBool;
#else

#ifndef QCA401x

typedef uint16_t CsrSize;
typedef unsigned char CsrBool;

#else

typedef size_t CsrSize;
typedef unsigned char CsrBool;

#endif

#endif

#define DEFINE_STATIC(VAR_TYPE, VAR_NAME, VAR_INIT_VAL) static VAR_TYPE VAR_NAME = VAR_INIT_VAL
#define DEFINE_STATIC_CONST(VAR_TYPE, VAR_NAME, VAR_INIT_VAL) static const VAR_TYPE VAR_NAME = VAR_INIT_VAL
#define DEFINE_EXTERN(VAR_TYPE, VAR_NAME, VAR_INIT_VAL) VAR_TYPE VAR_NAME = VAR_INIT_VAL

#define DSTATIC static
#define DEXTERN extern

/* Enable the following define to enable selective logging in a debug disabled build */

/* #define CSR_MESH_CRITICAL_TEST_DEBUG */
#ifdef CSR_MESH_CRITICAL_TEST_DEBUG
#include <debug.h>
#include <uart.h>
#endif

#ifdef __cplusplus
}
#endif

/*!@} */
#endif /*__CSR_TYPES_H__ */


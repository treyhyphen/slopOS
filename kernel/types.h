/*
 * Common type definitions for slopOS
 */

#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

// Standard integer types
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long size_t;

// Boolean type
typedef int bool;
#define true 1
#define false 0
#define NULL ((void*)0)

// Network constants
#define ETH_ALEN 6
#define ETH_FRAME_LEN 1514
#define IP_ADDR_LEN 4

#endif // KERNEL_TYPES_H

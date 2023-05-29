#ifndef TESTMACHINE_DISK_H
#define TESTMACHINE_DISK_H

/*
 *  Definitions used by the "disk" device in GXemul.
 *
 *  This file is in the public domain.
 */

#define DEV_DISK_ADDRESS 0x13000000 // Simulated IDE disk 的地址
#define DEV_DISK_OFFSET 0x0000 //写：设置下一次读写操作时的磁盘镜像偏移的字节数
#define DEV_DISK_OFFSET_HIGH32 0x0008 //写：设置高 32 位的偏移的字节数
#define DEV_DISK_ID 0x0010 //写：设置下一次读写操作的磁盘编号
#define DEV_DISK_START_OPERATION 0x0020 //写：开始一次读写操作（写 0 表示读操作，1 表示写操作）
#define DEV_DISK_STATUS 0x0030 //读：获取上一次操作的状态返回值（读 0 表示失败，非 0 则表示成功）
#define DEV_DISK_BUFFER 0x4000 //读/写：512 字节的读写缓存

#define DEV_DISK_BUFFER_LEN 0x200

/*  Operations:  */
#define DEV_DISK_OPERATION_READ 0
#define DEV_DISK_OPERATION_WRITE 1

#endif /*  TESTMACHINE_DISK_H  */

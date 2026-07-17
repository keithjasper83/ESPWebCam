/**
 * sd_read_write.h
 *
 * SD card helper functions.
 *
 * SD card support is a diagnostic-only feature gated behind the
 * ENABLE_SD_CARD compile-time flag.  The normal camera stream and recording
 * workflow does NOT use the SD card; all recording happens on the iPhone.
 *
 * To enable SD support, add -DENABLE_SD_CARD=1 to build_flags in
 * firmware/platformio.ini.
 *
 * SD_MMC pin assignments for Freenove ESP32-S3 WROOM:
 *   CMD = GPIO38, CLK = GPIO39, D0 = GPIO40
 *
 * IMPORTANT: The boot behaviour that deleted and recreated /video on every
 * power-on has been REMOVED.  The SD card is never written to in the normal
 * operation path.
 */
#ifndef __SD_READ_WRITE_H
#define __SD_READ_WRITE_H

#ifdef ENABLE_SD_CARD

#include "Arduino.h"
#include "FS.h"
#include "SD_MMC.h"

#define SD_MMC_CMD  38
#define SD_MMC_CLK  39
#define SD_MMC_D0   40

void sdmmcInit(void);

void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void createDir(fs::FS &fs, const char * path);
void removeDir(fs::FS &fs, const char * path);
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);
void testFileIO(fs::FS &fs, const char * path);

void writejpg(fs::FS &fs, const char * path, const uint8_t *buf, size_t size);
int readFileNum(fs::FS &fs, const char * dirname);

#endif  // ENABLE_SD_CARD
#endif  // __SD_READ_WRITE_H

#ifndef __TCSM_LAYOUT_H__
#define __TCSM_LAYOUT_H__

/**
 *      |-------------|
 *      |   BANK 0    |
 *	|-------------| <--- VOICE_TCSM_DATA_BUFFER
 *	|    ...      |
 *	|   BUFFER    |
 *	|    ...      |
 *	|-------------|
 *
 *      |-------------|
 *      |    BANK 1   |
 *      |-------------| <--- SLEEP_TCSM_BOOTCODE_TEXT
 *      |  BOOT CODE  |
 *      |-------------| <--- SLEEP_TCSM_RESUMECODE_TEXT
 *      |    ...      |
 *	| RESUME CODE |
 *      |    ...      |
 *      |-------------| <--- SLEEP_TCSM_RESUME_DATA
 *      | RESUME DATA |
 *      |_____________|
 *
 *      |-------------|
 *      |   BANK 2    |
 *      |-------------| <--- VOICE_DMA_DRSC_ADDR
 *      |   DMA DESC  |
 *      |-------------|
 *
 *      |-------------|
 *      |   BANK 3    |
 *      |-------------|
 *      |-------------| <--- CPU_RESMUE_SP
 *      |  RESUME SP  |
 *      |-------------|
 *
 */

#define SLEEP_TCSM_SPACE           0xb3423000
#define SLEEP_TCSM_LEN             4096

#define SLEEP_TCSM_BOOT_LEN        256
#define SLEEP_TCSM_DATA_LEN        64
#define SLEEP_TCSM_RESUME_LEN      (SLEEP_TCSM_LEN - SLEEP_TCSM_BOOT_LEN - SLEEP_TCSM_DATA_LEN)

#define SLEEP_TCSM_BOOT_TEXT       (SLEEP_TCSM_SPACE)
#define SLEEP_TCSM_RESUME_TEXT     (SLEEP_TCSM_BOOT_TEXT + SLEEP_TCSM_BOOT_LEN)
#define SLEEP_TCSM_RESUME_DATA     (SLEEP_TCSM_RESUME_TEXT + SLEEP_TCSM_RESUME_LEN)

#define CPU_RESUME_SP               0xb3425FFC


#define VOICE_TCSM_DATA_BUF		(0xb3422000)
#define VOICE_TCSM_DATA_BUF_SIZE	SLEEP_TCSM_LEN
#define VOICE_TCSM_DESC_ADDR		(0xb3424000) /* bank2 start */
#define CPU_RESUME_SP_END_ADDR		(0xb3425fff) /* bank3 end */

#endif

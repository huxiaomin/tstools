/* vim: set tabstop=8 shiftwidth=8:
 * name: catts.c
 * funx: generate text line with bin ts file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strcmp, etc */
#include <stdint.h> /* for uintN_t, etc */

#include "version.h"
#include "common.h"
#include "if.h"

static int rpt_lvl = RPT_WRN; /* report level: ERR, WRN, INF, DBG */

enum FILE_TYPE
{
        FILE_MTS,
        FILE_TSRS,
        FILE_TS,
        FILE_BIN,
        FILE_UNKNOWN
};

static FILE *fd_i = NULL;
static char file_i[FILENAME_MAX] = "";
static int npline = 16; /* data number per line */
static int type = FILE_TS;
static int aim_start = 0; /* first byte */
static int aim_stop = 0; /* last byte */
static long long int pkt_addr = 0;
static long long int pkt_mts = 0;

static int deal_with_parameter(int argc, char *argv[]);
static int show_help();
static int show_version();
static int judge_type();
static int mts_time(long long int *mts, uint8_t *bin);

int main(int argc, char *argv[])
{
        int cnt;
        unsigned char bbuf[LINE_LENGTH_MAX / 3 + 10]; /* bin data buffer */
        char tbuf[LINE_LENGTH_MAX + 10]; /* txt data buffer */

        if(0 != deal_with_parameter(argc, argv)) {
                return -1;
        }

        fd_i = fopen(file_i, "rb");
        if(NULL == fd_i) {
                RPT(RPT_ERR, "open \"%s\" failed", file_i);
                return -1;
        }

        judge_type();
        while(0 < (cnt = fread(bbuf, 1, npline, fd_i))) {
                switch(type) {
                        case FILE_TS:
                                fprintf(stdout, "*ts, ");
                                b2t(tbuf, bbuf, 188);
                                fprintf(stdout, "%s", tbuf);

                                fprintf(stdout, "*addr, %llX, \n", pkt_addr);
                                break;
                        case FILE_MTS:
                                fprintf(stdout, "*ts, ");
                                b2t(tbuf, bbuf + 4, 188);
                                fprintf(stdout, "%s", tbuf);

                                fprintf(stdout, "*addr, %llX, ", pkt_addr);

                                mts_time(&pkt_mts, bbuf);
                                fprintf(stdout, "*mts, %llX, \n", pkt_mts);
                                break;
                        case FILE_TSRS:
                                fprintf(stdout, "*ts, ");
                                b2t(tbuf, bbuf, 188);
                                fprintf(stdout, "%s", tbuf);

                                fprintf(stdout, "*rs, ");
                                b2t(tbuf, bbuf + 188, 16);
                                fprintf(stdout, "%s", tbuf);

                                fprintf(stdout, "*addr, %llX, \n", pkt_addr);
                                break;
                        default: /* FILE_BIN */
                                fprintf(stdout, "*data, ");
                                b2t(tbuf, bbuf, cnt);
                                fprintf(stdout, "%s", tbuf);

                                fprintf(stdout, "*addr, %llX, \n", pkt_addr);
                                break;
                }
                pkt_addr += cnt;

                if(0 != aim_stop && pkt_addr >= aim_stop) {
                        break;
                }
        }

        fclose(fd_i);

        return 0;
}

static int deal_with_parameter(int argc, char *argv[])
{
        int i;
        int dat;

        if(1 == argc) {
                /* no parameter */
                fprintf(stderr, "No binary file to process...\n\n");
                show_help();
                return -1;
        }

        for(i = 1; i < argc; i++) {
                if('-' == argv[i][0]) {
                        if(0 == strcmp(argv[i], "-s") ||
                           0 == strcmp(argv[i], "--start")) {
                                i++;
                                if(i >= argc) {
                                        fprintf(stderr, "no parameter for 'start'!\n");
                                        exit(EXIT_FAILURE);
                                }
                                sscanf(argv[i], "%i" , &dat);
                                if(0 < dat) {
                                        aim_start = dat;
                                }
                                else {
                                        fprintf(stderr,
                                                "bad variable for 'start': %u(0 < x), use 0 instead!\n",
                                                dat);
                                }
                        }
                        else if(0 == strcmp(argv[i], "-p") ||
                                0 == strcmp(argv[i], "--stop")) {
                                i++;
                                if(i >= argc) {
                                        fprintf(stderr, "no parameter for 'stop'!\n");
                                        exit(EXIT_FAILURE);
                                }
                                sscanf(argv[i], "%i" , &dat);
                                if(0 < dat) {
                                        aim_stop = dat;
                                }
                                else {
                                        fprintf(stderr,
                                                "bad variable for 'stop': %u(0 < x), use 0 instead!\n",
                                                dat);
                                }
                        }
                        else if(0 == strcmp(argv[i], "-w") ||
                                0 == strcmp(argv[i], "--width")) {
                                i++;
                                if(i >= argc) {
                                        fprintf(stderr, "no parameter for 'width'!\n");
                                        exit(EXIT_FAILURE);
                                }
                                sscanf(argv[i], "%i" , &dat);
                                if(0 < dat && dat < (LINE_LENGTH_MAX / 3)) {
                                        npline = dat;
                                }
                                else {
                                        fprintf(stderr,
                                                "bad variable for 'width': %u(0 < x < %u), use 16 instead!\n",
                                                dat, LINE_LENGTH_MAX / 3);
                                }
                        }
                        else if(0 == strcmp(argv[i], "-h") ||
                                0 == strcmp(argv[i], "--help")) {
                                show_help();
                                return -1;
                        }
                        else if(0 == strcmp(argv[i], "-v") ||
                                0 == strcmp(argv[i], "--version")) {
                                show_version();
                                return -1;
                        }
                        else {
                                RPT(RPT_ERR, "Wrong parameter: %s", argv[i]);
                                return -1;
                        }
                }
                else {
                        strcpy(file_i, argv[i]);
                }
        }

        return 0;
}

static int show_help()
{
        puts("'catts' read binary file, translate 0xXY to 'XY ' format, then send to stdout.");
        puts("");
        puts("Usage: catts [OPTION] file [OPTION]");
        puts("");
        puts("Options:");
        puts("");
        puts(" -w, --width <n>          n-byte per line for FILE_BIN, default: 16");
        puts(" -s, --start <a>          cat from, default: 0(from first byte)");
        puts(" -p, --stop <b>           cat to, default: 0(to last byte)");
        puts(" -h, --help               display this information");
        puts(" -v, --version            display my version");
        puts("");
        puts("Examples:");
        puts("  catts xxx.ts");
        puts("");
        puts("Report bugs to <zhoucheng@tsinghua.org.cn>.");
        return 0;
}

static int show_version()
{
        char str[100];

        sprintf(str, "catts of tstools v%s.%s.%s", VER_MAJOR, VER_MINOR, VER_RELEA);
        puts(str);
        sprintf(str, "Build time: %s %s", __DATE__, __TIME__);
        puts(str);
        puts("");
        puts("Copyright (C) 2009,2010,2011,2012 ZHOU Cheng.");
        puts("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>");
        puts("This is free software; contact author for additional information.");
        puts("There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR");
        puts("A PARTICULAR PURPOSE.");
        puts("");
        puts("Written by ZHOU Cheng.");
        return 0;
}

#define SYNC_TIME       3 /* SYNC_TIME syncs means TS sync */
#define ASYNC_BYTE      4096 /* head ASYNC_BYTE bytes async means BIN file */
/* for TS data: use "state machine" to determine sync position and packet size */
static int judge_type()
{
        uint8_t dat;
        int sync_cnt = 0;
        int state = FILE_UNKNOWN;

        pkt_addr = 0;
        type = FILE_UNKNOWN;
        while(FILE_UNKNOWN == type) {
                switch(state) {
                        case FILE_UNKNOWN:
                                fseek(fd_i, pkt_addr, SEEK_SET);
                                if(1 != fread(&dat, 1, 1, fd_i)) {
                                        return -1;
                                }
                                if(0x47 == dat) {
                                        sync_cnt = 1;
                                        state = FILE_TS;
                                }
                                else {
                                        pkt_addr++;
                                        if(pkt_addr > ASYNC_BYTE) {
                                                pkt_addr = aim_start;
                                                type = FILE_BIN;
                                                state = FILE_BIN;
                                        }
                                }
                                break;
                        case FILE_TS:
                                fseek(fd_i, +187, SEEK_CUR);
                                if(1 != fread(&dat, 1, 1, fd_i)) {
                                        return -1;
                                }
                                if(0x47 == dat) {
                                        sync_cnt++;
                                        if(sync_cnt >= SYNC_TIME) {
                                                npline = 188;
                                                type = FILE_TS;
                                        }
                                }
                                else {
                                        fseek(fd_i, pkt_addr + 1, SEEK_SET);
                                        sync_cnt = 1;
                                        state = FILE_MTS;
                                }
                                break;
                        case FILE_MTS:
                                fseek(fd_i, +191, SEEK_CUR);
                                if(1 != fread(&dat, 1, 1, fd_i)) {
                                        return -1;
                                }
                                if(0x47 == dat) {
                                        sync_cnt++;
                                        if(sync_cnt >= SYNC_TIME) {
                                                if(pkt_addr < 4) {
                                                        fseek(fd_i, pkt_addr + 1, SEEK_SET);
                                                        sync_cnt = 1;
                                                        state = FILE_TSRS;
                                                }
                                                else {
                                                        pkt_addr -= 4;
                                                        npline = 192;
                                                        type = FILE_MTS;
                                                }
                                        }
                                }
                                else {
                                        fseek(fd_i, pkt_addr + 1, SEEK_SET);
                                        sync_cnt = 1;
                                        state = FILE_TSRS;
                                }
                                break;
                        case FILE_TSRS:
                                fseek(fd_i, +203, SEEK_CUR);
                                if(1 != fread(&dat, 1, 1, fd_i)) {
                                        return -1;
                                }
                                if(0x47 == dat) {
                                        sync_cnt++;
                                        if(sync_cnt >= SYNC_TIME) {
                                                npline = 204;
                                                type = FILE_TSRS;
                                        }
                                }
                                else {
                                        pkt_addr = aim_start;
                                        type = FILE_BIN;
                                        state = FILE_BIN;
                                }
                                break;
                        default:
                                RPT(RPT_ERR, "bad state: %d", state);
                                return -1;
                }
        }

        fseek(fd_i, pkt_addr, SEEK_SET);
        if(pkt_addr != 0) {
                fprintf(stderr, "%lld-byte passed\n", pkt_addr);
        }
        return 0;
}

static int mts_time(long long int *mts, uint8_t *bin)
{
        int i;

        for(*mts = 0, i = 0; i < 4; i++) {
                *mts <<= 8;
                *mts |= *bin++;
        }

        return 0;
}
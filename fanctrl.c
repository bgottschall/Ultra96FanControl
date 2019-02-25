/*
 * Based on https://github.com/AndrewCapon/Ultra96FanControl
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <getopt.h>

#ifndef S_AXI_BASEADDR
#error "You need to define S_AXI_BASEADDR!"
#endif

float regToFloat(u_int32_t f) {
    static const unsigned int _shift = 8;
    return ((float)f / (float)(1 << _shift));
}

void printHelp(char opt, char *optarg) {
    if (opt != 0) {
        if (optarg) {
            printf("Invalid parameter - %c %s\n", opt, optarg);
        } else {
            printf("Invalid parameter - %c\n", opt);
        }
    }
    printf("FanCTRL - Control Ultra96 Fan PWM\n");
    printf("\nRead Fan State (default):\n");
    printf("    -r, --read                 read state\n");
    printf("    -w, --watch                continous read\n");
    printf("    -u, --update               set update interval (ms)\n");
    printf("\nSet Fan Paramters:\n");
    printf("    -l, --low-temp <celsius>   set low temp\n");
    printf("    -m, --max-temp <celsius>   set max temp\n");
    printf("    -s, --smooth <smooth>      set smooth divisor\n");
    printf("    -a, --auto                 automatic pwm control\n");
    printf("    -f, --fixed <duty-cycle>   fixed pwm duty cycle\n");
    printf("\nMiscellaneous:\n");
    printf("    -h, --help                 this text\n");
    printf("    -d, --debug                much more information\n");
    printf("\nWARNING: misconfiguration can lead to an emergency shutdown or hardware damages!\n");
}


#define regLowTemp      (*((volatile unsigned *)(baseRegister + 0)))
#define regMaxTemp      (*((volatile unsigned *)(baseRegister + 1)))
#define regSmooth       (*((volatile unsigned *)(baseRegister + 2)))
#define regFixed        (*((volatile unsigned *)(baseRegister + 3)))
#define regTemp         (*((volatile unsigned *)(baseRegister + 4)))
#define regAlarm        (*((volatile unsigned *)(baseRegister + 5)))
#define regDbgState     (*((volatile unsigned *)(baseRegister + 6)))
#define regDbgPWM       regToFloat((*((volatile unsigned *)(baseRegister + 7))))
#define regDbgUsePWM    regToFloat((*((volatile unsigned *)(baseRegister + 8))))
#define regDbgLastPWM   regToFloat((*((volatile unsigned *)(baseRegister + 9))))
#define regDbgRealTemp  regToFloat((*((volatile unsigned *)(baseRegister + 10))))
#define regDbgUseTemp   regToFloat((*((volatile unsigned *)(baseRegister + 11))))
#define regDbgLastTemp  regToFloat((*((volatile unsigned *)(baseRegister + 12))))
#define regDbgTempError regToFloat((*((volatile unsigned *)(baseRegister + 13))))
#define regDbgLinear    regToFloat((*((volatile unsigned *)(baseRegister + 14))))
#define regDbgMaxPWM    regToFloat((*((volatile unsigned *)(baseRegister + 15))))

#define dbgRegAddrLowTemp       S_AXI_BASEADDR + 0 * sizeof(unsigned)
#define dbgRegAddrMaxTemp       S_AXI_BASEADDR + 1 * sizeof(unsigned)
#define dbgRegAddrSmooth        S_AXI_BASEADDR + 2 * sizeof(unsigned)
#define dbgRegAddrFixed         S_AXI_BASEADDR + 3 * sizeof(unsigned)
#define dbgRegAddrTemp          S_AXI_BASEADDR + 4 * sizeof(unsigned)
#define dbgRegAddrAlarm         S_AXI_BASEADDR + 5 * sizeof(unsigned)
#define dbgRegAddrDbgState      S_AXI_BASEADDR + 6 * sizeof(unsigned)
#define dbgRegAddrDbgPWM        S_AXI_BASEADDR + 7 * sizeof(unsigned)
#define dbgRegAddrDbgUsePWM     S_AXI_BASEADDR + 8 * sizeof(unsigned)
#define dbgRegAddrDbgLastPWM    S_AXI_BASEADDR + 9 * sizeof(unsigned)
#define dbgRegAddrDbgRealTemp   S_AXI_BASEADDR + 10 * sizeof(unsigned)
#define dbgRegAddrDbgUseTemp    S_AXI_BASEADDR + 11 * sizeof(unsigned)
#define dbgRegAddrDbgLastTemp   S_AXI_BASEADDR + 12 * sizeof(unsigned)
#define dbgRegAddrDbgTempError  S_AXI_BASEADDR + 13 * sizeof(unsigned)
#define dbgRegAddrDbgLinear     S_AXI_BASEADDR + 14 * sizeof(unsigned)
#define dbgRegAddrDbgMaxPWM     S_AXI_BASEADDR + 15 * sizeof(unsigned)


int main(int argc, char **argv) {
    double ltemp = -1.0;
    double mtemp = -1.0;
    int opAuto = -1;
    double opFixed = -1.0;
    long smooth = -1;
    int modeRead  = 1;
    int modeWatch = 0;
    int modeDebug = 0;
    long updateInt = 1000;

    static struct option const long_options[] =  {
        {"read",    no_argument, 0, 'r'},
        {"watch",   no_argument, 0, 'w'},
        {"debug",   no_argument, 0, 'd'},
        {"auto",    no_argument, 0, 'a'},
        {"help",    no_argument, 0, 'h'},
        {"update",  required_argument, 0, 'u'},
        {"low-temp",  required_argument, 0, 'l'},
        {"max-temp",  required_argument, 0, 'm'},
        {"smooth",    required_argument, 0, 's'},
        {"fixed",     required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    static char const *fanStates[] = {"FAN_OFF", "FAN_PROCESS", "FAN_ON", "FAN_FORCED"};

    while (1) {
        char *endptr;
        int c;
        int option_index = 0;

        c = getopt_long (argc, argv, "rwdahl:m:s:f:u:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
                break;
            case 'h':
                printHelp(0, NULL);
                return 0;
            case 'l':
                ltemp = strtod(optarg, &endptr);
                if (endptr == optarg || ltemp < 0.0) {
                    printHelp(c, optarg);
                    return -1;
                }
                break;
            case 'm':
                mtemp = strtod(optarg, &endptr);
                if (endptr == optarg || mtemp < 0.0) {
                    printHelp(c, optarg);
                    return -1;
                }
                break;
            case 'f':
                opFixed = strtod(optarg, &endptr);
                if (endptr == optarg || opFixed < 0.0 || opFixed > 100.0) {
                    printHelp(c, optarg);
                    return -1;
                }
                break;
            case 's':
                smooth = strtol(optarg, &endptr, 10);
                if (endptr == optarg || smooth < 0) {
                    printHelp(c, optarg);
                    return -1;
                }
                break;
            case 'u':
                updateInt = strtol(optarg, &endptr, 10);
                if (endptr == optarg || updateInt < 0) {
                    printHelp(c, optarg);
                    return -1;
                }
                break;
            case 'a':
                opAuto = 1;
                break;
            case 'r':
                modeRead = 2;
                break;
            case 'w':
                modeWatch = 1;
                break;
            case 'd':
                modeDebug = 1;
                break;
            default:
                abort();
        }

    }

    unsigned const uPageSize = sysconf(_SC_PAGESIZE);
    unsigned const uPageAddress = S_AXI_BASEADDR;
    unsigned const uPageOffset = uPageAddress - (uPageAddress & (~(uPageSize-1)));

    int fd = open ("/dev/mem", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Can't open /dev/mem!\n");
        return -1;
    }

    void *pMem = mmap(NULL, uPageSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, uPageAddress); //memory map
    if (pMem == NULL) {
        fprintf(stderr, "Can't map address 0x%x!\n", uPageAddress);
        close(fd);
        return -1;
    }

    unsigned *baseRegister = (pMem + uPageOffset);

    double const maxPWM = regDbgMaxPWM;

    int changed = 0;
    if (mtemp >= 0.0) {
        if (modeDebug) printf("(Debug) write 0x%x to %p\n", (unsigned) (mtemp * 100.0), dbgRegAddrMaxTemp);
        regMaxTemp = (unsigned) (mtemp * 100.0);
        changed++;
    }
    if (ltemp >= 0.0) {
        if (modeDebug) printf("(Debug) write 0x%x to %p\n", (unsigned) (ltemp * 100.0), dbgRegAddrLowTemp);
        regLowTemp = (unsigned) (ltemp * 100.0);
        changed++;
    }

    if (smooth >= 0) {
        if (modeDebug) printf("(Debug) write 0x%x to %p\n", (unsigned) smooth, dbgRegAddrSmooth);
        regSmooth = (unsigned) (smooth);
        changed++;
    }

    if (opAuto >= 0.0) {
        if (modeDebug) printf("(Debug) write 0x%x to %p\n", (unsigned) 0, dbgRegAddrFixed);
        regFixed = (unsigned) (0);
        changed++;
    } else if (opFixed >= 0.0) {
        unsigned duty =  ((unsigned) (maxPWM * (opFixed / 100.0))) & 0xFFF;
        unsigned dutyReg = 0x1000 | duty;
        if (modeDebug) printf("(Debug) write 0x%x to %p\n", dutyReg, dbgRegAddrFixed);
        regFixed = dutyReg;
        changed++;
    }

    // Settings were changed, conitnue only if explicitly wished for
    if (changed && modeRead <= 1) {
        return 0;
    }

    unsigned int retLines = 0;
    do {
        if (retLines) {
            usleep(updateInt * 1000);
            printf("\r\033[%dA", retLines);
            retLines=0;
        }

        printf("Temperatur:          %.2f C       \n", (double) regTemp / 100.0); retLines++;
        printf("Fan Duty Cycle:      %.2f %       \n", 100.0 * regDbgPWM / maxPWM); retLines++;
        printf("                                  \n"); retLines++;
        printf("Low Temperatur:      %.2f C       \n", (double) regLowTemp / 100.0); retLines++;
        printf("Max Temperatur:      %.2f C       \n", (double) regMaxTemp / 100.0); retLines++;
        printf("Smooth Divisor:      %u           \n", regSmooth); retLines++;
        if (modeDebug) {
        printf("                                  \n"); retLines++;
        printf("(Debug) State:       %s           \n", fanStates[regDbgState]); retLines++;
        printf("(Debug) PWM:         %.2f         \n", regDbgPWM ); retLines++;
        printf("(Debug) Use PWM:     %.2f         \n", regDbgUsePWM ); retLines++;
        printf("(Debug) Last PWM:    %.2f         \n", regDbgLastPWM ); retLines++;
        printf("(Debug) Real Temp:   %.2f C       \n", regDbgRealTemp / 100.0 ); retLines++;
        printf("(Debug) Use Temp:    %.2f C       \n", regDbgUseTemp / 100.0 ); retLines++;
        printf("(Debug) Last Temp:   %.2f C       \n", regDbgLastTemp / 100.0 ); retLines++;
        printf("(Debug) Temp Error:  %.2f         \n", regDbgTempError / 100.0 ); retLines++;
        printf("(Debug) Linear:      %.2f         \n", regDbgLinear ); retLines++;
        printf("(Debug) Max PWM:     %.2f         \n", regDbgMaxPWM); retLines++;
        }

    } while(modeWatch);

    close(fd);
    return 0;
}

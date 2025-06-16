#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define FAN_PATH "/proc/acpi/ibm/fan"
#define SEN_PATH "/proc/acpi/ibm/thermal"
#define PID_FILE "/run/tpfan.pid"

typedef enum EFanLevel {
    EFanLevel0,
    EFanLevel1,
    EFanLevel2,
    EFanLevel3,
    EFanLevel4,
    EFanLevel5,
    EFanLevel6,
    EFanLevel7,
    EFanLevelAuto,
    EFanLevelDisengaged,
    EFanLevelFullSpeed,
} EFanLevel;

typedef struct TempSpeed{
    long m_iMinTemp;
    long m_iMaxTemp;
    EFanLevel m_eFanLevel;
} TempSpeed;

static TempSpeed g_aTempSpeeds[] = {
    { 0, 60, EFanLevelAuto },
    { 60, 70, EFanLevel7 },
    { 70, 0x7fffffff, EFanLevelDisengaged }
};

static size_t g_iCurrentSpeed = 2;
static int g_iInterrupt = 0;

void SetFanLevel(EFanLevel eLevel){
    FILE* pFile = fopen(FAN_PATH, "w");
    if(!pFile){
        return;
    }

    switch(eLevel){
    case(EFanLevel0):
        fputs("level 0", pFile);
        break;

    case(EFanLevel1):
        fputs("level 1", pFile);
        break;

    case(EFanLevel2):
        fputs("level 2", pFile);
        break;    
    
    case(EFanLevel3):
        fputs("level 3", pFile);
        break;

    case(EFanLevel4):
        fputs("level 4", pFile);    
        break;

    case(EFanLevel5):
        fputs("level 5", pFile);    
        break;

    case(EFanLevel6):
        fputs("level 6", pFile);    
        break;

    case(EFanLevel7):
        fputs("level 7", pFile);
        break;

    case(EFanLevelDisengaged):
        fputs("level disengaged", pFile);    
        break;

    case(EFanLevelFullSpeed):
        fputs("level full-speed", pFile);    
        break;

    default:
        fputs("level auto", pFile);
        break;
    }

    fclose(pFile);
}

long GetTemperature(){
    static long iTemperature = 0;
    char szData[32];
    size_t sizeData = 0;
    FILE* pFile = fopen(SEN_PATH, "r");
    if(!pFile){
        return iTemperature;
    }
    
    iTemperature = 0;
    for(char c = fgetc(pFile); c > 0; c = fgetc(pFile)){
        if(c == '-' || (c >= '0' && c <= '9')){
            if(sizeData < 30){
                szData[sizeData++] = c;
            }
        }
        else if(sizeData > 0){
            szData[sizeData] = 0;
            long iTemp = strtol(&szData[0], NULL, 10);
            if(iTemp > iTemperature){
                iTemperature = iTemp;
            }

            sizeData = 0;
        }
    }

    if(!sizeData){
        return iTemperature;
    }

    szData[sizeData] = 0;
    long iTemp = strtol(&szData[0], NULL, 10);
    if(iTemp > iTemperature){
        iTemperature = iTemp;
    }

    return iTemperature;
}

// Signal handler that will just set a global variable
void SigHandler(int iSignal){
    g_iInterrupt = iSignal;
}

int main(int iArgs, char** aArgs){    
    long iTemperature = GetTemperature();

    // Initialize signal handler
    struct sigaction stSigHandler;
    memset(&stSigHandler, 0, sizeof(struct sigaction));
    stSigHandler.sa_handler = SigHandler;
    
    { // Check that we can open all required files.
        FILE* pFile = fopen(FAN_PATH, "w");
        if(!pFile){
            printf("Cannot open \"%s\"!\n", FAN_PATH);
            return -1;
        }
        fclose(pFile);

        pFile = fopen(SEN_PATH, "r");
        if(!pFile){
            printf("Cannot open \"%s\"!\n", SEN_PATH);
            return -1;
        }
        fclose(pFile);
    }

    // Check the pid file to see if this app is already open.
    FILE* pPIDFile = fopen(PID_FILE, "r");
    if(pPIDFile){
        fclose(pPIDFile);
        printf("tpfan is likely already active, if nessisary pkill tpfan and delete \"%s\"!\n", PID_FILE);
        return -1;
    }

    pPIDFile = fopen(PID_FILE, "w+");
    if(!pPIDFile){
        puts("Unable to open PID File!");
        return -1;
    }

    // Set the signal handler to handle terminating signals.
    if(sigaction(SIGHUP, &stSigHandler, NULL) || sigaction(SIGINT, &stSigHandler, NULL) ||
        sigaction(SIGTERM, &stSigHandler, NULL) || sigaction(SIGUSR2, &stSigHandler, NULL)){
        puts("Failed to initialize signal handlers!");
        return -1;
    }

    // Fork this process to the background.
    pid_t pidChild = fork();
    if(pidChild < 0){
        puts("Unable to fork process!");
        return -1;
    }
    else if(pidChild > 0){
        printf("Child process %d created!\n", pidChild);
        fprintf(pPIDFile, "%d", pidChild);
        fclose(pPIDFile);
        return 0;
    }
    
    // Mainloop, exit once a terminating signal is hit.
    while(!g_iInterrupt){
        sleep(1);
        
        iTemperature = GetTemperature();
        if(iTemperature < g_aTempSpeeds[g_iCurrentSpeed].m_iMinTemp || iTemperature > g_aTempSpeeds[g_iCurrentSpeed].m_iMaxTemp){
            for(size_t i = 0; i < sizeof(g_aTempSpeeds) / sizeof(TempSpeed); ++i){
                if(iTemperature >= g_aTempSpeeds[i].m_iMinTemp && iTemperature <= g_aTempSpeeds[i].m_iMaxTemp){
                    g_iCurrentSpeed = i;
                    break;
                }
            }
        }

        SetFanLevel(g_aTempSpeeds[g_iCurrentSpeed].m_eFanLevel);
    }

    SetFanLevel(EFanLevelAuto);
    remove(PID_FILE);
}
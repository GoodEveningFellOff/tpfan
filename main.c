#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

#define FAN_PATH "/proc/acpi/ibm/fan"
#define SEN_PATH "/proc/acpi/ibm/thermal"
#define CONFIG_FILE "/etc/tpfan.conf"

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

static TempSpeed* g_aTempSpeeds = NULL;
static size_t g_sizeTempSpeeds = 0;
static size_t g_iCurrentSpeed = 0;

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

void LoadConfig(){
    // Check if the file has been changed since we last checked it.
    static time_t timeLast = 0;
    struct stat statFile;
    if(stat(CONFIG_FILE, &statFile) == -1 || statFile.st_mtim.tv_sec == timeLast){
        return;
    }
    timeLast = statFile.st_mtim.tv_sec;

    FILE* pFile = fopen(CONFIG_FILE, "r");
    if(!pFile){
        return;
    }

    size_t iLines = 0;
    {
        size_t sizeLine = 0;
        int c;
        while((c = fgetc(pFile)) != -1){
            ++sizeLine;
            if(c == '\n'){
                if(sizeLine > 1){
                    ++iLines;
                }
                sizeLine = 0;
            }
        }
    }

    if(iLines == 0){
        fclose(pFile);
        return;
    }

    rewind(pFile);
    char szLine[1024];
    char* pCursor = NULL;

    // Free the old config.
    if(g_aTempSpeeds){
        free(g_aTempSpeeds);
        g_aTempSpeeds = NULL;
    }
    g_sizeTempSpeeds = 0;
    
    // Allocate enough space for the new config.
    g_aTempSpeeds = malloc(iLines * sizeof(TempSpeed));
    if(!g_aTempSpeeds){
        fclose(pFile);
        return;
    }

    while(fgets(szLine, sizeof(szLine), pFile)){
        pCursor = szLine;
        
        // Look for the start of the first number.
        while(*pCursor && *pCursor != '-' && (*pCursor < '0' || *pCursor > '9')){
            ++pCursor;
        }
        if(!*pCursor){
            continue;
        }

        // Convert this to the minimum temperature.
        g_aTempSpeeds[g_sizeTempSpeeds].m_iMinTemp = strtol(pCursor, &pCursor, 10);
        if(!pCursor || !*pCursor){
            continue;
        }

        // Look for the start of the second number.
        while(*pCursor && *pCursor != '-' && (*pCursor < '0' || *pCursor > '9')){
            ++pCursor;
        }
        if(!*pCursor){
            continue;
        }

        // Convert this to the maximum temperature.
        g_aTempSpeeds[g_sizeTempSpeeds].m_iMaxTemp = strtol(pCursor, &pCursor, 10);
        if(!pCursor || !*pCursor){
            continue;
        }

        // Look for the fan speed
        while(*pCursor && *pCursor != 'a' && *pCursor != 'd' && *pCursor != 'f' && (*pCursor < '0' || *pCursor > '7')){
            ++pCursor;
        }
        if(!*pCursor){
            continue;
        }

        // Dont really need to match the full string of the fan speed, just need the first character.
        switch(*pCursor){
        case('d'):
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevelDisengaged;
            break;
            
        case('f'):
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevelFullSpeed;
            break;
            
        case('0'):
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevel0;
            break;
            
        case('1'):
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevel1;
            break;
            
        case('2'):
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevel2;
            break;
            
        case('3'):
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevel3;
            break;
            
        case('4'):
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevel4;
            break;
            
        case('5'):
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevel5;
            break;
            
        case('6'):
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevel6;
            break;
            
        case('7'):
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevel7;
            break;

        default:
            g_aTempSpeeds[g_sizeTempSpeeds].m_eFanLevel = EFanLevelAuto;
            break;
        }

        ++g_sizeTempSpeeds;
        if(g_sizeTempSpeeds == iLines){
            break;
        }
    }

    fclose(pFile);
    return;
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
    for(int c = fgetc(pFile); c > 0; c = fgetc(pFile)){
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

void SigHandler(int iSignal){
    SetFanLevel(EFanLevelAuto);
    exit(0);
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

    // Set the signal handler to handle terminating signals.
    #define DEF_SIGACTION(n) (sigaction(n, &stSigHandler, NULL))
    if(DEF_SIGACTION(SIGHUP) || DEF_SIGACTION(SIGINT) || DEF_SIGACTION(SIGTERM)){
    #undef DEF_SIGACTION
        puts("Failed to initialize signal handlers!");
        return -1;
    }
    
    
    while(1){
        sleep(1);

        LoadConfig();

        if(!g_aTempSpeeds){
            SetFanLevel(EFanLevelAuto);
            continue;
        }

        if(g_iCurrentSpeed >= g_sizeTempSpeeds){
            g_iCurrentSpeed = 0;
        }
        
        iTemperature = GetTemperature();
        if(iTemperature < g_aTempSpeeds[g_iCurrentSpeed].m_iMinTemp || iTemperature > g_aTempSpeeds[g_iCurrentSpeed].m_iMaxTemp){
            for(size_t i = 0; i < g_sizeTempSpeeds; ++i){
                if(iTemperature >= g_aTempSpeeds[i].m_iMinTemp && iTemperature <= g_aTempSpeeds[i].m_iMaxTemp){
                    g_iCurrentSpeed = i;
                    break;
                }
            }
        }

        SetFanLevel(g_aTempSpeeds[g_iCurrentSpeed].m_eFanLevel);
    }
}
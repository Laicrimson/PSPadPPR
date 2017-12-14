/**
  History:
  0.8.2  Support PspadPPR.cfg to skip directories
  0.8.1  Show project name
  0.8    First release
**/
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <stdio.h>

#define DEBUG_MODE 1

#ifdef DEBUG_MODE
#define DBGPRINTF(s)    printf s;
#else
#define DBGPRINTF(s)
#endif

static char SIGNATURE[] = "By Crimson, PSPAD PPR GENERATOR ver 0.8.2";

BOOLEAN PrintDirAndFiles(FILE * fp, char * dir_str, unsigned int level, WIN32_FIND_DATA * pFindFileData, char * temp_str);
BOOLEAN ListFilesInDir(FILE * fp, char * dir_str, unsigned int level);

BOOLEAN OS_X64 = FALSE;
PVOID OldValue;

char CurrentDir[256];
unsigned int CurrentDirLen;

char ProjectFile[64] = "PSPad.ppr";

char SkipFile1[] = ".";
char SkipFile2[] = "..";

//0.8.2 >>>
char PspadCfgFileName[64] = "PSPadPPR.cfg";  // default configuration file name
#define MAX_CFG_SKIP_DIR 64
char CfgSkipDirList[MAX_CFG_SKIP_DIR][256];
unsigned int CfgSkipDirCnt;
//0.8.2 <<<

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
void IdentifyOS()
{
	LPFN_ISWOW64PROCESS fnIsWow64Process;
	BOOL bIsWow64 = FALSE;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

	if (NULL != fnIsWow64Process){
	        if (!fnIsWow64Process(GetCurrentProcess(),(PBOOL)&bIsWow64)){
			// handle error
			DBGPRINTF(("IsWow64Process fail...\n"));
	        }
	}

	if(bIsWow64)
		OS_X64 = TRUE;
}
typedef BOOL (WINAPI *LPFN_WOW64DISABLEWOW64FSREDIRECTION) (PVOID * OldValue);

BOOLEAN PatchForX64()
{
	LPFN_WOW64DISABLEWOW64FSREDIRECTION fnWow64DisableWow64FsRedirection;
	PVOID OldValue;

	fnWow64DisableWow64FsRedirection = (LPFN_WOW64DISABLEWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32")),"Wow64DisableWow64FsRedirection");

	if (NULL != fnWow64DisableWow64FsRedirection){
	        if(!fnWow64DisableWow64FsRedirection(&OldValue)){
			// handle error
			DBGPRINTF(("fnWow64DisableWow64FsRedirection fail...\n"));
			return FALSE;
	        }
	}
	return TRUE;
}

BOOLEAN PrintDirAndFiles(FILE * fp, char * dir_str, unsigned int level, WIN32_FIND_DATA * pFindFileData, char * temp_str)
{
	int i;
	if(strcmp(pFindFileData->cFileName, SkipFile1) && strcmp(pFindFileData->cFileName, SkipFile2)){
		for(i=0;i<level;i++) fprintf(fp, "\t");
		if(pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			// show '-' and dir name without relative path
               		fprintf(fp, "-%s\n", pFindFileData->cFileName);
			// recursive show files in dir
			if(dir_str){
				strcpy(temp_str, dir_str);
				strcat(temp_str, "\\");
				strcat(temp_str, pFindFileData->cFileName);
			}else{
				strcpy(temp_str, pFindFileData->cFileName);
			}
			if(!ListFilesInDir(fp, temp_str, level+1))
				return FALSE;
		}else{
			// show file name with relative path
			if(dir_str) fprintf(fp, "%s\\", dir_str);
			fprintf(fp, "%s\n", pFindFileData->cFileName);
		}
	}
	return TRUE;
}

//0.8.2 >>>
BOOLEAN FoundMatchedSkipDir(char * dir_str)
{
  int i;
  if(dir_str != NULL){
    for(i=0;i<CfgSkipDirCnt;i++){
      if(strcmp(dir_str, CfgSkipDirList[i])==0){
        DBGPRINTF(("Skip folder %s\n", dir_str));
        return TRUE;
      }
    }
  }
  return FALSE;
}
//0.8.2 <<<

BOOLEAN ListFilesInDir(FILE * fp, char * dir_str, unsigned int level)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
        DWORD dwError;
	char temp_str[256];

//0.8.2 >>>
  if(FoundMatchedSkipDir(dir_str)){
    return TRUE;
  }
//0.8.2 <<<

	if(dir_str){
		strcpy(temp_str, dir_str);
		strcat(temp_str, "\\*.*");
	}else{
		strcpy(temp_str, "*.*");
	}
	hFind = FindFirstFile(temp_str, &FindFileData);
	if(hFind == INVALID_HANDLE_VALUE){
		DBGPRINTF(("Invalid File Handle. GetLastError reports %x\n", GetLastError()));
		return FALSE;
	}
  if(!PrintDirAndFiles(fp, dir_str, level, &FindFileData, temp_str))
    return FALSE;

	while (FindNextFile(hFind, &FindFileData) != FALSE){
    if(!PrintDirAndFiles(fp, dir_str, level, &FindFileData, temp_str))
      return FALSE;
  }

	FindClose(hFind);
	dwError = GetLastError();
	if( dwError != ERROR_NO_MORE_FILES){
		DBGPRINTF(("FindNextFile error. Error is %x\n", dwError));
		return FALSE;
	}

	return TRUE;
}

BOOLEAN Pre_ProcessFile(FILE * fp)
{
	int i;


	fprintf(fp, "[Config]\n");
	if(CurrentDir[CurrentDirLen-1] == '\\' || CurrentDir[CurrentDirLen-1] == '/'){
		fprintf(fp, "Compilator.FileName=%spspad_mk.bat\n", CurrentDir);
	}else{
		fprintf(fp, "Compilator.FileName=%s\\pspad_mk.bat\n", CurrentDir);
	}
	fprintf(fp, "Compilator.PSPar=%%Input%%\n");
	fprintf(fp, "Compilator.DefaultDir=%s\n", CurrentDir);
	fprintf(fp, "Compilator.SaveAll=0\n");
	fprintf(fp, "Compilator.Capture=1\n");
	fprintf(fp, "Compilator.HideOutput=0\n");
	fprintf(fp, "Compilator.LogType=1\n");
	fprintf(fp, "DefaultDir=%s\n", CurrentDir);
	fprintf(fp, "DefaultCPIndex=0\n");
	fprintf(fp, "LogtoEnd=1\n");
	fprintf(fp, "DontOpen=0\n");
	fprintf(fp, "AbsolutePath=0\n");
	fprintf(fp, "FileFormat=0\n");
	fprintf(fp, "ProjectFilesOnly=0\n");
	fprintf(fp, "[Project tree]\n");
	// root dir
	if(CurrentDirLen == 3){
		// if CurrentDir is root of partition
		fprintf(fp, "%s\n", CurrentDir);
	}else{
		// find last slash
		for(i=CurrentDirLen;i>0;i--){
			if(CurrentDir[i] == '\\' || CurrentDir[i] == '/')
				break;
		}
		fprintf(fp, "%s\n", &CurrentDir[i+1]);
	}

	return TRUE;
}

BOOLEAN Post_ProcessFile(FILE * fp)
{
	fprintf(fp, "[Open project files]\n");
	fprintf(fp, "[Selected Project Files]\n");
	fprintf(fp, "Main=\n");

	return TRUE;
}

//0.8.1 >>>
VOID GenerateProjectName(char * argv)
{
  int i;

  if(argv[0] == '.'){
    if(CurrentDirLen == 3){
      // if CurrentDir is root of partition
      return;
    }else{
      // find last slash
      for(i=CurrentDirLen;i>0;i--){
        if(CurrentDir[i] == '\\' || CurrentDir[i] == '/') {
          break;
        }
      }
      strcpy (ProjectFile, "PS-");
      strcat (ProjectFile, &CurrentDir[i+1]);
      strcat (ProjectFile, ".ppr");
  	}
  }else{
    strcpy(ProjectFile, argv);
    strcat (ProjectFile, ".ppr");
  }
}
//0.8.1 <<<

//0.8.2 >>>
enum {
  CfgSecInit,
  CfgSecSkipDir,
  CfgSecMax
};

void GetCfgParam(FILE * fp_cfg)
{
  char str[256];
  int len;
  unsigned int state_machine = CfgSecInit;

  while(fgets(str, sizeof(str), fp_cfg) != NULL){
    // this w/a for the 0x0A in the end of string
    len = strlen(str);
    if(str[len-1] == 0x0A){
      str[len-1] = 0;
    }
    DBGPRINTF(("PspadCfgFile str: %s\n", str));

    if(str[0] == '['){
      if(strcmp(str, "[SKIP_DIR]")==0){
        state_machine = CfgSecSkipDir;
        continue;
      }
    }

    switch(state_machine){
    case CfgSecSkipDir:
      if(CfgSkipDirCnt < MAX_CFG_SKIP_DIR){
        strcpy(CfgSkipDirList[CfgSkipDirCnt++], str);
      }else{
        DBGPRINTF(("Config file error: More than %x skip directories.\n", MAX_CFG_SKIP_DIR));
      }
      break;
    }
  }
}

VOID ProcessCfgFile (char * CfgFile)
{
	FILE * fp_cfg;

  fp_cfg = fopen(CfgFile, "r");
  if(fp_cfg){
    GetCfgParam(fp_cfg);
  	fclose(fp_cfg);
  }
}
//0.8.2 <<<

int main(int argc, char * argv[])
{
	FILE * fp;
  char temp_str[256];
	int i;

	IdentifyOS();
	if(OS_X64){
		if(!PatchForX64()){
			DBGPRINTF(("Wow64DisableWow64FsRedirection fail... %x\n", GetLastError()));
			return -1;
		}
	}

        CurrentDirLen = GetCurrentDirectory(sizeof(CurrentDir), CurrentDir);
	if(!CurrentDirLen){
		DBGPRINTF(("GetCurrentDirectory fail... %x\n", GetLastError()));
		return -1;
	}

//0.8.1 >>>
  if(argc > 1){
    GenerateProjectName(argv[1]);
  }
//0.8.1 <<<

//0.8.2 >>>
  strcpy(temp_str, CurrentDir);
  strcat(temp_str, "\\");
  strcat(temp_str, PspadCfgFileName);
  ProcessCfgFile(temp_str);
//0.8.2 <<<

	fp = fopen(ProjectFile, "w");
	if(!fp){
		DBGPRINTF(("Open file fail... %s\n", ProjectFile));
		return -1;
	}

	if(!Pre_ProcessFile(fp)){
		DBGPRINTF(("Pre_ProcessFile fail...\n"));
		fclose(fp);
		return -1;
	}

	ListFilesInDir(fp, NULL, 1);

	if(!Post_ProcessFile(fp)){
		DBGPRINTF(("Pre_ProcessFile fail...\n"));
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

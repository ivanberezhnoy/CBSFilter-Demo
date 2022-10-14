/*
 * CBFS Filter 2022 C++ Edition - Demo Application
 *
 * Copyright (c) 2022 Callback Technologies, Inc. - All rights reserved. - www.callback.com
 *
 */

#include <iostream>
#include <tchar.h>

#ifdef _UNICODE
#include "../../../include/unicode/cbencrypt.h"
#else
#include "../../../include/cbencrypt.h"
#endif

// Specifies a global salt for encryption.
LPCTSTR gGlobalSalt = _T("TestSale");

// Some custom data can be stored in encrypted file header when OpenFile event is received.
#define ENC_HEADER_SIZE 1024

class SecureFolder : public CBEncrypt
{
public:
  virtual INT FireOpenFile(CBEncryptOpenFileEventParams *e)
  {
    return 0;
  }

  virtual INT FirePasswordNeeded(CBEncryptPasswordNeededEventParams *e)
  {
    return 0;
  }

  virtual INT FireError(CBEncryptErrorEventParams* e)
  {
    return 0;
  }
public:

  INT StartEncrypt(LPCTSTR folderPath, LPCTSTR password)
  {
    INT ret;

    ret = this->SetGlobalSalt(gGlobalSalt);
    if (ret != 0) return ret;

    ret = this->AddEncryptRule(folderPath, TRUE, ENC_HEADER_SIZE, cbfConstants::FILEENC_EM_DEFAULT, password);
    if (ret != 0) return ret;

    ret = this->Start(0);
    if (ret == 0) mFolderPath = _tcsdup(folderPath);

    return ret;
  }

  VOID StopEncrypt()
  {
    if (mFolderPath == NULL) return;
    this->Stop();
    this->RemoveEncryptRule(mFolderPath);
    free(mFolderPath);
    mFolderPath = NULL;
  }
private:
  LPTSTR mFolderPath = NULL;
};

LPCTSTR gProductId                 = _T("{713CC6CE-B3E2-4fd9-838D-E28F558F6866}");
LPCTSTR gAltitudeFakeValueForDebug = _T("360000");

int optcmp(char* arg, const char* opt)
{
  while (1)
  {
    if (*arg >= 'A' && *arg <= 'Z')
      *arg = *arg - 'A' + 'a';
    if (*arg != *opt)
      return 0;
    if (*arg == 0)
      return 1;
    arg++;
    opt++;
  }
}

void banner(void)
{
  printf("CBFS Filter Copyright (c) Callback Technologies, Inc.\n\n");
}

void usage(void)
{
  printf("Usage: securefolder [-<switch 1> ... -<switch N>] <folder path> <password> [<items domain>]\n\n");
  printf("<Switches>\n");
  printf("  -drv {cab_file} - Install drivers from CAB file\n");
  printf("  -- Stop switches scanning\n\n");
}

void check_driver()
{
  int state;
  CBEncrypt encrypt;

  state = encrypt.GetDriverStatus(gProductId);
  if (state == SERVICE_RUNNING)
  {
    LONG64 version;
    version = encrypt.GetDriverVersion(gProductId);
    printf("CBFSFilter driver is installed, version: %d.%d.%d.%d\n",
      (int)((version & 0x7FFF000000000000) >> 48),
      (int)((version & 0xFFFF00000000) >> 32),
      (int)((version & 0xFFFF0000) >> 16),
      (int)( version & 0xFFFF));
  }
  else
  {
    printf("CBFSFilter driver is not installed\n");
    exit(0);
  }
}

LPTSTR a2t(char* source)
{
  LPTSTR result = NULL;
  if (source == NULL)
  {
    result = (LPTSTR)malloc(sizeof(TCHAR));
    result[0] = 0;
    return result;
  }
  else
  {
#ifdef _UNICODE
    int wstrLen = MultiByteToWideChar(CP_ACP, 0, source, -1, NULL, 0);
    if (wstrLen > 0)
    {
      result = (LPTSTR)malloc((wstrLen + 1) * sizeof(TCHAR));

      if (MultiByteToWideChar(CP_ACP, 0, source, -1, result, wstrLen) == 0)
        return NULL;
      else
        return result;
    }
    else
      return NULL;
#else
    return _tcsdup(source);
#endif
  }
}

int main(int argc, char* argv[])
{
  SecureFolder secureFolder;

  LPTSTR cab_file = NULL;
  LPTSTR folder_path = NULL, password = NULL;

  int drv_reboot = 0;
  int opt_terminate = 0;
  int opt_password_size = 0;

  int argi, arg_len,
  int stop_opt = 0;
  int flags = 0;

  banner();

  if (argc < 2)
  {
    usage();
    check_driver();
    return 0;
  }

  int retVal;
  for (argi = 1; argi < argc; argi++)
  {
    arg_len = (int)strlen(argv[argi]);
    if (arg_len > 0)
    {
      if ((argv[argi][0] == '-') && !stop_opt)
      {
        if (arg_len < 2)
        {
          fprintf(stderr, "invalid option: '%s'", argv[argi]);
          return 1;
        }
        else
        if (optcmp(argv[argi], "--"))
          break;
        else
        if (optcmp(argv[argi], "-drv"))
        {
          argi++;
          if (argi < argc)
          {
            printf("Installing drivers from '%s'\n", argv[argi]);

            drv_reboot = secureFolder.Install(a2t(argv[argi]), gProductId, NULL, gAltitudeFakeValueForDebug, cbfConstants::INSTALL_REMOVE_OLD_VERSIONS);

            retVal = secureFolder.GetLastErrorCode();
            if (0 != retVal)
            {
              if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                fprintf(stderr, "Drivers are not installed due to insufficient privileges. Please, run installation with administrator rights");
              else
                fprintf(stderr, "Drivers are not installed, error %s", secureFolder.GetLastError());
              return retVal;
            }

            printf("Drivers installed successfully");
            if (drv_reboot != 0)
            {
              printf(", reboot is required\n");
              exit(0);
            }
            else
              printf("\n");

            argi++;
            break;
          }
        }
        else
          fprintf(stderr, "invalid option: '%s'", argv[argi]);        
      }
      else
      if (argv[argi][0] != '-')
      {
        break;
      }
    }
  }
  if ((argc - argi) < 4)
  {
    usage();
    return 0;
  }

  folder_path = a2t(argv[argi++]);
  password    = a2t(argv[argi++]);

  retVal = secureFolder.Initialize(gProductId);
  if (0 != retVal)
  {
    fprintf(stderr, "Error: %s", secureFolder.GetLastError());
    return retVal;
  }

  retVal = secureFolder.StartEncrypt(folder_path, password);
  if (0 != retVal)
  {
    fprintf(stderr, "Error: %s", secureFolder.GetLastError());
    return retVal;
  }

  printf("Press Enter to stop securefolder\n");

  getc(stdin);

  printf("Stop the secureFolder\n");

  secureFolder.StopEncrypt();

  printf("Done\n");

  return 0;
}







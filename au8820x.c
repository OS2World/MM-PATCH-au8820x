/*
** Module   :AU8820X.C
** Abstract :Patch Aureal Vortex1 registers
**
** Copyright (C) 2002 Artem Davidenko
*/

/*
 coded by sNOa,
 thx to defan ;)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>

#define INCL_BASE
#include <os2.h>
#include <bsedos.h>

#define OEMHLP "OEMHLP$"
#define OEM_CAT  0x80
#define PCI_FUNC 0x0b
#define PCI_READ_CONFIG  3
#define PCI_WRITE_CONFIG 4
#define PCI_FIND_DEVICE  1

#pragma pack(1)

typedef struct _PCI_PARM {
   UCHAR PCISubFunc;
   union {
      struct {
         USHORT DeviceID;
         USHORT VendorID;
         UCHAR  Index;
      }Parm_Find_Dev;
      struct {
         ULONG  ClassCode;
         UCHAR  Index;
      }Parm_Find_ClassCode;
      struct {
         UCHAR  BusNum;
         UCHAR  DevFunc;
         UCHAR  ConfigReg;
         UCHAR  Size;
      }Parm_Read_Config;
      struct {
         UCHAR  BusNum;
         UCHAR  DevFunc;
         UCHAR  ConfigReg;
         UCHAR  Size;
         ULONG  Data;
      }Parm_Write_Config;
   };
} PCI_PARM;

typedef struct _PCI_DATA {
   UCHAR bReturn;
   union {
      struct {
         UCHAR HWMech;
         UCHAR MajorVer;
         UCHAR MinorVer;
         UCHAR LastBus;
      } Data_Bios_Info;
      struct {
         UCHAR  BusNum;
         UCHAR  DevFunc;
      }Data_Find_Dev;
      struct {
         ULONG  Data;
      }Data_Read_Config;
   };
} PCI_DATA;

#pragma pack()


static void do_ioctl(HFILE hDevice, PCI_PARM *pciparm, PCI_DATA *pcidata)
{
    USHORT rc;
    ULONG pcbParmLen=sizeof(PCI_PARM), pcbDataLen=sizeof(PCI_DATA);

    if( rc = DosDevIOCtl2(hDevice, OEM_CAT, PCI_FUNC, pciparm, sizeof(PCI_PARM),
                         &pcbParmLen, pcidata, sizeof(PCI_DATA), &pcbDataLen) )
    {
        switch(rc)
        {
        case 0x86:
            printf("Device not found\n");
            break;
        default:
            printf("IOCtl failed, rc=0x%04x\n", rc);
        }
        DosClose(hDevice);
        exit(1);
    }

}

static void write_pci_reg(HFILE hf, PCI_PARM *pparm, unsigned int reg, unsigned char b)
{
    PCI_DATA pd;

    pparm->PCISubFunc=PCI_WRITE_CONFIG;
    pparm->Parm_Write_Config.ConfigReg=reg;
    pparm->Parm_Write_Config.Size=1;
    pparm->Parm_Write_Config.Data=b;
    do_ioctl(hf, pparm, &pd);
}

static unsigned char read_pci_reg(HFILE hf, PCI_PARM *pparm, unsigned int reg)
{
    PCI_DATA pd;

    pparm->PCISubFunc=PCI_READ_CONFIG;
    pparm->Parm_Read_Config.ConfigReg=reg;
    pparm->Parm_Read_Config.Size=1;
    do_ioctl(hf, pparm, &pd);
    return(pd.Data_Read_Config.Data&0xFF);
}

int main()
{
    ULONG action;
    HFILE hf;
    PCI_PARM pciparm;
    PCI_DATA pcidata;
    UCHAR vvalue;

    setbuf(stdout, NULL);

    if(DosOpen(OEMHLP, &hf, &action, 0L, FILE_NORMAL,
               OPEN_ACTION_FAIL_IF_NEW|OPEN_ACTION_OPEN_IF_EXISTS,
               OPEN_ACCESS_READONLY|OPEN_SHARE_DENYNONE, 0L))

    {
        puts("Can't open the Resource Manager!");
        return(1);
    }

    puts("vortex1 searching...");

    pciparm.PCISubFunc=PCI_FIND_DEVICE;
    pciparm.Parm_Find_Dev.DeviceID=0x0001; // Device ID
    pciparm.Parm_Find_Dev.VendorID=0x12eb; // Vendor ID
    pciparm.Parm_Find_Dev.Index=0;

    do_ioctl(hf, &pciparm, &pcidata);

    printf("found on bus number %u, device number %u, function number %u\n", pcidata.Data_Find_Dev.BusNum,
           (pcidata.Data_Find_Dev.DevFunc)>>3, pcidata.Data_Find_Dev.DevFunc&0x07);

    pciparm.Parm_Read_Config.BusNum=pcidata.Data_Find_Dev.BusNum;
    pciparm.Parm_Read_Config.DevFunc=pcidata.Data_Find_Dev.DevFunc;

    vvalue = read_pci_reg(hf, &pciparm, 0x40);

    printf( "current value: 0x%02x\n", vvalue );

    if( vvalue == 0xff )
    {
        puts("ok");
    }
    else
    {
        write_pci_reg(hf, &pciparm, 0x40, 0xff);
        printf( "new value: 0x%02x\n", read_pci_reg(hf, &pciparm, 0x40) );
    }

    DosClose(hf);

    return(0);
}


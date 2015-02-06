

#ifndef _CFG_BT_FILE_H
#define _CFG_BT_FILE_H


// the record structure define of bt nvram file
#if defined(MTK_MT6611) || defined(MTK_MT6612) || defined(MTK_MT6616)
typedef struct
{
    unsigned char addr[6];
    unsigned char CapId[1];
    unsigned char Codec[1];
} ap_nvram_btradio_mt6610_struct;
#elif defined(MTK_MT6620) || defined(MTK_MT6628)
typedef struct
{
    unsigned char addr[6];
    unsigned char Voice[2];
    unsigned char Codec[4];
    unsigned char Radio[6];
    unsigned char Sleep[7];
    unsigned char BtFTR[2];
    unsigned char TxPWOffset[3];
} ap_nvram_btradio_mt6610_struct;
#else
typedef struct
{
    unsigned char addr[6];                      /*0*/
    unsigned char CapId[1];                     /*6*/
    unsigned char LinkKeyType[1];               /*7*/
    unsigned char UintKey[16];                  /*8*/
    unsigned char Encryption[3];                /*24*/
    unsigned char PinCodeType[1];               /*27*/	
    unsigned char Voice[2];                     /*28*/
    unsigned char Codec[4];                     /*30*/
    unsigned char Radio[6];                     /*34*/
    unsigned char Sleep[7];                     /*40*/
    unsigned char BtFTR[2];                     /*47*/
    unsigned char TxPWOffset[3];                /*49*/
    unsigned char ECLK_SEL[1];                  /*52*/
    unsigned char Reserved1[1];                 /*53*/
    unsigned char Reserved2[2];                 /*54*/
    unsigned char Reserved3[4];                 /*56*/
    unsigned char Reserved4[4];                 /*60*/
    unsigned char Reserved5[16];                /*64*/
    unsigned char Reserved6[16];                /*80*/
} ap_nvram_btradio_mt6610_struct;
#endif

//the record size and number of bt nvram file
#define CFG_FILE_BT_ADDR_REC_SIZE    sizeof(ap_nvram_btradio_mt6610_struct)
#define CFG_FILE_BT_ADDR_REC_TOTAL   1

#endif



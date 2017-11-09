//
//  readdic.c
//  openwnn
//
//  Created by admin on 2017/11/08.
//  Copyright © 2017年 admin. All rights reserved.
//

#include "readdic.h"
//
//  main.c
//  openwnn
//
//  Created by admin on 2017/11/01.
//  Copyright © 2017年 admin. All rights reserved.
//


#include "nj_dic.h"
#include "nj_err.h"
#include "nj_lib.h"
#include "njd.h"
#include "njx_lib.h"
#include "nj_ext.h"
#include "OpenWnnJni.h"
#include "WnnJpnDic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <iconv.h>

#include <sys/stat.h>

#include "predef_table.h"
#define QUE_TYPE_EMPTY  0
#define QUE_TYPE_NEXT   0
#define QUE_TYPE_JIRI   1
#define QUE_TYPE_FZK    2
#define POS_DATA_OFFSET  0x20
#define POS_LEARN_WORD   0x24
#define POS_MAX_WORD     0x28
#define POS_QUE_SIZE     0x2C
#define POS_NEXT_QUE     0x30
#define POS_WRITE_FLG    0x34
#define POS_INDEX_OFFSET        0x3C
#define POS_INDEX_OFFSET2       0x40
#define QUE_SIZE(h)     ((NJ_UINT16)NJ_INT32_READ((h) + POS_QUE_SIZE))
#define LEARN_INDEX_TOP_ADDR(x) ((x) + (NJ_INT32_READ((x) + POS_INDEX_OFFSET)))
#define LEARN_INDEX_TOP_ADDR2(x) ((x) + (NJ_INT32_READ((x) + POS_INDEX_OFFSET2)))
#define LEARN_DATA_TOP_ADDR(x)  ((x) + (NJ_INT32_READ((x) + POS_DATA_OFFSET)))

#define LEARN_INDEX_BOTTOM_ADDR(x) (LEARN_DATA_TOP_ADDR(x) - 1)

#define LEARN_QUE_STRING_OFFSET 5

#define ADDRESS_TO_POS(x,adr)   (((adr) - LEARN_DATA_TOP_ADDR(x)) / QUE_SIZE(x))
#define POS_TO_ADDRESS(x,pos)   (LEARN_DATA_TOP_ADDR(x) + QUE_SIZE(x) * (pos))

#define GET_UINT16(ptr) ((((NJ_UINT16)(*(ptr))) << 8) | (*((ptr) + 1) & 0x00ff))



static FILE *fp;



#define GET_FPOS_FROM_DATA(x) ((NJ_UINT16)NJ_INT16_READ((x)+1) >> 7)
NJ_UINT16 get_fpos_from_data(NJ_UINT8 *x,NJ_UINT8 *d){
    printf("%p\n",x-d);
    printf("%x\n",*x);
    NJ_UINT16 p=(NJ_UINT16)NJ_INT16_READ((x)+1);
    printf("%x\n",p);
    return p >> 7;
};
#define GET_YSIZE_FROM_DATA(x) ((NJ_UINT8)((NJ_UINT16)NJ_INT16_READ((x)+1) & 0x7F))
NJ_UINT8 get_ysize_from_data(NJ_UINT8 *x,NJ_UINT8 *d){
    printf("%p\n",x-d);
    printf("%x\n",*x);
    NJ_UINT16 p=(NJ_UINT16)NJ_INT16_READ((x)+1);
    printf("%x\n",p);
    return p & 0x7F;
}

#define GET_BPOS_FROM_DATA(x) ((NJ_UINT16)NJ_INT16_READ((x)+3) >> 7)
NJ_UINT16 get_bpos_from_data(NJ_UINT8 *x,NJ_UINT8 *d){
    printf("%p\n",x-d);
    printf("%x\n",*x);
    NJ_UINT16 p=(NJ_UINT16)NJ_INT16_READ((x)+3);
    printf("%x\n",p);
    return p >> 7;
};
#define GET_KSIZE_FROM_DATA(x) ((NJ_UINT8)((NJ_UINT16)NJ_INT16_READ((x)+3) & 0x7F))
NJ_UINT8 get_ksize_from_data(NJ_UINT8 *x,NJ_UINT8 *d){
    printf("%p\n",x-d);
    printf("%x\n",*x);
    NJ_UINT16 p=(NJ_UINT16)NJ_INT16_READ((x)+3);
    printf("%x\n",p);
    return p & 0x7F;
}

#define GET_BPOS_FROM_EXT_DATA(x) ((NJ_UINT16)NJ_INT16_READ(x) >> 7)
#define GET_YSIZE_FROM_EXT_DATA(x) ((NJ_UINT8)((NJ_UINT16)NJ_INT16_READ(x) & 0x7F))

#define SET_BPOS_AND_YSIZE(x,bpos,ysize)                                \
NJ_INT16_WRITE((x), ((NJ_UINT16)((bpos) << 7) | ((ysize) & 0x7F)))
#define SET_FPOS_AND_YSIZE(x,fpos,ysize)                                \
NJ_INT16_WRITE(((x)+1), ((NJ_UINT16)((fpos) << 7) | ((ysize) & 0x7F)))
#define SET_BPOS_AND_KSIZE(x,bpos,ksize)                                \
NJ_INT16_WRITE(((x)+3), ((NJ_UINT16)((bpos) << 7) | ((ksize) & 0x7F)))

#define GET_TYPE_FROM_DATA(x) (*(x) & 0x03)
NJ_UINT8 get_type_from_data(NJ_UINT8 *x,NJ_UINT8 *d){
    printf("%p\n",x-d);
    printf("%x\n",*x);
    return *(x) & 0x03;
}
#define GET_UFLG_FROM_DATA(x) (*(x) >> 7)
#define GET_FFLG_FROM_DATA(x) ((*(x) >> 6) & 0x01)
NJ_UINT8 get_fflg_from_data(NJ_UINT8 *x,NJ_UINT8 *d){
    printf("%p\n",x-d);
    printf("%x\n",*x);
    NJ_UINT8 p=*(x) >> 6;
    printf("%x\n",p);
    return p & 0x01;
}
#define GET_MFLG_FROM_DATA(x) (*(x) & 0x10)
NJ_UINT8 get_mflg_from_data(NJ_UINT8 *x,NJ_UINT8 *d){
    printf("%p\n",x-d);
    printf("%x\n",*x);
    
    return *(x) & 0x10;
}



#define SET_TYPE_UFLG_FFLG(x,type,u,f)                                  \
(*(x) = (NJ_UINT8)(((type) & 0x03) |                                \
(((u) & 0x01) << 7) | (((f) & 0x01) << 6)))
#define SET_TYPE_ALLFLG(x,type,u,f,m)                                   \
(*(x) = (NJ_UINT8)(((type) & 0x03) |                                \
(((u) & 0x01) << 7) | (((f) & 0x01) << 6) | (((m) & 0x01) << 4)))

#define RESET_FFLG(x) (*(x) &= 0xbf)

#define STATE_COPY(to, from)                                    \
{ ((NJ_UINT8*)(to))[0] = ((NJ_UINT8*)(from))[0];            \
((NJ_UINT8*)(to))[1] = ((NJ_UINT8*)(from))[1];          \
((NJ_UINT8*)(to))[2] = ((NJ_UINT8*)(from))[2];          \
((NJ_UINT8*)(to))[3] = ((NJ_UINT8*)(from))[3]; }

#define USE_QUE_NUM(que_size, str_size)    \
( (((str_size) % ((que_size) - 1)) == 0)                           \
? ((str_size) / ((que_size) - 1))                                \
: ((str_size) / ((que_size) - 1) + 1) )

#define NEXT_QUE(que, max)  ( ((que) < ((max) - 1)) ? ((que) + 1) : 0 )

#define PREV_QUE(que, max)  ( ((que) == 0) ? ((max) - 1) : ((que) - 1) )

#define COPY_QUE(handle, src, dst)                                      \
nj_memcpy(POS_TO_ADDRESS((handle), (dst)), POS_TO_ADDRESS((handle), (src)), QUE_SIZE(handle))


#define INIT_HINDO          (-10000)

#define LOC_CURRENT_NO_ENTRY  0xffffffffU



#define NJ_INT16_READ(in)                                               \
(((((NJ_INT16)((in)[0])) << 8) & 0xff00U) + ((in)[1] & 0xffU))

NJ_UINT32 i4(NJ_UINT8 *d,int i){
    NJ_UINT32 p=(NJ_UINT32)NJ_INT32_READ(&d[i]);
    return p;
}

void p4(NJ_UINT8 *d,int i,char* hdr){
    printf("%s:%x%x%x%x\n",hdr,d[i],d[i+1],d[i+2],d[i+3]);
    fprintf(fp,"%s:%x%x%x%x\n",hdr,d[i],d[i+1],d[i+2],d[i+3]);
}
void p2(NJ_UINT8 *d,int i,char* hdr){
    printf("%s:%x%x\n",hdr,d[i],d[i+1]);
    fprintf(fp,"%s:%x%x\n",hdr,d[i],d[i+1]);
}
#define MAX_BUF 1024
//http://www.naturalsoftware.jp/entry/20100312/1268413354
void get_moji(NJ_UINT8 *ptr,NJ_UINT8 siz,char* ret){
    printf("%p,%d,%x\n",ptr,siz,ptr);
    char utf8[MAX_BUF];
    utf8[0]='\0';
    //strncpy(utf8,ptr,siz);
    int i;
    for(i=0;i<siz;i++){
        utf8[i]=*ptr;
        printf("%x\n",*ptr);
        ptr++;
    }
    utf8[i]='\0';
    
    
    char    inbuf[MAX_BUF+1] = { 0 };
    char    outbuf[MAX_BUF+1] = { 0 };
    char    *in = inbuf;
    char    *out = outbuf;
    size_t  in_size = (size_t)MAX_BUF;
    size_t  out_size = (size_t)MAX_BUF;
    iconv_t ic = iconv_open("SJIS", "UTF-8");
    
    memcpy( in, utf8, sizeof(utf8) );
    
    iconv( ic, &in, &in_size, &out, &out_size );
    iconv_close(ic);
    
    printf("%s\n", outbuf );
    for(int i=0;i<sizeof(outbuf);i++){
        ret[i]=outbuf[i];
    }
}


void get_data(NJ_UINT8 *src,NJ_UINT8 *top){
    printf("%p\n",src-top);
    char yomi[MAX_BUF+1];
    char hyoki[MAX_BUF+1];
    NJ_UINT16 ysize=GET_YSIZE_FROM_DATA(src);
    NJ_UINT16 ksize=GET_KSIZE_FROM_DATA(src);
    printf("%p\n",src+5-top);
    get_moji(src+5,ysize,yomi);
    get_moji(src+5+ysize,ksize,hyoki);
    NJ_UINT16 fflg=GET_FFLG_FROM_DATA(src);
    NJ_UINT16 mflg=GET_MFLG_FROM_DATA(src);
    NJ_UINT16 type=GET_TYPE_FROM_DATA(src);
    NJ_UINT16 fpos=GET_FPOS_FROM_DATA(src);
    NJ_UINT16 bpos=GET_BPOS_FROM_DATA(src);
    
    printf("fflag:%x,mflag:%x,type:%x,fpos:%x,ysize:%x,bpos:%x,ksize:%x,yomi:%s,hyoki:%s\n",
           fflg,mflg,type,
           fpos,ysize,bpos,
           ksize,yomi,hyoki);
    fprintf(fp,"fflag:%x,mflag:%x,type:%x,fpos:%x,ysize:%x,bpos:%x,ksize:%x,yomi:%s,hyoki:%s\n",
            fflg,mflg,type,
            fpos,ysize,bpos,
            ksize,yomi,hyoki);
}

void readdic() {
    NJ_UINT8 *d=dic_05_data;
    NJ_WQUE *que;
    NJ_UINT16 que_id;
    NJ_UINT8 offset;
    NJ_UINT8 *learn_index_top_addr;
    NJ_UINT8 *ptr;
    /*
     unsigned char *buf;
     buf=(unsigned char*)malloc(sizeof(dic_03_data)*sizeof(unsigned char));
     memset(buf,0,sizeof(dic_03_data));
     for(int i=0;i<sizeof(dic_03_data);i++){
     buf[i]=dic_03_data[i];
     }
     fp=fopen("/tmp/dump.txt","w");
     for(int i=0;i<sizeof(dic_03_data);i++){
     fprintf(fp,"%c",buf[i]);
     }
     fclose(fp);
     */
    
    fp=fopen("/tmp/res.txt","w");
    
    printf("%d\n",sizeof(dic_05_data));
    learn_index_top_addr = LEARN_INDEX_TOP_ADDR(d);
    printf("%x\n",learn_index_top_addr-d);
    NJ_UINT8* adr=learn_index_top_addr +((0 & 0xffff) * NJ_INDEX_SIZE);
    printf("%x\n",adr-d);
    que_id = (NJ_UINT16)GET_UINT16(adr);// indexの中身１６ビット
    
    p4(d,0,"NJDC");
    p4(d,4,"Version");
    p4(d,8,"Type");
    p4(d,12,"DataSize");
    p4(d,16,"ExtraSize");
    p4(d,20,"NJ_MAX_LEN");
    p4(d,24,"NJ_MAX_RESULT_REN");
    p4(d,32,"Pos DataTop");
    p4(d,36,"Pos LearnWord");
    p4(d,40,"Pos MaxWord");
    p4(d,44,"Pos QueSize");
    p4(d,48,"Pos NextQue");
    p4(d,52,"Pos WriteFlag");
    p4(d,60,"Pos IndexTop");
    p4(d,64,"Pos Index2Top");
    
    p2(d,0x48,"que_id");
    
    ptr = POS_TO_ADDRESS(d, que_id);
    //ptr = POS_TO_ADDRESS(buf, que_id);
    printf("POS_TO_ADDRESS:%x\n",ptr);
    fprintf(fp,"POS_TO_ADDRESS:%x\n",ptr);
    
    fprintf(fp,"GET_TYPE_FROM_DATA:%x\n",GET_TYPE_FROM_DATA(ptr));
    printf("GET_TYPE_FROM_DATA:%x\n",get_type_from_data(ptr,d));
    fprintf(fp,"GET_FPOS_FROM_DATA:%x\n",GET_FPOS_FROM_DATA(ptr));
    printf("GET_FPOS_FROM_DATA:%x\n",get_fpos_from_data(ptr,d));
    fprintf(fp,"GET_BPOS_FROM_DATA:%x\n",GET_BPOS_FROM_DATA(ptr));
    printf("GET_BPOS_FROM_DATA:%x\n",get_bpos_from_data(ptr,d));
    
    fprintf(fp,"GET_YSIZE_FROM_DATA:%x\n",GET_YSIZE_FROM_DATA(ptr));
    printf("GET_YSIZE_FROM_DATA:%x\n",get_ysize_from_data(ptr,d));
    //que->yomi_len   = que->yomi_byte / sizeof(NJ_CHAR);
    fprintf(fp,"GET_KSIZE_FROM_DATA:%x\n",GET_KSIZE_FROM_DATA(ptr));
    printf("GET_KSIZE_FROM_DATA:%x\n",get_ksize_from_data(ptr,d));
    //que->hyouki_len = que->hyouki_byte / sizeof(NJ_CHAR);
    fprintf(fp,"GET_FFLG_FROM_DATA:%x\n",GET_FFLG_FROM_DATA(ptr));
    printf("GET_FFLG_FROM_DATA:%x\n",get_fflg_from_data(ptr,d));
    
    fprintf(fp,"GET_MFLG_FROM_DATA:%x\n",GET_MFLG_FROM_DATA(ptr));
    printf("GET_MFLG_FROM_DATA:%x\n",get_mflg_from_data(ptr,d));
    
    
    printf("-----index1.start-----\n");
    NJ_UINT32 index_top=i4(d,60);
    NJ_UINT32 data_top=i4(d,0x20);
    printf("%d\n",index_top);
    NJ_UINT32 quesize=i4(d,0x2c);
    while(1){
        //NJ_UINT16 index_val2=d[index_top];
        NJ_UINT16 index_val=NJ_INT16_READ(&d[(int)index_top]);
        if(index_val==0x00)break;
        printf("%x\n",index_val);
        printf("src_adr:%d\n",data_top+index_val);
        //NJ_UINT8* src=&d[data_top+index_val];
        ptr = POS_TO_ADDRESS(d, index_val);
        printf("%p\n",ptr-d);
        get_data(ptr,d);
        index_top+=2;
    }
    printf("----index1.end------\n");
    
    printf("----index2.start------\n");
    
    NJ_UINT32 index2_top=i4(d,60);
    printf("%d\n",index2_top);
    
    while(1){
        NJ_UINT16 index2_val=NJ_INT16_READ(&d[(int)index2_top]);
        if(index2_val==0x00)break;
        printf("%x\n",index2_val);
        index2_top+=2;
    }
    printf("----index2.end------\n");
    
    
    fclose(fp);
    
}

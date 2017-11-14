//
//  writedic.cpp
//  openwnn
//
//  Created by admin on 2017/11/08.
//  Copyright © 2017年 admin. All rights reserved.
//

#include "writedic.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include "nj_lib.h"
#include <stdio.h>

#undef DEBUG


// OpenWnnDictionaryImplJni.c
// 文字はこれで変換する
// dst:[ NJ_MAX_LEN + NJ_TERM_LEN ]
static int convertStringToNjChar( const unsigned char* src, NJ_CHAR* dst)
{
    int maxChars=NJ_MAX_LEN;
    if( src != NULL ) {
        int     i, o;
        
        /* convert UTF-8 to UTF-16BE */
        for( i = o = 0 ; src[ i ] != 0x00 && o < maxChars ; ) {
            NJ_UINT8* dst_tmp;
            dst_tmp = ( NJ_UINT8* )&( dst[ o ] );
            
            if( ( src[ i ] & 0x80 ) == 0x00 ) {
                /* U+0000 ... U+007f */
                /* 8[0xxxxxxx] -> 16BE[00000000 0xxxxxxx] */
                dst_tmp[ 0 ] = 0x00;
                dst_tmp[ 1 ] = src[ i + 0 ] & 0x7f;
                i++;
                o++;
            } else if( ( src[ i ] & 0xe0 ) == 0xc0 ) {
                /* U+0080 ... U+07ff */
                /* 8[110xxxxx 10yyyyyy] -> 16BE[00000xxx xxyyyyyy] */
                if( src[ i + 1 ] == 0x00 ) {
                    break;
                }
                dst_tmp[ 0 ] = ( ( src[ i + 0 ] & 0x1f ) >> 2 );
                dst_tmp[ 1 ] = ( ( src[ i + 0 ] & 0x1f ) << 6 ) |   ( src[ i + 1 ] & 0x3f );
                i += 2;
                o++;
            } else if( ( src[ i ] & 0xf0 ) == 0xe0 ) {
                /* U+0800 ... U+ffff */
                /* 8[1110xxxx 10yyyyyy 10zzzzzz] -> 16BE[xxxxyyyy yyzzzzzz] */
                if( src[ i + 1 ] == 0x00 || src[ i + 2 ] == 0x00 ) {
                    break;
                }
                dst_tmp[ 0 ] = ( ( src[ i + 0 ] & 0x0f ) << 4 ) | ( ( src[ i + 1 ] & 0x3f ) >> 2 );
                dst_tmp[ 1 ] = ( ( src[ i + 1 ] & 0x3f ) << 6 ) |   ( src[ i + 2 ] & 0x3f );
                i += 3;
                o++;
            } else if( ( src[ i ] & 0xf8 ) == 0xf0 ) {
                NJ_UINT8    dst1, dst2, dst3;
                /* U+10000 ... U+10ffff */
                /* 8[11110www 10xxxxxx 10yyyyyy 10zzzzzz] -> 32BE[00000000 000wwwxx xxxxyyyy yyzzzzzz] */
                /*                                        -> 16BE[110110WW XXxxxxyy 110111yy yyzzzzzz] */
                /*                                                      -- --======       == --------  */
                /*                                                      dst1   dst2          dst3      */
                /*                                        "wwwxx"(00001-10000) - 1 = "WWXX"(0000-1111) */
                if( !( o < maxChars - 1 ) ) {
                    /* output buffer is full */
                    break;
                }
                if( src[ i + 1 ] == 0x00 || src[ i + 2 ] == 0x00 || src[ i + 3 ] == 0x00 ) {
                    break;
                }
                dst1 = ( ( ( src[ i + 0 ] & 0x07 ) << 2 ) | ( ( src[ i + 1 ] & 0x3f ) >> 4 ) ) - 1;
                dst2 =   ( ( src[ i + 1 ] & 0x3f ) << 4 ) | ( ( src[ i + 2 ] & 0x3f ) >> 2 );
                dst3 =   ( ( src[ i + 2 ] & 0x3f ) << 6 ) |   ( src[ i + 3 ] & 0x3f );
                
                dst_tmp[ 0 ] = 0xd8 | ( ( dst1 & 0x0c ) >> 2 );
                dst_tmp[ 1 ] =        ( ( dst1 & 0x03 ) << 6 ) | ( ( dst2 & 0xfc ) >> 2 );
                dst_tmp[ 2 ] = 0xdc |                            ( ( dst2 & 0x03 ) );
                dst_tmp[ 3 ] =                                                              dst3;
                i += 4;
                o += 2;
            } else {    /* Broken code */
                break;
            }
        }
        dst[ o ] = NJ_CHAR_NUL;
#ifdef DEBUG
        printf("%s\n",src);
        printf("%x\n",dst);
#endif
        return 0;
    }
    return -1;
}


//https://qiita.com/iseki-masaya/items/70b4ee6e0877d12dafa8
static std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        if (!item.empty()) {
            elems.push_back(item);
        }
    }
    return elems;
}

static void write_to_file(const char* outfile,std::vector<std::string> &binary){
    
    FILE *rfp;
    rfp=fopen(outfile,"w");
    for(int i=0;i<binary.size();i++){
        if(i%16==0){
            fprintf(rfp,"\n");
        }
        fprintf(rfp,"%s, ",binary.at(i).c_str());
    }
    fclose(rfp);
}

static std::string to_s(long value){
    char buf[2048];
    sprintf(buf,"0x%02x",value);
    return std::string(buf);
}

static int moji_size(unsigned const char* src){
    NJ_CHAR moji[ NJ_MAX_LEN + NJ_TERM_LEN ];

    convertStringToNjChar(src,moji);
    int siz=0;
    for(int i=0;i<sizeof(moji);i++){
        if(moji[i]==NJ_CHAR_NUL)break;
        siz++;
    }
    return siz;
}


static void read_from_file(const char* infile,std::vector<std::string> &yomi,std::vector<std::string> &kanji,std::vector<std::string> &yomi_sorted,std::vector<std::string> &kanji_sorted,int &max_yomi_size,int& max_kanji_size,int &max_size){
    std::ifstream input(infile);
    std::string line;

    int adr=0;
    char buf[2048];
    
    max_yomi_size=0;
    max_kanji_size=0;
    max_size=0;
    
    while(std::getline(input,line)){
        std::vector<std::string> vec=split(line,'\t');
#ifdef DEBUG
        printf("%s,%s\n",vec.at(0).c_str(),vec.at(1).c_str());
#endif
        //std::string yomi_str=to_utf16_str((unsigned const char*)vec.at(0).c_str());
        std::string yomi_str;
        NJ_CHAR char_buf[ NJ_MAX_LEN + NJ_TERM_LEN ];
        char str_buf[NJ_MAX_LEN + NJ_TERM_LEN];
        for(int i=0;i<sizeof(char_buf);i++)char_buf[i]=0;
        // utf16これでソートするが上位下位バイトを逆転させたものでソート
        convertStringToNjChar((unsigned const char*)vec.at(0).c_str(),char_buf);
        for(int i=0;i<sizeof(char_buf);i++){
            if(char_buf[i]==0)break;
            
            // 上位下位バイトを反転
            int upp_byte=char_buf[i] & 0xff;
            int low_byte=char_buf[i] >> 8 ;
            low_byte = low_byte & 0xff;
            int rev=upp_byte << 8;
            rev = rev + low_byte;
            //printf("%05d\n",char_buf[i]);
            sprintf(str_buf,"%05d",rev);
            yomi_str.append(str_buf);
        }
        
        
        std::string kanji_str;
        for(int i=0;i<sizeof(char_buf);i++)char_buf[i]=0;
        // utf16これでソートするが上位下位バイトを逆転させたものでソート
        convertStringToNjChar((unsigned const char*)vec.at(1).c_str(),char_buf);
        for(int i=0;i<sizeof(char_buf);i++){
            if(char_buf[i]==0)break;
            // 上位下位バイトを反転
            int upp_byte=char_buf[i] & 0xff;
            int low_byte=char_buf[i] >> 8 ;
            low_byte = low_byte & 0xff;
            int rev=upp_byte << 8;
            rev = rev + low_byte;
            
            //printf("%05d\n",char_buf[i]);
            sprintf(str_buf,"%05d",rev);
            kanji_str.append(str_buf);
        }


        sprintf(buf,"%d",adr++);
        yomi_str.append("\t");yomi_str.append(buf); // for sort
        yomi.push_back(vec.at(0));
        yomi_sorted.push_back(yomi_str);
        kanji_str.append("\t");kanji_str.append(buf);
        kanji.push_back(vec.at(1));
        kanji_sorted.push_back(kanji_str);
        
        int yomi_str_size=moji_size((unsigned const char*)vec.at(0).c_str());
        int kanji_str_size=moji_size((unsigned const char*)vec.at(1).c_str());
        max_yomi_size=max_yomi_size>yomi_str_size?max_yomi_size:yomi_str_size;
        max_kanji_size=max_kanji_size>kanji_str_size?max_kanji_size:kanji_str_size;
        int siz=yomi_str_size+kanji_str_size;
        max_size=max_size>siz?max_size:siz;
    }
    
    
    char fname[128];
    sprintf(fname,"%s.yomi.txt",infile);
    FILE *fp=fopen(fname,"w");
    std::sort(yomi_sorted.begin(),yomi_sorted.end());
    for(int i=0;i<yomi_sorted.size();i++){
        std::vector<std::string> sp=split(yomi_sorted.at(i),'\t');
        fprintf(fp,"%s\t%s\n",yomi_sorted.at(i).c_str(),yomi.at(atoi(sp.at(1).c_str())).c_str());
    }
    fclose(fp);
    
    sprintf(fname,"%s.kanji.txt",infile);
    fp=fopen(fname,"w");
    std::sort(kanji_sorted.begin(),kanji_sorted.end());
    for(int i=0;i<kanji.size();i++){
        std::vector<std::string> sp=split(kanji_sorted.at(i),'\t');
        fprintf(fp,"%s\t%s\n",kanji_sorted.at(i).c_str(),kanji.at(atoi(sp.at(1).c_str())).c_str());
    }
#ifdef DEBUG
    printf("max_yomi_size:%d,max_kanji_size:%d\n",max_yomi_size,max_kanji_size);
    printf("max_size=%d\n",max_size);
#endif
    fclose(fp);
}

static void set_mojiint16(std::vector<std::string> &b,long value){
    b.push_back(to_s(value & 0xff));  // エンディアン対応
    b.push_back(to_s(value >> 8 & 0xff));
}

static void set_int16(std::vector<std::string> &b,long value){
    b.push_back(to_s(value >> 8 & 0xff));
    b.push_back(to_s(value & 0xff));
}
static void set_int32(std::vector<std::string> &b,long value){
    b.push_back(to_s(value >> 24 & 0xff));
    b.push_back(to_s(value >> 16 & 0xff));
    b.push_back(to_s(value >> 8 & 0xff));
    b.push_back(to_s(value & 0xff));
}
static void set_byte(std::vector<std::string> &b,long value){
    b.push_back(to_s(value));
}
static void set_zero32(std::vector<std::string> &b){
    for(int i=0;i<4;i++)set_byte(b,0);
}
static void set_header(std::vector<std::string> &b,int que_size,int yomi_size,int kanji_size){
    long pos_index=0x48;
    long pos_index2=pos_index+yomi_size*2;
    long pos_data_top=pos_index+yomi_size*2+kanji_size*2;
    long data_size=yomi_size*2+kanji_size*2+kanji_size*que_size; // index_topからNJDCまでのバイト数
    set_byte(b,'N');
    set_byte(b,'J');
    set_byte(b,'D');
    set_byte(b,'C');
    
    // version
    set_byte(b,0);
    set_byte(b,0x02);
    set_byte(b,0);
    set_byte(b,0);
    
    // type
    set_byte(b,0x80);
    set_byte(b,0x03);
    set_byte(b,0x00);
    set_byte(b,0x00);
    
    // data size ,
    set_int32(b,data_size);
    
    // ext size,
    set_int32(b,0x2c);  // for max checkの次からindex_topまでの距離
    
    // for max check, わからないので０
    set_zero32(b);
    
    // for max check, わからないので０  ここまでのバイト数にdata sizeを足すと終わりのNDJCになること
    set_zero32(b);
    
    // blank
    set_zero32(b);
    
    // pos data top
    set_int32(b,pos_data_top);
    
    // pos learn word count,単語数？
    set_int32(b,kanji_size);
    
    // pos max word, とりあえずpos learn word count
    set_int32(b,kanji_size);

    // pos que size
    set_int32(b,que_size);
    
    // pos max que
    set_zero32(b);
    
    // pos_write_flag
    set_zero32(b);
    
    // blank
    set_zero32(b);
    
    // pos_index
    set_int32(b,pos_index);

    // pos index2
    set_int32(b,pos_index2);
    
    // blank
    set_zero32(b);
}

static void swap(std::vector<NJ_UINT8>& moji,int idx1,int idx2){
    if(idx2>=moji.size())return;
    NJ_UINT8 tmp=moji.at(idx1);
    moji[idx1]=moji[idx2];
    moji[idx2]=tmp;
}

// https://stackoverflow.com/questions/3081289/how-to-read-a-line-from-a-text-file-in-c-c
void writedic(const char* infile,const char* outfile){
    std::vector<std::string> yomi;
    std::vector<std::string> kanji;
    std::vector<std::string> yomi_sorted;
    std::vector<std::string> kanji_sorted;
    
    NJ_CHAR yomi_buf[ NJ_MAX_LEN + NJ_TERM_LEN ];
    NJ_CHAR kanji_buf[ NJ_MAX_LEN + NJ_TERM_LEN ];

    int max_yomi_size=0; // byte size
    int max_kanji_size=0; // byte size
    int max_size=0;
    printf("%s\n",outfile);
    read_from_file(infile,yomi,kanji,yomi_sorted,kanji_sorted,max_yomi_size,max_kanji_size,max_size);


    
    
    
    std::vector<std::string> b;
    int data_header_size=5;// DATAのヘッダ部分のバイト数
    int que_size=max_size*2+data_header_size; // utf-16 一文字2バイト
#ifdef DEBUG
    printf("que_size=%d\n",que_size);
#endif
    set_header(b,que_size,yomi.size(),kanji.size());
    
    // yomi
    for(int i=0;i<yomi_sorted.size();i++){
        std::vector<std::string> item=split(yomi_sorted.at(i),'\t');
        set_int16(b,atol(item.at(1).c_str()));
    }
    // kanji
    for(int i=0;i<kanji_sorted.size();i++){
        std::vector<std::string> item=split(kanji_sorted.at(i),'\t');
        set_int16(b,atol(item.at(1).c_str()));
    }
    
    // data
    for(int i=0;i<kanji.size();i++){
#ifdef DEBUG
        printf("i=%d\n",i);
        if (i%10000==0)printf("i=%d\n",i);
#endif

        set_byte(b,0x41); // toriaezu
        const unsigned char *yomi_ptr=(const unsigned char *)yomi.at(i).c_str();
        const unsigned char *kanji_ptr=(const unsigned char *)kanji.at(i).c_str();

        int yomi_size=0;
        int kanji_size=0;
        int data_adr=5; // 5 byte to que sizeまで
        // 読み部分
        convertStringToNjChar(yomi_ptr,yomi_buf);
        for(int j=0;j<sizeof(yomi_buf);j++){
            if(NJ_CHAR_NUL == yomi_buf[j])break;
            yomi_size++;
        }

        // 漢字部分
        convertStringToNjChar(kanji_ptr,kanji_buf);
        for(int j=0;j<sizeof(kanji_buf);j++){
            if(NJ_CHAR_NUL == kanji_buf[j])break;
            kanji_size++;
        }
#ifdef DEBUG
        printf("yomi_size:%d,kanji_size:%d\n",yomi_size,kanji_size);
#endif
        
        set_int16(b,yomi_size*2);
        set_int16(b,kanji_size*2);
        for(int j=0;j<yomi_size;j++){
#ifdef DEBUG
            printf("%d\n",yomi_buf[j]);
#endif
            set_mojiint16(b,yomi_buf[j]);
            data_adr+=2;
        }
        for(int j=0;j<kanji_size;j++){
            set_mojiint16(b,kanji_buf[j]);
            data_adr+=2;
        }
        
        // 後ろを０で埋める，固定長のため
        for(int j=data_adr;j<que_size;j++){
            set_byte(b,0x0);
        }
        
    }
    set_byte(b,'N');
    set_byte(b,'J');
    set_byte(b,'D');
    set_byte(b,'C');
    
    
    write_to_file(outfile,b);
    printf("size:%d\n\n",b.size());
}

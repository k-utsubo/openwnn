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

static void write_to_file(std::vector<NJ_UINT8> &binary){
    NJ_UINT8 *data=(NJ_UINT8 *)malloc(sizeof(NJ_UINT8)*binary.size());
    
    FILE *fp;
    fp=fopen("/tmp/res.bin","wb");
    FILE *rfp;
    rfp=fopen("/tmp/res.dat","w");
    for(int i=0;i<binary.size();i++){
        if(i%16==0){
            //printf("\n");
            fprintf(rfp,"\n");
        }
        //printf("0x%x, ",binary.at(i));
        fprintf(rfp,"0x%02x, ",binary.at(i));
        
        fwrite(&binary.at(i),sizeof(NJ_UINT8),1,fp);
        data[i]=binary.at(i);
    }
    fclose(fp);
    fclose(rfp);
}

static void read_from_file(std::vector<std::string> &yomi,std::vector<std::string> &kanji,std::vector<std::string> &yomi_sorted,std::vector<std::string> &kanji_sorted,int &max_yomi_size,int& max_kanji_size,int &max_size){
    std::ifstream input("/Users/admin/Downloads/dict.txt");
    std::string line;

    int adr=0;
    char buf[2048];
    
    max_yomi_size=0;
    max_kanji_size=0;
    max_size=0;
    
    while(std::getline(input,line)){
        std::vector<std::string> vec=split(line,'\t');
        //printf("%s,%s\n",vec.at(0).c_str(),vec.at(1).c_str());
        std::string yomi_str(vec.at(0));
        std::string kanji_str(vec.at(1));
        sprintf(buf,"%d",adr++);
        yomi_str.append("\t");yomi_str.append(buf); // for sort
        yomi.push_back(vec.at(0));
        yomi_sorted.push_back(yomi_str);
        kanji_str.append("\t");kanji_str.append(buf);
        kanji.push_back(vec.at(1));
        kanji_sorted.push_back(kanji_str);
        max_yomi_size=max_yomi_size>vec.at(0).size()?max_yomi_size:vec.at(0).size();
        max_kanji_size=max_kanji_size>vec.at(1).size()?max_kanji_size:vec.at(1).size();
        int siz=vec.at(0).size()+vec.at(1).size();
        max_size=max_size>siz?max_size:siz;
    }
    //printf("%d,%d\n",max_yomi_size,max_kanji_size);
    
    
    std::sort(yomi_sorted.begin(),yomi_sorted.end());
    //for(int i=0;i<yomi.size();i++){
    //    printf("%s\n",yomi.at(i).c_str());
    //}
    std::sort(kanji_sorted.begin(),kanji_sorted.end());
    //for(int i=0;i<kanji.size();i++){
    //    printf("%s\n",kanji.at(i).c_str());
    //}
}

void set_zero32(std::vector<NJ_UINT8> &b){
    for(int i=0;i<4;i++)b.push_back(0);
}
void set_int16(std::vector<NJ_UINT8> &b,long value){
    b.push_back(value >> 8 & 0xff);
    b.push_back(value & 0xff);
}
void set_int32(std::vector<NJ_UINT8> &b,long value){
    b.push_back(value >> 24 & 0xff);
    b.push_back(value >> 16 & 0xff);
    b.push_back(value >> 8 & 0xff);
    b.push_back(value & 0xff);
}
void set_header(std::vector<NJ_UINT8> &b,int que_size,int yomi_size,int kanji_size){
    long pos_index=0x48;
    long pos_index2=pos_index+yomi_size*2;
    long pos_data_top=pos_index+yomi_size*2+kanji_size*2;
    long data_size=yomi_size*2+kanji_size*2+kanji_size*que_size; // index_topからNJDCまでのバイト数
    b.push_back('N');
    b.push_back('J');
    b.push_back('D');
    b.push_back('C');
    
    // version
    b.push_back(0);
    b.push_back(0x02);
    b.push_back(0);
    b.push_back(0);
    
    // type
    b.push_back(0);
    b.push_back(0x02);
    b.push_back(0);
    b.push_back(0x02);
    
    // data size , あとで入れる
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
    
    // pos learn word, わからないので０
    set_zero32(b);
    
    // pos max word, わからないので０
    set_zero32(b);
    
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

// https://stackoverflow.com/questions/3081289/how-to-read-a-line-from-a-text-file-in-c-c
void writedic(){
    std::vector<std::string> yomi;
    std::vector<std::string> kanji;
    std::vector<std::string> yomi_sorted;
    std::vector<std::string> kanji_sorted;
    int max_yomi_size=0; // byte size
    int max_kanji_size=0; // byte size
    int max_size=0;
    read_from_file(yomi,kanji,yomi_sorted,kanji_sorted,max_yomi_size,max_kanji_size,max_size);

    std::vector<NJ_UINT8> b;
    int data_header_size=5;// DATAのヘッダ部分のバイト数
    //int que_size=max_yomi_size*3+max_kanji_size*3+data_header_size; // utf-8 一文字３バイト
    int que_size=max_size*3+data_header_size; // utf-8 一文字３バイト
    printf("que_size=%d\n",que_size);
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
        b.push_back(0x41); // toriaezu
        const char *yomi_ptr=yomi.at(i).c_str();
        const char *kanji_ptr=kanji.at(i).c_str();

        
        int yomi_byte=yomi.at(i).size();
        int kanji_byte=kanji.at(i).size();
        set_int16(b,yomi_byte);
        set_int16(b,kanji_byte);
        int data_adr=5; // 5 byte to que sizeまで
        for(int j=0;j<yomi_byte;j++){
            b.push_back(*yomi_ptr);
            yomi_ptr++;
            data_adr++;
        }
        for(int j=0;j<kanji_byte;j++){
            b.push_back(*kanji_ptr);
            kanji_ptr++;
            data_adr++;
        }
        for(int j=data_adr;j<que_size;j++){
            b.push_back(0x0);
        }
        
    }
    b.push_back('N');
    b.push_back('J');
    b.push_back('D');
    b.push_back('C');
    
    
    write_to_file(b);
    printf("\nsize:%d\n",b.size());

}

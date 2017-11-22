//
//  main.c
//  openwnn
//
//  Created by admin on 2017/11/01.
//  Copyright © 2017年 admin. All rights reserved.
//


#include "readdic.h"
#include "writedic.hpp"
#include "utfsample.h"
#include "nj_lib.h"

int nj_char_diff(NJ_CHAR *a, NJ_CHAR *b) {
    printf("a=%x,b=%x\n",*a,*b);
    printf("a0=%d,b0=%d\n",((NJ_UINT8*)(a))[0],((NJ_UINT8*)(b))[0] );
    printf("a1=%d,b1=%d\n",((NJ_UINT8*)(a))[1],((NJ_UINT8*)(b))[1] );
    return((NJ_INT16)( (((NJ_UINT8*)(a))[0] == ((NJ_UINT8*)(b))[0])? (((NJ_UINT8*)(a))[1] - ((NJ_UINT8*)(b))[1]): (((NJ_UINT8*)(a))[0] - ((NJ_UINT8*)(b))[0]) )) ;
}

int main(){
    NJ_CHAR str=35888;
    NJ_CHAR yomi=4351;
    
    //printf("%d\n",nj_char_diff(&yomi,&str));
    
    writedic("/Users/admin/Downloads/dict_small.txt","/Users/admin/Downloads/dict_small.dat");
    //writedic("/Users/admin/Downloads/dict_with_upress_moji.txt","/Users/admin/Downloads/dict_with_upress_moji.dat");

    // max 65535 lines
    //writedic("/Users/admin/Downloads/xaa","/Users/admin/Downloads/xaa.dat");
    //writedic("/Users/admin/Downloads/xab","/Users/admin/Downloads/xab.dat");
    //writedic("/Users/admin/Downloads/xac","/Users/admin/Downloads/xac.dat");
    //writedic("/Users/admin/Downloads/xad","/Users/admin/Downloads/xad.dat");

}

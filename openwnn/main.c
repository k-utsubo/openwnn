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

int main(){
    
    //writedic("/Users/admin/Downloads/dict.txt","/Users/admin/Downloads/dict.dat");
    // max 65535 lines
    writedic("/Users/admin/Downloads/xaa","/Users/admin/Downloads/xaa.dat");
    writedic("/Users/admin/Downloads/xab","/Users/admin/Downloads/xab.dat");
    writedic("/Users/admin/Downloads/xac","/Users/admin/Downloads/xac.dat");
    writedic("/Users/admin/Downloads/xad","/Users/admin/Downloads/xad.dat");

}

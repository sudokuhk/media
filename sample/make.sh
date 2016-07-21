#########################################################################
# File Name:make.sh
# Author: sudoku.huang
# mail: sudoku.huang@gmail.com
# Created Time:2016年07月15日 星期五 17时28分45秒
#########################################################################
#!/bin/bash

g++ decode.cpp -I./../inc -L./../lib -pthread -lavcodec -lavutil -lz -o decode -ldl -lrt
g++ encode.cpp -I./../inc -L./../lib -lx264 -pthread -ldl -o encode

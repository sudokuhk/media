#!/bin/bash

cwd=`pwd`
output_dir=$cwd/video;

function clear()
{
    if [ -d "$output_dir" ]; then
        rm -r $output_dir;
    fi
    
    mkdir -p $output_dir;
    mkdir -p $output_dir/lib;
    mkdir -p $output_dir/inc;
}

function collect()
{
    local type=$1;
    local full=$2;
    local path=${full%/*}
    local file=${full##*/}

    #echo "$full, path:$path, file:$file"
    
    if [ $type = "header" ]; then
        mkdir -p $output_dir/inc/$path
        cp $full $output_dir/inc/$full;
        #echo "cp $full $output_dir/inc/$full";
    elif [ $type = "library" ]; then
        #echo "cp $full $output_dir/lib/";
        cp $full $output_dir/lib/;
    else
        echo "unkown type!";
        exit;
    fi
}

function yasm() 
{
    local yasm_url="http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz";
    local yasm_pkg="yasm-1.3.0.tar.gz"
    local yasm_dir="yasm-1.3.0"
    
    ## yasm, make if need!
    if [ 1 -eq 0 ]; then
        if [ -d "$yasm_dir" ]; then
            rm -rf $yasm_dir;
        fi
        
        if [ -f $yasm_pkg ]; then
            tar -xvf $yasm_pkg -C $yasm_dir;
        else
            echo "download $yasm_pkg from $yasm_url";
            curl -o $yasm_pkg $yasm_url;
            if [ $? -eq 1 ]; then 
                echo "no yasm-1.3.0.tar.gz exists. and download from $yasm_url failed!"
                exit;
            fi
        fi
        
        cd $yasm_dir;
        ./configure && make;
        sudo make install;
        cd -;
    fi
}

## x264
function x264()
{
    local x264_pkg="x264-snapshot-20160301-2245.zip";
    local x264_dir="x264"
    
    if [ -d "$x264_dir" ]; then
        rm -rf $x264_dir;
    fi
    
    if [ -f $x264_pkg ]; then
        local def_path=${x264_pkg%.*}
        unzip -o $x264_pkg;
        mv $def_path $x264_dir;
        ##unzip $x264_pkg -d  $x264_dir;
        ##tar -xvf $x264_pkg -C $x264_dir;
    else
        echo "no package $x264_pkg exists!";
        exit;
    fi
    
    cd $x264_dir;
    chmod a+x config* version*;
    ./configure --disable-cli  --enable-static;
    make -j4;
    cd -;
    
    collect "header" $x264_dir/x264.h;
    collect "header" $x264_dir/x264_config.h;
    collect "library" $x264_dir/libx264.a;

    rm -rf $x264_dir;
}
 
## ffmpeg
function ffmpeg()
{
    local ffmpeg_pkg="ffmpeg20160714.tar.gz";
    local ffmpeg_dir="ffmpeg"
    
    if [ -d "$ffmpeg_dir" ]; then
        rm -rf $ffmpeg_dir;
    fi
    
    if [ -f $ffmpeg_pkg ]; then
        mkdir -p $ffmpeg_dir;
        tar -xvf $ffmpeg_pkg -C $ffmpeg_dir;
    else
        echo "no package $ffmpeg_pkg exists!";
        exit;
    fi
    
    cd $ffmpeg_dir/ffmpeg-beifen;
    
    ./configure --disable-shared    \
            --enable-yasm       \
            --enable-gpl        \
            --enable-pthreads   \
            --extra-cflags=-I../x264-snapshot-20160301-2245     \
            --extra-ldflags=-L../x264-snapshot-20160301-2245   \
            --disable-ffplay    \
            --disable-ffprobe   \
            --disable-ffserver  \
            --disable-doc       \
            --disable-htmlpages \
            --disable-manpages  \
            --disable-podpages  \
            --disable-txtpages  \
            --disable-avformat  \
            --disable-swscale   \
            --disable-postproc  \
            --disable-avfilter  \
            --disable-avdevice  \
            --disable-d3d11va   \
            --disable-everything    \
            --disable-audiotoolbox  \
            --enable-encoder=libx264   \
            --enable-decoder=h264; 
            
    make -j4;

    local headers="libavcodec/version.h        \
        libavcodec/avcodec.h        \
        libavutil/avconfig.h        \
        libavutil/avutil.h          \
        libavutil/buffer.h          \
        libavutil/pixfmt.h          \
        libavutil/mathematics.h     \
        libavutil/attributes.h      \
        libavutil/channel_layout.h  \
        libavutil/common.h          \
        libavutil/rational.h        \
        libavutil/version.h         \
        libavutil/macros.h          \
        libavutil/frame.h           \
        libavutil/samplefmt.h       \
        libavutil/cpu.h             \
        libavutil/dict.h            \
        libavutil/error.h           \
        libavutil/log.h             \
        libavutil/mem.h             \
        libavutil/intfloat.h        \
        libavformat/avio.h          \
        libavformat/version.h       \
        libavformat/avformat.h      \
        libswscale/swscale.h        \
        libswscale/version.h        \
        ";
        
    local librarys="libavcodec/libavcodec.a \
        libavutil/libavutil.a   \
        ";

    for header in $headers; do
        collect "header" $header;
    done;

    for library in $librarys; do
        collect "library" $library;
    done;

    cd -;            

    rm -rf $ffmpeg_dir;
}

function main()
{
    clear;
    yasm;
    x264;    
    ffmpeg;
}

main $*

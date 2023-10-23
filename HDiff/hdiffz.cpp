//hdiffz.cpp
// diff tool
//
/*
 The MIT License (MIT)
 Copyright (c) 2012-2021 HouSisong
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>  //find search
#include "diff.h"
#include "match_block.h"
#include "../HPatch/patch.h"
#include "_clock_for_demo.h"
#include "_atosize.h"
#include "file_for_patch.h"
#include "./private_diff/mem_buf.h"

//#include "quicklz.h"

#ifndef _IS_NEED_MAIN
#   define  _IS_NEED_MAIN 1
#endif


static void printVersion(){
    printf("HDiffPatch::hdiffz v" HDIFFPATCH_VERSION_STRING "\n\n");
}

static void printUsage(){
    printVersion();
    printf("diff    usage: hdiffz [options] oldPath newPath outDiffFile\n"
           "test    usage: hdiffz    -t     oldPath newPath testDiffFile\n"
           "resave  usage: hdiffz [-c-...]  diffFile outDiffFile\n"
#if (_IS_NEED_DIR_DIFF_PATCH)
           "get  manifest: hdiffz [-g#...] [-C-checksumType] inputPath -M#outManifestTxtFile\n"
           "manifest diff: hdiffz [options] -M-old#oldManifestFile -M-new#newManifestFile\n"
           "                      oldPath newPath outDiffFile\n"
           "  oldPath newPath inputPath can be file or directory(folder),\n"
#endif
           "  oldPath can empty, and input parameter \"\"\n"
           "memory options:\n"
           "  -m[-matchScore]\n"
           "      DEFAULT; all file load into Memory; best diffFileSize;\n"
           "      requires (newFileSize+ oldFileSize*5(or *9 when oldFileSize>=2GB))+O(1)\n"
           "        bytes of memory;\n"
           "      matchScore>=0, DEFAULT -m-6, recommended bin: 0--4 text: 4--9 etc...\n"
           "  -s[-matchBlockSize]\n"
           "      all file load as Stream; fast;\n"
           "      requires O(oldFileSize*16/matchBlockSize+matchBlockSize*5)bytes of memory;\n"
           "      matchBlockSize>=4, DEFAULT -s-64, recommended 16,32,48,1k,64k,1m etc...\n"
           "special options:\n"
           "  -block[-fastMatchBlockSize] \n"
           "      must run with -m;\n"
           "      set is use fast block match befor slow match, DEFAULT false;\n"
           "      fastMatchBlockSize>=4, DEFAULT 4k, recommended 256,1k,64k,1m etc...;\n"
           "      if newData similar to oldData then diff speed++ & diff memory--,\n"
           "      but small possibility outDiffFile's size+\n"
           "  -cache \n"
           "      must run with -m;\n"
           "      set is use a big cache for slow match, DEFAULT false;\n"
           "      if newData not similar to oldData then diff speed++,\n"
           "      big cache max used O(oldFileSize) memory, and build slow(diff speed--)\n" 
           "  -SD[-stepSize]\n"
           "      create single compressed diffData, only need one decompress buffer\n"
           "      when patch, and support step by step patching when step by step downloading!\n"
           "      stepSize>=" _HDIFFPATCH_EXPAND_AND_QUOTE(hpatch_kStreamCacheSize) ", DEFAULT -SD-256k, recommended 64k,2m etc...\n"
#if (_IS_USED_MULTITHREAD)
           "  -p-parallelThreadNumber\n"
           "      if parallelThreadNumber>1 then open multi-thread Parallel mode;\n"
           "      DEFAULT -p-4; requires more memory!\n"
#endif
           "  -c-compressType[-compressLevel]\n"
           "      set outDiffFile Compress type & level, DEFAULT uncompress;\n"
           "      for resave diffFile,recompress diffFile to outDiffFile by new set;\n"
           "      support compress type & level:\n"
           "       (re. https://github.com/sisong/lzbench/blob/master/lzbench171_sorted.md )\n"
#ifdef _CompressPlugin_zlib
           "        -c-zlib[-{1..9}]                DEFAULT level 9\n"
#   if (_IS_USED_MULTITHREAD)
           "        -c-pzlib[-{1..9}]               DEFAULT level 6\n"
           "            support run by multi-thread parallel, fast!\n"
           "            WARNING: code not compatible with it compressed by -c-zlib!\n"
           "              and code size may be larger than if it compressed by -c-zlib. \n"
#   endif
#endif
#ifdef _CompressPlugin_bz2
           "        -c-bzip2[-{1..9}]               (or -bz2) DEFAULT level 9\n"
#   if (_IS_USED_MULTITHREAD)
           "        -c-pbzip2[-{1..9}]              (or -pbz2) DEFAULT level 8\n"
           "            support run by multi-thread parallel, fast!\n"
           "            WARNING: code not compatible with it compressed by -c-bzip2!\n"
           "               and code size may be larger than if it compressed by -c-bzip2.\n"
#   endif
#endif
#ifdef _CompressPlugin_lzma
           "        -c-lzma[-{0..9}[-dictSize]]     DEFAULT level 7\n"
           "            dictSize can like 4096 or 4k or 4m or 128m etc..., DEFAULT 8m\n"
#   if (_IS_USED_MULTITHREAD)
           "            support run by 2-thread parallel.\n"
#   endif
#endif
#ifdef _CompressPlugin_lzma2
           "        -c-lzma2[-{0..9}[-dictSize]]    DEFAULT level 7\n"
           "            dictSize can like 4096 or 4k or 4m or 128m etc..., DEFAULT 8m\n"
#   if (_IS_USED_MULTITHREAD)
           "            support run by multi-thread parallel, fast!\n"
#   endif
           "            WARNING: code not compatible with it compressed by -c-lzma!\n"
#endif
#ifdef _CompressPlugin_lz4
           "        -c-lz4[-{1..50}]                DEFAULT level 50 (as lz4 acceleration 1)\n"
#endif
#ifdef _CompressPlugin_lz4hc
           "        -c-lz4hc[-{3..12}]              DEFAULT level 11\n"
#endif
#ifdef _CompressPlugin_zstd
           "        -c-zstd[-{0..22}[-dictBits]]    DEFAULT level 20\n"
           "            dictBits can 10--31, DEFAULT 24.\n"
#   if (_IS_USED_MULTITHREAD)
           "            support run by multi-thread parallel, fast!\n"
#   endif
#endif
#ifdef _CompressPlugin_brotli
           "        -c-brotli[-{0..11}[-dictBits]]  DEFAULT level 9\n"
           "            dictBits can 10--30, DEFAULT 24.\n"
#endif
#ifdef _CompressPlugin_lzham
           "        -c-lzham[-{0..5}[-dictBits]]    DEFAULT level 4\n"
           "            dictBits can 15--29, DEFAULT 24.\n"
#   if (_IS_USED_MULTITHREAD)
           "            support run by multi-thread parallel, fast!\n"
#   endif
#endif
#if (_IS_NEED_DIR_DIFF_PATCH)
           "  -C-checksumType\n"
           "      set outDiffFile Checksum type for directory diff, DEFAULT "
#ifdef _ChecksumPlugin_fadler64
           "-C-fadler64;\n"
#else
#   ifdef _ChecksumPlugin_crc32
           "-C-crc32;\n"
#   else
           "no checksum;\n"
#   endif
           "      (if need checksum for diff between two files, add -D)\n"
#endif
           "      support checksum type:\n"
           "        -C-no                   no checksum\n"
#ifdef _ChecksumPlugin_crc32
#   ifdef _ChecksumPlugin_fadler64
           "        -C-crc32\n"
#   else
           "        -C-crc32                DEFAULT\n"
#   endif
#endif
#ifdef _ChecksumPlugin_adler32
           "        -C-adler32\n"
#endif
#ifdef _ChecksumPlugin_adler64
           "        -C-adler64\n"
#endif
#ifdef _ChecksumPlugin_fadler32
           "        -C-fadler32\n"
#endif
#ifdef _ChecksumPlugin_fadler64
           "        -C-fadler64             DEFAULT\n"
#endif
#ifdef _ChecksumPlugin_fadler128
           "        -C-fadler128\n"
#endif
#ifdef _ChecksumPlugin_md5
           "        -C-md5\n"
#endif
#ifdef _ChecksumPlugin_blake3
           "        -C-blake3\n"
#endif
           "  -n-maxOpenFileNumber\n"
           "      limit Number of open files at same time when stream directory diff;\n"
           "      maxOpenFileNumber>=8, DEFAULT -n-48, the best limit value by different\n"
           "        operating system.\n"
           "  -g#ignorePath[#ignorePath#...]\n"
           "      set iGnore path list when Directory Diff; ignore path list such as:\n"
           "        #.DS_Store#desktop.ini#*thumbs*.db#.git*#.svn/#cache_*/00*11/*.tmp\n"
           "      # means separator between names; (if char # in name, need write #: )\n"
           "      * means can match any chars in name; (if char * in name, need write *: );\n"
           "      / at the end of name means must match directory;\n"
           "  -g-old#ignorePath[#ignorePath#...]\n"
           "      set iGnore path list in oldPath when Directory Diff;\n"
           "      if oldFile can be changed, need add it in old ignore list;\n"
           "  -g-new#ignorePath[#ignorePath#...]\n"
           "      set iGnore path list in newPath when Directory Diff;\n"
           "      in general, new ignore list should is empty;\n"
           "  -M#outManifestTxtFile\n"
           "      create a Manifest file for inputPath; it is a text file, saved infos of\n"
           "      all files and directoriy list in inputPath; this file while be used in \n"
           "      manifest diff, support re-checksum data by manifest diff;\n"
           "      can be used to protect historical versions be modified!\n"
           "  -M-old#oldManifestFile\n"
           "      oldManifestFile is created from oldPath; if no oldPath not need -M-old;\n"
           "  -M-new#newManifestFile\n"
           "      newManifestFile is created from newPath;\n"
           "  -D  force run Directory diff between two files; DEFAULT (no -D) run \n"
           "      directory diff need oldPath or newPath is directory.\n"
#endif //_IS_NEED_DIR_DIFF_PATCH
           "  -d  Diff only, do't run patch check, DEFAULT run patch check.\n"
           "  -t  Test only, run patch check, patch(oldPath,testDiffFile)==newPath ? \n"
           "  -f  Force overwrite, ignore write path already exists;\n"
           "      DEFAULT (no -f) not overwrite and then return error;\n"
           "      if used -f and write path is exist directory, will always return error.\n"
           "  --patch\n"
           "      swap to hpatchz mode.\n"
           "  -h or -?\n"
           "      output Help info (this usage).\n"
           "  -v  output Version info.\n\n"
           );
}



typedef enum THDiffResult {
    HDIFF_SUCCESS=0,
    HDIFF_OPTIONS_ERROR,
    HDIFF_OPENREAD_ERROR,
    HDIFF_OPENWRITE_ERROR,
    HDIFF_FILECLOSE_ERROR,
    HDIFF_MEM_ERROR, // 5
    HDIFF_DIFF_ERROR,
    HDIFF_PATCH_ERROR,
    HDIFF_RESAVE_FILEREAD_ERROR,
    //HDIFF_RESAVE_OPENWRITE_ERROR = HDIFF_OPENWRITE_ERROR
    HDIFF_RESAVE_DIFFINFO_ERROR,
    HDIFF_RESAVE_COMPRESSTYPE_ERROR, // 10
    HDIFF_RESAVE_ERROR,
    HDIFF_RESAVE_CHECKSUMTYPE_ERROR,
    
    HDIFF_PATHTYPE_ERROR, //adding begin v3.0
    HDIFF_TEMPPATH_ERROR,
    HDIFF_DELETEPATH_ERROR, // 15
    HDIFF_RENAMEPATH_ERROR,
    
    DIRDIFF_DIFF_ERROR=101,
    DIRDIFF_PATCH_ERROR,
    MANIFEST_CREATE_ERROR,
    MANIFEST_TEST_ERROR,
} THDiffResult;
#define HDIFF_RESAVE_OPENWRITE_ERROR HDIFF_OPENWRITE_ERROR

typedef enum DiffResult {
    SUCCESS = 0,
    ARGUMENTS_NUMBER_ERROR,
    OPEN_OLDFILE_ERROR,
    OPEN_NEWFILE_ERROR,
    CREATE_PATCHFILE_ERROR,
};

int hdiff_cmd_line(int argc,const char * argv[]);
int hdiff_cmd_line_v2(int argc, const char* argv[]);

struct TDiffSets:public THDiffSets{
    hpatch_BOOL isDoDiff;
    hpatch_BOOL isDoPatchCheck;
};


int hdiff(const char* oldFileName,const char* newFileName,const char* outDiffFileName,
          const hdiff_TCompress* compressPlugin,hpatch_TDecompress* decompressPlugin,
          const TDiffSets& diffSets);
int hdiff_resave(const char* diffFileName,const char* outDiffFileName,
                 const hdiff_TCompress* compressPlugin);

#define _checkPatchMode(_argc,_argv)            \
    if (isSwapToPatchMode(_argc,_argv)){        \
        printf("hdiffz swap to hpatchz mode.\n\n"); \
        return hpatch_cmd_line(_argc,_argv);    \
    }

//#if (_IS_NEED_MAIN)
//#   if (_IS_USED_WIN32_UTF8_WAPI)
//int wmain(int argc,wchar_t* argv_w[]){
//    hdiff_private::TAutoMem  _mem(hpatch_kPathMaxSize*4);
//    char** argv_utf8=(char**)_mem.data();
//    if (!_wFileNames_to_utf8((const wchar_t**)argv_w,argc,argv_utf8,_mem.size()))
//        return HDIFF_OPTIONS_ERROR;
//    SetDefaultStringLocale();
//    _checkPatchMode(argc,(const char**)argv_utf8);
//    return hdiff_cmd_line_v2(argc,(const char**)argv_w);
//}
//#   else
//int main(int argc,char* argv[]){
//    _checkPatchMode(argc,(const char**)argv);
//    return  hdiff_cmd_line_v2(argc,(const char**)argv);
//}
//#   endif
//#endif

int main(int argc, const char** argv) {
    return  hdiff_cmd_line_v2(argc, argv);
}
static hpatch_BOOL _getIsCompressedDiffFile(const char* diffFileName){
    hpatch_TFileStreamInput diffData;
    hpatch_TFileStreamInput_init(&diffData);
    if (!hpatch_TFileStreamInput_open(&diffData,diffFileName)) return hpatch_FALSE;
    hpatch_compressedDiffInfo diffInfo;
    hpatch_BOOL result=getCompressedDiffInfo(&diffInfo,&diffData.base);
    if (!hpatch_TFileStreamInput_close(&diffData)) return hpatch_FALSE;
    return result;
}

static hpatch_BOOL _getIsSingleStreamDiffFile(const char* diffFileName){
    hpatch_TFileStreamInput diffData;
    hpatch_TFileStreamInput_init(&diffData);
    if (!hpatch_TFileStreamInput_open(&diffData,diffFileName)) return hpatch_FALSE;
    hpatch_singleCompressedDiffInfo diffInfo;
    hpatch_BOOL result=getSingleCompressedDiffInfo(&diffInfo,&diffData.base,0);
    if (!hpatch_TFileStreamInput_close(&diffData)) return hpatch_FALSE;
    return result;
}


static inline bool _trySetDecompress(hpatch_TDecompress** out_decompressPlugin,const char* compressType,
                                     hpatch_TDecompress* testDecompressPlugin){
    assert(0==*out_decompressPlugin);
    if (!testDecompressPlugin->is_can_open(compressType)) return false;
    *out_decompressPlugin=testDecompressPlugin;
    return true;
}
#define __setDecompress(_decompressPlugin) \
    if (_trySetDecompress(out_decompressPlugin,compressType,_decompressPlugin)) return hpatch_TRUE;

static hpatch_BOOL findDecompress(hpatch_TDecompress** out_decompressPlugin,const char* compressType){
    *out_decompressPlugin=0;
    if (strlen(compressType)==0) return hpatch_TRUE;
    return hpatch_FALSE;
}



static bool _tryGetCompressSet(const char** isMatchedType,const char* ptype,const char* ptypeEnd,
                               const char* cmpType,const char* cmpType2=0,
                               size_t* compressLevel=0,size_t levelMin=0,size_t levelMax=0,size_t levelDefault=0,
                               size_t* dictSize=0,size_t dictSizeMin=0,size_t dictSizeMax=0,size_t dictSizeDefault=0){
    if (*isMatchedType) return true; //ok
    const size_t ctypeLen=strlen(cmpType);
    const size_t ctype2Len=(cmpType2!=0)?strlen(cmpType2):0;
    if ( ((ctypeLen==(size_t)(ptypeEnd-ptype))&&(0==strncmp(ptype,cmpType,ctypeLen)))
        || ((cmpType2!=0)&&(ctype2Len==(size_t)(ptypeEnd-ptype))&&(0==strncmp(ptype,cmpType2,ctype2Len))) )
        *isMatchedType=cmpType; //ok
    else
        return true;//type mismatch
    
    if ((compressLevel)&&(ptypeEnd[0]=='-')){
        const char* plevel=ptypeEnd+1;
        const char* plevelEnd=findUntilEnd(plevel,'-');
        if (!a_to_size(plevel,plevelEnd-plevel,compressLevel)) return false; //error
        if (*compressLevel<levelMin) *compressLevel=levelMin;
        else if (*compressLevel>levelMax) *compressLevel=levelMax;
        if ((dictSize)&&(plevelEnd[0]=='-')){
            const char* pdictSize=plevelEnd+1;
            const char* pdictSizeEnd=findUntilEnd(pdictSize,'-');
            if (!kmg_to_size(pdictSize,pdictSizeEnd-pdictSize,dictSize)) return false; //error
            if (*dictSize<dictSizeMin) *dictSize=dictSizeMin;
            else if (*dictSize>dictSizeMax) *dictSize=dictSizeMax;
        }else{
            if (plevelEnd[0]!='\0') return false; //error
            if (dictSize) *dictSize=dictSizeDefault;
        }
    }else{
        if (ptypeEnd[0]!='\0') return false; //error
        if (compressLevel) *compressLevel=levelDefault;
        if (dictSize) *dictSize=dictSizeDefault;
    }
    return true;
}

#define _options_check(value,errorInfo){ \
    if (!(value)) { LOG_ERR("options " errorInfo " ERROR!\n\n"); printUsage(); return HDIFF_OPTIONS_ERROR; } }

static int _checkSetCompress(hdiff_TCompress** out_compressPlugin,
                             hpatch_TDecompress** out_decompressPlugin,
                             const char* ptype,const char* ptypeEnd){
    const char* isMatchedType=0;
    size_t      compressLevel=0;
#if (defined _CompressPlugin_lzma)||(defined _CompressPlugin_lzma2)
    size_t       dictSize=0;
    const size_t defaultDictSize=(1<<20)*8;
#endif
#if (defined _CompressPlugin_zstd)||(defined _CompressPlugin_brotli)||(defined _CompressPlugin_lzham)
    size_t       dictBits=0;
    const size_t defaultDictBits=20+4;
#endif

    _options_check((*out_compressPlugin!=0)&&(*out_decompressPlugin!=0),"-c-?");
    return HDIFF_SUCCESS;
}

#define _return_check(value,exitCode,errorInfo){ \
    if (!(value)) { LOG_ERR(errorInfo " ERROR!\n"); return exitCode; } }

#define _kNULL_VALUE    ((hpatch_BOOL)(-1))
#define _kNULL_SIZE     (~(size_t)0)

#define _THREAD_NUMBER_NULL     0
#define _THREAD_NUMBER_MIN      1
#define _THREAD_NUMBER_DEFUALT  1
#define _THREAD_NUMBER_MAX      (1<<8)

static size_t getFileSize(FILE* file) {
    fseek(file, 0, SEEK_END);
    size_t read_len = ftell(file);
    fseek(file, 0, SEEK_SET);
    return read_len;
}

int create_hdiffo_patch(const char* oldname, const char* newname, const char* patchname) 
{
    FILE* oldfile = fopen(oldname, "rb");
    if (oldfile == NULL)
    {
        printf("error: open old file \"%s\" failed.\n", oldname);
        return DiffResult::OPEN_OLDFILE_ERROR;
    }
    FILE* newfile = fopen(newname, "rb");
    if (newfile == NULL)
    {
        printf("error: open new file \"%s\" failed.\n", newname);
        return DiffResult::OPEN_NEWFILE_ERROR;
    }
    FILE* patchfile = fopen(patchname, "wb");
    if (patchfile == NULL)
    {
        printf("error: open patch file \"%s\" failed.\n", patchname);
        return DiffResult::CREATE_PATCHFILE_ERROR;
    }


    size_t oldsize = getFileSize(oldfile);
    size_t newsize = getFileSize(newfile);

    TByte* oldData = (TByte*)malloc(oldsize);
    TByte* newData = (TByte*)malloc(newsize);
    fread(oldData, 1, oldsize, oldfile);
    fread(newData, 1, newsize, newfile);
    std::vector<TByte> diffData;
    create_diff(newData, newData+newsize, oldData, oldData+oldsize, diffData);

    ////Ñ¹Ëõpatch
    //{
    //    unsigned short crc = CRC16_CCITT(diffData.data(), diffData.size());
    //    char c1 = (crc >> 8) & 0xff;
    //    char c2 = crc & 0xff;
    //    fwrite(&c1, 1, 1, patchfile);
    //    fwrite(&c2, 1, 1, patchfile);
    //    size_t d, c;
    //    size_t count = diffData.size() / 3696 + 1;
    //    char* compressed = (char*)malloc(4096);
    //    qlz_state_compress* state_compress = (qlz_state_compress*)malloc(sizeof(qlz_state_compress));
    //    for (size_t i = 0; i < count; i++) {
    //        d = (i == count - 1) ? diffData.size() - i * 3696 : 3696;
    //        c = qlz_compress(diffData.data() + i * 3696, compressed, d, state_compress);
    //        printf("%u bytes compressed into %u\n", (unsigned int)d, (unsigned int)c);
    //        fwrite(compressed, c, 1, patchfile);
    //    }
    //    
    //}


    fwrite(diffData.data(), 1, diffData.size(), patchfile);

    fclose(oldfile);
    fclose(patchfile);
    fclose(newfile);



    printf("success!\n");
    return DiffResult::SUCCESS;
}

int hdiff_cmd_line_v2(int argc, const char* argv[]) {
    if (argc != 4)
    {
        printf("error: There must be three parameters: oldfile, newfile and patchfile.\n");
        printf("Example: diff.exe old.bin new.bin patch\n");
        return DiffResult::ARGUMENTS_NUMBER_ERROR;
    }
    return create_hdiffo_patch(argv[1], argv[2], argv[3]);
}

int hdiff_cmd_line(int argc, const char * argv[]){
    TDiffSets diffSets; 
    memset(&diffSets,0,sizeof(diffSets));
    diffSets.isDoDiff =_kNULL_VALUE;
    diffSets.isDoPatchCheck=_kNULL_VALUE;
    diffSets.isDiffInMem   =_kNULL_VALUE;
    diffSets.isSingleCompressedDiff =_kNULL_VALUE;
    diffSets.isUseBigCacheMatch =_kNULL_VALUE;
    diffSets.isUseFastMatchBlock=_kNULL_VALUE;
    hpatch_BOOL isForceOverwrite=_kNULL_VALUE;
    hpatch_BOOL isOutputHelp=_kNULL_VALUE;
    hpatch_BOOL isOutputVersion=_kNULL_VALUE;
    hpatch_BOOL isOldPathInputEmpty=_kNULL_VALUE;
    size_t      threadNum = _THREAD_NUMBER_NULL;
    hdiff_TCompress*        compressPlugin=0;
    hpatch_TDecompress*     decompressPlugin=0;
    std::vector<const char *> arg_values;
    for (int i=1; i<argc; ++i) {
        const char* op=argv[i];
        _options_check(op!=0,"?");
        if (op[0]!='-'){
            hpatch_BOOL isEmpty=(strlen(op)==0);
            if (isEmpty){
                if (isOldPathInputEmpty==_kNULL_VALUE)
                    isOldPathInputEmpty=hpatch_TRUE;
                else
                    _options_check(!isEmpty,"?"); //error return
            }else{
                if (isOldPathInputEmpty==_kNULL_VALUE)
                    isOldPathInputEmpty=hpatch_FALSE;
            }
            arg_values.push_back(op); //path:file or directory
            continue;
        }
        switch (op[1]) {
            case 'm':{ //diff in memory
                _options_check((diffSets.isDiffInMem==_kNULL_VALUE)&&((op[2]=='-')||(op[2]=='\0')),"-m");
                diffSets.isDiffInMem=hpatch_TRUE;
                if (op[2]=='-'){
                    const char* pnum=op+3;
                    _options_check(kmg_to_size(pnum,strlen(pnum),&diffSets.matchScore),"-m-?");
                    _options_check((0<=(int)diffSets.matchScore)&&(diffSets.matchScore==(size_t)(int)diffSets.matchScore),"-m-?");
                }else{
                    diffSets.matchScore=kMinSingleMatchScore_default;
                }
            } break;
            case 's':{
                diffSets.isDiffInMem=hpatch_FALSE; //diff by stream
                if (op[2]=='-'){
                    const char* pnum=op+3;
                    _options_check(kmg_to_size(pnum,strlen(pnum),&diffSets.matchBlockSize),"-s-?");
                    _options_check(kMatchBlockSize_min<=diffSets.matchBlockSize,"-s-?");
                }else{
                    diffSets.matchBlockSize=kMatchBlockSize_default;
                }
            } break;
            case 'S':{
                _options_check((diffSets.isSingleCompressedDiff==_kNULL_VALUE)
                               &&(op[2]=='D')&&((op[3]=='\0')||(op[3]=='-')),"-SD");
                diffSets.isSingleCompressedDiff=hpatch_TRUE;
                if (op[3]=='-'){
                    const char* pnum=op+4;
                    _options_check(kmg_to_size(pnum,strlen(pnum),&diffSets.patchStepMemSize),"-SD-?");
                    _options_check((diffSets.patchStepMemSize>=hpatch_kStreamCacheSize),"-SD-?");
                }else{
                    diffSets.patchStepMemSize=kDefaultPatchStepMemSize;
                }
            } break;
            case '?':
            case 'h':{
                _options_check((isOutputHelp==_kNULL_VALUE)&&(op[2]=='\0'),"-h");
                isOutputHelp=hpatch_TRUE;
            } break;
            case 'v':{
                _options_check((isOutputVersion==_kNULL_VALUE)&&(op[2]=='\0'),"-v");
                isOutputVersion=hpatch_TRUE;
            } break;
            case 't':{
                _options_check((diffSets.isDoDiff==_kNULL_VALUE)&&(diffSets.isDoPatchCheck==_kNULL_VALUE)&&(op[2]=='\0'),"-t -d?");
                diffSets.isDoDiff=hpatch_FALSE;
                diffSets.isDoPatchCheck=hpatch_TRUE; //only test diffFile
            } break;
            case 'd':{
                _options_check((diffSets.isDoDiff==_kNULL_VALUE)&&(diffSets.isDoPatchCheck==_kNULL_VALUE)&&(op[2]=='\0'),"-t -d?");
                diffSets.isDoDiff=hpatch_TRUE; //only diff
                diffSets.isDoPatchCheck=hpatch_FALSE;
            } break;
            case 'f':{
                _options_check((isForceOverwrite==_kNULL_VALUE)&&(op[2]=='\0'),"-f");
                isForceOverwrite=hpatch_TRUE;
            } break;
#if (_IS_USED_MULTITHREAD)
            case 'p':{
                _options_check((threadNum==_THREAD_NUMBER_NULL)&&(op[2]=='-'),"-p-?");
                const char* pnum=op+3;
                _options_check(a_to_size(pnum,strlen(pnum),&threadNum),"-p-?");
                _options_check(threadNum>=_THREAD_NUMBER_MIN,"-p-?");
            } break;
#endif
            case 'b':{
                _options_check((diffSets.isUseFastMatchBlock==_kNULL_VALUE)&&
                    (op[2]=='l')&&(op[3]=='o')&&(op[4]=='c')&&(op[5]=='k')&&
                    ((op[6]=='\0')||(op[6]=='-')),"-block?");
                diffSets.isUseFastMatchBlock=hpatch_TRUE; //use block match 
                if (op[6]=='-'){
                    const char* pnum=op+7;
                    _options_check(kmg_to_size(pnum,strlen(pnum),&diffSets.fastMatchBlockSize),"-block-?");
                    _options_check(kMatchBlockSize_min<=diffSets.fastMatchBlockSize,"-block-?");
                }else{
                    diffSets.fastMatchBlockSize=kDefaultFastMatchBlockSize;
                }
            } break;
            case 'c':{
                if (op[2]=='-'){
                    _options_check((compressPlugin==0),"-c-");
                    const char* ptype=op+3;
                    const char* ptypeEnd=findUntilEnd(ptype,'-');
                    int result=_checkSetCompress(&compressPlugin,&decompressPlugin,ptype,ptypeEnd);
                    if (HDIFF_SUCCESS!=result)
                        return result;
                }else if (op[2]=='a'){
                    _options_check((diffSets.isUseBigCacheMatch==_kNULL_VALUE)&&
                        (op[3]=='c')&&(op[4]=='h')&&(op[5]=='e')&&(op[6]=='\0'),"-cache?");
                    diffSets.isUseBigCacheMatch=hpatch_TRUE; //use big cache for match 
                }else{
                    _options_check(false,"-c?");
                }
            } break;
            default: {
                _options_check(hpatch_FALSE,"-?");
            } break;
        }//switch
    }
    
    if (isOutputHelp==_kNULL_VALUE)
        isOutputHelp=hpatch_FALSE;
    if (isOutputVersion==_kNULL_VALUE)
        isOutputVersion=hpatch_FALSE;
    if (isForceOverwrite==_kNULL_VALUE)
        isForceOverwrite=hpatch_FALSE;
    if (isOutputHelp||isOutputVersion){
        if (isOutputHelp)
            printUsage();//with version
        else
            printVersion();
        if (arg_values.empty())
            return 0; //ok
    }
    if (diffSets.isSingleCompressedDiff==_kNULL_VALUE)
        diffSets.isSingleCompressedDiff=hpatch_FALSE;


    if (threadNum==_THREAD_NUMBER_NULL)
        threadNum=_THREAD_NUMBER_DEFUALT;
    else if (threadNum>_THREAD_NUMBER_MAX)
        threadNum=_THREAD_NUMBER_MAX;
    if (compressPlugin!=0){
        compressPlugin->setParallelThreadNumber(compressPlugin,(int)threadNum);
    }
    
    if (isOldPathInputEmpty==_kNULL_VALUE)
        isOldPathInputEmpty=hpatch_FALSE;
    _options_check((arg_values.size()==1)||(arg_values.size()==2)||(arg_values.size()==3),"input count");
    if (arg_values.size()==3){ //diff
        if (diffSets.isDiffInMem==_kNULL_VALUE){
            diffSets.isDiffInMem=hpatch_TRUE;
            diffSets.matchScore=kMinSingleMatchScore_default;
        }
        if (diffSets.isDoDiff==_kNULL_VALUE)
            diffSets.isDoDiff=hpatch_TRUE;
        if (diffSets.isDoPatchCheck==_kNULL_VALUE)
            diffSets.isDoPatchCheck=hpatch_TRUE;
        assert(diffSets.isDoDiff||diffSets.isDoPatchCheck);
        if (diffSets.isUseFastMatchBlock==_kNULL_VALUE)
            diffSets.isUseFastMatchBlock=hpatch_FALSE;
        if (diffSets.isUseBigCacheMatch==_kNULL_VALUE)
            diffSets.isUseBigCacheMatch=hpatch_FALSE;
        if (diffSets.isDoDiff&&(!diffSets.isDiffInMem)){
            _options_check(!diffSets.isUseBigCacheMatch, "-cache must run with -m");
            _options_check(!diffSets.isUseFastMatchBlock,"-block must run with -m");
        }

        const char* oldPath        =arg_values[0];
        const char* newPath        =arg_values[1];
        const char* outDiffFileName=arg_values[2];
        
        _return_check(!hpatch_getIsSamePath(oldPath,outDiffFileName),
                      HDIFF_PATHTYPE_ERROR,"oldPath outDiffFile same path");
        _return_check(!hpatch_getIsSamePath(newPath,outDiffFileName),
                      HDIFF_PATHTYPE_ERROR,"newPath outDiffFile same path");
        if (!isForceOverwrite){//0
            hpatch_TPathType   outDiffFileType;
            _return_check(hpatch_getPathStat(outDiffFileName,&outDiffFileType,0),
                          HDIFF_PATHTYPE_ERROR,"get outDiffFile type");
            _return_check((outDiffFileType==kPathType_notExist)||(!diffSets.isDoDiff),
                          HDIFF_PATHTYPE_ERROR,"diff outDiffFile already exists, overwrite");
        }
        hpatch_TPathType oldType;
        hpatch_TPathType newType;
        if (isOldPathInputEmpty){
            oldType=kPathType_file;     //as empty file
            {
                diffSets.isDiffInMem=hpatch_FALSE;     //not need -m, set as -s
                diffSets.matchBlockSize=hpatch_kStreamCacheSize; //not used
                diffSets.isUseBigCacheMatch=hpatch_FALSE;
                diffSets.isUseFastMatchBlock=hpatch_FALSE;
            }
        }else{
            _return_check(hpatch_getPathStat(oldPath,&oldType,0),HDIFF_PATHTYPE_ERROR,"get oldPath type");
            _return_check((oldType!=kPathType_notExist),HDIFF_PATHTYPE_ERROR,"oldPath not exist");
        }
        _return_check(hpatch_getPathStat(newPath,&newType,0),HDIFF_PATHTYPE_ERROR,"get newPath type");
        _return_check((newType!=kPathType_notExist),HDIFF_PATHTYPE_ERROR,"newPath not exist");

        {
            return hdiff(oldPath,newPath,outDiffFileName,
                         compressPlugin,decompressPlugin,diffSets);
        }

    }else{// (arg_values.size()==2)  //resave
        _options_check(!isOldPathInputEmpty,"can't resave, must input a diffFile");
        _options_check((diffSets.isDoDiff==_kNULL_VALUE),"-d unsupport run with resave mode");
        _options_check((diffSets.isDoPatchCheck==_kNULL_VALUE),"-t unsupport run with resave mode");

        

        const char* diffFileName   =arg_values[0];
        const char* outDiffFileName=arg_values[1];
        hpatch_BOOL isDiffFile=_getIsCompressedDiffFile(diffFileName);
        isDiffFile=isDiffFile || _getIsSingleStreamDiffFile(diffFileName);

        _return_check(isDiffFile,HDIFF_RESAVE_DIFFINFO_ERROR,"can't resave, input file is not diffFile");
        if (!isForceOverwrite){
            hpatch_TPathType   outDiffFileType;
            _return_check(hpatch_getPathStat(outDiffFileName,&outDiffFileType,0),
                          HDIFF_PATHTYPE_ERROR,"get outDiffFile type");
            _return_check(outDiffFileType==kPathType_notExist,
                          HDIFF_PATHTYPE_ERROR,"resave outDiffFile already exists, overwrite");
        }
        hpatch_BOOL isSamePath=hpatch_getIsSamePath(diffFileName,outDiffFileName);
        if (isSamePath)
            _return_check(isForceOverwrite,HDIFF_PATHTYPE_ERROR,"diffFile outDiffFile same name");
        
        if (!isSamePath){
            return hdiff_resave(diffFileName,outDiffFileName,compressPlugin);
        }else{
            // 1. resave to newDiffTempName
            // 2. if resave ok    then  { delelte oldDiffFile; rename newDiffTempName to oldDiffName; }
            //    if resave error then  { delelte newDiffTempName; }
            char newDiffTempName[hpatch_kPathMaxSize];
            _return_check(hpatch_getTempPathName(outDiffFileName,newDiffTempName,newDiffTempName+hpatch_kPathMaxSize),
                          HDIFF_TEMPPATH_ERROR,"getTempPathName(diffFile)");
            printf("NOTE: out_diff temp file will be rename to in_diff name after resave!\n");
            int result=hdiff_resave(diffFileName,newDiffTempName,compressPlugin);
            if (result==0){//resave ok
                _return_check(hpatch_removeFile(diffFileName),
                              HDIFF_DELETEPATH_ERROR,"removeFile(diffFile)");
                _return_check(hpatch_renamePath(newDiffTempName,diffFileName),
                              HDIFF_RENAMEPATH_ERROR,"renamePath(temp,diffFile)");
                printf("out_diff temp file renamed to in_diff name!\n");
            }else{//resave error
                if (!hpatch_removeFile(newDiffTempName)){
                    printf("WARNING: can't remove temp file \"");
                    hpatch_printPath_utf8(newDiffTempName); printf("\"\n");
                }
            }
            return result;
        }
    }
}

#define _check_readFile(value) { if (!(value)) { hpatch_TFileStreamInput_close(&file); return hpatch_FALSE; } }
static hpatch_BOOL readFileAll(hdiff_private::TAutoMem& out_mem,const char* fileName){
    size_t              dataSize;
    hpatch_TFileStreamInput    file;
    hpatch_TFileStreamInput_init(&file);
    _check_readFile(hpatch_TFileStreamInput_open(&file,fileName));
    dataSize=(size_t)file.base.streamSize;
    _check_readFile(dataSize==file.base.streamSize);
    try {
        out_mem.realloc(dataSize);
    } catch (...) {
        _check_readFile(false);
    }
    _check_readFile(file.base.read(&file.base,0,out_mem.data(),out_mem.data_end()));
    return hpatch_TFileStreamInput_close(&file);
}

#define  _check_on_error(errorType) { \
    if (result==HDIFF_SUCCESS) result=errorType; if (!_isInClear){ goto clear; } }
#define check(value,errorType,errorInfo) { \
    std::string erri=std::string()+errorInfo+" ERROR!\n"; \
    if (!(value)){ hpatch_printStdErrPath_utf8(erri.c_str()); _check_on_error(errorType); } }

static int hdiff_in_mem(const char* oldFileName,const char* newFileName,const char* outDiffFileName,
                        const hdiff_TCompress* compressPlugin,hpatch_TDecompress* decompressPlugin,
                        const TDiffSets& diffSets){
    double diff_time0=clock_s();
    int    result=HDIFF_SUCCESS;
    int    _isInClear=hpatch_FALSE;
    hpatch_TFileStreamOutput diffData_out;
    hpatch_TFileStreamOutput_init(&diffData_out);
    hdiff_private::TAutoMem oldMem(0);
    hdiff_private::TAutoMem newMem(0);
    if (oldFileName&&(strlen(oldFileName)>0))
        check(readFileAll(oldMem,oldFileName),HDIFF_OPENREAD_ERROR,"open oldFile");
    check(readFileAll(newMem,newFileName),HDIFF_OPENREAD_ERROR,"open newFile");
    printf("oldDataSize : %" PRIu64 "\nnewDataSize : %" PRIu64 "\n",
           (hpatch_StreamPos_t)oldMem.size(),(hpatch_StreamPos_t)newMem.size());
    if (diffSets.isDoDiff){
        check(hpatch_TFileStreamOutput_open(&diffData_out,outDiffFileName,~(hpatch_StreamPos_t)0),
                HDIFF_OPENWRITE_ERROR,"open out diffFile");
        hpatch_TFileStreamOutput_setRandomOut(&diffData_out,hpatch_TRUE);
        try {
            if (diffSets.isSingleCompressedDiff){
              if (diffSets.isUseFastMatchBlock)
                create_single_compressed_diff_block(newMem.data(),newMem.data_end(),oldMem.data(),oldMem.data_end(),
                                                    &diffData_out.base,compressPlugin,(int)diffSets.matchScore,
                                                    diffSets.patchStepMemSize,diffSets.isUseBigCacheMatch,diffSets.fastMatchBlockSize);   
              else
                create_single_compressed_diff(newMem.data(),newMem.data_end(),oldMem.data(),oldMem.data_end(),
                                              &diffData_out.base,compressPlugin,(int)diffSets.matchScore,
                                              diffSets.patchStepMemSize,diffSets.isUseBigCacheMatch);        
            }else{
              if (diffSets.isUseFastMatchBlock)
                create_compressed_diff_block(newMem.data(),newMem.data_end(),oldMem.data(),oldMem.data_end(),
                                             &diffData_out.base,compressPlugin,(int)diffSets.matchScore,
                                             diffSets.isUseBigCacheMatch,diffSets.fastMatchBlockSize);
              else
                create_compressed_diff(newMem.data(),newMem.data_end(),oldMem.data(),oldMem.data_end(),
                                       &diffData_out.base,compressPlugin,(int)diffSets.matchScore,
                                       diffSets.isUseBigCacheMatch);
            }
            diffData_out.base.streamSize=diffData_out.out_length;
        }catch(const std::exception& e){
            check(!diffData_out.fileError,HDIFF_OPENWRITE_ERROR,"write diffFile");
            check(false,HDIFF_DIFF_ERROR,"diff run error: "+e.what());
        }
        const hpatch_StreamPos_t outDiffDataSize=diffData_out.base.streamSize;
        check(hpatch_TFileStreamOutput_close(&diffData_out),HDIFF_FILECLOSE_ERROR,"out diffFile close");
        printf("diffDataSize: %" PRIu64 "\n",outDiffDataSize);
        printf("diff    time: %.3f s\n",(clock_s()-diff_time0));
        printf("  out diff file ok!\n");
    }
    if (diffSets.isDoPatchCheck){
        hpatch_BOOL isSingleCompressedDiff=hpatch_FALSE;
        hdiff_private::TAutoMem diffMem(0);
        double patch_time0=clock_s();
        printf("\nload diffFile for test by patch:\n");
        check(readFileAll(diffMem,outDiffFileName),
              HDIFF_OPENREAD_ERROR,"open diffFile for test");
        printf("diffDataSize: %" PRIu64 "\n",(hpatch_StreamPos_t)diffMem.size());
        
        hpatch_TDecompress* saved_decompressPlugin=0;
        {
            hpatch_compressedDiffInfo diffinfo;
            hpatch_singleCompressedDiffInfo sdiffInfo;
            const char* compressType=0;
            if (getCompressedDiffInfo_mem(&diffinfo,diffMem.data(),diffMem.data_end())){
                compressType=diffinfo.compressType;
            }else if (getSingleCompressedDiffInfo_mem(&sdiffInfo,diffMem.data(),diffMem.data_end())){
                compressType=sdiffInfo.compressType;
                isSingleCompressedDiff=hpatch_TRUE;
                if (!diffSets.isDoDiff)
                    printf("test single compressed diffData!\n");
            }else{
                check(hpatch_FALSE,HDIFF_PATCH_ERROR,"get diff info");
            }
            if (!saved_decompressPlugin)
                check(findDecompress(&saved_decompressPlugin,compressType),
                      HDIFF_PATCH_ERROR,"diff data saved compress type");
        }
        
        bool diffrt;
        if (isSingleCompressedDiff)
            diffrt=check_single_compressed_diff(newMem.data(),newMem.data_end(),oldMem.data(),oldMem.data_end(),
                                                diffMem.data(),diffMem.data_end(),saved_decompressPlugin);
        else
            diffrt=check_compressed_diff(newMem.data(),newMem.data_end(),oldMem.data(),oldMem.data_end(),
                                         diffMem.data(),diffMem.data_end(),saved_decompressPlugin);
        check(diffrt,HDIFF_PATCH_ERROR,"patch check diff data");
        printf("patch   time: %.3f s\n",(clock_s()-patch_time0));
        printf("  patch check diff data ok!\n");
    }
clear:
    _isInClear=hpatch_TRUE;
    check(hpatch_TFileStreamOutput_close(&diffData_out),HDIFF_FILECLOSE_ERROR,"out diffFile close");
    return result;
}

static int hdiff_by_stream(const char* oldFileName,const char* newFileName,const char* outDiffFileName,
                           const hdiff_TCompress* compressPlugin,hpatch_TDecompress* decompressPlugin,
                           const TDiffSets& diffSets){
    double diff_time0=clock_s();
    int result=HDIFF_SUCCESS;
    int _isInClear=hpatch_FALSE;
    hpatch_TFileStreamInput  oldData;
    hpatch_TFileStreamInput  newData;
    hpatch_TFileStreamOutput diffData_out;
    hpatch_TFileStreamInput  diffData_in;
    hpatch_TFileStreamInput_init(&oldData);
    hpatch_TFileStreamInput_init(&newData);
    hpatch_TFileStreamOutput_init(&diffData_out);
    hpatch_TFileStreamInput_init(&diffData_in);
    
    if (oldFileName&&(strlen(oldFileName)>0)){
        check(hpatch_TFileStreamInput_open(&oldData,oldFileName),HDIFF_OPENREAD_ERROR,"open oldFile");
    }else{
        mem_as_hStreamInput(&oldData.base,0,0);
    }
    check(hpatch_TFileStreamInput_open(&newData,newFileName),HDIFF_OPENREAD_ERROR,"open newFile");
    printf("oldDataSize : %" PRIu64 "\nnewDataSize : %" PRIu64 "\n",
           oldData.base.streamSize,newData.base.streamSize);
    if (diffSets.isDoDiff){
        check(hpatch_TFileStreamOutput_open(&diffData_out,outDiffFileName,~(hpatch_StreamPos_t)0),
              HDIFF_OPENWRITE_ERROR,"open out diffFile");
        hpatch_TFileStreamOutput_setRandomOut(&diffData_out,hpatch_TRUE);
        try{
            if (diffSets.isSingleCompressedDiff)
                create_single_compressed_diff_stream(&newData.base,&oldData.base, &diffData_out.base,
                                                     compressPlugin,diffSets.matchBlockSize,diffSets.patchStepMemSize);
            else
                create_compressed_diff_stream(&newData.base,&oldData.base, &diffData_out.base,
                                              compressPlugin,diffSets.matchBlockSize);
            diffData_out.base.streamSize=diffData_out.out_length;
        }catch(const std::exception& e){
            check(!newData.fileError,HDIFF_OPENREAD_ERROR,"read newFile");
            check(!oldData.fileError,HDIFF_OPENREAD_ERROR,"read oldFile");
            check(!diffData_out.fileError,HDIFF_OPENWRITE_ERROR,"write diffFile");
            check(false,HDIFF_DIFF_ERROR,"stream diff run an error: "+e.what());
        }
        const hpatch_StreamPos_t outDiffDataSize=diffData_out.base.streamSize;
        check(hpatch_TFileStreamOutput_close(&diffData_out),HDIFF_FILECLOSE_ERROR,"out diffFile close");
        printf("diffDataSize: %" PRIu64 "\n",outDiffDataSize);
        printf("diff    time: %.3f s\n",(clock_s()-diff_time0));
        printf("  out diff file ok!\n");
    }
    if (diffSets.isDoPatchCheck){
        double patch_time0=clock_s();
        printf("\nload diffFile for test by patch check:\n");
        check(hpatch_TFileStreamInput_open(&diffData_in,outDiffFileName),HDIFF_OPENREAD_ERROR,"open check diffFile");
        printf("diffDataSize: %" PRIu64 "\n",diffData_in.base.streamSize);

        hpatch_BOOL isSingleCompressedDiff=hpatch_FALSE;
        hpatch_TDecompress* saved_decompressPlugin=0;
        {
            hpatch_compressedDiffInfo diffInfo;
            hpatch_singleCompressedDiffInfo sdiffInfo;
            const char* compressType=0;
            if (getCompressedDiffInfo(&diffInfo,&diffData_in.base)){
                compressType=diffInfo.compressType;
            }else if (getSingleCompressedDiffInfo(&sdiffInfo,&diffData_in.base,0)){
                compressType=sdiffInfo.compressType;
                isSingleCompressedDiff=hpatch_TRUE;
                if (!diffSets.isDoDiff)
                    printf("test single compressed diffData!\n");
            }else{
                check(hpatch_FALSE,HDIFF_PATCH_ERROR,"get diff info");
            }
            if (!saved_decompressPlugin)
              check(findDecompress(&saved_decompressPlugin,compressType),
                    HDIFF_PATCH_ERROR,"diff data saved compress type");
        }
        bool diffrt;
        if (isSingleCompressedDiff)
            diffrt=check_single_compressed_diff(&newData.base,&oldData.base,&diffData_in.base,saved_decompressPlugin);
        else
            diffrt=check_compressed_diff(&newData.base,&oldData.base,&diffData_in.base,saved_decompressPlugin);
        check(diffrt,HDIFF_PATCH_ERROR,"patch check diff data");
        printf("patch   time: %.3f s\n",(clock_s()-patch_time0));
        printf("  patch check diff data ok!\n");
    }
clear:
    _isInClear=hpatch_TRUE;
    check(hpatch_TFileStreamOutput_close(&diffData_out),HDIFF_FILECLOSE_ERROR,"out diffFile close");
    check(hpatch_TFileStreamInput_close(&diffData_in),HDIFF_FILECLOSE_ERROR,"in diffFile close");
    check(hpatch_TFileStreamInput_close(&newData),HDIFF_FILECLOSE_ERROR,"newFile close");
    check(hpatch_TFileStreamInput_close(&oldData),HDIFF_FILECLOSE_ERROR,"oldFile close");
    return result;
}

int hdiff(const char* oldFileName,const char* newFileName,const char* outDiffFileName,
          const hdiff_TCompress* compressPlugin,hpatch_TDecompress* decompressPlugin,
          const TDiffSets& diffSets){
    double time0=clock_s();
    std::string fnameInfo=std::string("old : \"")+oldFileName+"\"\n"
                                     +"new : \""+newFileName+"\"\n"
                             +(diffSets.isDoDiff?"out : \"":"test: \"")+outDiffFileName+"\"\n";
    hpatch_printPath_utf8(fnameInfo.c_str());
    
    if (diffSets.isDoDiff) {
        const char* compressType="";
        if (compressPlugin) compressType=compressPlugin->compressType();
        printf("hdiffz run with compress plugin: \"%s\"\n",compressType);
        if (diffSets.isSingleCompressedDiff)
            printf("create single compressed diffData!\n");
    }
    
    int exitCode;
    if (diffSets.isDiffInMem)
        exitCode=hdiff_in_mem(oldFileName,newFileName,outDiffFileName,
                              compressPlugin,decompressPlugin,diffSets);
    else
        exitCode=hdiff_by_stream(oldFileName,newFileName,outDiffFileName,
                                 compressPlugin,decompressPlugin,diffSets);
    if (diffSets.isDoDiff && diffSets.isDoPatchCheck)
        printf("\nall   time: %.3f s\n",(clock_s()-time0));
    return exitCode;
}

int hdiff_resave(const char* diffFileName,const char* outDiffFileName,
                 const hdiff_TCompress* compressPlugin){
    double time0=clock_s();
    std::string fnameInfo=std::string("in_diff : \"")+diffFileName+"\"\n"
        +"out_diff: \""+outDiffFileName+"\"\n";
    hpatch_printPath_utf8(fnameInfo.c_str());
    
    int result=HDIFF_SUCCESS;
    hpatch_BOOL  _isInClear=hpatch_FALSE;
    hpatch_BOOL  isDirDiff=hpatch_FALSE;
    hpatch_BOOL  isSingleDiff=hpatch_FALSE;
    hpatch_compressedDiffInfo diffInfo;
    hpatch_singleCompressedDiffInfo singleDiffInfo;
    hpatch_TFileStreamInput  diffData_in;
    hpatch_TFileStreamOutput diffData_out;
    hpatch_TFileStreamInput_init(&diffData_in);
    hpatch_TFileStreamOutput_init(&diffData_out);
    
    hpatch_TDecompress* decompressPlugin=0;
    check(hpatch_TFileStreamInput_open(&diffData_in,diffFileName),HDIFF_OPENREAD_ERROR,"open diffFile");
    if (getSingleCompressedDiffInfo(&singleDiffInfo,&diffData_in.base,0)){
        isSingleDiff=hpatch_TRUE;
        diffInfo.newDataSize=singleDiffInfo.newDataSize;
        diffInfo.oldDataSize=singleDiffInfo.oldDataSize;
        diffInfo.compressedCount=(singleDiffInfo.compressedSize>0)?1:0;
        memcpy(diffInfo.compressType,singleDiffInfo.compressType,strlen(singleDiffInfo.compressType)+1);
        printf("  resave as single stream diffFile \n");
    }else if(getCompressedDiffInfo(&diffInfo,&diffData_in.base)){
        //ok
    }else{
        check(!diffData_in.fileError,HDIFF_RESAVE_FILEREAD_ERROR,"read diffFile");
        check(hpatch_FALSE,HDIFF_RESAVE_DIFFINFO_ERROR,"is hdiff file? get diff info");
    }
    {//decompressPlugin
        findDecompress(&decompressPlugin,diffInfo.compressType);
        if (decompressPlugin==0){
            if (diffInfo.compressedCount>0){
                check(false,HDIFF_RESAVE_COMPRESSTYPE_ERROR,
                      "can no decompress \""+diffInfo.compressType+" data");
            }else{
                if (strlen(diffInfo.compressType)>0)
                    printf("  diffFile added useless compress tag \"%s\"\n",diffInfo.compressType);
                decompressPlugin=0;
            }
        }else{
            printf("resave diffFile with decompress plugin: \"%s\" (need decompress %d)\n",diffInfo.compressType,diffInfo.compressedCount);
        }
    }
    {
        const char* compressType="";
        if (compressPlugin) compressType=compressPlugin->compressType();
        printf("resave diffFile with compress plugin: \"%s\"\n",compressType);
    }

    check(hpatch_TFileStreamOutput_open(&diffData_out,outDiffFileName,~(hpatch_StreamPos_t)0),HDIFF_OPENWRITE_ERROR,
          "open out diffFile");
    hpatch_TFileStreamOutput_setRandomOut(&diffData_out,hpatch_TRUE);
    printf("inDiffSize : %" PRIu64 "\n",diffData_in.base.streamSize);
    try{
        if (isSingleDiff)
            resave_single_compressed_diff(&diffData_in.base,decompressPlugin,
                                          &diffData_out.base,compressPlugin,&singleDiffInfo);
        else
            resave_compressed_diff(&diffData_in.base,decompressPlugin,
                                   &diffData_out.base,compressPlugin);
        diffData_out.base.streamSize=diffData_out.out_length;
    }catch(const std::exception& e){
        check(!diffData_in.fileError,HDIFF_RESAVE_FILEREAD_ERROR,"read diffFile");
        check(!diffData_out.fileError,HDIFF_RESAVE_OPENWRITE_ERROR,"write diffFile");
        check(false,HDIFF_RESAVE_ERROR,"resave diff run an error: "+e.what());
    }
    printf("outDiffSize: %" PRIu64 "\n",diffData_out.base.streamSize);
    check(hpatch_TFileStreamOutput_close(&diffData_out),HDIFF_FILECLOSE_ERROR,"out diffFile close");
    printf("  out diff file ok!\n");
    
    printf("\nhdiffz resave diffFile time: %.3f s\n",(clock_s()-time0));
clear:
    _isInClear=hpatch_TRUE;
    check(hpatch_TFileStreamOutput_close(&diffData_out),HDIFF_FILECLOSE_ERROR,"out diffFile close");
    check(hpatch_TFileStreamInput_close(&diffData_in),HDIFF_FILECLOSE_ERROR,"in diffFile close");
    return result;
}

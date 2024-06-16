#include "options.h"
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef pp_OSX
#include <unistd.h>
#ifdef pp_LUA
#include <sys/syslimits.h>
#endif
#endif
#include <math.h>
#ifdef WIN32
#ifdef __MINGW32__
#undef S_IFBLK
#undef S_ISBLK
#undef S_ISFIFO
#undef S_ISDIR
#undef S_ISCHR
#undef S_ISREG
#endif
#include <io.h>
#include <direct.h>
#include <dirent_win.h>
#else
#include <dirent.h>
#endif
#include "MALLOCC.h"
#include "string_util.h"
#include "file_util.h"
#include "threader.h"

FILE *alt_stdout=NULL;

/* ------------------ TestWrite ------------------------ */

void TestWrite(char *scratchdir, char **fileptr){
  char *file, filecopy[1024], newfile[1024], *beg;

  if(Writable(".")==1||fileptr==NULL||scratchdir==NULL)return;
  strcpy(filecopy, *fileptr);
  FREEMEMORY(*fileptr);

  strcpy(newfile, scratchdir);
  strcat(newfile, dirseparator);
  file = filecopy;
  beg = strrchr(filecopy, SEP);
  if(beg!=NULL)file = beg+1;
  strcat(newfile, file);
  NewMemory((void **)fileptr, strlen(newfile)+1);
  strcpy(*fileptr, newfile);
}

/* ------------------ FFLUSH ------------------------ */

int FFLUSH(void){
  int return_val=0;

  if(alt_stdout!=NULL){
    return_val = fflush(alt_stdout);
  }
  return return_val;
}

/* ------------------ PRINTF ------------------------ */

int PRINTF(const char * format, ...){
  va_list args;
  int return_val=0;

  if(alt_stdout!=NULL){
    va_start(args, format);
    return_val=vfprintf(alt_stdout, format, args);
    va_end(args);
  }
  return return_val;
}

/* ------------------ SetStdOut ------------------------ */

void SetStdOut(FILE *stream){
  alt_stdout=stream;
}

#define FILE_BUFFER 1000

/* ------------------ CopyFile ------------------------ */

void CopyFILE(char *destdir, char *file_in, char *file_out, int mode){
  char buffer[FILE_BUFFER];
  FILE *streamin=NULL, *streamout=NULL;
  char *full_file_out=NULL;
  size_t chars_in;

  if(destdir==NULL||file_in==NULL)return;
  streamin=fopen(file_in,"rb");
  if(streamin==NULL)return;

  full_file_out=NULL;
  NewMemory((void **)&full_file_out,strlen(file_out)+strlen(destdir)+1+1);
  strcpy(full_file_out,destdir);
  if(destdir[strlen(destdir)-1]!=*dirseparator){
    strcat(full_file_out,dirseparator);
  }
  strcat(full_file_out,file_out);

  if(mode==REPLACE_FILE){
    streamout=fopen(full_file_out,"wb");
  }
  else if(mode==APPEND_FILE){
    streamout=fopen(full_file_out,"ab");
  }
  else{
    assert(0);
  }

  if(streamout==NULL){
    FREEMEMORY(full_file_out);
    fclose(streamin);
    return;
  }
  PRINTF("  Copying %s to %s\n",file_in,file_out);
  for(;;){
    int end_of_file;

    end_of_file=0;
    chars_in=fread(buffer,1,FILE_BUFFER,streamin);
    if(chars_in!=FILE_BUFFER)end_of_file=1;
    if(chars_in>0)fwrite(buffer,chars_in,1,streamout);
    if(end_of_file==1)break;
  }
  FREEMEMORY(full_file_out);
  fclose(streamin);
  fclose(streamout);
}

/* ------------------ HaveProg ------------------------ */

int HaveProg(char *prog){
  if(system(prog) == 0)return 1;
  return 0;
}

/* ------------------ GetSmokeZipPath ------------------------ */

char *GetSmokeZipPath(char *progdir){
  char *zip_path;

  if(progdir!=NULL){
    NewMemory((void **)&zip_path,strlen(progdir)+20);
    strcpy(zip_path,progdir);
  }
  else{
    NewMemory((void **)&zip_path,2+20);
    strcpy(zip_path,".");
    strcat(zip_path,dirseparator);
  }

  strcat(zip_path,"smokezip");
#ifdef WIN32
  strcat(zip_path,".exe");
#endif
  if(FILE_EXISTS(zip_path)==YES)return zip_path;
  FREEMEMORY(zip_path);
  return NULL;
}

/* ------------------ SetDir ------------------------ */

char *SetDir(char *argdir){
  int lendir;
  char *dir;

  lendir=strlen(argdir);
  NewMemory((void **)&dir,lendir+2);
  strcpy(dir,argdir);
  if(dir[lendir-1]!=dirseparator[0]){
    strcat(dir,dirseparator);
  }
  return dir;
}

/* ------------------ GetBaseFileName ------------------------ */

char *GetBaseFileName(char *buffer,char *file){
  char *filebase,*ext;

  strcpy(buffer,file);
#ifdef WIN32
  filebase=strrchr(buffer,'\\');
#else
  filebase=strrchr(buffer,'/');
#endif
  if(filebase==NULL){
    filebase=buffer;
  }
  else{
    filebase++;
  }
  ext = strrchr(filebase,'.');
  if(ext!=NULL)*ext=0;
  return filebase;
}

/* ------------------ GetFileName ------------------------ */

char *GetFileName(char *temp_dir, char *file, int force_in_temp_dir){
  char *file2;
  char *file_out=NULL;
  FILE *stream=NULL;

  TrimBack(file);
  file2=TrimFront(file);
  if(force_in_temp_dir==NOT_FORCE_IN_DIR){
    stream=fopen(file2,"r");
    if(Writable(".")==YES||stream!=NULL){
      NewMemory((void **)&file_out,strlen(file2)+1);
      strcpy(file_out,file2);
    }
    if(stream!=NULL)fclose(stream);
  }
  if(file_out==NULL&&temp_dir!=NULL&&Writable(temp_dir)==YES){
    NewMemory((void **)&file_out,strlen(temp_dir)+1+strlen(file2)+1);
    strcpy(file_out, "");
    if(strcmp(temp_dir, ".")!=0){
      strcat(file_out, temp_dir);
      strcat(file_out, dirseparator);
    }
    strcat(file_out, file);
  }
  return file_out;
}

/* ------------------ FullFile ------------------------ */

void FullFile(char *file_out, char *dir, char *file){
  char *file2;

  TrimBack(file);
  file2=TrimFront(file);
  strcpy(file_out,"");
  if(dir!=NULL)strcat(file_out,dir);
  strcat(file_out,file2);
}

/* ------------------ StreamCopy ------------------------ */

unsigned int StreamCopy(FILE *stream_in, FILE *stream_out, int flag){
  int c;
  unsigned int nchars = 0;

  if(stream_in == NULL || stream_out == NULL)return 0;

  rewind(stream_in);
  c = fgetc(stream_in);
  while(c != EOF){
    if(flag == 0)return 1;
    fputc(c, stream_out);
    c = fgetc(stream_in);
    nchars++;
  }
  return nchars;
}

/* ------------------ FileErase ------------------------ */

void FileErase(char *file){
  if(FileExistsOrig(file) == 1){
    UNLINK(file);
  }
}

/* ------------------ FileCopy ------------------------ */

void FileCopy(char *file_in, char *file_out){
  FILE *stream_in, *stream_out;
  int c;

  if(file_in == NULL || file_out == NULL)return;
  stream_in = fopen(file_in, "rb");
  if(stream_in == NULL)return;
  stream_out = fopen(file_out, "wb");
  if(stream_out == NULL){
    fclose(stream_in);
    return;
  }

  c = fgetc(stream_in);
  while(c != EOF){
    fputc(c, stream_out);
    c = fgetc(stream_in);
  }
  fclose(stream_in);
  fclose(stream_out);
}

  /* ------------------ FileCat ------------------------ */

int FileCat(char *file_in1, char *file_in2, char *file_out){
  char buffer[FILE_BUFFER];
  FILE *stream_in1, *stream_in2, *stream_out;
  int chars_in;

  if(file_in1==NULL||file_in2==NULL)return -1;
  if(file_out==NULL)return -2;

  stream_in1=fopen(file_in1,"r");
  if(stream_in1==NULL)return -1;

  stream_in2=fopen(file_in2,"r");
  if(stream_in2==NULL){
    fclose(stream_in1);
    return -1;
  }

  stream_out=fopen(file_out,"w");
  if(stream_out==NULL){
    fclose(stream_in1);
    fclose(stream_in2);
    return -2;
  }

  for(;;){
    int end_of_file;

    end_of_file=0;
    chars_in=fread(buffer,1,FILE_BUFFER,stream_in1);
    if(chars_in!=FILE_BUFFER)end_of_file=1;
    if(chars_in>0)fwrite(buffer,chars_in,1,stream_out);
    if(end_of_file==1)break;
  }
  fclose(stream_in1);

  for(;;){
    int end_of_file;

    end_of_file=0;
    chars_in=fread(buffer,1,FILE_BUFFER,stream_in2);
    if(chars_in!=FILE_BUFFER)end_of_file=1;
    if(chars_in>0)fwrite(buffer,chars_in,1,stream_out);
    if(end_of_file==1)break;
  }
  fclose(stream_in2);
  fclose(stream_out);
  return 0;

}

/* ------------------ MakeOutFile ------------------------ */

void MakeOutFile(char *outfile, char *destdir, char *file1, char *ext){
  char filename_buffer[1024], *file1_noext;

  TrimBack(file1);
  strcpy(filename_buffer,TrimFront(file1));
  file1_noext=strstr(filename_buffer,ext);
  strcpy(outfile,"");
  if(file1_noext==NULL)return;
  file1_noext[0]='\0';
  if(destdir!=NULL){
    strcpy(outfile,destdir);
  }
  strcat(outfile,filename_buffer);
  strcat(outfile,"_diff");
  strcat(outfile,ext);
}


/* ------------------ Writable ------------------------ */

int Writable(char *dir){

  // returns 1 if the directory can be written to, 0 otherwise

  if(dir == NULL || strlen(dir) == 0)return NO;

#ifdef pp_LINUX
  if(ACCESS(dir,F_OK|W_OK)==-1){
    return NO;
  }
  else{
    return YES;
  }
#else
  {
    char tempfullfile[100], tempfile[40];
    FILE *stream;

    strcpy(tempfullfile,dir);
    strcat(tempfullfile,dirseparator);
    RandStr(tempfile,35);
    strcat(tempfullfile,tempfile);
    stream = fopen(tempfullfile,"w");
    if(stream==NULL){
      UNLINK(tempfullfile);
      return NO;
    }
    fclose(stream);
    UNLINK(tempfullfile);
    return YES;
  }
#endif
}

/* ------------------ IfFirstLineBlank ------------------------ */

int IfFirstLineBlank(char *file){

  // returns 1 if first line of file is blank

  STRUCTSTAT statbuff1;
  int statfile1;
  FILE *stream = NULL;
  char buffer[255], *buffptr;

  if(file==NULL)return 1;

  statfile1 = STAT(file, &statbuff1);
  if(statfile1!=0)return 1;

  stream = fopen(file, "r");
  if(stream==NULL||fgets(buffer, 255, stream)==NULL){
    if(stream!=NULL)fclose(stream);
    return 1;
  }
  fclose(stream);
  buffptr = TrimFrontBack(buffer);
  if(strlen(buffptr)==0)return 1;
  return 0;
}

/* ------------------ IsFileNewer ------------------------ */

int IsFileNewer(char *file1, char *file2){

// returns 1 if file1 is newer than file2, 0 otherwise

  STRUCTSTAT statbuff1, statbuff2;
  int statfile1, statfile2;

  if(file1==NULL||file2==NULL)return -1;

  statfile1=STAT(file1,&statbuff1);
  statfile2=STAT(file2,&statbuff2);
  if(statfile1!=0||statfile2!=0)return -1;

  if(statbuff1.st_mtime>statbuff2.st_mtime)return 1;
  return 0;
}

  /* ------------------ GetFileInfo ------------------------ */

int GetFileInfo(char *filename, char *source_dir, FILE_SIZE *filesize){
  STRUCTSTAT statbuffer;
  int statfile;
  char buffer[1024];

  if(source_dir==NULL){
    strcpy(buffer,filename);
  }
  else{
    strcpy(buffer,source_dir);
    strcat(buffer,filename);
  }
  if(filesize!=NULL)*filesize=0;
  statfile=STAT(buffer,&statbuffer);
  if(statfile!=0)return statfile;
  if(filesize!=NULL)*filesize=statbuffer.st_size;
  return statfile;
}

/* ------------------ GetFileSizeSMV ------------------------ */

FILE_SIZE GetFileSizeSMV(const char *filename){
  STRUCTSTAT statbuffer;
  int statfile;
  FILE_SIZE return_val;

  return_val=0;
  if(filename==NULL)return return_val;
  statfile=STAT(filename,&statbuffer);
  if(statfile!=0)return return_val;
  return_val = statbuffer.st_size;
  return return_val;
}

/* ------------------ fread_mt ------------------------ */

void *fread_mt(void *mtfileinfo){
  FILE_SIZE first, last, length, file_size;
  FILE *stream;
  int i, nthreads;
  char *file, *buffer;
  mtfiledata *mtf;

  mtf = (mtfiledata *)mtfileinfo;

  i         = mtf->i;
  nthreads  = mtf->nthreads;
  file      = mtf->file;
  buffer    = mtf->buffer;
  file_size = mtf->file_size;
  
  first = i*file_size/nthreads;
  last  = first + file_size/nthreads - 1;
  if(last > file_size - 1)last = file_size - 1;
  length = last + 1 - first;
  stream = fopen(file, "rb");
  FSEEK(stream, first, SEEK_SET);
  mtf->chars_read = fread(buffer + first, 1, length, stream);
  fclose(stream);

#ifdef pp_THREAD
  if(nthreads>1)pthread_exit(NULL);
#endif
  return NULL;
}

/* ------------------ SetMtFileInfo ------------------------ */

mtfiledata *SetMtFileInfo(char *file, char *buffer, int nthreads){
  mtfiledata *mtfileinfo;
  int i;
  FILE_SIZE file_size;

  NewMemory((void **)&mtfileinfo,nthreads*sizeof(mtfiledata));
  file_size = GetFileSizeSMV(file);

  for(i=0;i<nthreads;i++){
    mtfiledata *mti;

    mti = mtfileinfo + i;
    mti->i          = i;
    mti->nthreads   = nthreads;
    mti->file       = file;
    mti->buffer     = buffer;
    mti->file_size  = file_size;
    mti->chars_read = 0;
  }
  return mtfileinfo;
}
    //chars_in=fread(buffer,1,FILE_BUFFER,stream_in1);

/* ------------------ fread_p ------------------------ */

FILE_SIZE fread_p(char *file, char *buffer, int nthreads){
  FILE_SIZE chars_read;
  mtfiledata *mtfileinfo;

  mtfileinfo = SetMtFileInfo(file, buffer, nthreads);
  if(nthreads == 1){
    FILE *stream;

    stream = fopen(file, "rb");
    if(stream == NULL)return 0;
    chars_read = fread(buffer, 1, mtfileinfo->file_size, stream);
    fclose(stream);
  }
#ifdef pp_THREAD
  else{
    threaderdata *read_threads;
    int use_read_threads, i;

    use_read_threads = 1;
    read_threads = THREADinit(&nthreads, &use_read_threads, fread_mt);
    THREADrun(read_threads, &mtfileinfo);
    THREADcontrol(read_threads, THREAD_JOIN);
    chars_read = 0;
    for(i = 0;i < nthreads;i++){
      chars_read += mtfileinfo[i].chars_read;
    }
  }
#else
  else{
    int i;
    
    chars_read = 0;
    for(i = 0;i < nthreads;i++){
      mtfiledata *mti;

      mti = mtfileinfo + i;
      fread_mt(mti);
      chars_read += mti->chars_read;
    }
  }
#endif
  return chars_read;
}

/* ------------------ THREADreadi ------------------------ */

void THREADreadi(threaderdata *thi, mtfiledata *mtfileinfo){
#ifdef pp_THREAD
  if(thi == NULL)return;
  if(thi->use_threads_ptr != NULL)thi->use_threads = *(thi->use_threads_ptr);
  if(thi->n_threads_ptr != NULL){
    thi->n_threads = *(thi->n_threads_ptr);
    if(thi->n_threads > MAX_THREADS)thi->n_threads = MAX_THREADS;
  }
  int i;

  for(i = 0; i < thi->n_threads; i++){
    mtfiledata *mti;

    mti = mtfileinfo + i;
    if(thi->use_threads == 1){
      pthread_create(thi->thread_ids + i, NULL, thi->run, (void *)mti);
    }
    else{
      thi->run((void *)mti);
    }
  }
#else
//  args[0] = 1;
//  args[1] = -1;
//  thi->run(args);
#endif
}


/* ------------------ FileExistsOrig ------------------------ */

int FileExistsOrig(char *filename){
  if(ACCESS(filename, F_OK) == -1){
    return NO;
  }
  else{
    return YES;
  }
}

  /* ------------------ FileExists ------------------------ */

int FileExists(char *filename, filelistdata *filelist, int nfilelist, filelistdata *filelist2, int nfilelist2){

// returns YES if the file filename exists, NO otherwise

  if(filename == NULL)return NO;
  if(filelist != NULL&&nfilelist>0){
    if(FileInList(filename, filelist, nfilelist, filelist2, nfilelist2) != NULL){
      return YES;
    }
  }
  if(ACCESS(filename,F_OK)==-1){
    return NO;
  }
  return YES;
}

/* ------------------ FreeFileList ------------------------ */

void FreeFileList(filelistdata *filelist, int *nfilelist){
  int i;

  for(i=0;i<*nfilelist;i++){
    FREEMEMORY(filelist[i].file);
  }
  FREEMEMORY(filelist);
  *nfilelist=0;
}

/* ------------------ GetFileListSize ------------------------ */

int GetFileListSize(const char *path, char *filter, int mode){
  struct dirent *entry;
  DIR *dp;
  int maxfiles=0;
  int d_type;

  if(path == NULL||filter==NULL)return maxfiles;
  dp = opendir(path);
  if(dp == NULL)return 0;
  d_type = DT_REG;
  if(mode==DIR_MODE)d_type = DT_DIR;
  while( (entry = readdir(dp))!=NULL ){
    if(((entry->d_type==d_type||entry->d_type==DT_UNKNOWN)&&MatchWild(entry->d_name,filter)==1)){
      if(strcmp(entry->d_name,".")==0||strcmp(entry->d_name,"..")==0)continue;
      maxfiles++;
    }
  }
  closedir(dp);
  return maxfiles;
}

/* ------------------ fopen_indir  ------------------------ */

FILE *fopen_indir(char *dir, char *file, char *mode){
  FILE *stream;

  if(file==NULL||strlen(file)==0)return NULL;
  if(dir==NULL||strlen(dir)==0){
#ifdef WIN32
    stream = _fsopen(file, mode, _SH_DENYNO);
#else
    stream = fopen(file,mode);
#endif
  }
  else{
    char *filebuffer;
    int lenfile;

    lenfile = strlen(dir)+1+strlen(file)+1;
    NewMemory((void **)&filebuffer,lenfile*sizeof(char));
    strcpy(filebuffer,dir);
    strcat(filebuffer,dirseparator);
    strcat(filebuffer,file);
#ifdef WIN32
    stream = _fsopen(filebuffer, mode, _SH_DENYNO);
#else
    stream = fopen(filebuffer, mode);
#endif
    FREEMEMORY(filebuffer);
  }
  return stream;
}

/* ------------------ fopen_2dir ------------------------ */

FILE *fopen_2dir(char *file, char *mode, char *scratch_dir){
  FILE *stream;

  if(file == NULL)return NULL;
#ifdef WIN32
  stream = _fsopen(file,mode,_SH_DENYNO);
#else
  stream = fopen(file,mode);
#endif
  if(stream == NULL && scratch_dir != NULL){
    stream = fopen_indir(scratch_dir, file, mode);
  }
  return stream;
}


/* ------------------ CompareFileList ------------------------ */

int CompareFileList(const void *arg1, const void *arg2){
  filelistdata *x, *y;

  x = (filelistdata *)arg1;
  y = (filelistdata *)arg2;

  return strcmp(x->file, y->file);
}

/* ------------------ FileInList ------------------------ */

filelistdata *FileInList(char *file, filelistdata *filelist, int nfiles, filelistdata *filelist2, int nfiles2){
  filelistdata *entry=NULL, fileitem;

  if(file==NULL)return NULL;
  fileitem.file = file;
  fileitem.type = 0;
  if(filelist!=NULL&&nfiles>0){
    entry = bsearch(&fileitem, (filelistdata *)filelist, (size_t)nfiles, sizeof(filelistdata), CompareFileList);
    if(entry!=NULL)return entry;
  }
  if(filelist2!=NULL&&nfiles2>0){
    entry = bsearch(&fileitem, (filelistdata *)filelist2, (size_t)nfiles2, sizeof(filelistdata), CompareFileList);
  }
  return entry;
}

/* ------------------ MakeFileList ------------------------ */

int MakeFileList(const char *path, char *filter, int maxfiles, int sort_files, filelistdata **filelist, int mode){
  struct dirent *entry;
  DIR *dp;
  int nfiles=0;
  filelistdata *flist;
  int d_type;

  // DT_DIR - is a directory
  // DT_REG - is a regular file

  if (maxfiles == 0||path==NULL||filter==NULL) {
    *filelist = NULL;
    return 0;
  }
  dp = opendir(path);
  *filelist=NULL;
  if(dp == NULL)return 0;
  NewMemory((void **)&flist,maxfiles*sizeof(filelistdata));
  d_type = DT_REG;
  if(mode==DIR_MODE)d_type = DT_DIR;
  while( (entry = readdir(dp))!=NULL&&nfiles<maxfiles ){
    if((entry->d_type==d_type||entry->d_type==DT_UNKNOWN)&&MatchWild(entry->d_name,filter)==1){
      char *file;
      filelistdata *flisti;

      if(strcmp(entry->d_name,".")==0||strcmp(entry->d_name,"..")==0)continue;
      flisti = flist + nfiles;
      if(mode == DIR_MODE){
        NewMemory((void **)&file, strlen(path)+1+strlen(entry->d_name) + 1);
      }
      else{
        NewMemory((void **)&file, strlen(entry->d_name) + 1);
      }
      strcpy(file, "");
      if(mode == DIR_MODE){
        strcat(file, path);
        strcat(file, dirseparator);
      }
      strcat(file,entry->d_name);
      flisti->file=file;
      flisti->type=0;
      nfiles++;
    }
  }
  closedir(dp);
  if(sort_files == YES&&nfiles>0){
    qsort((filelistdata *)flist, (size_t)nfiles, sizeof(filelistdata), CompareFileList);
  }
  *filelist=flist;
  return nfiles;
}

/* ------------------ GetFileSizeLabel ------------------------ */

void GetFileSizeLabel(int size, char *sizelabel){
  int leftsize,rightsize;

#define sizeGB   1000000000
#define size100MB 100000000
#define size10MB   10000000
#define sizeMB     1000000
#define size100KB    100000
#define size10KB      10000

  if(size>=sizeGB){
    size/=size10MB;
    leftsize=size/100;
    rightsize=size-100*leftsize;
    sprintf(sizelabel,"%i.%02i GB",leftsize,rightsize);
  }
  else if(size>=size100MB&&size<sizeGB){
    size/=sizeMB;
    leftsize=size;
    sprintf(sizelabel,"%i MB",leftsize);
  }
  else if(size>=size10MB&&size<size100MB){
    size/=size100KB;
    leftsize=size/10;
    rightsize=size-10*leftsize;
    sprintf(sizelabel,"%i.%i MB",leftsize,rightsize);
  }
  else if(size>=sizeMB&&size<size10MB){
    size/=size10KB;
    leftsize=size/100;
    rightsize=size-100*leftsize;
    sprintf(sizelabel,"%i.%02i MB",leftsize,rightsize);
  }
  else if(size>=size100KB&&size<sizeMB){
    size/=1000;
    leftsize=size;
    sprintf(sizelabel,"%i KB",leftsize);
  }
  else{
    size/=10;
    leftsize=size/100;
    rightsize=size-100*leftsize;
    sprintf(sizelabel,"%i.%02i KB",leftsize,rightsize);
  }
}

/* ------------------ GetFloatFileSizeLabel ------------------------ */

char *GetFloatFileSizeLabel(float size, char *sizelabel){
  int leftsize, rightsize;

  if(size>=sizeGB){
    size /= size10MB;
    leftsize = size/100;
    rightsize = size-100*leftsize;
    sprintf(sizelabel, "%i.%02i GB", leftsize, rightsize);
  }
  else if(size>=size100MB&&size<sizeGB){
    size /= sizeMB;
    leftsize = size;
    sprintf(sizelabel, "%i MB", leftsize);
  }
  else if(size>=size10MB&&size<size100MB){
    size /= size100KB;
    leftsize = size/10;
    rightsize = size-10*leftsize;
    sprintf(sizelabel, "%i.%i MB", leftsize, rightsize);
  }
  else if(size>=sizeMB&&size<size10MB){
    size /= size10KB;
    leftsize = size/100;
    rightsize = size-100*leftsize;
    sprintf(sizelabel, "%i.%02i MB", leftsize, rightsize);
  }
  else if(size>=size100KB&&size<sizeMB){
    size /= 1000;
    leftsize = size;
    sprintf(sizelabel, "%i KB", leftsize);
  }
  else{
    size /= 10;
    leftsize = size/100;
    rightsize = size-100*leftsize;
    sprintf(sizelabel, "%i.%02i KB", leftsize, rightsize);
  }
  return sizelabel;
}

/* ------------------ GetProgDir ------------------------ */

char *GetProgDir(char *progname, char **svpath){

// returns the directory containing the file progname

  char *progpath, *lastsep, *smokeviewpath2;

  lastsep=strrchr(progname,dirseparator[0]);
  if(lastsep==NULL){
    char *dir;

    dir = Which(progname, NULL);
    if(dir==NULL){
      NewMemory((void **)&progpath,(unsigned int)3);
      strcpy(progpath,".");
      strcat(progpath,dirseparator);
    }
    else{
      int lendir;

      lendir=strlen(dir);
      NewMemory((void **)&progpath,(unsigned int)(lendir+2));
      strcpy(progpath,dir);
      if(progpath[lendir-1]!=dirseparator[0])strcat(progpath,dirseparator);
    }
    NewMemory((void **)&smokeviewpath2,(unsigned int)(strlen(progpath)+strlen(progname)+1));
    strcpy(smokeviewpath2,progpath);
  }
  else{
    int lendir;

    lendir=lastsep-progname+1;
    NewMemory((void **)&progpath,(unsigned int)(lendir+1));
    strncpy(progpath,progname,lendir);
    progpath[lendir]=0;
    NewMemory((void **)&smokeviewpath2,(unsigned int)(strlen(progname)+1));
    strcpy(smokeviewpath2,"");
  }
  strcat(smokeviewpath2,progname);
  *svpath=smokeviewpath2;
  return progpath;
}

/* ------------------ IsSootFile ------------------------ */

int IsSootFile(char *shortlabel, char *longlabel){
  if(STRCMP(shortlabel, "rho_C")==0)return 1;
  if(STRCMP(shortlabel, "rho_Soot")==0)return 1;
  if(STRCMP(shortlabel, "rho_C0.9H0.1")==0)return 1;
  if(strlen(longlabel)>=12&&strncmp(longlabel, "SOOT DENSITY",12)==0)return 1;
  return 0;
}

/* ------------------ getprogdirabs ------------------------ */

#ifdef pp_LUA
char *getprogdirabs(char *progname, char **svpath){

// returns the absolute path of the directory containing the file progname
  char *progpath;
#ifdef WIN32
  NewMemory((void **)&progpath,_MAX_PATH);
  _fullpath(progpath,GetProgDir(progname,svpath),_MAX_PATH);
#else
  NewMemory((void **)&progpath,PATH_MAX);
  realpath(GetProgDir(progname,svpath),progpath);
#endif
  return progpath;
}
#endif

/* ------------------ LastName ------------------------ */

char *LastName(char *argi){

// returns the file name contained in the full path name argi

  char *lastdirsep;
  char *dir, *filename, cwdpath[1000];

  filename=argi;
  lastdirsep=strrchr(argi,SEP);
  if(lastdirsep!=NULL){
    dir=argi;
    filename=lastdirsep+1;
    lastdirsep[0]=0;
    GETCWD(cwdpath,1000);
    if(strcmp(cwdpath,dir)!=0){
      CHDIR(dir);
    }
  }
  return filename;
}

/* ------------------ GetZoneFileName ------------------------ */

char *GetZoneFileName(char *bufptr){
  char *full_name, *last_name, *filename;

  full_name=bufptr;
  if(FILE_EXISTS(full_name)==NO)full_name=NULL;

  last_name= LastName(bufptr);
  if(FILE_EXISTS(last_name)==NO)last_name=NULL;

  if(last_name!=NULL&&full_name!=NULL){
    if(strcmp(last_name,full_name)==0){
      last_name=NULL;
    }
  }

  if(last_name!=NULL&&full_name!=NULL){
    filename=last_name;
  }
  else if(last_name==NULL&&full_name!=NULL){
    filename=full_name;
  }
  else if(last_name!=NULL&&full_name==NULL){
    filename=last_name;
  }
  else{
    filename=NULL;
  }
  return filename;
}

/* ------------------ FileModtime ------------------------ */

time_t FileModtime(char *filename){

// returns the modification time of the file named filename

  STRUCTSTAT statbuffer;
  time_t return_val;
  int statfile;

  return_val=0;
  if(filename==NULL)return return_val;
  statfile=STAT(filename,&statbuffer);
  if(statfile!=0)return return_val;
  return_val = statbuffer.st_mtime;
  return return_val;
}

/* ------------------ GetProgFullPath ------------------------ */

void GetProgFullPath(char *progexe, int maxlen_progexe){
  char *end, savedir[1024], tempdir[1024], *tempexe;

  strcpy(tempdir, progexe);
  end = strrchr(tempdir, dirseparator[0]);
  if(end == NULL){
    char *progpath;

    progpath = Which(progexe, NULL);
    if(progpath != NULL){
      char copy[1024];

      strcpy(copy, progexe);
      strcpy(progexe, progpath);
      if(progexe[strlen(progexe) - 1] != dirseparator[0])strcat(progexe, dirseparator);
      strcat(progexe, copy);
    }
  }
  else{
    end[0] = 0;
    tempexe = end + 1;
    GETCWD(savedir, 1024);
    CHDIR(tempdir);
    GETCWD(progexe, maxlen_progexe);
    if(progexe[strlen(progexe) - 1] != dirseparator[0])strcat(progexe, dirseparator);
    strcat(progexe, tempexe);
    CHDIR(savedir);
  }
}

/* ------------------ Which ------------------------ */

char *Which(char *progname, char **fullprognameptr){

// returns the PATH directory containing the file progname

  char *pathlist, *pathlistcopy, *fullprogname, *prognamecopy;
  char *dir,*pathentry;
  char pathsep[2], dirsep[2];

#ifdef WIN32
  strcpy(pathsep,";");
  strcpy(dirsep,"\\");
#else
  strcpy(pathsep,":");
  strcpy(dirsep,"/");
#endif

  pathlist = getenv("PATH");
  if(pathlist==NULL||strlen(pathlist)==0||progname==NULL||strlen(progname)==0)return NULL;

  NewMemory((void **)&prognamecopy, (unsigned int)(strlen(progname)+4+1));
  strcpy(prognamecopy, progname);

  NewMemory((void **)&pathlistcopy, (unsigned int)(strlen(pathlist)+1));
  strcpy(pathlistcopy, pathlist);

#ifdef WIN32
  {
    const char *ext;

    ext = prognamecopy+strlen(progname)-4;
    if(strlen(progname)<=4|| (STRCMP(ext,".exe")!=0 && STRCMP(ext, ".bat") != 0))strcat(prognamecopy, ".exe");
  }
#endif

  NewMemory((void **)&fullprogname, (unsigned int)(strlen(progname)+4+strlen(dirsep)+strlen(pathlist)+1));

  dir=strtok(pathlistcopy,pathsep);
  while(dir!=NULL&&strlen(dir)>0){
    strcpy(fullprogname,dir);
    strcat(fullprogname,dirsep);
    strcat(fullprogname,prognamecopy);
    if(FILE_EXISTS(fullprogname)==YES){
      NewMemory((void **)&pathentry,(unsigned int)(strlen(dir)+2));
      strcpy(pathentry,dir);
      strcat(pathentry,dirsep);
      FREEMEMORY(pathlistcopy);
      if(fullprognameptr != NULL){
        *fullprognameptr = fullprogname;
      }
      else{
        FREEMEMORY(fullprogname);
      }
      FREEMEMORY(prognamecopy);
      return pathentry;
    }
    dir=strtok(NULL,pathsep);
  }
  FREEMEMORY(pathlistcopy);
  FREEMEMORY(fullprogname);
  FREEMEMORY(prognamecopy);
  return NULL;
}

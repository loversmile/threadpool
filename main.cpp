/*
 * main.cpp
 *
 *  Created on: 2013-3-13
 *  Author: Sai
 */

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

using namespace std;

#include "dh_thread_pool.h"
#include "dh_thread_task.h"

#include "dh_download_manager.h"

typedef struct DiskData
{
  FILE* file;
}DiskData;

size_t WriteToDisk(void *contents, size_t size, size_t nmemb,
    void *userp)
{
  DiskData* dd = (DiskData*) userp;
  size_t number = nmemb*size;
  size_t writed_num = fwrite(contents,1,number,dd->file);
  return writed_num;
}

void *RunTaskFunc(void * arg)
{
  int* i = (int*) arg;
/*  cout << "thread index: " << *i << endl;
  DhDownloadManager* manager = new DhDownloadManager();
  static const char* url =
      "http://www.istonsoft.com/downloads/iston-video-converter.exe";

  DiskData dd;
  char path[8];
  memset(path,0,sizeof(path));
  sprintf(path,"%d.exe",*i);
  dd.file = fopen(path,"wb");
  manager->Process(url,&WriteToDisk,&dd);

  fclose(dd.file);*/
  printf("HaHa i = %d\n\n", *i);
  return NULL;
}

void *TestTaskFunc(void *arg)
{
    int *i = (int *) arg;
    cout << "HTIH" << endl;
    printf("i = %d\n", *i);
    return NULL;
}

int main(int argc, char *argv[])
{
  setbuf(stdout, (char*) NULL);
  setbuf(stderr, (char*) NULL);
  printf("Hello, world\n");
  DhThreadPool *pool = new DhThreadPool(5);
  pool->Activate();
int o = 0;
  for (o = 0; o < 10; ++o)
  {
    int *i = new int;
    *i = o;
    pool->AddAsynTask(&RunTaskFunc, i);
  }
  sleep(10);
  
  for (o = 10; o < 20; ++o)
  {
    int *i = new int;
    *i = o;
    pool->AddAsynTask(&TestTaskFunc, i);
  }

  while(1)
  {
    sleep(2);
  }

  pool->Destroy();
  delete pool;

  return 0;
}
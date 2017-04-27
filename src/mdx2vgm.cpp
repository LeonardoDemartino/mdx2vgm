#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mxdrvg.h"
#define MXDRVG_EXPORT
#define MXDRVG_CALLBACK

#include "mxdrvg_core.h"
#include "stream.h"

#define VERSION "0.2"

const int MAGIC_OFFSET = 10;

size_t WriteStream::write(Buffer &b)
{
    return write(b.data, b.len);
}

bool read_file(const char *name, int *fsize, unsigned char **fdata, int offset)
{
    FILE* fd = fopen(name, "rb");
    if (fd == NULL)
    {
        printf("cannot open %s\n", name);
        return false;
    }

    //count
    int file_size = 0;
    char c=0;
    fseek(fd, 0, SEEK_SET);
    while(!feof(fd)){
        fread(&c, 1, 1, fd);
        if(!feof(fd))
            file_size++;
    }

    //read
    unsigned char *data = new unsigned char[file_size + offset];
    int i=0;
    fseek(fd, 0, SEEK_SET);
    while(!feof(fd)){
        fread(&data[i+offset], 1, 1, fd);
        i++;
    }

    fclose(fd);

    *fdata = data;
    *fsize = file_size+offset;
    return true;
}

bool LoadMDX(const char *mdx_name, char *title, int title_len)
{
    unsigned char *mdx_buf = 0, *pdx_buf = 0;
    int mdx_size = 0, pdx_size = 0;

    int MDX_BUF_SIZE = 256 * 1024;
    int PDX_BUF_SIZE = 1024 * 1024;

    // Load MDX file
    if (!read_file(mdx_name, &mdx_size, &mdx_buf, MAGIC_OFFSET))
    {
        printf("Cannot open/read %s.\n", mdx_name);
        return false;
    }

    // Skip title.
    int pos = MAGIC_OFFSET;
    {
        char *ptitle = title;
        while (pos < mdx_size && --title_len > 0)
        {
            *ptitle++ = mdx_buf[pos];
            if (mdx_buf[pos] == 0x0d && mdx_buf[pos + 1] == 0x0a)
                break;
            pos++;
        }
        *ptitle = 0;
    }

    while (pos < mdx_size)
    {
        unsigned char c = mdx_buf[pos++];
        if (c == 0x1a) break;
    }

    char pdx_path[FILENAME_MAX] = {'\0'};
    int i = 0;
    while (true)
    {
        if (mdx_buf[pos] == 0) break;
        pdx_path[i] = mdx_buf[pos++];
        i++;
    }
    pos++;

    int mdx_body_pos = pos;
    if(strlen(pdx_path)>1){
      if(!(strlen(pdx_path)>4 && pdx_path[strlen(pdx_path)-4]=='.'))
        strcat(pdx_path, ".PDX");
      printf("PDX FILE: %s\n", pdx_path);

      if (!read_file(pdx_path, &pdx_size, &pdx_buf, MAGIC_OFFSET))
      {
          printf("Error while reading .pdx file: %s.\n", pdx_path);
          return false;
      }

      printf("mdx body pos  :0x%x\n", mdx_body_pos - MAGIC_OFFSET);
      printf("mdx body size :0x%x\n", mdx_size - mdx_body_pos - MAGIC_OFFSET);
    }
    MXDRVG_Start(MDX_BUF_SIZE, PDX_BUF_SIZE, pdx_path);

    unsigned char *mdx_head = mdx_buf + mdx_body_pos - MAGIC_OFFSET;
    
    for(int i=0; i<10; i++){
      if (pdx_buf)pdx_buf[i]=0;
      mdx_head[i]=0;
    }

    mdx_head[2] = (pdx_buf ? 0 : 0xff);
    mdx_head[3] = (pdx_buf ? 0 : 0xff);
    mdx_head[5] = 0x0a;
    mdx_head[7] = 0x08;

    if(pdx_buf){
      pdx_buf[5] = 0x0a;
      pdx_buf[7] = 0x02;
    }

    printf("Instrument pos: 0x%x\n", mdx_body_pos - 10 + (mdx_head[10] << 8) + mdx_head[11]);

    MXDRVG_SetData(mdx_head, mdx_size, pdx_buf, pdx_size);

    delete []mdx_buf;
    delete []pdx_buf;

    return true;
}

void intro(){
    printf(
        "mdx2vgm version "VERSION"\n"
        "Copyright 2017 delek.net/deflemask.com\n"
        " based on MDXDRVg V1.50a (C) 2000 GORRY.\n"
        "  converted from X68k MXDRV music driver version 2.06+17 Rel.X5-S\n"
        "   (c)1988-92 milk.,K.MAEKAWA, Missy.M, Yatsube\n\n");
}

void tutorial(){
  printf("Usage: mdx2vgm <file>\n");
}

void help()
{
    intro();
    tutorial();
}

int main(int argc, char **argv)
{
    if(argc>2){
        help();
        return 0;
    }

    const char *mdx_name = argv[optind];
    if (mdx_name == 0 || *mdx_name == 0)
    {
        help();
        return 0;
    }
    intro();

    char title[FILENAME_MAX];

    if (!LoadMDX(mdx_name, title, sizeof(title)))
    {
        printf("Error on LoadMDX function!\n");
        tutorial();
        return -1;
    }

    while (true)
    {
        if (MXDRVG_GetTerminated())break;
        MXDRVG_Tick();
    }

    char vgm_name[FILENAME_MAX] = {'\0'};
    strcat(vgm_name, mdx_name);
    if(strlen(vgm_name)>4 && vgm_name[strlen(vgm_name)-4]=='.')
    {
        vgm_name[strlen(vgm_name)-4] = '\0';
    }
    strcat(vgm_name, ".vgm");
    MXDRVG_End(vgm_name);

    printf("Title: %s\n", title);
    printf("Processing to: %s\n", vgm_name);
    printf("Completed.\n");

    return 0;
}

/*  @(#)lunkwill.c, Dirk Haun, 16.11.1997
 *
 *  23.12.1996  Version 1.13 released
 *  16.11.1997  letzte Žnderungen:
 *              Abh„ngigkeiten von meiner eigenen Lib entfernt
 */

#define HEXVERSION 0x115
#define ASCVERSION  "1.15"

#include <stdio.h>
#include <stdlib.h>
#include <tos.h>
#include <string.h>
#include <ctype.h>
#include "lib\lib.h"

#ifndef FALSE
# define FALSE (0)
# define TRUE  (1)
#endif

#define fexists(a) (Fattrib(a,0,0)<0 ? FALSE : TRUE)

typedef struct _faq
{
 char *gruppe,
      *name,
      *von,
      *betreff1,
      *betreff2,
      *datei,
      *info,
      *key;
 struct _faq *next;
 int append;
} FAQ;

#define E_ARGS      1
#define E_MEM       2
#define E_OUTNF     3
#define E_INFNF     4
#define E_INFOPEN   5
#define E_INFEMPTY  6
#define E_INFREAD   7
#define E_ILLKEY    8
#define E_EMPTYKEY  9
#define E_NOTALL   10
#define E_OUTOPEN  11
#define E_OUTREAD  12
#define E_FAQOPEN  13

const char *const c_error[] =
{
 "Nicht genug Parameter.",
 "Nicht genug Speicher.",
 "OUTFILE.TXT nicht gefunden.",
 "LUNKWILL.INF nicht gefunden.",
 "Konnte LUNKWILL.INF nicht ”ffnen.",
 "LUNKWILL.INF enth„lt keine Infos.",
 "Lesefehler in LUNKWILL.INF.",
 "Unbekanntes Schlsselwort.",
 "Schlsselwort ohne Parameter.",
 "Unvollst„ndige Angaben zur FAQ.",
 "Konnte OUTFILE.TXT nicht ”ffnen.",
 "Lesefehler in OUTFILE.TXT.",
 "Konnte die FAQ-Ausgabedatei nicht ”ffnen."
};

#define KEY_GRUPPE  0
#define KEY_NAME    1
#define KEY_VON     2
#define KEY_BETREFF 3
#define KEY_DATEI   4
#define KEY_INFO    5
#define KEY_KEY     6

const char *const keyword[] =
{
 "Gruppe:", "Name:", "Von:", "Betreff:", "Datei:", "Info:", "Keyword:",
 0L
};

#define INF_FILE  "lunkwill.inf"
#define INF_EXT   ".inf"
#define OUTFILE   "outfile.txt"
#define OUTFILE_X "outfile."

char outfile[128], inf_file[128], log_file[128];
static char logbuffer[1024];
static int logfh=0, errcode=0, errline=0, blockstart=0, zeile=0, faqs=0;
static int testmode=FALSE, nolog=FALSE;
FAQ *faq;

static char *get_line(char *to,char *from,char *endadr,int maxlen)
{
 maxlen--;
 while(*from!='\r' && *from!='\n' && from<endadr)
 {
  if(*from=='#') break;
  if(*from==' ' || *from=='\t') from++;
  else
  {
   while(*from!='\r' && *from!='\n' && *from!='#' && from<endadr && maxlen--)
     *to++=*from++;
   break;
  }
 }
 while(from<endadr && (*from!='\r' && *from!='\n')) from++;
 if(from>=endadr) zeile++;
 else while(from<endadr && (*from=='\r' || *from=='\n'))
 {
  zeile++;
  if(*from=='\r' && *(from+1)=='\n') from+=2;
  else from++;
 }
 *to++='\0';
 return(from);
}

static void same_path(char *path,const char *file)
{
 char *cp;

 cp=strrchr(path,'/');
 if(cp==0L) cp=strrchr(path,'\\');
 if(cp==0L) strcpy(path,file);
 else strcpy(cp+1,file);
}

void open_logfile(void)
{
 strcpy(log_file,inf_file);
 same_path(log_file,"lunkwill.log");
 logfh=(int)Fcreate(log_file,0);
 if(logfh>0)
 {
  strcpy(logbuffer,"Lunkwill V"ASCVERSION" ("__DATE__")\r\nLogfile vom ");
  gdtoa(Tgetdate(),logbuffer+strlen(logbuffer));
  strcat(logbuffer,"\r\n\r\n");
  gttoa(Tgettime(),logbuffer+strlen(logbuffer));
  strcat(logbuffer," Programmstart\r\n");
  Fwrite(logfh,strlen(logbuffer),logbuffer);
 }
}

void close_logfile(int err)
{
 char *cp, x[10];

 if(logfh>0)
 {
  logbuffer[0]='\0';
  if(err>0)
  {
   gttoa(Tgettime(),logbuffer);
   strcat(logbuffer," ***** Fehler");
   if(errcode!=0)
   {
    x[0]=' ';
    itoa(errcode,&x[1],10);
    strcat(logbuffer,x);
   }
   else if(errline!=0)
   {
    if(err==E_NOTALL)
    {
     strcat(logbuffer," im Block ab Zeile ");
     itoa(blockstart,logbuffer+strlen(logbuffer),10);
    }
    else
    {
     strcat(logbuffer," in Zeile ");
     itoa(errline,logbuffer+strlen(logbuffer),10);
    }
   }
   strcat(logbuffer,": ");
   strcat(logbuffer,c_error[err-1]);
   strcat(logbuffer," *****\r\n");
   cp=strchr(logbuffer,'|');
   if(cp) *cp=' ';
  }
  gttoa(Tgettime(),logbuffer+strlen(logbuffer));
  strcat(logbuffer," Programmende\r\n");
  if(err==0 && faqs==0)
  {
   if(testmode) strcat(logbuffer,"\r\nKeine Fehler aufgetreten.\r\n");
   else strcat(logbuffer,"\r\nKeine FAQs gefunden.\r\n");
  }
  Fwrite(logfh,strlen(logbuffer),logbuffer);
  Fclose(logfh);
  logfh=0;
  if(nolog && err==0 && faqs==0) Fdelete(log_file);
 }
}

void free_faq(void)
{
 FAQ *f, *fn;

 f=faq;
 while(f)
 {
  if(f->von) free(f->von);
  if(f->name) free(f->name);
  free(f->gruppe);
  free(f->betreff1);
  if(f->betreff2) free(f->betreff2);
  free(f->datei);
  fn=f->next;
  free(f);
  f=fn;
 }
}

int add_faq(FAQ *f)
{
 int ret=0;
 FAQ *fn, *fr;

 if(f->von || f->name)
 {
  if(f->gruppe && f->betreff1 && f->datei)
  {
   fn=malloc(sizeof(FAQ));
   if(fn)
   {
    memcpy(fn,f,sizeof(FAQ));
    fn->next=0L;
    if(faq==0L) faq=fn;
    else
    {
     fr=faq;
     while(fr->next) fr=fr->next;
     fr->next=fn;
    }
   }
   else ret=E_MEM;
  }
  else
  {
   ret=E_NOTALL;
   errline=zeile;
  }
 }
 else
 {
  ret=E_NOTALL;
  errline=zeile;
 }
 return(ret);
}

int add_line(char *line,char **adr)
{
 int ret=0;
 char *cp, *mem;

 cp=line;
 while(*cp++!=':');
 while(*cp==' ' || *cp=='\t') cp++;
 if(*cp)
 {
  mem=malloc(strlen(cp)+1);
  if(mem)
  {
   strcpy(mem,cp);
   cp=mem+strlen(mem)-1;
   while(*cp==' ' || *cp=='\t') cp--;
   *(cp+1)='\0';
   *adr=mem;
  }
  else ret=E_MEM;
 }
 else
 {
  ret=E_EMPTYKEY;
  errline=zeile;
 }
 return(ret);
}

int read_inf(void)
{
 int i, ret=0, fc=0, found_key=FALSE;
 long len;
 char *mem, *cp, line[128];
 FILE *fh;
 FAQ f;

 fh=fopen(inf_file,"rb");
 if(fh)
 {
  fseek(fh,0L,SEEK_END);
  len=ftell(fh);
  fseek(fh,0L,SEEK_SET);
  if(len>0)
  {
   mem=malloc(len+2);
   if(mem)
   {
    if(fread(mem,1L,len,fh)==len)
    {
     memset(&f,0,sizeof(FAQ));
     cp=mem;
     while(cp<mem+len && ret==0)
     {
      cp=get_line(line,cp,mem+len,128);
      if(line[0])
      {
       found_key=FALSE;
       for(i=0;keyword[i] && ret==0;i++)
       {
        if(strncmpi(line,keyword[i],strlen(keyword[i]))==0)
        {
         found_key=TRUE;
         switch(i)
         {
          case KEY_GRUPPE:  if(f.gruppe || f.name || f.von || f.betreff1 || f.datei)
                            {
                             ret=add_faq(&f);
                             if(ret==0) fc++;
                             memset(&f,0,sizeof(FAQ));
                            }
                            if(ret==0)
                            {
                             blockstart=zeile;
                             ret=add_line(line,&f.gruppe);
                            }
                            break;
          case KEY_NAME:    ret=add_line(line,&f.name);
                            break;
          case KEY_VON:     ret=add_line(line,&f.von);
                            break;
          case KEY_BETREFF: if(f.betreff1==0L) ret=add_line(line,&f.betreff1);
                            else if(f.betreff2==0L) ret=add_line(line,&f.betreff2);
                            break;
          case KEY_DATEI:   ret=add_line(line,&f.datei);
                            break;
          case KEY_INFO:    ret=add_line(line,&f.info);
                            break;
          case KEY_KEY:     ret=add_line(line,&f.key);
                            break;
         }
         break;
        }
       }
       if(!found_key)
       {
        ret=E_ILLKEY;
        errline=zeile;
       }
      }
     }
     if(ret==0 && (f.gruppe || f.name || f.von || f.betreff1 || f.datei))
     {
      ret=add_faq(&f);
      if(ret==0) fc++;
     }
     if(ret==0 && fc==0) ret=E_INFEMPTY;
    }
    else ret=E_INFREAD;
    free(mem);
   }
   else ret=E_MEM;
  }
  else ret=E_INFEMPTY;
  fclose(fh);
 }
 else ret=E_INFOPEN;
 return(ret);
}

static char *line=0L;
static long line_len=0L;

int read_line(FILE *fh)
{
 int i, complete=FALSE, ret=0;
 char *cp, buffer[130];

 line[0]='\0';
 while(!feof(fh) && !complete)
 {
 /*
  buffer[0]='\0';
  fgets(buffer,128,fh);
 */
  memset(buffer,0,128);
  for(i=0;i<128;i++)
  {
   buffer[i]=getc(fh);
   if(buffer[i]==EOF)
   {
    buffer[i]='\n';
    break;
   }
   if(buffer[i]=='\n') break;
  }
  if(buffer[0])
  {
   cp=buffer+strlen(buffer)-1;
   if(*cp=='\r' || *cp=='\n')
   {
    while(*cp=='\r' || *cp=='\n') cp--;
    *(cp+1)='\0';
    complete=TRUE;
   }
  }
  if(strlen(line)+strlen(buffer)+1>line_len)
  {
   cp=realloc(line,strlen(line)+strlen(buffer)+1);
   if(cp)
   {
    line=cp;
    line_len=strlen(line)+strlen(buffer)+1;
   }
   else ret=E_MEM;
  }
  strcat(line,buffer);
 }
 if(ret==0 && feof(fh)) ret=-1;
 return(ret);
}

int add_entry(char *line,char **adr)
{
 int ret=0;
 long len;

 len=strlen(line);
 if(*adr) free(*adr);
 *adr=malloc(len);
 if(*adr) strcpy(*adr,line+1);
 else ret=E_MEM;
 return(ret);
}

int find_key(FILE *fh,char *key)
{
 int found=FALSE;
 long fpos;
 char *templine;

 fpos=ftell(fh);
 templine=malloc(strlen(line)+1);
 if(templine)
 {
  strcpy(templine,line);
  do
  {
   if(line[0]!=':') break;
   if(strstr(line,key)!=0L)
   {
    found=TRUE;
    break;
   }
  }
  while(read_line(fh)==0);
  fseek(fh,fpos,SEEK_SET);
  strcpy(line,templine);
  free(templine);
 }
 return(found);
}

#ifdef LUNKWILL_110
FAQ *find_faq(FAQ *f,FILE *fh)
{
 int found=FALSE;
 FAQ *fp;

 fp=faq;
 while(fp)
 {
  if(strcmp(f->gruppe,fp->gruppe)==0)
  {
   if((f->von && fp->von && strcmp(f->von,fp->von)==0) ||
      (f->name && fp->name && strcmp(f->name,fp->name)==0))
   {
    if(match(fp->betreff1,f->betreff1))
    {
     found=TRUE;
     break;
    }
    if(fp->betreff2)
      if(match(fp->betreff2,f->betreff1))
      {
       found=TRUE;
       break;
      }
   }
  }
  fp=fp->next;
 }
 if(found && fp->key && !fp->append) found=find_key(fh,fp->key);
 return(found ? fp : 0L);
}
#endif

#ifdef LUNKWILL_111
FAQ *find_faq(FAQ *f, FILE *fh)
{
  int found = FALSE;
  FAQ *fp;

  fp = faq;
  while (fp)
  {
    if(strcmp (f->gruppe, fp->gruppe) == 0)
    {
      if( (f->von && fp->von && strcmp (f->von,fp->von) == 0) ||
         (f->name && fp->name && strcmp (f->name,fp->name) == 0))
      {
        if (match (fp->betreff1, f->betreff1))
          found = TRUE;
        else if (fp->betreff2)
        {
          if (match (fp->betreff2, f->betreff1))
            found = TRUE;
        }
        if (found)
        {
          if (fp->key && !fp->append)
            found = find_key (fh, fp->key);
          if (found)
            break;
        }
      }
    }
    fp=fp->next;
  }
  return(found ? fp : 0L);
}
#endif

FAQ *find_faq(FAQ *f, FILE *fh)
{
  int found = FALSE;
  FAQ *fp, *temp = 0L;

  fp = faq;
  while (fp)
  {
    if(strcmp (f->gruppe, fp->gruppe) == 0)
    {
      if( (f->von && fp->von && strcmp (f->von,fp->von) == 0) ||
         (f->name && fp->name && strcmp (f->name,fp->name) == 0))
      {
        if (match (fp->betreff1, f->betreff1))
          found = TRUE;
        else if (fp->betreff2)
        {
          if (match (fp->betreff2, f->betreff1))
            found = TRUE;
        }
        if (found)
        {
          if (fp->key)
          {
            if (fp->append)
            {
              temp = fp;
              found = FALSE;
            }
            else
              found = find_key (fh, fp->key);
          }
          if (found)
            break;
        }
      }
    }
    fp = fp->next;
  }
  if (!found && temp)
  {
    fp = temp;
    found = TRUE;
  }
  else if (found && temp)
  {
    temp->append = FALSE;
  }
  return(found ? fp : 0L);
}

void write_info(FILE *out,char *info)
{
 char *cp, string[32];

 cp=strchr(info,'%');
 if(cp==0L) fprintf(out,"%s\n",info);
 else
 {
  cp=info;
  while(*cp)
  {
   if(*cp=='%')
   {
    cp++;
    switch(toupper(*cp))
    {
     case '%': fputc('%',out);
               break;
     case 'D': gdtoa(Tgetdate(),string);
               fprintf(out,string);
               break;
     case 'L': fprintf(out,"Lunkwill V"ASCVERSION);
               break;
     case 'T': gttoa(Tgettime(),string);
               string[strlen(string)-3]='\0';
               fprintf(out,string);
               break;
     case 'Z': fputc('\n',out);
               break;
     default:  fputc(*cp,out);
    }
   }
   else fputc(*cp,out);
   cp++;
  }
  fputc('\n',out);
 }
}

int work_outfile(void)
{
 int pm, ret=0, found_faq=FALSE;
 FILE *fh, *out;
 FAQ f, *fx;

 memset(&f,0,sizeof(FAQ));
 fh=fopen(outfile,"rb");
 if(fh)
 {
  line_len=256;
  line=malloc(256);
  if(line)
  {
   ret=read_line(fh);
   while(ret==0)
   {
    pm=FALSE;
    do
    {
     ret=read_line(fh);
     if(line[0]==':') break;
     switch(line[0])
     {
      case 'A': pm=TRUE;
                break;
      case 'G': ret=add_entry(line,&f.gruppe);
                break;
      case 'N': ret=add_entry(line,&f.name);
                break;
      case 'V': ret=add_entry(line,&f.von);
                break;
      case 'W': ret=add_entry(line,&f.betreff1);
                break;
     }
    }
    while(ret==0 && !pm);
    found_faq=FALSE;
    if(ret==0 && !pm)
    {
     if((f.name || f.von) && f.gruppe && f.betreff1)
     {
      fx=find_faq(&f,fh);
      if(fx)
      {
       found_faq=TRUE;
       out=fopen(fx->datei,fx->append ? "a" : "w");
       if(out)
       {
        if(!fx->append && logfh>0)
        {
         gttoa(Tgettime(),logbuffer);
         strcat(logbuffer," FAQ in der Gruppe ");
         strcat(logbuffer,fx->gruppe);
         strcat(logbuffer," gefunden.\r\n");
         Fwrite(logfh,strlen(logbuffer),logbuffer);
         strcpy(logbuffer,"         Betreff: ");
         strcat(logbuffer,f.betreff1);
         strcat(logbuffer,"\r\n");
         Fwrite(logfh,strlen(logbuffer),logbuffer);
         strcpy(logbuffer,"         gespeichert unter ");
         strcat(logbuffer,fx->datei);
         strcat(logbuffer,"\r\n");
         Fwrite(logfh,strlen(logbuffer),logbuffer);
         faqs++;
        }
        if(!fx->append && fx->info) write_info(out,fx->info);
        do
        {
         if(line[0]==':') fprintf(out,"%s\n",line+1);
         ret=read_line(fh);
        }
        while(ret==0 && line[0]!='#');
        f.gruppe[0]=f.betreff1[0]='\0';
        fclose(out);
        fx->append=TRUE;
       }
       else ret=E_FAQOPEN;
      }
     }
    }
    if(!found_faq) /* Rest berlesen */
    {
     if(line[0]!=':') /* erstmal bis zum Anfang der Mail lesen */
     {
      do
      {
       ret=read_line(fh);
      }
      while(ret==0 && line[0]!=':');
     }
     do
     {
      ret=read_line(fh);
     }
     while(ret==0 && line[0]==':');
    }
   }
  }
  else ret=E_MEM;
  fclose(fh);
 }
 else ret=E_OUTOPEN;
 if(f.gruppe) free(f.gruppe);
 if(f.von) free(f.von);
 if(f.name) free(f.name);
 if(f.betreff1) free(f.betreff1);
 if(f.betreff2) free(f.betreff2);
 if(ret==-1) ret=0; /* EOF ist ja kein Fehler */
 return(ret);
}

int main(int argc,char *argv[])
{
 int i, ret=E_ARGS;

 if(argc>1)
 {
  outfile[0]=inf_file[0]='\0';
  for(i=1;i<argc;i++)
    if(stricmp(&argv[i][strlen(argv[i])-strlen(INF_EXT)],INF_EXT)==0)
      strcpy(inf_file,argv[i]);
    else if(stricmp(&argv[i][strlen(argv[i])-strlen(OUTFILE)],OUTFILE)==0)
      strcpy(outfile,argv[i]);
    else if(strncmpi(&argv[i][strlen(argv[i])-strlen(OUTFILE)],OUTFILE_X,8)==0)
      strcpy(outfile,argv[i]);
    else if(stricmp(argv[i],"--test")==0)
      testmode=TRUE;
    else if(stricmp(argv[i],"--nolog")==0)
      nolog=TRUE;
  if(outfile[0]=='\0') ret=E_ARGS;
  else
  {
   ret=0;
   if(inf_file[0]=='\0') strcpy(inf_file,INF_FILE);
   if(!fexists(outfile)) ret=E_OUTNF;
   else if(!fexists(inf_file)) ret=E_INFNF;
   else
   {
    open_logfile();
    ret=read_inf();
    if(ret==0 && !testmode)
    {
     ret=work_outfile();
    }
    close_logfile(ret);
    free_faq();
   }
  }
 }
 if(ret==E_ARGS)
 {
  Cconws("Lunkwill, V"ASCVERSION" ("__DATE__")\r\n"
         "Public Doman, written 1996/97 by Dirk Haun\r\n\r\n"
         "Aufruf: ");
  if(argv[0][0]) Cconws(argv[0]);
  else Cconws("lunkwill.ttp");
  Cconws(" [--test] [--nolog] "OUTFILE" ["INF_FILE"]\r\n\r\n");
  ret=1;
 }
 return(ret);
}

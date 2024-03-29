/*  

    YABASIC ---  a simple Basic Interpreter
    written by Marc-Oliver Ihm 1995-2004
    homepage: www.yabasic.de
    
    function.c --- code for functions
    
    This file is part of yabasic and may be copied only 
    under the terms of either the Artistic License or 
    the GNU General Public License (GPL), both of which 
    can be found at www.yabasic.de

*/


/* ------------- includes ---------------- */
#include "YabInterface.h"
#ifndef YABASIC_INCLUDED
#include "yabasic.h"       /* all prototypes and structures */
#endif


/* ------------- external references ---------------- */

extern int mylineno;   /* current line number */
extern int yyparse();  /* call bison parser */


/* ------------- local functions ---------------- */

static char *fromto(char *,int,int); /* get portion of string (mid$ et al) */
static void clear_buff(); /* clear system-input buffers */
static void store_buff(char *,int); /* store system-input buffer */
static int do_glob(char *,char *); /* actually do the globbing */
static double other2dec(char *,int); /* convert hex to decimal */
static char *dec2other(double,int); /* convert decimal to hex */
static double peek(char *, YabInterface *); /* peek into internals */
static char *peek2(char *,struct command *); /* peek into internals */
static char *peek3(char *,char *); /* peek into internals */
static int peekfile(int); /* read a byte from stream */
static char *do_system(char *); /* executes command via command.com */
static int do_system2(char *); /* execute command as system */


/* ------------- global variables ---------------- */

struct command *lastdata=NULL; /* used to associate all data-commands with each others */
static struct buff_chain *buffroot; /* start of sys-input buffer */
static struct buff_chain **buffcurr; /* current entry in buff_chain */
static int buffcount; /* number of filled buffers */


/* ------------- subroutines ---------------- */


void token(struct command *cmd) /* extract token from variable */
{
  int split;
  struct stackentry *s;
  struct symbol *sym;
  struct array *ar;
  int num=0,i;
  char *p,*q;
  char **pp;
  char *del,*line;
  int wasdel,isdel;

  
  if (cmd->type==cSPLIT2 || cmd->type==cTOKEN2)
    del=pop(stSTRING)->pointer;
  else
    del=" \t";
  split=(cmd->type==cSPLIT || cmd->type==cSPLIT2);
  s=pop(stSTRINGARRAYREF);
  line=pop(stSTRING)->pointer;
  sym=get_sym(s->pointer,syARRAY,amSEARCH);
  if (!sym || !sym->pointer) {
    sprintf(string,"array '%s()' is not defined",strip(s->pointer));
    error(ERROR,string);
    goto token_done;
  }
  ar=sym->pointer;
  if (ar->dimension>1) {
    error(ERROR,"only one dimensional arrays allowed");
    goto token_done;
  }

  /* count number of tokens */
  isdel=TRUE;
  if (split && *line) num=1;
  else num=0;
  for(p=line;*p;++p) {
    wasdel=isdel;
    isdel=(strchr(del,*p)!=NULL);
    if (split) {
      if (isdel) num++;
    } else {
      if (isdel && isdel!=wasdel) num++;
    }
  }
  if (!split && !isdel) num++;

  /* free previous array content */
  for(i=0;i<ar->bounds[0];i++) free(((char **)ar->pointer)[i]);
  free(ar->pointer);
  ar->pointer=my_malloc((num+1)*sizeof(char *));
  pp=ar->pointer;
  pp[0]=my_strdup("");
  
  /* extract tokens */
  i=1;
  isdel=TRUE;
  if (*line) {
    for(p=q=line;;p++) {
      wasdel=isdel;
      isdel=(strchr(del,*p)!=NULL) || !*p;
      if ((split && isdel) || (!split && (isdel && isdel!=wasdel))) {
	while(strchr(del,*q) && q<p) q++;
	pp[i]=my_strndup(q,p-q+1);
	pp[i][p-q]='\0';
	q=p+1;
	i++;
      }
      if (!*p) break;
    }
  }
      
  ar->bounds[0]=num+1;
 token_done:
  s=push();
  s->type=stNUMBER;
  s->value=num;
}


void tokenalt(struct command *cmd) /* extract token from variable with alternate semantics */
{
  char *del; /* delimiter for strings */
  struct stackentry *t;
  char *old,*new,*tok;
  int split;
  
  if (cmd->type==cSPLITALT2 || cmd->type==cTOKENALT2)
    del=pop(stSTRING)->pointer;
  else
    del=" \t";
  split=(cmd->type==cSPLITALT || cmd->type==cSPLITALT2);
  
  t=pop(stSTRING);
  old=t->pointer;
  t->pointer=NULL; /* prevent push from freeing the memory */
  t=push();
  t->type=stSTRING;
  new=old;
  tok=NULL;
  while(*new) {
    if (!tok && (!strchr(del,*new) || split)) tok=new; /* found start of token */
    if (tok && strchr(del,*new)) break; /* found end of token */
    new++;
  }
  if (*new) {
    *new='\0'; /* terminate token */
    new++;
    if (!split) {
      while(*new) {
        if (!strchr(del,*new)) break; /* found start of next token */
        new++;
      }
    }
  }
  t->pointer=my_strdup(tok?tok:""); /* copy token */
  /* move rest of string */
  while(*new) {
    *old=*new;
    old++;
    new++;
  };
  *old='\0';
}


void glob(void) /* check, if pattern globs string */
{
  char *str,*pat;
  struct stackentry *stack;
  int res;
  
  pat=(char *)pop(stSTRING)->pointer;
  str=(char *)pop(stSTRING)->pointer;
  
  res=do_glob(str,pat);
  stack=push();
  stack->value=res;
  stack->type=stNUMBER;
}


static int do_glob(char *str,char *pat) /* actually do the globbing */
{
  int res;
  
  if (infolevel>=DEBUG) {
    sprintf(string,"globbing '%s' on '%s'",str,pat);
    error(DEBUG,string);
  }
  if (*pat=='\0' && *str=='\0') return TRUE;
  else if (*pat=='\0') return FALSE;
  else if (*pat=='?' && *str=='\0') return FALSE;
  else if (*pat=='?') {
    if (*str=='\0') return FALSE;
    pat++;
    str++;
  }
  else if (*pat=='*') {
    pat++;
    res=FALSE;
    while(*str && !(res=do_glob(str,pat))) str++;
    if (res) return TRUE;
  }
  else if (*str=='\0') return FALSE;
  else {
    while(*pat && *pat!='?' && *pat!='*') {
      if (*pat!=*str) return FALSE;
      str++;
      pat++;
    }
  }
  return do_glob(str,pat);
}


void concat() /* concatenates two strings from stack */
{
  struct stackentry *c;
  char *aa,*bb,*cc;
  
  aa=pop(stSTRING)->pointer;
  bb=pop(stSTRING)->pointer;
  cc=(char *) my_malloc(sizeof(char)*(strlen(aa)+strlen(bb)+1));
  strcpy(cc,bb);
  strcat(cc,aa);
  c=push();
  c->type=stSTRING;
  c->pointer=cc;
}  


void create_changestring(int type) /* create command 'changestring' */
{
  struct command *cmd;
  
  cmd=add_command(cCHANGESTRING,FALSE);
  cmd->args=type;
}


void changestring(struct command *current) /* changes a string */
{
  int type,a2,a3;
  char *newpart;
  char *oldstring;
  int i,len;
  struct stackentry *a1;
  
  type=current->args;
  newpart=pop(stSTRING)->pointer;
  if (type>fTWOARGS) a3=(int)pop(stNUMBER)->value;
  if (type>fONEARGS) a2=(int)pop(stNUMBER)->value;
  a1=pop(stSTRING);
  oldstring=a1->pointer; 
  a1->pointer=NULL; /* this prevents push from freeing the memory */
  
  if (!oldstring || !*oldstring) return;
  switch(type) {
  case fMID:
    for(i=1;i<a2+a3;i++) {
      if (!oldstring[i-1]) break;
      if (i>=a2) {
        if (!newpart[i-a2]) break;
        oldstring[i-1]=newpart[i-a2];
      }
    }
    break;
  case fMID2:
    len=strlen(oldstring);
    for(i=1;i<=len;i++) {
      if (!oldstring[i-1]) break;
      if (i>=a2) {
        if (!newpart[i-a2]) break;
        oldstring[i-1]=newpart[i-a2];
      }
    }
    break;
  case fLEFT:
    for(i=1;i<=a2;i++) {
      if (!oldstring[i-1] || !newpart[i-1]) break;
      oldstring[i-1]=newpart[i-1];
    }
    break;
  case fRIGHT:
    len=strlen(oldstring);
    for(i=1;i<=len;i++) {
      if (i>len-a2) {
        if (!newpart[i-1-len+a2]) break;
        oldstring[i-1]=newpart[i-1-len+a2];
      }
    }
    break;
  }
}


void create_function(int type) /* create command 'function' */
/* type can be sin,cos,mid$ ... */
{
  struct command *cmd;
  
  cmd=add_command(cFUNCTION,FALSE);
  cmd->args=type;  
}


void function(struct command *current,YabInterface* yab) /* performs a function */
{
  struct stackentry *stack,*a1,*a2,*a3,*a4,*a5,*a6;
  char *pointer;
  double value,result2;
  time_t datetime;
  int type,result,len,start,i,max, linenum,x;
  char *str,*str2, *str3, *str4, *str5;
	char out[512];
	int d = 0;
  a3=NULL;
  a4=NULL;
  a5=NULL;
  a6=NULL;
  type=current->args;
  if (type>fFIVEARGS) a6=pop(stSTRING_OR_NUMBER);
  if (type>fFOURARGS) a5=pop(stSTRING_OR_NUMBER);
  if (type>fTHREEARGS) a4=pop(stSTRING_OR_NUMBER);
  if (type>fTWOARGS) a3=pop(stSTRING_OR_NUMBER);
  if (type>fONEARGS) a2=pop(stSTRING_OR_NUMBER);
  if (type>fZEROARGS) a1=pop(stSTRING_OR_NUMBER);
  linenum = current->line;

  switch (type) {
  case fSIN:
    value=sin(a1->value);
    result=stNUMBER;
    break;
  case fASIN:
    value=asin(a1->value);
    result=stNUMBER;
    break;
  case fCOS:
    value=cos(a1->value);
    result=stNUMBER;
    break;
  case fACOS:
    value=acos(a1->value);
    result=stNUMBER;
    break;
  case fTAN:
    value=tan(a1->value);
    result=stNUMBER;
    break;
  case fATAN:
    value=atan(a1->value);
    result=stNUMBER;
    break;
  case fEXP:
    value=exp(a1->value);
    result=stNUMBER;
    break;
  case fLOG:
    value=log(a1->value);
    result=stNUMBER;
    break;
  case fLOG2:
    value=log(a1->value)/log(a2->value);
    result=stNUMBER;
    break;
  case fLEN:
    value=(double) strlen(a1->pointer);
    result=stNUMBER;
    break;
  case fSTR:
    sprintf(string,"%g",a1->value);
    pointer=my_strdup(string);
    result=stSTRING;
    break;
  case fSTR2:
  case fSTR3:
    result=stSTRING;
    if (!myformat(string,INBUFFLEN, a1->value,a2->pointer,a3?a3->pointer:NULL)) {
      pointer=my_strdup("");
      sprintf(string,"'%s' is not a valid format",(char *)a2->pointer);
      error(ERROR,string);
      break;
    }
    pointer=my_strdup(string);
    break;
  case fCHOMP: //new command from yabasic 2.8.6  05.06.2020 Lorglas
        result = stSTRING;
	pointer = a1->pointer;
	a1->pointer = NULL;
	pointer[strcspn(pointer, "\r\n")] = 0;
	break;
  case fSQRT:
    value=sqrt(a1->value);
    result=stNUMBER;
    break;
  case fSQR:
    value=a1->value*a1->value;
    result=stNUMBER;
    break;
  case fINT:
    if (a1->value<0)
      value=-floor(-a1->value);
    else
      value=floor(a1->value);
    result=stNUMBER;
    break;  
  case fCEIL: //added new command Lorglas 2020.09.11
        value = ceil(a1->value);
        result = stNUMBER;
        break;
    case fFLOOR: //added new command Lorglas 2020.09.11
	value = floor(a1->value);
        result = stNUMBER;
        break;
  case fROUND: //added new command Lorglas 2020.09.11
	value = round(a1->value);
    result = stNUMBER;
  break;
  case fFRAC:
    if (a1->value<0)
      value=a1->value+floor(-a1->value);
    else
      value=a1->value-floor(a1->value);
    result=stNUMBER;
    break;
  case fABS:
    value=fabs(a1->value);
    result=stNUMBER;
    break;
  case fSIG:
    if (a1->value<0) value=-1.;
    else if (a1->value>0) value=1.;
    else value=0.;
    result=stNUMBER;
    break;
  case fMOD:
    value=a1->value-a2->value*(int)(a1->value/a2->value);
    result=stNUMBER;
    break;
  case fRAN:
    value=a1->value*((double)rand()/RAND_MAX);
    result=stNUMBER;
    break;
  case fRAN2:
    value=(double)rand()/RAND_MAX;
    result=stNUMBER;
    break;
  case fMIN:
    if (a1->value>a2->value)
      value=a2->value;
    else
      value=a1->value;
    result=stNUMBER;
    break;
  case fMAX:
    if (a1->value>a2->value)
      value=a1->value;
    else
      value=a2->value;
    result=stNUMBER;
    break;
  case fVAL:
    i=sscanf((char *) a1->pointer,"%lf",&value);
    if (i!=1) value=0;
    result=stNUMBER;
    break;
  case fATAN2:
    value=atan2(a1->value,a2->value);
    result=stNUMBER;
    break;
  case fLEFT:
    str=a1->pointer;
    len=(int)a2->value;
    pointer=fromto(str,0,len-1);
    result=stSTRING;
    break;
  case fRIGHT:
    str=a1->pointer;
    max=strlen(str);
    len=(int)a2->value;
    pointer=fromto(str,max-len,max-1);
    result=stSTRING;
    break;
  case fMID:
    str=a1->pointer;
    start=(int)a2->value;
    len=(int)a3->value;
    pointer=fromto(str,start-1,start+len-2);
    result=stSTRING;
    break;
  case fMID2:
    str=a1->pointer;
    start=(int)a2->value;
    pointer=fromto(str,start-1,strlen(str));
    result=stSTRING;
    break;
  case fINKEY:
    pointer=inkey(a1->value);
    result=stSTRING;
    break;
  case fAND:
    value=(int)a1->value & (int)a2->value;
    result=stNUMBER;
    break;
  case fOR:
    value=(int)a1->value | (int)a2->value;
    result=stNUMBER;
    break;
  case fEOR:
    value=(int)a1->value ^ (int)a2->value;
    result=stNUMBER;
    break;
  case fBITNOT: //new command from yabasic 2.8.6  05.06.2020 Lorglas
        value = ~ (unsigned int) a1->value;
        result = stNUMBER;
        break;
  case fSHL: //new command from yabasic 2.8.6  05.06.2020 Lorglas
	value = (unsigned int) a1->value << (int) a2->value;
	result = stNUMBER;
  break;
  case fSHR: //new command from yabsic 2.8.6  05.06.2020 Lorglas
	value = (unsigned int) a1->value >> (int) a2->value;
	result = stNUMBER;
  break;
/*
  case fMOUSEX:
    getmousexybm(a1->pointer,&i,NULL,NULL,NULL);
    value=i;
    result=stNUMBER;
    break;
  case fMOUSEY:
    getmousexybm(a1->pointer,NULL,&i,NULL,NULL);
    value=i;
    result=stNUMBER;
    break;
  case fMOUSEB:
    getmousexybm(a1->pointer,NULL,NULL,&i,NULL);
    value=i;
    result=stNUMBER;
    break;
  case fMOUSEMOD:
    getmousexybm(a1->pointer,NULL,NULL,NULL,&i);
    value=i;
    result=stNUMBER;
    break;*/
  case fCHR:
    pointer=my_malloc(2);
    i=(int)floor(a1->value);
    if (i>255 || i<0) {
      sprintf(string,"can't convert %g to character",a1->value);
      error(ERROR,string);
      return;
    }
    pointer[1]='\0';
    pointer[0]=(unsigned char)i;
    result=stSTRING;
    break;
  case fASC:
    value=((unsigned char *)a1->pointer)[0];
    result=stNUMBER;
    break;
  case fBIN:
    pointer=dec2other(a1->value,2);
    result=stSTRING;
    break;
  case fHEX:
    pointer=dec2other(a1->value,16);
    result=stSTRING;
    break;
  case fDEC:
    value=other2dec(a1->pointer,16);
    result=stNUMBER;
    break;
  case fDEC2:
    value=other2dec(a1->pointer,(int)(a2->value));
    result=stNUMBER;
    break;
  case fUPPER:
    str=a1->pointer;
    pointer=my_malloc(strlen(str)+1);
    i=-1;
    do {
      i++;
      pointer[i]=toupper((int)str[i]);
    } while(pointer[i]);
    result=stSTRING;
    break;
  case fLOWER:
    str=a1->pointer;
    pointer=my_malloc(strlen(str)+1);
    i=-1;
    do {
      i++;
      pointer[i]=tolower((int)str[i]);
    } while(pointer[i]);
    result=stSTRING;
    break;
  case fLTRIM:
    str=a1->pointer;
    while(isspace(*str)) str++;
    pointer=my_strdup(str);
    result=stSTRING;
    break;
  case fRTRIM:
    str=a1->pointer;
    i=strlen(str)-1;
    while(isspace(str[i]) && i>=0) i--;
    str[i+1]='\0';
    pointer=my_strdup(str);
    result=stSTRING;
    break;
  case fTRIM:
    str=a1->pointer;
    i=strlen(str)-1;
    while(isspace(str[i]) && i>=0) i--;
    str[i+1]='\0';
    while(isspace(*str)) str++;
    pointer=my_strdup(str);
    result=stSTRING;
    break;
  case fINSTR:
    str=a1->pointer;
    str2=a2->pointer;
    if (*str2)
      pointer=strstr(str,str2);
    else
      pointer=NULL;
    if (pointer==NULL) 
      value=0;
    else
      value=pointer-str+1;
    result=stNUMBER;
    break;   
  case fINSTR2:
    str=a1->pointer;
    str2=a2->pointer;
    start=(int)a3->value;
    if (start>strlen(str)) {
      value=0;
    } else {
      if (start<1) start=1;
      pointer=strstr(str+start-1,str2);
      if (pointer==NULL) 
	value=0;
      else
	value=pointer-str+1;
    }
    result=stNUMBER;
    break;   
  case fRINSTR:
    str=a1->pointer;
    str2=a2->pointer;
    len=strlen(str2);
    for(i=strlen(str)-1;i>=0;i--) if (!strncmp(str+i,str2,len)) break;
    value=i+1;
    result=stNUMBER;
    break;   
  case fRINSTR2:
    str=a1->pointer;
    str2=a2->pointer;
    len=strlen(str2);
    start=(int)a3->value;
    if (start<1) {
      value=0;
    } else {
      if (start>strlen(str)) start=strlen(str);
      for(i=start-1;i;i--) if (!strncmp(str+i,str2,len)) break;
      value=i+1;
    }
    result=stNUMBER;
    break;       
   case fSTR_REPLACE:   //added new command Lorglas 2020.09.11
    str=a1->pointer;
    str2=a2->pointer;
     str3=a3->pointer;
   pointer= replaceWordInText(str, str2, str3);
    result=stSTRING;

    break;
  case fDATE:
    pointer=my_malloc(100);
    time(&datetime);
    strftime(pointer,100,"%w-%m-%d-%Y-%a-%b",localtime(&datetime));
    result=stSTRING;
    break;
  case fTIME:
    pointer=my_malloc(100);
    time(&datetime);
    strftime(pointer,100,"%H-%M-%S",localtime(&datetime));
    sprintf(pointer+strlen(pointer),"-%d",
      (int)(time(NULL)-compilation_start));
    result=stSTRING;
    break;
  case fSYSTEM:
    str=a1->pointer;
    pointer=do_system(str);
    result=stSTRING;
    break;
  case fSYSTEM2:
    str=a1->pointer;
    value=do_system2(str);
    result=stNUMBER;
    break;
  case fPEEK:
    str=a1->pointer;
    value=peek(str, yab);
    result=stNUMBER;
    break;
  case fPEEK2:
    str=a1->pointer;
    pointer=peek2(str,current);
    result=stSTRING;
    break;
  case fPEEK3:
    str=a1->pointer;
    str2=a2->pointer;
    pointer=peek3(str,str2);
    result=stSTRING;
    break;
  case fPEEK4:
    value=peekfile((int)a1->value);
    result=stNUMBER;
    break;
  case fGETCHAR:
    pointer=getchars((int)a1->value,(int)a2->value,(int)a3->value,(int)a4->value,(int)a5->value,(int)a6->value);
    result=stSTRING;
    break;
  case fTELL:
    i=(int)(a1->value);
    if (badstream(i,0)) return;
    if (!(stream_modes[i] & (smREAD | smWRITE| smREADWRITE))) {
      sprintf(string,"stream %d not opened",i);
      error(ERROR,string);
      value=0;
    } else {
      value=ftell(streams[i]);
    }
    result=stNUMBER;
    break;
  case fMESSAGE:
    pointer = getmessages(yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fMOUSE:
    str = a1->pointer;
    pointer = getmousemessages(str,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fTRANSLATE:
    str = a1->pointer;
    pointer = gettranslation(str,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fMENUTRANSLATE:
    str = a1->pointer;
    pointer = getmenutranslation(str,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fTEXTGET:
    str = a1->pointer;
    pointer = textget(str,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fTEXTGET3:
    str = a1->pointer;
    pointer = textget3(str,a2->value,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fTEXTGET6:
    str = a1->pointer;
    str2 = a2->pointer;
    pointer = textget6(str,str2,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fTEXTCONTROLGET:
    str = a1->pointer;
    pointer = textcontrolget(str,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fNUMWINDOWS:
    value = numwindows();
    result = stNUMBER;
    break;
  case fISMOUSEIN:
    str = a1->pointer;
    value = ismousein(str,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fMOUSEMOVE: //vasper
    pointer = getmousein(yab,linenum, current->lib->s); 
    result = stSTRING;
    break;    
  case fDRAWIMAGE:
    str=a3->pointer;
    str2=a4->pointer;
    value = createimage(a1->value,a2->value,str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fDRAWIMAGE2:
    str=a5->pointer;
    str2=a6->pointer;
    value = createimage2(a1->value,a2->value,a3->value,a4->value,str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fDRAWSVG:
    str=a5->pointer;
    str2=a6->pointer;
    value = createsvg(a1->value,a2->value,a3->value,a4->value,str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fLOAD:
    str=a1->pointer;
    str2=a2->pointer;
    str3=a3->pointer;
    pointer = (str,str2,str3,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fSAVE:
    str=a1->pointer;
    str2=a2->pointer;
    str3=a3->pointer;
    str4=a4->pointer;
    pointer = getsavefilepanel(str,str2,str3,str4,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fKEYBOARD:
    str=a1->pointer;
    pointer = keyboardmessages(str,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fCLIPBOARDPASTE:
    pointer = clipboardpaste(yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fCOLUMNBOXGET:
    str=a1->pointer;
    pointer = columnboxget(str,a2->value,a3->value,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fLISTBOXGET:
    str=a1->pointer;
    pointer = listboxget(str,a2->value,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fTREEBOXGET:
    str=a1->pointer;
    pointer = treeboxget(str,a2->value,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fTEXTGET2:
    str=a1->pointer;
    str2=a2->pointer;
    value = textget2(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fTEXTGET4:
    str=a1->pointer;
    str2=a2->pointer;
    value = textget4(str,str2,a3->value,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fTEXTGET5:
    str=a1->pointer;
    str2=a2->pointer;
    str3=a3->pointer;
    value = textget5(str,str2,str3,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fPOPUPMENU:
    str=a3->pointer;
    str2=a4->pointer;
    pointer = popupmenu(a1->value, a2->value, str, str2,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fSCROLLBARGET:
    str=a1->pointer;
    str2=a2->pointer;
    value = scrollbarget(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fSPLITVIEWGET:
    str=a1->pointer;
    str2=a2->pointer;
    value = splitviewget(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fSTACKVIEWGET:
    str=a1->pointer;
    value = stackviewget(str,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fTABVIEWGET:
    str=a1->pointer;
    value = tabviewget(str,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fSPINCONTROLGET:
    str=a1->pointer;
    value = spincontrolget(str,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fLISTBOXCOUNT:
    str=a1->pointer;
    value = listboxcount(str,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fTREEBOXCOUNT:
    str=a1->pointer;
    value = treeboxcount(str,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fDROPBOXCOUNT:
    str=a1->pointer;
    value = dropboxcount(str,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fDROPBOXGET:
    str=a1->pointer;
    pointer = dropboxget(str,a2->value,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fDRAWGET3:
    str=a1->pointer;
    pointer = drawget3(str,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fSLIDERGET:
    str=a1->pointer;
    value = sliderget(str,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fCOLORCONTROLGET:
    str=a1->pointer;
    str2=a2->pointer;
    value = colorcontrolget(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fCALENDAR:
    str=a1->pointer;
    pointer = calendar2(str,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fCOLUMNBOXCOUNT:
    str=a1->pointer;
    value = columnboxcount(str,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fWINDOWGET:
    str=a1->pointer;
    str2=a2->pointer;
    value = windowgetnum(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fVIEWGET: //vasper
    str=a1->pointer;
    str2=a2->pointer;
    value = viewgetnum(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;    
  case fDRAWGET1:
    str=a1->pointer;
    str2=a2->pointer;
    str3=a3->pointer;
    value = drawget1(str,str2,str3,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fDRAWGET2:
    str=a1->pointer;
    str2=a2->pointer;
    value = drawget2(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fDRAWGET4:
    str=a3->pointer;
    str2=a4->pointer;
    value = drawget4(a1->value, a2->value, str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fLISTBOXGETNUM:
    str=a1->pointer;
    value = listboxgetnum(str, yab, linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fDROPBOXGETNUM:
    str=a1->pointer;
    value = dropboxgetnum(str, yab, linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fTREEBOXGETNUM:
    str=a1->pointer;
    value = treeboxgetnum(str, yab, linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fCOLUMNBOXGETNUM:
    str=a1->pointer;
    value = columnboxgetnum(str, yab, linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fBITMAPLOAD:
    str=a1->pointer;
    str2=a2->pointer;
    value = bitmapload(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fBITMAPGET:
    str=a1->pointer;
    str2=a2->pointer;
    value = bitmapgetnum(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fBITMAPCOLOR:
    str = a3->pointer;
    str2 = a4->pointer;
    value = bitmapcolor(a1->value, a2->value, str, str2, yab, linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fBITMAPSAVE:
    str=a1->pointer;
    str2=a2->pointer;
    str3=a3->pointer;
    value = bitmapsave(str,str2,str3,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;  
  case fSOUND:  //Reactivate Sound Lorglas 2020.01.02
 	str=a1->pointer;
    //value=a2->value;
    //sprintf(string,"stream %s %s not opened",str,value);
      //error(ERROR,string);
    value = sound(str, a2->value, yab, linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fMEDIASOUND: //added Sound Lorglas 2020.01.02
    str=a1->pointer;
    value = mediasound(str, yab, linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fTREEBOXGETOPT:
    str=a1->pointer;
    str2=a2->pointer;
    value = treeboxgetopt(str, str2, a3->value, yab, linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fTHREADKILL:
    str=a1->pointer;
    value = threadkill(str, a2->value, yab, linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fTHREADGET:
    str=a1->pointer;
    str2=a2->pointer;
    value = threadget(str, str2, yab, linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fALERT:
    str=a1->pointer;
    str2=a2->pointer;
    str3=a3->pointer;
    str4=a4->pointer;
    str5=a5->pointer;
    value = newalert(str,str2,str3,str4,str5,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fPRINTER:
    str=a1->pointer;
    str2=a2->pointer;
    str3=a3->pointer;
    value = printer(str,str2,str3,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fISCOMPUTERON:
    value = iscomputeron(yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fPCWORKSPACES: //added new command Lorglas 2020.09.11
    value = pcworkspaces(yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fMESSAGESEND:
    str=a1->pointer;
    str2=a2->pointer;
    value = messagesend(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fATTRIBUTEGET1:
    str=a1->pointer;
    str2=a2->pointer;
    pointer = attributeget1(str,str2,yab,linenum, current->lib->s);
    result = stSTRING;
    break;
  case fATTRIBUTEGET2:
    str=a1->pointer;
    str2=a2->pointer;
    value = attributeget2(str,str2,yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  case fAVAILABLELANGUAGE:  //added new command Lorglas 2020.09.11
    str=a1->pointer; 
  	pointer = availablelanguage(str,yab,linenum, current->lib->s);
    result = stSTRING;
    break;    
  case fAVAILABLETIMEZONES:  //added new command Lorglas 2020.08.04
    //str=a1->pointer;    
  	pointer = availabletimezones(str,yab,linenum, current->lib->s);
    result = stSTRING;
    break;    
  case fDEFAULTTIMEZONE: //added new command Lorglas 2020.09.11
    value = defaulttimezone(yab,linenum, current->lib->s);
    result = stNUMBER;
    break;
  default:
    error(ERROR,"function called but not implemented");
    return;
  }
  
  stack=push();
  /* copy result */
  stack->type=result;
  if (result==stSTRING)
    stack->pointer=pointer;
  else
    stack->value=value;
}


static int do_system2(char *cmd) /* execute command as system */
{
#ifdef UNIX
  int ret;
  #ifdef BUILD_NCURSES
  if (curinized) reset_shell_mode();
  #endif
  ret=system(cmd);
  #ifdef BUILD_NCURSES
  if (curinized) reset_prog_mode();
  #endif
  return ret;
#else
  STARTUPINFO start;
  PROCESS_INFORMATION proc;
  DWORD ec; /* exit code */
  SECURITY_ATTRIBUTES prosec;
  SECURITY_ATTRIBUTES thrsec;
  char *comspec;
		
  ZeroMemory(&prosec,sizeof(prosec));
  prosec.nLength=sizeof(prosec);
  prosec.bInheritHandle=TRUE;
  ZeroMemory(&thrsec,sizeof(thrsec));
  thrsec.nLength=sizeof(thrsec);
  thrsec.bInheritHandle=TRUE;
  ZeroMemory(&start,sizeof(start));
  start.cb=sizeof(STARTUPINFO); 
  start.dwFlags=STARTF_USESTDHANDLES;
  start.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
  start.hStdError=GetStdHandle(STD_ERROR_HANDLE);
  start.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
  comspec=getenv("COMSPEC");
  if (!comspec) comspec="command.com";
  sprintf(string,"%s /C %s",comspec,cmd);
  if (!CreateProcess(NULL,string,&prosec,&thrsec,TRUE,0,
    NULL,NULL,&start,&proc)) {
    sprintf(string,"couldn't execute '%s'",cmd);
    error(ERROR,string);
    return -1;
  }
  WaitForSingleObject(proc.hProcess,INFINITE);
  if (!GetExitCodeProcess(proc.hProcess,&ec)) ec=-1;
  CloseHandle(proc.hProcess);
  CloseHandle(proc.hThread);
  return ec;
#endif  
}


static void clear_buff() /* clear system-input buffers */
{
  buffcurr=&buffroot;
  buffcount=0;
}


static void store_buff(char *buff,int len) /* store system-input buffer */
{
  *buffcurr=my_malloc(sizeof(struct buff_chain));
  memcpy((*buffcurr)->buff,buff,SYSBUFFLEN+1);
  (*buffcurr)->len=len;
  buffcurr=&((*buffcurr)->next);
  buffcount++;
}


char *recall_buff() /* recall store buffer */
{
  struct buff_chain *curr,*old;
  char *result;
  int done,len;
		
  result=(char *)my_malloc(buffcount*(SYSBUFFLEN+1));
  curr=buffroot;
  len=0;
  for(done=0;done<buffcount;done++) {
    memcpy(result+len,curr->buff,SYSBUFFLEN);
    len+=curr->len;
    old=curr;
    curr=curr->next;
    my_free(old);
  }
  return result;
}


static char *do_system(char *cmd) /* executes command via command.com */
{
  static char buff[SYSBUFFLEN+1]; /* buffer to store command */
  int len; /* number of bytes read */
#ifdef UNIX
  FILE *p; /* points to pipe */
  int c; /* char read from pipe */
#else
  int ret;
  STARTUPINFO start;
  PROCESS_INFORMATION proc;
  HANDLE piperead,pipewrite; /* both ends of pipes */
  SECURITY_ATTRIBUTES prosec;
  SECURITY_ATTRIBUTES thrsec;
  char *comspec;
#endif
  
  clear_buff();
  
#ifdef UNIX
  p=popen(cmd,"r");
  if (p==NULL) {
    sprintf(string,"couldn't execute '%s'",cmd);
    error(ERROR,string);
    return my_strdup("");
  }
  do {
    len=0;
    while(len<SYSBUFFLEN) {
      c=fgetc(p);
      if (c==EOF) {
        buff[len]='\0';
        break;
      }
      buff[len]=c;
      len++;
    }
    store_buff(buff,len);
  } while(c!=EOF);
  pclose(p);
		
#else	     
  ZeroMemory(&prosec,sizeof(prosec));
  prosec.nLength=sizeof(prosec);
  prosec.bInheritHandle=TRUE;
  ZeroMemory(&thrsec,sizeof(thrsec));
  thrsec.nLength=sizeof(thrsec);
  thrsec.bInheritHandle=TRUE;
		
  /* create pipe for writing */
  CreatePipe(&piperead,&pipewrite,&prosec,0);
		
  ZeroMemory(&start,sizeof(start));
  start.cb=sizeof(STARTUPINFO); 
  start.dwFlags=STARTF_USESTDHANDLES;
  start.hStdOutput=pipewrite;
  start.hStdError=pipewrite;
  start.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
		
  comspec=getenv("COMSPEC");
  if (!comspec) comspec="command.com";
  sprintf(string,"%s /C %s",comspec,cmd);
  if (!CreateProcess(NULL,string,&prosec,&thrsec,TRUE,0,
    NULL,NULL,&start,&proc)) {
    sprintf(string,"couldn't execute '%s'",cmd);
    error(ERROR,string);
    return my_strdup("");
  }
  CloseHandle(pipewrite);
		
  do {
    /* wait for output to arrive */
    if (!ReadFile(piperead,buff,SYSBUFFLEN,(LPDWORD)&len,NULL))
      ret=GetLastError();
    else
      ret=0;
    buff[len]='\0';
    if (len>0) store_buff(buff,len);
  } while(ret!=ERROR_BROKEN_PIPE && ret!=ERROR_HANDLE_EOF);
  CloseHandle(piperead);
  CloseHandle(proc.hProcess);
  CloseHandle(proc.hThread);
#endif
  return recall_buff();
}


void getmousexybm(char *s,int *px,int *py,int *pb,int *pm) /* get mouse coordinates */
{
  int x=0,y=0,b=0,m=0;
  char c;
  
  if (*s) {
    sscanf(s,"MB%d%c+%d:%04d,%04d",&b,&c,&m,&x,&y);
    if (px) *px=x;
    if (py) *py=y;
    if (pb) {
      if (c=='d') 
        *pb=b;
      else
        *pb=-b;
    }
    if (pm) *pm=m;
    return;
  }
/*
  if (px) *px=mousex;
  if (py) *py=mousey;
  if (pb) *pb=mouseb;
  if (pm) *pm=mousemod;*/
}


static char *dec2other(double d,int base) /* convert double to hex or binary number */
{
  int len;
  double dec,dec2;
  char *other;
  int negative=FALSE;
  
  if (d<0) {
    dec2=floor(-d);
    negative=TRUE;
  } else {
    dec2=floor(d);
  }
  len=negative?2:1;
  for(dec=dec2;dec>=base;dec/=base) len++;
  other=my_malloc(len+1);
  other[len]='\0';
  dec=dec2;
  for(len--;len>=0;len--) {
    other[len]="0123456789abcdef"[(int)(floor(dec-base*floor(dec/base)+0.5))];
    dec=floor(dec/base);
  }
  if (negative) other[0]='-';
  return other;
}


static double other2dec(char *hex,int base) /* convert hex or binary to double number */
{
  double dec;
  static char *digits="0123456789abcdef";
  char *found;
  int i,len;
  
  if (base!=2 && base !=16) {
    sprintf(string,"Cannot convert base-%d numbers",base);
    error(ERROR,string);
    return 0.;
  }
  dec=0;
  len=strlen(hex);
  for(i=0;i<len;i++) {
    dec*=base;
    found=strchr(digits,tolower(hex[i]));
    if (!found || found-digits>=base) {
      sprintf(string,"Not a base-%d number: '%s'",base,hex);
      error(ERROR,string);
      return 0.;
    }
    dec+=found-digits;
  }
  return dec;
}


//int myformat(char *dest,double num,char *format,char *sep) /* format number according to string */
//{
 // int i1,i2; /* dummy */
  //char c1; /* dummy */
/*  static char ctrl[6];
  char *found,*form;
  int pre,post,dot,len,i,j,digit,colons,dots;
  int neg=FALSE;
  double ip,fp,round;
  static char *digits="0123456789";
  
  form=format;
  if (*form=='%') { /* c-style format */
//    strcpy(ctrl,"+- #0"); /* allowed control chars for c-format */
/*    form++;
    while((found=strchr(ctrl,*form))!=NULL) {
      *found='?';
      form++;
    } 
    if (sscanf(form,"%u.%u%c%n",&i1,&i2,&c1,&i)!=3 &&
	sscanf(form,"%u.%c%n",&i2,&c1,&i)!=2 &&
	sscanf(form,".%u%c%n",&i2,&c1,&i)!=2 &&
	sscanf(form,"%u%c%n",&i2,&c1,&i)!=2) return FALSE;
    if (!strchr("feEgG",c1) || form[i]) return FALSE;
    /* seems okay, let's print */
//    sprintf(dest,format,num);
//  } else { /* basic-style format */
/*    if (num<0) {
      neg=TRUE;
      num=-num;
    }
    colons=0;
    dots=0;
    pre=0;
    post=0;
    for(form=format;*form;form++) {
      if (*form==',') {
	if (dots) return FALSE;
	colons++;
      } else if (*form=='.') {
	dots++;
      } else if (*form=='#') {
	if (dots) 
	  post++;
	else
	  pre++;
      } else {
	return FALSE;
      }
    }
    if (dots>1) return FALSE;
    len=strlen(format);
    dest[len]='\0';
    round=0.5;
    for(i=0;i<post;i++) round/=10.;
    if (fabs(num)<round) neg=FALSE;
    num+=round;
    ip=floor(num);
    fp=num-ip;
    if (fp>1 || fp<0) fp=0;
    dest[pre+colons]=format[pre+colons];
    if ((int)ip) {
      for(i=pre+colons-1;i>=0;i--) {
	if (neg && !(int)ip) {
	  neg=0;
	  dest[i]='-';
	} else {
	  if (format[i]=='#') {
	    digit=((int)ip)%10;
	    ip/=10;
	    if (((int)ip) || digit>0) 
	      dest[i]=digits[digit];
	    else
	      dest[i]=' ';
	  } else {
	    if ((int)ip)
	      dest[i]=format[i];
	    else
	      dest[i]=' ';
	  }
	}
      }
    } else {
      i=pre+colons-1;
      dest[i--]='0';
    }
    if ((neg && i<0) || ((int)ip)) {
      strcpy(dest,format);
      return TRUE;
    }
    if (neg) dest[i--]='-';
    for(;i>=0;i--) dest[i]=' ';
    for(i=pre+colons+1;i<len;i++) {
      fp*=10;
      digit=(int)fp;
      fp-=digit;
      dest[i]=digits[digit];
    }
    if (sep && sep[0] && sep[1]) {
      for(i=0;i<len;i++) {
	if (dest[i]==',') dest[i++]=sep[0];
	if (dest[i]=='.') dest[i++]=sep[1];
      }
    }
  }
  return TRUE;
}*/
int myformat (char *dest, int max, double num, char *format, char *sep)	/* format number according to string */
{
    int ret = myformat2 (dest, max, num, format, sep);
    if (ret == 0) return TRUE;
    if (ret == 1) sprintf (errorstring, "'%s' is not a valid format", format);
    if (ret == 2) sprintf (errorstring, "length of formatted string exceeds maximum of %d bytes", INBUFFLEN);
    error (ERROR, errorstring);
    return FALSE;
}

int myformat2 (char *dest, int max, double num, char *format, char *sep)	/* do the work for myformat */
{
    static char *ctrl = "+- #0";	/* allowed control chars for c-format */
    char formchar;
    char *found, *form;
    int pre, post, len, nread, digit, commas, dots, i, cr;
    int neg = FALSE;
    double ipdbl, fp, round;
    unsigned long ip;
    static char *digits = "0123456789";

    form = format;
    if (*form == '%') {
        /* c-style format */
        form++;
        while ((found = strchr (ctrl, *form)) != NULL) form++;
        if (sscanf (form, "%*u.%*u%c%n", &formchar, &nread) != 1 &&
	    sscanf (form, "%*u.%c%n", &formchar, &nread) != 1 &&
	    sscanf (form, ".%*u%c%n", &formchar, &nread) != 1 &&
	    sscanf (form, "%*u%c%n", &formchar, &nread) != 1 &&
	    sscanf (form, "%c%n", &formchar, &nread) != 1) {
            return 1;
        }
        if (!strchr ("feEgG", formchar) || form[nread]) {
            return 1;
        }
        /* seems okay, let's try to print */
        len = snprintf (dest, max, format, num);
	if (len >= max) {
	    return 2;
	}
    } else {
        /* basic-style format */

	/* make num positive and remember if it has been negative initially */
        if (num < 0) {
            neg = TRUE;
            num = fabs (num);
        }

	/* verify form of ##.###.###,## (e.g.) up front to be able to rely on this; 
	   also count various parts */
        commas = 0;
        dots = 0;
        pre = 0;
        post = 0;
        for (form = format; *form; form++) {
            if (*form == ',') {
                if (dots) {
		    /* commas in fractional part are not supported */
                    return 1; 
                }
                commas++;
            } else if (*form == '.') {
		if (dots) {
		    /* format has more than one decimal dot */
		    return 1;
		}
                dots++;
            } else if (*form == '#') {
                if (dots) {
                    post++;
                } else {
                    pre++;
                }
            } else {
		/* neither '#' nor '.' nor ',' */
                return 1;
            }
        }

	/* prepare destination */
        len = strlen (format);
        dest[len] = '\0';

	/* round to given precision; round away from zero */
        round = 0.5;
        for (i = 0; i < post; i++) {
            round /= 10.;
        }
	/* if number is below round offset, treat it as zero */
        if (num < round) {
            neg = FALSE;
	    num = 0.0;
	} else {
	    /* do the rounding away from zero */
	    num += round;
	}

	/* because we cast to long we cannot cope with numbers larger than its max */
	/* not casting to long on the other hand leads to frequent arithmetic errors */
        if (num > LONG_MAX) {
            strcpy (dest, format);
            return 0;
        }

	/* disassemble in integer and fractional part; both ip and fp will be consumed stepwise in the process */
	fp = modf(num, &ipdbl);
	ip = (unsigned long) ipdbl;
	
	/* variable cr serves as our cursor running from right to left and marks the position to be written next */
	
	/* write integer part */
	cr = pre + commas - 1;
	do {
	    if (format[cr] == '#') {
		/* get digit and reduce integer part */
		digit = ip % 10;
		ip = ip/10;
		dest[cr--] = digits[digit];
	    } else {
		/* format[cr] == ','; i.e. we do not need a new digit */
		dest[cr--] = ip ? ',' : ' ';
	    }
	} while (ip && cr >= 0);

	/* given format does not have enough room, this is an error; just copy format into dest and return */
        if ((neg && cr < 0) || ip) {
            strcpy (dest, format);
            return 0;
        }

	/* minus if appropriate */
        if (neg) {
            dest[cr--] = '-';
        }
	
	/* fill from cursor position back to start */
        while (cr >= 0) {
            dest[cr--] = ' ';
        }

	/* cursor cr now runs from left to right */
	
	/* do we need to write a fractional part ? */
	cr = pre + commas;
	if (dots) {
	    /* write decimal dot */
	    dest[cr++] = '.';
	    /* construct fractional part digit by digit */
	    while (cr < len) {
		fp *= 10;
		digit = ((unsigned long) fp) % 10;
		dest[cr++] = digits[digit];
	    }
	} else {
	    /* no fractional part needed */
	    dest[cr++] = '\0';
	}

	/* until now we used fixed separators ',' for thousands and '.' for decimal; but if
	   user has given his own separators (e.g. german or swiss style) we need to correct this */
        if (sep) {
	    if (sep[0] && sep[1]) {
		for (i = 0; i < len; i++) {
		    if (dest[i] == ',') {
			dest[i++] = sep[0];
		    }
		    if (dest[i] == '.') {
			dest[i++] = sep[1];
		    }
		}
	    } else {
		return 1;
	    }
        }
    }
    return 0;
}
static char *fromto(char *str,int from,int to) /* gives back portion of string */
/* from and to can be in the range 1...strlen(str) */
{
  int len,i;
  char *part;
  
  len=strlen(str);
  if (from>to || to<0 || from>len-1) {
    /* give back empty string */
    part=my_malloc(1);
    part[0]='\0';
  }
  else {
    if (from<=0) from=0;
    if (to>=len) to=len-1;
    part=my_malloc(sizeof(char)*(to-from+2)); /* characters and '/0' */
    for(i=from;i<=to;i++) part[i-from]=str[i]; /* copy */
    part[i-from]='\0';
  }
  return part;
}



void mywait() /* wait given number of seconds */
{
  double delay;
  
#ifdef UNIX
  struct timeval tv;
#else	
  MSG msg;
  int timerid;
#endif
  
  delay=pop(stNUMBER)->value;
  if (delay<0) delay=0.;
#ifdef UNIX
  tv.tv_sec=(int)delay;
  tv.tv_usec=(delay-(int)delay)*1000000;
  select(0,NULL,NULL,NULL,&tv);
#else /* WINDOWS */
  timerid=SetTimer(NULL,0,(int)(delay*1000),(TIMERPROC) NULL);
  GetMessage((LPMSG)&msg,NULL,WM_TIMER,WM_TIMER);
  KillTimer(NULL,timerid);
#endif
}


void mybell() /* ring ascii bell */
{
#ifdef UNIX
#ifdef BEOS	
  yi_beep();
#else
  printf("\007");
  fflush(stdout);
#endif
#else /* WINDOWS */
  Beep(1000,100);
#endif
}



void create_poke(char flag) /* create Command 'cPOKE' */
{
  struct command *cmd;
		
  if (flag=='S' || flag=='D')
    cmd=add_command(cPOKEFILE,FALSE);
  else
    cmd=add_command(cPOKE,FALSE);
  cmd->tag=flag;
}


void poke(struct command *cmd) /* poke into internals */
{
  char *dest,*s,c;
  char *sarg=NULL;
  double darg;
		
  if (cmd->tag=='s')
    sarg=pop(stSTRING)->pointer;
  else 
    darg=pop(stNUMBER)->value;
		
  dest=pop(stSTRING)->pointer;
  for(s=dest;*s;s++) *s=tolower((int)*s);
  /*
  if (!strcmp(dest,"fontheight") && !sarg) {
    fontheight=(int)darg;
#ifdef UNIX
    calc_psscale();
#endif
  }
  else if (!strcmp(dest,"font") && sarg) {
    font=my_strdup(sarg);
  } */
  if (!strcmp(dest,"dump") && sarg && !strcmp(sarg,"symbols")) {
    dump_sym();
  }
  else if (!strcmp(dest,"dump") && sarg && 
	   (!strcmp(sarg,"sub") || !strcmp(sarg,"subs") || !strcmp(sarg,"subroutine") || !strcmp(sarg,"subroutines"))) {
    dump_sub(0);
  }
  /*
  else if (!strcmp(dest,"winwidth") && !sarg) {
    winwidth=(int)darg;
    if (winwidth<1) {
      error(ERROR,"winwidth less than 1 pixel");
      return;
    }
#ifdef UNIX
    calc_psscale();
#endif
  }
  else if (!strcmp(dest,"winheight") && !sarg) {
    winheight=(int)darg;
    if (winheight<1) {
      error(ERROR,"winheight less than 1 pixel");
      return;
    }
#ifdef UNIX
    calc_psscale();
#endif
  }
  else if (!strcmp(dest,"textalign") && sarg) {
    if (!check_alignement(sarg)) return;
    strncpy(text_align,sarg,2);
  }
  else if (!strcmp(dest,"windoworigin") && sarg) {
    moveorigin(sarg);
  }*/
  else if (!strcmp(dest,"infolevel") && sarg) {
    c=tolower((int)*sarg);
    switch(c) {
    case 'd': infolevel=DEBUG;break;
    case 'n': infolevel=NOTE;break;
    case 'w': infolevel=WARNING;break;
    case 'e': infolevel=ERROR;break;
    case 'f': infolevel=FATAL;break;
    default:
      error(ERROR,"invalid infolevel");
      return;
    }
    if (infolevel>=DEBUG) {
      sprintf(string,"switching infolevel to '%c'",c);
      error(DEBUG,string);
    }
  }
  else if (!strcmp(dest,"stdout") && sarg) {
    fputs(sarg,stdout);
  }
  else if (!strcmp(dest,"read_controls") && !sarg) {
    read_controls= darg ? 1:0;
  }
  else if (dest[0]=='#') {
    error(ERROR,"don't use quotes when poking into file");
  }
  else {
    error(ERROR,"invalid poke");
  }
  return;
}


void pokefile(struct command *cmd) /* poke into file */
{
  char *sarg=NULL;
  double darg;
  int stream;
		
  if (cmd->tag=='S')
    sarg=pop(stSTRING)->pointer;
  else 
    darg=pop(stNUMBER)->value;
  stream=(int)(pop(stNUMBER)->value);
  
  if (badstream(stream,0)) return;
  
  if (!(stream_modes[stream] & smWRITE)) {
    sprintf(string,"Stream %d not open for writing",stream);
    error(ERROR,string);
    return;
  } 
  if (sarg) {
    fputs(sarg,streams[stream]);
  } else {
    if (darg<0 || darg>255) {
      error(ERROR,"stream poke out of byte range (0..255)");
      return;
    }
    fputc((int)darg,streams[stream]);
  }
}


static double peek(char *dest, YabInterface *yab) /* peek into internals */
{
  char *s;
		
  for(s=dest;*s;s++) *s=tolower((int)*s);
  /*if (!strcmp(dest,"winwidth")) return winwidth;
  else if (!strcmp(dest,"winheight")) return winheight;
  else if (!strcmp(dest,"fontheight")) return fontheight;
  else*/
  if (!strcmp(dest,"screenheight")) return LINES;
  else if (!strcmp(dest,"screenwidth")) return COLS;
  else if (!strcmp(dest, "deskbarposition")) return yi_DeskbarPosition(yab);	//deskbarposition;
  else if (!strcmp(dest, "deskbarexpanded")) return yi_DeskbarExpanded(yab);	//deskbarexpanded;
  else if (!strcmp(dest, "deskbarwidth")) return yi_DeskbarWidth(yab); 			//deskbarwidth;
  else if (!strcmp(dest, "deskbarheight")) return yi_DeskbarHeight(yab);		//deskbarheight;
  else if (!strcmp(dest, "deskbar-x")) return yi_DeskbarX(yab);					//deskbar-x;
  else if (!strcmp(dest, "deskbar-y")) return yi_DeskbarY(yab);					//deskbar-y;
  else if (!strcmp(dest, "desktopwidth")) return yi_DesktopWidth(yab);			//desktopwidth;
  else if (!strcmp(dest, "desktopheight")) return yi_DesktopHeight(yab);		//desktopheight;
  else if (!strcmp(dest, "scrollbarwidth")) return yi_ScrollbarWidth(yab);		// scrollbarwidth
  else if (!strcmp(dest, "menuheight")) return yi_MenuHeight(yab);				// menuheight
  else if (!strcmp(dest, "tabheight")) return yi_TabHeight(yab);				// tabheight
  else if (!strcmp(dest,"argument") || !strcmp(dest,"arguments") ) return yabargc;	// number of arguments
  else if (!strcmp(dest,"version")) return strtod(VERSION,NULL);
  else if (!strcmp(dest,"error")) return errorcode;
  else if (!strcmp(dest,"read_controls")) return read_controls;
  else if (!strcmp(dest,"isbound")) return is_bound;
  else if (!strcmp(dest, "loudness")) return yi_LoudnessGet(yab);	//added new command Lorglas
   
  else if (dest[0]=='#') {
    error(ERROR,"don't use quotes when peeking into a file");
    return 0;
  }
 
  sprintf(stderr, "PEEK is set to %s\n", dest);
  error(ERROR,"invalid peek");
  return 0;
}


static int peekfile(int stream) /* read a byte from stream */
{
  if (stream && badstream(stream,0)) return 0;
  if (stream && !(stream_modes[stream] & smREAD | smREADWRITE)) {
    sprintf(string,"stream %d not open for reading",stream);
    error(ERROR,string);
    return 0;
  } 
  return fgetc(stream?streams[stream]:stdin);
}


static char *peek2(char *dest,struct command *curr) /* peek into internals */
{
  char *s;
  
		
  for(s=dest;*s;s++) *s=tolower((int)*s);
  if (!strcmp(dest,"infolevel")) {
    if (infolevel==DEBUG) return my_strdup("debug");
    else if (infolevel==NOTE) return my_strdup("note");
    else if (infolevel==WARNING) return my_strdup("warning");
    else if (infolevel==ERROR) return my_strdup("error");
    else if (infolevel==FATAL) return my_strdup("fatal");
    else return my_strdup("unkown");
  }
  /*
  else if (!strcmp(dest,"textalign")) return my_strdup(text_align);
  else if (!strcmp(dest,"windoworigin")) return my_strdup(winorigin);*/
  else if (!strcmp(dest,"error")) return my_strdup(errorstring);
  else if (!strcmp(dest,"library")) return my_strdup(curr->lib->s);
  else if (!strcmp(dest,"os")) {
#ifdef HAIKU
    return my_strdup("Haiku");
#else
    return my_strdup("BeOS");
#endif
  }
  else if (!strcmp(dest,"directory")) return my_strdup(appdirectory);
  else if (!strcmp(dest,"refsreceived")) return my_strdup(refsRec);
  /* else if (!strcmp(dest,"font")) return my_strdup(font); */
  else if (!strcmp(dest,"argument") || !strcmp(dest,"arguments")) {
    if (yabargc>0) {
      s=yabargv[0];
      yabargc--;
      yabargv++;
    }
    else {
      s="";
    }
    return my_strdup(s);
  }
  else {
    error(ERROR,"invalid peek");
  }
  return my_strdup("");
}


static char *peek3(char *dest,char *cont) /* peek into internals */
{
  char *s;
		
  for(s=dest;*s;s++) *s=tolower((int)*s);
  if (!strcmp(dest,"env") || !strcmp(dest,"environment")) {
    return my_strdup(getenv(cont));
  } else {
    error(ERROR,"invalid peek");
  }
  return my_strdup("");
}


void create_exception(int flag) /* create command 'exception' */
{
  struct command *cmd;  
  
  cmd=add_command(cEXCEPTION,FALSE);
  cmd->args=flag;
}


void exception(struct command *cmd) /* change handling of exceptions */
{
  if (cmd->args) {
    signal(SIGINT,signal_handler); /* enable keyboard interrupt */
#ifdef SIGHUP
    signal(SIGHUP,signal_handler);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT,signal_handler);
#endif
#ifdef SIGABRT
    signal(SIGABRT,signal_handler);
#endif
#ifdef SIGTERM
    signal(SIGTERM,signal_handler);
#endif
  }
  else {
    signal(SIGINT,SIG_IGN); /* ignore keyboard interrupt */
#ifdef SIGHUP
    signal(SIGHUP,SIG_IGN);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT,SIG_IGN);
#endif
#ifdef SIGABRT
    signal(SIGABRT,SIG_IGN);
#endif
#ifdef SIGTERM
    signal(SIGTERM,SIG_IGN);
#endif
  }
  return;
}


void create_restore(char *label) /* create command 'restore' */
{
  struct command *c;
  
  c=add_command(cRESTORE,FALSE);
  c->pointer=my_strdup(label);
}


void restore(struct command *cmd) /* reset data pointer to given label */
{
  struct command *label;
  struct command **datapointer;
  
  datapointer=&(cmd->lib->datapointer);
  if (cmd->type==cRESTORE) { /* first time; got to search the label */
    if (*((char *)cmd->pointer)=='\0') {
      /* no label, restore to first command */
      label=cmd->lib->firstdata;
    } else {
      label=search_label(cmd->pointer,smLABEL|smGLOBAL);
      if (!label) {
	/* did not find label */
	sprintf(string,"can't find label '%s'",(char *)cmd->pointer);
	error(ERROR,string);
	return;
      }
    }
    *datapointer=label;
    if (lastdata) {
      while((*datapointer)->type!=cDATA && (*datapointer)!=cmdhead) {
	*datapointer=(*datapointer)->next;
      }
    }
    cmd->pointer=*datapointer;
    cmd->type=cQRESTORE;
  } else {
    *datapointer=cmd->pointer;
  }
  return;
}


void create_dbldata(double value)  /* create command dbldata */
{
  struct command *c;
  
  c=add_command(cDATA,FALSE);
  c->pointer=my_malloc(sizeof(double));
  if (lastdata) lastdata->nextassoc=c;
  lastdata=c;
  *((double *)c->pointer)=value;
  c->tag='d'; /* double value */
}


void create_strdata(char *value)  /* create command strdata */
{
  struct command *c;
  
  c=add_command(cDATA,FALSE);
  if (lastdata) lastdata->nextassoc=c;
  lastdata=c;
  c->pointer=my_strdup(value);
  c->tag='s'; /* string value */
}


void create_readdata(char type) /* create command readdata */
{
  struct command *cmd;
  
  cmd=add_command(cREADDATA,FALSE);
  cmd->tag=type;
}


void readdata(struct command *cmd) /* read data items */
{
  struct stackentry *read;
  char type;
  struct command **datapointer;
  
  datapointer=&(cmd->lib->datapointer);
  type=cmd->tag;
  while(*datapointer && ((*datapointer)->type!=cDATA || cmd->lib!=(*datapointer)->lib)) {
    *datapointer=(*datapointer)->nextassoc;
  }
  if (!*datapointer) {
    error(ERROR,"run out of data items");
    return;
  }
  if (type!=(*datapointer)->tag) {
    error(ERROR,"type of READ and DATA don't match");
    return;
  }
  read=push();
  if (type=='d') { /* read a double value */
    read->type=stNUMBER;
    read->value= *((double *)(*datapointer)->pointer);}
  else {
    read->type=stSTRING;
    read->pointer=my_strdup((*datapointer)->pointer);
  }
  *datapointer=(*datapointer)->nextassoc; /* next item */
}


void create_dblrelop(char c) /* create command dblrelop */ 
{
  int type;
  
  switch(c) {
  case '=': type=cEQ;break;
  case '!': type=cNE;break;
  case '<': type=cLT;break;
  case '{': type=cLE;break;
  case '>': type=cGT;break;
  case '}': type=cGE;break;
  }
  add_command(type,FALSE);
}


void dblrelop(struct command *type)  /* compare topmost double-values */
{
  double a,b,c;
  struct stackentry *result;
  
  b=pop(stNUMBER)->value;
  a=pop(stNUMBER)->value;
  switch(current->type) {
  case cEQ:c=(a==b);break; //added new command Lorglas 2022.01.26
  case cNE:c=(a!=b);break; //added new command Lorglas 2022.01.26
  case cLE:c=(a<=b);break;
  case cLT:c=(a<b);break;
  case cGE:c=(a>=b);break;
  case cGT:c=(a>b);break;
  }
  result=push();
  result->value=c;
  result->type=stNUMBER;
}    


void create_strrelop(char c) /* create command strrelop */ 
{
  int type;
  
  switch(c) {
  case '=': type=cSTREQ;break;
  case '!': type=cSTRNE;break;
  case '<': type=cSTRLT;break;
  case '{': type=cSTRLE;break;
  case '>': type=cSTRGT;break;
  case '}': type=cSTRGE;break;
  }
  add_command(type,FALSE);
}


void strrelop(struct command *type)  /* compare topmost string-values */
{
  char *a,*b;
  double c;
  struct stackentry *result;
  
  b=pop(stSTRING)->pointer;
  a=pop(stSTRING)->pointer;
  switch(current->type) {
  case cSTREQ:c=(strcmp(a,b)==0);break; //added new command Lorglas 2022.01.26
  case cSTRNE:c=(strcmp(a,b)!=0);break; //added new command Lorglas 2022.01.26
  case cSTRLT:c=(strcmp(a,b)<0);break;
  case cSTRLE:c=(strcmp(a,b)<=0);break;
  case cSTRGT:c=(strcmp(a,b)>0);break;
  case cSTRGE:c=(strcmp(a,b)>=0);break;
  }
  result=push();
  result->value=c;
  result->type=stNUMBER;
}    

void switch_compare(void) /* compare topmost values for switch statement */
{
  struct stackentry *result,*first,*second;
  double r=0.;

  first=pop(stANY);
  second=stackhead->prev;
  if ((second->type==stSWITCH_STRING || second->type==stSTRING) && first->type==stSTRING) {
    if (second->type==stSWITCH_STRING) 
      r=1.;
    else 
      r=(strcmp(first->pointer,second->pointer)==0)?1.:0.;
  } else if ((second->type==stSWITCH_NUMBER || second->type==stNUMBER) && first->type==stNUMBER) {
    if (second->type==stSWITCH_NUMBER)
      r=1.;
    else
      r=(first->value==second->value);
  } else {
    error(ERROR,"Cannot mix strings and numbers in a single switch statement");
  }
  result=push();
  result->type=stNUMBER;
  result->value=r;
}


void logical_shortcut(struct command *type)  /* shortcut and/or if possible */
{
  struct stackentry *result;
  double is;

  is=stackhead->prev->value;
  if ((type->type==cORSHORT && is!=0) || (type->type==cANDSHORT && is==0)) {
    result=push();
    error(DEBUG,"logical shortcut taken");
    result->type=stNUMBER;
    result->value=is;
  } else {
    current=current->next;
  }
}


void create_boole(char c) /* create command boole */ 
{
  int type;
  
  switch(c) {
  case '|': type=cOR;break;
  case '&': type=cAND;break;
  case '!': type=cNOT;break;
  }
  add_command(type,FALSE);
}


void boole(struct command *type)  /* perform and/or/not */
{
  int a,b,c;
  struct stackentry *result;
  
  a=(int)pop(stNUMBER)->value;
  if (current->type==cNOT) 
    c=!a;
  else {
    b=(int)pop(stNUMBER)->value;
    if (current->type==cAND)
      c=a&&b;
    else
      c=a||b;
  }
  result=push();
  result->value=c;
  result->type=stNUMBER;
}

char* replaceWordInText(char *text,  char *oldWord,  char *newWord) {
	char *newString;
	int i = 0, cnt = 0; int u=1;
   	int len1 = strlen(newWord);
   	int len2 = strlen(oldWord);     
	
   for (i = 0; text[i] != '\0'; i++) 
   {
      if (strstr(&text[i], oldWord) == &text[i]) 
      {
         cnt++;
         i += len2 - 1;
      }
   }
   
  newString = (char *)malloc(i + cnt * (len1 - len2) + 1);
   i = 0;
   while (*text) {
   	
        	 if (strstr(text, oldWord) == text) 
        	 {
        
        	 strcpy(&newString[i], newWord);
         	i += len1;
        	 text += len2;   
        	
      		}
     		else
     		{
     		 newString[i++] = *text++;
     		}
   }
   //printf("New String: %s\n", newString);
    //return newString; 
	return my_strdup(newString); 
     	
 }

/***********************************************************
php2c++ Version 20011030
Copyright 2001, Forrest J. Cavalier III d-b-a Mib Software

License, documentation, and original and updated source code
are available from http://www.mibsoftware.com/

Outputs are not derived works of this software.
***********************************************************/

/***********************************************************
Modified
2016, Thomas PASQUIER
Computer Laboratory
University of Cambridge
***********************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef WIN32
#include <malloc.h>
#define strncasecmp _strnicmp
#endif
FILE *fileMain;
FILE *fileFunctions;
FILE *fileHeader;
FILE *fileClass;

char *aszOut;

FILE *fileOutMain;
FILE *fileOutFunctions;
FILE *fileOutHeader;
FILE *fileOutClass;

int bIncludeOnce = 0;

int nLINE;
int isThis;
char *aszfname = 0;

static int inVdecl = 0; /* Determines variable declaration writes */

void ProcessFile(const char *fname);

char *astrn0cat(char **ppasz,const char *src,int len);
char *astrn0cpy(char **ppasz,const char *src,int len);
char *astrcat(char **ppasz,const char *src);
char *astrcpy(char **ppasz,const char *src);
char *afgets(char **ppasz,FILE *f);
char *astrfree(char **ppasz);

unsigned int countbl(const char *sptr);
unsigned int countid(const char *sptr);
unsigned int counttoch(const char *sptr,char ch);

#define MVARPREFIX "_s_"
#define VARPREFIX "_s_"

void CPPnstr(const char *str,int len)
{
    if (fileOutHeader) {
       fwrite(str,1,len,fileOutHeader);
    }else if (fileOutMain) {
	     fwrite(str,1,len,fileOutMain);
    }
    if (fileOutFunctions) {
	     fwrite(str,1,len,fileOutFunctions);
    }

    if (fileOutClass) {
	     fwrite(str,1,len,fileOutClass);
    }
    if (aszOut) {
	     astrn0cat(&aszOut,str,len);
    }
}

void CPPstr(const char *str)
{
    CPPnstr(str,strlen(str));
}
void CPPchar(char ch)
{
    CPPnstr(&ch,1);
}

void CPPline()
{
    char buf[128];
    sprintf(buf,"\n#line %d \"",nLINE);
    CPPstr(buf);
    CPPstr(aszfname);
    CPPstr("\"\n");
}


void showhtml(const char *ptr,int len)
{
    int ateol = 0;
    if (!len) {
	return;
    }
    CPPstr("    showhtml(\"");
    /* Generate a C string, insert appropriate backslash escapes */
    while(len > 0) {
	if ((*ptr == '\\')||(*ptr == '\"')) {
	    /* Precede by \\ */
	    CPPchar('\\');
	    CPPchar(*ptr);
	    len--;
	    ptr++;
	} else if (*ptr == '\t') {
	    CPPstr("\\t");
	    len--;
	    ptr++;
	} else if (*ptr == '\r') {
	    CPPstr("\\r");
	    len--;
	    ptr++;
	} else if (*ptr == '\n') {
	    /* also keep track of end of line */
	    CPPstr("\\n");
	    len--;
	    ptr++;
	    ateol = 1;
	} else {
	    CPPchar(*ptr);
	    len--;
	    ptr++;
	}
    }
    CPPstr("\");");
    if (ateol) {
	CPPstr("\n");
    }
} /* showhtml */

int (*handler)(const char *sptr,int len);
#define SetHandler(h) handler = h

int ShouldRewriteSubscript(char **ppasz,const char *ptr)
{
    ptr++;
    ptr += countbl(ptr);
    astrn0cpy(ppasz,ptr,counttoch(ptr,']'));
    char *wptr = *ppasz + strlen(*ppasz);
    while((wptr > *ppasz)&& countbl(wptr-1)) {
	wptr--;
	*wptr = '\0';
    }
    if (!strcmp(*ppasz,"")) {
	astrcpy(ppasz,"_APPEND_");
	return 1;
    }
    /* If there are only integers, spaces, and integer operators,
       leave it as is.
    */
    if (strspn(*ppasz,"01234567890 \t*-+/")==strlen(*ppasz)) {
	return 0;
    }
    /* If there are only alphanumeric, enclose in "" */
    if (countid(*ppasz)==strlen(*ppasz)) {
	char *aszret =0;
	astrcat(&aszret,"\"");
	astrcat(&aszret,*ppasz);
	astrcat(&aszret,"\"");
	astrfree(ppasz);
	*ppasz = aszret;
	return 1;
    }
    if (**ppasz == '$') {
	char *aszret =0;
	astrcat(&aszret,VARPREFIX);
	astrcat(&aszret,*ppasz+1);
	astrfree(ppasz);
	*ppasz = aszret;
	return 1;
    }
    return 0;

} /* ShouldRewriteSubscript */

int transSubscript(const char *sptr,int len)
{
if (handler != transSubscript) {
    if (*sptr == '[') {
    /* Limitation: unnested subscript must appear on one line */
	const char *ptr = sptr;
	char *asz = 0;
	if (ShouldRewriteSubscript(&asz,ptr)) {
	    CPPchar('[');
	    CPPstr(asz);
	    CPPchar(']');
	    ptr += counttoch(ptr,']');
	    if (*ptr) {
		ptr++;
	    }
	} else {
	    CPPchar(*ptr);
	    ptr++;
	}
	astrfree(&asz);
	return ptr-sptr;
    }
    return 0;
}
return 0;
} /* transSubscript */

/**************************************************************/
static int nestlevel = 0; /* Determines inside/outside functions */
static int Pnestlevel = 0; /* For Adding paren in while() if()  */
/**************************************************************/
int transcomment(const char *sptr,int len)
{ /* Not re-entrant, but shouldn't need to be */
static int toEOL;

if (handler != transcomment) {
    if ((len >= 2) &&!strncmp(sptr,"/*",2)) {
	toEOL = 0;
	SetHandler(transcomment);
	CPPstr("/*");
	return 2;
    } else if (*sptr == '#') {
	CPPstr("//");
	toEOL = 1;
	SetHandler(transcomment);
	return 1;
    } else if ((len >= 2)&& !strncmp(sptr,"//",2)) {
	toEOL = 1;
	SetHandler(transcomment);
	CPPstr("//");
	return 2;
    }
    return 0;
}

const char *ptr = sptr;

while(ptr < sptr + len) {
    if (toEOL) {
	if (*ptr == '\n') {
	    CPPchar(*ptr);
	    ptr++;
	    SetHandler(0); /* return to old handler */
	    return ptr - sptr;
	}
    } else {
	if ((ptr <= sptr+len-2) && !strncmp(ptr,"*/",2)) {
	    CPPstr("*/");
	    SetHandler(0); /* return to old handler */
	    return ptr+2 - sptr;
	}
    }
    CPPchar(*ptr);
    ptr++;
}
return ptr - sptr;

} /* transcomment */
/**************************************************************/


/**************************************************************/
int transhint(const char *sptr,int len)
{ /* Not re-entrant */
  if (handler != transhint) {
    if (!strncmp(sptr,"/*int*/",7)||
      !strncmp(sptr,"/*double*/",8)||
      !strncmp(sptr,"/*bool*/",8)||
      !strncmp(sptr,"/*void*/",8)||
      !strncmp(sptr,"/*string*/",10)||
      !strncmp(sptr,"/*mixed*/",9)||
      !strncmp(sptr,"/*array*/",9)||
      !strncmp(sptr,"/*string_array*/",16)) {
        FILE* tmp=fileOutMain;
        if (nestlevel == 0) {
          inVdecl = 1;
          /* goes in header, not main */
          fileOutHeader = fileHeader;
          fileOutMain=0;
          CPPstr("static ");
        }
        CPPnstr(sptr+2,counttoch(sptr+2,'*'));
        CPPstr(" ");
        fileOutMain=tmp;
        return 4 + counttoch(sptr+2,'*');
      } else if ((len >= 6) &&!strncmp(sptr,"/*c++:",6)) {
        SetHandler(transhint);
        return 6;
      } else if ((len >= 8) &&!strncmp(sptr,"/*php:*/",8)) {
        CPPstr("/*php:"); /* 20010823 Fix */
        SetHandler(transcomment); // Ignore until /**/
        return 8;
      }
      return 0;
    }
    const char *ptr = sptr;

    while(ptr < sptr + len) {
      if ((ptr <= sptr+len-2) && !strncmp(ptr,"*/",2)) {
        if (inVdecl) {
          CPPstr(" ");
        }
        SetHandler(0); /* return to old handler */
        return ptr+2 - sptr;
      }
      if (*ptr == '$') {
        char *asz = 0;
        int len = countid(ptr+1)+1;
        astrn0cpy(&asz,ptr,len);
        CPPstr(VARPREFIX);
        ptr++;
        CPPnstr(ptr,len-1);
        ptr += len-1;
        astrfree(&asz);
      } else if (inVdecl && (*ptr == ';')) {
        inVdecl = 0;
        if (fileOutHeader) {
          fputs(";\n",fileOutHeader);
          CPPline();
          fileOutHeader = 0;
        }
      } else {
        CPPchar(*ptr);
        ptr++;
      }
    }
    return ptr - sptr;
} /* transhint */
/**************************************************************/





/**************************************************************/
int transNumb(const char *sptr,int len)
{
if (handler != transNumb) {
    if (isdigit(*sptr)) {
	/* We do this so that we can catch the '.'
	   which is part of a number, not the concat op.
	*/
	int c = strspn(sptr,"0123456789");
	if (sptr[c] == '.') {
	    c += 1 + strspn(sptr+c+1,"0123456789");
	}
	/* exponents are handled by passthrough */
	CPPnstr(sptr,c);
	return c;
    }
}
return 0;
} /* transNumb */
/**************************************************************/

/**************************************************************/
int transAT(const char *sptr,int len)
{

if (handler != transAT) {
    if (*sptr == '@') {
	CPPstr("/*@*/"); /* OK for translation, not operation */
	return 1;
    }
}
return 0;
} /* transAT */
/**************************************************************/

/**************************************************************/

int transglobal(const char *sptr,int len)
{
const char *ptr = sptr;

if (handler != transglobal) {
    len = countid(ptr);
    if ((len == 6)&&!strncasecmp(ptr,"global",len)) {
        /* Translation */
        CPPstr("/*global*/");
	return 6;
    }
}
return 0;

} /* transglobal */
/**************************************************************/



/**************************************************************/
int transDquote(const char *sptr,int len)
{ /* Not re-entrant */
static int multiline;

if (handler != transDquote) {
    if (*sptr == '\"') {
	 SetHandler(transDquote);
	 multiline = 0;
	 CPPchar(*sptr);
	 return 1;
    }
    return 0;
}

const char *ptr = sptr;

while(ptr < sptr + len) {
    if (*ptr == '\\') {
	/* Assume at least two characters */
	if (ptr[1] != '$') {
	    CPPchar(*ptr);
	}
	ptr++;
	CPPchar(*ptr);ptr++;
    } else if (*ptr == '$') {
	/* $ inside a PHP string is interpreted */
	int c;
	ptr++;
	CPPstr("\" + strval(");
	CPPstr(VARPREFIX);
	char *asz = 0;
	c = countid(ptr);
	while (1) {
	    if ((ptr[c] == '-')&&(ptr[c+1] == '>')) {
		CPPnstr(ptr,c);
		ptr += c+2;
		CPPchar('.');
		CPPstr(MVARPREFIX);
		c = countid(ptr);
	    } else if ((ptr[c] == '[')&& strchr(ptr+c,']')) {
		if (ShouldRewriteSubscript(&asz,ptr+c)) {
		    CPPnstr(ptr,c);
		    CPPchar('[');
		    CPPstr(asz);
		    CPPchar(']');
		    ptr += counttoch(ptr,']');
		    if (*ptr) {
			ptr++;
		    }
		    c = 0;
		} else {
		    c = strchr(ptr+c,']')+1 - ptr;
		}
	    } else {
		break;
	    }
	}
	if (c) {
	    CPPnstr(ptr,c);
	    ptr += c;
	}
	CPPstr(") + \"");
    } else if (*ptr == '\"') {
	CPPchar('\"');ptr++;
	SetHandler(0);
	return ptr - sptr;
    } else if (*ptr == '\n') {
	CPPstr("\\n");ptr++;
	if (!multiline) {
	    fprintf(stderr,"Warning: multiline string\n");
	}
	multiline++;
    } else {
	CPPchar(*ptr);ptr++;
    }
}
return ptr - sptr;

} /* transDquote */
/**************************************************************/



/**************************************************************/
int transSquote(const char *sptr,int len)
{ /* Not re-entrant, but shouldn't need to be */
static int multiline;

if (handler != transSquote) {
    if (*sptr == '\'') {
        SetHandler(transSquote);
	multiline =0;
        CPPchar('\"');
        return 1;
    }
    return 0;
}

const char *ptr = sptr;

/* PHP single-quote string.  \\ \' are the only processed escapes.
   Enclose in C++ '"'
*/
while(ptr < sptr + len) {
    if ((*ptr == '\\')&&(len>=2)) {
	if ((ptr[1] == '\'')|| (ptr[1] == '\\')) {
	    CPPchar(*ptr);ptr++;
	    CPPchar(*ptr);ptr++;
	} else {
	    CPPchar('\\');ptr++;
	}
    } else if (*ptr == '\'') {
	CPPchar('\"');ptr++;
	SetHandler(0);
	return ptr - sptr;
    } else if (*ptr == '\n') {
	CPPstr("\\n");ptr++;
	if (!multiline) {
	    fprintf(stderr,"Warning: multiline string starting %d\n",
		nLINE);
	}
	multiline++;
    } else {
	CPPchar(*ptr);ptr++;
    }
}
return ptr - sptr;

} /* transSquote */
/**************************************************************/



/**************************************************************/
int transMember(const char *sptr,int len)
{
if (handler != transMember) {
    if ((len >= 2) && (*sptr == '-')&&(sptr[1]=='>')) {
	sptr += 2;
	if (isThis) {
	    CPPstr("->");
	} else {
	    CPPstr(".");
	}
	int len = countid(sptr);
	if (sptr[len] != '(') { /* not a function call */
	    CPPstr(MVARPREFIX);
	}
	return 2;
    }
}
return 0;
} /* transMember */
/**************************************************************/


/**************************************************************/
int transDot(const char *sptr,int len)
{
if (handler != transDot) {
    if (*sptr == '.') {
	/* Assume a concatenation operator if there are no digits
	   following. Preceding '.' was taken care of in transNumb
	 */
	if (isdigit(sptr[1])) { /* A digit following */
	    CPPchar('.'); /* Probably a decimal point */
	} else { /* Must be an operator */
	    CPPchar('+');
	}
	return 1;
    }
}
return 0;
} /* transDot */
/**************************************************************/


/**************************************************************/
int transVar(const char *sptr,int len)
{
if (handler != transVar) {
    if (*sptr == '$') {
	const char *ptr = sptr;
	int len = countid(ptr+1)+1;
	isThis = 0;
	if ((len == 5)&&!strncmp(ptr,"$this",5)) {
	    CPPstr("this");
	    isThis = 1;
	    return transMember(sptr+5,len-5)+5;
	}
	CPPstr(VARPREFIX);
	ptr++;
	CPPnstr(ptr,len-1);
	ptr += len-1;
        return ptr-sptr;
    }
}
return 0;

} /* transVar */

static int bAddingParen = 0;
static int bCond = 0;
static int inFdecl = 0; /* Determines function header writes */
static int inCdecl = 0; /* Determines class declaration writes */

int transReserved(const char *sptr,int len)
{
if (handler != transReserved) {
    const char *ptr = sptr;
    if (isalpha(*ptr)||(*ptr == '_')) {
	/* identifier, reserved word, etc */
	len = countid(ptr);
	if ((len == 6)&&!strncasecmp(ptr,"elseif",len)) {
	    CPPstr("else if");
	    bCond = 1;
	    Pnestlevel = 0;
	} else if ((len == 5)&&!strncasecmp(ptr,"while",len)) {
	    CPPstr("while");
	    bCond = 1;
	    Pnestlevel = 0;
	} else if ((len == 2)&&!strncasecmp(ptr,"if",len)) {
	    bCond = 1;
	    Pnestlevel = 0;
	    CPPstr("if");
	} else if ((len == 2)&&!strncasecmp(ptr,"or",len)) {
	    CPPstr("/*or*/||"); /* OK for lexical scan only */
	} else if ((len == 3)&&!strncasecmp(ptr,"and",len)) {
	    CPPstr("/*and*/&&"); /* OK for lexical scan only */
	} else if ((len == 5)&&!strncasecmp(ptr,"print",len)) {
	    /* a language construct. Ensure in parentheses. */
	    CPPstr("echo(");
	    bAddingParen = 1;
	} else if ((len == 4)&&!strncasecmp(ptr,"echo",len)) {
	    /* a language construct. Ensure in parentheses. */
	    CPPstr("echo(");
	    bAddingParen = 1;
	} else if ((len == 7)&&!strncasecmp(ptr,"require",len)) {
	    CPPstr("//require");
	    astrcpy(&aszOut,"");
	} else if ((len == 7)&&!strncasecmp(ptr,"include",len)) {
	    CPPstr("//include");
	    astrcpy(&aszOut,"");
	} else if ((len == 12)&&!strncasecmp(ptr,"require_once",len)) {
	    bIncludeOnce = 1;
	    CPPstr("//require_once");
	    astrcpy(&aszOut,"");
	} else if ((len == 12)&&!strncasecmp(ptr,"include_once",len)) {
	    bIncludeOnce = 1;
	    CPPstr("//include_once");
	    astrcpy(&aszOut,"");
	} else if ((len == 3)&&!strncasecmp(ptr,"var",len)) {
	    if (inCdecl) {
		inVdecl = 1;
	    }
	} else if ((len == 5)&&!strncasecmp(ptr,"class",len)) {
	    inCdecl =1;
	    fileOutMain = 0;
	    fileOutClass = fileClass;
	    CPPline();
	    CPPstr("class");
	} else if ((len == 8)&&!strncasecmp(ptr,"function",len)) {
	    if (!inCdecl && (nestlevel==0)) {
		inFdecl = 1;
		fileOutHeader = fileHeader;
		fileOutFunctions = fileFunctions;
		fileOutMain = 0;
		if (fileHeader||fileFunctions) {
		    CPPline();
		}
	    }

	} else {
	    CPPnstr(ptr,len);
	}
	ptr += len;
	return ptr - sptr;
    }
    return 0;
}
return 0;
} /* transReserved */
/**************************************************************/


/**************************************************************/
int transBrace(const char *sptr,int len)
{
if (handler != transBrace) {
    if (*sptr == '}') {
	nestlevel--;
	CPPchar(*sptr);
	if (nestlevel < 0) {
	    fprintf(stderr,"extra '}'\n");
	    nestlevel = 0;
	}
	if (nestlevel == 0) {
	    fileOutMain = fileMain;
	    CPPline();
	    if (fileOutFunctions) {
		fputs("\n",fileOutFunctions);
		fileOutFunctions = 0;
	    }
	    if (fileOutClass) {
		fputs(";\n",fileOutClass);
		fileOutClass = 0;
	    }
    	    if (inCdecl) { /* 20010823 Fix */
		inCdecl = 0;
	    }

	}
	return 1;
    } else if (*sptr == '{') {
	nestlevel++;
	if (inFdecl) {
	    inFdecl = 0;
	    if (fileOutHeader) {
		fputs(";\n",fileOutHeader);
		CPPline();
	    }
	    fileOutHeader = 0;
	}
	CPPchar(*sptr);
	if (inCdecl && (nestlevel==1)) {
	    CPPstr("public:");
	}
	return 1;
    }

    return 0;
}
return 0;
} /* transBrace */
/**************************************************************/

/**************************************************************/
int transParen(const char *sptr,int len)
{
if (handler != transParen) {
    if (*sptr == ')') {
	Pnestlevel--;
	CPPchar(*sptr);
	if (Pnestlevel < 0) {
	    fprintf(stderr,"extra ')'\n");
	    Pnestlevel = 0;
	}
	if (Pnestlevel == 0) {
	    if (bCond) {
		CPPchar(')');
		bCond = 0;
	    }
	}
	return 1;
    } else if (*sptr == '(') {
	Pnestlevel++;
	CPPchar(*sptr);
	if ((Pnestlevel == 1)&&bCond) {
	    CPPstr("!!(");
	}
	return 1;
    }

    return 0;
}
return 0;
} /* transParen */
/**************************************************************/


/**************************************************************/
int transSemi(const char *sptr,int len)
{
if (handler != transSemi) {
    if (*sptr == ';') {
	if (bAddingParen) {
	    CPPchar(')');
	    bAddingParen = 0;
	}
	if (inVdecl) {
	    inVdecl = 0;
	    if (fileOutHeader) {
		fputs(";\n",fileOutHeader);
		CPPline();
	    }
	    fileOutHeader = 0;
	}
	if (aszOut) { /* Include, Require */
	    ProcessFile(0);
	    CPPline();
	}
	CPPchar(';');
	return 1;
    }
}
return 0;
} /* transSemi */
/**************************************************************/


/**************************************************************/
int transEqual(const char *sptr,int len)
{ /* Just to catch inVdecl, and strip initializers inFdecl */
if (handler != transEqual) {
    if (*sptr == '=') {
	if (inVdecl) {
	    inVdecl = 0;
	    if (fileOutHeader) {
		fputs(";\n",fileOutHeader);
		CPPline();
	    }
	    if (inCdecl && fileOutClass) {
		CPPstr(";//"); /* initializing value not checked */
	    }
	    fileOutHeader = 0;
	}
	if (inFdecl) {
	    if (fileOutFunctions) {
		/* Don't write initializers at function body*/
		CPPstr("/*");
		int i = 0;
		while(i < len) {
		    if (sptr[i] == ',') {
			break;
		    }
		    if (sptr[i] == ')') {
			break;
		    }
		    i++;
		}
		CPPnstr(sptr,i);
		CPPstr("*/");
		return i;
	    }
	}
	CPPchar('=');
	return 1;
    }
    return 0;
}
return 0;
} /* transEqual */
/**************************************************************/


/**************************************************************/
int (*translators[])(const char *sptr,int len) = {
    transhint,       // '/*c++:', et al
    transcomment,    // /* // #
    transSquote,     // '
    transDquote,     // "
    transNumb,	     // 0-9 or 0-9.0-9
    transVar,        // $
    transDot,        // .
    transSubscript,  // []
    transMember,     // ->
    transglobal,     // global
    transReserved,   // some reserved words
    transBrace,	     // { or }
    transParen,	     // ( or )
    transSemi,	     // ;
    transEqual,	     // =
    transAT,	     // @
    0
};


void transphp(const char *sptr,int len)
{
    const char *ptr = sptr;

    while(ptr < sptr+len) {
	if (handler) { /* Existing handler gets control */
	    ptr += (*handler)(ptr,sptr+len-ptr);
	} else {
	    int i = 0;
	    while(translators[i]) {
		/* Let each handler check if they want it */
		int cnt = (*(translators[i]))(ptr,sptr+len-ptr);

		if (cnt || handler) {
		    ptr += cnt;
		    break;
		}
		i++;
	    }
	    if (!translators[i]) { /* No handlers. Copy */
		CPPchar(*ptr);
		ptr++;
	    }
	}
    }
} /* transphp */
/**************************************************************/


char *aszSeen;
/**************************************************************/
void ProcessFile(const char *fname)
{
    char *ptr;
    char *ptr2;
    if (!fname) {
	fname = aszOut;
	ptr = strchr(aszOut,'\"');
	if (!ptr || !strchr(ptr+1,'\"')) {
	    fprintf(stderr,"can't include %s\n",aszOut);
	    astrfree(&aszOut);
	    return;
	}
	ptr++;
	*strchr(ptr,'\"') = '\0';
	fname = ptr;
    }
    if (bIncludeOnce) {
	bIncludeOnce = 0;
	ptr2 = aszSeen;
	while(ptr2 && (ptr2 = strstr(ptr2,fname))) {
	    if ((ptr2[-1] <= ' ')&&
		(ptr2[strlen(fname)] <= ' ')) {
	    astrfree(&aszOut);
		return; /* Already included */
	    }
	    ptr2 += strlen(fname);
	}
    }
    astrcat(&aszSeen," ");
    astrcat(&aszSeen,fname);

    FILE *f = fopen(fname,"r");
    if (!f) {
	fprintf(stderr,"can't open %s\n",fname);
	astrfree(&aszOut);
	return;
    }
    char *oldfname = aszfname;
    aszfname = 0;
    astrcpy(&aszfname,fname);
    astrfree(&aszOut);

    char *asz = 0;
    int bPHPmode = 0;
    int sline = nLINE;
    nLINE =0;
    int startnest = nestlevel;
    while(afgets(&asz,f)) {
	nLINE++;
	if (aszfname) {
	    //CPPline();  /* for #line directives everywhere */
	}

	/* a PHP source file is made up of HTML and blocks of
	   PHP enclosed within '<?php' and '?>'  This loop
	   locates the PHP escape tags and calls the appropriate
	   handlers.
	 */
	ptr = asz;
	while(*ptr) {
	    if (bPHPmode) { /* php mode */
		ptr2 = strstr(ptr,"?>");
		if (ptr2) {
		    transphp(ptr,ptr2-ptr);
		    transphp(";",1);
		    CPPchar('\n');
		    bPHPmode = 0;
		    ptr = ptr2+2;
		} else {
		    transphp(ptr,strlen(ptr));
		    ptr += strlen(ptr);
		}
	    } else { /* HTML mode */
		ptr2 = strstr(ptr,"<?php");
		if (ptr2) {
		    showhtml(ptr,ptr2-ptr);
		    CPPline();
		    bPHPmode = 1;
		    ptr = ptr2+5;
		} else {
		    showhtml(ptr,strlen(ptr));
		    ptr += strlen(ptr);
		}
	    }

	}
    }
    fclose(f); /* 2001-10-30 */
    if (startnest != nestlevel) {
	fprintf(stderr,"WARNING: {} mismatch in %s\n",aszfname);
    }
    nLINE = sline;
    astrfree(&aszfname);
    aszfname = oldfname;
} /* ProcessFile */

/**************************************************************/

/**************************************************************/
int main(int argc,char **argv)
{
    FILE *f;
    if (argc != 4) {
	fprintf(stderr,"%s",
"usage: php2c++ input.php output.cpp [function name]\n");
	return -1;
    }
    f = fopen(argv[2],"wb");

    /* Four passes: declarations first */
    fputs(
"/* ------------ php2c++ pass 1: Declarations ----------------*/\n",f);
    fputs("#include \"libphp.h\"\n\n",f);
    fputs("using namespace std;\n",f);
    fileHeader = f;
    fileMain = 0;
    ProcessFile(argv[1]);
    fileHeader = 0;
    fileOutHeader = 0; /* 2001-10-30 */


    fputs(
"/* ------------ php2c++ pass 2: Class Declarations ----------*/\n",f);
    astrfree(&aszSeen);
    fileClass = f;
    ProcessFile(argv[1]);
    fileClass = 0;
    fileOutClass = 0; /* 2001-10-30 */

    fputs(
"/* ------------ php2c++ pass 3: Code outside functions ------*/\n",f);
    astrfree(&aszSeen);
    fileMain = f;
    fileOutMain = fileMain;
    fprintf(f, "void %s( void )\n{\n", argv[3]);
    ProcessFile(argv[1]);
    fprintf(f, "} /* %s */\n", argv[3]);

    fputs(
"/* ------------ php2c++ pass 4: Function bodies -------------*/\n",f);
    astrfree(&aszSeen);
    fileFunctions = f;
    fileMain = 0;
    fileOutMain = fileMain;
    ProcessFile(argv[1]);
    fclose(f);

    return 0;
} /* main */

unsigned int countbl(const char *sptr) {
    return strspn(sptr," \t");
}

unsigned int counttoch(const char *sptr,char ch)
{
    const char *ptr = strchr(sptr,ch);
    if (!ptr) {
	return strlen(sptr);
    }
    return ptr - sptr;
}

unsigned int countid(const char *sptr)
{ /* Count characters in a legal PHP identifier */
    const char *ptr = sptr;
    while(1) {
	int len = strspn(ptr,
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
	if (!len) {
	    if ((*ptr == 0x7f)||(*ptr & 0x80)) {
		len = 1;
	    }
	}
	if (!len) {
	    break;
	}
	ptr += len;
    }
    return ptr - sptr;
}

/*********************************************************/
/* Automatic allocating string functions.  More efficient
   versions are available from mibsoftware.com, but these
   work just fine and are small enough to include here.
 */
char *astrn0cpy(char **ppasz,const char *src,int len)
{ /* allocate and strncpy, always store '\0' */
    *ppasz = (char *) realloc(*ppasz,len+1);
    if (!*ppasz) {
	return 0;
    }
    strncpy(*ppasz,src,len);
    (*ppasz)[len] = 0;
    return *ppasz;
} /* astrn0cpy */

char *astrn0cat(char **ppasz,const char *src,int len)
{ /* reallocate and strncat, always store '\0' */
int cBefore = 0;
    if (*ppasz) {
        cBefore = strlen(*ppasz);
	*ppasz = (char *) realloc(*ppasz,len+1+cBefore);
    } else {
	return astrn0cpy(ppasz,src,len);
    }
    if (!*ppasz) {
	return 0;
    }
    strncat(*ppasz+cBefore,src,len);
    (*ppasz)[cBefore + len] = 0;
    return *ppasz;
} /* astrn0cat */


char *astrcat(char **ppasz,const char *src)
{
    return astrn0cat(ppasz,src,strlen(src));
}
char *astrcpy(char **ppasz,const char *src)
{
    return astrn0cpy(ppasz,src,strlen(src));
}

char *afgets(char **ppasz,FILE *f)
{
    char buf[1024];
    if (fgets(buf,sizeof(buf),f)) {
	astrcpy(ppasz,"");
	do {
	    astrcat(ppasz,buf);
	    if (buf[0] && (buf[strlen(buf)-1] == '\n')) {
    		return *ppasz;
	    }
	} while(fgets(buf,sizeof(buf),f));
    } else {
	astrfree(ppasz);
    }
    return *ppasz;
} /* afgets */


char *astrfree(char **ppasz)
{
    if (*ppasz) {
	free(*ppasz);
	*ppasz = 0;
    }
    return *ppasz;
}

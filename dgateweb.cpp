//
//  dgateweb.cpp
//

/*
bcb 20120710  Created.
bcb 20120820  Changes for amap ip wildcards and Dictionary base dir.
bcb 20130226  Replaced gpps with IniValue class .
                Made ACRNemaClass a singleton.  Version to 1.4.18a.
bcb 20130226  Killed line0,1,... and sourceOrLine0.
mvh 20130523  fixed 'webscriptad(d)ress'
*/

#ifndef UNIX
#include <io.h>
#include <fcntl.h>
#endif
#include "dgateweb.hpp"
#include "luaint.hpp"
#include "parse.hpp"
#include "configpacs.hpp"

/////////////////////////////////////////////////////////////////////////////////////////////
// Elementary web interface
/////////////////////////////////////////////////////////////////////////////////////////////

int isxdig(char ch)
{ if ((ch>='0' && ch<='9') ||
      (ch>='A' && ch<='F') ||
      (ch>='a' && ch<='f'))
    return 1;
  else
    return 0;
}

static int xdigit(char ch)
{  if (ch>='0' && ch<='9')
    return ch - '0';
  else
    return (ch & 15) + 9;
}

int htoin(const char *value, int len)//Cannot be static, used in dgate
{ int i, result;

  result = 0;
  i      = 0;
  while (i < len && value[i] == ' ') i++;

  while (i < len && isxdig(value[i]))
  { result = result * 16 + xdigit(value[i]);
    i++;
  }

  return result;
}

int console;

void HTML(const char *string, ...)
{ char n[2]="\n";
  char s[1250];
  va_list vargs;

  va_start(vargs, string);
  vsprintf(s, string, vargs);
  va_end(vargs);

  write(console, s, strlen(s));
  write(console, n, 1);
}

static char *post_buf=NULL;
static char uploadedfile[256];

int CGI(char *out, const char *name, const char *def)
{ char *p = getenv( "QUERY_STRING" );
  char *q = getenv( "CONTENT_LENGTH" );
  char *r = getenv( "REQUEST_METHOD" );
  char tmp[512];
  int  i, j, len=0;
  char *fstart=NULL;
  int flen=0;

//FILE *g = fopen("c:\\temp\\postbuf.txt", "wt");
//fprintf(g, "%s\n%s\n%s\n", p,q,r);
//fclose(g);

  if (out!=def) *out = 0;

  if (r!=NULL && memcmp(r, "POST", 3)==0)
  { if (q!=NULL && *q!=0 && post_buf==NULL) 
    { len = atoi(q);
      post_buf = (char *)malloc(len+1);
      post_buf[len]=0;
#ifndef UNIX
      setmode(fileno(stdin), O_BINARY);
#endif
      fread(post_buf, len, 1, stdin);
      p = post_buf;

//FILE *g = fopen("c:\\temp\\postbuf.bin", "wb");
//fwrite(post_buf, len, 1, g);
//fclose(g);
    }
    else
      if (post_buf) p = post_buf;

    if (p==NULL) return 0;

    if (p[0]=='-')      // multipart data, locate the file (one assumed)
    { q = strstr(p, "filename=");
            
      if (q)
      { *q=0;	      // cut contents short to allow fast read of other entries

        char *ext = strchr(q+1, 0x0d);   // parse extension from filename
        q = ext+1;
        if (ext)
        { *ext--=0;
          if (ext[0]=='"')
          { *ext--=0;
          }
          while (*ext!='.' && *ext!=0) ext--;
        }

        q = strchr(q+1, 0x0a);  // file starts after three newlines
        if (q)
        { q = strchr(q+1, 0x0a);
	  *q = 0;
	  fstart = q+1;
	                      // file ends after two newlines (one at end)
	  flen = len - (int)(fstart-post_buf);//bcb Size warning.
	  flen-=4;
	  while (fstart[flen]!=0x0d && flen>0) flen--;

	  NewTempFile(uploadedfile, ext);
	  FILE *g = fopen(uploadedfile, "wb");
          fwrite(fstart, flen, 1, g);
          fclose(g);
        }
      }

      strcpy(tmp, "name=\"");
      strcat(tmp, name);
      strcat(tmp, "\x22\x0d\x0a\x0d\x0a");

      p = strstr(p, tmp);	      // parse data
      if (p)
      { p += strlen(tmp);
        q = strchr(p, 0x0d);
        if (q) *q=0;
        strcpy(out, p);
        return 0;
      }

      if (out!=def) strcpy(out, def);
      return 0;
    }
    else if (*uploadedfile==0 && p[0]=='<')      // xml
    { NewTempFile(uploadedfile, ".xml");
      FILE *g = fopen(uploadedfile, "wb");
      fwrite(p, len, 1, g);
      fclose(g);
      p = getenv( "QUERY_STRING" );
    }
    else if (*uploadedfile==0)      		// any other type
    { NewTempFile(uploadedfile, ".dat");
      FILE *g = fopen(uploadedfile, "wb");
      fwrite(p, len, 1, g);
      fclose(g);
      p = getenv( "QUERY_STRING" );
    }
    else
      p = getenv( "QUERY_STRING" );
  }


  strcpy(tmp, "&");	// & only hits on second item
  strcat(tmp, name);
  strcat(tmp, "=");
  
  // check first item
  if (p)
  { if (strlen(p)>=strlen(tmp+1) && memcmp(p, tmp+1, strlen(tmp+1))==0) q=p-1;
    else q = strstr(p, tmp);
  }
  else
    q=NULL;

  if (q==NULL)
  { if (out!=def) strcpy(out, def);
    return 0;
  }

  q = q + strlen(tmp);

  i = 0; j = 0;
  while (q[i] != 0  && q[i] != '&')
  { if (q[i] == '%')
    { tmp[j++] = (char)(htoin(q+i+1, 2));
      i = i+3;
    }
    else if (q[i] == '+')
    { tmp[j++] = ' ';
      i++;
    }
    else
      tmp[j++] = q[i++];
  }
  tmp[j++] = 0;

  strcpy(out, tmp);

  return 0;
}

static BOOL Tabulate(const char *c1, const char *par, const char *c4, BOOL edit)
{ const char *p=strchr(par, ':');
  char buf[512];

  SendServerCommand("", par, 0, buf);
  if (buf[0])
  { if (edit) HTML("<TR><TD>%s<TD>%s<TD><INPUT SIZE=64 NAME=\"%s\" VALUE=\"%s\"><TD>%s<TD></TR>", c1, p+1, p+1, buf, c4, p+1);
    else      HTML("<TR><TD>%s<TD>%s<TD>%s<TD>%s<TD></TR>", c1, p+1, buf, c4, p+1);
    return TRUE;
  }
  else
    return FALSE;
}

static void replace(char *string, const char *key, const char *value)
{ char temp[1000];
  char *p = strstr(string, key), *q = string;

  if (p==NULL) return;
  if (value==NULL) return;

  temp[0] = 0;
  while(p)
  { *p = 0;
    strcat(temp, string);
    *p = key[0];
    strcat(temp, value);
    string = p + strlen(key);
    p = strstr(string, key);
  };

  strcat(temp, string);
  strcpy(q, temp);
}

// bcb 20120703 move to hpp.
//static BOOL DgateWADO(char *query_string, char *ext);

void DgateCgi(char *query_string, char *ext)
{ char mode[512], command[8192], size[32], dsize[32], iconsize[32], slice[512], slice2[512], query[512], buf[512], 
       patientidmatch[512], patientnamematch[512], studydatematch[512], startdatematch[512], 
       db[256], series[512], study[512], compress[64]/*, WebScriptAddress[LONG_NAME_LEN], WebMAG0Address[LONG_NAME_LEN], 
       WebServerFor[SHORT_NAME_LEN], WebCodeBase[VLONG_NAME_LEN]*/, lw[64], source[64], dest[64], series2[512], study2[512],
       DefaultPage[256], AnyPage[256], key[512];
  char ex[128], extra[256], graphic[32], viewer[128], studyviewer[128];
  unsigned int  i, j;
  DBENTRY *DBE;
//  char  RootSC[ROOTCONFIG_SZ];
//  char  Temp[MAX_PATH];
  char  *p1;

  *uploadedfile=0;
  if (DgateWADO(query_string, ext)) return;

  BOOL ReadOnly=FALSE;
//  BOOL WebPush=TRUE;
	
  UNUSED_ARGUMENT(query_string);

  console = fileno(stdout);
#ifndef UNIX
  strcpy(ex, ".exe");
  if (ext) strcpy(ex, ext);
  setmode(console, O_BINARY);
#else
  ex[0]=0;
  if (ext) strcpy(ex, ext);
#endif

  IniValue *iniValuePtr = IniValue::GetInstance();
  LoadKFactorFile(iniValuePtr->sscscpPtr->kFactorFile);

  if(iniValuePtr->sscscpPtr->Dictionary[0])
  {
    VRType.AttachRTC(iniValuePtr->sscscpPtr->Dictionary);
    globalPDU.AttachRTC(&VRType);
    ACRNemaMap::GetInstance()->InitACRNemaAddressArray(iniValuePtr->sscscpPtr->ACRNemaMap);
  }

  ReadOnly = iniValuePtr->sscscpPtr->WebReadOnly;

  if(iniValuePtr->sscscpPtr->WebScriptAddress[0] == 0)
    sprintf(iniValuePtr->sscscpPtr->WebScriptAddress, "http://%s/cgi-bin/dgate%s", getenv("SERVER_NAME"), ex);

  if(iniValuePtr->sscscpPtr->WebMAG0Address[0] == 0)
    sprintf(iniValuePtr->sscscpPtr->WebMAG0Address, "http://%s/mag0", getenv("SERVER_NAME"));//, ex); bcb, only 1 %s

// If iniValuePtr->webdefaults->address exists, it is copied to iniValuePtr->sscscpPtr->WebServerFor when the ini file is read.
  if(iniValuePtr->sscscpPtr->WebServerFor[0] == 0)
    sprintf(iniValuePtr->sscscpPtr->WebServerFor, "127.0.0.1");
  strcpy(ServerCommandAddress, iniValuePtr->sscscpPtr->WebServerFor);

  if(iniValuePtr->sscscpPtr->WebCodeBase[0] == 0)
  {
    strcpy(iniValuePtr->sscscpPtr->WebCodeBase, iniValuePtr->sscscpPtr->WebScriptAddress);		// note: WebCodeBase should include trailing slash or backslash
    p1 = strrchr(iniValuePtr->sscscpPtr->WebCodeBase, '/'); 
    if (p1) p1[0]=0;
    p1 = strrchr(iniValuePtr->sscscpPtr->WebCodeBase, '/'); 
    if (p1) p1[1]=0;
  }

  // If no mode is specified, go to this page
  *DefaultPage=0;
  if(iniValuePtr != NULL && iniValuePtr->DefaultPagePtr != NULL && iniValuePtr->DefaultPagePtr->source[0] != 0)
    strcpy(DefaultPage, "DefaultPage");//bcb? Should we check iniValuePtr->sscscpPtr->DefaultPage?

  // If this page is specified, all requests land here (effectively disabling the built-in web pages)
  *AnyPage=0;
  if(iniValuePtr != NULL && iniValuePtr->AnyPagePtr != NULL && iniValuePtr->AnyPagePtr->source[0] != 0)
    strcpy(AnyPage, "AnyPage");//bcb? Should we check iniValuePtr->sscscpPtr->AnyPage?

  CGI(iniValuePtr->sscscpPtr->Port,         "port",    iniValuePtr->sscscpPtr->Port);	// allow serving any server
  CGI(ServerCommandAddress, "address", ServerCommandAddress);

  CGI(mode,    "mode",     "");		// web page
  if (AnyPage[0]) strcpy(mode, AnyPage);// AnyPage redirects all requests to one page

  CGI(query,   "query",    "");		// query for most db selectors
  CGI(slice2,  "slice",    "");		// patid:sop for slice
  CGI(series2, "series",   "");		// patid:seriesuid for seriesviewer/move/delete
  CGI(study2,  "study",    "");		// patid:studyuid for move/delete
  CGI(db,      "db",       "");		// database to edit or list
  CGI(lw,      "lw",       "0/0");	// level/window
  CGI(source,  "source",   "(local)");  // source for move
  CGI(dest,    "dest",     "");		// destination for move
  CGI(key,     "key",      "");		// key for mark

  j = 0;
  for(i=0; i<strlen(slice2); i++) if (slice2[i]==' ') { slice[j++]='%'; slice[j++]='2'; slice[j++]='0'; } else slice[j++]=slice2[i];
  slice[j++]=0;

  j = 0;
  for(i=0; i<strlen(series2); i++) if (series2[i]==' ') { series[j++]='%'; series[j++]='2'; series[j++]='0'; } else series[j++]=series2[i];
  series[j++]=0;

  j = 0;
  for(i=0; i<strlen(study2); i++) if (study2[i]==' ') { study[j++]='%'; study[j++]='2'; study[j++]='0'; } else study[j++]=study2[i];
  study[j++]=0;

  if(iniValuePtr->webdefaultsPtr == NULL)
  {
    strcpy(size, "512");
    strcpy(dsize, "0");
    strcpy(compress, "n4");
    strcpy(iconsize, "482");
    strcpy(graphic, "bmp");
    strcpy(viewer, "seriesviewer");
    strcpy(studyviewer, "");
  }
  else
  {// Size limits set in iniValuePtr->webdefaultsPtr.
    strcpy(size, iniValuePtr->webdefaultsPtr->size);
    strcpy(dsize, iniValuePtr->webdefaultsPtr->dsize);
    strcpy(compress, iniValuePtr->webdefaultsPtr->compress);
    strcpy(iconsize, iniValuePtr->webdefaultsPtr->iconsize);
    strcpy(graphic, iniValuePtr->webdefaultsPtr->graphic);
    strcpy(viewer, iniValuePtr->webdefaultsPtr->viewer);
    strcpy(studyviewer, iniValuePtr->webdefaultsPtr->studyviewer);
  }
  CGI(size,    "size",     size);	// size of viewer in pixels or %
  CGI(dsize,   "dsize",    dsize);	// max size of transmitted dicom images in pixels, 0=original
  CGI(compress,"compress", compress);	// compression of transmitted dicom images to (our) web viewer
  CGI(iconsize,"iconsize", iconsize);	// size of icons in image table
  CGI(graphic, "graphic",  graphic);	// style of transmitting thumbnails and slices (gif, bmp, or jpg)
  CGI(viewer,  "viewer",   viewer);	// mode of used viewer

  CGI(patientidmatch,   "patientidmatch",   "");	// search strings
  CGI(patientnamematch, "patientnamematch", "");
  CGI(studydatematch,   "studydatematch",   "");
  CGI(startdatematch,   "startdatematch",   "");

  sprintf(extra, "port=%s&address=%s", iniValuePtr->sscscpPtr->Port, ServerCommandAddress);

  if (patientidmatch[0]!=0)
  { if (query[0]) strcat(query, " and ");
    strcat(query, "PatientID like '%");
    strcat(query, patientidmatch);
    strcat(query, "%'");
  };

  if (patientnamematch[0]!=0)
  { if (query[0]) strcat(query, " and ");
    strcat(query, "PatientNam like '%");
    strcat(query, patientnamematch);
    strcat(query, "%'");
  };

  if (studydatematch[0]!=0)
  { if (query[0]) strcat(query, " and ");
    strcat(query, "StudyDate like '%");
    strcat(query, studydatematch);
    strcat(query, "%'");
  };

  if (startdatematch[0]!=0)
  { if (query[0]) strcat(query, " and ");
    strcat(query, "StartDate like '%");
    strcat(query, startdatematch);
    strcat(query, "%'");
  };

  if      (stricmp(db, "dicomworklist")==0) DBE=WorkListDB;
  else if (stricmp(db, "dicompatients")==0) DBE=PatientDB;
  else if (stricmp(db, "dicomstudies" )==0) DBE=StudyDB;
  else if (stricmp(db, "dicomseries"  )==0) DBE=SeriesDB;
  else if (stricmp(db, "dicomimages"  )==0) DBE=ImageDB;
  else                                      DBE=WorkListDB;

  // ************************** top page ************************** //

  if ((strcmp(mode, "")==0 && !DefaultPage[0]) || strcmp(mode, "top")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H1>Welcome to the Conquest DICOM server - version %s</H1>", DGATE_VERSION);
    HTML("<IMG SRC='%sconquest.jpg' ALT='Developed in the Conquest project'>", iniValuePtr->sscscpPtr->WebCodeBase);
    HTML("<HR>");
    HTML("<PRE>");
    SendServerCommand("", "display_status:", console);
    HTML("</PRE>");

    HTML("<HR>");

    HTML("<table>");
    HTML("<tr>");
    //HTML("<FORM ACTION=\"dgate%s\" METHOD=POST>", ex);
    HTML("<FORM ACTION=\"dgate%s\">", ex);
    HTML("<INPUT NAME=mode    TYPE=HIDDEN VALUE=querypatients>");
    HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
    HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);
    HTML("<INPUT NAME=key     TYPE=HIDDEN VALUE=%s>", key);
    HTML("<td>List local patients");
    HTML("<td>Patient ID: <INPUT NAME=patientidmatch TYPE=Text VALUE=>");
    HTML("<td>Name: <INPUT NAME=patientnamematch TYPE=Text VALUE=>");
    HTML("<td>");
    HTML("<td><INPUT TYPE=SUBMIT VALUE=Go>");
    HTML("</FORM>");
    HTML("</tr>");

    HTML("<tr>");
    HTML("<FORM ACTION=\"dgate%s\">", ex);
    HTML("<INPUT NAME=mode    TYPE=HIDDEN VALUE=querystudies>");
    HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
    HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);
    HTML("<INPUT NAME=key     TYPE=HIDDEN VALUE=%s>", key);
    HTML("<td>List local studies");
    HTML("<td>Patient ID: <INPUT NAME=patientidmatch TYPE=Text VALUE=>");
    HTML("<td>Name: <INPUT NAME=patientnamematch TYPE=Text VALUE=>");
    HTML("<td>Date: <INPUT NAME=studydatematch TYPE=Text VALUE=>");
    HTML("<td><INPUT TYPE=SUBMIT VALUE=Go>");
    HTML("</FORM>");
    HTML("</tr>");

    HTML("<tr>");
    HTML("<FORM ACTION=\"dgate%s\">", ex);
    HTML("<INPUT NAME=mode    TYPE=HIDDEN VALUE=queryworklist>");
    HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
    HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);
    HTML("<td>List local worklist");
    HTML("<td>Patient ID: <INPUT NAME=patientidmatch TYPE=Text VALUE=>");
    HTML("<td>Name: <INPUT NAME=patientnamematch TYPE=Text VALUE=>");
    HTML("<td>Date: <INPUT NAME=startdatematch TYPE=Text VALUE=>");
    HTML("<td><INPUT TYPE=SUBMIT VALUE=Go>");
    HTML("</FORM>");
    HTML("</tr>");

    HTML("<tr>");
    HTML("<FORM ACTION=\"dgate%s\">", ex);
    HTML("<td>Find ");
    HTML("<select name=mode>");
    HTML("  <option value='patientfinder'>Patient</option>");
    HTML("  <option value='studyfinder'>Study</option>");
    HTML("  <option value='seriesfinder'>Series</option>");
    HTML("</select>");
    HTML("<td>on server");
    HTML("<select name=dest>");
    SendServerCommand("", "get_amaps:<option value='%s'>%0.0s%0.0s%0.0s%s</option>", console);
    HTML("</select>");
    HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
    HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);
    HTML("<INPUT NAME=source  TYPE=HIDDEN VALUE=%s>", source);
    HTML("<INPUT NAME=dest    TYPE=HIDDEN VALUE=%s>", dest);
    HTML("<INPUT NAME=key     TYPE=HIDDEN VALUE=%s>", key);
    HTML("<td>Search: <INPUT NAME=query TYPE=Text VALUE=>");
    HTML("<td><td><INPUT TYPE=SUBMIT VALUE=Go>");
    HTML("</FORM>");
    HTML("</tr>");
    HTML("</table>");

    HTML("<FORM ACTION=\"dgate%s\" METHOD=POST ENCTYPE=\"multipart/form-data\">", ex);
    HTML("<INPUT NAME=mode    TYPE=HIDDEN VALUE=addlocalfile>");
    HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
    HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);
    HTML("Upload file to enter into server (dcm/v2/HL7/zip/7z/gz/tar): <INPUT NAME=filetoupload SIZE=40 TYPE=file VALUE=>");
    HTML("<INPUT TYPE=SUBMIT VALUE=Go>");
    HTML("</FORM>");

    HTML("<HR>");
    HTML("<BR><A HREF=\"dgate%s?%s&mode=showconfig\">Show server configuration</A>", ex, extra);
    HTML("<BR><A HREF=\"dgate%s?%s&mode=showsops\">Show server accepted SOPs</A>", ex, extra);
    HTML("<BR><A HREF=\"dgate%s?%s&mode=showdb\">Show database layout</A>", ex, extra);
    HTML("<BR><A HREF=\"dgate%s?%s&mode=showdictionary\">Show DICOM dictionary (Long!)</A>", ex, extra);
    HTML("</BODY>");
    exit(0);
  };

  // ************************** configuration ************************** //

  if (strcmp(mode, "showconfig")==0)
  { ReadOnly = TRUE;

    HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H1>Welcome to the Conquest DICOM server - version %s</H1>", DGATE_VERSION);
    HTML("<FORM ACTION=\"dgate%s\">", ex);
    if (!ReadOnly) HTML("<INPUT NAME=mode TYPE=HIDDEN VALUE=updateconfig>");
    HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
    HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);
    HTML("<HR>");
    HTML("<table RULES=ALL BORDER=1>");

    HTML("<Caption>List of configuration parameters</caption>");
    HTML("<TR><TD>File<TD>Parameter<TD>Value<TD>Description</TR>");
    Tabulate("dicom.ini", "get_param:MyACRNema", "DICOM name of server", !ReadOnly);
    Tabulate("dicom.ini", "get_param:TCPPort",   "TCP/IP port where server listens", !ReadOnly);
    Tabulate("dicom.ini", "get_param:FileNameSyntax", "Determines name for new files", !ReadOnly);
    Tabulate("dicom.ini", "get_param:FixPhilips", "Strip leading zeros from 10 digit patient ID", !ReadOnly);
    Tabulate("dicom.ini", "get_param:FixKodak", "Strip leading zeros from 8 digit patient ID", !ReadOnly);
    Tabulate("dicom.ini", "get_param:IncomingCompression", "Compression mode for incoming files", !ReadOnly);
    Tabulate("dicom.ini", "get_param:ArchiveCompression", "Compression mode for acrhived files", !ReadOnly);
    Tabulate("dicom.ini", "get_param:MAGDevices", "Number of storage devices", !ReadOnly);
    for (i=0; i<100; i++)
    { sprintf(command, "get_param:MAGDevice%d", i);
      if (!Tabulate("dicom.ini", command, "Directory where data is stored", !ReadOnly)) break;
    }   
    for (i=0; i<10; i++)
    { sprintf(command, "get_param:VirtualServerFor%d", i);
      if (!Tabulate("dicom.ini", command, "Act as pass through for this server", !ReadOnly)) break;
    }
    for (i=0; i<100; i++)
    { sprintf(command, "get_param:ExportConverter%d", i);
      if (!Tabulate("dicom.ini", command, "Script applied to just saved data", !ReadOnly)) break;
    }   
    for (i=0; i<100; i++)
    { sprintf(command, "get_param:ImportConverter%d", i);
      if (!Tabulate("dicom.ini", command, "Script applied to incoming images", !ReadOnly)) break;
    }   
    Tabulate("dicom.ini", "get_param:QueryConverter0", "Script applied to queries", !ReadOnly);
    Tabulate("dicom.ini", "get_param:WorkListQueryConverter0", "Script applied to worklist queries", !ReadOnly);
    Tabulate("dicom.ini", "get_param:RetrieveConverter0", "Script applied to transmitted images", !ReadOnly);
    Tabulate("dicom.ini", "get_param:RetrieveResultConverter0", "Script applied to query results", !ReadOnly);
    Tabulate("dicom.ini", "get_param:ModalityWorkListQueryResultConverter0", "Script applied to worklist query results", !ReadOnly);
    HTML("<TR></TR>");
    for (i=0; i<1000; i++)
    { sprintf(command, "get_amap:%d", i);
      if (!Tabulate("acrnema.map", command, "Known DICOM provider")) break;
    }

    HTML("</table>");
    if (!ReadOnly) HTML("<INPUT TYPE=SUBMIT VALUE=Update>");
    HTML("</FORM>");
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "showsops")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H1>Welcome to the Conquest DICOM server - version %s</H1>", DGATE_VERSION);
    HTML("<HR>");
    HTML("<table RULES=ALL BORDER=1>");

    HTML("<Caption>List of uids</caption>");
    HTML("<TR><TD>File<TD>Parameter<TD>Value<TD>Description</TR>");
    for (i=0; i<1000; i++)
    { sprintf(command, "get_sop:%d", i);
      if (!Tabulate("dgatesop.lst", command, "Accepted SOP class uid")) break;
    }
    for (i=0; i<1000; i++)
    { sprintf(command, "get_transfer:%d", i);
      if (!Tabulate("dgatesop.lst", command, "Accepted transfer syntax ")) break;
    }
    for (i=0; i<1000; i++)
    { sprintf(command, "get_application:%d", i);
      if (!Tabulate("dgatesop.lst", command, "Accepted application uid")) break;
    }
    for (i=0; i<1000; i++)
    { sprintf(command, "get_localae:%d", i);
      if (!Tabulate("dgatesop.lst", command, "Accepted called AE title")) break;
    }
    for (i=0; i<1000; i++)
    { sprintf(command, "get_remotea:%d", i);
      if (!Tabulate("dgatesop.lst", command, "Accepted calling AE title")) break;
    }
    HTML("</table>");
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "showdb")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H1>Welcome to the Conquest DICOM server - version %s</H1>", DGATE_VERSION);
    HTML("<HR>");
    HTML("<table RULES=ALL BORDER=1>");

    HTML("<Caption>List of configuration parameters</caption>");
    HTML("<TR><TD>File<TD>Parameter<TD>Value<TD>Description</TR>");
    for (i=0; i<1000; i++)
    { sprintf(command, "get_sqldef:PATIENT,%d", i);
      if (!Tabulate("dicom.sql", command, "Patient database field definition")) break;
    }
    for (i=0; i<1000; i++)
    { sprintf(command, "get_sqldef:STUDY,%d", i);
      if (!Tabulate("dicom.sql", command, "Study database field definition")) break;
    }
    for (i=0; i<1000; i++)
    { sprintf(command, "get_sqldef:SERIES,%d", i);
      if (!Tabulate("dicom.sql", command, "Series database field definition")) break;
    }
    for (i=0; i<1000; i++)
    { sprintf(command, "get_sqldef:IMAGE,%d", i);
      if (!Tabulate("dicom.sql", command, "Image database field definition")) break;
    }
    for (i=0; i<1000; i++)
    { sprintf(command, "get_sqldef:WORKLIST,%d", i);
      if (!Tabulate("dicom.sql", command, "Worklist database field definition")) break;
    }
    HTML("</table>");
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "showdictionary")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H1>Welcome to the Conquest DICOM server - version %s</H1>", DGATE_VERSION);
    HTML("<HR>");
    HTML("<table RULES=ALL BORDER=1>");

    HTML("<Caption>List of configuration parameters</caption>");
    HTML("<TR><TD>File<TD>Parameter<TD>Value<TD>Description</TR>");
    for (i=0; i<9000; i++)
    { sprintf(command, "get_dic:%d", i);
      if (!Tabulate("dgate.dic", command, "Dictionary item")) break;
    }
    HTML("</table>");
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "updateconfig")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H1>Welcome to the Conquest DICOM server - version %s</H1>", DGATE_VERSION);
    HTML("<HR>");
    if (!ReadOnly) HTML("Updating the configuration through the WEB interface is not yet supported");
    else           HTML("Updating the configuration through the WEB interface is not allowed");
    HTML("</BODY>");
    exit(0);
  };

  // ************************** local browsers ************************** //

  if (strcmp(mode, "querypatients")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>",DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    if (query[0]) HTML("<Caption>List of selected patients on local server</caption>");
    else          HTML("<Caption>List of all patients on local server</caption>");
    HTML("<TR><TD>Patient ID<TD>Name<TD>Sex<TD>Birth date</TR>");

    strcpy(command,                 "query:DICOMPatients|patientid,patientnam,patientsex,patientbir|");
    strcpy(command+strlen(command),  query);
    sprintf(command+strlen(command), "|<TR><TD><A HREF=dgate%s?%s&mode=querystudies&key=%s&query=patientid+=+'%%s'>%%0.0s %%0.0s %%0.0s", ex, extra, key);
    strcpy(command+strlen(command), "%s</A><TD>%s<TD>%s<TD>%s</TR>");
    SendServerCommand("", command, console);

    HTML("</table>");
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "querystudies")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    if (query[0]) HTML("<Caption>List of selected studies on local server</caption>");
    else          HTML("<Caption>List of all studies on local server</caption>");
    HTML("<TR><TD>Patient ID<TD>Name<TD>Study date<TD>Study description<TD>Study modality</TR>");

    strcpy(command,                 "query:DICOMStudies|patientid,studyinsta,patientnam,studydate,studydescr,studymodal|");
    strcpy(command+strlen(command),  query);
    sprintf(command+strlen(command), "|<TR><TD><A HREF=dgate%s?%s&mode=queryseries&key=%s&query=DICOMStudies.patientid+=+'%%s'+and+DICOMSeries.studyinsta+=+'%%s'>%%0.0s %%0.0s %%0.0s %%0.0s", ex, extra, key);
    strcpy(command+strlen(command), "%s</A><TD>%0.0s %s<TD>%s<TD>%s<TD>%s");
    if (studyviewer[0])
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=%s&study=%%s:%%s&size=%s>view%%0.0s %%0.0s %%0.0s %%0.0s</A>", ex, extra, studyviewer, size);
    if (iniValuePtr->sscscpPtr->WebPush)
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=studymover&study=%%s:%%s&source=%s&key=%s>push%%0.0s %%0.0s %%0.0s %%0.0s</A>", ex, extra, source, key);
    if (key[0] && iniValuePtr->markstudyPtr != NULL && iniValuePtr->markstudyPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=markstudy&study=%%s:%%s&source=%s&key=%s>%s%%0.0s %%0.0s %%0.0s %%0.0s %%0.0s</A>", ex, extra, source, key, iniValuePtr->markstudyPtr->caption);
    }
    if (!ReadOnly) 
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=studydeleter&study=%%s:%%s>delete%%0.0s %%0.0s %%0.0s %%0.0s</A>", ex, extra);
    strcpy(command+strlen(command), "</TR>");
    SendServerCommand("", command, console);

    HTML("</table>");

    if (key[0] && iniValuePtr->shoppingcartPtr != NULL && iniValuePtr->shoppingcartPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command, "<A HREF=dgate%s?%s&mode=shoppingcart&key=%s>%s</A>",  ex, extra, key, iniValuePtr->shoppingcartPtr->caption);
      HTML("%s", command);
    }

    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "queryseries")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    HTML("<Caption>List of series on local server</caption>");
    HTML("<TR><TD>Patient ID<TD>Name<TD>Series date<TD>Series time<TD>Series description<TD>Modality</TR>");

    strcpy(command,                 "query:DICOMSeries,DICOMStudies|DICOMStudies.patientid,DICOMSeries.seriesinst,DICOMStudies.patientnam,DICOMSeries.seriesdate,DICOMSeries.seriestime,DICOMSeries.seriesdesc,DICOMSeries.modality|");
    strcpy(command+strlen(command),  query);
    strcpy(command+strlen(command), " and DICOMStudies.studyinsta=DICOMSeries.studyinsta");
    sprintf(command+strlen(command), "|<TR><TD><A HREF=dgate%s?%s&mode=queryallimages&key=%s&query=DICOMStudies.patientid+=+'%%s'+and+DICOMSeries.seriesinst+=+'%%s'>%%0.0s %%0.0s %%0.0s %%0.0s %%0.0s %%s %%0.0s</A>", ex, extra, key);
    strcpy(command+strlen(command), "<TD>%s<TD>%s<TD>%s<TD>%s<TD>%s");
    sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=queryimages&query=DICOMStudies.patientid+=+'%%s'+and+DICOMSeries.seriesinst+=+'%%s'&size=%s>thumbs%%0.0s %%0.0s %%0.0s %%0.0s %%0.0s</A>", ex, extra, size);
    if (viewer[0])
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=%s&series=%%s:%%s&size=%s>view%%0.0s %%0.0s %%0.0s %%0.0s %%0.0s</A>", ex, extra, viewer, size);
    if (iniValuePtr->sscscpPtr->WebPush)
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=seriesmover&series=%%s:%%s&source=%s&key=%s>push%%0.0s %%0.0s %%0.0s %%0.0s %%0.0s</A>", ex, extra, source, key);
    if (key[0] && iniValuePtr->markseriesPtr != NULL && iniValuePtr->markseriesPtr->source[0] != 0)//(buf[0] && key[0])
    {
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=markseries&series=%%s:%%s&source=%s&key=%s>%s%%0.0s %%0.0s %%0.0s %%0.0s %%0.0s</A>", ex, extra, source, key, iniValuePtr->markseriesPtr->caption);
    }
    if (!ReadOnly) 
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=seriesdeleter&series=%%s:%%s>delete%%0.0s %%0.0s %%0.0s %%0.0s %%0.0s</A>", ex, extra);
    strcpy(command+strlen(command), "</TR>");
    SendServerCommand("", command, console);

    HTML("</table>");

    if (key[0] && iniValuePtr->shoppingcartPtr != NULL && iniValuePtr->shoppingcartPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command, "<A HREF=dgate%s?%s&mode=shoppingcart&key=%s>%s</A>",  ex, extra, key, iniValuePtr->shoppingcartPtr->caption);
      HTML("%s", command);
    }

    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "queryimages")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    HTML("<Caption>List of images with thumbnails on local server (maximal 50)</caption>");
    HTML("<TR><TD>Patient ID<TD>Name<TD>Date<TD>Image number<TD>Slice location<TD>Icon</TR>");

    strcpy(command,                 "query2:DICOMImages,DICOMSeries,DICOMStudies|DICOMStudies.patientid,DICOMImages.sopinstanc,DICOMStudies.patientnam,DICOMSeries.seriesdate,DICOMImages.imagenumbe,DICOMImages.slicelocat|");
    strcpy(command+strlen(command),  query);
    strcpy(command+strlen(command), " and DICOMStudies.studyinsta=DICOMSeries.studyinsta and DICOMSeries.seriesinst=DICOMImages.seriesinst");
    sprintf(command+strlen(command), "|<TR><TD><A HREF=dgate%s?%s&mode=sliceviewer&slice=%%s:%%s&size=%s&graphic=%s>%%0.0s %%0.0s %%0.0s %%0.0s", ex, extra, size, graphic);
    strcpy(command+strlen(command), "%s</A><TD>%0.0s %s<TD>%s<TD>%s<TD>%s");
    sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=headerdump&slice=%%s:%%s %%0.0s %%0.0s %%0.0s %%0.0s>", ex, extra);
    sprintf(command+strlen(command), "<IMG SRC=dgate%s?%s&mode=slice&slice=%%s:%%s&size=%s&graphic=%s></A></TR>", ex, extra, iconsize, graphic);
    strcpy(command+strlen(command), "|50");
    SendServerCommand("", command, console);

    HTML("</table>");
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "queryallimages")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    HTML("<Caption>List of images on local server (all)</caption>");
    HTML("<TR><TD>Patient ID<TD>Name<TD>Date<TD>Image number<TD>Slice location<TD>Header</TR>");

    strcpy(command,                 "query:DICOMImages,DICOMSeries,DICOMStudies|DICOMStudies.patientid,DICOMImages.sopinstanc,DICOMStudies.patientnam,DICOMSeries.seriesdate,DICOMImages.imagenumbe,DICOMImages.slicelocat|");
    strcpy(command+strlen(command),  query);
    strcpy(command+strlen(command), " and DICOMStudies.studyinsta=DICOMSeries.studyinsta and DICOMSeries.seriesinst=DICOMImages.seriesinst");
    sprintf(command+strlen(command), "|<TR><TD><A HREF=dgate%s?%s&mode=sliceviewer&slice=%%s:%%s&size=%s&graphic=%s>%%0.0s %%0.0s %%0.0s %%0.0s", ex, extra, size, graphic);
    strcpy(command+strlen(command), "%s</A><TD>%0.0s %s<TD>%s<TD>%s<TD>%s");
    sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=headerdump&slice=%%s:%%s %%0.0s %%0.0s %%0.0s %%0.0s>", ex, extra);
    strcpy(command+strlen(command), "*</A></TR>");
    SendServerCommand("", command, console);

    HTML("</table>");
    HTML("</BODY>");
    exit(0);
  };

  // ************************** remote query ************************** //

  if (strcmp(mode, "patientfinder")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    if (query[0]) HTML("<Caption>List of selected patients on %s</caption>", dest);
    else          HTML("<Caption>List of all patients on %s</caption>", dest);
    HTML("<TR><TD>ID<TD>Name</TR>");

    sprintf(command,                 "patientfinder:%s|%s", dest, query);
    sprintf(command+strlen(command), "|<TR><TD><A HREF=dgate%s?%s&mode=studyfinder&dest=%s&key=%s&query==%%0.0s%%s>", ex, extra, dest, key);
    strcpy(command+strlen(command),  "%0.0s%s</A><TD>%s");
    SendServerCommand("", command, console);

    HTML("</table>");
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "studyfinder")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    if (query[0]) HTML("<Caption>List of selected studies on %s</caption>", dest);
    else          HTML("<Caption>List of all studies on %s</caption>", dest);
    HTML("<TR><TD>Date<TD>Modality<TD>Name<TD>Id<TD>UID</TR>");

    sprintf(command,                 "studyfinder:%s|%s", dest, query);
    sprintf(command+strlen(command), "|<TR><TD><A HREF=dgate%s?%s&mode=seriesfinder&dest=%s&key=%s&query==%%0.0s%%0.0s%%0.0s%%0.0s%%s>", ex, extra, dest, key);
    strcpy(command+strlen(command),  "%s</A><TD>%s<TD>%s<TD>%s<TD>%s");
    if (iniValuePtr->sscscpPtr->WebPush)
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=studymover&study=%%0.0s%%0.0s%%0.0s%%s:%%s&source=%s&key=%s>push</A>", ex, extra, dest, key);
    if (key[0] && iniValuePtr->markstudyPtr != NULL && iniValuePtr->markstudyPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=markstudy&study=%%0.0s%%0.0s%%0.0s%%s:%%s&source=%s&key=%s>%s</A>", ex, extra, dest, key, iniValuePtr->markstudyPtr->caption);
    }
    strcpy(command+strlen(command), "</TR>");
    SendServerCommand("", command, console);

    HTML("</table>");

    if (key[0] && iniValuePtr->shoppingcartPtr != NULL && iniValuePtr->shoppingcartPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command, "<A HREF=dgate%s?%s&mode=shoppingcart&key=%s>%s</A>",  ex, extra, key, iniValuePtr->shoppingcartPtr->caption);
      HTML("%s", command);
    }

    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "seriesfinder")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    if (query[0]) HTML("<Caption>List of selected series on %s</caption>", dest);
    else          HTML("<Caption>List of all series on %s</caption>", dest);
    HTML("<TR><TD>Date<TD>Time<TD>Modality<TD>Name<TD>ID<TD>UID</TR>");

    sprintf(command,                 "seriesfinder:%s|%s", dest, query);
    sprintf(command+strlen(command),  "|<TR><TD><A HREF=dgate%s?%s&mode=imagefinder&dest=%s&key=%s&query==%%0.0s%%0.0s%%0.0s%%0.0s%%0.0s%%0.0s%%s>%%s</A><TD>%%s<TD>%%s<TD>%%s<TD>%%s%%0.0s<TD>%%s</TR>", ex, extra, dest, key);
    if (iniValuePtr->sscscpPtr->WebPush)
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=seriesmover&series=%%0.0s%%0.0s%%0.0s%%0.0s%%s:%%0.0s%%s&study=%%0.0s%%0.0s%%0.0s%%0.0s%%s:%%s%%0.0s&source=%s&key=%s>push</A>", ex, extra, dest, key);
    if (key[0] && iniValuePtr->markseriesPtr != NULL && iniValuePtr->markseriesPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command+strlen(command), "<TD><A HREF=dgate%s?%s&mode=markseries&series=%%0.0s%%0.0s%%0.0s%%0.0s%%s:%%0.0s%%s&study=%%0.0s%%0.0s%%0.0s%%0.0s%%s:%%s%%0.0s&source=%s&key=%s>%s</A>", ex, extra, dest, key, iniValuePtr->markseriesPtr->caption);
    }
    SendServerCommand("", command, console);

    HTML("</table>");

    if (key[0] && iniValuePtr->shoppingcartPtr != NULL && iniValuePtr->shoppingcartPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command, "<A HREF=dgate%s?%s&mode=shoppingcart&key=%s>%s</A>",  ex, extra, key, iniValuePtr->shoppingcartPtr->caption);
      HTML("%s", command);
    }

    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "imagefinder")==0)
  { char WADOserver[256], *WADOserverPtr;
    WADOserverPtr = iniValuePtr->GetIniString(dest, "wadoservers");
    if(WADOserverPtr != NULL)
    {
      sprintf(WADOserver, "%s", WADOserverPtr);
      free(WADOserverPtr);
    }
    else sprintf(WADOserver, "dgate%s?%s", ex, extra);

    HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    if (query[0]) HTML("<Caption>List of selected images on %s</caption>", dest);
    else          HTML("<Caption>List of all images on %s</caption>", dest);
    HTML("<TR><TD>Sop<TD>ID<TD>Slice<TD>Location</TR>");

    sprintf(command,                 "imagefinder:%s|%s", dest, query);
    strcpy(command+strlen(command),  "|<TR><TD>%s%0.0s<TD>%s%0.0s%0.0s<TD>%s<TD>%s</TR>");
    sprintf(command+strlen(command), "<TD><A HREF=%s&requestType=WADO&objectUID=%%s%%0.0s%%0.0s&studyUID=%%s&seriesUID=%%s&contentType=image/gif>view (WADO)</A>", WADOserver);
    SendServerCommand("", command, console);

    HTML("</table>");
    HTML("</BODY>");
    exit(0);
  };

  // unused //
  if (strcmp(mode, "dicomfind")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<FORM ACTION=\"dgate%s\">", ex);

    HTML("Find ");
    HTML("<select name=mode>");
    HTML("  <option value='patientfinder'>Patient</option>");
    HTML("  <option value='studyfinder'>Study</option>");
    HTML("  <option value='seriesfinder'>Series</option>");
    HTML("</select>");
    HTML("on server");

    HTML("<select name=dest>");
    SendServerCommand("", "get_amaps:<option value='%s'>%0.0s%0.0s%0.0s%s</option>", console);
    HTML("</select>");

    HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
    HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);
    HTML("<INPUT NAME=source  TYPE=HIDDEN VALUE=%s>", source);
    HTML("<INPUT NAME=dest    TYPE=HIDDEN VALUE=%s>", dest);
    HTML("Search: <INPUT NAME=query TYPE=Text VALUE=>");
    HTML("<INPUT TYPE=SUBMIT VALUE=Go>");
    HTML("</FORM>");
    HTML("</BODY>");
    exit(0);
  };

  // ************************** movers ************************** //

  if (strcmp(mode, "studymover")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    HTML("<Caption>Send study: %s</caption>", study2);
    HTML("<TR><TD>Source<TD>Destination</TR>");

    for (i=0; i<1000; i++)
    { char com[80], buf1[80];
      sprintf(com, "get_amap:%d,%%s", i);
      SendServerCommand("", com, 0, buf1);
      if (buf1[0]==0) break;
      if (strchr(buf1, '*')==NULL && strcmp(source, buf1)!=0)
      { sprintf(command, "<TR><TD>%s<TD><A HREF=dgate%s?%s&mode=movestudy&study=%s&source=%s&dest=%s>%s</A></TR>", source, ex, extra, study, source, buf1, buf1);
        HTML("%s", command);
      }
    }

    if (strcmp(source, "(local)")==0)
    { sprintf(command, "<TR><TD>(local)<TD><A HREF=dgate%s?%s&mode=zipstudy&study=%s&dum=.zip>Zip file</A></TR>", ex, extra, study2);
      HTML("%s", command);
    }

    if (key[0] && iniValuePtr->markstudyPtr != NULL && iniValuePtr->markstudyPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command, "<TR><TD>%s<TD><A HREF=dgate%s?%s&mode=markstudy&study=%s&source=%s&key=%s>%s</A></TR>", source, ex, extra, study2, source, key, iniValuePtr->markstudyPtr->caption);
      HTML("%s", command);
    }

    HTML("</table>");

    if (key[0] && iniValuePtr->shoppingcartPtr != NULL && iniValuePtr->shoppingcartPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command, "<A HREF=dgate%s?%s&mode=shoppingcart&key=%s>%s</A>",  ex, extra, key, iniValuePtr->shoppingcartPtr->caption);
      HTML("%s", command);
    }

    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "seriesmover")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    HTML("<Caption>Send series: %s</caption>", series2);
    HTML("<TR><TD>Source<TD>Destination</TR>");

    for (i=0; i<1000; i++)
    { char com[80], buf1[80];
      sprintf(com, "get_amap:%d,%%s", i);
      SendServerCommand("", com, 0, buf1);
      if (buf1[0]==0) break;
      if (strchr(buf1, '*')==NULL && strcmp(source, buf1)!=0)
      { sprintf(command, "<TR><TD>%s<TD><A HREF=dgate%s?%s&mode=moveseries&series=%s&study=%s&source=%s&dest=%s>%s</A></TR>", source, ex, extra, series, study, source, buf1, buf1);
        HTML("%s", command);
      }
    }

    if (strcmp(source, "(local)")==0)
    { sprintf(command, "<TR><TD>(local)<TD><A HREF=dgate%s?%s&mode=zipseries&series=%s&dum=.zip>Zip file</A></TR>", ex, extra, series);
      HTML("%s", command);
    }

    if (key[0] && iniValuePtr->markseriesPtr != NULL && iniValuePtr->markseriesPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command, "<TR><TD>%s<TD><A HREF=dgate%s?%s&mode=markseries&series=%s&source=%s&key=%s>%s</A></TR>", source, ex, extra, series, source, key, iniValuePtr->markseriesPtr->caption);
      HTML("%s", command);
    }

    HTML("</table>");

    if (key[0] && iniValuePtr->shoppingcartPtr != NULL && iniValuePtr->shoppingcartPtr->source[0] != 0)// (buf[0] && key[0])
    {
      sprintf(command, "<A HREF=dgate%s?%s&mode=shoppingcart&key=%s>%s</A>",  ex, extra, key, iniValuePtr->shoppingcartPtr->caption);
      HTML("%s", command);
    }

    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "movestudy")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
    sprintf(command, "movestudy:%s,%s,%s", source, dest, study2);
    SendServerCommand("", command, 0, buf);
    HTML("%s", command);
    HTML("<BR>");
    HTML("Done %s", study2);
    HTML("</BODY>");
    exit(0);
  }

  if (strcmp(mode, "moveseries")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
    sprintf(command, "moveseries:%s,%s,%s", source, dest, series2);
    if (study2[0])
    { p1 = strchr(study2, ':'); if(!p1) p1 = study2; else p1++;
      sprintf(command+strlen(command), ",%s", p1);
    }
    SendServerCommand("", command, 0, buf);
    HTML("%s", command);
    HTML("<BR>");
    HTML("Done %s", series2);
    HTML("</BODY>");
    exit(0);
  }

  if (strcmp(mode, "addlocalfile")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
    HTML("</BODY>");
    HTML("Uploaded and processed file:");
    HTML(uploadedfile);
    char com[512], file[512];
    sprintf(com, "addimagefile:%s", uploadedfile);
    strcpy(file, uploadedfile);
    SendServerCommand("", com, 0, uploadedfile, FALSE, TRUE);
    unlink(file);
    exit(0);
  };

  // ************************** delete ************************** //

  if (!ReadOnly) 
  { if (strcmp(mode, "studydeleter")==0)
    { HTML("Content-type: text/html\n");
      HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
      HTML("<BODY BGCOLOR='CFDFCF'>");
      HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
      HTML("<Caption>Delete study: %s</caption>", study);
  
      HTML("<FORM ACTION=\"dgate%s\">", ex);
      HTML("<INPUT NAME=mode    TYPE=HIDDEN VALUE=deletestudy>");
      HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
      HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);
      HTML("<INPUT NAME=study   TYPE=HIDDEN VALUE='%s'>", study);
      HTML("<INPUT TYPE=SUBMIT VALUE='Delete the study'>");
      HTML("</FORM>");
      HTML("</BODY>");
      exit(0);
    };
  
    if (strcmp(mode, "seriesdeleter")==0)
    { HTML("Content-type: text/html\n");
      HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
      HTML("<BODY BGCOLOR='CFDFCF'>");
      HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
      HTML("<Caption>Delete series: %s</caption>", series);
  
      HTML("<FORM ACTION=\"dgate%s\">", ex);
      HTML("<INPUT NAME=mode    TYPE=HIDDEN VALUE=deleteseries>");
      HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
      HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);
      HTML("<INPUT NAME=series  TYPE=HIDDEN VALUE='%s'>", series);
      HTML("<INPUT TYPE=SUBMIT VALUE='Delete the series'>");
      HTML("</FORM>");
      HTML("</BODY>");
      exit(0);
    };
  
    if (strcmp(mode, "deletestudy")==0)
    { HTML("Content-type: text/html\n");
      HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
      HTML("<BODY BGCOLOR='CFDFCF'>");
      HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
      sprintf(command, "deletestudy:%s", study2);
      SendServerCommand("", command, 0, buf);
      HTML(command);
      HTML("<BR>");
      HTML("Done");
      HTML("</BODY>");
      exit(0);
    }
  
    if (strcmp(mode, "deleteseries")==0)
    { HTML("Content-type: text/html\n");
      HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
      HTML("<BODY BGCOLOR='CFDFCF'>");
      HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
      sprintf(command, "deleteseries:%s", series2);
      SendServerCommand("", command, 0, buf);
      HTML(command);
      HTML("<BR>");
      HTML("Done");
      HTML("</BODY>");
      exit(0);
    }
  }

  // ************************** worklist browser and editor ************************** //

  if (strcmp(mode, "queryworklist")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<table RULES=ALL BORDER=1>");
    if (query[0]) HTML("<Caption>List of selected worklist entries</caption>");
    else          HTML("<Caption>List of all worklist entries (not all fields displayed)</caption>");
    HTML("<TR><TD>AccessionN<TD>Patient ID<TD>Name<TD>Birth date<TD>Sex<TD>Physician<TD>Description<TD>Modality<TD>Date<TD>Time</TR>");

    strcpy(command,                  "query:dicomworklist|AccessionN,PatientID,PatientNam,PatientBir,PatientSex,ReqPhysici,ReqProcDes,Modality,StartDate,StartTime|");
    strcpy(command+strlen(command),  query);
    sprintf(command+strlen(command), "|<TR><TD><A HREF=dgate%s?%s&mode=editrecord&db=dicomworklist&query=AccessionN+=+'%%s'>%%0.0s %%0.0s %%0.0s %%0.0s %%0.0s %%0.0s %%0.0s %%0.0s %%0.0s", ex, extra);
    sprintf(command+strlen(command), "%%s</A><TD>%%s<TD>%%s<TD>%%s<TD>%%s<TD>%%s<TD>%%s<TD>%%s<TD>%%s<TD>%%s<TD><A HREF=dgate%s?%s&mode=deleterecord&db=dicomworklist&query=AccessionN+=+'%%s'>Delete</A></TR>", ex, extra);
    SendServerCommand("", command, console);

    HTML("</table>");

    sprintf(command, "<A HREF=dgate%s?%s&mode=addrecord&db=dicomworklist>Add worklist entry</A>", ex, extra);
    HTML(command);
    HTML("</BODY>");
    exit(0);
  };

  // ************************** general purpose database editing ************************** //

  if (strcmp(mode, "editrecord")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    if (!DBE)
    { HTML("<H2>File DICOM.SQL not found - please check configuration</H2>");
      exit(0);
    }

    HTML("<FORM ACTION=\"dgate%s\">", ex);
    if (!ReadOnly) HTML("<INPUT TYPE=HIDDEN NAME=mode  VALUE=saverecord>");
    HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
    HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);
    HTML("<INPUT NAME=db      TYPE=HIDDEN VALUE=%s>", db);
    HTML("<INPUT NAME=query   TYPE=HIDDEN VALUE=%s>", query);

    HTML("<table RULES=ALL BORDER=2>");
    HTML("<Caption>Edit %s entry</caption>", db);
    HTML("<TR><TD>Field<TD>Value</TR>");

    strcpy(command, "query2:");
    strcat(command, db);
    strcat(command, "|");
    i = 0;
    while ( TRUE )
    { if(!DBE[i].Element) break;			// end of fields
      if (DBE[i].DICOMType!=DT_STARTSEQUENCE && DBE[i].DICOMType!=DT_ENDSEQUENCE)
      {	strcat(command, DBE[i].SQLColumn);
	strcat(command, ",");
      }
      ++i;
    }
    command[strlen(command)-1]=0;			// remove trailing ,

    strcat(command, "|");
    strcat(command, query);
    strcat(command, "|");
    
    i = 0;
    while ( TRUE )
    { if(!DBE[i].Element) break;			// end of fields
      if (DBE[i].DICOMType!=DT_STARTSEQUENCE && DBE[i].DICOMType!=DT_ENDSEQUENCE)
      {	strcat(command, "<TR><TD>");
      	strcat(command, DBE[i].SQLColumn);
	strcat(command, "<TD><INPUT NAME=");
      	strcat(command, DBE[i].SQLColumn);
      	strcat(command, " TYPE=Text VALUE='%s'></TR>");
      }
      ++i;
    };
    strcat(command, "|1");	// max 1 record !!!!!
    
    SendServerCommand("", command, console);

    HTML("</table>");
    if (!ReadOnly) HTML("<INPUT TYPE=SUBMIT VALUE=Save>");
    HTML("</FORM>");
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "addrecord")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    if (!DBE)
    { HTML("<H2>File DICOM.SQL not found - please check configuration</H2>");
      exit(0);
    }

    HTML("<FORM ACTION=\"dgate%s\">", ex);
    if (!ReadOnly) HTML("<INPUT TYPE=HIDDEN NAME=mode VALUE=saverecord>");
    HTML("<INPUT NAME=db      TYPE=HIDDEN VALUE=%s>", db);
    HTML("<INPUT NAME=port    TYPE=HIDDEN VALUE=%s>", iniValuePtr->sscscpPtr->Port);
    HTML("<INPUT NAME=address TYPE=HIDDEN VALUE=%s>", ServerCommandAddress);

    HTML("<table RULES=ALL BORDER=2>");
    sprintf(command, "<Caption>Add %s entry</caption>", db);
    HTML(command);

    HTML("<TR><TD>Field<TD>Value</TR>");

    i = 0;
    while ( TRUE )
    { if(!DBE[i].Element) break;			// end of fields
      if (DBE[i].DICOMType!=DT_STARTSEQUENCE && DBE[i].DICOMType!=DT_ENDSEQUENCE)
      {	strcpy(command, "<TR><TD>");
      	strcat(command, DBE[i].SQLColumn);
	strcat(command, "<TD><INPUT NAME=");
      	strcat(command, DBE[i].SQLColumn);
      	strcat(command, " TYPE=Text VALUE=''></TR>");
      	HTML(command);
      }
      ++i;
    }

    HTML("</table>");
    if (!ReadOnly) HTML("<INPUT TYPE=SUBMIT VALUE=Save>");
    HTML("</FORM>");
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "saverecord")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    if (!DBE)
    { HTML("<H2>File DICOM.SQL not found - please check configuration</H2>");
      exit(0);
    }

    if (ReadOnly || DBE!=WorkListDB)
    { HTML("<H2>This table is readonly</H2>");
      exit(0);
    }
    
    if (strlen(query))
    { sprintf(command, "deleterecord:%s,%s", db, query); 
      SendServerCommand("", command, console);
      sprintf(command, "Updated/added %s entry for %s", db, query);
      HTML(command);
    }
    else
    { sprintf(command, "Added new record in %s", db);
      HTML(command);
    };
     
    sprintf(command, "addrecord:%s|", db);
    i = 0;
    while ( TRUE )
    { if(!DBE[i].Element) break;			// end of fields
      if (DBE[i].DICOMType!=DT_STARTSEQUENCE && DBE[i].DICOMType!=DT_ENDSEQUENCE)
      {	strcat(command, DBE[i].SQLColumn);
	strcat(command, ",");
      }
      ++i;
    }
    command[strlen(command)-1]=0;			// remove trailing ,
    strcat(command, "|");

    i = 0;
    while ( TRUE )
    { if(!DBE[i].Element) break;			// end of fields
      if (DBE[i].DICOMType!=DT_STARTSEQUENCE && DBE[i].DICOMType!=DT_ENDSEQUENCE)
      {	strcat(command, "'"); 
        CGI(buf, DBE[i].SQLColumn, ""); 
        strcat(command, buf); 
        strcat(command, "', "); 
      }
      ++i;
    }
    command[strlen(command)-2]=0;			// remove trailing , and space

    SendServerCommand("", command, console);
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "deleterecord")==0)
  { HTML("Content-type: text/html\nCache-Control: no-cache\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    if (ReadOnly || DBE!=WorkListDB)
    { HTML("<H2>This table is readonly</H2>");
      exit(0);
    }

    sprintf(command, "deleterecord:%s,%s", db, query); 
    SendServerCommand("", command, console);
    HTML("Deleted record entry for ");
    HTML(db);
    HTML("Where ");
    HTML(query);
    HTML("</BODY>");
    exit(0);
  };

  // ************************** viewers ************************** //

  // page with one slice //
  if (strcmp(mode, "sliceviewer")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<A HREF=dgate%s?%s&mode=headerdump&slice=%s>", ex, extra, slice);
    if (size[0])
      HTML("<IMG SRC=dgate%s?%s&mode=slice&slice=%s&size=%s&graphic=%s&lw=%s HEIGHT=%s>", ex, extra, slice, dsize, graphic, lw, size);
    else
      HTML("<IMG SRC=dgate%s?%s&mode=slice&slice=%s&size=%s&graphic=%s&lw=%s>", ex, extra, slice, dsize, graphic, lw);
    HTML("</A>");
    HTML("</BODY>");
    exit(0);
  };

  if (strcmp(mode, "headerdump")==0)
  { HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
    HTML("<BR>Header dump of DICOM object:<BR>");
    HTML("<PRE>");

    sprintf(command, "dump_header:%s", slice2);
    SendServerCommand("", command, console, NULL, FALSE);

    HTML("</PRE>");
    HTML("</BODY>");
    exit(0);
  };

  // just generates the bitmap //
  if (strcmp(mode, "slice")==0)
  { sprintf(command, "convert_to_%s:%s,%s,cgi,%s", graphic, slice2, size, lw); // 1.4.16i: use size instead of dsize
    SendServerCommand("", command, console, NULL, FALSE);
    exit(0);
  };

  // transmits the image contents in dicom format //
  if (strcmp(mode, "dicom")==0)
  { sprintf(command, "convert_to_dicom:%s,%s,%s", slice2, dsize, compress);
    SendServerCommand("", command, console, NULL, FALSE);
    exit(0);
  };

  // transmits the series in zipped dicom format //
  if (strcmp(mode, "zipseries")==0)
  { char *p = strchr(series2, ':');
    if (p) 
    { *p++ = ',';
      memmove(p, p-1, strlen(p-1)+1);
      *p++ = ',';
    }

    sprintf(command, "export:%s,,cgi,call zip.cq", series2);
    SendServerCommand("", command, console, NULL, FALSE);
    exit(0);
  };

  // transmits the study in zipped dicom format //
  if (strcmp(mode, "zipstudy")==0)
  { char *p = strchr(study2, ':');
    if (p) *p = ',';

    sprintf(command, "export:%s,,,cgi,call zip.cq", study2);
    SendServerCommand("", command, console, NULL, FALSE);
    exit(0);
  };

  // transmits the image list with urls in text format //
  if (strcmp(mode, "imagelisturls")==0)
  { char *p;

    p = strchr(series2, ':');
    if (p) *p = '|';

    sprintf(command, "imagelister:local|%s|:%s?%s&mode=dicom&slice=%%s:%%s&dsize=%s&compress=%s&dum=.dcm|cgi", series2, iniValuePtr->sscscpPtr->WebScriptAddress, extra, dsize, compress);
    SendServerCommand("", command, console, NULL, FALSE);
    exit(0);
  };

  // transmits the image list with http references in text format //
  if (strcmp(mode, "imagelisthttp")==0)
  { char *p;

    p = strchr(series2, ':');
    if (p) *p = '|';

    sprintf(command, "imagelister:local|%s|@%s/%%0.0s%%s*|cgi", series2, iniValuePtr->sscscpPtr->WebMAG0Address);
    SendServerCommand("", command, console, NULL, FALSE);
    exit(0);
  };

  // transmits the image list with filenames in text format //
  if (strcmp(mode, "imagelistfiles")==0)
  { char *p;

    p = strchr(series2, ':');
    if (p) *p = '|';

    sprintf(command, "imagelister:local|%s|%%s|cgi", series2);// %% warning, bcb -> ok but wrong change ;->>> mvh
//	sprintf(command, "imagelister:local|%s|%%s|cgi", series2, iniValuePtr->sscscpPtr->WebMAG0Address);
    SendServerCommand("", command, console, NULL, FALSE);
    exit(0);
  };

  // k-pacs viewer in an OCX; internet explorer only //
  if (strcmp(mode, "seriesviewer")==0)
  { char *p;
  
    if (size[0]==0) strcpy(size, "80%25%25");

    HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);

    HTML("<OBJECT");
    HTML("          classid=\"clsid:0AA480F2-78EB-4A31-B4C8-F981C5004BBB\"");
    HTML("          codebase=\"%sActiveFormProj1.ocx\"", iniValuePtr->sscscpPtr->WebCodeBase);
    HTML("          width=%s", size);
    HTML("          height=%s", size);
    HTML("          align=center");
    HTML("          hspace=0");
    HTML("          vspace=0");
    HTML(">");
    HTML("<PARAM name=DCMFilelist value=");

    p = strchr(series2, ':');
    if (p) *p = '|';
    sprintf(command, "imagelister:local|%s|:%s?%s&mode=dicom&slice=%%s:%%s&dsize=%s&compress=%s*", series2, iniValuePtr->sscscpPtr->WebScriptAddress, extra, dsize, compress);
    SendServerCommand("", command, console);

    HTML(">");
    HTML("</OBJECT>");
    HTML("</BODY>");
    exit(0);
  };

  // k-pacs viewer in an OCX; internet explorer only; data access through http served image files //
  if (strcmp(mode, "seriesviewer2")==0)
  { char wwwserveraddress[512] = "http://";
    strcat(wwwserveraddress, getenv("SERVER_NAME"));

    if (size[0]==0) strcpy(size, "80%25%25");

    HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
    HTML("<OBJECT");
    HTML("          classid=\"clsid:0AA480F2-78EB-4A31-B4C8-F981C5004BBB\"");
    HTML("          codebase=\"%sActiveFormProj1.ocx\"", iniValuePtr->sscscpPtr->WebCodeBase);
    HTML("          width=%s", size);
    HTML("          height=%s", size);
    HTML("          align=center");
    HTML("          hspace=0");
    HTML("          vspace=0");
    HTML(">");
    HTML("<PARAM name=DCMFilelist value=");

    char *p = strchr(series2, ':');
    if (p) *p = '|';

    if (memcmp(wwwserveraddress, "file", 4)==0)
      sprintf(command, "imagelister:local|%s", series2);
    else
      sprintf(command, "imagelister:local|%s|@%s/%%0.0s%%s*", series2, iniValuePtr->sscscpPtr->WebMAG0Address);

    SendServerCommand("", command, console);

    HTML(">");
    HTML("</OBJECT>");
    HTML("</BODY>");
    exit(0);
  };

  // no viewer //
  if (strcmp(mode, "noviewer")==0)
  { //char *p;

    HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
    HTML("<HR>");
    HTML("<H2>No viewer is installed</H2>");
    HTML("</OBJECT>");
    HTML("</BODY>");
    exit(0);
  };

  // The Japanese java-based viewer; requires some modifications to work //
  if (strcmp(mode, "aiviewer")==0)
  { char *p;

    HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s with AiViewer</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s with AiViewer</H2>", DGATE_VERSION);

    HTML("<APPLET");
    HTML("  CODEBASE = '%sdicomviewer100'", iniValuePtr->sscscpPtr->WebCodeBase);
    HTML("  CODE = 'dicomviewer.Viewer.class'");
    HTML("  NAME = 'Viewer.java'");
    HTML("  WIDTH = '100%%'");
    HTML("  HEIGHT = '90%%'");
    HTML("  HSPACE = 0");
    HTML("  VSPACE = 0");
    HTML("  ALIGN = middle >");
    HTML("<PARAM NAME = tmpSize VALUE = 1>");
    HTML("<PARAM NAME = NUM VALUE = 0>");
    HTML("<PARAM NAME = currentNo VALUE = 0>");
    HTML("<PARAM NAME = dicURL VALUE = '%sdicomviewer100/Dicom.dic'>", iniValuePtr->sscscpPtr->WebCodeBase);
    HTML("<PARAM NAME = imgURL0 VALUE = ");

    p = strchr(series2, ':');
    if (p) *p = '|';
    sprintf(command, "imagelister:local|%s|:%s?%s&mode=dicom&slice=%%s:%%s&dsize=%s&compress=%s*", series2, iniValuePtr->sscscpPtr->WebScriptAddress, extra, dsize, "un");
    SendServerCommand("", command, console);

    HTML(">");

    HTML("</APPLET>");
    HTML("</BODY>");
    exit(0);
  };

  // very simple jave-script based viewer with server side processing //
  if (strcmp(mode, "serversideviewer")==0)
  { char *p;

    p = strchr(series2, ':');
    if (p) *p = '|';

    if (size[0]==0) strcpy(size, "80%25%25");

    HTML("Content-type: text/html\n");
    HTML("<HEAD><TITLE>Conquest DICOM server - version %s server side viewer</TITLE></HEAD>", DGATE_VERSION);
    HTML("<BODY BGCOLOR='CFDFCF'>");
    HTML("<H2>Conquest DICOM server - version %s server side viewer</H2>", DGATE_VERSION);

    HTML("<SCRIPT language=JavaScript>");
    char s[] = "var frames, nframes ='";
    write(console, s, strlen(s));
    sprintf(command, "imagelister:local|%s|$%%04d", series2);
    SendServerCommand("", command, console);
    HTML("';");
    HTML("function load()");
    HTML("{ document.images[0].src = 'dgate%s?%s&mode=slice'+", ex, extra);
    HTML("                           '&graphic=' + document.forms[0].graphic.value +");
    HTML("                           '&slice='   + document.forms[0].slice.value +");
    HTML("                           '&dsize=%s' +", dsize);
    HTML("                           '&lw='      + document.forms[0].level.value + '/' + document.forms[0].window.value + '/' + document.forms[0].frame.value;");
    HTML("  frames = parseInt(nframes.slice(document.forms[0].slice.selectedIndex*4, document.forms[0].slice.selectedIndex*4+4), 10);");
    HTML("  if (frames < 2) document.forms[0].frame.style.visibility = 'hidden'; else document.forms[0].frame.style.visibility = 'visible';");
    HTML("  if (frames < 2) document.forms[0].nframe.style.visibility = 'hidden'; else document.forms[0].nframe.style.visibility = 'visible';");
    HTML("  if (frames < 2) document.forms[0].pframe.style.visibility = 'hidden'; else document.forms[0].pframe.style.visibility = 'visible';");
    HTML("}");
    HTML("function nextslice()");
    HTML("{ if (document.forms[0].slice.selectedIndex == document.forms[0].slice.length-1) document.forms[0].slice.selectedIndex = 0; else document.forms[0].slice.selectedIndex = document.forms[0].slice.selectedIndex + 1; load()");
    HTML("}");
    HTML("function nextframe()");
    HTML("{ if (parseInt(document.forms[0].frame.value, 10) == frames-1) document.forms[0].frame.value = 0; else document.forms[0].frame.value = parseInt(document.forms[0].frame.value, 10)+1; load()");
    HTML("}");
    HTML("function prevframe()");
    HTML("{ document.forms[0].frame.value = parseInt(document.forms[0].frame.value, 10)-1; if (document.forms[0].frame.value<0) document.forms[0].frame.value = 0; load()");
    HTML("}");
    HTML("</SCRIPT>");

    HTML("<IMG SRC=loading.jpg BORDER=0 HEIGHT=%s>", size);

    HTML("<FORM>");
    HTML("Slice: ");
    HTML("  <select name=slice onchange=load()>");

    sprintf(command, "imagelister:local|%s|:<option value=%%s:%%s>%%s</option>", series2);
    SendServerCommand("", command, console);

    HTML("  </select>");
    HTML("  <INPUT TYPE=BUTTON VALUE='>' onclick=nextslice() >");

    HTML("Level:");
    HTML("  <INPUT name=level size=4 onchange=load() >");
    HTML("Window:");
    HTML("  <INPUT name=window size=4 onchange=load() >");
    HTML("Frame:");
    HTML("  <INPUT name=pframe TYPE=BUTTON VALUE='<' onclick=prevframe() >");
    HTML("  <INPUT name=frame size=3 onchange=load() value=0>");
    HTML("  <INPUT name=nframe TYPE=BUTTON VALUE='>' onclick=nextframe() >");
    HTML("Color:");
    HTML("  <select name=graphic onchange=load()>");
    if (strcmp(graphic, "bmp")==0)
    { HTML("    <option value=bmp>yes</option>");
      HTML("    <option value=gif>no</option>");
      HTML("    <option value=jpg>jpg</option>");
    }
    else if (strcmp(graphic, "gif")==0)
    { HTML("    <option value=gif>no</option>");
      HTML("    <option value=bmp>yes</option>");
      HTML("    <option value=jpg>jpg</option>");
    }
    else if (strcmp(graphic, "jpg")==0)
    { HTML("    <option value=jpg>jpg</option>");
      HTML("    <option value=bmp>yes</option>");
      HTML("    <option value=gif>no</option>");
    }
    HTML("  </select>");
    HTML("  <INPUT TYPE=BUTTON VALUE=load onclick=load() >");
    HTML("</FORM>");

    HTML("<SCRIPT language=JavaScript>");
    HTML("  document.onload=load()");
    HTML("</SCRIPT>");

    HTML("</BODY>");
    exit(0);
  };

  // ************************** general purpose web scripting ************************** //

/*
This is a general purpose web script processor; it can be used to create any web page, not just viewers.

This is a sample from dicom.ini:

[flexviewer]
line0 = <HEAD><TITLE>Conquest DICOM server - version %version% and %windowname%</TITLE></HEAD>
line1 = <BODY BGCOLOR='CFDFCF'>
line2 = <H2>Conquest DICOM server - version %version% and AiViewer v1.00</H2>
line3 = <APPLET CODEBASE=%webcodebase%dicomviewer100 CODE='dicomviewer.Viewer.class'
line4 = NAME='Viewer.java' WIDTH='100%' HEIGHT='90%' HSPACE=0 VSPACE=0 ALIGN=middle >
line5 = <PARAM NAME = tmpSize VALUE = 10>
line6 = <PARAM NAME = NUM VALUE = 0>
line7 = <PARAM NAME = currentNo VALUE = 0>
line8 = <PARAM NAME = dicURL VALUE = '%webcodebase%dicomviewer100/Dicom.dic'>
line9 = <PARAM NAME = imgURL0 VALUE = 
line10 = --imagelister:local|%patid%|%seruid%|:%webscriptadress%?%extra%&mode=dicom&slice=%s:%s&compress=un*
line11 = >
line12 = </APPLET>
line13 = </BODY>
# this is the default; the variable can be passed in the url
windowname = AiViewer V1.00

[flexviewer2]
source = flexviewer2.cq
windowname = AiViewer V1.00

;[AnyPage]
;source = anypage.cq

;[DefaultPage]
;source = ecrf\*.lua
*/

  // check mode of web page against dicom.ini sections //
  char *bufPtr;
  buf[0]=0;
  if((bufPtr = iniValuePtr->GetIniString("line0", mode)) == NULL && // Drops out if found
    (bufPtr = iniValuePtr->GetIniString("source", mode)) == NULL)
  {
    strcpy(mode, DefaultPage);
    if(iniValuePtr->DefaultPagePtr != NULL && iniValuePtr->DefaultPagePtr->source[0] != 0)
    {// No mode value, but we have a DefaultPage section.
/*      if(iniValuePtr->DefaultPagePtr->linesPtr != NULL)//Killed line0,1,...
        bufPtr = iniValuePtr->DefaultPagePtr->linesPtr->Get(0);
      else */bufPtr = iniValuePtr->DefaultPagePtr->source;
        if(bufPtr)strcpylim(buf, bufPtr, 512);
    }
  }
  else
  {
    if(bufPtr)strcpylim(buf, bufPtr, 512);
    free(bufPtr);
  }
  // if source contains a * substitute the mode //
  char *p = strchr(buf, '*');
  if (p)
  { char tmp[256];
    *p=0;
    CGI(tmp, "mode", "");
    strcat(tmp, p+1);
    strcat(buf, tmp);
  }

  if (buf[0])
  { char string[1000], temp[1000], server[1000], patid[66], patid2[200], seruid[66], studyuid[66], sopuid[66], chunk[8192];
    FILE *f=NULL;

    // create several variables useful for scripting //

    sprintf(server, "%s?%s", iniValuePtr->sscscpPtr->WebScriptAddress, extra);

    patid[0] = seruid[0] = studyuid[0] = sopuid[0] = 0;

    if (study2[0])
    { strcpy(temp, study2);
      p = strchr(temp, ':');
      if (p) 
      { *p = 0;
        strcpy(patid2, temp);
        *p = '|';
        strcpy(studyuid, p+1);
      }
    
      strcpy(temp, study);
      p = strchr(temp, ':');
      if (p) 
      { *p = 0;
        strcpy(patid, temp);
      }
    }

    if (series2[0])
    { strcpy(temp, series2);
      p = strchr(temp, ':');
      if (p) 
      { *p = 0;
        strcpy(patid2, temp);
        *p = '|';
        strcpy(seruid, p+1);
      }
    
      strcpy(temp, series);
      p = strchr(temp, ':');
      if (p) 
      { *p = 0;
        strcpy(patid, temp);
      }
    }

    if (slice2[0])
    { strcpy(temp, slice2);
      p = strchr(temp, ':');
      if (p) 
      { *p = 0;
        strcpy(patid2, temp);
        *p = '|';
        strcpy(sopuid, p+1);
      }
    
      strcpy(temp, slice);
      p = strchr(temp, ':');
      if (p) 
      { *p = 0;
        strcpy(patid, temp);
      }
    }

    // contents may come from file (lua extension runs lua script) or from dicom.ini //
//  MyGetPrivateProfileString ( mode, "source", buf, temp, 1000, ConfigFile); //buf is the default, we get temp if mode:source is real
//  strcpy(temp, buf); //bcb? We may have gotten temp, but we just overwrite it?
    if((bufPtr = iniValuePtr->GetIniString("source", mode)) != NULL) //If there.
    { strcpylim(temp, bufPtr, sizeof(temp)); // Make it temp.
      free(bufPtr);
    }
    else strcpy(temp, buf);// or the default buf.
    if (strcmp(temp+strlen(temp)-4, ".lua")==0)
    { if((bufPtr = iniValuePtr->GetIniString("header", mode)) != NULL)// This all could be done nicer.
      { strcpylim(buf, bufPtr, sizeof(buf));// bcb? bufPtr is mine now, do I need to copy.  Does HTML need a fixed buffer size?
        free(bufPtr);
      }
      else buf[0] = 0;
      while ((p = strstr(buf, "\\"))) p[0]='\n';// By moving this inside of if().
      if (buf[0]) HTML(buf);// and this
//    if (bufPtr != NULL))// bcb? Does this look better, does buf need to change?
//    { while ((p = strstr(bufPtr, "\\"))) p[0]='\n'
//      HTML(bufPtr);
//      free(bufPtr);
//    }
    
      struct scriptdata lsd1 = {&globalPDU, NULL, NULL, -1, NULL, NULL, NULL, NULL, NULL, 0, 0};
      char script[512]; int i1, n;
      strcpy(script, "dofile('");
      for (i1=0, n=8; i1<strnlenint(temp, sizeof(temp)); i1++)
      { if (temp[i1]=='\\')
        { script[n++] = '\\';
          script[n++] = '\\';
	}
	else 
	{ script[n++] = temp[i1];
	}
      }
      script[n++] = 0;
      strcat(script, "')");
      OperatorConsole.On();

      lua_setvar(&globalPDU, "query_string",    getenv( "QUERY_STRING"));      
      lua_setvar(&globalPDU, "server_name",     getenv( "SERVER_NAME" ));
      lua_setvar(&globalPDU, "script_name",     getenv( "SCRIPT_NAME" ));
      lua_setvar(&globalPDU, "path_translated", getenv( "PATH_TRANSLATED" ));
      lua_setvar(&globalPDU, "port",            iniValuePtr->sscscpPtr->Port);
      lua_setvar(&globalPDU, "address",         ServerCommandAddress);
      lua_setvar(&globalPDU, "webcodebase",     iniValuePtr->sscscpPtr->WebCodeBase);
      lua_setvar(&globalPDU, "webscriptadress", iniValuePtr->sscscpPtr->WebScriptAddress);
      lua_setvar(&globalPDU, "extra",           extra);
      lua_setvar(&globalPDU, "version",         DGATE_VERSION);
      lua_setvar(&globalPDU, "mode",            mode);
      lua_setvar(&globalPDU, "uploadedfile",    uploadedfile);
          
      lua_setvar(&globalPDU, "series",          series);
      lua_setvar(&globalPDU, "series2",         series2);
      lua_setvar(&globalPDU, "slice",           slice);
      lua_setvar(&globalPDU, "slice2",          slice2);
      lua_setvar(&globalPDU, "study",           study);
      lua_setvar(&globalPDU, "study2",          study2);
      lua_setvar(&globalPDU, "patid",           patid);
      lua_setvar(&globalPDU, "patid2",          patid2);
      lua_setvar(&globalPDU, "seruid",          seruid);
      lua_setvar(&globalPDU, "studyuid",        studyuid);
      lua_setvar(&globalPDU, "sopuid",          sopuid);
	  
      lua_setvar(&globalPDU, "size",            size);
      lua_setvar(&globalPDU, "dsize",           dsize);
      lua_setvar(&globalPDU, "compress",        compress);
      lua_setvar(&globalPDU, "iconsize",        iconsize);
      lua_setvar(&globalPDU, "graphic",         graphic);
      lua_setvar(&globalPDU, "viewer",          viewer);
      lua_setvar(&globalPDU, "lw",              lw);
      lua_setvar(&globalPDU, "query",           query);
      lua_setvar(&globalPDU, "db",              db);
      lua_setvar(&globalPDU, "source",          source);
      lua_setvar(&globalPDU, "dest",            dest);
      lua_setvar(&globalPDU, "patientidmatch",  patientidmatch);
      lua_setvar(&globalPDU, "patientnamematch",patientnamematch);
      lua_setvar(&globalPDU, "studydatematch",  studydatematch);
      lua_setvar(&globalPDU, "startdatematch",  startdatematch);

      do_lua(&(globalPDU.L), script, &lsd1);

      exit(0);
    }

    if (temp[0]) f = fopen(temp, "rt");

    char *tempPtr;
    tempPtr = iniValuePtr->GetIniString("header", mode);
    if(tempPtr != NULL)
    {
      strcpylim(temp, tempPtr, 1000);
      free(tempPtr);
    }
    else strcpy(temp, "Content-type: text/html\n");
    while ((p = strstr(temp, "\\"))) p[0]='\n';
    HTML(temp);
    
    int inlua=0;

    for (i=0; i<10000; i++)
    { if (f)
      { if (fgets(string, sizeof(string), f) == NULL)
          break;
        if (!inlua && string[strlen(string)-1]=='\n') string[strlen(string)-1]=0;
      }
      else
      { sprintf(temp, "line%d", i);
        if((tempPtr = iniValuePtr->GetIniString(temp, mode)) != NULL)
        {
          strcpylim(string, tempPtr, 1000);
          free(tempPtr);
        }
        else string[0]=0;
        if (string[0]==0) break;
      }
      
      if (inlua)
      { char *p3 = strstr(string, "?>");
        if (p3)
	{ *p3=0;
	  strcat(chunk, string);
	  inlua=0;
          struct scriptdata lsd1 = {&globalPDU, NULL, NULL, -1, NULL, NULL, NULL, NULL, NULL, 0, 0};

          lua_setvar(&globalPDU, "query_string",    getenv( "QUERY_STRING"));      
          lua_setvar(&globalPDU, "server_name",     getenv( "SERVER_NAME" ));
          lua_setvar(&globalPDU, "script_name",     getenv( "SCRIPT_NAME" ));
          lua_setvar(&globalPDU, "path_translated", getenv( "PATH_TRANSLATED" ));
          lua_setvar(&globalPDU, "port",            iniValuePtr->sscscpPtr->Port);
          lua_setvar(&globalPDU, "address",         ServerCommandAddress);
          lua_setvar(&globalPDU, "webcodebase",     iniValuePtr->sscscpPtr->WebCodeBase);
          lua_setvar(&globalPDU, "webscriptadress", iniValuePtr->sscscpPtr->WebScriptAddress);
          lua_setvar(&globalPDU, "webscriptaddress", iniValuePtr->sscscpPtr->WebScriptAddress);
          lua_setvar(&globalPDU, "extra",           extra);
          lua_setvar(&globalPDU, "version",         DGATE_VERSION);
          lua_setvar(&globalPDU, "mode",            mode);
	  lua_setvar(&globalPDU, "uploadedfile",    uploadedfile);
          
          lua_setvar(&globalPDU, "series",          series);
          lua_setvar(&globalPDU, "series2",         series2);
          lua_setvar(&globalPDU, "slice",           slice);
          lua_setvar(&globalPDU, "slice2",          slice2);
          lua_setvar(&globalPDU, "study",           study);
          lua_setvar(&globalPDU, "study2",          study2);
          lua_setvar(&globalPDU, "patid",           patid);
          lua_setvar(&globalPDU, "patid2",          patid2);
          lua_setvar(&globalPDU, "seruid",          seruid);
          lua_setvar(&globalPDU, "studyuid",        studyuid);
          lua_setvar(&globalPDU, "sopuid",          sopuid);
	  
	  lua_setvar(&globalPDU, "size",            size);
          lua_setvar(&globalPDU, "dsize",           dsize);
          lua_setvar(&globalPDU, "compress",        compress);
          lua_setvar(&globalPDU, "iconsize",        iconsize);
          lua_setvar(&globalPDU, "graphic",         graphic);
          lua_setvar(&globalPDU, "viewer",          viewer);
          lua_setvar(&globalPDU, "lw",              lw);
          lua_setvar(&globalPDU, "query",           query);
          lua_setvar(&globalPDU, "db",              db);
          lua_setvar(&globalPDU, "source",          source);
          lua_setvar(&globalPDU, "dest",            dest);
          lua_setvar(&globalPDU, "patientidmatch",  patientidmatch);
          lua_setvar(&globalPDU, "patientnamematch",patientnamematch);
          lua_setvar(&globalPDU, "studydatematch",  studydatematch);
          lua_setvar(&globalPDU, "startdatematch",  startdatematch);

          do_lua(&(globalPDU.L), chunk, &lsd1);
	}
	else
	  strcat(chunk, string);
      }

      // fill in predefined scripting variables //

      replace(string, "%query_string%",    getenv( "QUERY_STRING" ));
      replace(string, "%server_name%",     getenv( "SERVER_NAME" ));
      replace(string, "%script_name%",     getenv( "SCRIPT_NAME" ));
      replace(string, "%path_translated%", getenv( "PATH_TRANSLATED" ));
      replace(string, "%uploadedfile%",    uploadedfile);

      replace(string, "%port%",            iniValuePtr->sscscpPtr->Port);
      replace(string, "%address%",         ServerCommandAddress);
      replace(string, "%webcodebase%",     iniValuePtr->sscscpPtr->WebCodeBase);
      replace(string, "%webscriptadress%", iniValuePtr->sscscpPtr->WebScriptAddress);
      replace(string, "%extra%",           extra);
      replace(string, "%server%",          server);
      replace(string, "%version%",         DGATE_VERSION);
      replace(string, "%mode%",            mode);

      replace(string, "%series%",          series2); // unprocessed
      replace(string, "%series2%",         series);  // replaced spaces by %20
      replace(string, "%slice%",           slice2);  // unprocessed
      replace(string, "%slice2%",          slice);   // replaced spaces by %20
      replace(string, "%study%",           study2);  // unprocessed
      replace(string, "%study2%",          study);   // replaced spaces by %20
      replace(string, "%patid%",           patid2);  // unprocessed
      replace(string, "%patid2%",          patid);   // replaced spaces by %20
      replace(string, "%seruid%",          seruid);
      replace(string, "%studyuid%",        studyuid);
      replace(string, "%sopuid%",          sopuid);

      replace(string, "%size%",            size);
      replace(string, "%dsize%",           dsize);
      replace(string, "%compress%",        compress);
      replace(string, "%iconsize%",        iconsize);
      replace(string, "%graphic%",         graphic);
      replace(string, "%viewer%",          viewer);
      replace(string, "%lw%",              lw);

      replace(string, "%query%",           query);
      replace(string, "%db%",              db);
      replace(string, "%source%",          source);
      replace(string, "%dest%",            dest);
      replace(string, "%patientidmatch%",  patientidmatch);
      replace(string, "%patientnamematch%",patientnamematch);
      replace(string, "%studydatematch%",  studydatematch);
      replace(string, "%startdatematch%",  startdatematch);

      //       this code will substitute any other %var% with a cgi variable 
      //         with a default given in section for this server mode in dicom.ini 
      //	 
      //	 or substitute ... with the string result of a lua expression
      //	 <%= .... %>
      
      
      char *p4 = strstr(string, "<%=");
      if (p4)
      { char *p2 = strstr(string, "%>");
        char script[1000];
        *p4=0;
        *p2=0;
        struct scriptdata lsd1 = {&globalPDU, NULL, NULL, -1, NULL, NULL, NULL, NULL, NULL, 0, 0};
	strcpy(script, "return ");
	strcat(script, p4+3);
        HTML("%s", string);
        HTML("%s", do_lua(&(globalPDU.L), script, &lsd1));
	HTML("%s", p2+2);
	string[0] = '#';
      }
      else
      { char *p2 = strchr(string, '%');
        if (p2)
        { char *q = strchr(p2+1, '%');
          if (q && q!=p2+1)
          { char var[512], val[512], var2[512];
            *q=0;
            strcpy(var, p2+1);
            *q='%';;
            strcpy(var2, "%");
            strcat(var2, var);
            strcat(var2, "%");

            if((tempPtr = iniValuePtr->GetIniString(var, mode)) != NULL)
            { strcpylim(val, tempPtr, 512);
              free(tempPtr);
            }
            else strcpylim(val, var2, 512);
            CGI(val, var, val);
            replace(string, var2, val);
          }
        }
      }

      // runs: #comment, --servercommand, <?lua .... ?> as lua, or straight HTML output //
      if      (!inlua && string[0]=='#')                   strcpy(string, "");
      else if (!inlua && string[0]=='-' && string[1]=='-') SendServerCommand("", string+2, console);
      else if (!inlua && (p=strstr(string, "<?lua")))      {*p=0; HTML("%s", string); inlua=1; strcpy(chunk, p+5); strcat(chunk, "\n");}
      else if (!inlua)                                     HTML("%s", string);
    };

    if (f) fclose(f);
    exit(0);
  };

  exit(0);
}

/* Dicom web viewer works as follows:

client		-> 	webserver			url		http://127.0.0.1/scripts/dgate.exe?mode=seriesviewer&series=...
webserver 	-> 	dicomserver 			query		imagelister:local|.....
client  	<- 	webserver <- dicomserver	query results	(to build list of urls of dicom slices)
client  	<- 	webserver			activex control http://127.0.0.1/ActiveFormProj1.ocx

Then for each slice that is required:

control 	-> 	webserver			url of slice	http://127.0.0.1/scripts/dgate.exe?mode=dicom&slice=......
webserver	-> 	dicomserver			dicom request   convert_to_dicom:....
control 	<- 	webserver <- dicomserver	dicom data

*/

static BOOL DgateWADO(char *query_string, char *ext)
{ UNUSED_ARGUMENT(query_string);
  UNUSED_ARGUMENT(ext);

  char requestType[256];
  CGI(requestType,   "requestType",    "");		// is this a WADO request?
  if (strcmp(requestType, "WADO")!=0) return FALSE;

  char studyUID[256];
  CGI(studyUID,   "studyUID",    "");

  char seriesUID[256];
  CGI(seriesUID,   "seriesUID",    "");

  char objectUID[256];
  CGI(objectUID,   "objectUID",    "");

  char contentType[256];
  CGI(contentType,   "contentType",    "");

  char rows[256];
  CGI(rows,   "rows",    "");

  char columns[256];
  CGI(columns,   "columns",    "");

  char region[256];
  CGI(region,   "region",    "");
  for (unsigned int i=0; i<strlen(region); i++) if (region[i]==',') region[i] = '/';

  char windowCenter[256];
  CGI(windowCenter,   "windowCenter",    "");

  char windowWidth[256];
  CGI(windowWidth,   "windowWidth",    "");

  char frameNumber[256];
  CGI(frameNumber,   "frameNumber",    "");

  char transferSyntax[256];
  CGI(transferSyntax,   "transferSyntax",    "");

  char anonymize[256];
  CGI(anonymize,   "anonymize",    "");

  char annotation[256];
  CGI(annotation,   "annotation",    "");
  for (unsigned int i=0; i<strlen(annotation); i++) if (annotation[i]==',') annotation[i] = '/';

  char imageQuality[256];
  CGI(imageQuality,   "imageQuality",    "");

  char charset[256];
  CGI(charset,   "charset",    "");   // ignored for now

  char presentationUID[256];
  CGI(presentationUID,   "presentationUID",    ""); // ignored for now
  
  char bridge[256];
  IniValue *iniValuePtr = IniValue::GetInstance();
  if(iniValuePtr->wadoserversPtr != NULL && iniValuePtr->wadoserversPtr->bridge != NULL)
          strcpylim( bridge, iniValuePtr->wadoserversPtr->bridge, 256);
  else bridge[0] = 0;
  CGI(bridge,   "bridge",    bridge);

  console = fileno(stdout);
#ifndef UNIX
  setmode(console, O_BINARY);
#endif

  LoadKFactorFile(iniValuePtr->sscscpPtr->kFactorFile);

  // If iniValuePtr->webdefaults->address exists, it is copied to iniValuePtr->sscscpPtr->WebServerFor when the ini file is read.
  if(iniValuePtr->sscscpPtr->WebServerFor[0] == 0)
    sprintf(iniValuePtr->sscscpPtr->WebServerFor, "127.0.0.1");
  strcpy(ServerCommandAddress, iniValuePtr->sscscpPtr->WebServerFor);

  char obj[256];
  sprintf(obj, "%s\\%s\\%s", studyUID, seriesUID, objectUID);

  char lwfq[256];
  sprintf(lwfq, "%d/%d/%d/%d", atoi(windowCenter), atoi(windowWidth)/2, atoi(frameNumber), atoi(imageQuality));

  char size[256];
  sprintf(size, "%d/%d", atoi(rows), atoi(columns));

  char command[512];
  sprintf(command, "wadorequest:%s,%s,%s,%s,%s,%s,%s,%s,%s", obj, lwfq, size, region, contentType, transferSyntax, anonymize, annotation, bridge);

#if 1
  SendServerCommand("", command, console, NULL, FALSE);
#else
  HTML("Content-type: text/html\n");
  HTML("<HEAD><TITLE>Conquest DICOM server - version %s</TITLE></HEAD>", DGATE_VERSION);
  HTML("<BODY BGCOLOR='CFDFCF'>");
  HTML("<H2>Conquest DICOM server - version %s</H2>", DGATE_VERSION);
  HTML("<HR>");
  HTML("<H2>WADO is not yet available</H2>");
  HTML("command: %s", command);
  HTML("</OBJECT>");
  HTML("</BODY>");
#endif

  exit(0);
};

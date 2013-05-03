//
//  dgatehl7.cpp
//  SueliXx86
//
/*
bcb 20120710  Created
bcb 20130226  Replaced gpps with IniValue class.  Removed all globals now in IniValue class.
                Version to 1.4.18a.
*/


#include "dgatehl7.hpp"
#include "dgate.hpp"
#include "configpacs.hpp"

extern int QueryFromGui;
extern DBENTRY	*WorkListDB;

//////////////////////////////////////////////////////////////////
// Elementary HL7 interface
//////////////////////////////////////////////////////////////////

// These contain HL7 DateTime code: provide .DATE and .TIME split, note: should start/end with |

static char HL7DateTimeTypes[]=
"|OBR.7|OBR.8|OBR.22|PID.7|PID.29|PV1.44|PV1.45|TXA.4|TXA.6|TXA.7|TXA.8|OBR.6|OBR.36|MSH.6|OBX.12|OBX.14|ORC.9|ORC.15|";

// read next item from data passed in p; data->item, type->name, tmp->internal
// note that data, type, and tmp are also used to maintain the parser state

void parseHL7(char **p, char *data, char *type, char *tmp, char *HL7FieldSep, char *HL7SubFieldSep, char *HL7RepeatSep)
{ int field;
  char *q;
  char t[32];
  unsigned int i, dots=0;

  sprintf(t, "|%s|", type);								// to seek types
  if (strlen(type)>2)									// count .N
    for (i=0; i<strlen(type)-1; i++) dots += (type[i]=='.' && type[i+1]>='0' && type[i+1]<='9');

  if (**p==0)
  { strcpy(type, "EOM");								// end of message
    data[0]=0;
  }
  else if (strstr(HL7DateTimeTypes, t))							// translate datetime type
  { tmp[0] = 0;
    if (strlen(data)>8) strcpy(tmp, data+8);						// get time for later use
    data[8]=0;
    strcat(type, ".DATE");								// XXX.N.DATE
  }
  else if (strstr(type, ".DATE")>0)							// date was returned, now get time
  { strcpy(data, tmp);									// time
    strcpy(type+strlen(type)-4, "TIME");						// XXX.N.TIME
  }
  else if (strchr(data, *HL7SubFieldSep))						// translate field type, first entry
  { q = strchr(data, *HL7SubFieldSep);
    if (q) *q=0;
    if (q) strcpy(tmp, q+1);
    else   tmp[0]=0;

    strcat(type, ".0");									// XXX.N.0
  }
  else if (strchr(data, *HL7RepeatSep))							// translate repeat type, first entry
  { q = strchr(data, *HL7RepeatSep);
    if (q) *q=0;
    if (q) strcpy(tmp, q+1);
    else   tmp[0]=0;

    strcat(type, ".0");									// XXX.N.0
  }
  else if (dots>1)									// process subfields/repeats
  { if (tmp[0]==0)
    { q = strrchr(type, '.');
      if (q) *q=0;
      strcat(type, ".END");								// internal token
      data[0]=0;
    }
    else
    { strcpy(data, tmp);
              q = strchr(data, *HL7SubFieldSep);
      if (!q) q = strchr(data, *HL7RepeatSep);
      if (q) *q=0;
      if (q) strcpy(tmp, q+1);
      else   tmp[0]=0;

      q = strrchr(type, '.');
      sprintf(q+1, "%d", atoi(q+1)+1);							// XXX.N.M
    }
  }
  else if (**p==0x0d)
  { strcpy(type, "EOS"); strcpy(data, ""); (*p)+=1;					// end of segment
    if (**p==0x0a) (*p)+=1;
    if (strncmp(*p, "MSH", 3)==0) strcpy(type, "EOM");					// peek ahead for end of message
  }
  else if (strcmp(type, "EOS")==0 || strcmp(type, "EOM")==0 || strcmp(type, "")==0)	// new segment
  { field = 0;
    if (strncmp(*p, "MSH", 3)==0)
    { *HL7FieldSep    = (*p)[3];
      *HL7SubFieldSep = (*p)[4];
      *HL7RepeatSep   = (*p)[5];
      strncpy(type, *p, 3); strcat(type, ".0");
      strncpy(data, *p, 3); 
      (*p)+=4; 
    }
    else if ((*p)[0]>='A' && (*p)[0] <='Z' && 						// message header 
        (*p)[1]>='A' && (*p)[1] <='Z' &&
        (((*p)[2]>='A' && (*p)[2] <='Z') || ((*p)[2]>='0' && (*p)[2] <='9')))
    { strncpy(type, *p, 3); strcat(type, ".0");
      strncpy(data, *p, 3); 
      (*p)+=4; 
    }
    else
    { strcpy(type, "UNK.0");
      data[0]=0;
    }
  }
  else
  { field = atoi(type+4);								// genererate new segment
    sprintf(type+4, "%d", field+1);

    q = strchr(*p, *HL7FieldSep);							// upto segment separator
    if (q)
    { *q=0;
      strncpy(data, *p, 255);
      data[255]=0;
      *q = *HL7FieldSep;
      *p = q+1;
    }
    else
    { q = strchr(*p, 0x0d);								// or 0x0d
      if (q)
      { *q = 0;
        strncpy(data, *p, 255);
        data[255]=0;
        *q = 0x0d;
        *p = q;										// process 0x0d again
      }
      else
      { strcpy(data, "");
        strcpy(type, "ERR");								// internal token
      }
    }
  }

  SystemDebug.printf("HL7 item: %s, contents: %s\n", type, data);  
}

// load HL7 data into modality worklist using the translation table from dicom.sql

void ProcessHL7Data(char *data)
{ char *p=data;
  char fields[1024], values[4096], type[32], item[256], uid[66], tmp[256];//, command[8192];
  int i;
  
  char HL7FieldSep    = '|';
  char HL7SubFieldSep = '^';
  char HL7RepeatSep   = '~';

  IniValue *iniValuePtr = IniValue::GetInstance();
  Database DB;
  if (!DB.Open ( iniValuePtr->sscscpPtr->DataSource, iniValuePtr->sscscpPtr->UserName,
    iniValuePtr->sscscpPtr->Password, iniValuePtr->sscscpPtr->DataHost ) ) 
  { OperatorConsole.printf("*** HL7 import: cannot open database");
    return;
  }

  fields[0]=0;
  values[0]=0;
  type[0]=0;	// used for context of parser
  tmp[0]=0;

  while (TRUE)
  { parseHL7(&p, item, type, tmp, &HL7FieldSep, &HL7SubFieldSep, &HL7RepeatSep);
    if (strcmp(type, "ERR")==0) break;			// error

    // search type in database; if found prepare strings to write
    i = 0;
    while ( TRUE )
    { if(!WorkListDB[i].Element) break;
      if (WorkListDB[i].DICOMType!=DT_STARTSEQUENCE && WorkListDB[i].DICOMType!=DT_ENDSEQUENCE)
      {	if (strcmp(type, WorkListDB[i].HL7Tag)==0)
        { strcat(fields, WorkListDB[i].SQLColumn);
          strcat(fields, ",");

          strcat(values, "'");
          item[WorkListDB[i].SQLLength]=0;		// truncate data to make fit
          strcat(values, item);
          strcat(values, "',");
        }
      }
      ++i;
    }

    // on end of message save the data into the database

    if (strcmp(type, "EOM")==0)
    { // search for special items (start with *) that are not read from the HL7 parser
      i = 0;
      while ( TRUE )
      { if(!WorkListDB[i].Element) break;

        if (WorkListDB[i].DICOMType!=DT_STARTSEQUENCE && WorkListDB[i].DICOMType!=DT_ENDSEQUENCE)
        { if (WorkListDB[i].HL7Tag[0]=='*')
          { strcat(fields, WorkListDB[i].SQLColumn);
            strcat(fields, ",");

            strcat(values, "'");		

            if (strcmp("*AN", WorkListDB[i].HL7Tag)==0)		// generate new accession number
            { GenUID(uid);
              strcat(values, uid + strlen(uid)-16);
            }
            else if (strcmp("*UI", WorkListDB[i].HL7Tag)==0)	// generate new uid
            { GenUID(uid);
              strcat(values, uid);
            }

            strcat(values, "',");
          }
        }
        ++i;
      }

      fields[strlen(fields)-1]=0;	// remove trailing ,
      values[strlen(values)-1]=0;	// remove trailing ,

      DB.AddRecord("dicomworklist", fields, values);
      SystemDebug.printf("Entering modality worklist fields: %s\n", fields);  
      QueryFromGui++;

      fields[0]=0;
      values[0]=0;

      HL7FieldSep    = '|';
      HL7SubFieldSep = '^';
      HL7RepeatSep   = '~';
    }

    if (strcmp(type, "EOM")==0)		// end of message
      if (*p==0) break;			// end of file
  }

  DB.Close();
}

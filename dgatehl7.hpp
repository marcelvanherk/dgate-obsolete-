//
//  dgatehl7.hpp
//

/*
bcb 20210705 Created
 */

#ifndef dgatehl7_hpp
#define dgatehl7_hpp

void parseHL7(char **p, char *data, char *type, char *tmp, char *HL7FieldSep, char *HL7SubFieldSep, char *HL7RepeatSep);
void ProcessHL7Data(char *data);


#endif

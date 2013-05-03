/****************************************************************************
 
 THIS SOFTWARE IS MADE AVAILABLE, AS IS, AND IMAGE RETERIVAL SERVICES
 DOES NOT MAKE ANY WARRANTY ABOUT THE SOFTWARE, ITS
 PERFORMANCE, ITS MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR
 USE, FREEDOM FROM ANY COMPUTER DISEASES OR ITS CONFORMITY TO ANY
 SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND PERFORMANCE OF
 THE SOFTWARE IS WITH THE USER.
 
 Copyright of the software and supporting documentation is
 owned by the Image Reterival Services, and free access
 is hereby granted as a license to use this software, copy this
 software and prepare derivative works based upon this software.
 However, any distribution of this software source code or
 supporting documentation or derivative works (source code and
 supporting documentation) must include this copyright notice.
 ****************************************************************************/

/***************************************************************************
 *
 * Image Reterival Services
 * Version 0.1 Beta
 *
 ***************************************************************************/

/* 
bcb 20120820: Created this file.
bcb 20120931: Added prevent copy.
bcb 20130226: Made the ACRNemaMap class a singleton. Version to 1.4.18a.
bcb 20130311: Added GetACRNemaNoWild.

*/

#ifndef amap_hpp
#define amap_hpp

#define ACRNEMA_AE_LEN 20
#define ACRNEMA_IP_LEN 128
#define ACRNEMA_PORT_LEN 16
#define ACRNEMA_COMP_LEN 16
#define DOT_FOUR_LEN 16

typedef	struct	_ACRNemaAddress
{
        char	Name[ACRNEMA_AE_LEN];
        char	IP[ACRNEMA_IP_LEN];
        char	Port[ACRNEMA_PORT_LEN];
        char	Compress[ACRNEMA_COMP_LEN];
        char	LastGoodIP[DOT_FOUR_LEN];//For future, push after found?
}	ACRNemaAddress;

class   ACRNemaMap
        {
        private:
                char                            *ACRNEMAMAP;
                char                            *defaultFormat;
                Array<ACRNemaAddress *>	*ACRNemaAddressArrayPtr;
                void                            wildIPCheck(char *remoteIP, const char *mappedIP, char *lastGoodIP);
                BOOL                            iswhitespacea(char ch);
                BOOL                            getItemNotEnd(char *item, char **from, size_t limit);
                BOOL                            AppendMapToOpenFile(FILE *f, char *format);
        protected:
// Create and destroy the class protected to make it a singleton.
                ACRNemaMap();
                ~ACRNemaMap();
        public:
                void                            ClearMap();
                BOOL                            DeleteEntry(unsigned int theEntry);
                BOOL                            GetACRNemaNoWild(char *, char *, char *, char*);
                BOOL                            GetACRNema(char *, char *, char *, char*);
                const char*                     GetFileName();
                void                            GetOneEntry(unsigned int theEntry, char *output, unsigned int outLen,
                                                            char *format = NULL);
                static ACRNemaMap*              GetInstance()
                {
                        //"Lazy" initialization. Singleton not created until it's needed
                        static ACRNemaMap acrInstance; // One and only one instance.
                        return &acrInstance;
                }
                ACRNemaAddress*                 GetOneACRNemaAddress(unsigned int theEntry);
                unsigned int                    GetSize();
                //Do not forget to free the return strings!
                char*                           GetWholeAMapToString(char *format);
                BOOL                            InitACRNemaAddressArray(char *amapFile);
                char*                           PrintAMapToString(char *format = NULL);
                BOOL                            ReplaceEntry(unsigned int position, const char *name, const char *ip,
                                                             const char *port, const char *compress = "un");
                BOOL                            WriteAMapToAFile(const char *outFile, char *format);
                BOOL                            WriteAMapToTheFile(char *format);
        private:// This will prevent it from being copied (it has a pointer)
                ACRNemaMap(const ACRNemaMap&);
                const	ACRNemaMap & operator = (const ACRNemaMap&);
        };

#endif

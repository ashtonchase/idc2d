#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <FL/fl_ask.H>
#include "browser_log.h"
#include "i_current-2.h"

browser_log::browser_log(Fl_Browser* pFlBrowser) 
    : p_browser(pFlBrowser)
    , n_lines(0)
    , max_lines(10000)
    , paste_buffer(NULL)
    , paste_count(0)
    , paste_limit(999)
{
    paste_buffer = new char [max_lines * 80];  // up to 1000 lines of 80 char each
    p_browser->clear();
}

browser_log::~browser_log(void) {
    if (paste_buffer) {
        delete [] paste_buffer;
        paste_buffer = NULL;
    }
    p_browser = NULL;
}

int browser_log::clear_log(void) { 
    p_browser->clear(); 
    return 0; 
}
int browser_log::clear_log_t(void) { 
    Fl::lock();
    p_browser->clear(); 
    Fl::awake((void*) p_browser);
    Fl::unlock();

    return 0; 
}
/**
* threaded version to be called from child threads
* (locks gui for to itself for a bit)
* do not append LF or CR
* TODO should really chomp off LF or CR to ensure no CR or LF because
*      it messes up the log window
*/

int browser_log::add_line_t(const char* pString) {
    void* pVoid = NULL;
    Fl::lock();
    // add chomp function later
    if (n_lines < max_lines) {
        p_browser->add(pString, pVoid);
        n_lines++;
    }
    else {
        p_browser->remove(1); // remove the top line
        p_browser->add(pString, pVoid);
    }
    // make last line visible
    p_browser->bottomline(max_lines);

    Fl::awake((void*) p_browser);
    Fl::unlock();

    return 0;
}
/**
* primary thread or non-thread version */
int browser_log::add_line(const char* pString) {
    void* pVoid = NULL;

    // add chomp function later
    if (n_lines < max_lines) {
        p_browser->add(pString, pVoid);
        n_lines++;
    }
    else {
        p_browser->remove(1); // remove the top line
        p_browser->add(pString, pVoid);
    }
    // make last line visible
    p_browser->bottomline(max_lines);
    return 0;
}

void browser_log::error_line(const char *err)
{
    if (err)
    {
        add_line(err);
        fl_alert(err);
    }
}

/**
* routine for dumping a block of bytes in hex
*
*/

int browser_log::dump_blk(char* blk_start, int blk_size) {
    int i = 0;
    char* pc = blk_start;
    char tString[80];

    while (i < blk_size) {
        sprintf(tString, "%03x : %02x %02x %02x %02x %02x %02x %02x %02x",
            i, *pc++, *pc++, *pc++, *pc++, *pc++, *pc++, *pc++, *pc++);
        add_line(tString);
        i =+ 8;
    }
    return(0);
}

/**
* routine for dumping a block of bytes in hex for secondary threads
*
*/

int browser_log::dump_blk_t(char* blk_start, int blk_size) {
    int i = 0;
    unsigned char* pc;
    char tString[80];
    Fl::lock();
    pc = (unsigned char*)blk_start;    
    while (i < blk_size) {
        sprintf(tString, "%03x : %02x %02x %02x %02x %02x %02x %02x %02x",
            i, *(pc), *(pc+1), *(pc+2), *(pc+3), *(pc+4), 
            *(pc+5), *(pc+6), *(pc+7));
        pc+=8;  
        add_line(tString);
        i += 8;
    }

    Fl::awake((void*) p_browser);
    Fl::unlock();
    return(0);
}


/**
* writes the log buffer to a file
*
* the filename is log_yymmddssss (year,month,day,seconds since midnite)
* \warning year 2100 warning
*
*/

char* browser_log::write_log(void) {
    int j;
    char* ret_val;
    FILE*    log_file;

    time_t rawtime;
    struct tm * timeinfo;

    ret_val = 0;  ///< return value 0=success -1=fail

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    /// 
    sprintf(log_name, "log_%02i%02i%02i%05i.txt",
        timeinfo->tm_year - 100,
        timeinfo->tm_mon,
        timeinfo->tm_mday,
        timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec);
    log_file = fopen(log_name, "w+");
    if (log_file)  {
        j = 1;
        while (j <= n_lines) {
            fprintf(log_file, "%s\r\n", p_browser->text(j));
            j++ ;
        }
        fclose(log_file);
        ret_val = log_name;
    }
    else {
        ret_val = (char)0;
    }

    return(ret_val);
}

int
browser_log::save_log(const char *log_name) {
    FILE* pf;

    if (NULL == log_name)
        return -1;

    if (NULL != (pf = fopen(log_name, "w+")))  {
        for (int i = 1; i <= n_lines; i++) {
            fprintf(pf, "%s\n", p_browser->text(i));
        }
        fclose(pf);
        return 0;
    }

    return errno;
}

int browser_log::paste_selected(void) {
    int i,n,len;
    const char *pc;
    char *pb;
    n = n_lines;
    i = 0;
    // copy selected lines to paste buffer
    paste_count = 0;
    pb = paste_buffer;
    len = 0;
    while (i <= n) {
        if (p_browser->selected(i)) {
            pc = p_browser->text(i);
            while (*pc) {
                *pb++ = *pc++;
                len++;
            }
            *pb++ = 0x0d;
            *pb++ = 0x0a;
            len+=2;
            paste_count++;
        }
        if (paste_count > paste_limit) i = n+1;
        i++;
    }
    *pb = 0;
    Fl::copy(paste_buffer, len, 1);
    return(0);
}


#ifndef BROWSER_LOG
#define BROWSER_LOG
#define TERM_WIDTH 82

#include <FL/Fl_Browser.H>
#include <FL/filename.H>

class browser_log {
public:
    int initialize();
    //int add_line(const char* pString);
    //int add_line_t(const char* pString);
	int add_line(const char *format, ...);
	int add_line_t(const char *format, ...);
    void error_line(const char *format, ...);
    int dump_blk(char* blk_start, int blk_size);
    int dump_blk_t(char* blk_start, int blk_size);
    int clear_log(void);
    int clear_log_t(void);
    int paste_selected(void);
    char* write_log(void);
    int save_log(const char *log_name);
    browser_log(Fl_Browser* pFlBrowser);
    ~browser_log();
     
private:
    Fl_Browser* p_browser;
    int         n_lines;
    int         max_lines;
    char        log_name[FL_PATH_MAX];
    char*       paste_buffer;
    int         paste_count;
    int         paste_limit;
};

#endif

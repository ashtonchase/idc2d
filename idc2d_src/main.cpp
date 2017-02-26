#include <pthread.h>
#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Shared_Image.H>
#include <time.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
/*****************************************************************************/
#ifdef WIN32
	#include <windows.h>
	#include <FL/x.H>
	#include "resource.h"
#endif

/*****************************************************************************/
#ifdef _OPENMP
#include "omp.h"
#endif

/*****************************************************************************/
#include "i_current_private.h"
#include "i_current-2.h"
#include "browser_log.h"
/*****************************************************************************/
#include "main.h"
#include "button_callbacks.h"

/*****************************************************************************/
int         stop_tests;
int         msg_lvl;
browser_log *central_log;
/*****************************************************************************/
typedef struct 
{
	int argc;
	char **argv;
} params_t;
/*****************************************************************************/
pthread_mutex_t guard		= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition	= PTHREAD_COND_INITIALIZER;
volatile int v_app;
volatile int v_running;
/*****************************************************************************/
void log_header(browser_log* pl);
void log_date(browser_log* pl);
/*****************************************************************************/
void 
main_window_close(Fl_Widget* widget, void* nothing)
{
	//exit(0);// hiding the main window should cause normal exit by ending Fl::run()
	pthread_mutex_lock(&guard);
	v_app = FALSE;
	v_running = FALSE;
	pthread_cond_signal(&condition);
	pthread_mutex_unlock(&guard);
	pthread_exit(NULL);

}
/*****************************************************************************/
void 
calc_thread_start(void)
{
	pthread_mutex_lock(&guard);
	v_running = TRUE;
	pthread_cond_signal(&condition);
	pthread_mutex_unlock(&guard);
}
/*****************************************************************************/
void
*gui_thread_run(void *params)
{
	params_t *args = (params_t *)params;

    Fl::scheme("plastic");
    Fl_File_Icon::load_system_icons();
    //fl_register_images();
    Fl::get_system_colors();
    Fl::visual(FL_RGB);
    Fl_Image_Display::set_gamma();

    main_window     = make_window();        ///< construct main panel
    about_window    = make_about_window();  ///< construct about window
	source_edit_window = make_edit_source_window();
	sink_edit_window   = make_edit_sink_window();
	substrate_edit_window  = make_edit_substr_window();
	material1_edit_window  = make_edit_m1_window();
	additional_edit_window = make_edit_additional_window();
    about_window->set_modal();

    char ver_buf[265];
    sprintf(ver_buf, "fltk %d.%d.%d - fltk.org", FL_MAJOR_VERSION, FL_MINOR_VERSION, FL_PATCH_VERSION);
    fltk_version_box->label(ver_buf);
    config_window   = make_config_window(); ///< construct configuration window
    config_window->set_modal();
    
    main_window->callback(main_window_close);
    main_window->position(0, 0);

    /** initiatize the main buffer for the text log window */    
    main_log->has_scrollbar(Fl_Browser::VERTICAL_ALWAYS);
    central_log = new browser_log(main_log);
    log_header(central_log);   

    box_source->color(fl_rgb_color(0, 0xFF, 0));
    box_sink->color(fl_rgb_color(0, 0, 0xFF));
    box_substrate->color(fl_rgb_color(0, 0, 0));
    box_material1->color(fl_rgb_color(0xFF, 0, 0));
    box_material2->color(fl_rgb_color(0xC0, 0xC0,0xC0));
    box_material3->color(fl_rgb_color(0xFF, 0xD7, 0));
	box_material4->color(fl_rgb_color(0x80, 0x80, 0x80));
    // force the decimal entry points to be updated to color box settings
	r_source_d->value(0);  // Note that this should align with R value in box_source->color() above
	r_source_d->do_callback();

    choice_cu_thickness->value(6); // set 1oz copper thickness as the inital selection
	choice_cu_thickness->do_callback();
	choice_material_1->value(5);  // copper
	choice_material_1->do_callback();
	choice_material_2->value(14);  // silver
	choice_material_2->do_callback();
	choice_material_3->value(6);  // gold
	choice_material_3->do_callback();
	choice_material_4->value(15);  //tin
	choice_material_4->do_callback();

    spinner_threads->minimum(1);
    spinner_threads->maximum(16);
#ifndef _OPENMP
    spinner_threads->deactivate(); 
#endif

#ifdef WIN32
	main_window->icon((char *)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON1)));
#endif

    main_window->show(args->argc, args->argv);  ///< make the main panel visible

    Fl::lock();           ///< to initialize threading
    Fl::run();

	return NULL;
}
/*****************************************************************************/
int 
gui_thread_start(void *args)
{
    int             retval = NO_ERROR;
    pthread_t       thread;
    pthread_attr_t  attr;

    if ((retval = pthread_attr_init(&attr)))
    {
        return ERROR_ABORT;
    }

    if ((retval = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)))
    {
        return ERROR_ABORT;
    }

    if ((retval = pthread_create(&thread, &attr, gui_thread_run, args)))
    {
        return ERROR_ABORT;
    }
    
    if ((retval = pthread_attr_destroy(&attr)))
    {
        return ERROR_ABORT;
    }

    return NO_ERROR;
}
/*****************************************************************************/
//extern int debug = 1;  // 1 = debug on
//int 
//WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCMdShow)
int 
main(int argc, char **argv)
{
	params_t args;
    msg_lvl = 1;
	
	v_running = FALSE;
	v_app = TRUE;
    
	setlocale(LC_ALL, ".ACP");

	args.argc = argc;
	args.argv = argv;
	gui_thread_start((void *)&args);

	for (;;)
	{
		pthread_mutex_lock(&guard);
		while (v_app && !v_running)
		{
			pthread_cond_wait(&condition, &guard);
		}
		pthread_mutex_unlock(&guard);
		if (!v_app)
			break;
		if (v_running)
			calc_thread_run();
	}
	pthread_mutex_destroy(&guard);
	pthread_cond_destroy(&condition);


    return EXIT_SUCCESS;
}
/*****************************************************************************/
void 
log_header(browser_log* pl)
{
    pl->add_line("-----------------------------------");
    pl->add_line("Copper Plane Current Analysis Program");
    pl->add_line("Version %s", VER_STRING);
    log_date(pl);
    pl->add_line("GPL Version 2.0");
    pl->add_line("-----------------------------------");

#ifdef _OPENMP
    int i = omp_get_max_threads();
    pl->add_line("Number of processors: %d", omp_get_num_procs()); 
    pl->add_line("Max number of OMP threads: %d", i);
    pl->add_line("-----------------------------------");
    spinner_threads->value(i);
#endif
}
/*****************************************************************************/
void
log_date(browser_log* pl)
{
    char tString[80];
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime); 
    // using precision to truncate \n off asctime - watch implementations of asctime!!!
    pl->add_line("Time: %.24s", asctime(timeinfo));

}
/*****************************************************************************/

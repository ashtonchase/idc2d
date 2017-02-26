/*****************************************************************************/
#ifdef _OPENMP
#include "omp.h"
#endif
/*****************************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/filename.H>
#include <FL/fl_draw.H>
/*****************************************************************************/
#ifndef WIN32
#include <sys/time.h>
#include <errno.h>
#endif
/*****************************************************************************/
#include "button_callbacks.h"
#include "material_database.h"
#include "i_current-2.h"
#include "browser_log.h"
#include "i_dc.h"
#include "o_image.h"
#include "timeval.h"
#include "main.h"
/*****************************************************************************/
#define BUFFER_SIZE	1024
#define ERROR_ABORT -1
/*****************************************************************************/
#ifndef FALSE
#define FALSE       0
#endif
/*****************************************************************************/
#ifndef TRUE
#define TRUE        1
#endif
/*****************************************************************************/
#ifndef NO_ERROR
#define NO_ERROR    0
#endif
/*****************************************************************************/
// Most of the button work is done here.  The actual button callbacks 
// call these functions.  Because of the complexity and threading,
// doing the code in this module is better than fluid generated code.
/*****************************************************************************/
//volatile int v_running = FALSE;
/*****************************************************************************/
extern browser_log *central_log;
//TooltipBox *copper_plane            = NULL;
//TooltipBox *current_density_plot    = NULL;
/*****************************************************************************/
rgb_materials_t plane[N_MATERIALS + 1];
Fl_Shared_Image    *rgb_image = NULL;
/*****************************************************************************/
// try to keep internals with SI
const IDC_FP mil2m      = 2.54e-5;
const IDC_FP m2mil      = 1 / mil2m;
const IDC_FP in2m       = 0.0254;
const IDC_FP m2in       = 1 / in2m;
const IDC_FP milPerOz	= 1.340830823;

IDC_FP thickness_global;    // (in meters) shared value to allow reporting without recalculating

IDC_FP g_pixel_x[N_MATERIALS];
IDC_FP g_pixel_y[N_MATERIALS];
IDC_FP vcc;
unsigned int n_iterations;
IDC_FP cutoff;

extern volatile int v_running;
extern volatile int v_app;

/*****************************************************************************/
char str_buffer[256];
/*****************************************************************************/
int rgb_height;
int rgb_width;
int rgb_depth;
unsigned int last_r;
unsigned int last_g;
unsigned int last_b;
/*****************************************************************************/
IDC_FP dtimeval(const struct timeval *stop, const struct timeval *start, struct timeval *diff);
int check_colors(void);
int set_pix(unsigned char *pix);
void get_out_pix_coords(const int x, const int y);
/*****************************************************************************/
void 
calc_thread_run(void)
//calc_thread_run(void)
{
    IDC_FP  i_value, r_eff;

    IDC_FP  i_min, i_max;

    struct timeval start;
    struct timeval stop;

    v_running = TRUE;

    Fl::lock();
    btn_analyze->label("Abort");
    btn_analyze->activate();
//	 btn_show->hide();
//	 progress_bar->show();
    Fl::awake((void*) btn_analyze);
	 Fl::awake((void*) btn_show);
//	 Fl::awake((void*) progress_bar);
    Fl::unlock();


    gettimeofday(&start, NULL);
    
    i_dc_setconductance( g_pixel_x , g_pixel_y );
    i_dc_setvcc(vcc);               // will change to constant current source some time in future
    i_dc_set_limits(n_iterations, cutoff);

    int i = i_dc_dbmx();
    if (!v_app)
		return ;  

    central_log->add_line_t("  Total iteration count     = %i\n", i);


    if (0 < i)
    {
        i_value = i_dc_get_vcc_current();
        if (i_value != 0)
        {
            r_eff = vcc / -i_value;
        }
        else 
        {
            r_eff = -1;
        }
		i_dc_get_minmax(IDC_A, i_min, i_max);

		central_log->add_line_t("  Total supply current      = %14.7g  A", -i_value);
		central_log->add_line_t("  Effective DC resistance   = %14.7g  Ohms", r_eff);
        central_log->add_line_t("  Maximal pixel current     = %14.7g  A", i_max);    
        central_log->add_line_t("  Minimum pixel current     = %14.7g  A", i_min); 

//        i_dc_get_minmax(IDC_X, i_min, i_max_x);
//        central_log->add_line_t("  Max pixel current (X)     = %14.7g  A", i_max_x);    
//        central_log->add_line_t("  Min pixel current (X)     = %14.7g  A", i_min);         

//        i_dc_get_minmax(IDC_Y, i_min, i_max_y);
//        central_log->add_line_t("  Max pixel current (Y)     = %14.7g  A", i_max_y);    
//        central_log->add_line_t("  Min pixel current (Y)     = %14.7g  A", i_min);         

		central_log->add_line_t("  Peak current density      = %14.7g  MA/m^2",i_max/((pixel_x_umeters->value() * 1e-6)*thickness_global)* 1e-6 );


        Fl::lock();
        group_output->activate();
        btn_show->activate();
        Fl::awake((void*) group_output);
        Fl::awake((void*) btn_show);
        Fl::unlock();
    }
    else
    {
        central_log->add_line_t("Simulation failed/aborted");
    }
    gettimeofday(&stop, NULL);
    central_log->add_line_t("Finished in %f s", dtimeval(&stop, &start, NULL)); 
    central_log->add_line_t("-----------------------------------"); 


    Fl::lock();
    btn_check->activate();
    btn_analyze->label("Analyze");
    btn_analyze->activate();
    btn_save_log->activate();
    group_input->activate();
    //fl_cursor(FL_CURSOR_DEFAULT);

    Fl::awake((void*) btn_check);
    Fl::awake((void*) btn_analyze);
    Fl::awake((void*) btn_save_log);
    Fl::awake((void*) group_input);
    Fl::unlock();
    v_running = FALSE;

//    pthread_exit(NULL);
    return ;           // windows
}
/*****************************************************************************/
//int 
//calc_thread_start(void)
//{
//    int             retval = NO_ERROR;
//    pthread_t       thread;
//    pthread_attr_t  attr;
//
//    if ((retval = pthread_attr_init(&attr)))
//    {
//        return ERROR_ABORT;
//    }
//
//    if ((retval = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)))
//    {
//        return ERROR_ABORT;
//    }
//
//    if ((retval = pthread_create(&thread, &attr, calc_thread_run, NULL)))
//    {
//        return ERROR_ABORT;
//    }
//    
//    if ((retval = pthread_attr_destroy(&attr)))
//    {
//        return ERROR_ABORT;
//    }
//
//    return NO_ERROR;
//}
/*****************************************************************************/
int 
copy2paste(void)
{
    central_log->paste_selected();
    return NO_ERROR;
}



//
// NOTE: KEEP THIS ALIGNED WITH MATERIAL_DATEBASE.C AND MATERIAL_DATABASE.H !!!!!
//
Fl_Menu_Item menu_choice_material[] = {
 {Materials[0 ].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 0 
 {Materials[1 ].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 1 
 {Materials[2 ].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 2 
 {Materials[3 ].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 3 
 {Materials[4 ].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 4 
 {Materials[5 ].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 5 
 {Materials[6 ].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 6 
 {Materials[7 ].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 7 
 {Materials[8 ].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 8 
 {Materials[9 ].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 9 
 {Materials[10].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 10
 {Materials[11].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 11
 {Materials[12].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 12
 {Materials[13].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 13
 {Materials[14].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 14
 {Materials[15].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 15
 {Materials[16].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 16
 {Materials[17].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 17
 {Materials[18].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 18
 {Materials[19].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 19
 {Materials[20].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 20
 {Materials[21].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 21
 {Materials[22].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 22
 {Materials[23].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 23
 {Materials[24].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 24
 {Materials[25].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 25
 {Materials[26].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 26
 {Materials[27].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 27
 {Materials[28].pName, 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 28
// entry template
 {"",   0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 0
 {0,                      0,  0, 0, 0, 0,               0, 0,  0}
};

/*****************************************************************************/
void cb_metal_chosen(Fl_Choice* pChoice)
{// Get the parameters for the selected metal from the datbase and put them in the correct layer's settings
	fc_metal_chosen(pChoice);
}
/*****************************************************************************/

/*****************************************************************************/
void cb_conductance_m1_changed(Fl_Value_Input *pEdit)
{// change the selector to the relevant entry if there is one, else blank it
	fc_conductance_edit_changed(pEdit, choice_material_1);
}

void cb_conductance_m2_changed(Fl_Value_Input *pEdit)
{// change the selector to the relevant entry if there is one, else blank it
	fc_conductance_edit_changed(pEdit, choice_material_2);
}

void cb_conductance_m3_changed(Fl_Value_Input *pEdit)
{// change the selector to the relevant entry if there is one, else blank it
	fc_conductance_edit_changed(pEdit, choice_material_3);
}

void cb_conductance_m4_changed(Fl_Value_Input *pEdit)
{// change the selector to the relevant entry if there is one, else blank it
	fc_conductance_edit_changed(pEdit, choice_material_4);
}

// KEEP MENU ARRAY SYNCHRONIZED TO DEFAULT VALUE IN MAIN.CPP

Fl_Menu_Item menu_choice_cu_thickness[] = {
 {"4.257\xB5m (1/8oz)",   0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 0
 {"5.676\xB5m (1/6oz)",   0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 1
 {"8.514\xB5m (1/4oz)",   0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 2
 {"11.352\xB5m (1/3oz)",  0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 3
 {"17.029\xB5m (1/2oz)",  0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 4
 {"25.543\xB5m (3/4oz)",  0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 5
 {"34.057\xB5m (1oz)",    0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 6
 {"51.086\xB5m (1.5oz)",  0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 7
 {"68.114\xB5m (2oz)",    0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 8
 {"85.143\xB5m (2.5oz)",  0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 9
 {"102.171\xB5m (3oz)",   0,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0}, // index 10
 {0,                      0,  0, 0, 0, 0,               0, 0,  0}
};



/*****************************************************************************/
// scans the bmp file to produce information necessary to read into analyzer
void
fc_btn_check(void)
{
    btn_analyze->deactivate();
    fl_cursor(FL_CURSOR_WAIT);
    if (NO_ERROR == check_colors())
    {
        if (rgb_image)
        {
            central_log->add_line("Allocating memory...");
            i_dc_allocate(rgb_width, rgb_height);        
            if (NO_ERROR == set_pix(i_dc_get_pixmap()))
            {
                central_log->add_line("Image check:");
                central_log->add_line("  Source pixel count  = %i", plane[MATERIAL_VCC].pix_count);
                central_log->add_line("  Sink pixel count    = %i", plane[MATERIAL_GND].pix_count);
                central_log->add_line("  FR-4 pixel count    = %i", plane[MATERIAL_FR4].pix_count);
                central_log->add_line("  Material 1 pixel count = %i", plane[MATERIAL_M1].pix_count);
                central_log->add_line("  Material 2 pixel count = %i", plane[MATERIAL_M2].pix_count);
                central_log->add_line("  Material 3 pixel count = %i", plane[MATERIAL_M3].pix_count);
                central_log->add_line("  Material 4 pixel count = %i", plane[MATERIAL_M4].pix_count);
                central_log->add_line("  Unknown pixel count = %i", plane[MATERIAL_XXX].pix_count);
                btn_analyze->activate();
            }            
        }
    }
    group_input->hide();
    group_analyze->show();
    fl_cursor(FL_CURSOR_DEFAULT);
}
/*****************************************************************************/
// start the calculations
void
fc_btn_analyze(void)
{
    IDC_FP cu_oz;
    IDC_FP thickness;
	IDC_FP pix_dim_x;
	IDC_FP pix_dim_y;
	IDC_FP metal_1_sigma;
	IDC_FP metal_2_sigma;
	IDC_FP metal_3_sigma;
	IDC_FP metal_4_sigma;
	IDC_FP area_x;
	IDC_FP area_y;
	IDC_FP delta_20_temp;


    if (FALSE == v_running)
    {        
        choice_cu_thickness->deactivate();
        input_max_iter->deactivate();
        input_cutoff->deactivate();
        btn_show->deactivate();
        btn_analyze->deactivate();
        btn_save_log->deactivate();

        group_input->deactivate();
        group_output->deactivate();
        //fl_cursor(FL_CURSOR_WAIT);

#if defined (_OPENMP)
       spinner_threads->deactivate();
       int i = (int)spinner_threads->value();
       omp_set_num_threads(i);
       central_log->add_line("Number of threads: %d", i);

#endif
//        cu_oz = choice_cu_thickness->value() ?  choice_cu_thickness->value() * 0.5 : 0.25; // This returns bad values. Need lookup table 
//        thickness = 1.35 * cu_oz * mil2m; // The constant here could stand improved precision so thickness constants more closely resemble copper weights
		switch (choice_cu_thickness->value())
		{
			case 0  : cu_oz = 0.12500000000000000; break; // 1/8     oz copper
			case 1  : cu_oz = 0.16666666666666667; break; // 1/6     oz copper
			case 2  : cu_oz = 0.25000000000000000; break; // 1/4     oz copper
			case 3  : cu_oz = 0.33333333333333333; break; // 1/3     oz copper
			case 4  : cu_oz = 0.50000000000000000; break; // 1/2     oz copper
			case 5  : cu_oz = 0.75000000000000000; break; // 3/4     oz copper
			case 6  : cu_oz = 1.00000000000000000; break; // 1       oz copper
			case 7  : cu_oz = 1.50000000000000000; break; // 1 & 1/2 oz copper
			case 8  : cu_oz = 2.00000000000000000; break; // 2       oz copper
			case 9  : cu_oz = 2.50000000000000000; break; // 2 & 1/2 oz copper
			case 10 : cu_oz = 3.00000000000000000; break; // 3       oz copper
			default : cu_oz = 0.25000000000000000; break; // 1/8     oz copper
		}
        thickness = milPerOz * cu_oz * mil2m;
		thickness_global = thickness;
//        thickness = 1.340830823 * cu_oz * mil2m; // thickness in meters = mils_per_oz * oz * convert_mil_to_meter
        n_iterations = (unsigned int)input_max_iter->value();
        cutoff = input_cutoff->value();
        vcc = source_voltage->value();

        // volume conductance of each pixel = G * L / (W * T)
        //     where G is conductance in S/m                   /\ 
        //           L is length in meters               L (X)/  \W (Y)
        //           W is width in meters                    /    \
        //           T is thickness in meters          T (Z)|\    /|
		//                                                   \\  //
		//                                                    \\//
		//                                                     |/ 
		// NOTE: The original program swapped T for W, meaning thickness inversely impacted conductance
		//       The original program also presumed square pixels 1.0mm x 1.0mm
		// Now that the user interface permits arbitrary dimensions, X and Y need to be treated independently
//        g_pixel[MATERIAL_GND] = conductance_m1->value() * 1e6 * thickness; // directly from the edit control <- since gnd may not be copper, is this correct?
//        g_pixel[MATERIAL_VCC] = conductance_m1->value() * 1e6 * thickness; // directly from the edit control <- since vcc may not be copper, is this correct?
//        g_pixel[MATERIAL_FR4] = 0;
//        g_pixel[MATERIAL_M1]  = conductance_m1->value() * 1e6 * thickness; // directly from the edit control
//        g_pixel[MATERIAL_M2]  = conductance_m2->value() * 1e6 * thickness; // directly from the edit control
//        g_pixel[MATERIAL_M3]  = conductance_m3->value() * 1e6 * thickness; // directly from the edit control
//        g_pixel[MATERIAL_M4]  = conductance_m4->value() * 1e6 * thickness; // directly from the edit control
		delta_20_temp = ( initial_temp_out->value() - 20 );
		pix_dim_x = pixel_x_umeters->value() * 1e-6; // convert from micrometers to meters
		pix_dim_y = pixel_y_umeters->value() * 1e-6; // convert from micrometers to meters
		metal_1_sigma = ( 1 - ( 0.001 * delta_20_temp * (Materials[choice_material_1->value()].Resistance_TempCo) ) ) * conductance_m1->value() * 1e6; // convert from MS/m to S/m
		metal_2_sigma = ( 1 - ( 0.001 * delta_20_temp * (Materials[choice_material_2->value()].Resistance_TempCo) ) ) * conductance_m2->value() * 1e6; // convert from MS/m to S/m
		metal_3_sigma = ( 1 - ( 0.001 * delta_20_temp * (Materials[choice_material_3->value()].Resistance_TempCo) ) ) * conductance_m3->value() * 1e6; // convert from MS/m to S/m
		metal_4_sigma = ( 1 - ( 0.001 * delta_20_temp * (Materials[choice_material_4->value()].Resistance_TempCo) ) ) * conductance_m4->value() * 1e6; // convert from MS/m to S/m
		area_x = pix_dim_y * thickness;
		area_y = pix_dim_x * thickness;
		// material in current source and sink not issue right now as pixel voltages are set
		// and only bond conductance slightly affected.  However, future surface contact
		// modelling will be affected.

        g_pixel_x[MATERIAL_GND] = metal_1_sigma / pix_dim_x * area_x; // <- since gnd may not be copper, is this correct?
        g_pixel_x[MATERIAL_VCC] = metal_1_sigma / pix_dim_x * area_x; // <- since vcc may not be copper, is this correct?
        g_pixel_x[MATERIAL_FR4] = 0;
        g_pixel_x[MATERIAL_M1]  = metal_1_sigma / pix_dim_x * area_x;
        g_pixel_x[MATERIAL_M2]  = metal_2_sigma / pix_dim_x * area_x;
        g_pixel_x[MATERIAL_M3]  = metal_3_sigma / pix_dim_x * area_x;
        g_pixel_x[MATERIAL_M4]  = metal_4_sigma / pix_dim_x * area_x;

        g_pixel_y[MATERIAL_GND] = metal_1_sigma / pix_dim_y * area_y; // <- since gnd may not be copper, is this correct?
        g_pixel_y[MATERIAL_VCC] = metal_1_sigma / pix_dim_y * area_y; // <- since vcc may not be copper, is this correct?
        g_pixel_y[MATERIAL_FR4] = 0;
        g_pixel_y[MATERIAL_M1]  = metal_1_sigma / pix_dim_y * area_y;
        g_pixel_y[MATERIAL_M2]  = metal_2_sigma / pix_dim_y * area_y;
        g_pixel_y[MATERIAL_M3]  = metal_3_sigma / pix_dim_y * area_y;
        g_pixel_y[MATERIAL_M4]  = metal_4_sigma / pix_dim_y * area_y;

        central_log->add_line("Simulation parameters:");
        sprintf(str_buffer, "  Max. iterations           = %d", n_iterations);
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Cutoff                    = %g", cutoff);
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Pixel dimensions          = %g x %g \xB5m (%g x %g mil)", pix_dim_x * 1e6, pix_dim_y * 1e6, pix_dim_x / 25.4e-6, pix_dim_y / 25.4e-6);
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Image dimensions          = %g x %g mm (%g x %g in)", pix_dim_x * 1e3 * rgb_width, pix_dim_y * 1e3 * rgb_height, pix_dim_x / 25.4e-3 * rgb_width, pix_dim_y / 25.4e-3 * rgb_height); 
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Layer thickness           = %g \xB5m (%g mil)", thickness * 1.0e+6, thickness * m2mil);
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Source voltage            = %g V", vcc);
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Initial temperature       = %g deg C", initial_temp_out->value() );
        central_log->add_line(str_buffer);

        sprintf(str_buffer, "  Material 1 conductivity   = %10.6g MS/m   %s", metal_1_sigma/1e6, Materials[choice_material_1->value()].pName );
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Material 2 conductivity   = %10.6g MS/m   %s", metal_2_sigma/1e6, Materials[choice_material_2->value()].pName );
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Material 3 conductivity   = %10.6g MS/m   %s", metal_3_sigma/1e6, Materials[choice_material_3->value()].pName );
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Material 4 conductivity   = %10.6g MS/m   %s", metal_4_sigma/1e6, Materials[choice_material_4->value()].pName );
        central_log->add_line(str_buffer);

		// NOTE: NOT supporting non-square pixel at the moment - just report x dimension.
        sprintf(str_buffer, "  Material 1 conductance    = %g S/pixel  ", g_pixel_x[MATERIAL_M1]);
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Material 2 conductance    = %g S/pixel  ", g_pixel_x[MATERIAL_M2]);
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Material 3 conductance    = %g S/pixel  ", g_pixel_x[MATERIAL_M3]);
        central_log->add_line(str_buffer);
        sprintf(str_buffer, "  Material 4 conductance    = %g S/pixel  ", g_pixel_x[MATERIAL_M4] );
        central_log->add_line(str_buffer);


        calc_thread_start();
        //if (NO_ERROR == calc_thread_start())        
        //{
        //    central_log->add_line_t("Simulation started...");
        //}
        //else
        //{
        //    central_log->error_line("Error! Simulation could not be started");
        //    btn_check->activate();
        //    btn_analyze->activate();
        //    btn_save_log->activate();

        //    group_input->activate();
        //    group_output->activate();
        //    //fl_cursor(FL_CURSOR_DEFAULT);
        //}

        choice_cu_thickness->activate();
        input_max_iter->activate();
        input_cutoff->activate(); 
#ifdef _OPENMP
        spinner_threads->activate();
#endif

    }
    else
    {
        btn_analyze->deactivate();
        central_log->add_line_t("Aborting...");
        v_running = FALSE;        
    }
}
/*****************************************************************************/
void
fc_btn_save_image(void)
{
    int ret = NO_ERROR;
    const char *ext;
    char *fn = fl_file_chooser("Save Image As", "png (*.png)", NULL, 0);
	
    if (fn)
    {
        fl_cursor(FL_CURSOR_WAIT);
        ext = fl_filename_ext(fn);
        if (ext && (0 == strcmp(ext, ".png")))
        {
            ret = oi_save_image(fn);
        }
        else
        {
            char *nn = new char[strlen(fn) + 5];
            strcpy(nn, fn);
            strcat(nn, ".png");
            ret = oi_save_image(nn);
            delete [] nn;
        }
        fl_cursor(FL_CURSOR_DEFAULT);
	}
    if (ret)
    {
            central_log->error_line("Error! Save %s failed: %s", fn, strerror(ret));
    }
}
/*****************************************************************************/
void
fc_btn_save_data(void)
{
    int ret;
    char *fn = fl_file_chooser("Save current density as", "dat (*.dat)", NULL, 0);
	if (fn)
    {    
        fl_cursor(FL_CURSOR_WAIT);
        ret = oi_save_data(fn);
	    if (ret)
        {
            central_log->error_line("Error! Save %s failed: %s", fn, strerror(ret));
        }
        fl_cursor(FL_CURSOR_DEFAULT);
    }
}
/*****************************************************************************/
void
fc_btn_open_cfg(void)
{
    FILE *pf = NULL;
    char buffer[BUFFER_SIZE+1];
    char *fn = fl_file_chooser("Open configuration file", "cfg (*.cfg)", NULL, 0);
	
    if (fn)
    {        
        fl_cursor(FL_CURSOR_WAIT);
        if ((pf = fopen(fn, "r")))
        {
            if (fgets(buffer, BUFFER_SIZE, pf))
                source_voltage->value(atof(buffer));
            if (fgets(buffer, BUFFER_SIZE, pf))
			{
                conductance_m1->value(atof(buffer));
				fc_conductance_edit_changed(conductance_m1, choice_material_1);
			}
            if (fgets(buffer, BUFFER_SIZE, pf))
			{
                conductance_m2->value(atof(buffer));
 				fc_conductance_edit_changed(conductance_m2, choice_material_2);
			}
            if (fgets(buffer, BUFFER_SIZE, pf))
			{
                conductance_m3->value(atof(buffer));
 				fc_conductance_edit_changed(conductance_m3, choice_material_3);
			}
            if (fgets(buffer, BUFFER_SIZE, pf))
			{
                conductance_m4->value(atof(buffer));
 				fc_conductance_edit_changed(conductance_m4, choice_material_4);
			}
            if (fgets(buffer, BUFFER_SIZE, pf))
                choice_cu_thickness->value(atoi(buffer));
            if (fgets(buffer, BUFFER_SIZE, pf))
                spinner_threads->value(atof(buffer));
            if (fgets(buffer, BUFFER_SIZE, pf))
                input_max_iter->value(atof(buffer));
            if (fgets(buffer, BUFFER_SIZE, pf))
                input_cutoff->value(atof(buffer));
            if (fgets(buffer, BUFFER_SIZE, pf))
                choice_colormap->value(atoi(buffer));
            if (fgets(buffer, BUFFER_SIZE, pf))
                pixel_x_umeters->value(atof(buffer));
            if (fgets(buffer, BUFFER_SIZE, pf))
                pixel_y_umeters->value(atof(buffer));
            fclose(pf);
        }
        else
        {
            central_log->error_line("Error! Open %s failed: %s", fn, strerror(errno));

        }
        fl_cursor(FL_CURSOR_DEFAULT);
	}
}/*****************************************************************************/
void
fc_btn_save_cfg(void)
{
    FILE *pf = NULL;
    char *fn = fl_file_chooser("Save configuration file as", "cfg (*.cfg)", NULL, 0);
	if (fn)
    {
        fl_cursor(FL_CURSOR_WAIT);
        if ((pf = fopen(fn, "w")))
        {        
            fprintf( pf
                   , "%g\n%g\n%g\n%g\n%g\n%d\n%d\n%d\n%g\n%d\n%g\n%g"
                   , source_voltage->value()
                   , conductance_m1->value()
                   , conductance_m2->value()
                   , conductance_m3->value()
                   , conductance_m4->value()
                   , choice_cu_thickness->value()
                   , (unsigned int)spinner_threads->value()
                   , (unsigned int)input_max_iter->value()
                   , input_cutoff->value()
                   , choice_colormap->value()
                   , pixel_x_umeters->value()
                   , pixel_y_umeters->value()
                   );
            fclose(pf);
        }
        else
        {
            central_log->error_line("Error! Save %s failed: %s", fn, strerror(errno));

        }
        fl_cursor(FL_CURSOR_DEFAULT);
    }
}
/*****************************************************************************/
void
fc_btn_save_log(void)
{
    int ret;
	//char rel_fn[FL_PATH_MAX];
	//char buff[FL_PATH_MAX];
	char *fn;
	//fn = buff;
	//const char default_fn="idc2d.log";
	//fl_filename_relative( &rel_fn[0] , "idc2d.log" );
	//fl_filename_relative( &fn[0] , "idc2d.log" );
    fn = fl_file_chooser("Save log file as", "log (*.log)", "idc2d.log" , 0);
    if (fn)
    {
        fl_cursor(FL_CURSOR_WAIT);
        ret = central_log->save_log(fn);
        if (ret)
        {
            central_log->error_line("Error! Save %s failed: %s", fn, strerror(ret));

        }
		else 
		{
			central_log->add_line("Log file save name:"); 
			central_log->add_line( fn);
			central_log->add_line( "" );
	    }
        fl_cursor(FL_CURSOR_DEFAULT);
    }
}
void
fc_btn_qsave_log(void)
{
	char *fn;
	fn = central_log->write_log();
	central_log->add_line("Log file save name:"); 
    fn = central_log->write_log();
    if ( fn ) {
         central_log->add_line( fn);
         central_log->add_line( "" );
         } 
    else {
         central_log->add_line("!File write failed.");
         }
}
/*****************************************************************************/
/*****************************************************************************/
void
fc_btn_open_image(void)
{
    FILE* fp;

    char *fn = fl_file_chooser("Image file", "*.bmp\t*.png", NULL, 0);

    if (fn)
    {                
        btn_check->deactivate();
        btn_analyze->deactivate();
        group_output->deactivate();

        fp = fopen(fn, "rb");
        if (fp == NULL)
        {
            central_log->error_line("Error! Open %s failed: %s", fn, strerror(errno));            
           
            return;
        }

        fl_cursor(FL_CURSOR_WAIT);

        if (rgb_image) 
        { 
            rgb_image->release();
            rgb_image = NULL;
        }
		central_log->add_line("Reading image:");
        central_log->add_line(fn);
        rgb_image = Fl_Shared_Image::get(fn);
        if (rgb_image)
        {
            rgb_depth = rgb_image->d();
            if (3 != rgb_depth)
            {
                central_log->error_line("Error! Unsupported color depth. Only 24-bit images.");
                fl_cursor(FL_CURSOR_DEFAULT);
                return;
            }
            rgb_width  = rgb_image->w();
            rgb_height = rgb_image->h();

            copper_plane_s->value(rgb_image, 1.0f);
			copper_plane_s->set_callback(get_in_pix_coords);
			copper_plane_s->set_send_coords(1);
            current_density_plot_s->set_callback(get_out_pix_coords);
            current_density_plot_s->set_send_coords(1);

            central_log->add_line("Image pixel dimensions: %i wide x %i high", rgb_width, rgb_height);


            btn_check->activate();
        }
        else
        {
            central_log->error_line("Error loading image");
        }
        fl_cursor(FL_CURSOR_DEFAULT);
    }
}
/*****************************************************************************/
void
fc_btn_color(Fl_Button *pButton)
{
    uchar r, g, b;

    Fl::get_color(pButton->color(), r, g, b);

    if (fl_color_chooser(pButton->label(), r, g, b))
    {
        pButton->color(fl_rgb_color(r, g, b));
    }
    pButton->value(0);
}
/*****************************************************************************/
IDC_FP 
dtimeval(const struct timeval *stop, const struct timeval *start, struct timeval *diff)
{
    long    stopTime;
    long    startTime;

    if (!(stop && start))
        return 0.0;

    stopTime    = stop->tv_sec * 1000000 + stop->tv_usec;
    startTime   = start->tv_sec * 1000000 + start->tv_usec;

    stopTime -= startTime;

    if (0 > stopTime)
    {
        stopTime = -1000000;
    }
    if (diff)
    {
        diff->tv_sec    = stopTime / 1000000;
        diff->tv_usec   = stopTime % 1000000;
    }

    return stopTime / 1000000.0;
}
/*****************************************************************************/
int
check_colors(void)
{
    int ret = NO_ERROR;

    plane[MATERIAL_GND].pix_count = 0;
    plane[MATERIAL_VCC].pix_count = 0;
    plane[MATERIAL_FR4].pix_count = 0;
    plane[MATERIAL_M1].pix_count  = 0;
    plane[MATERIAL_M2].pix_count  = 0;
    plane[MATERIAL_M3].pix_count  = 0;
    plane[MATERIAL_M4].pix_count  = 0;
    plane[MATERIAL_XXX].pix_count = 0;

    plane[MATERIAL_GND].material_hash = box_source->color()        & 0xFFFFFF00;   // color for material 0 (GND Copper) - blue
    plane[MATERIAL_VCC].material_hash = box_sink->color()     & 0xFFFFFF00;   // color for material 1 (VCC Copper) - green
    plane[MATERIAL_FR4].material_hash = box_substrate->color()   & 0xFFFFFF00;   // color for material 2 (FR4 Insulator) - black
    plane[MATERIAL_M1].material_hash  = box_material1->color()    & 0xFFFFFF00;   // color for material 3 (Metal - Conductor) - red
    plane[MATERIAL_M2].material_hash  = box_material2->color()    & 0xFFFFFF00;   // color for material 3 (Metal - Conductor) - red
    plane[MATERIAL_M3].material_hash  = box_material3->color()    & 0xFFFFFF00;   // color for material 3 (Metal - Conductor) - red
    plane[MATERIAL_M4].material_hash  = box_material4->color()    & 0xFFFFFF00;   // color for material 3 (Metal - Conductor) - red

    for (int i = MATERIAL_GND; i < N_MATERIALS; i++)
    {
        for (int j = i + 1; j < N_MATERIALS; j++)
        {
            if (plane[i].material_hash == plane[j].material_hash)
            {
                central_log->error_line("Error! Materials %d and %d have equal colors!", i, j);

                ret = 1;
            }
        }
    }
    return ret;
}
/*****************************************************************************/
int
set_pix(unsigned char *pix)
{
    const  char     *jr;
    const  char     *jg;
    const  char     *jb;
    unsigned int    h;      // material hash
    unsigned char   m;      // material
        
    if (NULL == pix)
    {
        central_log->add_line("Allocating memory error!");
        return ERROR_ABORT;
    }

    memset(pix, MATERIAL_FR4, i_dc_get_ns2());
    pix += i_dc_get_nx2();

    jr = rgb_image->data()[0];
    jg = jr + 1;
    jb = jr + 2;

    for (int i = 0; i < rgb_height; i++)
    {
        pix++;
        for (int j = 0; j < rgb_width; j++)
        {
            h = ((((*jr << 8) | *jg & 0xFF) << 8) | *jb & 0xFF) << 8;

            if (h == plane[MATERIAL_GND].material_hash)
                m = MATERIAL_GND;
            else if (h == plane[MATERIAL_VCC].material_hash)
                m = MATERIAL_VCC;
            else if (h == plane[MATERIAL_FR4].material_hash)
                m = MATERIAL_FR4;
            else if (h == plane[MATERIAL_M1].material_hash)
                m = MATERIAL_M1;
            else if (h == plane[MATERIAL_M2].material_hash)
                m = MATERIAL_M2;
            else if (h == plane[MATERIAL_M3].material_hash)
                m = MATERIAL_M3;
            else if (h == plane[MATERIAL_M4].material_hash)
                m = MATERIAL_M4;
            else
                m = MATERIAL_XXX;

            plane[m].pix_count++;
            *pix++ = (MATERIAL_XXX != m) ? m : MATERIAL_FR4;
            
            jr += rgb_depth;
            jg += rgb_depth;
            jb += rgb_depth;
        }
        pix++;
    }
    if (0 == plane[MATERIAL_GND].pix_count)
    {
        central_log->error_line("Error! No GND pixels");
        return -1;
    }
    if (0 == plane[MATERIAL_VCC].pix_count)
    {
        central_log->error_line("Error! No source pixels");
        return -1;
    }
    if (!(plane[MATERIAL_M1].pix_count
          || plane[MATERIAL_M2].pix_count 
          || plane[MATERIAL_M3].pix_count 
          || plane[MATERIAL_M4].pix_count
          ))
    {
        central_log->error_line("Error! No conductor pixels");
        return ERROR_ABORT;
    }

    return NO_ERROR;
}
int 
in_get_image_pixel( const int x
                  , const int y
                  , uchar &r
                  , uchar &g
                  , uchar &b
                  )
{
    unsigned int index;
    if ( rgb_image 
        && (0 <= x) 
        && (x < rgb_width) 
        && (0 <= y)
        && (y < rgb_height))
    {
        index = 3 * (x + rgb_width * y);
        r = *(rgb_image->data()[0]+index);
        g = *(rgb_image->data()[0]+index + 1);
        b = *(rgb_image->data()[0]+index + 2);

        return 0;
    }
    return -1;
}
/*****************************************************************************/
void 
get_in_pix_coords(const int x, const int y)
{
    unsigned char r, g, b;
#if 0
        out_R->value(x);
        out_G->value(y);
#else
    if (!in_get_image_pixel(x, y, r, g, b))
	{
		last_r = r;
		last_g = g;
		last_b = b;
	}

#endif
}
/*****************************************************************************/
void 
get_out_pix_coords(const int x, const int y)
{
    unsigned char r, g, b;
#if 0
        out_R->value(x);
        out_G->value(y);
#else
    if (!oi_get_image_pixel(x, y, r, g, b))
    {
        out_R->value(r);
        out_G->value(g);
        out_B->value(b);
        out_I->value(i_dc_get(IDC_A, x, y));
        out_Ix->value(i_dc_get(IDC_X, x, y));
        out_Iy->value(i_dc_get(IDC_Y, x, y));
        out_V->value(i_dc_get(IDC_V, x, y));
    }
#endif
}
/*****************************************************************************/
void           
fc_choice_colormap(void)
{
    Fl_Shared_Image *output_image;
    
    fl_cursor(FL_CURSOR_WAIT);
    oi_setup(rgb_height, rgb_width);
    oi_set_colormap(choice_colormap->value());
    
    output_image = oi_get_image();

    float scale = current_density_plot_s->scale();
    if (0.0f == scale)
    {
        scale = 1.0f;
    }
    current_density_plot_s->value(output_image, scale);

    if (!btn_save_image->active())
        btn_save_image->activate();
    if (!btn_save_data->active())
        btn_save_data->activate();

    fl_cursor(FL_CURSOR_DEFAULT);
}
/*****************************************************************************/
void
fc_btn_show(void)
{
    group_analyze->hide();
    group_output->show();
    fc_choice_colormap();
}
/*****************************************************************************/
void fc_choice_show(void)
{
    oi_set_source(choice_show->value());
    fc_choice_colormap();
}
/*****************************************************************************/
IDC_FP QuantizeToPrecision(IDC_FP Value, unsigned int precision)
{
	IDC_FP temp;
	temp = Value * pow(10.0,(double)precision) + 0.5;
	temp = (IDC_FP)((long int)temp);
	temp /= pow(10.0,(double)precision);
	return(temp);
}
/*****************************************************************************/
// This routine finds the first matching material to the entered conductance value
// and sets the selector to it. If no match found, the selector is blanked.
void fc_conductance_edit_changed(Fl_Value_Input *pEdit, Fl_Choice *pChoice_material)
{
    IDC_FP value;
    IDC_FP entry;
    int index;
// Get value, see if it exists in the materials database (first match is used)
// if it exists, change the selector to the relevant entry, else blank it
	if((pEdit != NULL) && (pChoice_material != NULL))
	{
		// Quantize value
		value = QuantizeToPrecision(pEdit->value(),2);
//		for(index = 0; index < ((sizeof(Materials)/sizeof(Materials[0]))-1); index++)
		for(index = 0; index < (Material_DB_entries-1); index++)
		{// If the database gets too large, this search method will be too slow
			entry = 100.0/Materials[index].Resistivity; // converted to conductivity of proper units
			// Quantize entry
			entry = QuantizeToPrecision(entry, 2);
			if(entry == value)
			{
				break; // exit loop since match was found
			}
		}
		pChoice_material->value(index); // set to match, last entry (mismatch) reserved for blank
	}
}
/*****************************************************************************/
// Respond to a change in one of the metal choice controls.
// Obtain the conductance for the selected material and set the corresponding
// edit to the specified value.
// In the future, all related properties of the selected metal will be set
// to enable thermal analysis and the inclusion of thermal effects.
void fc_metal_chosen(Fl_Choice* pChoice)
{
	int index;
	IDC_FP	Resistivity;
	IDC_FP	Conductivity;
	Fl_Value_Input *pEdit=(Fl_Value_Input *)0;
	// identify which choice so we know which edit to fill
	// use if/elseif because control pointers are dynamic
	if(pChoice == choice_material_1)
	{
			pEdit = conductance_m1;
	}
	else if(pChoice == choice_material_2)
	{
			pEdit = conductance_m2;
	}
	else if(pChoice == choice_material_3)
	{
			pEdit = conductance_m3;
	}
	else if(pChoice == choice_material_4)
	{
			pEdit = conductance_m4;
	}
	if(pEdit != NULL)
	{
		// get the selected value
		index = pChoice->value();
		if((index >= 0) && (index < (Material_DB_entries-1)))
		{
			Resistivity = Materials[index].Resistivity; // This is in uOhms*cm
			// convert to the correct units, which is mega Simemens per meter
			Conductivity = 100.0/Resistivity;
			// set the edit
			pEdit->value(Conductivity);
		}
	}
}
/*****************************************************************************/

void cb_capture_color( Fl_Image_Display* image_display ) {
	if ( pick_source->value() != 0 ) {
		box_source->color( fl_rgb_color( last_r , last_g , last_b ) );
		pick_source->do_callback(pick_source); // will toggle button back up
	}
	else if ( pick_sink->value() != 0 ) {
		box_sink->color( fl_rgb_color( last_r , last_g , last_b ) );
		pick_sink->do_callback(pick_sink); // will toggle button back up
	}
	else if ( pick_substr->value() != 0 ) {
		box_substrate->color( fl_rgb_color( last_r , last_g , last_b ) );
		pick_substr->do_callback(pick_substr); // will toggle button back up
	}
	else if ( pick_material1->value() != 0 ) {
		box_material1->color( fl_rgb_color( last_r , last_g , last_b ) );
		pick_material1->do_callback(pick_material1); // will toggle button back up
	}
	else if ( pick_material2->value() != 0 ) {
		box_material2->color( fl_rgb_color( last_r , last_g , last_b ) );
		pick_material2->do_callback(pick_material2); // will toggle button back up
	}
	else if ( pick_material3->value() != 0 ) {
		box_material3->color( fl_rgb_color( last_r , last_g , last_b ) );
		pick_material3->do_callback(pick_material3); // will toggle button back up
	}
	else if ( pick_material4->value() != 0 ) {
		box_material4->color( fl_rgb_color( last_r , last_g , last_b ) );
		pick_material4->do_callback(pick_material4); // will toggle button back up
	}
	// clear all picks
    clear_picks();
	sync_color2dec();
    main_panel->redraw();
}

void
clear_picks( void ) {
	pick_source->value(0);
	pick_sink->value(0);
	pick_substr->value(0);
	pick_material1->value(0);
	pick_material2->value(0);
	pick_material3->value(0);
	pick_material4->value(0);
}

void
sync_color2dec( void ) {
	
	uchar r, g, b;
	char buffer[24];

    Fl::get_color(box_source->color(), r, g, b);
	r_source_d->value( r );
	g_source_d->value( g );
	b_source_d->value( b );
	sprintf( buffer , "0x%02x" , r);
	r_source_h->value( buffer );
	sprintf( buffer , "0x%02x" , g);
	g_source_h->value( buffer );
	sprintf( buffer , "0x%02x" , b);
	b_source_h->value( buffer );

    Fl::get_color(box_sink->color(), r, g, b);
	r_sink_d->value( r );
	g_sink_d->value( g );
	b_sink_d->value( b );
	sprintf( buffer , "0x%02x" , r);
	r_sink_h->value( buffer );
	sprintf( buffer , "0x%02x" , g);
	g_sink_h->value( buffer );
	sprintf( buffer , "0x%02x" , b);
	b_sink_h->value( buffer );

	Fl::get_color(box_substrate->color(), r, g, b);
	r_substrate_d->value( r );
	g_substrate_d->value( g );
	b_substrate_d->value( b );
	sprintf( buffer , "0x%02x" , r);
	r_substrate_h->value( buffer );
	sprintf( buffer , "0x%02x" , g);
	g_substrate_h->value( buffer );
	sprintf( buffer , "0x%02x" , b);
	b_substrate_h->value( buffer );

    Fl::get_color(box_material1->color(), r, g, b);
	r_material1_d->value( r );
	g_material1_d->value( g );
	b_material1_d->value( b );
	sprintf( buffer , "0x%02x" , r);
	r_material1_h->value( buffer );
	sprintf( buffer , "0x%02x" , g);
	g_material1_h->value( buffer );
	sprintf( buffer , "0x%02x" , b);
	b_material1_h->value( buffer );

    Fl::get_color(box_material2->color(), r, g, b);
	r_material2_d->value( r );
	g_material2_d->value( g );
	b_material2_d->value( b );
	sprintf( buffer , "0x%02x" , r);
	r_material2_h->value( buffer );
	sprintf( buffer , "0x%02x" , g);
	g_material2_h->value( buffer );
	sprintf( buffer , "0x%02x" , b);
	b_material2_h->value( buffer );

	Fl::get_color(box_material3->color(), r, g, b);
	r_material3_d->value( r );
	g_material3_d->value( g );
	b_material3_d->value( b );
	sprintf( buffer , "0x%02x" , r);
	r_material3_h->value( buffer );
	sprintf( buffer , "0x%02x" , g);
	g_material3_h->value( buffer );
	sprintf( buffer , "0x%02x" , b);
	b_material3_h->value( buffer );

	Fl::get_color(box_material4->color(), r, g, b);
	r_material4_d->value( r );
	g_material4_d->value( g );
	b_material4_d->value( b );
	sprintf( buffer , "0x%02x" , r);
	r_material4_h->value( buffer );
	sprintf( buffer , "0x%02x" , g);
	g_material4_h->value( buffer );
	sprintf( buffer , "0x%02x" , b);
	b_material4_h->value( buffer );
}
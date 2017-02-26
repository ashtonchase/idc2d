#ifndef __BUTTON_CB_H__
#define __BUTTON_CB_H__

#include <FL/Fl_Widget.H>
#include "Fl_image_Display.H"

extern Fl_Menu_Item menu_choice_material[];
extern Fl_Menu_Item menu_choice_cu_thickness[];
extern void cb_metal_chosen(Fl_Choice* pChoice);
extern void cb_conductance_m1_changed(Fl_Value_Input *pEdit);
extern void cb_conductance_m2_changed(Fl_Value_Input *pEdit);
extern void cb_conductance_m3_changed(Fl_Value_Input *pEdit);
extern void cb_conductance_m4_changed(Fl_Value_Input *pEdit);
void cb_capture_color(Fl_Image_Display* image_display);
void sync_color2dec( void );

void clear_picks( void );

int copy2paste(void);

void fc_btn_open_image(void);
void fc_btn_open_cfg(void);
void fc_btn_save_cfg(void);
void fc_btn_color(Fl_Button *pButton);
void fc_btn_check(void);

void fc_btn_analyze(void);
void fc_btn_save_log(void);
void fc_btn_qsave_log(void);
void fc_btn_show(void);

void fc_choice_colormap(void);
void fc_choice_show(void);
void fc_btn_save_image(void);
void fc_btn_save_data(void);

void fc_conductance_edit_changed(Fl_Value_Input *pEdit, Fl_Choice *pChoice_material);
void fc_metal_chosen(Fl_Choice* pChoice);

void calc_thread_run(void);

void get_in_pix_coords(const int x, const int y);

#endif /* __BUTTON_CB_H__   */

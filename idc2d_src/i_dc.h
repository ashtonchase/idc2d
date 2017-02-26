#ifndef __I_DC_H__
#define __I_DC_H__

#define N_MATERIALS     7
#define MATERIAL_XXX    7
#define MATERIAL_M4     6
#define MATERIAL_M3     5
#define MATERIAL_M2     4
#define MATERIAL_M1     3
#define MATERIAL_FR4    2
#define MATERIAL_VCC    1
#define MATERIAL_GND    0

#define IDC_A           0
#define IDC_X           1
#define IDC_Y           2
#define IDC_V           3

typedef struct
{
    unsigned int    material_hash;
    unsigned int    pix_count;
} rgb_materials_t;

typedef double IDC_FP;

// use these functions as the interface to allow C++ implementation in future

unsigned char*  i_dc_get_pixmap(void);
int			    i_dc_boundary(const unsigned char material);
int			    i_dc_init_list(void);
void    	    i_dc_setconductance(const IDC_FP _sigma_x[],const IDC_FP _sigma_y[]);
void		    i_dc_setvcc(IDC_FP vcc_set);
void            i_dc_set_limits(const unsigned int iterations, const IDC_FP cutoff);
int			    i_dc_get_nx2();
int             i_dc_get_ns2();
int             i_dc_get_nx(void);
int	            i_dc_get_ny2(void);
int			    i_dc_allocate(int x	, int y	);
int			    i_dc_set_bond_conductances(void);
IDC_FP*		    i_dc_get_gx(void);
IDC_FP*		    i_dc_get_gy(void);
char*		    i_dc_get_pix(void);
IDC_FP*		    i_dc_get_sigma_x(void	);
IDC_FP*		    i_dc_get_sigma_y(void	);
IDC_FP*		    i_dc_get_u(void	);
IDC_FP*		    i_dc_get_gb(void);
IDC_FP*		    i_dc_get_h(void	);
IDC_FP*		    i_dc_get_Ah(void);
IDC_FP*		    i_dc_get_idc();
IDC_FP*		    i_dc_get_idc_x(void);
IDC_FP*		    i_dc_get_idc_y(void);
int		        i_dc_dbmx(void);  // the field	solver
IDC_FP          i_dc_get(const int what, const int x, const int y);
void            i_dc_get_minmax(const int what, IDC_FP &min, IDC_FP &max);
IDC_FP          i_dc_get_vcc_current(void);

#endif /*  __I_DC_H__  */

/*****************************************************************************/
#include <string.h>
#include <math.h>
#include "i_dc.h"
#include "i_current-2.h" // for the progress bar

/*****************************************************************************/
//#include "debug_support.h"
//      real gx,gy(ns2),u(ns2)
//      real gb(ns2),h(ns2),Ah(ns2)
//      real currx,curry,sigma_x(100,2),sigma_y(100,2),be_x(100,100,2),be_y(100,100,2)
//      real a(100)
//      integer*2 pix(ns2)
//      integer*4 list(ns2)

//  gx(ns2)  - conductances to pixel neighbor in x direction
//  gy(ns2)  - conductances to pixel neighbor in y direction
//  u(ns2)   - voltage at pixel
//
//  gb(ns2)  - gradiant array
//  h(ns2)   - 
//  Ah(ns2)  - 
//  pix(ns2) - phase label for each pixel
//  list(ns2) - list of non-border pixels to allow for faster access
//
//  nx  - bmp image width
//  ny  - bmp image height
//  ns2 - number of elements for above quantities
//      - note that 1 extra pixel bounds the actual BMP input to allow for
//        boundary conditions.
//
//  currx      - average current in x direction
//  curry      - average current in y direction
//  sigma_x(ntot,2) - conductance array for x axis, 1 entry per material type
//  sigma_x(ntot,2) - conductance array for y axis, 1 entry per material type
//  be_x(ntot,ntot,2)    - imaginary conductance matrix or material types for x axis
//  be_y(ntot,ntot,2)    - imaginary conductance matrix or material types for y axis
//  a(ntot)       - fraction of pixels for each phase
//  ntot          - number of material phases in bitmap (initially 0 = FR4, 1 = copper, 2 = copper-VCC, 3 = copper-GND)
//  
//
//  gtest        - cutoff for norm squared of gradient
//                 When gb*gb is less than gtest=abc*ns2, then the rms value 
//                 of the gradient at a pixel is less than sqrt(abc).
//
// idc    - current vector magnitude for each block in the image
// idc_x  - abs X component of vector magnitude
// idc_y  - abs Y component of vector magnitude
//
/*****************************************************************************/
int ntot    = N_MATERIALS;
int nx      = 0;
int ny      = 0;
int nx1     = 0;
int ny1     = 0;
int nx2     = 0;
int ny2     = 0;
int ns2     = 0;
int nlist   = 0;
int start	= 0;
int stop	= 0;
/*****************************************************************************/
IDC_FP *gx     = NULL;
IDC_FP *gy     = NULL;
IDC_FP *u      = NULL; 
IDC_FP *gb     = NULL;
IDC_FP *h      = NULL;
IDC_FP *Ah     = NULL;
IDC_FP *idc    = NULL;
IDC_FP *idc_x  = NULL;
IDC_FP *idc_y  = NULL;
IDC_FP i_vcc   = 0.0;
/*****************************************************************************/
IDC_FP sigma_x[N_MATERIALS];
IDC_FP sigma_y[N_MATERIALS];
IDC_FP be_x[N_MATERIALS * N_MATERIALS];
IDC_FP be_y[N_MATERIALS * N_MATERIALS];
IDC_FP voltage[N_MATERIALS];
static IDC_FP i_max[3];
static IDC_FP i_min[3];
/*****************************************************************************/
unsigned char   *pix    = NULL;
unsigned int    *list   = NULL;
unsigned int    ncgsteps = 30000;
IDC_FP          gtest   = 0;
IDC_FP          vgnd    = 0;              // not adjustable at the moment
IDC_FP          gg      = 0;
IDC_FP          hAh     = 0;
/*****************************************************************************/
extern volatile int v_running;
/*****************************************************************************/
void    i_dc_prod(IDC_FP* gxw, IDC_FP* gyw, IDC_FP* xw, IDC_FP* yw);
void	i_dc_calc_current(void);
void    i_dc_correct_faces(IDC_FP* yw);
/*****************************************************************************/
IDC_FP* i_dc_get_gx(void)          { return (gx); }  // for debugging
IDC_FP* i_dc_get_gy(void)          { return (gy); }
char*   i_dc_get_pix(void)         { return ((char*)pix); } 
int     i_dc_get_ntot(void)        { return (ntot); }
IDC_FP* i_dc_get_sigma_x(void)     { return (sigma_x); }
IDC_FP* i_dc_get_sigma_y(void)     { return (sigma_y); }
IDC_FP* i_dc_get_u(void)           { return (u); }
IDC_FP* i_dc_get_gb(void)          { return (gb); }
IDC_FP* i_dc_get_h(void)           { return (h); }
IDC_FP* i_dc_get_Ah(void)          { return (Ah); }
IDC_FP* i_dc_get_idc(void)         { return (idc); }
IDC_FP* i_dc_get_idc_x(void)       { return (idc_x); }
IDC_FP* i_dc_get_idc_y(void)       { return (idc_y); }
int     i_dc_get_nx2(void)         { return (nx2); }
int     i_dc_get_nx(void)          { return (nx); }
int     i_dc_get_ny2(void)         { return (ny2); }
int     i_dc_get_ns2(void)         { return (ns2); }
IDC_FP  i_dc_get_vcc_current(void) { return i_vcc; }

unsigned char* i_dc_get_pixmap(void) { return (pix); }
/*****************************************************************************/

IDC_FP 
i_dc_get(const int what, const int x, const int y)
{
    if (idc && (0 <= x) && (x < nx2) 
            && (0 <= y) && (y < ny2))
    {
        switch (what)
        {
        case IDC_X:
            return idc_x[(x + 1) + (y + 1) * nx2];
        case IDC_Y:
            return idc_y[(x + 1) + (y + 1) * nx2];
        case IDC_A:
            return idc[(x + 1) + (y + 1) * nx2];
        case IDC_V:
            return u[(x + 1) + (y + 1) * nx2];
        default:
            break;;
        }
    }
    return HUGE_VAL;
}
/*****************************************************************************/
void
i_dc_get_minmax(const int what, IDC_FP &min, IDC_FP &max)
{
    if (IDC_Y >= what)
    {
        min = i_min[what];
        max = i_max[what];
    }
    else if (IDC_V == what)
    {
        min = voltage[MATERIAL_GND];
        max = voltage[MATERIAL_VCC];
    }
}
/*****************************************************************************/
void
i_dc_free(void)
{
    if (gx)
    {
        delete [] gx;
        gx = NULL;
    }
    if (gy)
    {
        delete [] gy;
        gy = NULL;
    }
    if (u)
    {
        delete [] u;
        u = NULL;
    }
    if (gb)
    {
        delete [] gb;
        gb = NULL;
    }
    if (h)
    {
        delete [] h;
        h = NULL;
    }
    if (Ah)
    {
        delete [] Ah;
        Ah = NULL;
    }
    if (idc)
    {
        delete [] idc;
        idc = NULL;
    }
    if (pix)
    {
        delete [] pix;
        pix = NULL;
    }
    if (pix)
    {
        delete [] pix;
        pix = NULL;
    }
    if (list)
    {
        delete [] list;
        list = NULL;
    }
    if (idc_x)
    {
        delete [] idc_x;
        idc_x = NULL;
    }
    if (idc_y)
    {
        delete [] idc_y;
        idc_y = NULL;
    }
}
/*****************************************************************************/
// width is x  height is y
int     
i_dc_allocate(int x, int y)
{
    nx  = x;
    ny  = y;
    nx1 = nx + 1;
    ny1 = ny + 1;
    nx2 = nx + 2;   /* ns + left and right border    */
    ny2 = ny + 2;   /* ns + top and bottom border    */
    
    nlist   = nx * ny;
    // total element count from image size

    if (nx2 * ny2 > ns2)
    {
        i_dc_free();
        ns2 = nx2 * ny2;
        // allocating
        gx      = new IDC_FP[ns2];    
        gy      = new IDC_FP[ns2];
        gb      = new IDC_FP[ns2];
        h       = new IDC_FP[ns2];
        Ah      = new IDC_FP[ns2];
        idc     = new IDC_FP[ns2];
        idc_x   = new IDC_FP[ns2];
        idc_y   = new IDC_FP[ns2];
        u       = new IDC_FP[ns2];
        pix     = new unsigned char[ns2];
        list    = new unsigned int[nlist];    
    }
    else
    {
        ns2 = nx2 * ny2;
    }

    start = nx2;
    stop  = ns2 - nx2 - 1;

    gtest = 1.0e-16 * ns2;

    return (0);
}
/*****************************************************************************/
void 
i_dc_reset(void)
{
    int i;

    #pragma omp for private(i)
    for (i = 0; i < ns2; i++)
    {
        gx[i]   = 0.0;
        gy[i]   = 0.0;
        gb[i]   = 0.0;
        h[i]    = 0.0;
        Ah[i]   = 0.0;
        idc[i]  = 0.0;
        idc_x[i]= 0.0;
        idc_y[i]= 0.0;
        u[i]    = voltage[pix[i]];
    }
	gg = 0.0;
	hAh = 0.0;

}
/*****************************************************************************/
/*****************************************************************************/
void       
i_dc_setvcc(IDC_FP vcc_set)
{
    voltage[MATERIAL_GND] = 0;
    voltage[MATERIAL_VCC] = vcc_set;
    voltage[MATERIAL_FR4] = 0;
    voltage[MATERIAL_M1] = vcc_set / 2;
    voltage[MATERIAL_M2] = vcc_set / 2;
    voltage[MATERIAL_M3] = vcc_set / 2;
    voltage[MATERIAL_M4] = vcc_set / 2;
}          
/*****************************************************************************/
void       
i_dc_setconductance(const IDC_FP _sigma_x[], const IDC_FP _sigma_y[])
{
    memcpy(sigma_x, _sigma_x, N_MATERIALS * sizeof(IDC_FP));
    memcpy(sigma_y, _sigma_y, N_MATERIALS * sizeof(IDC_FP));
}
/*****************************************************************************/
void
i_dc_set_limits(const unsigned int iterations, const IDC_FP cutoff)
{
    ncgsteps = iterations;
    gtest = cutoff * ns2;
}
/*****************************************************************************/
//c  Set values of conductor for phase(i)--phase(j) interface,
//c  store in array be(i,j,m). If either phase i or j has zero conductivity,
//c  then be(i,j,m)=0. 
//c  Do for both x and y axis

int        
i_dc_set_bond_conductances(void)
{
    int i, j;
    int index;

    #pragma omp for private(i, j, index)
    for (i = 0; i < ntot; i++) // number of materials
    {
        for (j = 0; j <= i; j++)
        {
            index = i * ntot + j;
            if (i == j)
            {
                be_x[index] = sigma_x[i];
                be_y[index] = sigma_y[i];
            }
            else
            {
                if ((sigma_x[i] == 0) || (sigma_x[j] == 0))
                {
                    be_x[index] = 0;
                }
                else
                {
                    be_x[index] =(2 * sigma_x[i] * sigma_x[j]) / (sigma_x[i] + sigma_x[j]);
                    be_x[j * ntot + i] = be_x[index];
                }
                if ((sigma_y[i] == 0) || (sigma_y[j] == 0))
                {
                    be_y[index] = 0;
                }
                else
                {
                    be_y[index] =(2 * sigma_y[i] * sigma_y[j]) / (sigma_y[i] + sigma_y[j]);
                    be_y[j * ntot + i] = be_y[index];
                }
            }
        }
    }
    // check original program at this point on initializing boundary
    // --- NOTE: Insulating border applied at pix image
    #pragma omp for private(i)
    for (i = start; i < stop; i++)
    {
        gx[i] = be_x[pix[i] * ntot + pix[i + 1]];     // fill  in gx conductances
        gy[i] = be_y[pix[i] * ntot + pix[i + nx2]];   // fill  in gy conductances
    }
    
    return (0);
}
/*****************************************************************************/

//c  Subroutine that performs the conjugate gradient solution routine to 
//c  find the correct set of nodal voltages
//
//      subroutine dembx(nx2,ny2,ns2,gx,gy,u,ic,gb,h,Ah,list,nlist,gtest)
int        
i_dc_dbmx(void)
{
    unsigned int    step   = 0;
    int             i;
    IDC_FP          gglast = 0;
    IDC_FP          lambda = 0;
    IDC_FP          gamma  = 0;
    IDC_FP 			m;
    unsigned int    l;
// progress bar: (convergence rate is unpredictable, but typically exponential in progress)
//     BarMin  = LOG(gtest)
//     BarMax  = LOG(the very first gg calculated)
//     current = LOG(the last gg calculated), changes each loop, indicates convergence
//     Delta   = BarMax - BarMin
//     BarRpt  = Delta - (current + BarMax)
//               clamp BarRpt between BarMin and BarMax


    IDC_FP          BarMin   = log(gtest);
    IDC_FP          BarMax   = 0;
    IDC_FP          BarDelta = 0;

    //c  First stage, compute initial value of gradient (gb), initialize h, the 
    //c  conjugate gradient direction, and compute norm squared of gradient vector.
    
    #pragma omp parallel
    {
    i_dc_reset();
    i_dc_init_list();
    i_dc_set_bond_conductances();

    i_dc_prod(gx, gy, u, gb); // load gb with the initial gradient value (x axis conductance, y axis conductance by voltage)
    i_dc_correct_faces(gb);
    #pragma omp for private(i)                                                  // not present in loop
    for (i = start; i < stop; i++) // for each block (pixel in image)
    {
        if (MATERIAL_M1 > pix[i]) // FR4, GND and VCC
        {
            gb[i] = 0; // trim non-conducting areas, remove VCC and GND
        }
        h[i] = gb[i]; // trimmed gradients, copy to h
    }
    
    #pragma omp for private(i,m) reduction(+: gg)                               // present in loop
    for (i = 0; i < nlist; i++) // accumulate gradients. Difference between itterations tested against completion limit
    {
        m = gb[list[i]]; // test pixels, transposition, exclude borders
		gg += m * m; // square to drop sign                                     // gg not initialized in this module
    }
    
    //c  Second stage, initialize Ah variable, compute parameter lamdba, 
    //c  make first change in voltage array, update gradient (gb) vector         
                                                                                // loop has stuff not here
    if (gg >= gtest) 
    {                                                                           // from here to loop start identical to loop end
		BarMax   = log(gg);
		BarMin   = log(gtest);
		BarDelta = BarMax - BarMin;
		

// Now that the first gg is calculated, set up the progress bar
//if(ProgBar != NULL)
//{
//    BarMax   = log(gg);
//    BarDelta = BarMax - BarMin;
//    ProgBar->maximum((float) BarDelta);
//    ProgBar->minimum((float) 0.0);
//    ProgBar->value((float) (BarDelta - log(gg) - BarMin));
//}
		Fl::lock();
		ProgBar->maximum((float) BarDelta);
		ProgBar->minimum(0.0f);
		ProgBar->value((float) (BarMax - log(gg)));
		Fl::awake((void*) ProgBar);
		Fl::unlock();

        i_dc_prod(gx, gy, h, Ah);   // load Ah with the initial gradient value (x axis conductance, y axis conductance by trimmed initial gradiants)
        i_dc_correct_faces(Ah);        
        #pragma omp for private(i,l) reduction(+: hAh)
        for (i = 0; i < nlist; i++)
        {
            l = list[i];
			hAh += h[l] * Ah[l];                                                                                                                          // accumulate hAh... don't know function (integral of trimmed gradient * Ah)
        }

        #pragma omp single
        lambda = gg / hAh;                                                                                                                                // initial lambda value. don't know what lambda is. gg is accumulated gradients, hAh is ?

        #pragma omp for private(i)
        for (i = start; i < stop; i++) // for each block (pixel in image)
        {
            u[i] -= lambda * h[i]; // adjust voltage array
            if (MATERIAL_M1 > pix[i]) // FR4, GND and VCC
            {
                gb[i] = 0; // trim non-conducting areas in the gradient array, remove VCC and GND
            }
            else
            {
                gb[i] -= lambda * Ah[i]; // adgust gradient array
            }
        }

        //c  third stage:  iterate conjugate gradient solution process until
        //c  real(gg) < gtest criterion is satisfied.  
        //c  (USER) The parameter ncgsteps is the total number of conjugate gradient steps
        //c  to go through.  Only in very unusual problems, like when the conductivity
        //c  of one phase is much higher than all the rest, will this many steps be 
        //c  used.                  

        while (v_running)
        {
            if (step > ncgsteps)                                                // not in init code
            {
                v_running = 0; // exhausted allowed itterations
            }
            #pragma omp single                                                  // not in init code as a block
            {
                gglast  = gg;  // store last itteration's accumulated gradients // done in init code
                gg      = 0;   // reset gradient accumulator
                hAh     = 0;
                step++;        // count this itteration
            }

            #pragma omp for private(i,m) reduction(+: gg)                       // present in init code
            for (i = 0; i < nlist; i++)
            {                    
                m = gb[list[i]];
				gg += m * m; // accumulate gradients
            }

// Report progress to the user
//if(ProgBar != NULL)
//{
//char str_buffer[32];
//    sprintf(str_buffer, "%g", gg);
//    Fl::lock();
//	ProgBar->value((float) (BarDelta - log(gg) + BarMin));
//	ProgBar->label(str_buffer);
//    Fl::awake((void*) ProgBar);
//    Fl::unlock();
//}
			Fl::lock();
			ProgBar->value((float) (BarMax - log(gg)));
			Fl::awake((void*) ProgBar);
			Fl::unlock();

            if (gg < gtest)                                                     // not in init code
            {
                i_dc_calc_current(); // fill idc, idc_x and idc_y with current vectors for each block. Store magnitudes only.
                break; // exit the loop as we are done!
            }
            #pragma omp single                                                  // not in init code
            gamma = gg / gglast; // convergence ratio

            #pragma omp for private(i)                                          // not in init code
            for (i = start; i < stop; i++) 
            {
                h[i] = gb[i] + h[i] * gamma;                                                                                                              // adjust h array. What is the difference between the h and gb arrays?
            }
// in the init code, this is where the gg >= gtest is placed. All code here to end of while is identical to init code
            i_dc_prod(gx, gy, h, Ah);   // load Ah with new gradient values (x axis conductance, y axis conductance by trimmed and adjusted gradiants)
            i_dc_correct_faces(Ah);
            #pragma omp for private(i,l) reduction(+: hAh)
            for (i = 0; i < nlist; i++)
            {
                l = list[i];
				hAh += h[l] * Ah[l];                                                                                                                      // hAh array adjustment. Don't know what hAh array is
            }

            #pragma omp single
            lambda = gg / hAh;                                                                                                                            // new lambda value. don't know what lambda is. gg is accumulated gradients, hAh is ?

            #pragma omp for private(i)
            for (i = start; i < stop; i++) // for each block (pixel in image)
            {
                u[i] -= h[i] * lambda; // adjust voltage array
                if (MATERIAL_M1 > pix[i]) // FR4, GND and VCC
                {
                    gb[i] = 0; // trim non-conducting areas in the gradient array, remove VCC and GND
                }
                else
                {
                    gb[i]-= Ah[i] * lambda; // adgust gradient array
                }
            }
        } // end while (v_running)
    } // end if (gg >= gtest)
    } // end anonymous block (#pragma omp parallel)
// reset the progress bar to 0
if(ProgBar != NULL)
{
	ProgBar->value((float)0.0);
	ProgBar->label("");
}

    if (gg < gtest)
    {        
        return step;
    }
    return -1;  // error
}
/*****************************************************************************/
/*  X                           V                           Y                  
/*  |\                          |\                          |\                 
/*  | \                         | \                         | \                
/*  |  \                        |  \                        |  \               
/*  |   \                       |   \                       |   \              
/*  |  A \                      |  A \                      |  A \             
/*  |\    |\                    |\    |\                    |\    |\           
/*  | \   | \                   | \   | \                   | \   | \          
/*  |  \ /----------\           |  \  |  \                  |  \  |  \         
/*  |   / |   \      *          |   \ |   /------*------------------\ \        
/*  |  D \|  B \     |\------------D \|  B \     |          |  D \|  B \       
/*  |\  \ |\    |\   |          |\    |\    |\   |          |\    |\  \ |\     
/*  | \  || \   | \  |          | \   | \   | \  |          | \   | \  \| \    
/*  |  \ ||  \  |  \  \         |  \  |  \  |  \  \         |  \  |  \  \  \   
/*  |   \||   \ |   \  \        |   \ |   \ |   \  \        |   \ |   \ ||  \  
/*  |  G ||  E \|  C \  \       |  G \|  E \|  C \  \       |  G \|  E \|| C \ 
/*   \   ||\/ \ |\    \  \       \    |\/   |\    \  \       \    |\/ \ ||    |
/*    \  |||\  \| \   |   \       \   ||\   | \   |   \       \   |/\  |||\   |
/*     \ ||| \  \--------------\   \  || \ /----------------*-----/  \ ||| \  |
/*      \|||  \ |   \ |     \   *   \ ||  / |   \ |     \   |   \ |   \|||  \ |
/*       ||| H \|  F \|      \  |\   \|| H \|  F \|      \  |    \|  H ||| F \|
/*       | |    |\    |      |  | \    |    |\ /  |      |  |      \   |||    |
/*       | |\   | \   |      |  |  \----------/   |      |  |       \  |||\   |
/*       | | \  |  \  |      |  |      | \  |  \  |      |  |        \ ||| \  |
/*       | |  \ |   \ |      |  |      |  \ |   \ |      |  |         \|||  \ |
/*       | |   \|  J \|      |  |      |   \|  J \|      |  |          ||| J \|
/*       | |     \    |      |  |      |     \    |      |  |          | |    |
/*       | |      \   |      |  |      |      \   |      |  |          | |\   |
/*       | |       \  |      |  |      |       \  |      |  |          | | \  |
/*       | |        \ |  |---|--|--div-|        \ |      |  |          | |  \ |
/*       | |         \|  |   |  |   |            \|      |  |          | |   \|
/*       | |             |   |  |   |                    |  |          | |     
/*       | |             |   |  |   sub==>RESULT         |  |          | |     
/*       | |             |   |  |   |                    |  |          | |     
/*       | |             |   |  |   |                    |  |          | |     
/*       | |             |   |  `--\|/-------------------'  |          | |     
/*       | |             |   `------+-----------------------'          | |     
/*       | |             |                                             | |     
/*       | `------------\|/--------------------------------------------' |     
/*       `---------------+-----------------------------------------------'     
 */
void        
i_dc_prod(IDC_FP* gxw, IDC_FP* gyw, IDC_FP* xw, IDC_FP* yw) 
{
    int i;
    #pragma omp for private(i)        // ABC     XcD*vD + XcE*vF + YcE*vH + YcB*vB - (XcD + XcE + YcE + YcB)*vE
    for (i = start; i < stop; i++)    // DEF     where the matrix from the X, Y and Voltage planes are superimposed
    {                                 // GHJ     where Xc is the X conductance plane, Yc is the Y conductance plane, v is the voltage plane and ABCDEFGHJ are matrix members positioned as shown
        yw[i] =  gxw[i - 1] * xw[i - 1]                                  // +  left_X_conductance *  left_voltage
              +  gxw[i] * xw[i + 1]                                      // +       X_conductance * right_voltage
              +  gyw[i] * xw[i + nx2]                                    // +       Y_conductance * below_voltage
              +  gyw[i - nx2] * xw[i - nx2]                              // + above_Y_conductance * above_voltage
              - (gxw[i - 1] + gxw[i] + gyw[i] + gyw[i - nx2]) * xw[i];   // - (left_X_conductance + X_conductance + Y_conductance + above_Y_conductance) * voltage
        
    }
}
/*****************************************************************************/
void        
i_dc_correct_faces(IDC_FP* yw)
{
	// perimeter of matrix not valid at this point.
    // correct x faces
    int i, m;

	
    // perimeter of matrix not valid at this point. Set borders equal to opposite image edge
    // correct x faces
    #pragma omp for private(i,m) 
    for (i = 0; i < ny2; i++)
    {
        m = nx2 * i;
        yw[m + nx1] = yw[m + 1]; // right border = left image
        yw[m] = yw[m + nx];      // left border = right image
    }
    // correct y faces of boundaries
    #pragma omp for private(i)
    for (i = 0; i < nx2; i++)
    {
        yw[i] = yw[nx2 * ny + i];         // top border = image bottom
        yw[ny1 * nx2 + i] = yw[nx2 + i];  // bottom border = image top
    }

} 

/*****************************************************************************/
int        // build a transposition table, account for borders in target, no borders in source
i_dc_init_list(void)
{
    int i, j, k;

    #pragma omp for private(i, j, k)
    for (i = 1; i < nx1; i++) // y_index (why terminate with the x extent?)
    {
        for (j = 1; j < ny1; j++) // x_index (why terminate with the y extent?)
        {
            k = (j - 1) + (i - 1) * (ny1 - 1); // k = linear pix position -- (x_index - 1) + (y_index - 1) * height
            list[k] = j * nx2 + i; // x_index * (width+borders) + y_index
        }
    }
    return (0);
}
/*****************************************************************************/
void       
i_dc_calc_current(void)
{
    int m;
    IDC_FP ix, ix1, ix2;
    IDC_FP iy, iy1, iy2;

    i_vcc = 0.0;
    i_max[IDC_A] = -HUGE_VAL;
    i_max[IDC_X] = -HUGE_VAL;
    i_max[IDC_Y] = -HUGE_VAL;
    
    i_min[IDC_A] = HUGE_VAL;
    i_min[IDC_X] = HUGE_VAL;
    i_min[IDC_Y] = HUGE_VAL;

    #pragma omp for private(m, ix1, ix2, iy1, iy2) reduction(+:i_vcc)
    for (m = start; m < stop; m++) // for each block (image pixel)
    {
        ix1 = gx[m - 1] * (u[m - 1] - u[m]);      // Left side current  = left_X_conductance * deltaE to left
        ix2 = gx[m] * (u[m] - u[m + 1]);          // Right side current =      X_conductance * deltaE to right
        ix  = (ix1 + ix2)/2.0;                          // Total average X current, this block

        iy1 = gy[m - nx2] * (u[m - nx2] - u[m]);  // Top side current    = top_Y_Conductance * deltaE to top
        iy2 = gy[m] * (u[m] - u[m + nx2]);        // Bottom side current =     Y_conductance * delta E to bottom
        iy  = (iy1 + iy2)/2.0;                          // Total average Y current, this block
            
        idc[m] = sqrt(ix * ix + iy * iy);         // This block total current is magnitude of vector(x,y)
        idc_x[m] = fabs(ix);                      // store unsigned X current
        idc_y[m] = fabs(iy);                      // store unsigned Y current

        if (MATERIAL_VCC == pix[m])
        {
            i_vcc += ix1 - ix2 + iy1 - iy2;
        }
        #pragma omp critical
        { // store min and max current values encountered
        if (idc[m] > i_max[IDC_A])
            i_max[IDC_A] = idc[m];
        else if ((MATERIAL_M1 <= pix[m]) && (idc[m] < i_min[IDC_A]))
            i_min[IDC_A] = idc[m];

        if (idc_x[m] > i_max[IDC_X])
            i_max[IDC_X] = idc_x[m];
        else if ((MATERIAL_M1 <= pix[m]) && (idc_x[m] < i_min[IDC_X]))
            i_min[IDC_X] = idc_x[m];

        if (idc_y[m] > i_max[IDC_Y])
            i_max[IDC_Y] = idc_y[m];
        else if ((MATERIAL_M1 <= pix[m]) && (idc_y[m] < i_min[IDC_Y]))
            i_min[IDC_Y] = idc_y[m];
        }
    }    
}
/*****************************************************************************/

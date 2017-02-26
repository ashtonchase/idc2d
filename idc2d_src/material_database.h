
#ifndef __MATERIAL_DATABASE_H__
#define __MATERIAL_DATABASE_H__


#include "i_dc.h" // if it isn't already included, we need the IDC_FP definition

typedef struct tag_Material_Properties
{
	IDC_FP	Density;                // Density @ 20 degrees C, g/cm3
	IDC_FP	Thermal_Conductivity;   // Thermal Conductivity 0-100 degrees C, W/m*K
	IDC_FP	Resistivity;            // Resistivity @20 degrees C, uOhm*cm
	IDC_FP	Resistance_TempCo;      // Temp Coefficient of Resistivity, 0-100 degrees C, K^-1 * 10^-3
	IDC_FP	Expansion_TempCo;       // Coefficient of Expansion, 0-100 degrees C, K^-1 * 10^-6
	char	*pName;
} Material_Properties;

extern Material_Properties Materials[];
extern const int Material_DB_entries;

#endif /*  __MATERIAL_DATABASE_H__  */

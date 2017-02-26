
#include "material_database.h"

Material_Properties Materials[] = { // This is the materials database
// The numbers for raw materials came from the Electronic Engineer's handbook, 2nd edition, McGraw Hill (C)1982
// The numbers for solders came from various manufacturer's published data.
// Use menu_choice_material's value as index into this property array
// Values not known at time of writing:
//
//    "Solder SAC359 Sn95.6Ag3.5Cu0.9".Density                  <- 7.4 used as a placeholder
//    "Solder SN100C Sn99.3Cu0.65Ni0.05Ge".Density              <- 7.4 used as a placeholder
//
//    "Solder SAC359 Sn95.6Ag3.5Cu0.9".Thermal_Conductivity     <- 64 used as a placeholder
//    "Solder SAC387 Sn95.5Ag3.8Cu0.7".Thermal_Conductivity     <- 64 used as a placeholder
//    "Solder SAC396 Sn95.5Ag3.9Cu0.6".Thermal_Conductivity     <- 64 used as a placeholder
//    "Solder SAC405 Sn95.5Ag4.0Cu0.5".Thermal_Conductivity     <- 64 used as a placeholder
//    "Solder SN100C Sn99.3Cu0.65Ni0.05Ge".Thermal_Conductivity <- 64 used as a placeholder
//    "Solder Sn99.3Cu0.7".Thermal_Conductivity                 <- 64 used as a placeholder
//    "Solder Sn42.0Bi58.0".Thermal_Conductivity                <- 100 used as a placeholder
//
//    "Solder SAC359 Sn95.6Ag3.5Cu0.9".Resistivity              <- 13.5 used as a placeholder
//    "Solder SAC396 Sn95.5Ag3.9Cu0.6".Resistivity              <- 12.65 used as a placeholder
//    "Solder SAC405 Sn95.5Ag4.0Cu0.5".Resistivity              <- 12.2 used as a placeholder
//
//    "Solder H63A Sn63Pb37".Resistance_TempCo
//    "Solder Sn60Pb40".Resistance_TempCo
//    "Solder Castin Sn96.1Ag2.6Cu0.8Sb0.5".Resistance_TempCo
//    "Solder SAC305 Sn96.5Ag3.0Cu0.5".Resistance_TempCo
//    "Solder SAC359 Sn95.6Ag3.5Cu0.9".Resistance_TempCo
//    "Solder SAC387 Sn95.5Ag3.8Cu0.7".Resistance_TempCo
//    "Solder SAC396 Sn95.5Ag3.9Cu0.6".Resistance_TempCo
//    "Solder SAC405 Sn95.5Ag4.0Cu0.5".Resistance_TempCo
//    "Solder SN100C Sn99.3Cu0.65Ni0.05Ge".Resistance_TempCo
//    "Solder Sn99.3Cu0.7".Resistance_TempCo
//    "Solder Sn42.0Bi58.0".Resistance_TempCo
//
//    "Solder Castin Sn96.1Ag2.6Cu0.8Sb0.5".Expansion_TempCo
//    "Solder SAC305 Sn96.5Ag3.0Cu0.5".Expansion_TempCo
//    "Solder SAC359 Sn95.6Ag3.5Cu0.9".Expansion_TempCo
//    "Solder SAC387 Sn95.5Ag3.8Cu0.7".Expansion_TempCo
//    "Solder SAC396 Sn95.5Ag3.9Cu0.6".Expansion_TempCo
//    "Solder SAC405 Sn95.5Ag4.0Cu0.5".Expansion_TempCo
//    "Solder SN100C Sn99.3Cu0.65Ni0.05Ge".Expansion_TempCo
//
// Unknown densities are loaded with something close here. These need updating as values are learned.
// Unknown thermal conductivities are loaded with something close here. These need updating as values are learned.
// Unknown resistivities are loaded with something close here. These need updating as values are learned.
// Unknown tempcos are all left at 0, effectively disabling them (no temp slope). These will need something else when the member is used.
//
//         Density       Resistivity         Expansion_TempCo 
//               Thermal_Conductivity  Resistance_TempCo   pName
// Materials that may be used in PCB construction or the components mounted thereon
	{  2.7 ,   2.38,   2.67      , 4.5 , 23.5, "Aluminum (Al)"                       }, // index 0 
	{  6.68,  23.8 ,  40.1       , 5.1 , 11.0, "Antimony (Sb)"                       }, // index 1 
	{  9.8 ,   9.0 , 117.0       , 4.6 , 13.4, "Bismuth (Bi)"                        }, // index 2 
	{  8.64, 103.0 ,   7.3       , 4.3 , 31.0, "Cadmium (Cd)"                        }, // index 3 
	{  7.1 ,  91.3 ,  13.2       , 2.14,  6.5, "Chromium (Cr)"                       }, // index 4 
	{  8.96, 397.0 ,   1.724     , 3.9 , 17.0, "Copper Foil (Cu)"                         }, // index 5 
    {  8.96, 397.0 ,   1.694     , 4.3 , 17.0, "Copper Pure (Cu)"                         }, // index 6 
	{ 19.3 , 315.5 ,   2.2       , 4.0 , 14.1, "Gold (Au)"                           }, // index 7 
//	{  7.3 ,  80.0 ,   8.8       , 5.2 , 24.8, "Indium (In)"                         }, // index 7 -- out to make space for Cu-PCB
	{  7.87,  78.2 ,  10.1       , 6.5 , 12.1, "Iron (Fe)"                           }, // index 8 
	{ 11.68,  34.9 ,  20.6       , 4.2 , 29.0, "Lead (Pb)"                           }, // index 9 
	{ 10.2 , 137.0 ,   5.7       , 4.35,  5.1, "Molybdenum (Mo)"                     }, // index 10
	{  8.9 ,  88.5 ,   6.9       , 6.8 , 13.3, "Nickel (Ni)"                         }, // index 11
	{ 12.0 ,  75.5 ,  10.8       , 4.2 , 11.0, "Palladium (Pd)"                      }, // index 12
	{ 21.45,  71.5 ,  10.58      , 3.92,  9.0, "Platinum (Pt)"                       }, // index 13
	{ 10.5 , 425.0 ,   1.63      , 4.1 , 19.1, "Silver (Ag)"                         }, // index 14
	{  7.3 ,  73.2 ,  12.6       , 4.6 , 23.5, "Tin (Sn)"                            }, // index 15
	{ 19.3 , 174.0 ,   5.4       , 4.8 ,  4.5, "Tungsten (W) "                       }, // index 16
	{  7.14, 119.5 ,   5.96      , 4.2 , 31.0, "Zinc (Zn)"                           }, // index 17
// Standard Tin Lead Solder formulations:
	{  8.4 ,  50.0 ,  14.95      , 0.0 , 25.0, "Solder H63A Sn63Pb37"                }, // index 18
	{  8.5 ,  49.0 ,  14.96      , 0.0 , 25.0, "Solder Sn60Pb40"                     }, // index 19
// Standard lead free Solder formulations:
	{  7.37,  57.26,  12.09989448, 0.0 ,  0.0, "Solder Castin Sn96.1Ag2.6Cu0.8Sb0.5" }, // index 20
	{  7.4 ,  64.0 ,  14.99956397, 0.0 ,  0.0, "Solder SAC305 Sn96.5Ag3.0Cu0.5"      }, // index 21
	{  7.4 ,  64.0 ,  13.5       , 0.0 ,  0.0, "Solder SAC359 Sn95.6Ag3.5Cu0.9"      }, // index 22
	{  7.4 ,  64.0 ,  13.03030303, 0.0 ,  0.0, "Solder SAC387 Sn95.5Ag3.8Cu0.7"      }, // index 23
	{  7.4 ,  64.0 ,  12.65      , 0.0 ,  0.0, "Solder SAC396 Sn95.5Ag3.9Cu0.6"      }, // index 24
	{  7.4 ,  64.0 ,  12.2       , 0.0 ,  0.0, "Solder SAC405 Sn95.5Ag4.0Cu0.5"      }, // index 25
	{  7.4 ,  64.0 ,  12.37410072, 0.0 ,  0.0, "Solder SN100C Sn99.3Cu0.65Ni0.05Ge"  }, // index 26
	{  8.19,  76.0 ,  19.54545455, 0.0 , 28.0, "Solder Sn99.3Cu0.7"                  }, // index 27
	{  8.72, 100.0 ,  38.22222222, 0.0 , 13.8, "Solder Sn42.0Bi58.0"                 }, // index 28
	{ 0.0, 0.0, 0.0, 0.0, 0.0, "Conductive Material"},
};

const int Material_DB_entries = (sizeof(Materials)/sizeof(Materials[0]));


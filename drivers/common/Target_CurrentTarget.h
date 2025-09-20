//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \file     Target.h
//! \brief    Specifies supporting hardware environment: external component values, processor
//!           resources, hardware capabilities, etc.
//!
//! \details  The purpose of this header file is to select the correct hardware environment
//!           specifications as specified by the build environment. It is serves to direct
//!           the preprocessor to import the correct specifications for the hardware environment
//!           targetted in Build.h.
//!
//! \note     This file is not meant to be edited unless a new hardware version or
//!           platform is implemented.
//!
//! \section  hardware_defs Hardware Definition Files
//!
//!           Hardware definition files are expected to define component values and symbolic
//!           names for pins used for drivers. All hardware target specifications are prefixed
//!           with "TARGET_" and use descriptive symbolic names.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _TARGET_CURRENT_TARGET_H_
#define _TARGET_CURRENT_TARGET_H_

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // Hardware Targets
    //////////////////////////////////////////////////////////////////////////////////////////////////        

    #define BUILD_TARGET_OPEN207V_C
	
	// add you hardware target here!

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // Include the hardware definitions as specified by build parameters
    //////////////////////////////////////////////////////////////////////////////////////////////////
    #if defined(BUILD_TARGET_OPEN207V_C)
        // Import hardware target
        #include "Target_Project_Open_207V_C.h"
		// alert what is the current target under use
		#warning target selected = "Target_Project_Open_207V_C.h"
    #elif defined(OTHER_TARGET)
        #error "Target not defined."
    #else
        #error "Target not defined."
    #endif

#endif /* _TARGET_CURRENT_TARGET_H_*/

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////




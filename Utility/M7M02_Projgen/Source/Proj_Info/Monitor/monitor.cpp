/******************************************************************************
Filename    : monitor.cpp
Author      : pry
Date        : 16/07/2019
Licence     : LGPL v3+; see COPYING for details.
Description : The virtual machine monitor class.
******************************************************************************/

/* Include *******************************************************************/
extern "C"
{
#include "xml.h"
}

#include "set"
#include "map"
#include "string"
#include "memory"
#include "vector"
#include "stdexcept"
#include "algorithm"

#define __HDR_DEF__
#include "rvm_gen.hpp"
#include "Proj_Info/Monitor/monitor.hpp"
#include "Mem_Info/mem_info.hpp"
#undef __HDR_DEF__

#define __HDR_CLASS__
#include "rvm_gen.hpp"
#include "Proj_Info/Monitor/monitor.hpp"
#include "Mem_Info/mem_info.hpp"
#undef __HDR_CLASS__
/* End Include ***************************************************************/
namespace RVM_GEN
{
/* Function:Monitor::Monitor **************************************************
Description : Constructor for virtual machine monitor class.
Input       : xml_node_t* Root - The node containing the whole project.
              ptr_t Code_Base - The monitor code base.
              ptr_t Data_Base - The monitor data base.
Output      : None.
Return      : None.
******************************************************************************/
/* void */ Monitor::Monitor(xml_node_t* Root, ptr_t Code_Base, ptr_t Data_Base)
{
    try
    {
        /* Code base/size */
        this->Code_Base=Code_Base;
        this->Code_Size=Main::XML_Get_Number(Root,"Code_Size","DXXXX","DXXXX");
        this->Code=std::make_unique<class Mem_Info>("Monitor_Code",this->Code_Base,this->Code_Size,MEM_CODE,MEM_CODE_MONITOR);
        /* Data base/size */
        this->Data_Base=Data_Base;
        this->Data_Size=Main::XML_Get_Number(Root,"Data_Size","DXXXX","DXXXX");
        this->Data=std::make_unique<class Mem_Info>("Monitor_Data",this->Data_Base,this->Data_Size,MEM_DATA,MEM_DATA_MONITOR);
        /* Stack size */
        this->Init_Stack_Size=Main::XML_Get_Number(Root,"Init_Stack_Size","DXXXX","DXXXX");
        this->Sftd_Stack_Size=Main::XML_Get_Number(Root,"Sftd_Stack_Size","DXXXX","DXXXX");
        this->Vmmd_Stack_Size=Main::XML_Get_Number(Root,"Vmmd_Stack_Size","DXXXX","DXXXX");
        /* Extra capability table size */
        this->Extra_Captbl=Main::XML_Get_Number(Root,"Extra_Captbl","DXXXX","DXXXX");
        /* Whether to enable idle sleeping */
        this->Idle_Sleep_Enable=Main::XML_Get_Yesno(Root,"Idle_Sleep_Enable","DXXXX","DXXXX");
        /* Virtual machine priorities */
        this->Virt_Prio=Main::XML_Get_Number(Root,"Virt_Prio","DXXXX","DXXXX");
        /* Virtual machine events */
        this->Virt_Event=Main::XML_Get_Number(Root,"Virt_Event","DXXXX","DXXXX");
        /* Virtual machine mappings */
        this->Virt_Map=Main::XML_Get_Number(Root,"Virt_Map","DXXXX","DXXXX");

        /* Buildsystem */
        this->Buildsystem=Main::XML_Get_String(Root,"Buildsystem","DXXXX","DXXXX");
        /* Toolchain */
        this->Toolchain=Main::XML_Get_String(Root,"Toolchain","DXXXX","DXXXX");
        /* Optimization */
        this->Optimization=Main::XML_Get_String(Root,"Optimization","DXXXX","DXXXX");
        /* Project_Output - relative to workspace */
        this->Project_Output=Main::XML_Get_String(Root,"Project_Output","DXXXX","DXXXX");
        this->Project_Output=Main::Path_Absolute(PATH_DIR, Main::Workspace_Output, this->Project_Output);
        this->Project_Overwrite=Main::XML_Get_Yesno(Root,"Project_Overwrite","DXXXX","DXXXX");
        /* Linker_Output - relative to project */
        this->Linker_Output=Main::XML_Get_String(Root,"Linker_Output","DXXXX","DXXXX");
        this->Linker_Output=Main::Path_Absolute(PATH_DIR, this->Project_Output, this->Linker_Output);
        /* Config_Header_Output - relative to project */
        this->Config_Header_Output=Main::XML_Get_String(Root,"Config_Header_Output","DXXXX","DXXXX");
        this->Config_Header_Output=Main::Path_Absolute(PATH_DIR, this->Project_Output, this->Config_Header_Output);
        /* Boot_Header_Output - relative to project */
        this->Boot_Header_Output=Main::XML_Get_String(Root,"Boot_Header_Output","DXXXX","DXXXX");
        this->Boot_Header_Output=Main::Path_Absolute(PATH_DIR, this->Project_Output, this->Boot_Header_Output);
        /* Boot_Source_Output - relative to project */
        this->Boot_Source_Output=Main::XML_Get_String(Root,"Boot_Source_Output","DXXXX","DXXXX");
        this->Boot_Source_Output=Main::Path_Absolute(PATH_DIR, this->Project_Output, this->Boot_Source_Output);
        /* Hook_Source_Output - relative to project */
        this->Hook_Source_Output=Main::XML_Get_String(Root,"Hook_Source_Output","DXXXX","DXXXX");
        this->Hook_Source_Output=Main::Path_Absolute(PATH_DIR, this->Project_Output, this->Hook_Source_Output);
        this->Hook_Source_Overwrite=Main::XML_Get_Yesno(Root,"Hook_Source_Overwrite","DXXXX","DXXXX");
    }
    catch(std::exception& Exc)
    {
        Main::Error(std::string("Monitor:\n")+Exc.what());
    }
}
/* End Function:Monitor::Monitor *********************************************/

/* Function:Monitor::Mem_Alloc ************************************************
Description : Allocate the kernel objects and memory for RVM user-level library.
Input       : ptr_t Code_Start - The code begin position for RVM.
              ptr_t Data_Start - The data begin position for RVM.
              ptr_t Kom_Order - The kernel memory order.
Output      : None.
Return      : None.
******************************************************************************/
void Monitor::Mem_Alloc(ptr_t Kom_Order)
{
    /* Init stack section - cut out from the data section */
    this->Init_Stack_Size=ROUND_UP_POW2(this->Init_Stack_Size,Kom_Order);
    this->Init_Stack_Base=this->Data_Base+this->Data_Size-this->Init_Stack_Size;
    Main::Info("> Init stack base 0x%llX size 0x%llX.", this->Init_Stack_Base, this->Init_Stack_Size);
    if(this->Init_Stack_Base<=this->Data_Base)
        Main::Error("XXXXX: Monitor data section is not big enough, unable to allocate init thread stack.");
    this->Data_Size=this->Init_Stack_Base-this->Data_Base;

    /* Safety daemon stack section - cut out from the data section */
    this->Sftd_Stack_Size=ROUND_UP_POW2(this->Sftd_Stack_Size,Kom_Order);
    this->Sftd_Stack_Base=this->Data_Base+this->Data_Size-this->Sftd_Stack_Size;
    Main::Info("> Sftd stack base 0x%llX size 0x%llX.", this->Sftd_Stack_Base, this->Sftd_Stack_Size);
    if(this->Sftd_Stack_Base<=this->Data_Base)
        Main::Error("XXXXX: Monitor data section is not big enough, unable to allocate safety daemon thread stack.");
    this->Data_Size=this->Sftd_Stack_Base-this->Data_Base;

    /* Only do these when we are using the virtual machine portion */
    if(this->Virt_Prio!=0)
    {
        /* VMM daemon stack section - cut out from the data section */
        this->Vmmd_Stack_Size=ROUND_UP_POW2(this->Vmmd_Stack_Size,Kom_Order);
        this->Vmmd_Stack_Base=this->Data_Base+this->Data_Size-this->Vmmd_Stack_Size;
        Main::Info("> Hypd stack base 0x%llX size 0x%llX.", this->Vmmd_Stack_Base, this->Vmmd_Stack_Size);
        if(this->Vmmd_Stack_Base<=this->Data_Base)
            Main::Error("XXXXX: Monitor data section is not big enough, unable to allocate monitor daemon thread stack.");
        this->Data_Size=this->Vmmd_Stack_Base-this->Data_Base;
    }
    else
    {
        Main::Info("> No virtual machines exist, skipping allocation for VMM daemon thread.");
        this->Vmmd_Stack_Base=0;
        this->Vmmd_Stack_Size=0;
    }

    /* Monitor data section - whatever is left */
    Main::Info("> Data base 0x%llX size 0x%llX.", this->Data_Base, this->Data_Size);
}
/* End Function:Monitor::Mem_Alloc *******************************************/
}
/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/

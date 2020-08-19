#pragma once
#include "../ir/intermediatecode.h"
#include "../ir/optimize.h"
extern vector<vector<CodeItem>> codetotal;
extern vector<vector<symbolTable>> total;
//void arm_generate_without_register(string sname);
void arm_generate(string sname);

//function stack
//-----------------------------------------> 0
//|									|
//|			paras after 4			|
//|									|	
//----------------------------------------->sp_without_para
//|									|
//|			store regs				|
//|									|
//----------------------------------------->sp_recover
//|									|
//|				VR					|
//|									|
//-------------------------------------
//|									|
//|			  R0~R3				    |
//|									|
//-------------------------------------
//|									|
//|			  local Vars			|
//|									|
//------------------------------------------>sp
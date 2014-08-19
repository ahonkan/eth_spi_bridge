/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME                                              
*
*       vtkeys.h                                       
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet
*
*   DESCRIPTION
*
*       Terminal emulation key mapping table
*
*   DATA STRUCTURES
*
*       NVT_KEY         The data structure containing the value of
*                       local key and its special string of emulation
*                       to be sent to the network.
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

/*
  the users need to modify this table, or add more elements, or more different
  emulation tables, according to their specific terminal simulation and the
  key value that their device and application produce

  in the following table, the emulation strings are known as standard,
  but the local key values are dependent on device and application program,
  for some local key value, we put 0 for the time being.
  EVEN the values are already set, the users still need to check them
  to make sure that the local key values are correct to their device and
  application program.
*/

#ifndef _VTKEYS
#define _VTKEYS

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* define the data structure for mapping table */
typedef struct nvt
{
    INT  key_value;                 /* the local key value */
    CHAR key_string[6];             /* the NVT ESC string */
    CHAR string2[6];                /* spare string */
} NVT_KEY;


/* the table between the key value and strings for vt100 emulation */
NVT_KEY vt100keys[] = {
        { 1421, "\033[A", "" },    /* ctrl-keypad up arrow for vt100 uparrow */
        { 1425, "\033[B", "" },    /* ctrl-keypad down arrow for vt100 downarrow */
        { 1396, "\033[C", "" },    /* ctrl-keypad right arrow for vt100 rightarrow */
        { 1395, "\033[D", "" },    /* ctrl-keypad left arrow for vt100 leftarrow */

        { 4424, "\033[A", "" },    /* grey up arrow for vt100 uparrow       */
        { 4432, "\033[B", "" },    /* grey down arrow for vt100 downarrow   */
        { 4429, "\033[C", "" },    /* grey right arrow for vt100 rightarrow */
        { 4427, "\033[D", "" },    /* grey left arrow for vt100 leftarrow   */

        { 338,  "\033Op", "" },    /* map 0 on keypad to vt100 keypad 0 */
        { 335,  "\033Oq", "" },    /* map 1 on keypad to vt100 keypad 1 */
        { 336,  "\033Or", "" },    /* map 2 on keypad to vt100 keypad 2 */
        { 337,  "\033Os", "" },    /* map 3 on keypad to vt100 keypad 3 */
        { 331,  "\033Ot", "" },    /* map 4 on keypad to vt100 keypad 4 */
        { 332,  "\033Ou", "" },    /* map 5 on keypad to vt100 keypad 5 */
        { 333,  "\033Ov", "" },    /* map 6 on keypad to vt100 keypad 6 */
    { 327,  "\033Ow", "" },    /* map 7 on keypad to vt100 keypad 7 */
    { 328,  "\033Ox", "" },    /* map 8 on keypad to vt100 keypad 8 */
    { 329,  "\033Oy", "" },    /* map 9 on keypad to vt100 keypad 9 */
    { 334,  "\033Ol", "" },    /* map + on keypad to vt100 keypad , */
    { 330,  "\033Om", "" },    /* map - on keypad to vt100 keypad - */
    { 339,  "\033On", "" },    /* map . on keypad to vt100 keypad . */
    { 4365, "\033OM", "" },    /* map enter on keypad to vt100 keypad enter */

    { 315,  "\033OP", "" },    /* F1 = vt100 function key 1 */
    { 316,  "\033OQ", "" },    /* F2 = vt100 function key 2 */
    { 317,  "\033OR", "" },    /* F3 = vt100 function key 3 */
    { 318,  "\033OS", "" },    /* F4 = vt100 function key 4 */
    {   0,  "", "" }
};


/* the table between the key value and strings for vt220 emulation */
NVT_KEY vt220keys[] = {
    {  319,    "\27m", "" },    /* F5 = vt220 f5 */
    {  320, "\27[17~", "" },    /* F6 = vt220 f6 */
    {  321, "\27[18~", "" },    /* F7 = vt220 f7 */
    {  322, "\27[19~", "" },    /* F8 = vt220 f8 */
    {  323, "\27[20~", "" },    /* F9 = vt220 f9 */
    {  324, "\27[21~", "" },    /* F10 = vt220 f10 */
    {  389, "\27[23~", "" },    /* F11 = vt220 f11 */
    {  390, "\27[24~", "" },    /* F12 = vt220 f12 */
    { 2408, "\27[23~", "" },    /* Alt f1 = vt220 f11 */
    { 2409, "\27[24~", "" },    /* Alt f2 = vt220 f12 */
    { 2410, "\27[25~", "" },    /* Alt f3 = vt220 f13 */
    { 2411, "\27[26~", "" },    /* Alt f4 = vt220 f14 */
    { 2412, "\27[28~", "" },    /* Alt f5 = vt220 f15 */
    { 2413, "\27[29~", "" },    /* Alt f6 = vt220 f16 */
    { 2414, "\27[31~", "" },    /* Alt f7 = vt220 f17 */
    { 2415, "\27[32~", "" },    /* Alt f8 = vt220 f18 */
    { 2416, "\27[33~", "" },    /* Alt f9 = vt220 f19 */
    { 2417, "\27[34~", "" },    /* Alt f10 = vt220 f20 */

    /* Set the Enhanced keypad keys to generate other vt220 strings */
    { 4423, "\27[1~", "" },    /* Enhanced Home = vt220 Find key */
    { 4434, "\27[2~", "" },    /* Enhanced Insert = vt220 Insert key */
    { 4435, "\27[3~", "" },    /* Enhanced Delete = vt220 Delete key */
    { 4431, "\27[4~", "" },    /* Enhanced End  = vt220 Select key   */
    { 4425, "\27[5~", "" },    /* Enhanced PgUp = vt220 Prev Screen key */
    { 4433, "\27[6~", "" },    /* Enhanced PgDn = vt220 Next Screen key */
    {   0,  "", "" }
};


/* the table between the key value and strings for vt52 emulation */
NVT_KEY vt52keys[] = {
    { 338,  "\033?p", "" },    /* map 0 on keypad to vt52 keypad 0 */
    { 335,  "\033?q", "" },    /* map 1 on keypad to vt52 keypad 1 */
    { 336,  "\033?r", "" },    /* map 2 on keypad to vt52 keypad 2 */
    { 337,  "\033?s", "" },    /* map 3 on keypad to vt52 keypad 3 */
    { 331,  "\033?t", "" },    /* map 4 on keypad to vt52 keypad 4 */
    { 332,  "\033?u", "" },    /* map 5 on keypad to vt52 keypad 5 */
    { 333,  "\033?v", "" },    /* map 6 on keypad to vt52 keypad 6 */
    { 327,  "\033?w", "" },    /* map 7 on keypad to vt52 keypad 7 */
    { 328,  "\033?x", "" },    /* map 8 on keypad to vt52 keypad 8 */
    { 329,  "\033?y", "" },    /* map 9 on keypad to vt52 keypad 9 */
    { 334,  "\033?l", "" },    /* map + on keypad to vt52 keypad , */
    { 330,  "\033?m", "" },    /* map - on keypad to vt52 keypad - */
    { 339,  "\033?n", "" },    /* map . on keypad to vt52 keypad . */
    { 4365, "\033?M", "" },    /* map enter on keypad to vt52 keypad enter */

    { 315,  "\033P", "" },    /* F1 = vt52 function key 1 */
    { 316,  "\033Q", "" },    /* F2 = vt52 function key 2 */
    { 317,  "\033R", "" },    /* F3 = vt52 function key 3 */
    { 318,  "\033S", "" },    /* F4 = vt52 function key 4 */

        { 0, "\033A", "" },    /* cursor up arrow for vt52 uparrow */
        { 0, "\033B", "" },    /* cursor down arrow for vt52 downarrow */
        { 0, "\033C", "" },    /* cursor right arrow for vt52 rightarrow */
        { 0, "\033D", "" },    /* cursor left arrow for vt52 leftarrow */
        { 0, "\033H", "" },    /* cursor left arrow for vt52 leftarrow */

        { 0, "\033D", "" },    /* grey left arrow for vt52 leftarrow   */
        { 0, "\033A", "" },    /* grey up arrow for vt52 uparrow       */
        { 0, "\033C", "" },    /* grey right arrow for vt52 rightarrow */
        { 0, "\033B", "" },    /* grey down arrow for vt52 downarrow   */

        {   0,  "", "" }
};

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _VTKEYS */

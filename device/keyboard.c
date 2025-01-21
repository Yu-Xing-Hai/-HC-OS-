#include "keyboard.h"
#include "print.h"
#include "interrupt.h"
#include "io.h"
#include "global.h"
#include "stdint.h"
#include "stdbool.h"

#define esc       '\033'
#define backspace '\b'
#define tab       '\t'
#define enter     '\r'
#define delete    '\177'

#define char_invisible 0
#define ctrl_l_char  char_invisible
#define ctrl_r_char  char_invisible
#define shift_l_char char_invisible
#define shift_r_char char_invisible
#define alt_l_char   char_invisible
#define alt_r_char   char_invisible
#define caps_lock_char char_invisible

#define shift_l_make  0x2a
#define shift_r_make  0x36
#define alt_l_make    0x38
#define alt_r_make    0xe038
#define alt_r_break   0xe0b8
#define ctrl_l_make   0x1d
#define ctrl_r_make   0xe01d
#define ctrl_r_break  0xe09d
#define caps_lock_make 0x3a

/*use these parameters to record whethre press these key, 
and ext_scancode was used to record whether the makecode's begin is 0xe0*/
static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode; ////the flag of extend scancode

/*ASCLL code array, who's index is make_code*/
/*keymap[i][0] is the ASCLL who is not combined with shift*/
/*keymap[i][1] is the ASCLL who is combined with shift*/
static char keymap[][2] = {
/*0x00*/        {0,    0},
/*0x01*/        {esc,    esc},
/*0x02*/        {'1',    '!'},
/*0x03*/        {'2',    '@'},
/*0x04*/        {'3',    '#'},
/*0x05*/        {'4',    '$'},
/*0x06*/        {'5',    '%'},
/*0x07*/        {'6',    '^'},
/*0x08*/        {'7',    '&'},
/*0x09*/        {'8',    '*'},
/*0x0a*/        {'9',    '('},
/*0x0b*/        {'0',    ')'},
/*0x0c*/        {'-',    '_'},
/*0x0d*/        {'=',    '+'},
/*0x0e*/        {backspace, backspace},
/*0x0f*/        {tab, tab},
/*0x10*/        {'q',    'Q'},
/*0x11*/        {'w',    'W'},
/*0x12*/        {'e',    'E'},
/*0x13*/        {'r',    'R'},
/*0x14*/        {'t',    'T'},
/*0x15*/        {'y',    'Y'},
/*0x16*/        {'u',    'U'},
/*0x17*/        {'i',    'I'},
/*0x18*/        {'o',    'O'},
/*0x19*/        {'p',    'P'},
/*0x1a*/        {'[',    '{'},
/*0x1b*/        {']',    '}'},
/*0x1c*/        {enter, enter},
/*0x1d*/        {ctrl_l_char, ctrl_l_char},
/*0x1e*/        {'a',    'A'},
/*0x1f*/        {'s',    'S'},
/*0x20*/        {'d',    'D'},
/*0x21*/        {'f',    'F'},
/*0x22*/        {'g',    'G'},
/*0x23*/        {'h',    'H'},
/*0x24*/        {'j',    'J'},
/*0x25*/        {'k',    'K'},
/*0x26*/        {'l',    'L'},
/*0x27*/        {';',    ':'},
/*0x28*/        {'\'',    '"'},
/*0x29*/        {'`',    '~'},
/*0x2a*/        {shift_l_char, shift_l_char},
/*0x2b*/        {'\\',    '|'},
/*0x2c*/        {'z',    'Z'},
/*0x2d*/        {'x',    'X'},
/*0x2e*/        {'c',    'C'},
/*0x2f*/        {'v',    'V'},
/*0x30*/        {'b',    'B'},
/*0x31*/        {'n',    'N'},
/*0x32*/        {'m',    'M'},
/*0x33*/        {',',    '<'},
/*0x34*/        {'.',    '>'},
/*0x35*/        {'/',    '?'},
/*0x36*/        {shift_r_char, shift_r_char},
/*0x37*/        {'*',    '*'},
/*0x38*/        {alt_l_char, alt_l_char},
/*0x39*/        {' ',    ' '},
/*0x3a*/        {caps_lock_char, caps_lock_char}
};


static void intr_keyboard_handler(void) {
    bool ctrl_down_last = ctrl_status;  //the key of control.
    bool shift_down_last = shift_status;
    bool caps_lock_last = caps_lock_status;

    bool break_code;
    uint16_t scancode = inb(KBD_BUF_PORT);

    if(scancode == 0xe0) {
        ext_scancode = true;
        return; //finish this interrupt and waite to receive next Byte.
    }
    if(ext_scancode == true) {
        scancode = ((0xe000) | scancode);  //we just process 2 Byte scancode now.
        ext_scancode = false;
    }

    break_code = ((scancode & 0x0080) != 0);  //whether is break code(true or false).
    if(break_code == true) {
        uint16_t make_code = (scancode &= 0xff7f); //get the make code.
        if(make_code == ctrl_l_make || make_code == ctrl_r_make) {
            ctrl_status = false;
        }
        else if(make_code == shift_l_make || make_code == shift_r_make) {
            shift_status = false;
        }
        else if(make_code == alt_l_make || make_code == alt_r_make) {
            alt_status = false;
        }
        return;  //when scancode is break code, we do nothing.
        //caps_lock need process individually.
    }
    else if((scancode > 0x00 && scancode < 0x3b) || (scancode == alt_r_make) || (scancode == ctrl_r_make)) {  //scancode is make code.
        bool shift = false; //this affect the answer of input's Uppercase,lowercase and different character.
        if((scancode < 0x0e) || (scancode == 0x29) || (scancode == 0x1a) || (scancode == 0x1b) || (scancode == 0x2b) || (scancode == 0x27) || (scancode == 0x28) || (scancode == 0x33) || (scancode == 0x34) || (scancode == 0x35)) {
            //the key of shift affect different character.
            if(shift_down_last == true) {
                shift = true;
            }
        }
        else { //the key of shift affect input's Uppercase and lowercase.
            if((shift_down_last == true) && (caps_lock_status == true)) {
                shift = false;
            }
            else if((shift_down_last == true) || (caps_lock_last == true)) {
                shift = true;
            }
            else {
                shift = false;
            }
        }
        uint8_t index = (scancode &= 0x00ff);
        char cur_char = keymap[index][shift];  //have funny.
        if(cur_char != 0) {  //we use keymap[0] to occupy place.
            put_char(cur_char);
            return;
        }

        if(scancode == ctrl_l_make || scancode == ctrl_r_make) {
            ctrl_status = true;
        }
        else if(scancode == shift_l_make || scancode == shift_r_make) {
            shift_status = true;
        }
        else if(scancode == alt_l_make || scancode == alt_r_make) {
            alt_status = true;
        }
        else if(scancode == caps_lock_make) {
            caps_lock_status = !caps_lock_status;
        }
        else {
            put_str("unknow key\n");
        }
    }
}

void keyboard_init() {
    put_str("keyboard init start\n");
    register_handler(0x21, intr_keyboard_handler);
    put_str("keyboard init done\n");
}
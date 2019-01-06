/* 
 * CS:APP Data Lab 
 * 
 * <Trevor Stanley; Student ID: 103389068   User ID?  trst9490>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* Copyright (C) 1991-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* We do not support C11 <threads.h>.  */
         
/* 
 * bitOr - x|y using only ~ and & 
 *   Example: bitOr(6, 5) = 7
 *   Legal ops: ~ &
 *   Max ops: 8
 *   Rating: 1
 */
/* EXPLANATION - De Morgan's Law:  !(P && Q) = !P || !Q    as well as  !(P || Q) = !P && !Q
 Either x or y will have a double negative, resulting in one OR the other*/
         
int bitOr(int x, int y) {
  return ~(~x & ~y);
}
/* 
 * evenBits - return word with all even-numbered bits set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */

int evenBits(void) {
  /* build word with even bits by shifting (get it to a 32 bit representation and then perform or operation; all even bit locations will have 1's
  - all even bits start at 1 (01010101); then shift by 8 to double this to 16   bits, use oron both of these #s to achieve 0101010101010101
  -repeat this for 16 bit left shift*/
  
  int byt = 0x55;
  int wrd = byt | (byt << 8);
  int TWords = wrd | (wrd << 16);
  return TWords;
}

/* 
 * minusOne - return a value of -1 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 2
 *   Rating: 1
 */

 /*Binary Ones Complement Operator is unary and has the effect of 'flipping' bits; boolean algebra says !0 = 1 */
int minusOne(void) {
  return ~0;
}
/* 
 * allEvenBits - return 1 if all even-numbered bits in word set to 1
 *   Examples allEvenBits(0xFFFFFFFE) = 0, allEvenBits(0x55555555) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */

/*for any x value, the logical right shift will be performed so many times as to always yield a positive value*/
/* use an exclusive or to see if the and'd number is equal to your mask; will need to have the mask match every case (e.g. 32, 16, 8, 4...)*/
int allEvenBits(int x) {
  x &= x>>16;
  x &= x>>8;
  x &= x>>4;
  x &= x>>2;
    return x&1;
}

/* 
 * anyOddBit - return 1 if any odd-numbered bit in word set to 1
 *   Examples anyOddBit(0x5) = 0, anyOddBit(0x7) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int anyOddBit(int x) {
   int bits;
  int Odd =(0xAA << 8) | 0xAA;
  Odd = Odd | (Odd << 16); /* Gives us 1010 1010 1010 1010 1010 1010 1010 1010 */
  bits = x & Odd;      /*irrespective of x's value, only the values for which a 1 is in the odd bit position will remain after & operation*/
  bits = !(!bits); /*We "Mask" the data we don't need here; ! is logical not, if I have bits = 300, then notting it will yield 0, if you not 0, it becomes 1*/
  return bits;
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
  /* get byte shift value by multipying by 8 (w = 3; 2^3 = 8)
   * 255 is a mask of the 8 least significant bits.
   255 << n_shift results in a mask to extract the byte number n. (if n was 1   , then this would result in 0x0000ff00 which is a mask for the second byte   (byte and bit numbering starts at the least most significant end and is 0     based))
  *2nd line: n-byte xor-ed w/ the m-byte. When xor-ed w/ the first component, this results in the original value w/ the n- and m-bytes masked off (set to 0.) 3rd is the n-byte extracted and shifted into the position of the m-byte, 4th is the m-byte extracted and shifted into the the position of the n-byte. These are both xor-ed w/ the original value that had zeros in those spots, which means they are copied into those locations.
   
   
   shift removes specific bytes from x and to
   * store the bytes that will be moved. Then sum */
    
    int n_shift = n << 3;
    int m_shift = m << 3;
    
    return    x ^
             (x & ((255 << n_shift) ^ (255 << m_shift))) ^
          ((((x & (255 << n_shift)) >> n_shift) & 255) << m_shift) ^
          ((((x & (255 << m_shift)) >> m_shift) & 255) << n_shift);
}

/* 
 * addOK - Determine if can compute x+y without overflow
 *   Example: addOK(0x80000000,0x80000000) = 0,
 *            addOK(0x80000000,0x70000000) = 1, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 masks are created from the sign extension of the sign bit into the whole word. The expression ~(mask_x ^ mask_y) will be all 1s if the signs of the operands were the same, otherwise all 0s. Likewise, the      
 expression (mask_x ^ total) will be all 1s if the sign of the first operand and the sign of the result were different, otherwise all 0s. Combining those, we get the outcome that overflow has occurred if the   
 two operands had the same sign, and that sign is not the sign of the sum, i.e. positive + positive = negative, or negative + negative = positive. It's impossible to overflow if the two operands have different   signs, so that case does not matter*/

int addOK(int x, int y) {
  int s = x + y;
  int msk_x = x >> 31;
  int msk_y = y >> 31;
  int ttl = s >> 31;
  return !( ~(msk_x ^ msk_y) & (msk_x ^ ttl));
}


/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
   if condition is true, first expression is evaluated and becomes the result; 
   if false, the second expression is evaluated and becomes the result. 
   only one of two expressions is ever evaluated.
   create msk that's either all 1's or 0's given a value of x. Then and z and y with msk*/

int conditional(int x, int y, int z) {
  /* create mask from x by adding x to all 1's
   * will either make 0x0 if x is 1 or 0xffffffff if x -s 0
   *select either y or z w/ msk*/
    
  int msk = ~0 + !x;
  return (y & msk) | (z & ~msk);
}


/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 if and only if b-a is positive and c- b is positive
 */


int isAsciiDigit(int x) {
    /* Shifts bytes left by 4 and compares it with XOR to 0x3, will return 1 if not 0x3 */
  int comp = (x >> 4) ^ 0x3;

  int is_0_to_9 = ((0x0f & x) + 0x6) & 0xf0;  //checks if 0 to 9 //15 & x + 6 &240, will return 1 if not 0 to 9

  return !(comp | is_0_to_9);  //ors it and bans it, if either one is false (1) will return 0.
}

/* 
 * replaceByte(x,n,c) - Replace byte n in x with c
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: replaceByte(0x12345678,1,0xab) = 0x1234ab78
 *   You can assume 0 <= n <= 3 and 0 <= c <= 255
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 10
 *   Rating: 3
 */

int replaceByte(int x, int n, int c) {
  int msk, shft;
  msk = 0xFF; // 11111111
  shft = n<<3; // same as multiplying n by 8 because 2^3 = 8.
  msk = ~(msk << shft); // shift byte-wide mask, flip to get byte-    wide mask of 0s
  c = c << shft; // shift bits of c into nth byte
  return (x & msk) | c; // apply mask to x, zeroing x's nth byte,  replacing it with byte given by c
}
/* reverseBits - reverse the bits in a 32-bit integer,
              i.e. b0 swaps with b31, b1 with b30, etc
 *  Examples: reverseBits(0x11111111) = 0x88888888
 *            reverseBits(0xdeadbeef) = 0xf77db57b
 *            reverseBits(0x88888888) = 0x11111111
 *            reverseBits(0)  = 0
 *            reverseBits(-1) = -1
 *            reverseBits(0x9) = 0x90000000
 *  Legal ops: ! ~ & ^ | + << >> and unsigned int type
 *  Max ops: 90
 *  Rating: 4
 * create msks w/ 1's for evry-othr byt, evry-othr
 * nibl, evry-othr 2 bits, & evry-othr bit. These msks
 * can thn be usd to msk & shft evry-othr component of x
 * strting by shftng x 16 bits, thn a byt, thn a 
 * nibl & so on. shftng is done bth wys & ea/ way
 * is added to the othr */

int reverseBits(int x) {
    int msk1x = ((0x55<<8)|(0x55));
    int msk1 = ((msk1x<<16)|(msk1x)); /* 0x55555555*/
    int msk2x = ((0x33<<8)|(0x33));
    int msk2 = ((msk2x<<16)|(msk2x)); /* 0x33333333*/
    int msk3x = ((0x0F<<8)|(0x0F));
    int msk3 = ((msk3x<<16)|(msk3x));/* 0x0F0F0F0F */
    int msk4x = ((0x00<<8)|0xFF);
    int msk4 = (msk4x<<16)|msk4x; /* 0x00FF00FF */
    
    unsigned int T = x;
    T = ((T >> 1) & msk1) | ((T & msk1) << 1);
    /*swp consecutive pairs*/
    T = ((T >> 2) & msk2) | ((T & msk2) << 2);
    /*swp nibls*/
    T = ((T >> 4) & msk3) | ((T & msk3) << 4);
    /*swp byts*/
    T = ((T >> 8) & msk4) | ((T & msk4) << 8);
    /*swp 2byt lng pairs*/
    T = ( T >> 16 ) | ( T << 16);
    return T;
}
/*
 * satAdd - adds two numbers but when positive overflow occurs, returns
 *          maximum possible value, and when negative overflow occurs,
 *          it returns minimum positive value.
 *   Examples: satAdd(0x40000000,0x40000000) = 0x7fffffff
 *             satAdd(0x80000000,0xffffffff) = 0x80000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 30
 *   Rating: 4
 */
int satAdd(int x, int y) {
  /* similar to ovrflw, sign of the sum must be the sme as inputs or ovrflw
    Use logic from addOk & conditional functions */
    
    int sm = x + y;
    
    /* sm XOR x, if difrnt get 0; if sme get 1    &  sm XOR if difrnt 0; sme get 1... & returns
    1 only if thy hve sme sign, othrwise returns 0; thn shft rght by 31. */
    int ovrflw = ((sm ^ x) & (sm ^ y)) >> 31;


    return (sm >> ovrflw) ^ (ovrflw << 31);
}

/*
 * Extra credit
 */
/* 
 * float_abs - Return bit-level equivalent of absolute value of f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument..
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_abs(unsigned uf) {
  return 2;
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
  return 2;
}
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
  return 2;
}

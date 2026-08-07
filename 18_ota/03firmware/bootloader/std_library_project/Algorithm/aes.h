#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
// 定义密钥长度
/* AES 128 192 256 */
#define AES_KEY_SIZE 256

// 定义加密方式
#define CBC 1 // 密码分组链接模式
#define ECB 2 // 电话密码本模式
#define CFB 3 // 密码反馈模式
#define OFB 4 // 输出反馈模式
#define CTR 5 // 计数器模式

#define AES_MODE CBC

void rotation_word(unsigned char *word);
void xor_bytes(unsigned char *word1, unsigned char *word2, unsigned char nCount);

/* 用s盒子置换状态数据 */
static void sub_bytes(unsigned char *word, unsigned char nCount, bool bInvert);

// /* GF（2^8）有限域乘法 */
uint8_t gmult(uint8_t a, uint8_t b);
unsigned char gf_mult(unsigned char num);
void shift_rows(unsigned char *word, bool bInvert);
void mix_columns(unsigned char *word, bool bInvert);
void coefmul(uint8_t *a, uint8_t *b, uint8_t *res);

void AES_key_expansion(const void *key);
static void block_encrypt(unsigned char *pState);
static void block_decrypt(unsigned char *pState);

void AES_encrypt(unsigned char *pPlaintext, unsigned char *pCipherText, unsigned int nDataLen, unsigned char *pIV);
void AES_decrypt(unsigned char *pPlaintext, unsigned char *pCipherText, unsigned int nDataLen, unsigned char *pIV);

#endif // _AES_H_
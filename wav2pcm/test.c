#include <stdio.h>
#include <stdlib.h>

#define MAX  (32635)

void encode(unsigned char *dst, short *src, size_t len)
{
    for (int i = 0; i < len; i++)
    {
        //*dst++ =  *src++;
        short pcm = *src++;
        int sign = (pcm & 0x8000) >> 8;
        if (sign != 0)
            pcm = -pcm;
        if (pcm > MAX)   pcm = MAX;
        int exponent = 7;
        int expMask;
        for (expMask = 0x4000; (pcm & expMask) == 0 && exponent > 0; exponent--, expMask >>= 1) {}
        int mantissa = (pcm >> ((exponent == 0) ? 4 : (exponent + 3))) & 0x0f;
        unsigned char alaw = (unsigned char)(sign | exponent << 4 | mantissa);
        *dst++ = (unsigned char)(alaw ^ 0xD5);
    }
}


void decode(short *dst, unsigned char *src, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        unsigned char alaw = *src++;
        alaw ^= 0xD5;
        int sign = alaw & 0x80;
        int exponent = (alaw & 0x70) >> 4;
        int data = alaw & 0x0f;
        data <<= 4;
        data += 8;   //丢失的a 写1
        if (exponent != 0)  //将wxyz前面的1补上
            data += 0x100;
        if (exponent > 1)
            data <<= (exponent - 1);
        *dst++ = (short)(sign == 0 ? data : -data);
    }
}

int main(){
    FILE* file ;
    file = fopen("2.wav", "rb");
    short* frame = (short*)malloc(480);
    unsigned char * data = (unsigned char *)malloc(480);
    while (1)
    {
        size_t read_len = fread(frame, sizeof(short), 480, file);
        if (480 != read_len)
        {
            fseek(file, 0, SEEK_SET);
            break;
        }
        encode(data, frame, read_len);
        decode(frame, data, read_len);
            }
    fclose(file);
    FILE *fp;
    fp = fopen("out.wav", "a");
    fprintf(fp,"%x", data);
    fclose(fp);

    return 0;
}

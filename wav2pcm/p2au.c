/********************************************************************
 function:  g.711 decoder and encoder
 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef unsigned short uint16;
typedef unsigned int uint32;
#pragma pack(push, 1)
// pcm�ļ�ͷ
typedef struct
{
	uint32 ChunkID;             //00H 4 char "RIFF"��־
	uint32 ChunkSize;           //04H 4 long int �ļ����� ���ܳ�-8
	uint32 Format;              //08H 4 char "WAVE"��־
	uint32 SubChunk1ID;         //0CH 4 char "fmt "��־
	uint32 SubChunk1Size;       //10H 4 0x10000000H(PCM)�����ֽ�(����)
	uint16 AudioFormat;         //14H 2 int ��ʽ���0x01HΪPCM��ʽ����������) 0x0100H
	uint16 NumChannels;         //16H 2 int ͨ������������Ϊ1��˫����Ϊ2
	uint32 SampleRate;          //18H 4 int �����ʣ�ÿ��������������ʾÿ��ͨ���Ĳ����ٶȣ�
	uint32 ByteRate;            //1CH 4 long int ������Ƶ���ݴ������ʣ���ֵChannels��SamplesPerSec��BitsPerSample/8
	uint16 BlockAlign;          //20H 2 int ���ݿ�ĵ����������ֽ���ģ�����ֵΪChannels��BitsPerSample/8
	uint16 BitsPerSample;       //22H 2 ÿ����������λ������ʾÿ�������и�������������λ��������ж����������ÿ���������ԣ�������С��һ����
	uint32 DataTag;           	//24H 4 char ���ݱ�Ƿ���data��
	uint32 DataLen;							//28H 4 long int �������ݵĳ���(�ĳ�-44)
}PCM_HEAD, *PPCM_HEAD;

// a-law�ļ�ͷ
typedef struct
{
	uint32 ChunkID;             //00H 4 char "RIFF"��־
	uint32 ChunkSize;           //04H 4 long int �ļ����� ���ܳ�-8
	uint32 Format;              //08H 4 char "WAVE"��־
	uint32 SubChunk1ID;         //0CH 4 char "fmt "��־
	uint32 SubChunk1Size;       //10H 4 0x12000000H(ALAW)
	uint16 AudioFormat;         //14H 2 int ��ʽ��� 0x0600H
	uint16 NumChannels;         //16H 2 int ͨ������������Ϊ1��˫����Ϊ2
	uint32 SampleRate;          //18H 4 int �����ʣ�ÿ��������������ʾÿ��ͨ���Ĳ����ٶȣ�
	uint32 ByteRate;            //1CH 4 long int ������Ƶ���ݴ������ʣ���ֵChannels��SamplesPerSec��BitsPerSample/8
	uint16 BlockAlign;          //20H 2 int ���ݿ�ĵ����������ֽ���ģ�����ֵΪChannels��BitsPerSample/8
	uint32 BitsPerSample;       //22H 2 ÿ����������λ������ʾÿ�������и�������������λ��������ж����������ÿ���������ԣ�������С��һ����
	uint32 WaveFact;						//26H 4 char "fact"��־
	uint32 Temp1;								//2AH 4 0x04000000H
	uint32 Temp2;								//2EH 4 0x00530700H
	uint32 DataTag;           	//32H 4 char ���ݱ�Ƿ���data��
	uint32 DataLen;							//36H 4 long int �������ݵĳ���(�ĳ�-58)
}ALAW_HEAD, *PALAW_HEAD;

#pragma pack(pop)

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)			/* Number of A-law segments. */
#define	SEG_SHIFT	(4)			/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */
#define	BIAS		(0x84)		/* Bias for linear code. */
#define CLIP        8159

static const short seg_aend[8] = { 0x1F, 0x3F, 0x7F, 0xFF,
				0x1FF, 0x3FF, 0x7FF, 0xFFF };
static const short seg_uend[8] = { 0x3F, 0x7F, 0xFF, 0x1FF,
				0x3FF, 0x7FF, 0xFFF, 0x1FFF };


/*********** �������ǲ��ò��ķ�ʽ������ln���㷨 *************/
//short alaw2linear(unsigned char a_val),ulaw2linear(unsigned char u_val);
//char  linear2alaw(short pcm_val),linear2ulaw(short pcm_val);

/*************** ��������ź����ĸ�ѹ������ *********************/
static short search(short	val, short	*table, short	size)
{
	short		i;
	for (i = 0; i < size; i++)
	{
		if (val <= *table++)     //�ҳ�һ���������ź��������ֵ�����ڣ�
		{
			return (i);
		}
	}
	return (size);
}

/********************* PCM 2 a-law ��ѹ�� ****************************/
char linear2alaw(short  pcm_val)	/* 2's complement (16-bit range) */
{
	short		mask;
	short		seg;
	unsigned char	aval;
	pcm_val = pcm_val >> 3;
	if (pcm_val >= 0)
	{
		mask = 0xD5;		/* sign (7th) bit = 1 */
	}
	else
	{
		mask = 0x55;		/* sign bit = 0 */
		pcm_val = -pcm_val - 1;
	}
	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, (short *)seg_aend, (short)8);
	/* Combine the sign, segment, and quantization bits. */
	if (seg >= 8)		/* out of range, return maximum value. */
	{
		return (unsigned char)(0x7F ^ mask);
	}
	else
	{
		aval = (unsigned char)seg << SEG_SHIFT;
		if (seg < 2)
		{
			aval |= (pcm_val >> 1) & QUANT_MASK;
		}
		else
		{
			aval |= (pcm_val >> seg) & QUANT_MASK;
		}
		return (aval ^ mask);
	}
}

/***************** a-law 2 PCM *******************/
/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
short alaw2linear(unsigned char	a_val)
{
	short		t;
	short		seg;
	a_val ^= 0x55;
	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg)
	{
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
		break;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}

/*********************** pcm 2 u law *************************/
char linear2ulaw(short pcm_val)	/* 2's complement (16-bit range) */
{
	short		mask;
	short		seg;
	unsigned char	uval;
	/* Get the sign and the magnitude of the value. */
	pcm_val = pcm_val >> 2;
	if (pcm_val < 0)
	{
		pcm_val = -pcm_val;
		mask = 0x7F;
	}
	else
	{
		mask = 0xFF;
	}
	if (pcm_val > CLIP)
	{
		pcm_val = CLIP;		/* clip the magnitude */
	}
	pcm_val += (BIAS >> 2);

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, (short *)seg_uend, (short)8);
	/*
	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	if (seg >= 8)		/* out of range, return maximum value. */
	{
		return (unsigned char)(0x7F ^ mask);
	}
	else
	{
		uval = (unsigned char)(seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
		return (uval ^ mask);
	}
}


/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */

short ulaw2linear(unsigned char	u_val)
{
	short		t;
	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;
	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;
	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

void print_error()
{
	printf("input Error!\n");
	printf("input argv[1] in filename\n");
	printf("input argv[2] 0=pcm:wav -> a-lam:wav\n");
	printf("              1=a-lam -> a-lam:wav\n");
	printf("              2=a-lam -> pcm:wav\n");
	printf("              3=pcm -> pcm:wav\n");
	printf("              4=pcm -> a-lam:wav\n");
	printf("              5=a-lam:wav -> pcm:wav\n");
	printf("input argv[3] out filename\n");
	printf("input argv[4] filehead len\n");
}

void init_pcm_head(PPCM_HEAD head)
{
	head->ChunkID = 0x46464952;
	//head->ChunkSize = 0xa17fe4;
	head->Format = 0x45564157;
	head->SubChunk1ID = 0x20746d66;
	head->SubChunk1Size = 0x10;
	head->AudioFormat = 0x1;
	head->NumChannels = 0x2;
	head->SampleRate = 0xac44;
	head->ByteRate = 0x2b110;
	head->BlockAlign = 0x4;
	head->BitsPerSample = 0x10;
	head->DataTag = 0x61746164;
	//head->DataLen = 0xa17fc0;
}

void init_alaw_head(PALAW_HEAD head)
{
	head->ChunkID = 0x46464952;
	//head->ChunkSize = 0x50c012;
	head->Format = 0x45564157;
	head->SubChunk1ID = 0x20746d66;
	head->SubChunk1Size = 0x12;//0x10;
	head->AudioFormat = 0x6;
	head->NumChannels = 0x1;//0x2;
	head->SampleRate = 0x3E80;//0x1f40;//0xac44;					// ������
	head->ByteRate = 0x3E80;//0x1f40;//0x2b110;					// ���δ�������
	head->BlockAlign = 0x01;//0x4;						// ������
	head->BitsPerSample = 0x08;//0x10;					// ������
	head->WaveFact = 0x74636166;
	head->Temp1 = 0x4;
	//head->Temp2 = 0x75300;
	head->DataTag = 0x61746164;
	//head->DataLen = 0x50bfe0;
}

int main(int argc, char * argv[])
{
	FILE *fpin = NULL;
	FILE *fpout = NULL;
	PCM_HEAD pcm_head;
	ALAW_HEAD alaw_head;
	char format[5] = ".wav";
	char filename[50];
	char c;
	short pcm_val;
	struct stat st;
	int move_len = 0;
	if (argc != 5 || ('0' > argv[2][0] || argv[2][0] > '5'))
	{
		print_error();
		return -1;
	}
	move_len = atol(argv[4]);
	if ('0' == argv[2][0])	// pcm:wav -> a-lam:wav
	{
		fpin = fopen(argv[1], "r");
		if (NULL == fpin)
		{
			printf("OpenFile Error!\n");
			return -1;
		}
		memset(filename, 0, sizeof(filename));
		if (NULL != strstr(argv[3], format))
		{
			sprintf(filename, "%s", argv[3]);
		}
		else
		{
			sprintf(filename, "%s%s", argv[3], format);
		}
		fpout = fopen(filename, "w+");
		if (NULL == fpout)
		{
			fclose(fpin);
			printf("OpenFile Error!\n");
			return -1;
		}
		if (1 != fread(&pcm_head, sizeof(PCM_HEAD), 1, fpin))
		{
			printf("ReadFile Error!\n");
			goto END;
		}
		if (pcm_head.AudioFormat != 0x1)
		{
			printf("AudioFormat Error!\n");
			goto END;
		}
		memset(&alaw_head, 0, sizeof(ALAW_HEAD));
		alaw_head.ChunkID = pcm_head.ChunkID;
		alaw_head.ChunkSize = (pcm_head.ChunkSize - 36) / 2 + 50;
		alaw_head.Format = pcm_head.Format;
		alaw_head.SubChunk1ID = pcm_head.SubChunk1ID;
		alaw_head.SubChunk1Size = pcm_head.SubChunk1Size;//0x00000012;
		alaw_head.AudioFormat = 0x0006;
		alaw_head.NumChannels = pcm_head.NumChannels;
		alaw_head.SampleRate = pcm_head.SampleRate;
		alaw_head.ByteRate = pcm_head.ByteRate;
		alaw_head.BlockAlign = pcm_head.BlockAlign;
		alaw_head.BitsPerSample = (uint32)pcm_head.BitsPerSample;
		alaw_head.WaveFact = 0x74636166;	// ֵҪ������ "fact"
		alaw_head.Temp1 = 0x00000004;
		alaw_head.Temp2 = (pcm_head.ChunkSize - 36) / 2;//0x00075300;
		alaw_head.DataTag = pcm_head.DataTag;
		alaw_head.DataLen = (pcm_head.ChunkSize - 36) / 2;
		fwrite(&alaw_head, sizeof(ALAW_HEAD), 1, fpout);
		while (1 == fread(&pcm_val, sizeof(short), 1, fpin))
		{
			c = linear2alaw(pcm_val);
			fputc(c, fpout);
		}
	}
	else if ('5' == argv[2][0])	// a-lam:wav -> pcm:wav
	{
		fpin = fopen(argv[1], "r");
		if (NULL == fpin)
		{
			printf("OpenFile Error!\n");
			return -1;
		}
		memset(filename, 0, sizeof(filename));
		if (NULL != strstr(argv[3], format))
		{
			sprintf(filename, "%s", argv[3]);
		}
		else
		{
			sprintf(filename, "%s%s", argv[3], format);
		}
		fpout = fopen(filename, "w+");
		if (NULL == fpout)
		{
			fclose(fpin);
			printf("OpenFile Error!\n");
			return -1;
		}
		if (1 != fread(&alaw_head, sizeof(ALAW_HEAD), 1, fpin))
		{
			printf("ReadFile Error!\n");
			goto END;
		}
		if (alaw_head.AudioFormat != 0x6)
		{
			printf("AudioFormat Error!\n");
			goto END;
		}
		memset(&pcm_head, 0, sizeof(PCM_HEAD));
		pcm_head.ChunkID = alaw_head.ChunkID;
		pcm_head.ChunkSize = alaw_head.DataLen * 2 + 36;
		pcm_head.Format = alaw_head.Format;
		pcm_head.SubChunk1ID = alaw_head.SubChunk1ID;
		pcm_head.SubChunk1Size = alaw_head.SubChunk1Size;//0x00000012;
		pcm_head.AudioFormat = 0x0001;
		pcm_head.NumChannels = alaw_head.NumChannels;
		pcm_head.SampleRate = alaw_head.SampleRate;
		pcm_head.ByteRate = alaw_head.ByteRate;
		pcm_head.BlockAlign = alaw_head.BlockAlign;
		pcm_head.BitsPerSample =(uint16)alaw_head.BitsPerSample;
		pcm_head.DataTag = alaw_head.DataTag;
		pcm_head.DataLen = alaw_head.DataLen * 2;
		fwrite(&pcm_head, sizeof(PCM_HEAD), 1, fpout);
		while (1 == fread(&c, sizeof(char), 1, fpin))
		{
			pcm_val = alaw2linear(c);
			fwrite(&pcm_val, sizeof(short), 1, fpout);
		}
	}
	else
	{
		if (NULL != strstr(argv[1], format))
		{
			printf("FileFormat Error!\n");
			return -1;
		}
		if (stat(argv[1], &st) != 0)
		{
			printf("Read FileSize Error!\n");
			return -1;
		}
		fpin = fopen(argv[1], "r");
		if (NULL == fpin)
		{
			printf("OpenFile Error!\n");
			return -1;
		}
		memset(filename, 0, sizeof(filename));
		if (NULL != strstr(argv[3], format))
		{
			sprintf(filename, "%s", argv[3]);
		}
		else
		{
			sprintf(filename, "%s%s", argv[3], format);
		}
		fpout = fopen(filename, "w+");
		if (NULL == fpout)
		{
			fclose(fpin);
			printf("OpenFile Error!\n");
			return -1;
		}
		if (move_len)
		{
			fseek(fpin, move_len, SEEK_SET);
		}
		if ('1' == argv[2][0])	// a-lam -> a-lam:wav
		{
			init_alaw_head(&alaw_head);
			alaw_head.ChunkSize = (st.st_size - move_len) + 50;
			alaw_head.Temp2 = (st.st_size - move_len);
			alaw_head.DataLen = (st.st_size - move_len);
			fwrite(&alaw_head, sizeof(ALAW_HEAD), 1, fpout);
			while (1 == fread(&c, sizeof(char), 1, fpin))
			{
				fputc(c, fpout);
			}
		}
		else if ('2' == argv[2][0])	// a-lam -> pcm:wav
		{
			init_pcm_head(&pcm_head);
			pcm_head.ChunkSize = (st.st_size - move_len) * 2 + 36;
			pcm_head.DataLen = (st.st_size - move_len) * 2;
			fwrite(&pcm_head, sizeof(PCM_HEAD), 1, fpout);
			while (1 == fread(&c, sizeof(char), 1, fpin))
			{
				pcm_val = alaw2linear(c);
				fwrite(&pcm_val, sizeof(short), 1, fpout);
			}
		}
		else if ('3' == argv[2][0])	// pcm -> pcm:wav
		{
			init_pcm_head(&pcm_head);
			pcm_head.ChunkSize = (st.st_size - move_len) + 36;
			pcm_head.DataLen = (st.st_size - move_len);
			fwrite(&pcm_head, sizeof(PCM_HEAD), 1, fpout);
			while (1 == fread(&pcm_val, sizeof(short), 1, fpin))
			{
				fwrite(&pcm_val, sizeof(short), 1, fpout);
			}
		}
		else if ('4' == argv[2][0])	// pcm -> a-lam:wav
		{
			init_alaw_head(&alaw_head);
			alaw_head.ChunkSize = (st.st_size - move_len) / 2 + 50;
			alaw_head.Temp2 = (st.st_size - move_len) / 2;
			alaw_head.DataLen = (st.st_size - move_len) / 2;
			fwrite(&alaw_head, sizeof(ALAW_HEAD), 1, fpout);
			while (1 == fread(&pcm_val, sizeof(short), 1, fpin))
			{
				c = linear2alaw(pcm_val);
				fputc(c, fpout);
			}
		}
	}
	printf("OK!\n");
END:
	fclose(fpin);
	fclose(fpout);
	return 1;
}

#include	<stdio.h>
#include	<stdlib.h>
#include	<sndfile.h>

void save(short *b1, double *b2, int n);

int main(int argc, char * argv[]){
	SF_INFO sf_info;
	SNDFILE *snd_file;
        char* format;
	short *buf1;
	double *buf2;
	if (argc != 2)
	{
		exit(1);
	}
	sf_info.format = 0;
	snd_file = sf_open(argv[1], SFM_READ, &sf_info);
        if(sf_info.format == 65552){
            format = "ADPCM U-Law";
        }
        else if(sf_info.format == 65553){
            format = "ADPCM A-Law";
        }
        else if(sf_info.format == 65538){
            format = "PCM";
        }
        else{
            format = "Other types of PCM";
        }
        if (sf_info.samplerate == 0){
            printf("Spoiled wav\n");
            exit(1);
        }
	printf("Using %s.\n", sf_version_string());
	printf("File Name : %s\n", argv[1]);
	printf("Sample Rate : %d\n", sf_info.samplerate);
	printf("Channels : %d\n", sf_info.channels);
	printf("Format : %s\n", format);
	printf("Sections  : %d\n", sf_info.sections);
	printf("Seekable  : %d\n", sf_info.seekable);
	printf("Frames   : %d\n", (int)sf_info.frames);
	buf1 = (short *)malloc(sf_info.channels*sf_info.frames * sizeof(short));
	buf2 = (double *)malloc(sf_info.channels*sf_info.frames * sizeof(double));
	sf_readf_short(snd_file, buf1, sf_info.frames);
        sf_seek(snd_file, 0, SEEK_SET);
	sf_readf_double(snd_file, buf2, sf_info.frames);
	save(buf1, buf2, sf_info.frames);
	free(buf1);
	free(buf2);
	FILE *fp3;
        fp3 = fopen("statisics.dat","a");
	fprintf(fp3,"Using %s.\n", sf_version_string());
	fprintf(fp3,"File Name : %s\n", argv[1]);
	fprintf(fp3,"Sample Rate : %d\n", sf_info.samplerate);
	fprintf(fp3,"Channels : %d\n", sf_info.channels);
	fprintf(fp3,"Format : %s\n", format);
	fprintf(fp3,"Sections  : %d\n", sf_info.sections);
	fprintf(fp3,"Seekable  : %d\n", sf_info.seekable);
	fprintf(fp3,"Frames   : %d\n\n", (int)sf_info.frames);
	fclose(fp3);
	sf_close(snd_file);
	return 0;
}

void save(short *b1, double *b2, int n){
	int i;
	FILE *fp1;
	FILE *fp2;
	fp1 = fopen("short.dat", "w");
	fp2 = fopen("double.dat", "w");
	for (i = 0; i < n; i++) {
            fprintf(fp1, "%d\n", (int)b1[i]);
            fprintf(fp2, "%f\n", b2[i]);
	}
	fclose(fp1);
	fclose(fp2);
}

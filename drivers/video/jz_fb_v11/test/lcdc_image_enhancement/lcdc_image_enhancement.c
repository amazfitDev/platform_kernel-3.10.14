#include<stdio.h>
#include<linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>

#include "lcdc_image_enhancement.h"
static int value_2;
static char st = 'w';
static int fb_fd0=0;
void Stop(int signo);
void Stop(int signo)
{
	st = 'q';
	printf("---get ctrl +c \n");
	close(fb_fd0);
}
static int v_a,v_b,v_c;
#if 0
void calcule_vee_table(int mid_x,int mid_y, short table[1024])
{
	int i=0,val;
	for(i=0;i<mid_x;i++){
		table[i] = i*(double)mid_y/mid_x;
	}
	for(i=mid_x;i<1024;i++){
		table[i] = (double)(1024-mid_y)*(i-mid_x)/(1024-mid_x) + mid_y;
	}
#if 0
	int i=0,val;
	double cc;
	for(i=0;i<1024;i++){
		val = i+value;
		table[i]=val > 1023 ? 1023:val;
	//	if ( i == 499*2) table[i]=table[i]&(~0x2);
	}
#endif

#if 0
	int i;
	double cc;
	printf("--value = %f\n",value);
	for(i=0;i<1024;i++){
		cc = 1024*((double)(log(((double)(i*2)/1024)+1))/value);
		table[i]=cc > 1023 ? 1023 : cc;
	}
#endif
#if 1
	for(i=0;i<1024;i++){
		printf("t[%4d]=%4d ",i,table[i]);
		if((i+1)%10==0)
			printf("\n");
	}
	printf("---%d %d\n",mid_x,mid_y);
#endif
#if 0
	for(i=0;i<1023;i++){
		cc = (double)i*255/1024 - v_a;
		val = pow(value,cc) - 1;
		table[i]=val > 1024 ? 1024:val;
		if(i%2 == 0 && table[i]%2 != 0)/
			table[i] -= 1;
		if(i%2 != 0 && table[i]%2 == 0)
			table[i] -= 1;
		table[i]=table[i] > 0 ? table[i]:-table[i];
	}
	for(i=0;i<1024;i++){
		printf("t[%4d]=%4d ",i,table[i]);
		if(i%5==0)
			printf("\n");
	}
#endif
}
#endif


/*
x=0~100
V = 1023*sqrt(2)/100 * x;
y1 = (2046/(sqrt(2)*V) - 1) * x
y2 = ((sqrt(2)*V/(2046-sqrt(2)*V))*x + (1023 - 1023*sqr(2)/(2046-sqrt(2)*V))
*/

#define AMBIT 100
void calcule_table(int value, short table[1024])
{
	double intersect;
	int inter,i;
	intersect = 1024*value / 100;
	inter = (int)intersect;

	for(i=0;i<inter;i++){
		table[i] = i*((double)(AMBIT-value)/value);
	}
	for(i=inter;i<1024;i++){
		table[i] = (value*(double)(i-1023)/(AMBIT-value)) + 1023;
	}

	for(i=0;i<1024;i++){
		printf("t[%4d]=%4d ",i,table[i]);
		if((i+1)%10==0)
			printf("\n");
	}
	printf("\n---------value=%d  intersect=%lf  inter=%d\n",value,intersect,inter);
}

#define P 3.1415926
#define E 0.000001
int main(int argc, char *argv[])
{
	int num=0,value=0,i=0,value1=0,val_s,val_c;
	double val_sin,val_cos,val;
	float value_vee;
	int mid_x,mid_y;
	struct enh_gamma  test_gamma;
	struct enh_csc	  test_csc;
	struct enh_luma	  test_luma;
	struct enh_hue	  test_hue;
	struct enh_chroma test_chroma;
	struct enh_vee	  test_vee;
	struct enh_dither test_dither;
	short table[1024];

	signal(SIGINT, Stop);
	printf("----Select control 1:Hdmi   0:Lcd\n");
	scanf("%d", &num);
	if(num == 1){
		fb_fd0 = open("/dev/graphics/fb1", O_RDWR);
		if (fb_fd0 < 0) {
			printf("open fb 0 fail\n");
			return -1;
		}
	}else{
		fb_fd0 = open("/dev/graphics/fb0", O_RDWR);
		if (fb_fd0 < 0) {
			printf("open fb 0 fail\n");
			return -1;
		}
	}

while(st!='q'){

	num = 0;
	value = 0;
	printf("===================================================\n");
	printf("please select image enhancement options:\n");
	printf("1--CONTRAST_ENH\n");
	printf("2--BRIGHTNESS_ENH\n\n");
	printf("3--HUE_ENH\n");
	printf("5--VEE_ENH\n");
	printf("6--SATURATION_ENH\n");
//	printf("7--DITHER_ENH\n");
	printf("7--GAMMA_ENH\n");
//	printf("8--rgb-ycc\nchoice num= ");
	printf("9--Enable or Disabel enh \nchoice num= ");
	scanf("%d", &num);
	if(num != 8 && num !=9 ){
		printf("===please set your value--( 0~100 ):\n");
		scanf("%d", &value1);
	}

	switch (num){
		case 1:
			//printf("===please set your contrast value--( 0~2047 D:1024):\n");
			//scanf("%d", &value);
			value = value1*2047/100;
			printf("set=%d--real=%d\n",value1,value);
			if (ioctl(fb_fd0, JZFB_GET_LUMA, &test_luma) < 0) {
				printf("JZFB_GET_CSC faild\n");
				return -1;
			}

			test_luma.contrast_en=1;
			test_luma.contrast=value;

			if (ioctl(fb_fd0, JZFB_SET_LUMA, &test_luma) < 0) {
				printf("JZFB_SET_CSC faild\n");
				return -1;
			}

			break;
		case 2:
			//printf("===please set your brightness value--( 0~2047 D:1024):\n");
			//scanf("%d", &value);
			//
			if(value1 >= 50)
				value = (value1-50)*2047/100;
			else
				value = (100-value1)*2047/100;

			printf("set=%d--real=%d\n",value1,value);
			if (ioctl(fb_fd0, JZFB_GET_LUMA, &test_luma) < 0) {
				printf("JZFB_GET_LUMA faild\n");
				return -1;
			}

			test_luma.brightness_en=1;
			test_luma.brightness=value;

			if (ioctl(fb_fd0, JZFB_SET_LUMA, &test_luma) < 0) {
				printf("JZFB_SET_LUMA faild\n");
				return -1;
			}

			break;
		case 3:
			if(value == 25)
				value = 24;
			if(value == 100)
				value = 100;

			val = value1*2*P/100;
			val_sin = sin(val);
			val_cos = cos(val);

			if(val_sin >= -E && val_sin <= E){
				val_s = 1024;
			}else if(val_sin>E){
				val_s = val_sin*1024 + 1 + 1024;
			}else{
				val_s = 3072 - (1+val_sin)*1024;
			}

			if(val_cos >= -E && val_cos <= E){
				val_c = 1024;
			}else if(val_cos > E){
				val_c = val_cos*1024 + 1024;
			}else{
				val_c = 3072 - (1+val_cos)*1024;
			}

			printf("set=%d---val=%lf val_sin=%lf %d val_cos=%lf  val_s=%d val_c=%d \n",
					value1,val,val_sin,(int)val_sin,val_cos,val_s,val_c);

			test_hue.hue_en=1;
			test_hue.hue_sin=val_s;
			test_hue.hue_cos=val_c;

			if (ioctl(fb_fd0, JZFB_SET_HUE, &test_hue) < 0) {
				printf("JZFB_SET_HUE faild\n");
				return -1;
			}
			break;
		case 5:
			value = 100 - value1;
			calcule_table(value, table);
			calcule_table(value, (short *)test_vee.vee_data);
			test_vee.vee_en=1;

			if (ioctl(fb_fd0, JZFB_SET_VEE, &test_vee) < 0) {
				printf("JZFB_SET_VEE faild\n");
				return -1;
			}

			break;
		case 6:
			value = value1*2047/100;
			printf("set=%d--real=%d\n",value1,value);

			if (ioctl(fb_fd0, JZFB_GET_CHROMA, &test_chroma) < 0) {
				printf("JZFB_GET_CHROMA faild\n");
				return -1;
			}
			test_chroma.saturation_en=1;
			test_chroma.saturation=value;

			if (ioctl(fb_fd0, JZFB_SET_CHROMA, &test_chroma) < 0) {
				printf("JZFB_SET_CHROMA faild\n");
				return -1;
			}

			break;
		case 7:
			value = 100 - value1;
			calcule_table(value, table);
			calcule_table(value, (short *)test_gamma.gamma_data);
			test_gamma.gamma_en=1;

			if (ioctl(fb_fd0, JZFB_SET_GAMMA, &test_gamma) < 0) {
				printf("JZFB_SET_GAMMA faild\n");
				return -1;
			}

			break;
		case 8:
			printf("===please set your ycc rgb value--( 0~3 ):\n");
			scanf("%d", &value);
			if (ioctl(fb_fd0, JZFB_GET_CSC, &test_csc) < 0) {
				printf("JZFB_GET_CSC faild\n");
				return -1;
			}

			test_csc.rgb2ycc_en=1;
			test_csc.rgb2ycc_mode=value;
			test_csc.ycc2rgb_en=1;
			test_csc.ycc2rgb_mode=value;

			if (ioctl(fb_fd0, JZFB_SET_CSC, &test_csc) < 0) {
				printf("JZFB_SET_CSC faild\n");
				return -1;
			}
			break;
		case 9:
			printf("===please set your enable or disable  value--( 0~1 ):\n");
			scanf("%d", &value);
			if (ioctl(fb_fd0, JZFB_ENABLE_ENH, &value) < 0) {
				printf("JZFB_ENABLE_ENH faild\n");
				return -1;
			}
			break;
		default:
			printf("--error\n");
			break;

	}
}
	return 0;
}


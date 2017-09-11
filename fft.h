#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
const int N = 1024;
const float PI = 3.1416;
inline void swap (float &a, float &b)
{
	float t;
	t = a;
	a = b;
	b = t;
}
void bitrp (float xreal [], float ximag [], int n)
{
	// 位反转置换 Bit-reversal Permutation
	int i, j, a, b, p;
	for (i = 1, p = 0; i < n; i *= 2)
	{
		p ++;
	}
	for(i = 0; i < n; i ++)
	{
		a = i;
		b = 0;
		for (j = 0; j < p; j ++)
		{
			b = (b << 1) + (a & 1);// b = b * 2 + a % 2;
			a >>= 1;         // a = a / 2;
		}
		if ( b > i)
		{
			swap (xreal [i], xreal [b]);
			swap (ximag [i], ximag [b]);
		}
	}
}
void FFT(float xreal [], float ximag [], int n)
{
	// 快速傅立叶变换，将复数 x 变换后仍保存在 x 中，xreal, ximag 分别是 x 的实部和虚部
	float wreal [N / 2], wimag [N / 2], treal, timag, ureal, uimag, arg;
	int m, k, j, t, index1, index2;
	bitrp (xreal, ximag, n);// 计算 1 的前 n / 2 个 n 次方根的共轭复数 W'j = wreal [j] + i * wimag [j] , j = 0, 1,   , n / 2 - 1
	arg = - 2 * PI / n;
	treal = cos (arg);
	timag = sin (arg);
	wreal [0] = 1.0;
	wimag [0] = 0.0;
	for (j = 1; j < n / 2; j ++)
	{
		wreal [j] = wreal [j - 1] * treal - wimag [j - 1] * timag;
		wimag [j] = wreal [j - 1] * timag + wimag [j - 1] * treal;
	}
	for (m = 2; m <= n; m *= 2)
	{
		for (k = 0; k < n; k += m)
		{
			for (j = 0; j < m / 2; j ++)
			{
				index1 = k + j;
				index2 = index1 + m / 2;
				t = n * j / m;// 旋转因子 w 的实部在 wreal [] 中的下标为 t
				treal = wreal [t] * xreal [index2] - wimag [t] * ximag [index2];
				timag = wreal [t] * ximag [index2] + wimag [t] * xreal [index2];
				ureal = xreal [index1];
				uimag = ximag [index1];
				xreal [index1] = ureal + treal;
				ximag [index1] = uimag + timag;
				xreal [index2] = ureal - treal;
				ximag [index2] = uimag - timag;
			}
		}
	}
}
void IFFT (float xreal [], float ximag [], int n) 
{
	// 快速傅立叶逆变换
	float wreal [N / 2], wimag [N / 2], treal, timag, ureal, uimag, arg;
	int m, k, j, t, index1, index2;
	bitrp (xreal, ximag, n); // 计算 1 的前 n / 2 个 n 次方根 Wj = wreal [j] + i * wimag [j] , j = 0, 1,   , n / 2 - 1 
	arg = 2 * PI / n;
	treal = cos (arg);
	timag = sin (arg);
	wreal [0] = 1.0;
	wimag [0] = 0.0;
	for (j = 1; j < n / 2; j ++)
	{
		wreal [j] = wreal [j - 1] * treal - wimag [j - 1] * timag;
		wimag [j] = wreal [j - 1] * timag + wimag [j - 1] * treal;
	}
	for (m = 2; m <= n; m *= 2)
	{
		for (k = 0; k < n; k += m)
		{
			for (j = 0; j < m / 2; j ++)
			{
				index1 = k + j;
				index2 = index1 + m / 2;
				t = n * j / m;// 旋转因子 w 的实部在 wreal [] 中的下标为 t
				treal = wreal [t] * xreal [index2] - wimag [t] * ximag [index2];
				timag = wreal [t] * ximag [index2] + wimag [t] * xreal [index2];
				ureal = xreal [index1];
				uimag = ximag [index1];
				xreal [index1] = ureal + treal;
				ximag [index1] = uimag + timag;
				xreal [index2] = ureal - treal;
				ximag [index2] = uimag - timag;
			}
		}
	}
	for(j=0; j < n; j ++)
	{
		xreal [j] /= n;
		ximag [j] /= n;
	}
}
void FFT_test ()
{
	char inputfile [] = "input.txt";// 从文件 input.txt 中读入原始数据
	char outputfile [] = "output.txt";// 将结果输出到文件 output.txt 中
	float xreal [N] = {}, ximag [N] = {};
	int n, i;
	FILE *input, *output;
	if (!(input = fopen (inputfile, "r")))
	{
		printf ("Cannot open file. ");
		exit (1);
	}
	if(!(output = fopen (outputfile, "w")))
	{
		printf ("Cannot open file. ");
		exit (1);
	}
	i = 0;
	while ((fscanf (input, "%f%f", xreal + i, ximag + i)) != EOF)
	{
		i ++;
	}
	n = i;// 要求 n 为 2 的整数幂
	while (i > 1)
	{
		if(i % 2)
		{
			fprintf (output, "%d is not a power of 2! ", n);
			exit (1);
		}
		i /= 2;
	}
	FFT (xreal, ximag, n);
	fprintf (output, "FFT:     \n   i      real         imag \n");
	for (i = 0; i < n; i ++)
	{
		fprintf (output, "%4d     %8.4f     %8.4f \n", i, xreal [i], ximag [i]);
	}
	fprintf (output, "================================= ");
	IFFT (xreal, ximag, n);
	fprintf (output, "IFFT:     \n   i      real         imag \n");
	for (i = 0; i < n; i ++)
	{
		fprintf (output, "%4d     %8.4f     %8.4f \n", i, xreal [i], ximag [i]);
	}
	if( fclose (input))
	{
		printf ("File close error. ");
		exit (1);
	}
	if ( fclose (output))
	{
		printf ("File close error. ");
		exit (1);
	}
}

input.txt
0.5751          0
0.4514          0
0.0439          0
0.0272          0
0.3127          0
0.0129          0
0.3840          0
0.6831          0
0.0928          0
0.0353          0
0.6124          0
0.6085          0
0.0158          0
0.0164          0
0.1901          0
0.5869          0


output.txt
FFT:     
 i        real         imag 
 0       4.6485       0.0000 
 1       0.0176       0.3122 
 2       1.1114       0.0429 
 3       1.6776      -0.1353 
 4      -0.2340       1.3897 
 5       0.3652      -1.2589 
 6      -0.4325       0.2073 
 7      -0.1312       0.3763 
 8      -0.1949       0.0000 
 9      -0.1312      -0.3763 
10      -0.4326      -0.2073 
11       0.3652       1.2589 
12      -0.2340      -1.3897 
13       1.6776       0.1353 
14       1.1113      -0.0429 
15       0.0176      -0.3122 
================================= IFFT:     
i      real         imag 
   0       0.5751      -0.0000 
   1       0.4514       0.0000 
   2       0.0439      -0.0000 
   3       0.0272      -0.0000 
   4       0.3127      -0.0000 
   5       0.0129      -0.0000 
   6       0.3840      -0.0000 
   7       0.6831       0.0000 
   8       0.0928       0.0000 
   9       0.0353      -0.0000 
  10       0.6124       0.0000 
  11       0.6085       0.0000 
  12       0.0158       0.0000 
  13       0.0164       0.0000 
  14       0.1901       0.0000 
  15       0.5869      -0.0000 

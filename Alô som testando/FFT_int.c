#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#define M_PI 3.14159265358979323846
#define windowSize 1024
#define hopSize 256

int window_multi[] = {
    0, 9, 37, 84, 150, 235, 338, 461, 602,
    762, 940, 1138, 1354, 1589, 1843, 2116, 2407, 2717,
    3046, 3394, 3760, 4145, 4548, 4970, 5411, 5871, 6349,
    6845, 7361, 7894, 8447, 9018, 9607, 10215, 10841, 11485,
    12148, 12830, 13530, 14248, 14984, 15738, 16511, 17302, 18111,
    18939, 19784, 20648, 21529, 22429, 23346, 24282, 25235, 26207,
    27196, 28203, 29227, 30270, 31330, 32408, 33503, 34616, 35746,
    36894, 38060, 39242, 40443, 41660, 42895, 44146, 45416, 46702,
    48005, 49325, 50662, 52016, 53387, 54775, 56180, 57601, 59039,
    60493, 61964, 63452, 64956, 66476, 68013, 69566, 71135, 72721,
    74322, 75939, 77573, 79222, 80887, 82568, 84265, 85977, 87705,
    89448, 91207, 92981, 94771, 96576, 98396, 100231, 102081, 103946,
    105826, 107721, 109631, 111555, 113494, 115448, 117416, 119398, 121395,
    123406, 125431, 127471, 129524, 131591, 133672, 135767, 137876, 139998,
    142134, 144283, 146446, 148622, 150811, 153014, 155229, 157458, 159699,
    161953, 164220, 166500, 168792, 171096, 173413, 175742, 178084, 180437,
    182803, 185180, 187570, 189971, 192384, 194808, 197244, 199691, 202150,
    204620, 207101, 209593, 212095, 214609, 217134, 219669, 222214, 224771,
    227337, 229914, 232501, 235098, 237705, 240322, 242948, 245584, 248230,
    250886, 253550, 256224, 258908, 261600, 264301, 267011, 269730, 272458,
    275194, 277938, 280691, 283453, 286222, 288999, 291785, 294578, 297379,
    300187, 303003, 305827, 308658, 311496, 314341, 317193, 320052, 322918,
    325790, 328669, 331555, 334446, 337344, 340248, 343159, 346075, 348997,
    351924, 354857, 357796, 360740, 363689, 366643, 369602, 372567, 375536,
    378509, 381488, 384470, 387458, 390449, 393444, 396444, 399447, 402454,
    405465, 408480, 411497, 414519, 417543, 420570, 423601, 426634, 429670,
    432709, 435750, 438794, 441840, 444888, 447939, 450991, 454045, 457101,
    460158, 463217, 466278, 469339, 472402, 475466, 478530, 481596, 484662,
    487729, 490796, 493864, 496932, 500000, 503067, 506135, 509203, 512270,
    515337, 518403, 521469, 524533, 527597, 530660, 533722, 536782, 539841,
    542898, 545954, 549008, 552060, 555111, 558159, 561205, 564249, 567290,
    570329, 573365, 576398, 579429, 582456, 585480, 588502, 591519, 594534,
    597545, 600552, 603555, 606555, 609550, 612542, 615529, 618511, 621490,
    624463, 627432, 630397, 633356, 636310, 639259, 642203, 645142, 648075,
    651003, 653924, 656840, 659751, 662655, 665553, 668444, 671330, 674209,
    677081, 679947, 682806, 685658, 688503, 691341, 694172, 696996, 699812,
    702620, 705421, 708214, 711000, 713777, 716546, 719308, 722061, 724805,
    727541, 730269, 732988, 735698, 738399, 741091, 743775, 746449, 749113,
    751769, 754415, 757051, 759678, 762294, 764901, 767498, 770085, 772662,
    775229, 777785, 780330, 782865, 785390, 787904, 790407, 792898, 795379,
    797849, 800308, 802755, 805191, 807615, 810028, 812429, 814819, 817196,
    819562, 821915, 824257, 826586, 828903, 831207, 833499, 835779, 838046,
    840300, 842541, 844770, 846985, 849188, 851377, 853553, 855716, 857865,
    860001, 862123, 864232, 866327, 868408, 870475, 872528, 874568, 876593,
    878604, 880601, 882583, 884551, 886505, 888444, 890368, 892278, 894173,
    896053, 897918, 899768, 901603, 903423, 905228, 907018, 908792, 910551,
    912294, 914022, 915734, 917431, 919112, 920777, 922426, 924060, 925677,
    927279, 928864, 930433, 931986, 933523, 935043, 936547, 938035, 939506,
    940960, 942398, 943819, 945224, 946612, 947983, 949337, 950674, 951994,
    953297, 954584, 955853, 957104, 958339, 959556, 960757, 961939, 963105,
    964253, 965383, 966496, 967591, 968669, 969729, 970772, 971796, 972803,
    973792, 974764, 975717, 976653, 977570, 978470, 979351, 980215, 981060,
    981888, 982697, 983488, 984261, 985015, 985751, 986470, 987169, 987851,
    988514, 989158, 989784, 990392, 990981, 991552, 992105, 992638, 993154,
    993650, 994128, 994588, 995029, 995451, 995854, 996239, 996606, 996953,
    997282, 997592, 997883, 998156, 998410, 998645, 998861, 999059, 999237,
    999397, 999538, 999661, 999764, 999849, 999915, 999962, 999990, 1000000,
    999990, 999962, 999915, 999849, 999764, 999661, 999538, 999397, 999237,
    999059, 998861, 998645, 998410, 998156, 997883, 997592, 997282, 996953,
    996606, 996239, 995854, 995451, 995029, 994588, 994128, 993650, 993154,
    992638, 992105, 991552, 990981, 990392, 989784, 989158, 988514, 987851,
    987169, 986470, 985751, 985015, 984261, 983488, 982697, 981888, 981060,
    980215, 979351, 978470, 977570, 976653, 975717, 974764, 973792, 972803,
    971796, 970772, 969729, 968669, 967591, 966496, 965383, 964253, 963105,
    961939, 960757, 959556, 958339, 957104, 955853, 954584, 953297, 951994,
    950674, 949337, 947983, 946612, 945224, 943819, 942398, 940960, 939506,
    938035, 936547, 935043, 933523, 931986, 930433, 928864, 927279, 925677,
    924060, 922426, 920777, 919112, 917431, 915734, 914022, 912294, 910551,
    908792, 907018, 905228, 903423, 901603, 899768, 897918, 896053, 894173,
    892278, 890368, 888444, 886505, 884551, 882583, 880601, 878604, 876593,
    874568, 872528, 870475, 868408, 866327, 864232, 862123, 860001, 857865,
    855716, 853553, 851377, 849188, 846985, 844770, 842541, 840300, 838046,
    835779, 833499, 831207, 828903, 826586, 824257, 821915, 819562, 817196,
    814819, 812429, 810028, 807615, 805191, 802755, 800308, 797849, 795379,
    792898, 790407, 787904, 785390, 782865, 780330, 777785, 775229, 772662,
    770085, 767498, 764901, 762294, 759678, 757051, 754415, 751769, 749113,
    746449, 743775, 741091, 738399, 735698, 732988, 730269, 727541, 724805,
    722061, 719308, 716546, 713777, 711000, 708214, 705421, 702620, 699812,
    696996, 694172, 691341, 688503, 685658, 682806, 679947, 677081, 674209,
    671330, 668444, 665553, 662655, 659751, 656840, 653924, 651003, 648075,
    645142, 642203, 639259, 636310, 633356, 630397, 627432, 624463, 621490,
    618511, 615529, 612542, 609550, 606555, 603555, 600552, 597545, 594534,
    591519, 588502, 585480, 582456, 579429, 576398, 573365, 570329, 567290,
    564249, 561205, 558159, 555111, 552060, 549008, 545954, 542898, 539841,
    536782, 533722, 530660, 527597, 524533, 521469, 518403, 515337, 512270,
    509203, 506135, 503067, 500000, 496932, 493864, 490796, 487729, 484662,
    481596, 478530, 475466, 472402, 469339, 466278, 463217, 460158, 457101,
    454045, 450991, 447939, 444888, 441840, 438794, 435750, 432709, 429670,
    426634, 423601, 420570, 417543, 414519, 411497, 408480, 405465, 402454,
    399447, 396444, 393444, 390449, 387458, 384470, 381488, 378509, 375536,
    372567, 369602, 366643, 363689, 360740, 357796, 354857, 351924, 348997,
    346075, 343159, 340248, 337344, 334446, 331555, 328669, 325790, 322918,
    320052, 317193, 314341, 311496, 308658, 305827, 303003, 300187, 297379,
    294578, 291785, 288999, 286222, 283453, 280691, 277938, 275194, 272458,
    269730, 267011, 264301, 261600, 258908, 256224, 253550, 250886, 248230,
    245584, 242948, 240322, 237705, 235098, 232501, 229914, 227337, 224771,
    222214, 219669, 217134, 214609, 212095, 209593, 207101, 204620, 202150,
    199691, 197244, 194808, 192384, 189971, 187570, 185180, 182803, 180437,
    178084, 175742, 173413, 171096, 168792, 166500, 164220, 161953, 159699,
    157458, 155229, 153014, 150811, 148622, 146446, 144283, 142134, 139998,
    137876, 135767, 133672, 131591, 129524, 127471, 125431, 123406, 121395,
    119398, 117416, 115448, 113494, 111555, 109631, 107721, 105826, 103946,
    102081, 100231, 98396, 96576, 94771, 92981, 91207, 89448, 87705,
    85977, 84265, 82568, 80887, 79222, 77573, 75939, 74322, 72721,
    71135, 69566, 68013, 66476, 64956, 63452, 61964, 60493, 59039,
    57601, 56180, 54775, 53387, 52016, 50662, 49325, 48005, 46702,
    45416, 44146, 42895, 41660, 40443, 39242, 38060, 36894, 35746,
    34616, 33503, 32408, 31330, 30270, 29227, 28203, 27196, 26207,
    25235, 24282, 23346, 22429, 21529, 20648, 19784, 18939, 18111,
    17302, 16511, 15738, 14984, 14248, 13530, 12830, 12148, 11485,
    10841, 10215, 9607, 9018, 8447, 7894, 7361, 6845, 6349,
    5871, 5411, 4970, 4548, 4145, 3760, 3394, 3046, 2717,
    2407, 2116, 1843, 1589, 1354, 1138, 940, 762, 602,
    461, 338, 235, 150, 84, 37, 9
};

long long cos_table[] = {-1000000, 0, 707106, 923879, 980785, 995184, 998795, 999698, 999924, 999981};
long long sin_table[] = {0, 1000000, 707106, 382683, 195090, 98017, 49067, 24541, 12271, 6135};

char *sprintf_int128(__int128_t n) {
    static char str[41] = { 0 };        // sign + log10(2**128) + '\0'
    char *s = str + sizeof( str ) - 1;  // start at the end
    bool neg = n < 0;
    if( neg )
        n = -n;
    do {
        *--s = "0123456789"[n % 10];    // save last digit
        n /= 10;                // drop it
    } while ( n );
    if( neg )
        *--s = '-';
    return s;
}

void fsmbFft(float *fftBuffer, long fftFrameSize, long sign) {
/*
    FFT routine, (C)1996 S.M.Bernsee. Sign = -1 is FFT, 1 is iFFT (inverse)

    Fills fftBuffer[0...2*fftFrameSize-1] with the Fourier transform of the time 
    domain data in fftBuffer[0...2*fftFrameSize-1]. The FFT array takes and returns
    the cosine and sine parts in an interleaved manner, ie. 
    fftBuffer[0] = cosPart[0], fftBuffer[1] = sinPart[0], asf. fftFrameSize must 
    be a power of 2. It expects a complex input signal (see footnote 2), ie. when 
    working with 'common' audio signals our input signal has to be passed as 
    {in[0],0.,in[1],0.,in[2],0.,...} asf. In that case, the transform of the 
    frequencies of interest is in fftBuffer[0...fftFrameSize].
*/
    float wr, wi, arg, *p1, *p2, temp;
    float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
    long i, bitm, j, le, le2, k, logN;
    logN = (long)(log(fftFrameSize)/log(2.)+.5);
	
    // for (i = 0; i < 50; i++)
    //     printf("%f\n", fftBuffer[i]);

    for (i = 2; i < 2*fftFrameSize-2; i += 2) {
        for (bitm = 2, j = 0; bitm < 2*fftFrameSize; bitm <<= 1) {
            if (i & bitm) j++;
            j <<= 1;
		}
		if (i < j) {
			p1 = fftBuffer+i; p2 = fftBuffer+j;
			temp = *p1; *(p1++) = *p2;
			*(p2++) = temp; temp = *p1;
			*p1 = *p2; *p2 = temp;
		}
	}

	for (k = 0, le = 2; k < logN; k++) {
		le <<= 1;
		le2 = le>>1;
		ur = 1.0;
		ui = 0.0;
		arg = M_PI / (le2>>1);
		wr = cos(arg);
		wi = sign*sin(arg);

		for (j = 0; j < le2; j += 2) {
			p1r = fftBuffer+j; p1i = p1r+1;
			p2r = p1r+le2; p2i = p2r+1;
			for (i = j; i < 2*fftFrameSize; i += le) {
				tr = *p2r * ur - *p2i * ui;
                // printf("%f\n", tr);
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr; *p2i = *p1i - ti;
				*p1r += tr; *p1i += ti;
				p1r += le; p1i += le;
				p2r += le; p2i += le;
			}
			tr = ur*wr - ui*wi;
			ui = ur*wi + ui*wr;
			ur = tr;
		}
	}
}

void smbFft(long long *fftBuffer, long fftFrameSize, long sign) {
/*
    FFT routine, (C)1996 S.M.Bernsee. Sign = -1 is FFT, 1 is iFFT (inverse)

    Fills fftBuffer[0...2*fftFrameSize-1] with the Fourier transform of the time 
    domain data in fftBuffer[0...2*fftFrameSize-1]. The FFT array takes and returns
    the cosine and sine parts in an interleaved manner, ie. 
    fftBuffer[0] = cosPart[0], fftBuffer[1] = sinPart[0], asf. fftFrameSize must 
    be a power of 2. It expects a complex input signal (see footnote 2), ie. when 
    working with 'common' audio signals our input signal has to be passed as 
    {in[0],0.,in[1],0.,in[2],0.,...} asf. In that case, the transform of the 
    frequencies of interest is in fftBuffer[0...fftFrameSize].
*/
    long long wr, wi, arg, temp;
    long long tr, ti, ur, ui;
    int p1r, p1i, p2r, p2i;
    long i, bitm, j, le, le2, k, logN;
    logN = (long)(log(fftFrameSize)/log(2.)+.5);
	
    // if (sign == -1) {
    //     for (i = 0; i < 50; i++)
    //         printf("%ld\n", fftBuffer[i]);
    // }

    for (i = 2; i < 2*fftFrameSize-2; i += 2) {
        for (bitm = 2, j = 0; bitm < 2*fftFrameSize; bitm <<= 1) {
            if (i & bitm) j++;
            j <<= 1;
		}
		if (i < j) {
            temp = fftBuffer[i];
            fftBuffer[i] = fftBuffer[j];
            fftBuffer[j] = temp;
            temp = fftBuffer[i + 1];
            fftBuffer[i + 1] = fftBuffer[j + 1];
            fftBuffer[j + 1] = temp;
		}
	}

	for (k = 0, le = 2; k < logN; k++) {
		le <<= 1;
		le2 = le>>1;
		ur = 1000000;
		ui = 0;
		arg = M_PI / (le2>>1);
		wr = cos_table[k];
		wi = sign * sin_table[k];

        for (j = 0; j < le2; j += 2) {
			p1r = j; p1i = p1r+1;
			p2r = p1r+le2; p2i = p2r+1;
			for (i = j; i < 2*fftFrameSize; i += le) {
				tr = (fftBuffer[p2r] * (ur / 100)) / 10000 - (fftBuffer[p2i] * (ui / 100)) / 10000;
                // if (sign == -1)
                //      printf("%ld\n", tr);
				ti = (fftBuffer[p2r] * (ui / 100)) / 10000 + (fftBuffer[p2i] * (ur / 100)) / 10000;
				fftBuffer[p2r] = fftBuffer[p1r] - tr; fftBuffer[p2i] = fftBuffer[p1i] - ti;
				fftBuffer[p1r] += tr; fftBuffer[p1i] += ti;
				p1r += le; p1i += le;
				p2r += le; p2i += le;
			}
			tr = (ur * wr) / 1000000 - (ui * wi) / 1000000;
			ui = (ur * wi) / 1000000 + (ui * wr) / 1000000;
			ur = tr;
		}
	}
}

int main(int argc, char **argv) {
    FILE *fin = fopen(argv[1], "r");
    FILE *fout = fopen(argv[2], "w");

    long long in_buffer[windowSize];
    float in_buffer_f[windowSize];
    long long out_buffer[2 * windowSize];
    float out_buffer_f[2 * windowSize];
    long long FFT_buffer[2 * windowSize];
    float FFT_buffer_f[2 * windowSize];
    int ac = 0;
    int cnt_bits = 0;
    int sample = 0;

    int in_buffer_ptr = windowSize - hopSize;
    int in_buffer_ptr_f = windowSize - hopSize;

    int windowSize2 = windowSize / 2;
    int osamp = windowSize / hopSize;

    memset(in_buffer, 0, windowSize * sizeof(long long));
    memset(in_buffer_f, 0, windowSize * sizeof(float));
    memset(out_buffer, 0, 2 * windowSize * sizeof(long long));
    memset(out_buffer_f, 0, 2 * windowSize * sizeof(float));

    for (int i = 0; i < 5000000; i++) {
        char c = fgetc(fin);

        cnt_bits += 8;
        if (cnt_bits == 8) {
            sample = (int)c;
        } else {
            sample += (((int)c) << 8);
            in_buffer[in_buffer_ptr++] = sample;
            in_buffer_f[in_buffer_ptr_f++] = sample;
            // printf("%ld %f\n", in_buffer[in_buffer_ptr], in_buffer_f[in_buffer_ptr_f]);
            ac++;

            cnt_bits = 0;
        }

        if (ac == hopSize) {
            // printf("-----------------WINDOW-----------------\n");
            for (int j = 0; j < windowSize; j++) {
                float window = -0.5 * cos(2.0 * M_PI * (float)j / (float)windowSize) + 0.5;
                FFT_buffer_f[2 * j] = window * in_buffer_f[j];
                FFT_buffer_f[2 * j + 1] = 0;

                FFT_buffer[2 * j] = window_multi[j] * in_buffer[j];
                FFT_buffer[2 * j + 1] = 0;

                // printf("%d %f\n", window_multi[j], window);

                //printf("%s\n", sprintf_int128(FFT_buffer[2 * j]));

                // printf("%ld\n", FFT_buffer[2 * j]);
            }

            // printf("------------------VALS------------------\n");
            smbFft(FFT_buffer, windowSize, -1);
            // printf("------------------------------------------\n");
            fsmbFft(FFT_buffer_f, windowSize, -1);

            smbFft(FFT_buffer, windowSize, 1);
            fsmbFft(FFT_buffer_f, windowSize, 1);

            // printf("-----------------VALUES-----------------\n");
            // for (int j = 0; j < windowSize; j++) {
            //     printf("%ld %f\n", FFT_buffer[2 * j], FFT_buffer_f[2 * j]);
            // }

            for (int j = 0; j < windowSize; j++) {
                long long to_add = 2 * (window_multi[j] / 100) * FFT_buffer[2 * j] / (windowSize2 * osamp);
                to_add /= 10000000000;
                out_buffer[j] += to_add;

                float window = -0.5 * cos(2.0 * M_PI * (float)j / (float)windowSize) + 0.5;
                out_buffer_f[j] += (2 * window * FFT_buffer_f[2 * j] / (windowSize2 * osamp));

                // printf("in: %ld %f\n", FFT_buffer[j], FFT_buffer_f[j]);
                // printf("out: %ld %f\n", out_buffer[j], out_buffer_f[j]);
            }
            // printf("-------------------------------------------");

            for (int j = 0; j < hopSize; j++) {
                long long transformed_sample = out_buffer[j];

                fprintf(fout, "%c", (char)(transformed_sample & 255));
                fprintf(fout, "%c", (char)(transformed_sample >> 8));
            }

            ac = 0;
            in_buffer_ptr = windowSize - hopSize;
            in_buffer_ptr_f = windowSize - hopSize;
            for (int j = 0; j < windowSize - hopSize; j++) {
                in_buffer[j] = in_buffer[j + hopSize];
                in_buffer_f[j] = in_buffer_f[j + hopSize];
            }
            memmove(out_buffer, out_buffer + hopSize, windowSize * sizeof(long long));
            memmove(out_buffer_f, out_buffer_f + hopSize, windowSize * sizeof(float));
        }
    }    

    return 0;
}
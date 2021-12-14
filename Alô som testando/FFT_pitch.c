#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define M_PI 3.14159265358979323846

const int windowSize = 1024;
const int hopSize = 256;

void smbFft(float *fftBuffer, long fftFrameSize, long sign) {
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
    float wr, wi, arg, temp;
    float tr, ti, ur, ui;
    int p1r, p1i, p2r, p2i;
    long i, bitm, j, le, le2, k, logN;
    logN = (long)(log(fftFrameSize)/log(2.)+.5);
	
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
		ur = 1.0;
		ui = 0.0;
		arg = M_PI / (le2>>1);
		wr = cos(arg);
		wi = sign*sin(arg);

        for (j = 0; j < le2; j += 2) {
			p1r = j; p1i = p1r+1;
			p2r = p1r+le2; p2i = p2r+1;
			for (i = j; i < 2*fftFrameSize; i += le) {
				tr = fftBuffer[p2r] * ur - fftBuffer[p2i] * ui;
				ti = fftBuffer[p2r] * ui + fftBuffer[p2i] * ur;
				fftBuffer[p2r] = fftBuffer[p1r] - tr; fftBuffer[p2i] = fftBuffer[p1i] - ti;
				fftBuffer[p1r] += tr; fftBuffer[p1i] += ti;
				p1r += le; p1i += le;
				p2r += le; p2i += le;
			}
			tr = ur*wr - ui*wi;
			ui = ur*wi + ui*wr;
			ur = tr;
		}
	}
}

long long arctan2(long long y, long long x) {
   long long coeff_1 = 785398;
   long long coeff_2 = 3 * coeff_1;
   long long abs_y = abs(y) + 1;
   long long angle = 0;
   if (x >= 0) {
      long long r = ((x - abs_y) * 1000000) / (x + abs_y);

      angle = (196300 * ((((r * r) / 1000000) * r) / 1000000)) / 1000000 - (981700 * r) / 1000000 + coeff_1;
        //angle = coeff_1 - (coeff_1 * r) / 1000000;
   } else {
      long long r = ((x + abs_y) * 1000000) / (abs_y - x);
      angle = (196300 * ((((r * r) / 1000000) * r) / 1000000)) / 1000000 - (981700 * r) / 1000000 + coeff_2;
       //angle = coeff_2 - (coeff_1 * r) / 1000000;
   }
   if (y < 0)
    return(-angle);     // negate if in quad III or IV
   else
    return(angle);
}

int main(int argc, char **argv) {
    FILE *fin = fopen(argv[1], "r");
    FILE *fout = fopen(argv[2], "w");

    float in_buffer[windowSize];
    float out_buffer[2 * windowSize];
    float FFT_buffer[2 * windowSize];
    float lastPhase[windowSize];
    float anaMagn[windowSize];
    float anaFreq[windowSize];
    float synMagn[windowSize];
    float synFreq[windowSize];
    float sumPhase[windowSize / 2 + 1];
    int ac = 0;
    int cnt_bits = 0;
    int sample = 0;

    int in_buffer_ptr = windowSize - hopSize;

    int windowSize2 = windowSize / 2;
    int osamp = windowSize / hopSize;

    float sampleRate = 44100.0;

    float freqPerBin = sampleRate / (double)windowSize;
    float expct = 2.0 * M_PI * (double)hopSize / (double)windowSize;

    memset(in_buffer, 0, windowSize * sizeof(float));
    memset(out_buffer, 0, 2 * windowSize * sizeof(float));
    memset(FFT_buffer, 0, 2 * windowSize * sizeof(float));
    memset(lastPhase, 0, windowSize * sizeof(float));
    memset(anaMagn, 0, windowSize * sizeof(float));
    memset(anaFreq, 0, windowSize * sizeof(float));
    memset(synMagn, 0, windowSize * sizeof(float));
    memset(synFreq, 0, windowSize * sizeof(float));
    memset(sumPhase, 0, (windowSize / 2 + 1) * sizeof(float));

    for (int i = 0; i < 5000000; i++) {
        char c = fgetc(fin);

        cnt_bits += 8;
        if (cnt_bits == 8) {
            sample = (int)c;
        } else {
            sample += (((int)c) << 8);
            in_buffer[in_buffer_ptr++] = sample;
            ac++;

            cnt_bits = 0;
        }

        if (ac == hopSize) {
            for (int j = 0; j < windowSize; j++) {
                float window = -0.5 * cos(2.0 * M_PI * (float)j / (float)windowSize) + 0.5;
                FFT_buffer[2 * j] = window * in_buffer[j];
                FFT_buffer[2 * j + 1] = 0;
            }
            smbFft(FFT_buffer, windowSize, -1);

            for (int j = 0; j < windowSize2; j++) {
                float real = FFT_buffer[2 * j];
                float imag = FFT_buffer[2 * j + 1];

                float magn = 2.0 * sqrt(real * real + imag * imag);
                float phase = atan2(imag, real);
                printf("%f\n", real * real + imag * imag);

                float tmp = phase - lastPhase[j];
                lastPhase[j] = phase;

                tmp -= (double)j * expct;

                int qpd = tmp / M_PI;
                if (qpd >= 0)
                    qpd += qpd & 1;
                else
                    qpd -= qpd & 1;
                
                tmp -= M_PI * (double)qpd;

                tmp = osamp * tmp / (2.0 * M_PI);

                tmp = (double)j * freqPerBin + tmp * freqPerBin;

                anaMagn[j] = magn;
                anaFreq[j] = tmp;
            }

            memset(synMagn, 0, windowSize * sizeof(float));
			memset(synFreq, 0, windowSize * sizeof(float));
			for (int j = 0; j <= windowSize2; j++) { 
				int index = j * 0.7;
				if (index <= windowSize2) { 
					synMagn[index] += anaMagn[j]; 
					synFreq[index] = anaFreq[j] * 0.7; 
				} 
			}

            /* this is the synthesis step */
			for (int j = 0; j <= windowSize2; j++) {

				/* get magnitude and true frequency from synthesis arrays */
				float magn = synMagn[j];
				float tmp = synFreq[j];

				/* subtract bin mid frequency */
				tmp -= (double)j * freqPerBin;

				/* get bin deviation from freq deviation */
				tmp /= freqPerBin;

				/* take osamp into account */
				tmp = 2.0 * M_PI * tmp / osamp;

				/* add the overlap phase advance back in */
				tmp += (double)j * expct;

				/* accumulate delta phase to get bin phase */
				sumPhase[j] += tmp;
				float phase = sumPhase[j];

				/* get real and imag part and re-interleave */
				FFT_buffer[2 * j] = magn * cos(phase);
				FFT_buffer[2 * j + 1] = magn * sin(phase);
			} 

			/* zero negative frequencies */
			for (int j = windowSize + 2; j < 2 * windowSize; j++) 
                FFT_buffer[j] = 0;

            smbFft(FFT_buffer, windowSize, 1);

            for (int j = 0; j < windowSize; j++) {
                float window = -0.5 * cos(2.0 * M_PI * (float)j / (float)windowSize) + 0.5;
                out_buffer[j] += 2.0 * window * FFT_buffer[2 * j] / (windowSize2 * osamp);
            }

            for (int j = 0; j < hopSize; j++) {
                int transformed_sample = out_buffer[j];

                fprintf(fout, "%c", (char)(transformed_sample & 255));
                fprintf(fout, "%c", (char)(transformed_sample >> 8));
            }

            ac = 0;
            in_buffer_ptr = windowSize - hopSize;
            for (int j = 0; j < windowSize - hopSize; j++) {
                in_buffer[j] = in_buffer[j + hopSize];
            }
            memmove(out_buffer, out_buffer + hopSize, windowSize * sizeof(float));
        }
    }    

    return 0;
}
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define windowSize 64

int x[windowSize];
float Xr[windowSize];
float Xi[windowSize];

void DFT(char *input_buffer) {
    for (int i = 0; i < windowSize; i++) {
        int sample = (((int)input_buffer[2 * i + 1]) << 8) + ((int)input_buffer[2 * i]);
        x[i] = sample;

        if (x[i] > 8191)
            x[i] = 8191;
        if (x[i] < -8192)
            x[i] = -8192;

        //printf("%d\n", x[i]);
    }

    const int N = windowSize;

    for (int k = 0; k < 12; k++) {
        Xr[k] = 0;
        Xi[k] = 0;
        for (int n = 0; n < windowSize; n++) {
            Xr[k] += x[n] * cos(2 * 3.141592 * k * n / N);
            Xi[k] += x[n] * sin(2 * 3.141592 * k * n / N);
        }
    }
}

void iDFT(char *output_buffer) {
    const int N = windowSize;

    int count = 500;
    for (int n = 0; n < N; n++) {
        x[n] = 0;
        for (int k = 0; k < 12; k++) {
            int theta = (2 * 3.141592 * k * n) / N;
            x[n] += Xr[k] * cos(theta) + Xi[k] * sin(theta);
            printf("%d ", x[n]);
        }
        x[n] /= N;
        if (count > 0) {
            printf("Xi: %f, x: %d\n", Xi[0], x[n]);
        }
        count--;
    }

    for (int i = 0; i < N; i++) {
        output_buffer[2 * i] = (char)(x[i] & 255);
        output_buffer[2 * i + 1] = (char)(x[i] >> 8);
    }
}

int main(int argc, char **argv) {
    FILE *fin = fopen(argv[1], "r");
    FILE *fout = fopen(argv[2], "w");

    char input_buffer[128];
    char output_buffer[128];
    int ac = 0;
    int sample = 0;
    bool flag = false;
    for (int i = 0; i < 5000000; i++) {
        char c = fgetc(fin);

        input_buffer[ac++] = c;
        if (ac == windowSize * 2) {
            DFT(input_buffer);
            iDFT(output_buffer);
            for (int j = 0; j < 2 * windowSize; j++) {
                fprintf(fout, "%c", output_buffer[j]);
                //printf("%d %d\n", (int)input_buffer[j], (int)output_buffer[j]);
            }
            memset(input_buffer, 0, 128 * sizeof(char));
            ac = 0;
        }
    }    

    return 0;
}
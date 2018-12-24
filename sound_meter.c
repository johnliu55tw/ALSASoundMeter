/*
MIT License

Copyright (c) [2016] [Hsin-Wu "John" Liu]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include<alsa/asoundlib.h>
#include<math.h>

/*
   Device name.
   This might be different on your computer, please run
   'arecord -L' to verify valide device names.
   (Thanks to @Prot_EuPhobox on youtube!)
*/
static char *device = "default";

/*
   Each sample is a 16-bit signed integer, which means
   the value can vary from -32768 to 32767.

   Size of the sample buffer = 8*1024 = 8192, which takes
   8*1024*2 = 16384 bytes.

   Sampling frequency = 48000Hz, in other words, sound
   card will record the sample value every 1/48000 second.
*/
short buffer[8*1024];

/*
   The unit of sizeof() function is byte, so we have to
   shift 1 bit to the right to calculate the real size of buffer.
*/
int buffer_size = sizeof(buffer)>>1;

/*
   Function for calculating the Root Mean Square of sample buffer.
   RMS can calculate an average amplitude of buffer.
*/
double rms(short *buffer)
{
    int i;
    long int square_sum = 0.0;
    for(i=0; i<buffer_size; i++)
        square_sum += (buffer[i] * buffer[i]);

    double result = sqrt(square_sum/buffer_size);
    return result;
}

/*
   Show the sound level and its peak value
*/
void show(int Pvalue, int peak)
{
    int j;
    int dB;
    int dBpeak;

    for(j=0; j<13; j++)
        printf("\b");
    fflush(stdout);

    if(Pvalue > 0)
    {
        dB = (int)20 * log10(Pvalue);
        printf("dB=%d,", dB);
    }
    else
        printf("dB=--,");

    if(peak > 0)
    {
        dBpeak = (int)20 * log10(peak);
        printf("Peak=%d", dBpeak);
    }
    else
        printf("Peak=--", dB, peak);

    fflush(stdout);
}

int main(void)
{
    snd_pcm_t *handle_capture;
    snd_pcm_sframes_t frames;
    
    int err;

    err = snd_pcm_open(&handle_capture, device, SND_PCM_STREAM_CAPTURE, 0);
    if(err < 0) {
        printf("Capture open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    err = snd_pcm_set_params(handle_capture,
                             SND_PCM_FORMAT_S16_LE,
                             SND_PCM_ACCESS_RW_INTERLEAVED,
                             1,
                             48000,
                             1,
                             500000);
    if(err < 0) {
        printf("Capture open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    /*
       Formula of dB is 20log((Sound Pressure)/P0)
       Assume that (Sound Pressure/P0) = k * sample value (Linear!),
       and by experiment, we found that k = 0.45255.
    */
    double k = 0.45255;
    double Pvalue = 0;
    double peak = 0;

    // Capture
    while(1) {
        
        frames = snd_pcm_readi(handle_capture, buffer, buffer_size);

        if(frames < 0) {
            // Try to recover
            frames = snd_pcm_recover(handle_capture, frames, 0);
            if(frames < 0) {
                printf("snd_pcm_readi failed: %s\n", snd_strerror(err));
            }
        }

        if(frames > 0 && frames < (long)buffer_size) {
            printf("Short read (expected %li, read %li)\n", (long)buffer_size, frames);
        }
        
        // Successfully read, calculate dB and update peak value
        Pvalue = rms(buffer) * k;
        if(Pvalue > peak)
            peak = Pvalue;
        
        show(Pvalue, peak);
    }
    printf("\n");
    snd_pcm_close(handle_capture);
    return 0;

}

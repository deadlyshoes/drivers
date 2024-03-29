// SPDX-License-Identifier: GPL-2.0-or-later
/*
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/bitrev.h>
#include <linux/ratelimit.h>
#include <linux/usb.h>
#include <linux/usb/audio.h>
#include <linux/usb/audio-v2.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "usbaudio.h"
#include "card.h"
#include "quirks.h"
#include "endpoint.h"
#include "helper.h"
#include "pcm.h"
#include "clock.h"
#include "power.h"
#include "media.h"
#include "implicit.h"

#define SUBSTREAM_FLAG_DATA_EP_STARTED	0
#define SUBSTREAM_FLAG_SYNC_EP_STARTED	1

#define MAX_WINDOW_SIZE 64

long long cos_alien[64] = {1000000, 997463, 989867, 977250, 959675, 937232, 910035, 878221, 841953,
801413, 756808, 708364, 656327, 600961, 542546, 481379, 417770, 352041,
284527, 215570, 145519, 74730, 3561, -67624, -138467, -208608, -277691,
-345365, -411287, -475122, -536548, -595252, -650936, -703318, -752133, -797132,
-838088, -874792, -907059, -934724, -957648, -975714, -988830, -996931, -999974,
-997945, -990853, -978736, -961653, -939692, -912965, -881606, -845775, -805654,
-761445, -713375, -661685, -606639, -548516, -487610, -424231, -358700, -291349,
-222521 };

const long long cos_test[12][64] = {{1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,
1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,
1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,
1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,
1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,
1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,
1000000,1000000,1000000,1000000},
{1000000,995184,980785,956940,923879,881921,831469,773010,707106,634393,
555570,471396,382683,290284,195090,98017,0,-98016,-195089,-290284,
-382683,-471396,-555569,-634392,-707106,-773010,-831469,-881921,-923879,-956940,
-980785,-995184,-999999,-995184,-980785,-956940,-923879,-881921,-831470,-773010,
-707107,-634393,-555570,-471397,-382684,-290285,-195091,-98018,0,98016,
195089,290283,382682,471395,555569,634392,707105,773009,831468,881920,
923879,956939,980785,995184},
{1000000,980785,923879,831469,707106,555570,382683,195090,0,-195089,
-382683,-555569,-707106,-831469,-923879,-980785,-999999,-980785,-923879,-831470,
-707107,-555570,-382684,-195091,0,195089,382682,555569,707105,831468,
923879,980785,999999,980785,923880,831470,707107,555571,382684,195091,
1,-195088,-382681,-555568,-707105,-831468,-923878,-980784,-999999,-980785,
-923880,-831470,-707108,-555572,-382685,-195092,-2,195088,382681,555568,
707105,831468,923878,980784},
{1000000,956940,831469,634393,382683,98017,-195089,-471396,-707106,-881921,
-980785,-995184,-923879,-773010,-555570,-290285,0,290283,555569,773009,
923879,995184,980785,881921,707107,471398,195091,-98015,-382681,-634391,
-831468,-956939,-999999,-956940,-831470,-634394,-382685,-98019,195088,471394,
707105,881920,980784,995184,923880,773012,555572,290287,2,-290281,
-555567,-773008,-923878,-995184,-980785,-881922,-707109,-471399,-195093,98013,
382680,634390,831467,956939},
{1000000,923879,707106,382683,0,-382683,-707106,-923879,-999999,-923879,
-707107,-382684,0,382682,707105,923879,999999,923880,707107,382684,
1,-382681,-707105,-923878,-999999,-923880,-707108,-382685,-2,382681,
707105,923878,999999,923880,707108,382686,2,-382680,-707104,-923878,
-999999,-923880,-707109,-382686,-3,382680,707104,923878,999999,923881,
707109,382687,4,-382679,-707103,-923877,-999999,-923881,-707110,-382687,
-4,382678,707103,923877},
{1000000,881921,555570,98017,-382683,-773010,-980785,-956940,-707107,-290285,
195089,634392,923879,995184,831470,471398,1,-471395,-831468,-995184,
-923880,-634394,-195092,290282,707105,956939,980785,773012,382686,-98014,
-555567,-881919,-999999,-881922,-555573,-98020,382680,773008,980784,956941,
707109,290288,-195086,-634389,-923877,-995185,-831472,-471400,-4,471392,
831466,995184,923881,634397,195095,-290279,-707102,-956938,-980786,-773014,
-382689,98010,555564,881918},
{1000000,831469,382683,-195089,-707106,-980785,-923879,-555570,0,555569,
923879,980785,707107,195091,-382681,-831468,-999999,-831470,-382685,195088,
707105,980784,923880,555572,2,-555567,-923878,-980785,-707109,-195093,
382680,831467,999999,831471,382687,-195086,-707103,-980784,-923881,-555574,
-4,555566,923877,980786,707110,195095,-382678,-831466,-999999,-831472,
-382689,195084,707102,980784,923882,555575,6,-555564,-923876,-980786,
-707111,-195097,382676,831465},
{1000000,773010,195090,-471396,-923879,-956940,-555570,98016,707105,995184,
831470,290286,-382681,-881920,-980785,-634394,-2,634391,980784,881922,
382686,-290281,-831467,-995185,-707109,-98020,555567,956939,923881,471400,
-195086,-773007,-999999,-773013,-195095,471392,923877,956941,555574,-98011,
-707102,-995184,-831472,-290290,382677,881918,980786,634398,6,-634387,
-980783,-881924,-382690,290277,831465,995185,707112,98025,-555563,-956937,
-923882,-471404,195081,773004},
{1000000,707106,0,-707106,-999999,-707107,0,707105,999999,707107,
1,-707105,-999999,-707108,-2,707105,999999,707108,2,-707104,
-999999,-707109,-3,707104,999999,707109,4,-707103,-999999,-707110,
-4,707103,999999,707110,5,-707102,-999999,-707111,-6,707102,
999999,707111,6,-707101,-999999,-707111,-7,707101,999999,707112,
8,-707100,-999999,-707112,-8,707100,999999,707113,9,-707099,
-999999,-707113,-10,707099},
{1000000,634393,-195089,-881921,-923879,-290285,555569,995184,707107,-98015,
-831468,-956940,-382685,471394,980784,773012,2,-773008,-980785,-471399,
382680,956939,831471,98021,-707103,-995185,-555574,290279,923877,881923,
195095,-634388,-999999,-634397,195084,881918,923882,290291,-555564,-995184,
-707111,98009,831465,956942,382690,-471389,-980783,-773015,-8,773004,
980787,471405,-382674,-956937,-831475,-98027,707099,995185,555579,-290274,
-923875,-881926,-195101,634384},
{1000000,555570,-382683,-980785,-707107,195089,923879,831470,1,-831468,
-923880,-195092,707105,980785,382686,-555567,-999999,-555573,382680,980784,
707109,-195086,-923877,-831472,-4,831466,923881,195095,-707102,-980786,
-382689,555564,999999,555575,-382677,-980783,-707111,195082,923876,831474,
8,-831464,-923882,-195098,707100,980787,382692,-555562,-999999,-555578,
382673,980783,707114,-195079,-923875,-831475,-11,831463,923884,195102,
-707098,-980787,-382695,555559},
{1000000,471396,-555569,-995184,-382684,634392,980785,290286,-707105,-956940,
-195092,773008,923880,98020,-831467,-881922,-3,881919,831471,-98012,
-923877,-773013,195085,956938,707110,-290279,-980784,-634397,382677,995184,
555575,-471390,-999999,-471403,555563,995185,382690,-634386,-980786,-290293,
707100,956943,195099,-773004,-923883,-98027,831463,881926,10,-881916,
-831475,98005,923875,773018,-195078,-956936,-707115,290272,980782,634403,
-382670,-995183,-555581,471384}
};

const long long sin_test[12][64] = {{0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0},
{0,98017,195090,290284,382683,471396,555570,634393,707106,773010,
831469,881921,923879,956940,980785,995184,999999,995184,980785,956940,
923879,881921,831469,773010,707107,634393,555570,471397,382683,290285,
195090,98017,0,-98016,-195089,-290283,-382682,-471396,-555569,-634392,
-707106,-773009,-831469,-881920,-923879,-956940,-980785,-995184,-999999,-995184,
-980785,-956940,-923879,-881921,-831470,-773011,-707107,-634394,-555571,-471397,
-382684,-290285,-195091,-98018},
{0,195090,382683,555570,707106,831469,923879,980785,999999,980785,
923879,831469,707107,555570,382683,195090,0,-195089,-382682,-555569,
-707106,-831469,-923879,-980785,-999999,-980785,-923879,-831470,-707107,-555571,
-382684,-195091,-1,195088,382682,555569,707105,831468,923878,980784,
999999,980785,923880,831470,707108,555571,382685,195092,1,-195088,
-382681,-555568,-707105,-831468,-923878,-980784,-999999,-980785,-923880,-831470,
-707108,-555572,-382685,-195092},
{0,290284,555570,773010,923879,995184,980785,881921,707107,471397,
195090,-98016,-382682,-634392,-831469,-956940,-999999,-956940,-831470,-634394,
-382684,-98018,195088,471395,707105,881920,980784,995184,923880,773011,
555571,290286,1,-290282,-555568,-773009,-923878,-995184,-980785,-881922,
-707108,-471398,-195092,98014,382680,634391,831468,956939,999999,956941,
831471,634395,382686,98020,-195087,-471393,-707104,-881919,-980784,-995185,
-923880,-773012,-555573,-290288},
{0,382683,707106,923879,999999,923879,707107,382683,0,-382682,
-707106,-923879,-999999,-923879,-707107,-382684,-1,382682,707105,923878,
999999,923880,707108,382685,1,-382681,-707105,-923878,-999999,-923880,
-707108,-382685,-2,382680,707104,923878,999999,923880,707108,382686,
3,-382680,-707104,-923878,-999999,-923880,-707109,-382686,-3,382679,
707103,923877,999999,923881,707109,382687,4,-382679,-707103,-923877,
-999999,-923881,-707110,-382688},
{0,471396,831469,995184,923879,634393,195090,-290283,-707106,-956940,
-980785,-773011,-382684,98015,555569,881920,999999,881922,555571,98019,
-382681,-773009,-980784,-956941,-707108,-290287,195087,634391,923878,995185,
831471,471399,3,-471393,-831467,-995184,-923880,-634396,-195094,290280,
707103,956939,980786,773013,382687,-98012,-555566,-881919,-999999,-881923,
-555574,-98022,382678,773007,980784,956941,707110,290290,-195084,-634388,
-923877,-995185,-831473,-471402},
{0,555570,923879,980785,707107,195090,-382682,-831469,-999999,-831470,
-382684,195088,707105,980784,923880,555571,1,-555568,-923878,-980785,
-707108,-195092,382680,831468,999999,831471,382686,-195087,-707104,-980784,
-923880,-555573,-3,555566,923877,980786,707109,195094,-382679,-831466,
-999999,-831472,-382688,195085,707102,980784,923881,555575,5,-555565,
-923877,-980786,-707111,-195096,382677,831465,999999,831473,382689,-195083,
-707101,-980783,-923882,-555576},
{0,634393,980785,881921,382683,-290283,-831469,-995184,-707107,-98018,
555569,956939,923880,471398,-195088,-773009,-999999,-773011,-195092,471394,
923878,956941,555572,-98013,-707104,-995184,-831471,-290288,382679,881919,
980786,634396,4,-634389,-980784,-881923,-382688,290279,831466,995185,
707110,98022,-555565,-956938,-923881,-471402,195083,773006,999999,773014,
195097,-471390,-923876,-956942,-555576,98009,707101,995183,831474,290292,
-382675,-881917,-980787,-634400},
{0,707106,999999,707107,0,-707106,-999999,-707107,-1,707105,
999999,707108,1,-707105,-999999,-707108,-2,707104,999999,707108,
3,-707104,-999999,-707109,-3,707103,999999,707109,4,-707103,
-999999,-707110,-5,707102,999999,707110,5,-707102,-999999,-707111,
-6,707102,999999,707111,7,-707101,-999999,-707112,-7,707101,
999999,707112,8,-707100,-999999,-707113,-9,707100,999999,707113,
9,-707099,-999999,-707114},
{0,773010,980785,471397,-382682,-956940,-831470,-98018,707105,995184,
555571,-290282,-923878,-881922,-195092,634391,999999,634395,-195087,-881919,
-923880,-290288,555566,995184,707109,-98012,-831466,-956941,-382688,471392,
980784,773014,5,-773006,-980786,-471402,382677,956938,831473,98024,
-707101,-995185,-555576,290277,923876,881925,195098,-634386,-999999,-634400,
195081,881916,923883,290294,-555561,-995183,-707114,98006,831463,956943,
382693,-471386,-980783,-773017},
{0,831469,923879,195090,-707106,-980785,-382684,555569,999999,555571,
-382681,-980784,-707108,195087,923878,831471,3,-831467,-923880,-195094,
707103,980786,382687,-555566,-999999,-555574,382678,980784,707110,-195084,
-923877,-831473,-6,831465,923882,195097,-707101,-980786,-382690,555563,
999999,555577,-382675,-980783,-707113,195081,923875,831474,9,-831464,
-923883,-195100,707099,980787,382693,-555560,-999999,-555579,382672,980782,
707115,-195078,-923874,-831476},
{0,881921,831469,-98016,-923879,-773011,195088,956939,707108,-290282,
-980784,-634395,382680,995184,555572,-471393,-999999,-471400,555566,995185,
382687,-634389,-980786,-290289,707102,956941,195096,-773006,-923881,-98023,
831465,881924,7,-881917,-831473,98009,923876,773015,-195081,-956937,
-707113,290275,980783,634400,-382674,-995183,-555578,471387,999999,471406,
-555560,-995185,-382694,634384,980787,290296,-707097,-956944,-195103,773002,
923884,98030,-831461,-881927}
};

long long x[MAX_WINDOW_SIZE];
long long Xr[MAX_WINDOW_SIZE];
long long Xi[MAX_WINDOW_SIZE];

void DFT(char *input_buffer, int windowSize) {
	int N, n, k, i, sample;

    for (i = 0; i < windowSize; i++) {
        sample = (((int)input_buffer[2 * i + 1]) << 8) + ((int)input_buffer[2 * i]);
        x[i] = sample;

        if (x[i] > 8191)
            x[i] = 8191;
        if (x[i] < -8192)
            x[i] = -8192;
    }

    N = windowSize;

    for (k = 0; k < 12; k++) {
        Xr[k] = 0;
        Xi[k] = 0;
        for (n = 0; n < windowSize; n++) {
            Xr[k] += x[n] * cos_test[k][n];
            Xi[k] += x[n] * sin_test[k][n];
        }
    }
}

void iDFT(char *output_buffer, int windowSize) {
	int i, n, k;
    const int N = windowSize;

    for (n = 0; n < N; n++) {
        x[n] = 0;
        for (k = 0; k < 12; k++) {
            if (cos_test[k][n] > 500000)
                x[n] += (Xr[k] / 1000000);
            if (sin_test[k][n] > 500000)
                x[n] += (Xi[k] / 1000000);
        }
        x[n] /= N;
    }

    for (i = 0; i < N; i++) {
        output_buffer[2 * i] += (char)(x[i] & 255);
        output_buffer[2 * i + 1] += (char)(x[i] >> 8);
    }
}

/* return the estimated delay based on USB frame counters */
static snd_pcm_uframes_t snd_usb_pcm_delay(struct snd_usb_substream *subs,
					   struct snd_pcm_runtime *runtime)
{
	unsigned int current_frame_number;
	unsigned int frame_diff;
	int est_delay;
	int queued;

	if (subs->direction == SNDRV_PCM_STREAM_PLAYBACK) {
		queued = bytes_to_frames(runtime, subs->inflight_bytes);
		if (!queued)
			return 0;
	} else if (!subs->running) {
		return 0;
	}

	current_frame_number = usb_get_current_frame_number(subs->dev);
	/*
	 * HCD implementations use different widths, use lower 8 bits.
	 * The delay will be managed up to 256ms, which is more than
	 * enough
	 */
	frame_diff = (current_frame_number - subs->last_frame_number) & 0xff;

	/* Approximation based on number of samples per USB frame (ms),
	   some truncation for 44.1 but the estimate is good enough */
	est_delay = frame_diff * runtime->rate / 1000;

	if (subs->direction == SNDRV_PCM_STREAM_PLAYBACK) {
		est_delay = queued - est_delay;
		if (est_delay < 0)
			est_delay = 0;
	}

	return est_delay;
}

/*
 * return the current pcm pointer.  just based on the hwptr_done value.
 */
static snd_pcm_uframes_t snd_usb_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_usb_substream *subs = runtime->private_data;
	unsigned int hwptr_done;

	if (atomic_read(&subs->stream->chip->shutdown))
		return SNDRV_PCM_POS_XRUN;
	spin_lock(&subs->lock);
	hwptr_done = subs->hwptr_done;
	runtime->delay = snd_usb_pcm_delay(subs, runtime);
	spin_unlock(&subs->lock);
	return bytes_to_frames(runtime, hwptr_done);
}

/*
 * find a matching audio format
 */
static const struct audioformat *
find_format(struct list_head *fmt_list_head, snd_pcm_format_t format,
	    unsigned int rate, unsigned int channels, bool strict_match,
	    struct snd_usb_substream *subs)
{
	const struct audioformat *fp;
	const struct audioformat *found = NULL;
	int cur_attr = 0, attr;

	list_for_each_entry(fp, fmt_list_head, list) {
		if (strict_match) {
			if (!(fp->formats & pcm_format_to_bits(format)))
				continue;
			if (fp->channels != channels)
				continue;
		}
		if (rate < fp->rate_min || rate > fp->rate_max)
			continue;
		if (!(fp->rates & SNDRV_PCM_RATE_CONTINUOUS)) {
			unsigned int i;
			for (i = 0; i < fp->nr_rates; i++)
				if (fp->rate_table[i] == rate)
					break;
			if (i >= fp->nr_rates)
				continue;
		}
		attr = fp->ep_attr & USB_ENDPOINT_SYNCTYPE;
		if (!found) {
			found = fp;
			cur_attr = attr;
			continue;
		}
		/* avoid async out and adaptive in if the other method
		 * supports the same format.
		 * this is a workaround for the case like
		 * M-audio audiophile USB.
		 */
		if (subs && attr != cur_attr) {
			if ((attr == USB_ENDPOINT_SYNC_ASYNC &&
			     subs->direction == SNDRV_PCM_STREAM_PLAYBACK) ||
			    (attr == USB_ENDPOINT_SYNC_ADAPTIVE &&
			     subs->direction == SNDRV_PCM_STREAM_CAPTURE))
				continue;
			if ((cur_attr == USB_ENDPOINT_SYNC_ASYNC &&
			     subs->direction == SNDRV_PCM_STREAM_PLAYBACK) ||
			    (cur_attr == USB_ENDPOINT_SYNC_ADAPTIVE &&
			     subs->direction == SNDRV_PCM_STREAM_CAPTURE)) {
				found = fp;
				cur_attr = attr;
				continue;
			}
		}
		/* find the format with the largest max. packet size */
		if (fp->maxpacksize > found->maxpacksize) {
			found = fp;
			cur_attr = attr;
		}
	}
	return found;
}

static const struct audioformat *
find_substream_format(struct snd_usb_substream *subs,
		      const struct snd_pcm_hw_params *params)
{
	return find_format(&subs->fmt_list, params_format(params),
			   params_rate(params), params_channels(params),
			   true, subs);
}

static int init_pitch_v1(struct snd_usb_audio *chip, int ep)
{
	struct usb_device *dev = chip->dev;
	unsigned char data[1];
	int err;

	data[0] = 1;
	err = snd_usb_ctl_msg(dev, usb_sndctrlpipe(dev, 0), UAC_SET_CUR,
			      USB_TYPE_CLASS|USB_RECIP_ENDPOINT|USB_DIR_OUT,
			      UAC_EP_CS_ATTR_PITCH_CONTROL << 8, ep,
			      data, sizeof(data));
	return err;
}

static int init_pitch_v2(struct snd_usb_audio *chip, int ep)
{
	struct usb_device *dev = chip->dev;
	unsigned char data[1];
	int err;

	data[0] = 1;
	err = snd_usb_ctl_msg(dev, usb_sndctrlpipe(dev, 0), UAC2_CS_CUR,
			      USB_TYPE_CLASS | USB_RECIP_ENDPOINT | USB_DIR_OUT,
			      UAC2_EP_CS_PITCH << 8, 0,
			      data, sizeof(data));
	return err;
}

/*
 * initialize the pitch control and sample rate
 */
int snd_usb_init_pitch(struct snd_usb_audio *chip,
		       const struct audioformat *fmt)
{
	int err;

	/* if endpoint doesn't have pitch control, bail out */
	if (!(fmt->attributes & UAC_EP_CS_ATTR_PITCH_CONTROL))
		return 0;

	usb_audio_dbg(chip, "enable PITCH for EP 0x%x\n", fmt->endpoint);

	switch (fmt->protocol) {
	case UAC_VERSION_1:
		err = init_pitch_v1(chip, fmt->endpoint);
		break;
	case UAC_VERSION_2:
		err = init_pitch_v2(chip, fmt->endpoint);
		break;
	default:
		return 0;
	}

	if (err < 0) {
		usb_audio_err(chip, "failed to enable PITCH for EP 0x%x\n",
			      fmt->endpoint);
		return err;
	}

	return 0;
}

static bool stop_endpoints(struct snd_usb_substream *subs)
{
	bool stopped = 0;

	if (test_and_clear_bit(SUBSTREAM_FLAG_SYNC_EP_STARTED, &subs->flags)) {
		snd_usb_endpoint_stop(subs->sync_endpoint);
		stopped = true;
	}
	if (test_and_clear_bit(SUBSTREAM_FLAG_DATA_EP_STARTED, &subs->flags)) {
		snd_usb_endpoint_stop(subs->data_endpoint);
		stopped = true;
	}
	return stopped;
}

static int start_endpoints(struct snd_usb_substream *subs)
{
	int err;

	if (!subs->data_endpoint)
		return -EINVAL;

	if (!test_and_set_bit(SUBSTREAM_FLAG_DATA_EP_STARTED, &subs->flags)) {
		err = snd_usb_endpoint_start(subs->data_endpoint);
		if (err < 0) {
			clear_bit(SUBSTREAM_FLAG_DATA_EP_STARTED, &subs->flags);
			goto error;
		}
	}

	if (subs->sync_endpoint &&
	    !test_and_set_bit(SUBSTREAM_FLAG_SYNC_EP_STARTED, &subs->flags)) {
		err = snd_usb_endpoint_start(subs->sync_endpoint);
		if (err < 0) {
			clear_bit(SUBSTREAM_FLAG_SYNC_EP_STARTED, &subs->flags);
			goto error;
		}
	}

	return 0;

 error:
	stop_endpoints(subs);
	return err;
}

static void sync_pending_stops(struct snd_usb_substream *subs)
{
	snd_usb_endpoint_sync_pending_stop(subs->sync_endpoint);
	snd_usb_endpoint_sync_pending_stop(subs->data_endpoint);
}

/* PCM sync_stop callback */
static int snd_usb_pcm_sync_stop(struct snd_pcm_substream *substream)
{
	struct snd_usb_substream *subs = substream->runtime->private_data;

	sync_pending_stops(subs);
	return 0;
}

/* Set up sync endpoint */
int snd_usb_audioformat_set_sync_ep(struct snd_usb_audio *chip,
				    struct audioformat *fmt)
{
	struct usb_device *dev = chip->dev;
	struct usb_host_interface *alts;
	struct usb_interface_descriptor *altsd;
	unsigned int ep, attr, sync_attr;
	bool is_playback;
	int err;

	alts = snd_usb_get_host_interface(chip, fmt->iface, fmt->altsetting);
	if (!alts)
		return 0;
	altsd = get_iface_desc(alts);

	err = snd_usb_parse_implicit_fb_quirk(chip, fmt, alts);
	if (err > 0)
		return 0; /* matched */

	/*
	 * Generic sync EP handling
	 */

	if (altsd->bNumEndpoints < 2)
		return 0;

	is_playback = !(get_endpoint(alts, 0)->bEndpointAddress & USB_DIR_IN);
	attr = fmt->ep_attr & USB_ENDPOINT_SYNCTYPE;
	if ((is_playback && (attr == USB_ENDPOINT_SYNC_SYNC ||
			     attr == USB_ENDPOINT_SYNC_ADAPTIVE)) ||
	    (!is_playback && attr != USB_ENDPOINT_SYNC_ADAPTIVE))
		return 0;

	sync_attr = get_endpoint(alts, 1)->bmAttributes;

	/*
	 * In case of illegal SYNC_NONE for OUT endpoint, we keep going to see
	 * if we don't find a sync endpoint, as on M-Audio Transit. In case of
	 * error fall back to SYNC mode and don't create sync endpoint
	 */

	/* check sync-pipe endpoint */
	/* ... and check descriptor size before accessing bSynchAddress
	   because there is a version of the SB Audigy 2 NX firmware lacking
	   the audio fields in the endpoint descriptors */
	if ((sync_attr & USB_ENDPOINT_XFERTYPE_MASK) != USB_ENDPOINT_XFER_ISOC ||
	    (get_endpoint(alts, 1)->bLength >= USB_DT_ENDPOINT_AUDIO_SIZE &&
	     get_endpoint(alts, 1)->bSynchAddress != 0)) {
		dev_err(&dev->dev,
			"%d:%d : invalid sync pipe. bmAttributes %02x, bLength %d, bSynchAddress %02x\n",
			   fmt->iface, fmt->altsetting,
			   get_endpoint(alts, 1)->bmAttributes,
			   get_endpoint(alts, 1)->bLength,
			   get_endpoint(alts, 1)->bSynchAddress);
		if (is_playback && attr == USB_ENDPOINT_SYNC_NONE)
			return 0;
		return -EINVAL;
	}
	ep = get_endpoint(alts, 1)->bEndpointAddress;
	if (get_endpoint(alts, 0)->bLength >= USB_DT_ENDPOINT_AUDIO_SIZE &&
	    get_endpoint(alts, 0)->bSynchAddress != 0 &&
	    ((is_playback && ep != (unsigned int)(get_endpoint(alts, 0)->bSynchAddress | USB_DIR_IN)) ||
	     (!is_playback && ep != (unsigned int)(get_endpoint(alts, 0)->bSynchAddress & ~USB_DIR_IN)))) {
		dev_err(&dev->dev,
			"%d:%d : invalid sync pipe. is_playback %d, ep %02x, bSynchAddress %02x\n",
			   fmt->iface, fmt->altsetting,
			   is_playback, ep, get_endpoint(alts, 0)->bSynchAddress);
		if (is_playback && attr == USB_ENDPOINT_SYNC_NONE)
			return 0;
		return -EINVAL;
	}

	fmt->sync_ep = ep;
	fmt->sync_iface = altsd->bInterfaceNumber;
	fmt->sync_altsetting = altsd->bAlternateSetting;
	fmt->sync_ep_idx = 1;
	if ((sync_attr & USB_ENDPOINT_USAGE_MASK) == USB_ENDPOINT_USAGE_IMPLICIT_FB)
		fmt->implicit_fb = 1;

	dev_dbg(&dev->dev, "%d:%d: found sync_ep=0x%x, iface=%d, alt=%d, implicit_fb=%d\n",
		fmt->iface, fmt->altsetting, fmt->sync_ep, fmt->sync_iface,
		fmt->sync_altsetting, fmt->implicit_fb);

	return 0;
}

static int snd_usb_pcm_change_state(struct snd_usb_substream *subs, int state)
{
	int ret;

	if (!subs->str_pd)
		return 0;

	ret = snd_usb_power_domain_set(subs->stream->chip, subs->str_pd, state);
	if (ret < 0) {
		dev_err(&subs->dev->dev,
			"Cannot change Power Domain ID: %d to state: %d. Err: %d\n",
			subs->str_pd->pd_id, state, ret);
		return ret;
	}

	return 0;
}

int snd_usb_pcm_suspend(struct snd_usb_stream *as)
{
	int ret;

	ret = snd_usb_pcm_change_state(&as->substream[0], UAC3_PD_STATE_D2);
	if (ret < 0)
		return ret;

	ret = snd_usb_pcm_change_state(&as->substream[1], UAC3_PD_STATE_D2);
	if (ret < 0)
		return ret;

	return 0;
}

int snd_usb_pcm_resume(struct snd_usb_stream *as)
{
	int ret;

	ret = snd_usb_pcm_change_state(&as->substream[0], UAC3_PD_STATE_D1);
	if (ret < 0)
		return ret;

	ret = snd_usb_pcm_change_state(&as->substream[1], UAC3_PD_STATE_D1);
	if (ret < 0)
		return ret;

	return 0;
}

static void close_endpoints(struct snd_usb_audio *chip,
			    struct snd_usb_substream *subs)
{
	if (subs->data_endpoint) {
		snd_usb_endpoint_set_sync(chip, subs->data_endpoint, NULL);
		snd_usb_endpoint_close(chip, subs->data_endpoint);
		subs->data_endpoint = NULL;
	}

	if (subs->sync_endpoint) {
		snd_usb_endpoint_close(chip, subs->sync_endpoint);
		subs->sync_endpoint = NULL;
	}
}

static int configure_endpoints(struct snd_usb_audio *chip,
			       struct snd_usb_substream *subs)
{
	int err;

	if (subs->data_endpoint->need_setup) {
		/* stop any running stream beforehand */
		if (stop_endpoints(subs))
			sync_pending_stops(subs);
		err = snd_usb_endpoint_configure(chip, subs->data_endpoint);
		if (err < 0)
			return err;
		snd_usb_set_format_quirk(subs, subs->cur_audiofmt);
	}

	if (subs->sync_endpoint) {
		err = snd_usb_endpoint_configure(chip, subs->sync_endpoint);
		if (err < 0)
			return err;
	}

	return 0;
}

/*
 * hw_params callback
 *
 * allocate a buffer and set the given audio format.
 *
 * so far we use a physically linear buffer although packetize transfer
 * doesn't need a continuous area.
 * if sg buffer is supported on the later version of alsa, we'll follow
 * that.
 */
static int snd_usb_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *hw_params)
{
	struct snd_usb_substream *subs = substream->runtime->private_data;
	struct snd_usb_audio *chip = subs->stream->chip;
	const struct audioformat *fmt;
	const struct audioformat *sync_fmt;
	int ret;

	ret = snd_media_start_pipeline(subs);
	if (ret)
		return ret;

	fmt = find_substream_format(subs, hw_params);
	if (!fmt) {
		usb_audio_dbg(chip,
			      "cannot find format: format=%s, rate=%d, channels=%d\n",
			      snd_pcm_format_name(params_format(hw_params)),
			      params_rate(hw_params), params_channels(hw_params));
		ret = -EINVAL;
		goto stop_pipeline;
	}

	if (fmt->implicit_fb) {
		sync_fmt = snd_usb_find_implicit_fb_sync_format(chip, fmt,
								hw_params,
								!substream->stream);
		if (!sync_fmt) {
			usb_audio_dbg(chip,
				      "cannot find sync format: ep=0x%x, iface=%d:%d, format=%s, rate=%d, channels=%d\n",
				      fmt->sync_ep, fmt->sync_iface,
				      fmt->sync_altsetting,
				      snd_pcm_format_name(params_format(hw_params)),
				      params_rate(hw_params), params_channels(hw_params));
			ret = -EINVAL;
			goto stop_pipeline;
		}
	} else {
		sync_fmt = fmt;
	}

	ret = snd_usb_lock_shutdown(chip);
	if (ret < 0)
		goto stop_pipeline;

	ret = snd_usb_pcm_change_state(subs, UAC3_PD_STATE_D0);
	if (ret < 0)
		goto unlock;

	if (subs->data_endpoint) {
		if (snd_usb_endpoint_compatible(chip, subs->data_endpoint,
						fmt, hw_params))
			goto unlock;
		close_endpoints(chip, subs);
	}

	subs->data_endpoint = snd_usb_endpoint_open(chip, fmt, hw_params, false);
	if (!subs->data_endpoint) {
		ret = -EINVAL;
		goto unlock;
	}

	if (fmt->sync_ep) {
		subs->sync_endpoint = snd_usb_endpoint_open(chip, sync_fmt,
							    hw_params,
							    fmt == sync_fmt);
		if (!subs->sync_endpoint) {
			ret = -EINVAL;
			goto unlock;
		}

		snd_usb_endpoint_set_sync(chip, subs->data_endpoint,
					  subs->sync_endpoint);
	}

	mutex_lock(&chip->mutex);
	subs->cur_audiofmt = fmt;
	mutex_unlock(&chip->mutex);

	ret = configure_endpoints(chip, subs);

 unlock:
	if (ret < 0)
		close_endpoints(chip, subs);

	snd_usb_unlock_shutdown(chip);
 stop_pipeline:
	if (ret < 0)
		snd_media_stop_pipeline(subs);

	return ret;
}

/*
 * hw_free callback
 *
 * reset the audio format and release the buffer
 */
static int snd_usb_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_usb_substream *subs = substream->runtime->private_data;
	struct snd_usb_audio *chip = subs->stream->chip;

	snd_media_stop_pipeline(subs);
	mutex_lock(&chip->mutex);
	subs->cur_audiofmt = NULL;
	mutex_unlock(&chip->mutex);
	if (!snd_usb_lock_shutdown(chip)) {
		if (stop_endpoints(subs))
			sync_pending_stops(subs);
		close_endpoints(chip, subs);
		snd_usb_unlock_shutdown(chip);
	}

	return 0;
}

/*
 * prepare callback
 *
 * only a few subtle things...
 */
static int snd_usb_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_usb_substream *subs = runtime->private_data;
	struct snd_usb_audio *chip = subs->stream->chip;
	int ret;

	ret = snd_usb_lock_shutdown(chip);
	if (ret < 0)
		return ret;
	if (snd_BUG_ON(!subs->data_endpoint)) {
		ret = -EIO;
		goto unlock;
	}

	ret = configure_endpoints(chip, subs);
	if (ret < 0)
		goto unlock;

	/* reset the pointer */
	subs->buffer_bytes = frames_to_bytes(runtime, runtime->buffer_size);
	subs->inflight_bytes = 0;
	subs->hwptr_done = 0;
	subs->transfer_done = 0;
	subs->last_frame_number = 0;
	subs->period_elapsed_pending = 0;
	runtime->delay = 0;

	/* check whether early start is needed for playback stream */
	subs->early_playback_start =
		subs->direction == SNDRV_PCM_STREAM_PLAYBACK &&
		(!chip->lowlatency ||
		 (subs->data_endpoint->nominal_queue_size >= subs->buffer_bytes));

	if (subs->early_playback_start)
		ret = start_endpoints(subs);

 unlock:
	snd_usb_unlock_shutdown(chip);
	return ret;
}

/*
 * h/w constraints
 */

#ifdef HW_CONST_DEBUG
#define hwc_debug(fmt, args...) pr_debug(fmt, ##args)
#else
#define hwc_debug(fmt, args...) do { } while(0)
#endif

static const struct snd_pcm_hardware snd_usb_hardware =
{
	.info =			SNDRV_PCM_INFO_MMAP |
				SNDRV_PCM_INFO_MMAP_VALID |
				SNDRV_PCM_INFO_BATCH |
				SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_BLOCK_TRANSFER |
				SNDRV_PCM_INFO_PAUSE,
	.channels_min =		1,
	.channels_max =		256,
	.buffer_bytes_max =	1024 * 1024,
	.period_bytes_min =	64,
	.period_bytes_max =	512 * 1024,
	.periods_min =		2,
	.periods_max =		1024,
};

static int hw_check_valid_format(struct snd_usb_substream *subs,
				 struct snd_pcm_hw_params *params,
				 const struct audioformat *fp)
{
	struct snd_interval *it = hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE);
	struct snd_interval *ct = hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS);
	struct snd_mask *fmts = hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);
	struct snd_interval *pt = hw_param_interval(params, SNDRV_PCM_HW_PARAM_PERIOD_TIME);
	struct snd_mask check_fmts;
	unsigned int ptime;

	/* check the format */
	snd_mask_none(&check_fmts);
	check_fmts.bits[0] = (u32)fp->formats;
	check_fmts.bits[1] = (u32)(fp->formats >> 32);
	snd_mask_intersect(&check_fmts, fmts);
	if (snd_mask_empty(&check_fmts)) {
		hwc_debug("   > check: no supported format 0x%llx\n", fp->formats);
		return 0;
	}
	/* check the channels */
	if (fp->channels < ct->min || fp->channels > ct->max) {
		hwc_debug("   > check: no valid channels %d (%d/%d)\n", fp->channels, ct->min, ct->max);
		return 0;
	}
	/* check the rate is within the range */
	if (fp->rate_min > it->max || (fp->rate_min == it->max && it->openmax)) {
		hwc_debug("   > check: rate_min %d > max %d\n", fp->rate_min, it->max);
		return 0;
	}
	if (fp->rate_max < it->min || (fp->rate_max == it->min && it->openmin)) {
		hwc_debug("   > check: rate_max %d < min %d\n", fp->rate_max, it->min);
		return 0;
	}
	/* check whether the period time is >= the data packet interval */
	if (subs->speed != USB_SPEED_FULL) {
		ptime = 125 * (1 << fp->datainterval);
		if (ptime > pt->max || (ptime == pt->max && pt->openmax)) {
			hwc_debug("   > check: ptime %u > max %u\n", ptime, pt->max);
			return 0;
		}
	}
	return 1;
}

static int apply_hw_params_minmax(struct snd_interval *it, unsigned int rmin,
				  unsigned int rmax)
{
	int changed;

	if (rmin > rmax) {
		hwc_debug("  --> get empty\n");
		it->empty = 1;
		return -EINVAL;
	}

	changed = 0;
	if (it->min < rmin) {
		it->min = rmin;
		it->openmin = 0;
		changed = 1;
	}
	if (it->max > rmax) {
		it->max = rmax;
		it->openmax = 0;
		changed = 1;
	}
	if (snd_interval_checkempty(it)) {
		it->empty = 1;
		return -EINVAL;
	}
	hwc_debug("  --> (%d, %d) (changed = %d)\n", it->min, it->max, changed);
	return changed;
}

static int hw_rule_rate(struct snd_pcm_hw_params *params,
			struct snd_pcm_hw_rule *rule)
{
	struct snd_usb_substream *subs = rule->private;
	const struct audioformat *fp;
	struct snd_interval *it = hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE);
	unsigned int rmin, rmax, r;
	int i;

	hwc_debug("hw_rule_rate: (%d,%d)\n", it->min, it->max);
	rmin = UINT_MAX;
	rmax = 0;
	list_for_each_entry(fp, &subs->fmt_list, list) {
		if (!hw_check_valid_format(subs, params, fp))
			continue;
		if (fp->rate_table && fp->nr_rates) {
			for (i = 0; i < fp->nr_rates; i++) {
				r = fp->rate_table[i];
				if (!snd_interval_test(it, r))
					continue;
				rmin = min(rmin, r);
				rmax = max(rmax, r);
			}
		} else {
			rmin = min(rmin, fp->rate_min);
			rmax = max(rmax, fp->rate_max);
		}
	}

	return apply_hw_params_minmax(it, rmin, rmax);
}


static int hw_rule_channels(struct snd_pcm_hw_params *params,
			    struct snd_pcm_hw_rule *rule)
{
	struct snd_usb_substream *subs = rule->private;
	const struct audioformat *fp;
	struct snd_interval *it = hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS);
	unsigned int rmin, rmax;

	hwc_debug("hw_rule_channels: (%d,%d)\n", it->min, it->max);
	rmin = UINT_MAX;
	rmax = 0;
	list_for_each_entry(fp, &subs->fmt_list, list) {
		if (!hw_check_valid_format(subs, params, fp))
			continue;
		rmin = min(rmin, fp->channels);
		rmax = max(rmax, fp->channels);
	}

	return apply_hw_params_minmax(it, rmin, rmax);
}

static int apply_hw_params_format_bits(struct snd_mask *fmt, u64 fbits)
{
	u32 oldbits[2];
	int changed;

	oldbits[0] = fmt->bits[0];
	oldbits[1] = fmt->bits[1];
	fmt->bits[0] &= (u32)fbits;
	fmt->bits[1] &= (u32)(fbits >> 32);
	if (!fmt->bits[0] && !fmt->bits[1]) {
		hwc_debug("  --> get empty\n");
		return -EINVAL;
	}
	changed = (oldbits[0] != fmt->bits[0] || oldbits[1] != fmt->bits[1]);
	hwc_debug("  --> %x:%x (changed = %d)\n", fmt->bits[0], fmt->bits[1], changed);
	return changed;
}

static int hw_rule_format(struct snd_pcm_hw_params *params,
			  struct snd_pcm_hw_rule *rule)
{
	struct snd_usb_substream *subs = rule->private;
	const struct audioformat *fp;
	struct snd_mask *fmt = hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);
	u64 fbits;

	hwc_debug("hw_rule_format: %x:%x\n", fmt->bits[0], fmt->bits[1]);
	fbits = 0;
	list_for_each_entry(fp, &subs->fmt_list, list) {
		if (!hw_check_valid_format(subs, params, fp))
			continue;
		fbits |= fp->formats;
	}
	return apply_hw_params_format_bits(fmt, fbits);
}

static int hw_rule_period_time(struct snd_pcm_hw_params *params,
			       struct snd_pcm_hw_rule *rule)
{
	struct snd_usb_substream *subs = rule->private;
	const struct audioformat *fp;
	struct snd_interval *it;
	unsigned char min_datainterval;
	unsigned int pmin;

	it = hw_param_interval(params, SNDRV_PCM_HW_PARAM_PERIOD_TIME);
	hwc_debug("hw_rule_period_time: (%u,%u)\n", it->min, it->max);
	min_datainterval = 0xff;
	list_for_each_entry(fp, &subs->fmt_list, list) {
		if (!hw_check_valid_format(subs, params, fp))
			continue;
		min_datainterval = min(min_datainterval, fp->datainterval);
	}
	if (min_datainterval == 0xff) {
		hwc_debug("  --> get empty\n");
		it->empty = 1;
		return -EINVAL;
	}
	pmin = 125 * (1 << min_datainterval);

	return apply_hw_params_minmax(it, pmin, UINT_MAX);
}

/* get the EP or the sync EP for implicit fb when it's already set up */
static const struct snd_usb_endpoint *
get_sync_ep_from_substream(struct snd_usb_substream *subs)
{
	struct snd_usb_audio *chip = subs->stream->chip;
	const struct audioformat *fp;
	const struct snd_usb_endpoint *ep;

	list_for_each_entry(fp, &subs->fmt_list, list) {
		ep = snd_usb_get_endpoint(chip, fp->endpoint);
		if (ep && ep->cur_audiofmt) {
			/* if EP is already opened solely for this substream,
			 * we still allow us to change the parameter; otherwise
			 * this substream has to follow the existing parameter
			 */
			if (ep->cur_audiofmt != subs->cur_audiofmt || ep->opened > 1)
				return ep;
		}
		if (!fp->implicit_fb)
			continue;
		/* for the implicit fb, check the sync ep as well */
		ep = snd_usb_get_endpoint(chip, fp->sync_ep);
		if (ep && ep->cur_audiofmt)
			return ep;
	}
	return NULL;
}

/* additional hw constraints for implicit feedback mode */
static int hw_rule_format_implicit_fb(struct snd_pcm_hw_params *params,
				      struct snd_pcm_hw_rule *rule)
{
	struct snd_usb_substream *subs = rule->private;
	const struct snd_usb_endpoint *ep;
	struct snd_mask *fmt = hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);

	ep = get_sync_ep_from_substream(subs);
	if (!ep)
		return 0;

	hwc_debug("applying %s\n", __func__);
	return apply_hw_params_format_bits(fmt, pcm_format_to_bits(ep->cur_format));
}

static int hw_rule_rate_implicit_fb(struct snd_pcm_hw_params *params,
				    struct snd_pcm_hw_rule *rule)
{
	struct snd_usb_substream *subs = rule->private;
	const struct snd_usb_endpoint *ep;
	struct snd_interval *it;

	ep = get_sync_ep_from_substream(subs);
	if (!ep)
		return 0;

	hwc_debug("applying %s\n", __func__);
	it = hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE);
	return apply_hw_params_minmax(it, ep->cur_rate, ep->cur_rate);
}

static int hw_rule_period_size_implicit_fb(struct snd_pcm_hw_params *params,
					   struct snd_pcm_hw_rule *rule)
{
	struct snd_usb_substream *subs = rule->private;
	const struct snd_usb_endpoint *ep;
	struct snd_interval *it;

	ep = get_sync_ep_from_substream(subs);
	if (!ep)
		return 0;

	hwc_debug("applying %s\n", __func__);
	it = hw_param_interval(params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE);
	return apply_hw_params_minmax(it, ep->cur_period_frames,
				      ep->cur_period_frames);
}

static int hw_rule_periods_implicit_fb(struct snd_pcm_hw_params *params,
				       struct snd_pcm_hw_rule *rule)
{
	struct snd_usb_substream *subs = rule->private;
	const struct snd_usb_endpoint *ep;
	struct snd_interval *it;

	ep = get_sync_ep_from_substream(subs);
	if (!ep)
		return 0;

	hwc_debug("applying %s\n", __func__);
	it = hw_param_interval(params, SNDRV_PCM_HW_PARAM_PERIODS);
	return apply_hw_params_minmax(it, ep->cur_buffer_periods,
				      ep->cur_buffer_periods);
}

/*
 * set up the runtime hardware information.
 */

static int setup_hw_info(struct snd_pcm_runtime *runtime, struct snd_usb_substream *subs)
{
	const struct audioformat *fp;
	unsigned int pt, ptmin;
	int param_period_time_if_needed = -1;
	int err;

	runtime->hw.formats = subs->formats;

	runtime->hw.rate_min = 0x7fffffff;
	runtime->hw.rate_max = 0;
	runtime->hw.channels_min = 256;
	runtime->hw.channels_max = 0;
	runtime->hw.rates = 0;
	ptmin = UINT_MAX;
	/* check min/max rates and channels */
	list_for_each_entry(fp, &subs->fmt_list, list) {
		runtime->hw.rates |= fp->rates;
		if (runtime->hw.rate_min > fp->rate_min)
			runtime->hw.rate_min = fp->rate_min;
		if (runtime->hw.rate_max < fp->rate_max)
			runtime->hw.rate_max = fp->rate_max;
		if (runtime->hw.channels_min > fp->channels)
			runtime->hw.channels_min = fp->channels;
		if (runtime->hw.channels_max < fp->channels)
			runtime->hw.channels_max = fp->channels;
		if (fp->fmt_type == UAC_FORMAT_TYPE_II && fp->frame_size > 0) {
			/* FIXME: there might be more than one audio formats... */
			runtime->hw.period_bytes_min = runtime->hw.period_bytes_max =
				fp->frame_size;
		}
		pt = 125 * (1 << fp->datainterval);
		ptmin = min(ptmin, pt);
	}

	param_period_time_if_needed = SNDRV_PCM_HW_PARAM_PERIOD_TIME;
	if (subs->speed == USB_SPEED_FULL)
		/* full speed devices have fixed data packet interval */
		ptmin = 1000;
	if (ptmin == 1000)
		/* if period time doesn't go below 1 ms, no rules needed */
		param_period_time_if_needed = -1;

	err = snd_pcm_hw_constraint_minmax(runtime,
					   SNDRV_PCM_HW_PARAM_PERIOD_TIME,
					   ptmin, UINT_MAX);
	if (err < 0)
		return err;

	err = snd_pcm_hw_rule_add(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
				  hw_rule_rate, subs,
				  SNDRV_PCM_HW_PARAM_RATE,
				  SNDRV_PCM_HW_PARAM_FORMAT,
				  SNDRV_PCM_HW_PARAM_CHANNELS,
				  param_period_time_if_needed,
				  -1);
	if (err < 0)
		return err;

	err = snd_pcm_hw_rule_add(runtime, 0, SNDRV_PCM_HW_PARAM_CHANNELS,
				  hw_rule_channels, subs,
				  SNDRV_PCM_HW_PARAM_CHANNELS,
				  SNDRV_PCM_HW_PARAM_FORMAT,
				  SNDRV_PCM_HW_PARAM_RATE,
				  param_period_time_if_needed,
				  -1);
	if (err < 0)
		return err;
	err = snd_pcm_hw_rule_add(runtime, 0, SNDRV_PCM_HW_PARAM_FORMAT,
				  hw_rule_format, subs,
				  SNDRV_PCM_HW_PARAM_FORMAT,
				  SNDRV_PCM_HW_PARAM_RATE,
				  SNDRV_PCM_HW_PARAM_CHANNELS,
				  param_period_time_if_needed,
				  -1);
	if (err < 0)
		return err;
	if (param_period_time_if_needed >= 0) {
		err = snd_pcm_hw_rule_add(runtime, 0,
					  SNDRV_PCM_HW_PARAM_PERIOD_TIME,
					  hw_rule_period_time, subs,
					  SNDRV_PCM_HW_PARAM_FORMAT,
					  SNDRV_PCM_HW_PARAM_CHANNELS,
					  SNDRV_PCM_HW_PARAM_RATE,
					  -1);
		if (err < 0)
			return err;
	}

	/* additional hw constraints for implicit fb */
	err = snd_pcm_hw_rule_add(runtime, 0, SNDRV_PCM_HW_PARAM_FORMAT,
				  hw_rule_format_implicit_fb, subs,
				  SNDRV_PCM_HW_PARAM_FORMAT, -1);
	if (err < 0)
		return err;
	err = snd_pcm_hw_rule_add(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
				  hw_rule_rate_implicit_fb, subs,
				  SNDRV_PCM_HW_PARAM_RATE, -1);
	if (err < 0)
		return err;
	err = snd_pcm_hw_rule_add(runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_SIZE,
				  hw_rule_period_size_implicit_fb, subs,
				  SNDRV_PCM_HW_PARAM_PERIOD_SIZE, -1);
	if (err < 0)
		return err;
	err = snd_pcm_hw_rule_add(runtime, 0, SNDRV_PCM_HW_PARAM_PERIODS,
				  hw_rule_periods_implicit_fb, subs,
				  SNDRV_PCM_HW_PARAM_PERIODS, -1);
	if (err < 0)
		return err;

	return 0;
}

static int snd_usb_pcm_open(struct snd_pcm_substream *substream)
{
	int direction = substream->stream;
	struct snd_usb_stream *as = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_usb_substream *subs = &as->substream[direction];
	int ret;

	runtime->hw = snd_usb_hardware;
	runtime->private_data = subs;
	subs->pcm_substream = substream;
	/* runtime PM is also done there */

	/* initialize DSD/DOP context */
	subs->dsd_dop.byte_idx = 0;
	subs->dsd_dop.channel = 0;
	subs->dsd_dop.marker = 1;

	ret = setup_hw_info(runtime, subs);
	if (ret < 0)
		return ret;
	ret = snd_usb_autoresume(subs->stream->chip);
	if (ret < 0)
		return ret;
	ret = snd_media_stream_init(subs, as->pcm, direction);
	if (ret < 0)
		snd_usb_autosuspend(subs->stream->chip);
	return ret;
}

static int snd_usb_pcm_close(struct snd_pcm_substream *substream)
{
	int direction = substream->stream;
	struct snd_usb_stream *as = snd_pcm_substream_chip(substream);
	struct snd_usb_substream *subs = &as->substream[direction];
	int ret;

	snd_media_stop_pipeline(subs);

	if (!snd_usb_lock_shutdown(subs->stream->chip)) {
		ret = snd_usb_pcm_change_state(subs, UAC3_PD_STATE_D1);
		snd_usb_unlock_shutdown(subs->stream->chip);
		if (ret < 0)
			return ret;
	}

	subs->pcm_substream = NULL;
	snd_usb_autosuspend(subs->stream->chip);

	return 0;
}

/* Since a URB can handle only a single linear buffer, we must use double
 * buffering when the data to be transferred overflows the buffer boundary.
 * To avoid inconsistencies when updating hwptr_done, we use double buffering
 * for all URBs.
 */
static void retire_capture_urb(struct snd_usb_substream *subs,
			       struct urb *urb)
{
	struct snd_pcm_runtime *runtime = subs->pcm_substream->runtime;
	unsigned int stride, frames, bytes, oldptr;
	int i, period_elapsed = 0;
	unsigned long flags;
	unsigned char *cp;
	int current_frame_number;

	int j, k, input_ptr = 0;
	char input_buffer[128];
	char output_buffer[128];

	/* read frame number here, update pointer in critical section */
	current_frame_number = usb_get_current_frame_number(subs->dev);

	stride = runtime->frame_bits >> 3;

	for (i = 0; i < urb->number_of_packets; i++) {
		cp = (unsigned char *)urb->transfer_buffer + urb->iso_frame_desc[i].offset + subs->pkt_offset_adj;
		if (urb->iso_frame_desc[i].status && printk_ratelimit()) {
			dev_dbg(&subs->dev->dev, "frame %d active: %d\n",
				i, urb->iso_frame_desc[i].status);
			// continue;
		}
		bytes = urb->iso_frame_desc[i].actual_length;
		if (subs->stream_offset_adj > 0) {
			unsigned int adj = min(subs->stream_offset_adj, bytes);
			cp += adj;
			bytes -= adj;
			subs->stream_offset_adj -= adj;
		}
		frames = bytes / stride;
		if (!subs->txfr_quirk)
			bytes = frames * stride;
		if (bytes % (runtime->sample_bits >> 3) != 0) {
			int oldbytes = bytes;
			bytes = frames * stride;
			dev_warn_ratelimited(&subs->dev->dev,
				 "Corrected urb data len. %d->%d\n",
							oldbytes, bytes);
		}

		for (j = 0; j < bytes; j++) {
			input_buffer[input_ptr] = cp[j];
			input_ptr = (input_ptr + 1) % 128;

			if (input_ptr == 0) {
				for (k = 0; k < 128; k++)
					output_buffer[k] = input_buffer[k];

				if (efeito == 1) {
					DFT(input_buffer, 64);
					iDFT(output_buffer, 64);
				} else if (efeito == 2) {
					for (k = 0; k < 64; k++) {
						long long sample = ((long long)input_buffer[2 * k + 1] << 8) + (long long)input_buffer[2 * k];
						sample *= cos_alien[k];
						sample /= 1000000;

						output_buffer[2 * k] = (char)(sample & 255);
						output_buffer[2 * k + 1] = (char)(sample >> 8);
					}
				}

				spin_lock_irqsave(&subs->lock, flags);
				oldptr = subs->hwptr_done;
				subs->hwptr_done += 128;
				if (subs->hwptr_done >= subs->buffer_bytes)
					subs->hwptr_done -= subs->buffer_bytes;
				frames = (128 + (oldptr % stride)) / stride;
				subs->transfer_done += frames;
				if (subs->transfer_done >= runtime->period_size) {
					subs->transfer_done -= runtime->period_size;
					period_elapsed = 1;
				}

				/* realign last_frame_number */
				subs->last_frame_number = current_frame_number;

				spin_unlock_irqrestore(&subs->lock, flags);

				if (oldptr + 128 > subs->buffer_bytes) {
					unsigned int bytes1 = subs->buffer_bytes - oldptr;

					memcpy(runtime->dma_area + oldptr, output_buffer, bytes1);
					memcpy(runtime->dma_area, output_buffer + bytes1, 128 - bytes1);
				} else {
					memcpy(runtime->dma_area + oldptr, output_buffer, 128);
				}
			}
		}
	}

	if (input_ptr != 0) {
		char tmp_buffer[128];
		unsigned int remaining = 128 - input_ptr;

		for (j = 0; j < 128; j++) {
			tmp_buffer[j] = input_buffer[j];
			output_buffer[j] = 0;
		}

		for (j = 0; j < remaining; j++)
			input_buffer[j] = tmp_buffer[input_ptr + j];
		for (j = remaining; j < 128; j++)
			input_buffer[j] = tmp_buffer[j - remaining];

		for (j = 0; j < 128; j++)
			output_buffer[j] = input_buffer[j];

		if (efeito == 1) {
			DFT(input_buffer, 64);
			iDFT(output_buffer, 64);
		} else if (efeito == 2) {
			for (j = 0; j < 64; j++) {
				long long sample = ((long long)input_buffer[2 * j + 1] << 8) + (long long)input_buffer[2 * j];
				sample *= cos_alien[j];
				sample /= 1000000;

				output_buffer[2 * j] = (char)(sample & 255);
				output_buffer[2 * j + 1] = (char)(sample >> 8);
			}
		}

		spin_lock_irqsave(&subs->lock, flags);
		oldptr = subs->hwptr_done;
		subs->hwptr_done += input_ptr;
		if (subs->hwptr_done >= subs->buffer_bytes)
			subs->hwptr_done -= subs->buffer_bytes;
		frames = (input_ptr + (oldptr % stride)) / stride;
		subs->transfer_done += frames;
		if (subs->transfer_done >= runtime->period_size) {
			subs->transfer_done -= runtime->period_size;
			period_elapsed = 1;
		}

		/* realign last_frame_number */
		subs->last_frame_number = current_frame_number;

		spin_unlock_irqrestore(&subs->lock, flags);

		if (oldptr + input_ptr > subs->buffer_bytes) {
			unsigned int bytes1 = subs->buffer_bytes - oldptr;

			memcpy(runtime->dma_area + oldptr, output_buffer + remaining, bytes1);
			memcpy(runtime->dma_area, output_buffer + remaining + bytes1, input_ptr - bytes1);
		} else {
			memcpy(runtime->dma_area + oldptr, output_buffer + remaining, input_ptr);
		}
	}

	if (period_elapsed)
		snd_pcm_period_elapsed(subs->pcm_substream);
}

static void urb_ctx_queue_advance(struct snd_usb_substream *subs,
				  struct urb *urb, unsigned int bytes)
{
	struct snd_urb_ctx *ctx = urb->context;

	ctx->queued += bytes;
	subs->inflight_bytes += bytes;
	subs->hwptr_done += bytes;
	if (subs->hwptr_done >= subs->buffer_bytes)
		subs->hwptr_done -= subs->buffer_bytes;
}

static inline void fill_playback_urb_dsd_dop(struct snd_usb_substream *subs,
					     struct urb *urb, unsigned int bytes)
{
	struct snd_pcm_runtime *runtime = subs->pcm_substream->runtime;
	unsigned int dst_idx = 0;
	unsigned int src_idx = subs->hwptr_done;
	unsigned int wrap = subs->buffer_bytes;
	u8 *dst = urb->transfer_buffer;
	u8 *src = runtime->dma_area;
	u8 marker[] = { 0x05, 0xfa };
	unsigned int queued = 0;

	/*
	 * The DSP DOP format defines a way to transport DSD samples over
	 * normal PCM data endpoints. It requires stuffing of marker bytes
	 * (0x05 and 0xfa, alternating per sample frame), and then expects
	 * 2 additional bytes of actual payload. The whole frame is stored
	 * LSB.
	 *
	 * Hence, for a stereo transport, the buffer layout looks like this,
	 * where L refers to left channel samples and R to right.
	 *
	 *   L1 L2 0x05   R1 R2 0x05   L3 L4 0xfa  R3 R4 0xfa
	 *   L5 L6 0x05   R5 R6 0x05   L7 L8 0xfa  R7 R8 0xfa
	 *   .....
	 *
	 */

	while (bytes--) {
		if (++subs->dsd_dop.byte_idx == 3) {
			/* frame boundary? */
			dst[dst_idx++] = marker[subs->dsd_dop.marker];
			src_idx += 2;
			subs->dsd_dop.byte_idx = 0;

			if (++subs->dsd_dop.channel % runtime->channels == 0) {
				/* alternate the marker */
				subs->dsd_dop.marker++;
				subs->dsd_dop.marker %= ARRAY_SIZE(marker);
				subs->dsd_dop.channel = 0;
			}
		} else {
			/* stuff the DSD payload */
			int idx = (src_idx + subs->dsd_dop.byte_idx - 1) % wrap;

			if (subs->cur_audiofmt->dsd_bitrev)
				dst[dst_idx++] = bitrev8(src[idx]);
			else
				dst[dst_idx++] = src[idx];
			queued++;
		}
	}

	urb_ctx_queue_advance(subs, urb, queued);
}

/* copy bit-reversed bytes onto transfer buffer */
static void fill_playback_urb_dsd_bitrev(struct snd_usb_substream *subs,
					 struct urb *urb, unsigned int bytes)
{
	struct snd_pcm_runtime *runtime = subs->pcm_substream->runtime;
	const u8 *src = runtime->dma_area;
	u8 *buf = urb->transfer_buffer;
	int i, ofs = subs->hwptr_done;

	for (i = 0; i < bytes; i++) {
		*buf++ = bitrev8(src[ofs]);
		if (++ofs >= subs->buffer_bytes)
			ofs = 0;
	}

	urb_ctx_queue_advance(subs, urb, bytes);
}

static void copy_to_urb(struct snd_usb_substream *subs, struct urb *urb,
			int offset, int stride, unsigned int bytes)
{
	struct snd_pcm_runtime *runtime = subs->pcm_substream->runtime;

	if (subs->hwptr_done + bytes > subs->buffer_bytes) {
		/* err, the transferred area goes over buffer boundary. */
		unsigned int bytes1 = subs->buffer_bytes - subs->hwptr_done;

		memcpy(urb->transfer_buffer + offset,
		       runtime->dma_area + subs->hwptr_done, bytes1);
		memcpy(urb->transfer_buffer + offset + bytes1,
		       runtime->dma_area, bytes - bytes1);
	} else {
		memcpy(urb->transfer_buffer + offset,
		       runtime->dma_area + subs->hwptr_done, bytes);
	}

	urb_ctx_queue_advance(subs, urb, bytes);
}

static unsigned int copy_to_urb_quirk(struct snd_usb_substream *subs,
				      struct urb *urb, int stride,
				      unsigned int bytes)
{
	__le32 packet_length;
	int i;

	/* Put __le32 length descriptor at start of each packet. */
	for (i = 0; i < urb->number_of_packets; i++) {
		unsigned int length = urb->iso_frame_desc[i].length;
		unsigned int offset = urb->iso_frame_desc[i].offset;

		packet_length = cpu_to_le32(length);
		offset += i * sizeof(packet_length);
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length += sizeof(packet_length);
		memcpy(urb->transfer_buffer + offset,
		       &packet_length, sizeof(packet_length));
		copy_to_urb(subs, urb, offset + sizeof(packet_length),
			    stride, length);
	}
	/* Adjust transfer size accordingly. */
	bytes += urb->number_of_packets * sizeof(packet_length);
	return bytes;
}

static void prepare_playback_urb(struct snd_usb_substream *subs,
				 struct urb *urb)
{
	struct snd_pcm_runtime *runtime = subs->pcm_substream->runtime;
	struct snd_usb_endpoint *ep = subs->data_endpoint;
	struct snd_urb_ctx *ctx = urb->context;
	unsigned int counts, frames, bytes;
	int i, stride, period_elapsed = 0;
	unsigned long flags;

	stride = ep->stride;

	frames = 0;
	ctx->queued = 0;
	urb->number_of_packets = 0;
	spin_lock_irqsave(&subs->lock, flags);
	subs->frame_limit += ep->max_urb_frames;
	for (i = 0; i < ctx->packets; i++) {
		counts = snd_usb_endpoint_next_packet_size(ep, ctx, i);
		/* set up descriptor */
		urb->iso_frame_desc[i].offset = frames * stride;
		urb->iso_frame_desc[i].length = counts * stride;
		frames += counts;
		urb->number_of_packets++;
		subs->transfer_done += counts;
		if (subs->transfer_done >= runtime->period_size) {
			subs->transfer_done -= runtime->period_size;
			subs->frame_limit = 0;
			period_elapsed = 1;
			if (subs->fmt_type == UAC_FORMAT_TYPE_II) {
				if (subs->transfer_done > 0) {
					/* FIXME: fill-max mode is not
					 * supported yet */
					frames -= subs->transfer_done;
					counts -= subs->transfer_done;
					urb->iso_frame_desc[i].length =
						counts * stride;
					subs->transfer_done = 0;
				}
				i++;
				if (i < ctx->packets) {
					/* add a transfer delimiter */
					urb->iso_frame_desc[i].offset =
						frames * stride;
					urb->iso_frame_desc[i].length = 0;
					urb->number_of_packets++;
				}
				break;
			}
		}
		/* finish at the period boundary or after enough frames */
		if ((period_elapsed ||
				subs->transfer_done >= subs->frame_limit) &&
		    !snd_usb_endpoint_implicit_feedback_sink(ep))
			break;
	}
	bytes = frames * stride;

	if (unlikely(ep->cur_format == SNDRV_PCM_FORMAT_DSD_U16_LE &&
		     subs->cur_audiofmt->dsd_dop)) {
		fill_playback_urb_dsd_dop(subs, urb, bytes);
	} else if (unlikely(ep->cur_format == SNDRV_PCM_FORMAT_DSD_U8 &&
			   subs->cur_audiofmt->dsd_bitrev)) {
		fill_playback_urb_dsd_bitrev(subs, urb, bytes);
	} else {
		/* usual PCM */
		if (!subs->tx_length_quirk)
			copy_to_urb(subs, urb, 0, stride, bytes);
		else
			bytes = copy_to_urb_quirk(subs, urb, stride, bytes);
			/* bytes is now amount of outgoing data */
	}

	subs->last_frame_number = usb_get_current_frame_number(subs->dev);

	if (subs->trigger_tstamp_pending_update) {
		/* this is the first actual URB submitted,
		 * update trigger timestamp to reflect actual start time
		 */
		snd_pcm_gettime(runtime, &runtime->trigger_tstamp);
		subs->trigger_tstamp_pending_update = false;
	}

	if (period_elapsed && !subs->running && !subs->early_playback_start) {
		subs->period_elapsed_pending = 1;
		period_elapsed = 0;
	}
	spin_unlock_irqrestore(&subs->lock, flags);
	urb->transfer_buffer_length = bytes;
	if (period_elapsed)
		snd_pcm_period_elapsed(subs->pcm_substream);
}

/*
 * process after playback data complete
 * - decrease the delay count again
 */
static void retire_playback_urb(struct snd_usb_substream *subs,
			       struct urb *urb)
{
	unsigned long flags;
	struct snd_urb_ctx *ctx = urb->context;
	bool period_elapsed = false;

	spin_lock_irqsave(&subs->lock, flags);
	if (ctx->queued) {
		if (subs->inflight_bytes >= ctx->queued)
			subs->inflight_bytes -= ctx->queued;
		else
			subs->inflight_bytes = 0;
	}

	subs->last_frame_number = usb_get_current_frame_number(subs->dev);
	if (subs->running) {
		period_elapsed = subs->period_elapsed_pending;
		subs->period_elapsed_pending = 0;
	}
	spin_unlock_irqrestore(&subs->lock, flags);
	if (period_elapsed)
		snd_pcm_period_elapsed(subs->pcm_substream);
}

static int snd_usb_substream_playback_trigger(struct snd_pcm_substream *substream,
					      int cmd)
{
	struct snd_usb_substream *subs = substream->runtime->private_data;
	int err;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		subs->trigger_tstamp_pending_update = true;
		fallthrough;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		snd_usb_endpoint_set_callback(subs->data_endpoint,
					      prepare_playback_urb,
					      retire_playback_urb,
					      subs);
		if (!subs->early_playback_start &&
		    cmd == SNDRV_PCM_TRIGGER_START) {
			err = start_endpoints(subs);
			if (err < 0) {
				snd_usb_endpoint_set_callback(subs->data_endpoint,
							      NULL, NULL, NULL);
				return err;
			}
		}
		subs->running = 1;
		dev_dbg(&subs->dev->dev, "%d:%d Start Playback PCM\n",
			subs->cur_audiofmt->iface,
			subs->cur_audiofmt->altsetting);
		return 0;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
		stop_endpoints(subs);
		snd_usb_endpoint_set_callback(subs->data_endpoint,
					      NULL, NULL, NULL);
		subs->running = 0;
		dev_dbg(&subs->dev->dev, "%d:%d Stop Playback PCM\n",
			subs->cur_audiofmt->iface,
			subs->cur_audiofmt->altsetting);
		return 0;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* keep retire_data_urb for delay calculation */
		snd_usb_endpoint_set_callback(subs->data_endpoint,
					      NULL,
					      retire_playback_urb,
					      subs);
		subs->running = 0;
		dev_dbg(&subs->dev->dev, "%d:%d Pause Playback PCM\n",
			subs->cur_audiofmt->iface,
			subs->cur_audiofmt->altsetting);
		return 0;
	}

	return -EINVAL;
}

static int snd_usb_substream_capture_trigger(struct snd_pcm_substream *substream,
					     int cmd)
{
	int err;
	struct snd_usb_substream *subs = substream->runtime->private_data;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		err = start_endpoints(subs);
		if (err < 0)
			return err;
		fallthrough;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		snd_usb_endpoint_set_callback(subs->data_endpoint,
					      NULL, retire_capture_urb,
					      subs);
		subs->last_frame_number = usb_get_current_frame_number(subs->dev);
		subs->running = 1;
		dev_dbg(&subs->dev->dev, "%d:%d Start Capture PCM\n",
			subs->cur_audiofmt->iface,
			subs->cur_audiofmt->altsetting);
		return 0;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
		stop_endpoints(subs);
		fallthrough;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		snd_usb_endpoint_set_callback(subs->data_endpoint,
					      NULL, NULL, NULL);
		subs->running = 0;
		dev_dbg(&subs->dev->dev, "%d:%d Stop Capture PCM\n",
			subs->cur_audiofmt->iface,
			subs->cur_audiofmt->altsetting);
		return 0;
	}

	return -EINVAL;
}

static const struct snd_pcm_ops snd_usb_playback_ops = {
	.open =		snd_usb_pcm_open,
	.close =	snd_usb_pcm_close,
	.hw_params =	snd_usb_hw_params,
	.hw_free =	snd_usb_hw_free,
	.prepare =	snd_usb_pcm_prepare,
	.trigger =	snd_usb_substream_playback_trigger,
	.sync_stop =	snd_usb_pcm_sync_stop,
	.pointer =	snd_usb_pcm_pointer,
};

static const struct snd_pcm_ops snd_usb_capture_ops = {
	.open =		snd_usb_pcm_open,
	.close =	snd_usb_pcm_close,
	.hw_params =	snd_usb_hw_params,
	.hw_free =	snd_usb_hw_free,
	.prepare =	snd_usb_pcm_prepare,
	.trigger =	snd_usb_substream_capture_trigger,
	.sync_stop =	snd_usb_pcm_sync_stop,
	.pointer =	snd_usb_pcm_pointer,
};

void snd_usb_set_pcm_ops(struct snd_pcm *pcm, int stream)
{
	const struct snd_pcm_ops *ops;

	ops = stream == SNDRV_PCM_STREAM_PLAYBACK ?
			&snd_usb_playback_ops : &snd_usb_capture_ops;
	snd_pcm_set_ops(pcm, stream, ops);
}

void snd_usb_preallocate_buffer(struct snd_usb_substream *subs)
{
	struct snd_pcm *pcm = subs->stream->pcm;
	struct snd_pcm_substream *s = pcm->streams[subs->direction].substream;
	struct device *dev = subs->dev->bus->sysdev;

	if (snd_usb_use_vmalloc)
		snd_pcm_set_managed_buffer(s, SNDRV_DMA_TYPE_VMALLOC,
					   NULL, 0, 0);
	else
		snd_pcm_set_managed_buffer(s, SNDRV_DMA_TYPE_DEV_SG,
					   dev, 64*1024, 512*1024);
}

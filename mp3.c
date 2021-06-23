#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <pspkernel.h>
#include <pspaudiolib.h>
#include <pspaudio.h>
#include "lib/id3tag.h"
#include "lib/mad.h"
#include "common/utils.h"
#include "mp3.h"
#define MP3_MAX_ENTRY 256
#define MAXVOLUME 0x8000

// resample from madplay, thanks to nj-zero
#define MAX_RESAMPLEFACTOR 6
#define MAX_NSAMPLES (1152 * MAX_RESAMPLEFACTOR)

struct resample_state {
	mad_fixed_t ratio;
	mad_fixed_t step;
	mad_fixed_t last;
};

static mad_fixed_t	(*Resampled)[2][MAX_NSAMPLES];
static struct resample_state Resample[2];

static struct OutputPCM{
    unsigned int nsamples;
	mad_fixed_t const * samples[2];
} OutputPCM;

static int resample_init(struct resample_state *state, unsigned int oldrate, unsigned int newrate)
{
	mad_fixed_t ratio;
	
	if (newrate == 0)
		return -1;
	
	ratio = mad_f_div(oldrate, newrate);
	if (ratio <= 0 || ratio > MAX_RESAMPLEFACTOR * MAD_F_ONE)
		return -1;
	
	state->ratio = ratio;
	
	state->step = 0;
	state->last = 0;
	
	return 0;
}

static unsigned int resample_block(struct resample_state *state, unsigned int nsamples, mad_fixed_t const *old, mad_fixed_t *new)
{
	mad_fixed_t const *end, *begin;
	
	if (state->ratio == MAD_F_ONE) {
		memcpy(new, old, nsamples * sizeof(mad_fixed_t));
		return nsamples;
	}
	
	end   = old + nsamples;
	begin = new;
	
	if (state->step < 0) {
		state->step = mad_f_fracpart(-state->step);
		
		while (state->step < MAD_F_ONE) {
			*new++ = state->step ?
				state->last + mad_f_mul(*old - state->last, state->step) : state->last;
			
			state->step += state->ratio;
			if (((state->step + 0x00000080L) & 0x0fffff00L) == 0)
				state->step = (state->step + 0x00000080L) & ~0x0fffffffL;
		}
		
		state->step -= MAD_F_ONE;
	}
	
	while (end - old > 1 + mad_f_intpart(state->step)) {
		old        += mad_f_intpart(state->step);
		state->step = mad_f_fracpart(state->step);
		
		*new++ = state->step ?
			*old + mad_f_mul(old[1] - old[0], state->step) : *old;
		
		state->step += state->ratio;
		if (((state->step + 0x00000080L) & 0x0fffff00L) == 0)
			state->step = (state->step + 0x00000080L) & ~0x0fffffffL;
	}
	
	if (end - old == 1 + mad_f_intpart(state->step)) {
		state->last = end[-1];
		state->step = -state->step;
	}
	else
		state->step -= mad_f_fromint(end - old);
	
	return new - begin;
}
// end of resample definitions & functions

static int mp3_handle = 0;
static int fd = -1;
static char mp3_tag[128], mp3_files[MP3_MAX_ENTRY][260];
static int mp3_direct = -1, mp3_nfiles = 0, mp3_index = 0;
static bool isPlaying = false, eos = true, isPause = true;

#define INPUT_BUFFER_SIZE	(8*1024)
#define OUTPUT_BUFFER_SIZE	4*1024	/* Must be an integer multiple of 4. */

static mad_fixed_t Filter[32];
static int DoFilter = 0;
static SceUID mp3_thid = 0;
static int mp3_bitrate = 0, mp3_sample = 0, mp3_length = 0, mp3_filesize = 0;

static struct mad_stream Stream;
static struct mad_frame Frame;
static struct mad_synth Synth;
static mad_timer_t Timer;
static signed short OutputBuffer[4][OUTPUT_BUFFER_SIZE];
static byte InputBuffer[INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD],
		* OutputPtr = (byte *)&OutputBuffer[0][0], * GuardPtr = NULL,
		* OutputBufferEnd = (byte *)&OutputBuffer[0][0] + OUTPUT_BUFFER_SIZE * 2;
static int OutputBuffer_Index = 0;

extern int dup (int fd1)
{
	return (fcntl (fd1, F_DUPFD, 0));
}

extern int dup2 (int fd1, int fd2)
{
	close (fd2);
	return (fcntl (fd1, F_DUPFD, fd2));
}

static void apply_filter(struct mad_frame *Frame)
{
	int Channel, Sample, Samples, SubBand;

	Samples = MAD_NSBSAMPLES(&Frame->header);
	if (Frame->header.mode != MAD_MODE_SINGLE_CHANNEL)
		for (Channel = 0; Channel < 2; Channel++)
			for (Sample = 0; Sample < Samples; Sample++)
				for (SubBand = 0; SubBand < 32; SubBand++)
					Frame->sbsample[Channel][Sample][SubBand] =
					mad_f_mul(Frame->sbsample[Channel][Sample][SubBand], Filter[SubBand]);
	else
		for (Sample = 0; Sample < Samples; Sample++)
			for (SubBand = 0; SubBand < 32; SubBand++)
				Frame->sbsample[0][Sample][SubBand] = mad_f_mul(Frame->sbsample[0][Sample][SubBand], Filter[SubBand]);
}

static signed short MadFixedToSshort(mad_fixed_t Fixed)
{
	if (Fixed >= MAD_F_ONE)
		return (SHRT_MAX);
	if (Fixed <= -MAD_F_ONE)
		return (-SHRT_MAX);

	Fixed = Fixed >> (MAD_F_FRACBITS - 15);
	return ((signed short) Fixed);
}

static bool mp3_load()
{
	if(mp3_direct == 0)
	{
		mp3_index --;
		if(mp3_index < 0)
			mp3_index = mp3_nfiles - 1;
	}
	else if(mp3_direct > 0)
	{
		mp3_index ++;
		if(mp3_index >= mp3_nfiles)
			mp3_index = 0;
	}
	mp3_direct = 1;

	if(fd >= 0)
		sceIoClose(fd);

	mad_synth_finish(&Synth);
	mad_frame_finish(&Frame);
	mad_stream_finish(&Stream);
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);
	if ((fd = sceIoOpen(mp3_files[mp3_index], PSP_O_RDONLY, 0777)) < 0)
		return false;
	mp3_filesize = sceIoLseek32(fd, 0, PSP_SEEK_END);
	sceIoLseek32(fd, 0, PSP_SEEK_SET);
	{
		struct id3_file * id3 = id3_file_open(mp3_files[mp3_index], ID3_FILE_MODE_READONLY);
		struct id3_tag * id3tag;
		struct id3_frame * id3frame;
		union id3_field * id3field;
		id3_ucs4_t const *ucs4 = NULL;
		id3_latin1_t *latin1 = NULL;
		if(id3 != NULL && (id3tag = id3_file_tag(id3)) != NULL && (id3frame = id3_tag_findframe(id3tag, ID3_FRAME_TITLE, 0)) != NULL && (id3field = &id3frame->fields[1]) != NULL && id3_field_getnstrings(id3field) > 0 && (ucs4 = id3_field_getstrings(id3field, 0)) != NULL && (latin1 = id3_ucs4_latin1duplicate(ucs4)) != NULL)
		{
			id3_ucs4_t const *ucs4_2 = NULL;
			id3_latin1_t *latin1_2 = NULL;
			if((id3frame = id3_tag_findframe(id3tag, ID3_FRAME_ARTIST, 0)) != NULL && (id3field = &id3frame->fields[1]) != NULL && id3_field_getnstrings(id3field) > 0 && (ucs4_2 = id3_field_getstrings(id3field, 0)) != NULL && (latin1_2 = id3_ucs4_latin1duplicate(ucs4_2)) != NULL)
			{
				sprintf(mp3_tag, "%s - ", (const char *)latin1_2);
				free(latin1_2);
			}
			else
				strcpy(mp3_tag, "? - ");
			strcat(mp3_tag, (const char *)latin1);
			free (latin1);
		}
		else
		{
			char * mp3_tag2 = strrchr(mp3_files[mp3_index], '/');
			if(mp3_tag2 != NULL)
				mp3_tag2 ++;
			else
				mp3_tag2 = "";
			strcpy(mp3_tag, mp3_tag2);
		}
		if(id3 != NULL)
			id3_file_close(id3);
	}
	memset(&OutputBuffer[0][0], 0, 4 * 2 * OUTPUT_BUFFER_SIZE);
	OutputPtr = (byte *)&OutputBuffer[0][0];
	OutputBufferEnd = OutputPtr + OUTPUT_BUFFER_SIZE * 2;
	mp3_bitrate = 0;
	mp3_sample = 0;
	mp3_length = 0;
	eos = false;

	return true;
}

static int mp3_thread(unsigned int args, void * argp)
{
	while(isPlaying && (!eos || mp3_load()))
	{
		while(isPause)
			sceKernelDelayThread(500000);
		if (Stream.buffer == NULL || Stream.error == MAD_ERROR_BUFLEN)
		{
			size_t ReadSize, Remaining = 0, BufSize;
			byte * ReadStart;
			if(Stream.next_frame != NULL)
			{
				Remaining = Stream.bufend - Stream.next_frame;
				memmove(InputBuffer, Stream.next_frame, Remaining);
				ReadStart = InputBuffer + Remaining;
				ReadSize = INPUT_BUFFER_SIZE - Remaining;
			}
			else
			{
				ReadSize = INPUT_BUFFER_SIZE;
				ReadStart = InputBuffer;
				Remaining = 0;
			}
			BufSize = sceIoRead(fd, ReadStart, ReadSize);
			if(BufSize == 0)
			{
				eos = true;
				continue;
			}
			if(BufSize < ReadSize)
			{
				GuardPtr = ReadStart + ReadSize;
				memset(GuardPtr, 0, MAD_BUFFER_GUARD);
				ReadSize += MAD_BUFFER_GUARD;
			}
			mad_stream_buffer(&Stream, InputBuffer, ReadSize + Remaining);
			Stream.error = 0;
		}

		if (mad_frame_decode(&Frame, &Stream) == -1)
		{
			if(MAD_RECOVERABLE(Stream.error) || Stream.error == MAD_ERROR_BUFLEN)
				continue;
			else
			{							
				eos = true;
				continue;
			}
		}
		if(mp3_bitrate == 0)
		{
			mp3_bitrate = Frame.header.bitrate;
			mp3_sample = Frame.header.samplerate;
			if(mp3_bitrate != 0)
				mp3_length = mp3_filesize * 8 / mp3_bitrate;
		}

		mad_timer_add(&Timer, Frame.header.duration);

		if (DoFilter)
			apply_filter(&Frame);

		mad_synth_frame(&Synth, &Frame);

		if(Synth.pcm.samplerate == 44100)
		{
			OutputPCM.nsamples = Synth.pcm.length;
			OutputPCM.samples[0] = Synth.pcm.samples[0];
			OutputPCM.samples[1] = Synth.pcm.samples[1];
		}
		else
		{
			if (resample_init(&Resample[0], Synth.pcm.samplerate, 44100) == -1 ||
				resample_init(&Resample[1], Synth.pcm.samplerate, 44100) == -1)
				continue;
			OutputPCM.nsamples = resample_block(&Resample[0], Synth.pcm.length, Synth.pcm.samples[0], (*Resampled)[0]);
			resample_block(&Resample[1], Synth.pcm.length, Synth.pcm.samples[1], (*Resampled)[1]);

			OutputPCM.samples[0] = (*Resampled)[0];
			OutputPCM.samples[1] = (*Resampled)[1];
		}

		if(OutputPCM.nsamples > 0)
		{
			int i;
			if(MAD_NCHANNELS(&Frame.header) == 2)
				for (i = 0; i < OutputPCM.nsamples; i++)
				{
					signed short	Sample;
					/* Left channel */
					Sample = MadFixedToSshort(OutputPCM.samples[0][i]);
					*(OutputPtr++) = Sample & 0xff;
					*(OutputPtr++) = (Sample >> 8);			
					Sample = MadFixedToSshort(OutputPCM.samples[1][i]);
					*(OutputPtr++) = Sample & 0xff;
					*(OutputPtr++) = (Sample >> 8);

					if(OutputPtr == OutputBufferEnd)
					{								
						sceAudioOutputPannedBlocking(mp3_handle, MAXVOLUME, MAXVOLUME, (char *)OutputBuffer[OutputBuffer_Index]);
						OutputBuffer_Index = (OutputBuffer_Index + 1) & 0x03;
						OutputPtr = (byte *)&OutputBuffer[OutputBuffer_Index][0];
						OutputBufferEnd = OutputPtr + OUTPUT_BUFFER_SIZE;
					}
				}
			else
				for (i = 0; i < OutputPCM.nsamples; i++)
				{
					signed short	Sample;
					/* Left channel */
					Sample = MadFixedToSshort(OutputPCM.samples[0][i]);
					*(OutputPtr++) = Sample & 0xff;
					*(OutputPtr++) = (Sample >> 8);			
					*(OutputPtr++) = Sample & 0xff;
					*(OutputPtr++) = (Sample >> 8);

					if(OutputPtr >= OutputBufferEnd)
					{
						sceAudioOutputPannedBlocking(mp3_handle, MAXVOLUME, MAXVOLUME, (char *)OutputBuffer[OutputBuffer_Index]);
						OutputBuffer_Index = (OutputBuffer_Index + 1) & 0x03;
						OutputPtr = (byte *)&OutputBuffer[OutputBuffer_Index][0];
						OutputBufferEnd = OutputPtr + OUTPUT_BUFFER_SIZE;
					}
				}
		}
		else
			eos = 1;
		while(isPause)
			sceKernelDelayThread(500000);
	}
	return 0;
}

extern bool mp3_init()
{
	isPlaying = true;
	isPause = true;
	eos = true;
	mp3_direct = -1;
	Resampled = (mad_fixed_t (*)[2][MAX_NSAMPLES])malloc(sizeof(*Resampled));
	mp3_handle = sceAudioChReserve( PSP_AUDIO_NEXT_CHANNEL, OUTPUT_BUFFER_SIZE / 4, 0 );
	mp3_thid = sceKernelCreateThread( "mp3 thread", mp3_thread, 0x8, 0x40000, 0, NULL );
	if(mp3_thid < 0)
		return false;
	return true;
}


static void mp3_freetune()
{
	mad_synth_finish(&Synth);
	mad_frame_finish(&Frame);
	mad_stream_finish(&Stream);

	free((void *)Resampled);
}

extern void mp3_start()
{
	mp3_direct = -1;
	eos = true;
	isPlaying = true;
	sceKernelStartThread(mp3_thid, 0, NULL);
	sceKernelWakeupThread(mp3_thid);
	sceKernelResumeThread(mp3_thid);
}

extern void mp3_end()
{
	mp3_stop();
	if(fd >= 0)
		sceIoClose(fd);
	mp3_freetune();
	sceAudioChRelease(mp3_handle);
}

extern void mp3_pause()
{
	isPause = true;
}

extern void mp3_resume()
{
	isPause = false;
}

extern void mp3_stop()
{
	isPlaying = false;
	isPause = false;
	eos = true;
	mp3_direct = -1;
	sceKernelWakeupThread(mp3_thid);
	sceKernelResumeThread(mp3_thid);
	sceKernelWaitThreadEnd(mp3_thid, NULL);
}

static bool lastpause;
static dword lastpos, lastindex;

extern void mp3_powerdown()
{
	lastpause = isPause;
	isPause = true;
	lastpos = sceIoLseek32(fd, 0, PSP_SEEK_CUR);
	lastindex = mp3_index;
	if(fd >= 0)
		sceIoClose(fd);
}

extern void mp3_powerup()
{
	mp3_index = lastindex;
	if((fd = sceIoOpen(mp3_files[mp3_index], PSP_O_RDONLY, 0777)) < 0)
		return;
	sceIoLseek32(fd, lastpos, PSP_SEEK_SET);
	isPause = lastpause;
}

extern void mp3_list(const char *path) {
	int dl = sceIoDopen(path);
	if(dl < 0)
		return;
	SceIoDirent sid;
	while(mp3_nfiles < MP3_MAX_ENTRY)
	{
		memset(&sid, 0, sizeof(SceIoDirent));
		if(sceIoDread(dl, &sid) <= 0)
			break;
		if(sid.d_name[0] == '.') continue; // hide file
		if(FIO_S_ISDIR(sid.d_stat.st_mode)) // dir
		{
			char subPath[260];
			sprintf(subPath, "%s%s/", path, sid.d_name);
			mp3_list(subPath);
			continue;
		}
		const char * ext = utils_fileext(sid.d_name);
		if(ext != NULL && stricmp(ext, "mp3") == 0)
		{
			sprintf(mp3_files[mp3_nfiles], "%s%s", path, sid.d_name);
			mp3_nfiles++;
		}
	}
	sceIoDclose(dl);
}

extern void mp3_prev()
{
	mp3_direct = 0;
	if(isPause)
		mp3_load();
	else
		eos = true;
	isPlaying = true;
}

extern void mp3_next()
{
	mp3_direct = 1;
	if(isPause)
		mp3_load();
	else
		eos = true;
	isPlaying = true;
}

extern void mp3_restart()
{
	eos = true;
	mp3_direct = -1;
	isPlaying = true;
	mad_timer_reset(&Timer);
}

extern bool mp3_paused()
{
	return isPause;
}

extern char * mp3_get_tag()
{
	return mp3_tag;
}

extern bool mp3_get_info(int * bitrate, int * sample, int * curlength, int * totallength)
{
	if(mp3_bitrate == 0)
		return false;
	* bitrate = mp3_bitrate;
	* sample = mp3_sample;
	* curlength = Timer.seconds;
	* totallength = mp3_length;
	return true;
}

#include "mdaDelay.h"

#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
	return new mdaDelay(audioMaster);
}

mdaDelay::mdaDelay(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, 1, 6)	// programs, parameters
{
	//inits here!
	left_delay = 0.50f; //left delay
	fParam1 = 0.27f; //right ratio
	fParam2 = 0.70f; //feedback
	fParam3 = 0.50f; //tone
	fParam4 = 0.33f; //wet mix
	fParam5 = 0.50f; //output

	max_delay_size = 32766;  //set max delay time at max sample rate
	buffer = new float[max_delay_size + 2]; //spare just in case!
	ipos = 0;
	fil0 = 0.0f;

	setNumInputs(2);
	setNumOutputs(2);
	setUniqueID('mday');  //identify here
	DECLARE_VST_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Delay");

	suspend();		//flush buffer
	setParameter(0, 0.5);
}

bool  mdaDelay::getProductString(char* text) { strcpy(text, "mda Delay"); return true; }
bool  mdaDelay::getVendorString(char* text) { strcpy(text, "mda"); return true; }
bool  mdaDelay::getEffectName(char* name) { strcpy(name, "Delay"); return true; }

void mdaDelay::setParameter(VstInt32 index, float value)
{
	float tmp;

	switch (index)
	{
	case 0: left_delay = value; break;
	case 1: fParam1 = value; break;
	case 2: fParam2 = value; break;
	case 3: fParam3 = value; break;
	case 4: fParam4 = value; break;
	case 5: fParam5 = value; break;
	}

	//calcs here
	ldel = (VstInt32)(max_delay_size * left_delay * left_delay);
	if (ldel < 4) ldel = 4;

	switch (int(fParam1 * 17.9f)) //fixed left/right ratios
	{
	case  17: tmp = 0.5000f; break;
	case  16: tmp = 0.6667f; break;
	case  15: tmp = 0.7500f; break;
	case  14: tmp = 0.8333f; break;
	case  13: tmp = 1.0000f; break;
	case  12: tmp = 1.2000f; break;
	case  11: tmp = 1.3333f; break;
	case  10: tmp = 1.5000f; break;
	case   9: tmp = 2.0000f; break;
	default: tmp = 4.0f * fParam1; break; //variable ratio
	}

	rdel = (VstInt32)(max_delay_size * left_delay * left_delay * tmp);
	if (rdel > max_delay_size) rdel = max_delay_size;
	if (rdel < 4) rdel = 4;

	fil = fParam3;

	if (fParam3 > 0.5f)  //simultaneously change crossover frequency & high/low mix
	{
		fil = 0.5f * fil - 0.25f;
		lmix = -2.0f * fil;
		hmix = 1.0f;
	}
	else
	{
		hmix = 2.0f * fil;
		lmix = 1.0f - hmix;
	}
	fil = (float)exp(-6.2831853f * pow(10.0f, 2.2f + 4.5f * fil) / getSampleRate());

	fbk = 0.495f * fParam2;
	wet = 1.0f - fParam4;
	wet = fParam5 * (1.0f - wet * wet); //-3dB at 50% mix
	dry = fParam5 * 2.0f * (1.0f - fParam4 * fParam4);

	//if(fParam2>0.99) { fbk=0.5f; wet=0.0f; } //freeze
}

mdaDelay::~mdaDelay()
{
	if (buffer) delete[] buffer;
}

void mdaDelay::suspend()
{
	memset(buffer, 0, max_delay_size * sizeof(float));
}

void mdaDelay::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaDelay::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaDelay::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* name)
{
	if (index == 0)
	{
		strcpy(name, programName);
		return true;
	}
	return false;
}

float mdaDelay::getParameter(VstInt32 index)
{
	float v = 0;

	switch (index)
	{
	case 0: v = left_delay; break;
	case 1: v = fParam1; break;
	case 2: v = fParam2; break;
	case 3: v = fParam3; break;
	case 4: v = fParam4; break;
	case 5: v = fParam5; break;
	}
	return v;
}

void mdaDelay::getParameterName(VstInt32 index, char *label)
{
	switch (index)
	{
	case 0: strcpy(label, "L Delay "); break;
	case 1: strcpy(label, "R Delay "); break;
	case 2: strcpy(label, "Feedback"); break;
	case 3: strcpy(label, "Fb Tone "); break;
	case 4: strcpy(label, "FX Mix  "); break;
	case 5: strcpy(label, "Output  "); break;
	}
}

#include <stdio.h>
void int2strng(VstInt32 value, char *string) { sprintf(string, "%d", value); }

void mdaDelay::getParameterDisplay(VstInt32 index, char *text)
{
	switch (index)
	{
	case 0: int2strng((VstInt32)(ldel * 1000.0f / getSampleRate()), text); break;
	case 1: int2strng((VstInt32)(100 * rdel / ldel), text); break;
	case 2: int2strng((VstInt32)(99 * fParam2), text); break;
	case 3: int2strng((VstInt32)(200 * fParam3 - 100), text); break;
	case 4: int2strng((VstInt32)(100 * fParam4), text); break;
	case 5: int2strng((VstInt32)(20 * log10(2.0 * fParam5)), text); break;
	}
}

void mdaDelay::getParameterLabel(VstInt32 index, char *label)
{
	switch (index)
	{
	case 0:  strcpy(label, "ms"); break;
	case 3:  strcpy(label, "Lo <> Hi"); break;
	case 5:  strcpy(label, "dB"); break;
	default: strcpy(label, "%"); break;
	}
}

//--------------------------------------------------------------------------------
// Processes inputs (stereo) with a given number of sampleFrames samples, and outputs it to outputs (stereo)
void mdaDelay::process(float **inputs, float **outputs, VstInt32 sampleFrames)
{
	// Input and outputs (stereo)
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];

	// Stores the current input in the loop
	float current_input1, current_input2;

	// Stores the current output in the loop
	float current_output1, current_output2;

	// ??
	float ol, or_;

	// Variables to store the current parameters of the plugin
	float wet_ratio = wet, dry_ratio = dry, feedback = fbk;

	float low_mix = lmix, high_mix = hmix;

	float f = fil, f0 = fil0, tmp;

	VstInt32 i = ipos, l, r;

	// Circular rotation in buffer
	l = (i + ldel) % (max_delay_size + 1);
	r = (i + rdel) % (max_delay_size + 1);

	// Return pointer back, trick so the ++in1 works in the loop
	--in1;
	--in2;
	--out1;
	--out2;

	// For all the samples in the current frame
	while (--sampleFrames >= 0)
	{

		// Get the current sample, increment the pointer of the inputs
		current_input1 = *++in1;
		current_input2 = *++in2;

		current_output1 = out1[1];
		current_output2 = out2[1];

		ol = *(buffer + l);
		or_ = *(buffer + r);

		// Mix input and feedback
		tmp = wet_ratio * (current_input1 + current_input2) + feedback * (ol + or_);

		// Low pass filter
		f0 = f * (f0 - tmp) + tmp;

		// Delay input
		*(buffer + i) = low_mix * f0 + high_mix * tmp;

		i--; if (i < 0) i = max_delay_size;
		l--; if (l < 0) l = max_delay_size;
		r--; if (r < 0) r = max_delay_size;

		// Mix wet and dry
		*++out1 = current_output1 + dry_ratio * current_input1 + ol;
		*++out2 = current_output2 + dry_ratio * current_input2 + or_;
	}
	ipos = i;
	if (fabs(f0) < 1.0e-10) fil0 = 0.0f; else fil0 = f0;
}





void mdaDelay::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, ol, or_, w = wet, y = dry, fb = fbk;
	float lx = lmix, hx = hmix, f = fil, f0 = fil0, tmp;
	VstInt32 i = ipos, l, r, s = max_delay_size;

	l = (i + ldel) % (s + 1);
	r = (i + rdel) % (s + 1);

	--in1;
	--in2;
	--out1;
	--out2;
	while (--sampleFrames >= 0)
	{
		a = *++in1;
		b = *++in2;

		ol = *(buffer + l); //delay outputs
		or_ = *(buffer + r);

		tmp = w * (a + b) + fb * (ol + or_); //mix input & feedback
		f0 = f * (f0 - tmp) + tmp;    //low-pass filter
		*(buffer + i) = lx * f0 + hx * tmp; //delay input

		i--; if (i < 0) i = s;
		l--; if (l < 0) l = s;
		r--; if (r < 0) r = s;

		*++out1 = y * a + ol; //mix wet & dry
		*++out2 = y * b + or_;
	}
	ipos = i;
	if (fabs(f0) < 1.0e-10) fil0 = 0.0f; else fil0 = f0; //trap denormals
}

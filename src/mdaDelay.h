#ifndef __mdaDelay_H
#define __mdaDelay_H

#include "audioeffectx.h"

class mdaDelay : public AudioEffectX
{
public:
	mdaDelay(audioMasterCallback audioMaster);
	~mdaDelay();

	virtual void process(float **inputs, float **outputs, VstInt32 sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* name);
	virtual void setParameter(VstInt32 index, float value);
	virtual float getParameter(VstInt32 index);
	virtual void getParameterLabel(VstInt32 index, char *label);
	virtual void getParameterDisplay(VstInt32 index, char *text);
	virtual void getParameterName(VstInt32 index, char *text);
	virtual void suspend();

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual VstInt32 getVendorVersion() { return 1000; }

protected:
	float left_delay;
	float fParam1;
	float fParam2;
	float fParam3;
	float fParam4;
	float fParam5;
	float fParam6;

	float *buffer; // Delay buffer
	VstInt32 max_delay_size; // Delay maximum time
	VstInt32 ipos; // Delay pointer
	VstInt32 ldel, rdel; // Delay time for left and right

	float wet, dry, fbk; // Wet and dry mix, feedback
	float lmix, hmix; // low & high mix 
	float fil, fil0;  // crossover filter coeff & buffer

	char programName[32];
};

#endif

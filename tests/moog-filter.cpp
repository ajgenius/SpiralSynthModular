#include "MoogFilterPlugin.h"
using Spiral::Core::Property;
#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

// Frozen copy of the pre-change Moog Execute() body on this branch,
// including the in==0 state wipe.
static const int GRANULARITY = 10;

struct LegacyRef
{
	float Cutoff, Resonance;
	float fc, f, p, q;
	float b0, b1, b2, b3, b4;
	float t1, t2;

	LegacyRef() :
		Cutoff(0.5f), Resonance(0.0f),
		fc(0.0f), f(0.0f), p(0.0f), q(0.0f),
		b0(0.1f), b1(0.0f), b2(0.0f), b3(0.0f), b4(0.0f),
		t1(0.0f), t2(0.0f)
	{}

	void Clear()
	{
		b0 = b1 = b2 = b3 = b4 = 0.0f;
		t1 = t2 = 0.0f;
	}

	void Process(int bufsize, const float *in, const float *cvCut, const float *cvRes,
	             float *lp, float *bp, float *hp)
	{
		for (int n=0; n<bufsize; n++)
		{
			if (n%GRANULARITY==0)
			{
				fc = (Cutoff+(cvCut?cvCut[n]:0.0f))*0.25f;
				if (fc<0) fc=0;
				q = 1.0f - fc;
				p = fc + 0.8f * fc * q;
				f = p + p - 1.0f;
				float Q = ((Resonance+(cvRes?cvRes[n]:0.0f))*6)-3.0f;
				q = Q + (1.0f + 0.5f * q * (1.0f - q + 5.6f * q * q));
			}

			float x = in[n];
			if (x == 0)
			{
				Clear();
				lp[n]=0; bp[n]=0; hp[n]=0;
			}
			else
			{
				x -= q * b4;
				if (x>1) x=1;
				if (x<-1) x=-1;
				t1 = b1; b1 = (x + b0) * p - b1 * f;
				t2 = b2; b2 = (b1 + t1) * p - b2 * f;
				t1 = b3; b3 = (b2 + t2) * p - b3 * f;
				     b4 = (b3 + t1) * p - b4 * f;
				b4 = b4 - b4 * b4 * b4 * 0.166667f;
				b0 = x;
				lp[n]=b4;
				bp[n]=(x-b4);
				hp[n]=3.0f * (b3 - b4);
			}
		}
	}
};

static int g_fails = 0;

static void Fail(const char *msg)
{
	std::fprintf(stderr, "FAIL: %s\n", msg);
	g_fails++;
}

static void ControlFloat(MoogFilterPlugin &p, const char *id, float v)
{
	p.GetChannelHandler()->Set(id, v);
	p.UpdateChannelHandler();
}

static void ControlMode(MoogFilterPlugin &p, unsigned mode)
{
	int m = (int)mode;
	p.GetChannelHandler()->Set("Mode", m);
	p.UpdateChannelHandler();
}

static void FillSine(Sample &s, int n, float sr, float hz, float amp)
{
	for (int i=0; i<n; i++)
		s.Set(i, amp * sinf(2.0f * 3.141592654f * hz * (float)i / sr));
}

static int CountNonFinite(Sample *s, int n)
{
	int c=0;
	for (int i=0; i<n; i++)
		if (!std::isfinite((*s)[i])) c++;
	return c;
}

static float MaxAbs(Sample *s, int n)
{
	float m=0;
	for (int i=0; i<n; i++)
	{
		float a=fabsf((*s)[i]);
		if (a>m) m=a;
	}
	return m;
}

static void TestMode0BitExact()
{
	const int SR=44100, BS=64;
	HostInfo host = HostInfo();
	host.BUFSIZE = BS;
	host.SAMPLERATE = SR;

	MoogFilterPlugin plugin;
	plugin.Initialise(&host);
	if (plugin.GetMode() != 0) Fail("new instance default Mode is not 0");

	Sample in(BS), cvC(BS), cvR(BS);
	assert(plugin.SetInput(0, &in));
	assert(plugin.SetInput(1, &cvC));
	assert(plugin.SetInput(2, &cvR));

	struct Case { float cut, res; int useCV; const char *name; } cases[] = {
		{0.5f, 0.0f, 0, "mid/no-res"},
		{0.5f, 0.8f, 0, "mid/high-res"},
		{0.0f, 0.0f, 0, "cut0"},
		{1.0f, 0.3f, 0, "cut1"},
		{0.2f, 0.5f, 1, "with-cv"},
	};

	double maxErr = 0.0;
	int compared = 0;
	for (unsigned c=0; c<sizeof(cases)/sizeof(cases[0]); c++)
	{
		plugin.Reset();
		ControlFloat(plugin, "Cutoff", cases[c].cut);
		ControlFloat(plugin, "Resonance", cases[c].res);
		ControlMode(plugin, 0);
		if (plugin.GetMode() != 0) Fail("Mode 0 did not stick");

		LegacyRef ref;
		ref.Cutoff = cases[c].cut;
		ref.Resonance = cases[c].res;
		ref.Clear();

		for (int block=0; block<32; block++)
		{
			if (block < 16)
				FillSine(in, BS, (float)SR, 220.0f + 40.0f*block, 0.7f);
			else
				for (int i=0; i<BS; i++) in.Set(i, 0.0f);
			if (cases[c].useCV)
			{
				for (int i=0; i<BS; i++)
				{
					cvC.Set(i, 0.05f * sinf(0.01f * (block*BS+i)));
					cvR.Set(i, 0.02f);
				}
			}
			else
			{
				for (int i=0; i<BS; i++) { cvC.Set(i, 0.0f); cvR.Set(i, 0.0f); }
			}

			plugin.Execute();
			std::vector<float> lp(BS), bp(BS), hp(BS), inv(BS), ic(BS), ir(BS);
			for (int i=0; i<BS; i++)
			{
				inv[i]=in[i]; ic[i]=cvC[i]; ir[i]=cvR[i];
			}
			ref.Process(BS, &inv[0], &ic[0], &ir[0], &lp[0], &bp[0], &hp[0]);

			Sample *ol=NULL, *ob=NULL, *oh=NULL;
			assert(plugin.GetOutput(0, &ol));
			assert(plugin.GetOutput(1, &ob));
			assert(plugin.GetOutput(2, &oh));
			for (int i=0; i<BS; i++)
			{
				float e0=fabsf((*ol)[i]-lp[i]);
				float e1=fabsf((*ob)[i]-bp[i]);
				float e2=fabsf((*oh)[i]-hp[i]);
				if (e0>maxErr) maxErr=e0;
				if (e1>maxErr) maxErr=e1;
				if (e2>maxErr) maxErr=e2;
				compared++;
				if (e0!=0.0f || e1!=0.0f || e2!=0.0f)
				{
					if (g_fails < 8)
						std::fprintf(stderr, "FAIL: Mode 0 mismatch case %s block %d n %d lp %g vs %g\n",
							cases[c].name, block, i, (*ol)[i], lp[i]);
					g_fails++;
				}
			}
		}
	}
	std::printf("Mode 0 null test: %d samples compared, max abs error = %.10g (%s)\n",
		compared, maxErr, maxErr==0.0 ? "BIT-EXACT" : "NOT exact");
}

static void TestProperties()
{
	HostInfo host = HostInfo();
	host.BUFSIZE = 8;
	host.SAMPLERATE = 44100;

	MoogFilterPlugin fresh;
	fresh.Initialise(&host);
	if (fresh.GetMode()!=0 || fresh.GetCutoff()!=0.5f || fresh.GetResonance()!=0.0f)
		Fail("default property values");

	ControlFloat(fresh, "Cutoff", 0.25f);
	ControlFloat(fresh, "Resonance", 0.75f);
	std::map<std::string, Property *> oldMap;
	fresh.DuplicateProperties(oldMap);
	oldMap.erase("MODE");

	MoogFilterPlugin fromOld;
	fromOld.Initialise(&host);
	assert(fromOld.LoadProperties(oldMap));
	if (fromOld.GetCutoff()!=0.25f || fromOld.GetResonance()!=0.75f)
		Fail("v1 property map did not restore cutoff/resonance");
	if (fromOld.GetMode()!=0)
		Fail("new instance loaded from a map without MODE must stay Mode 0");

	MoogFilterPlugin src;
	src.Initialise(&host);
	ControlFloat(src, "Cutoff", 0.4f);
	ControlFloat(src, "Resonance", 0.6f);
	ControlMode(src, 1);
	MoogFilterPlugin dst;
	dst.Initialise(&host);
	assert(dst.CopyDevice(&src));
	if (dst.GetMode()!=1 || dst.GetCutoff()!=0.4f || dst.GetResonance()!=0.6f)
		Fail("CopyDevice did not round-trip Mode 1");

	for (std::map<std::string, Property *>::iterator it=oldMap.begin(); it!=oldMap.end(); ++it)
		delete it->second;

	std::printf("Property test: v1 load Mode=%u (expect 0), CopyDevice Mode=%u\n",
		fromOld.GetMode(), dst.GetMode());
}

static float MappedHz(float cut, float sr)
{
	if (cut<0) cut=0;
	if (cut>1) cut=1;
	return 20.0f * powf((0.49f * sr)/20.0f, cut);
}

static void TestMode1Sanity()
{
	const int SR=44100, BS=64;
	HostInfo host = HostInfo();
	host.BUFSIZE = BS;
	host.SAMPLERATE = SR;

	MoogFilterPlugin plugin;
	plugin.Initialise(&host);
	ControlMode(plugin, 1);
	if (plugin.GetMode()!=1) Fail("Mode 1 did not stick");

	Sample in(BS);
	assert(plugin.SetInput(0, &in));

	ControlFloat(plugin, "Cutoff", 1.0f);
	ControlFloat(plugin, "Resonance", 1.0f);
	plugin.Reset();
	ControlMode(plugin, 1);
	ControlFloat(plugin, "Cutoff", 1.0f);
	ControlFloat(plugin, "Resonance", 1.0f);
	int nanCount=0;
	float peak=0;
	for (int block=0; block<200; block++)
	{
		FillSine(in, BS, (float)SR, 1000.0f, 0.9f);
		plugin.Execute();
		Sample *ol=NULL, *ob=NULL, *oh=NULL;
		assert(plugin.GetOutput(0, &ol));
		assert(plugin.GetOutput(1, &ob));
		assert(plugin.GetOutput(2, &oh));
		nanCount += CountNonFinite(ol, BS)+CountNonFinite(ob, BS)+CountNonFinite(oh, BS);
		float m=MaxAbs(ol, BS);
		if (m>peak) peak=m;
	}
	std::printf("Mode 1 max-res/high-cut driven: peak LP = %.6g, nonfinite = %d\n", peak, nanCount);
	if (nanCount) Fail("Mode 1 produced NaN/Inf at max resonance / high cutoff");
	if (peak > 20.0f) Fail("Mode 1 peak exploded at max resonance / high cutoff");

	const float cut = 0.55f;
	const float expectHz = MappedHz(cut, (float)SR);
	plugin.Reset();
	ControlMode(plugin, 1);
	ControlFloat(plugin, "Cutoff", cut);
	ControlFloat(plugin, "Resonance", 1.0f);

	const int total = SR * 2;
	std::vector<float> rec;
	rec.reserve(total);
	int produced = 0;
	while (produced < total)
	{
		for (int i=0; i<BS; i++)
		{
			int idx = produced + i;
			in.Set(i, (idx==0) ? 0.01f : 0.0f);
		}
		plugin.Execute();
		Sample *ol=NULL;
		assert(plugin.GetOutput(0, &ol));
		nanCount += CountNonFinite(ol, BS);
		for (int i=0; i<BS && produced<total; i++, produced++)
			rec.push_back((*ol)[i]);
	}

	const int start = SR/2;
	int crosses=0;
	for (int i=start+1; i<(int)rec.size(); i++)
	{
		if ((rec[i-1] < 0.0f && rec[i] >= 0.0f) || (rec[i-1] > 0.0f && rec[i] <= 0.0f))
			crosses++;
	}
	float dur = (float)((int)rec.size()-start) / (float)SR;
	float measHz = (dur>0) ? (0.5f * crosses / dur) : 0.0f;
	float latePeak=0;
	int lateNans=0;
	for (int i=start; i<(int)rec.size(); i++)
	{
		if (!std::isfinite(rec[i])) lateNans++;
		float a=fabsf(rec[i]);
		if (a>latePeak) latePeak=a;
	}
	std::printf("Mode 1 self-osc: mapped cutoff = %.3f Hz, measured = %.3f Hz, late peak = %.6g, late nonfinite = %d\n",
		expectHz, measHz, latePeak, lateNans);
	if (lateNans) Fail("Mode 1 self-osc produced NaN/Inf");
	if (latePeak < 0.05f) Fail("Mode 1 did not self-oscillate at max resonance");
	if (latePeak > 20.0f) Fail("Mode 1 self-osc peak exploded");
	float rel = (expectHz>0) ? fabsf(measHz-expectHz)/expectHz : 1;
	std::printf("Mode 1 self-osc relative freq error = %.2f%%\n", rel*100.0f);
	if (rel > 0.25f) Fail("Mode 1 self-osc frequency is far from the mapped cutoff");

	plugin.Reset();
	ControlMode(plugin, 0);
	ControlFloat(plugin, "Cutoff", 0.5f);
	ControlFloat(plugin, "Resonance", 0.9f);
	FillSine(in, BS, (float)SR, 220.0f, 0.8f);
	plugin.Execute();
	for (int i=0; i<BS; i++) in.Set(i, 0.0f);
	plugin.Execute();
	Sample *ol=NULL;
	assert(plugin.GetOutput(0, &ol));
	float m0sil = MaxAbs(ol, BS);
	std::printf("Mode 0 silence wipe peak after zero input = %.10g\n", m0sil);
	if (m0sil != 0.0f) Fail("Mode 0 silence wipe was removed");

	plugin.Reset();
	ControlMode(plugin, 1);
	ControlFloat(plugin, "Cutoff", cut);
	ControlFloat(plugin, "Resonance", 1.0f);
	in.Set(0, 0.01f);
	for (int i=1; i<BS; i++) in.Set(i, 0.0f);
	plugin.Execute();
	for (int i=0; i<BS; i++) in.Set(i, 0.0f);
	plugin.Execute();
	assert(plugin.GetOutput(0, &ol));
	float m1sil = MaxAbs(ol, BS);
	std::printf("Mode 1 residual peak on silent input (no wipe) = %.6g\n", m1sil);
	if (m1sil == 0.0f) Fail("Mode 1 appears to wipe state on silence");
}

int main()
{
	TestMode0BitExact();
	TestProperties();
	TestMode1Sanity();
	if (g_fails)
	{
		std::fprintf(stderr, "%d check(s) failed\n", g_fails);
		return 1;
	}
	std::printf("All Moog prototype checks passed.\n");
	return 0;
}

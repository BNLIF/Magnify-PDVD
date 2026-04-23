#ifndef RMS_ANALYZER_H
#define RMS_ANALYZER_H

#include <vector>
#include "TString.h"

class TH2F;

struct ChannelRms {
    int     channel;
    float   rms_prelim;    // RMS before signal flagging
    float   rms_final;     // RMS after signal regions are masked
    int     nSignalBins;   // number of bins flagged as signal
    TString srcHist;       // name of source TH2F histogram
};

// Per-channel noise RMS using the WCT percentile-based algorithm:
//   Step 1: CalcRMSWithFlags  — preliminary RMS on unflagged samples (< 4096)
//   Step 2: SignalFilter      — mark |ADC| > 4*rms bins, pad ±8, flag with +20000
//   Step 3: CalcRMSWithFlags  — final RMS skipping all flagged samples
// Algorithm matches Microboone.cxx:549 (CalcRMSWithFlags) and :573 (SignalFilter).
class RmsAnalyzer {
public:
    static std::vector<ChannelRms> AnalyzePlane(TH2F* h);

    static void Save(const std::vector<ChannelRms>& u,
                     const std::vector<ChannelRms>& v,
                     const std::vector<ChannelRms>& w,
                     const char* outFile);

    static bool Load(const char* inFile,
                     std::vector<ChannelRms>& u,
                     std::vector<ChannelRms>& v,
                     std::vector<ChannelRms>& w);

    // Returns "<magnifyFile>.rms.root"
    static TString CacheFilename(const char* magnifyFile);

    // Convenience batch entry point: open inFile, discover histograms, run all
    // three planes, and write the result to CacheFilename(inFile).
    static void AnalyzeFile(const char* inFile);
};

#endif

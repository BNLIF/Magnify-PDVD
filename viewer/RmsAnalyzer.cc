#include "RmsAnalyzer.h"

#include "TH2F.h"
#include "TFile.h"
#include "TTree.h"
#include "TSystem.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace std;

// ---- internal helpers (not exposed in header) ----

// Percentile-based RMS on unflagged samples (< 4096).
// Mirrors WCT CalcRMSWithFlags (Microboone.cxx:549):
//   p16/p50/p84 via nth_element, then sqrt(((p84-p50)^2 + (p50-p16)^2) / 2).
static float calcRmsWithFlags(const vector<float>& sig)
{
    vector<float> temp;
    temp.reserve(sig.size());
    for (float s : sig) {
        if (s < 4096.f) temp.push_back(s);
    }
    if ((int)temp.size() < 3) return 0.f;

    size_t n   = temp.size();
    size_t i16 = (size_t)(0.16 * n);
    size_t i50 = (size_t)(0.50 * n);
    size_t i84 = (size_t)(0.84 * n);
    if (i84 >= n) i84 = n - 1;
    if (i50 >= n) i50 = n - 1;

    // nth_element rearranges in place; run from highest index down to preserve earlier results
    nth_element(temp.begin(), temp.begin() + i84, temp.end());
    float p84 = temp[i84];
    nth_element(temp.begin(), temp.begin() + i50, temp.begin() + i84);
    float p50 = temp[i50];
    nth_element(temp.begin(), temp.begin() + i16, temp.begin() + i50);
    float p16 = temp[i16];

    return sqrtf(((p84 - p50) * (p84 - p50) + (p50 - p16) * (p50 - p16)) / 2.f);
}

// Mark signal regions and flag them with +20000.
// Mirrors WCT SignalFilter (Microboone.cxx:573):
//   sigFactor=4.0, padBins=8; condition: |ADC| > 4*rms AND ADC < 4096.
// Returns the number of bins flagged.
static int signalFilter(vector<float>& sig, float rms0)
{
    const float sigFactor = 4.0f;
    const int   padBins   = 8;
    int n = (int)sig.size();
    int nFlagged = 0;

    vector<bool> mark(n, false);
    for (int i = 0; i < n; ++i) {
        if (sig[i] < 4096.f && fabsf(sig[i]) > sigFactor * rms0)
            mark[i] = true;
    }
    for (int i = 0; i < n; ++i) {
        if (!mark[i]) continue;
        int lo = (i - padBins > 0)     ? i - padBins : 0;
        int hi = (i + padBins < n - 1) ? i + padBins : n - 1;
        for (int j = lo; j <= hi; ++j) {
            if (sig[j] < 4096.f) {
                sig[j] += 20000.f;
                ++nFlagged;
            }
        }
    }
    return nFlagged;
}

// ---- public API ----

vector<ChannelRms> RmsAnalyzer::AnalyzePlane(TH2F* h)
{
    vector<ChannelRms> result;
    if (!h) return result;

    int nCh = h->GetNbinsX();
    int nTk = h->GetNbinsY();
    int firstCh = (int)(h->GetXaxis()->GetBinCenter(1) + 0.5);
    TString srcName = h->GetName();

    result.reserve(nCh);
    for (int ix = 1; ix <= nCh; ++ix) {
        vector<float> sig(nTk);
        for (int iy = 1; iy <= nTk; ++iy)
            sig[iy - 1] = h->GetBinContent(ix, iy);

        float rms0   = calcRmsWithFlags(sig);
        int   nSig   = signalFilter(sig, rms0);
        float rmsF   = calcRmsWithFlags(sig);

        ChannelRms cr;
        cr.channel     = firstCh + (ix - 1);
        cr.rms_prelim  = rms0;
        cr.rms_final   = rmsF;
        cr.nSignalBins = nSig;
        cr.srcHist     = srcName;
        result.push_back(cr);
    }
    return result;
}

void RmsAnalyzer::Save(const vector<ChannelRms>& u,
                       const vector<ChannelRms>& v,
                       const vector<ChannelRms>& w,
                       const char* outFile)
{
    TFile* f = TFile::Open(outFile, "RECREATE");
    if (!f || f->IsZombie()) {
        printf("RmsAnalyzer::Save: cannot open %s for writing\n", outFile);
        delete f;
        return;
    }

    static const char* treeName[3] = {"rms_u", "rms_v", "rms_w"};
    const vector<ChannelRms>* planes[3] = {&u, &v, &w};

    int   channel, nSignalBins;
    float rms_prelim, rms_final;
    char  srcHist[256];

    for (int p = 0; p < 3; ++p) {
        TTree* t = new TTree(treeName[p], treeName[p]);
        t->Branch("channel",     &channel,     "channel/I");
        t->Branch("rms_prelim",  &rms_prelim,  "rms_prelim/F");
        t->Branch("rms_final",   &rms_final,   "rms_final/F");
        t->Branch("nSignalBins", &nSignalBins, "nSignalBins/I");
        t->Branch("srcHist",      srcHist,     "srcHist/C");

        for (const auto& r : *planes[p]) {
            channel     = r.channel;
            rms_prelim  = r.rms_prelim;
            rms_final   = r.rms_final;
            nSignalBins = r.nSignalBins;
            strncpy(srcHist, r.srcHist.Data(), 255);
            srcHist[255] = '\0';
            t->Fill();
        }
        t->Write();
    }
    f->Close();
    delete f;
    printf("RmsAnalyzer::Save: wrote %s\n", outFile);
}

bool RmsAnalyzer::Load(const char* inFile,
                       vector<ChannelRms>& u,
                       vector<ChannelRms>& v,
                       vector<ChannelRms>& w)
{
    if (gSystem->AccessPathName(inFile)) return false;

    TFile* f = TFile::Open(inFile, "READ");
    if (!f || f->IsZombie()) { delete f; return false; }

    static const char* treeName[3] = {"rms_u", "rms_v", "rms_w"};
    vector<ChannelRms>* planes[3]  = {&u, &v, &w};

    int   channel, nSignalBins;
    float rms_prelim, rms_final;
    char  srcHist[256];

    for (int p = 0; p < 3; ++p) {
        planes[p]->clear();
        TTree* t = (TTree*)f->Get(treeName[p]);
        if (!t) { f->Close(); delete f; return false; }

        t->SetBranchAddress("channel",     &channel);
        t->SetBranchAddress("rms_prelim",  &rms_prelim);
        t->SetBranchAddress("rms_final",   &rms_final);
        t->SetBranchAddress("nSignalBins", &nSignalBins);
        t->SetBranchAddress("srcHist",      srcHist);

        Long64_t nEntries = t->GetEntries();
        planes[p]->reserve(nEntries);
        for (Long64_t i = 0; i < nEntries; ++i) {
            t->GetEntry(i);
            ChannelRms cr;
            cr.channel     = channel;
            cr.rms_prelim  = rms_prelim;
            cr.rms_final   = rms_final;
            cr.nSignalBins = nSignalBins;
            cr.srcHist     = srcHist;
            planes[p]->push_back(cr);
        }
    }
    f->Close();
    delete f;
    return true;
}

TString RmsAnalyzer::CacheFilename(const char* magnifyFile)
{
    return TString(magnifyFile) + ".rms.root";
}

void RmsAnalyzer::AnalyzeFile(const char* inFile)
{
    TFile* f = TFile::Open(inFile, "READ");
    if (!f || f->IsZombie()) {
        printf("RmsAnalyzer::AnalyzeFile: cannot open %s\n", inFile);
        delete f;
        return;
    }

    // Discover anode suffix from Trun tree
    int anodeNo = 0;
    TTree* trun = (TTree*)f->Get("Trun");
    if (trun) {
        trun->SetBranchAddress("anodeNo", &anodeNo);
        if (trun->GetEntries() > 0) trun->GetEntry(0);
    }
    TString suf = TString::Format("%d", anodeNo);

    static const char* prefix[3]    = {"hu_", "hv_", "hw_"};
    static const char* planeName[3] = {"U",   "V",   "W"};
    static const char* tags[]       = {"raw", "decon", "gauss", "wiener", nullptr};

    vector<ChannelRms> results[3];
    for (int p = 0; p < 3; ++p) {
        TH2F* h = nullptr;
        for (int t = 0; tags[t]; ++t) {
            TString hname = TString(prefix[p]) + tags[t] + suf;
            h = (TH2F*)f->Get(hname);
            if (h) break;
        }
        if (!h) {
            printf("  plane %s: no histogram found, skipping\n", planeName[p]);
            continue;
        }
        printf("  plane %s: %s  (%d ch x %d ticks)\n",
            planeName[p], h->GetName(), h->GetNbinsX(), h->GetNbinsY());
        results[p] = AnalyzePlane(h);
    }
    f->Close();
    delete f;

    TString outFile = CacheFilename(inFile);
    Save(results[0], results[1], results[2], outFile.Data());
}

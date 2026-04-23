// Headless batch macro: compute per-channel RMS for one Magnify file and write
// <inFile>.rms.root alongside it.  Must be run from the scripts/ directory
// (same convention as magnify.sh) so that loadClasses.C can find ../event and
// ../viewer via relative paths.
//
// Usage (from Magnify_PDVD root):
//   root -b -q 'scripts/run_rms_analysis.C("input_files/040475_1/magnify-run040475-evt1-anode0.root")'

void run_rms_analysis(const char* inFile)
{
    // Load compiled classes first; all type usage dispatched through ProcessLine
    // so that RmsAnalyzer is resolved after compilation, not at macro parse time.
    gROOT->ProcessLine(".x loadClasses.C");
    TString cmd = TString::Format("RmsAnalyzer::AnalyzeFile(\"%s\")", inFile);
    gROOT->ProcessLine(cmd);
}

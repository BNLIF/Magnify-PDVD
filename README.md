# Magnify

## A magnifier to investigate raw and deconvoluted waveforms.

### Preprocess
```
./preprocess.sh /path/to/your/magnify.root
```
This is a wrapper for merging magnify histograms from all APAs. See more detailed description through "./preprocess.sh -h".


### Usage

```
cd scripts/
root -l loadClasses.C 'Magnify.C("path/to/rootfile")'
# or
root -l loadClasses.C 'Magnify.C("path/to/rootfile", <threshold>, "<frame>", <rebin>)'
```

The second argument is the default threshold for showing a box.

The third, optional argument names which output from the signal processing to display.  Likely names are:

- `decon` produced by the Wire Cell prototype (default).
- `wiener` produced by the Wire Cell toolkit, used to define ROI or "hits".
- `gauss` produced by the Wire Cell toolkit, used for charge measurement.

The call to ROOT can be be called somewhat more easily via a shell
script wrapper.  It assumes to stay in the source directory:

```
/path/to/magnify/magnify.sh /path/to/wcp-rootfile.root
# or
/path/to/magnify/magnify.sh /path/to/wct-rootfile.root 500 gauss 4
```

### Example files

An example ROOT file of waveforms can be found at twister:/home/wgu/Event27.root

If one omits the file name, a dialog will open to let user select the file:
```
cd scripts/
root -l loadClasses.C Magnify.C
```

### Per-Channel RMS Noise Analysis

Computes per-channel noise RMS for one or more Magnify ROOT files in batch (no display).  Output is written alongside each input as `<file>.rms.root`.

```
./scripts/run_rms_analysis.sh input_files/040475_1/magnify-run040475-evt1-anode0.root
# or process multiple files at once
./scripts/run_rms_analysis.sh input_files/040475_1/magnify-run040475-evt1-anode*.root
./scripts/run_rms_analysis.sh input_files/*/magnify-*.root
```

The algorithm follows the WCT percentile-based method (matching `Microboone.cxx`):
1. Preliminary RMS on unflagged ADC samples.
2. Signal flagging: mark |ADC| > 4×RMS bins, padded ±8 ticks.
3. Final RMS recomputed with signal-flagged samples excluded.

Results are stored in a `TTree` (one row per channel) inside the `.rms.root` cache file and are automatically loaded by the viewer at startup to apply per-channel Wiener thresholds.

### (Experimental feature) Channel Scan
```
./channelscan.sh path/to/rootfile
```
This is a wrapper for looping over channels where the channel list can be predefined in the `bad tree` or a text file. 

See detailed usage via `./channelscan.sh -h`.

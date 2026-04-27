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

### Event / Anode Navigation

The control window has a second row with a **Navigation** group for switching between events and anodes without restarting ROOT:

| Widget | Action |
| --- | --- |
| `anode` combo | Switch to a different anode (0–7) of the current event |
| `event` combo | Jump directly to any discovered event |
| `<` button | Previous event (same anode) |
| `>` button | Next event (same anode) |

Events are auto-discovered at startup by scanning the parent directory of the opened file for subdirectories matching `<run>_<event>` (e.g. `039324_0`, `039324_10`) that contain at least one `magnify-run<run>-evt<event>-anode*.root` file. Directories with extra suffixes (e.g. `039324_1_sel1`) are skipped. The list is sorted by `(run, event)`.

Switching tears down the active `Data`, opens the new file, and redraws all 9 pads in place; Region Sum / RMS Analysis sub-windows are hidden and their caches are reset.

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

#### Interactive RMS Analysis Panel (in-viewer)

Click **RMS Analysis** in the control window to open the RMS panel.  It supports two source modes:

| Checkbox | Source histograms | Cache file |
| --- | --- | --- |
| unchecked (default) | `hu_raw` / `hv_raw` / `hw_raw` (denoised) | `<file>.rms.root` |
| **Use original waveform** | `hu_orig` / `hv_orig` / `hw_orig` (raw ADC) | `<file>.orig.rms.root` |

Toggling the checkbox immediately loads the cache for the selected mode (status shows `[not found]` if the cache does not yet exist — press **Compute RMS** to generate it).  The two modes use separate cache files so switching back and forth is instantaneous once both caches exist.  The number of time ticks adapts automatically to whichever source histogram is selected.

#### RMS Noise Distribution window

Click **Show distribution** to open the distribution window.  It contains a 9-pad canvas and a frequency range control bar at the top.

**Channel-FFT sync:** the bottom row of the canvas shows the per-channel FFT spectrum (|F| vs. frequency in MHz).  The FFT pad updates automatically whenever the active channel changes — whether via the channel entry in the main control window, a click on a 2D wire view pad, or a click on the middle (RMS-vs-channel) pad inside the distribution window itself.

**Frequency zoom:** use the **Freq min** / **max** entries at the top of the distribution window to restrict the X axis of all three FFT pads.  Type a value and press Enter to apply; the Y axis auto-scales to the peak within the visible frequency range.  The range persists across channel changes, so you can compare the same frequency band across many channels without resetting it.

| Entry | Default | Effect |
| --- | --- | --- |
| Freq min | 0.00 MHz | Lower bound of the FFT X axis |
| Freq max | 1.00 MHz | Upper bound of the FFT X axis (Nyquist) |

### (Experimental feature) Channel Scan
```
./channelscan.sh path/to/rootfile
```
This is a wrapper for looping over channels where the channel list can be predefined in the `bad tree` or a text file. 

See detailed usage via `./channelscan.sh -h`.

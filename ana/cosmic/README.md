# ECal cosmic MIP timing at the ESA stand

Scripts for timing the ECal SMM to an asynchronous cosmic trigger and
extracting MIP amplitudes. They read the decoded CSV written by
`app/rogue-decoder.py` (or a pftool DAQ run):
`timestamp,orbit,bx,event,i_sample,i_link,channel,Tp,Tc,adc_tm1,adc,tot,toa`.

```
python3 -m venv venv
. venv/bin/activate
pip install pandas numpy matplotlib
```

## Workflow

```
# decode, ECal and HCal separately
python3 app/rogue-decoder.py run.dat --contrib ecal -o ecal.csv --n-links 12
python3 app/rogue-decoder.py run.dat --contrib hcal -o hcal.csv

# 1. pedestal, noise, common mode per channel (pedestal or cosmic run)
python3 ana/cosmic/pedestal_noise.py ecal.csv            # -> ecal_pedestals.csv

# 3. where is the cosmic pulse in the window? one CSV per L1OFFSET setting
python3 ana/cosmic/latency_scan.py run_l12.csv run_l16.csv --l1offset 12 16 \
    --pedestals ecal_pedestals.csv --plot latency.png

# 4. event time from HCal TOA
python3 ana/cosmic/hcal_phase.py hcal.csv                # -> hcal_phase.csv

# 4-5. calibrate the HCal to ECal offset, fixed-t0 fit of every channel
python3 ana/cosmic/mip_fit.py ecal.csv --pedestals ecal_pedestals.csv \
    --phase hcal_phase.csv --mip-adc 11 --plot spectrum.png
```

Without `--phase`, `mip_fit.py` does only the free fit (amplitude and time)
of seeded channels. The offset changes after resets, so calibrate it per run;
pass `--offset` to reuse a value.

## Conventions

- Sample `i` of a burst reads BX = L1A - L1OFFSET + i, so raising L1OFFSET
  by d moves the pulse d samples later in the window.
- Times are in ns from the start of sample 0, clock 26.92 ns (ESA bunch
  clock, `--clock` to change).
- The template (`hgcroc_fit.py`) is the ldmx-sw ECal emulator shape, normalized
  to 1 at the hit time; replace it with the CHARGE_TIMESCAN result.
  The fit is the same algorithm as ldmx-sw `ldmx::PulseFitter`.
- HCal TOA is taken as time from the start of its sample; check the direction
  on the bench (`--toa-reverse`) and add a time walk correction.

## Testing without data

```
python3 ana/cosmic/make_test_data.py -o test -n 1000
```
writes `test_ecal.csv`, `test_hcal.csv` and the truth (phase, hit cells) to
run the whole chain. With the defaults (1 ADC noise, MIP = 11 ADC) the
fixed-t0 fit finds 99% of the MIPs above 0.5 MIP with an unbiased amplitude.

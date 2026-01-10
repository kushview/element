# MIDI Clock Moving from DLL to PLL Cleanly

---

## Plan: DLL → PLL (tempo only)

### Step 0 — Keep your current I/O and timestamps

Don’t touch:

* MIDI parsing / queueing
* how you convert timestamps to an audio-domain “tick sample time” (or block-relative time)

Just make sure you have, per clock tick:

* `t_n` = tick arrival time in **samples** (or seconds with stable conversion)

---

## Step 1 — Add an “oscillator” state (frequency estimate)

Your DLL likely has something like “expected next tick time” and an error.

Add:

* `f` = **ticks per sample** (or its inverse `T = samples per tick`)
* `phi` = oscillator phase (in ticks or radians—ticks is easiest)

Recommended units (easy to reason about):

* `T` = **samplesPerTick** (double)
* Derived BPM:

  * `bpm = sampleRate * 60.0 / (T * 24.0)`

Initialize:

* From first few ticks: `T0 = median(diff(t_n))` over 5–9 intervals

---

## Step 2 — Keep a prediction and compute phase error the PLL way

For each tick `n`, predict when it *should* have arrived:

* `t̂_n = t̂_(n-1) + T`

Phase error (in samples):

* `e = t_n - t̂_n`

This is the same “error” your DLL probably computes, but now you’ll use it to update **T** (frequency), not just delay.

---

## Step 3 — Replace “delay correction” with a PI controller on frequency

A simple and very effective digital PLL for tempo is:

* Integrator accumulates phase error
* Frequency (period) is corrected from proportional + integral terms

Update rules (per tick):

1. (Optional but recommended) **Clamp** extreme errors to reject spikes
   `e_clamped = clamp(e, -Emax, +Emax)`
   Good starting point: `Emax = 0.5 * T` (half a tick period)

2. **Update integrator**
   `I = clamp(I + Ki * e_clamped, Imin, Imax)`

3. **Update tick period**
   `T = clamp(T + Kp * e_clamped + I, Tmin, Tmax)`

4. **Update predicted time**
   Instead of `t̂_n = t̂_n + T` blindly, do a phase pull-in:

   * `t̂_n = t_n` (hard align) during acquisition, OR
   * `t̂_n += alpha * e_clamped` (soft align) when locked

This is the key conceptual shift:

* DLL: adjust “delay/phase” to make prediction match
* PLL: adjust **period** so prediction *stays* matched

### Practical recommended structure

Use **two-mode gains**:

* **Acquire** (first ~12–24 ticks): higher Kp/Ki
* **Locked**: lower Kp/Ki (jitter rejection)

---

## Step 4 — Add a pre-filter on measured intervals (huge win)

Before feeding the PLL, improve your measurement quality:

* `d_n = t_n - t_(n-1)`
* Use `median(d_(n-2..n+2))` (median-of-5) to reject single late ticks

Then you can optionally run a “frequency assist” (FLL) that nudges `T` toward `d_n`:

* `T += Kf * (d_med - T)`

This makes lock faster and more robust than pure phase control.

A good “hybrid” tempo PLL is:

* **FLL term** handles tempo changes cleanly
* **PLL phase term** eliminates drift

---

## Step 5 — Decide what you output to Element: “tempo estimate” only

Since you’re only doing tempo sync, you can output a smoothed BPM:

* `bpm_raw = sampleRate * 60 / (T * 24)`
* then optionally smooth BPM with a light 1-pole lowpass (time constant 50–150ms)

But honestly, if your `T` is stable, you may not need additional smoothing.

---

## Step 6 — Lock detection and dropout handling (tempo-only)

You don’t need transport state, but you do need “is external tempo valid?”

### Lock criteria

Maintain a running statistic of `|e|`:

* locked if median(|e| over last 12 ticks) < ~`0.1 * T`
  (tune this; start with 10% of a tick period)

### Dropout

If no tick for > `2.5 * T`:

* hold last tempo (keep `T`)
* switch to acquire mode when ticks resume

---

## Step 7 — How to migrate safely (no regressions)

Do this in stages:

1. **Run PLL in parallel** with existing DLL (no behavior change yet)

   * log: DLL tempo vs PLL tempo
   * log: tick error stats

2. **A/B switch** behind a compile flag or runtime option

3. Tune:

   * Kf (interval follower)
   * Kp/Ki (phase controller)
   * clamp thresholds and lock detection

---

## Suggested starting gains (rule-of-thumb)

These depend on sampleRate and typical tempo range, so treat as starting points:

* Use normalized error: `en = e / T` (dimensionless, -1..+1-ish)
* Then update `T` using normalized control:

Example:

* `I += Ki * en`
* `T *= (1 + Kp * en + I)`

This makes tuning less sample-rate-dependent.

Starting points:

* Acquire: `Kp = 0.02`, `Ki = 0.001`, `Kf = 0.1`
* Locked: `Kp = 0.005`, `Ki = 0.0002`, `Kf = 0.02`

And clamp:

* `en` clamp to ±0.5

These aren’t magic—just sane “won’t explode” defaults.

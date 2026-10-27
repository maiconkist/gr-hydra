# gr-hydra

[![Build Status](https://travis-ci.org/maiconkist/gr-hydra.svg?branch=bleeding)](https://travis-ci.org/maiconkist/gr-hydra)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## GETTING STARTED

### REQUIRED LIBS

* build-essential
* cmake
* libfftw3-dev
* uhd-host
* libuhd-dev
* gnuradio-dev
* libopencv-dev
* libzmqpp-dev
* swig

```
sudo apt install build-essential cmake libfftw3-dev uhd-host libuhd-dev gnuradio-dev libopencv-dev libzmqpp-dev swig
```

## INSTALLATION

#### STANDARD VERSION - NO GNURADIO SUPPORT

Clone the repository and enter in the created folder (default: gr-hydra). Execute the following commands:

```
mkdir build; cd build;
cmake ../
make
sudo make install
sudo ldconfig
```

#### WITH GNURADIO SUPPORT

First install the STANDARD VERSION. Go to gr-hydra/grc_blocks and execute the following commands:

```
mkdir build; cd build;
cmake ../
make
sudo make install
```

## RUNNING THE EXAMPLES

### NOMENCLATURE

For all examples we:

- We associate a terminal ID to a all terminals open in the form **T_n**, where **n** is the terminal number. Wherewer needed we are going to refer to a specific terminal by specifying its ID.

- refer to **USRP_n** as a pair of **PC+USRP**, where **n** indentifies one of the pair. Ex: USRP_1 is what you consider your 1st PC+USRP, while **USRP_2** is your second. Whenever we refer to **USRP_1** you execute the instructions in that machine. 


### EXAMPLE 1
#### HARDWARE REQUIREMENTS: 2x [USRP] -- IDEAL: 3x [USRP]


1. Open a terminal in USRP_1 (**T1**). Start the Hydra-Server flowgraph:
```
python grc_blocks/app/hydra_gr_server_example.py
```
2. Open a terminal in USRP_1 (**T2**). Start the client that request 2 end-to-end TX/RX slices:
```
python grc_blocks/app/hydra_gr_client_2tx_2rx_pdu_gen.py
```
3. Open a terminal in USRP_2 (**T3**). Start the flowgraph that receives the messages (PDU) transmitted in slice 1:
```
python grc_blocks/app/usrp_vr1_pdu_gen.grc
```
4. In **T3** you should see the log of messages being received from USRP_1: 
```
LOG
LOG
LOG
LOG
```
5. In **T2** you should see the log of messages being received from USRP_2:
```
LOG
LOG
LOG
LOG
```
6. If you have only 2 USRP: stop the flowgraph in **T3**. Otherwise go to next step.
7.  Open a terminal (**T4**) in USRP_3 (or USRP_2 if you have only 2 USRPs).  Start the flowgraph that receives the messages (PDU) transmitted in slice 2:
```
python grc_blocks/app/usrp_vr2_pdu_gen.grc
```
8. In **T2** you should see the log of messages being received from **USRP_3** (or **USRP_2**):
```
LOG
LOG
LOG
LOG
```
9. In **T4** you should see the log of messages being received from USRP_1:
```
LOG
LOG
LOG
LOG
```


### EXAMPLE 2
#### HARDWARE REQUIREMENTS: 


## LICENSE

## ACKNOWLEDGEMENTS

## TROUBLESHOOTING

* Cmake cannot find FFTW ? 

   Put the file in https://github.com/jedbrown/cmake-modules/blob/master/FindFFTW.cmake in the folder /usr/share/cmake-2.8/Modules/

## BUGS AND IMPROVEMENT BACKLOG

The following items come from a review of the current implementation. This is
not an exhaustive bug inventory. Optimization ideas are candidates for profiling,
not measured performance improvements.

### Bug status

Resolved items are marked **Fixed**; unmarked entries remain open.

- **Fixed: invalid iterators when releasing spectrum.**
  `rf_front_end::delete_chunks` now checks both list boundaries before
  inspecting neighbors. The `hydra_resource_release` regression cases cover
  full-band, boundary, interior, and adjacent allocations, including preservation
  of occupied neighbors and coalescing of free chunks.
  Source: `lib/hydra_resource.cc`.

- **Fixed: repeated resource release can crash the server.**
  `Hypervisor::detach_virtual_radio` now returns `false` for a missing ID instead
  of dereferencing a null radio. The `hydra_repeated_release` regression covers
  unknown IDs and detaching the same radio more than once.
  Sources: `lib/hydra_hypervisor.cc`, `lib/hydra_core.cc`,
  `lib/hydra_client.cc`, `grc_blocks/lib/hydra_gr_client_*_impl.cc`.

- **Retuning can report success after losing the original allocation.**
  `rf_front_end::check_chunk` checks frequency bounds but not whether a chunk is
  occupied. `HydraCore` then frees the old allocation and ignores a failed
  replacement reservation; `Hypervisor::notify` also reports success when
  mapping fails. Reservation and DSP-map updates need a consistent commit or
  rollback.
  Sources: `lib/hydra_resource.cc`, `lib/hydra_core.cc`,
  `lib/hydra_hypervisor.cc`.

- **Bandwidth changes leave incompatible DSP state.** Updating a radio's
  bandwidth changes its mapping and FFT length, but does not recreate its FFT
  plans, framing configuration, or buffer capacities. Increasing bandwidth can
  make processing access beyond the original sample windows or FFT buffers.
  Sources: `lib/hydra_virtual_radio.cc`, `lib/hydra_hypervisor.cc`,
  `include/hydra/hydra_resampler.hpp`.

- **Malformed or empty control JSON is not handled safely.** The server
  continues to `root.front()` after a JSON parse failure, and an empty object
  also has no first entry. Invalid requests should receive an error without
  entering command dispatch.
  Source: `lib/hydra_server.cc`.

- **Backend receive results are incomplete.** The default UHD receive path,
  image backend, and file backend can reach the end of a non-void function
  without returning a sample count. `Hypervisor::rx_run` also treats the result
  as a boolean and forwards a full FFT window regardless of the actual count,
  potentially processing stale samples after short reads.
  Sources: `lib/hydra_uhd_interface.cc`, `lib/hydra_hypervisor.cc`.

- **Fixed: unsafe single-direction state and cleanup.** Core and hypervisor
  direction flags start disabled. Freeing an unconfigured direction is a
  successful no-op rather than a process exit. Regressions cover RX-only and
  TX-only resource managers, plus cleanup of an unconfigured core.
  Sources: `lib/hydra_core.cc`, `lib/hydra_hypervisor.cc`,
  `lib/hydra_resource.cc`.

- **Worker state is not consistently synchronized.** Stop flags are ordinary
  booleans shared across threads, and radio-vector and mapping accesses do not
  consistently use the same synchronization. Allocation, retuning, streaming,
  and shutdown can therefore race.
  Sources: `include/hydra/hydra_hypervisor.h`,
  `include/hydra/hydra_resampler.hpp`, `lib/hydra_hypervisor.cc`,
  `lib/hydra_virtual_radio.cc`, `lib/hydra_socket.cc`.

- **FFT and worker lifetimes lack complete cleanup.** `fft_complex` allocates
  FFTW buffers and a plan but has no destructor to release them. Hypervisor
  shutdown joins the front-end threads without stopping every virtual radio's
  socket and framing threads. Resource ownership should guarantee orderly
  stop, join, and release.
  Sources: `include/hydra/hydra_fft.h`, `lib/hydra_fft.cc`,
  `include/hydra/hydra_hypervisor.h`, `include/hydra/hydra_virtual_radio.h`.

- **Allocation responses have inconsistent success semantics.** TX response
  parsing returns success (`0`) on malformed JSON, leaving endpoint fields
  unset. Reconfiguring an existing radio returns `1`, which the server publishes
  as its data port instead of returning the existing port.
  Sources: `lib/hydra_client.cc`, `lib/hydra_core.cc`,
  `lib/hydra_server.cc`.

- **Sub-bin bandwidth requests can produce zero-length FFTs.** A positive
  bandwidth smaller than one front-end FFT bin truncates to zero samples.
  Validate the requested bandwidth and resulting FFT length before allocating
  a radio or starting workers.
  Sources: `lib/hydra_virtual_radio.cc`, `lib/hydra_hypervisor.cc`,
  `lib/hydra_fft.cc`.

- **Fixed: the USRP backend ignored the requested RX gain.**
  `device_uhd::set_rx_config` forwards the supplied normalized gain to UHD
  instead of forcing `0.0`, preserving the existing range validation.
  Source: `lib/hydra_uhd_interface.cc`.

### Possible optimizations

- **Replace short polling loops with explicit wakeups.** Profile blocking
  queues or condition-variable notifications instead of repeated one-microsecond
  sleeps in socket and framing workers. Measure CPU use and latency together.
  Sources: `include/hydra/hydra_resampler.hpp`, `lib/hydra_socket.cc`.

- **Reduce hot-path copies and allocations.** Reuse IQ windows and message
  storage, and consider moving buffers where ownership allows it. Buffer reads,
  per-radio FFT input, wideband output, and ZeroMQ messages currently involve
  repeated copies.
  Sources: `include/hydra/hydra_buffer.hpp`, `lib/hydra_virtual_radio.cc`,
  `lib/hydra_hypervisor.cc`, `lib/hydra_socket.cc`.

- **Reduce FFT memory and setup overhead.** Allocate using the single-precision
  complex size rather than `sizeof(fftw_complex)` for float IQ buffers. After
  fixing ownership, investigate FFTW wisdom or reusable planning information
  for repeated radio creation instead of repeating `FFTW_MEASURE` planning.
  Source: `lib/hydra_fft.cc`.

- **Remove or reconnect the idle RX framing stage.** `demap_iq_samples` writes
  directly to the RX resampler's output buffer, while its worker waits on the
  unused `rx_windows` input. A direct output buffer would avoid an unnecessary
  polling thread unless a real conversion stage is introduced.
  Sources: `lib/hydra_virtual_radio.cc`,
  `include/hydra/hydra_resampler.hpp`.

- **Reduce coarse-lock contention after fixing races.** Both streaming paths
  hold the radio-list mutex while processing individual radios. Profile stable
  snapshots or per-radio ownership that permit DSP work outside that global
  critical section.
  Source: `lib/hydra_hypervisor.cc`.

- **Bound and pace emulated streaming.** The loopback backend queues windows
  without a capacity or sample-rate limit. Evaluate bounded queues,
  backpressure, and pacing so emulation does not consume all available CPU or
  accumulate unbounded data.
  Source: `lib/hydra_uhd_interface.cc`.

### Other improvements

- **Add meaningful regression coverage and CI execution.** The three GNU Radio
  QA files currently run empty flowgraphs without assertions. Cover allocation
  boundaries, conflicting reservations, repeated release, malformed messages,
  bandwidth changes, FFT round trips, and short backend reads. Exercise both
  build paths and add a headless loopback integration scenario.
  Sources: `grc_blocks/python/qa_*.py`, `.travis.yml`.

- **Unify core build ownership and dependency declarations.** The standalone
  build uses C++17, while the GNU Radio build uses C++14 and recompiles the core
  sources into a separate library. Share a core build target, make required
  dependencies and include paths explicit, and keep installation instructions
  consistent with the CMake requirements.
  Sources: `CMakeLists.txt`, `lib/CMakeLists.txt`,
  `grc_blocks/CMakeLists.txt`, `grc_blocks/lib/CMakeLists.txt`.

- **Define a supported GNU Radio and Python baseline.** The integration uses
  GNU Radio 3.7-era APIs, SWIG, XML block definitions, and Python 2 examples.
  Choose and document the supported toolchain before updating bindings and
  generated flowgraphs.
  Sources: `grc_blocks/CMakeLists.txt`, `grc_blocks/swig/`,
  `grc_blocks/grc/`, `grc_blocks/app/`.

- **Document the actual protocol and radio identity model.** Control uses
  `xvl_*` JSON messages over ZeroMQ REQ/REP; active IQ streaming uses raw
  `complex<float>` samples over ZeroMQ PUSH/PULL on TCP. The `udp_port` names are
  legacy, while UDP ports 5001/5002 serve discovery. Document endpoint ownership,
  shared TX/RX radio IDs, and the reserved free-space markers: chunk ID `0` and
  FFT-bin ID `-1`.
  Sources: `lib/hydra_server.cc`, `lib/hydra_socket.cc`,
  `include/hydra/types.h`, `lib/hydra_resource.cc`,
  `lib/hydra_hypervisor.cc`.

- **Make sample-rate and FFT-bin constraints explicit.** `resampler` currently
  only packs or unpacks sample windows; it does not convert sample rates.
  Account for integer bin-width quantization and hardware-coerced rates, expose
  the actual configuration, and either reject unsupported rates or implement
  the required conversion.
  Sources: `include/hydra/hydra_resampler.hpp`,
  `lib/hydra_hypervisor.cc`, `lib/hydra_uhd_interface.cc`.

- **Refresh runnable examples.** Several example paths above do not exist in
  this checkout. Replace them with maintained examples and document the
  standalone server's backend options. Update `iq_generator` to discover the
  server and use the active TCP/float-IQ transport rather than its older
  UDP/double-IQ implementation.
  Sources: `grc_blocks/app/`, `app/server.cc`, `app/iq_generator.cc`.

- **Expose overload and lifecycle failures.** Report rejected buffer writes,
  ZeroMQ send timeouts, backend underruns/overruns, and queue depth. The existing
  statistics path is not wired into the main runtime. Replace library-level
  process exits with explicit error results and define bounded shutdown behavior
  when the server or a client disappears.
  Sources: `include/hydra/hydra_buffer.hpp`, `lib/hydra_socket.cc`,
  `lib/hydra_stats.cc`, `lib/hydra_main.cc`, `lib/hydra_client.cc`.
